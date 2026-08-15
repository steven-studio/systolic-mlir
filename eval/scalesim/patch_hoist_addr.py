#!/usr/bin/env python3
"""
patch_hoist_addr.py -- 把 reduce 讀取位址的加法器移出組合路徑。

    python3 patch_hoist_addr.py hls/multi_200t/_fp_beat/rtl_fp/systolic_pe_fold.sv

做什麼
------
樹狀歸約目前的第二個運算元是

    fp_add_b = acc_bank[reduce_ctx][reduce_i + reduce_stride];

`reduce_i + reduce_stride` 是組合邏輯,它的結果要先算出來才能開始解
32:1 mux 的 select。整條關鍵路徑是

    FF(reduce_i) -> 4-bit 加法器 -> select 解碼 -> 32:1 mux -> FP IP 輸入

這支腳本新增一個暫存器 reduce_j,與 reduce_i 在同一個 always_ff 裡同步
維護,恆等於 reduce_i + reduce_stride。mux 直接吃 reduce_j,加法器從
關鍵路徑上消失,剩下

    FF(reduce_j) -> select 解碼 -> 32:1 mux -> FP IP 輸入

代價:每 PE 多 ACC_SEL_W 個 FF(4 個),週期數完全不變。

為什麼不是「運算元打一級暫存」
------------------------------
那顆 mux 已經在兩個暫存器之間(acc_bank 的 FF 到 FP IP 的輸入暫存器)。
在中間插一個 red_a_q 只是把終點換一顆 FF,mux 一層都沒少,路徑長度不變,
卻多花 64 bit x 64 PE = 4096 個 FF 和每層一拍。要縮短 FF->邏輯->FF 的
路徑必須切開邏輯本身,搬動終點沒有用。

安全網
------
`ifndef SYNTHESIS 區塊裡有一條 assertion,只要 reduce_j 與
reduce_i + reduce_stride 不一致就在模擬時報錯。合成時不會產生邏輯。

冪等,可重複執行。原檔備份到 <file>.bak_hoist。
"""

import sys
import os
import shutil

DECL_ANCHOR = """    logic [ACC_SEL_W-1:0]
        reduce_i;
"""

DECL_NEW = """    logic [ACC_SEL_W-1:0]
        reduce_i;

    /*
     * reduce_j is maintained to satisfy, at every point where the
     * reduction path reads a bank:
     *
     *     reduce_j == reduce_i + reduce_stride
     *
     * Keeping it as state instead of recomputing it combinationally
     * takes the 4-bit adder off the path that feeds the bank mux
     * select. Cost is ACC_SEL_W flops per PE; cycle count is
     * unchanged.
     */
    logic [ACC_SEL_W-1:0]
        reduce_j;
"""

# (描述, 舊文字, 新文字)
SITES = [
    (
        "宣告 reduce_j",
        DECL_ANCHOR,
        DECL_NEW,
    ),
    (
        "移除死碼 reduce_level_last",
        """
    /*
     * Last issue of the current stride: second context, last index.
     */
    wire reduce_level_last =
        reduce_issue_fire &&
        (reduce_ctx == 1'b1) &&
        (reduce_i == reduce_stride - 1'b1);

""",
        "\n",
    ),
    (
        "mux 改吃 reduce_j",
        """            fp_add_b =
                acc_bank
                    [reduce_ctx]
                    [reduce_i + reduce_stride];
""",
        """            fp_add_b =
                acc_bank
                    [reduce_ctx]
                    [reduce_j];
""",
    ),
    (
        "reset",
        """            reduce_i <=
                '0;

            reduce_outstanding <=
""",
        """            reduce_i <=
                '0;

            reduce_j <=
                '0;

            reduce_outstanding <=
""",
    ),
    (
        "PE_ACCUM -> ISSUE(最寬的一層)",
        """                        reduce_stride <=
                            ACC_SEL_W'(ACC_BANKS / 2);

                        reduce_ctx <=
                            1'b0;

                        reduce_i <=
                            '0;
""",
        """                        reduce_stride <=
                            ACC_SEL_W'(ACC_BANKS / 2);

                        reduce_ctx <=
                            1'b0;

                        reduce_i <=
                            '0;

                        reduce_j <=
                            ACC_SEL_W'(ACC_BANKS / 2);
""",
    ),
    (
        "ISSUE ctx0 -> ctx1",
        """                            reduce_ctx <=
                                1'b1;

                            reduce_i <=
                                '0;
""",
        """                            reduce_ctx <=
                                1'b1;

                            reduce_i <=
                                '0;

                            reduce_j <=
                                reduce_stride;
""",
    ),
    (
        "ISSUE 逐格前進",
        """                        reduce_i <=
                            reduce_i + 1'b1;
""",
        """                        reduce_i <=
                            reduce_i + 1'b1;

                        reduce_j <=
                            reduce_j + 1'b1;
""",
    ),
    (
        "DRAIN stride 折半",
        """                            reduce_stride <=
                                reduce_stride >> 1;

                            reduce_ctx <=
                                1'b0;

                            reduce_i <=
                                '0;
""",
        """                            reduce_stride <=
                                reduce_stride >> 1;

                            reduce_ctx <=
                                1'b0;

                            reduce_i <=
                                '0;

                            reduce_j <=
                                reduce_stride >> 1;
""",
    ),
]

ASSERT_ANCHOR = """    fp_add u_fp_add ("""

ASSERT_NEW = """`ifndef SYNTHESIS
    /*
     * reduce_j is redundant state. If it ever drifts from the value
     * it stands in for, the reduction silently reads the wrong bank
     * and the result is merely wrong rather than obviously broken.
     * Check it every cycle a bank is actually read.
     */
    always_ff @(posedge clk) begin
        if (!rst && state == PE_REDUCE_ISSUE) begin
            if (reduce_j !== (reduce_i + reduce_stride)) begin
                $error({"reduce_j desync: j=%0d i=%0d stride=%0d ",
                        "expected=%0d"},
                       reduce_j, reduce_i, reduce_stride,
                       (reduce_i + reduce_stride));
            end
        end
    end
`endif


    fp_add u_fp_add ("""


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    path = sys.argv[1]
    if not os.path.isfile(path):
        print(f"ERROR: 找不到 {path}")
        return 1

    src = open(path, encoding="utf-8").read()
    orig = src

    if "reduce_j" in src:
        print("已經套用過(檔案裡已有 reduce_j),不做任何事。")
        return 0

    missing = []
    for name, old, new in SITES:
        n = src.count(old)
        if n != 1:
            missing.append(f"  {name}: 出現 {n} 次(需要剛好 1 次)")
    if src.count(ASSERT_ANCHOR) != 1:
        missing.append(f"  assertion 插入點: 出現 {src.count(ASSERT_ANCHOR)} 次")

    if missing:
        print("ERROR: 錨點對不上,檔案內容與預期不同。沒有做任何修改。")
        print("\n".join(missing))
        print("\n這通常代表這支檔案不是樹狀歸約版,或已被手動改過。")
        return 1

    for name, old, new in SITES:
        src = src.replace(old, new, 1)
    src = src.replace(ASSERT_ANCHOR, ASSERT_NEW, 1)

    bak = path + ".bak_hoist"
    if not os.path.exists(bak):
        shutil.copy2(path, bak)

    open(path, "w", encoding="utf-8").write(src)

    print(f"已修改 {path}")
    print(f"備份    {bak}")
    print()
    for name, _, _ in SITES:
        print(f"  ok  {name}")
    print("  ok  加入 reduce_j desync assertion")
    print()
    print(f"行數 {orig.count(chr(10))} -> {src.count(chr(10))}")
    print()
    print("下一步:")
    print("  1. 模擬四個 K,確認 drain 仍是 111、errors=0")
    print("  2. 重跑合成,比對 WNS 有沒有從 +0.099 拉回來")
    return 0


if __name__ == "__main__":
    sys.exit(main())
