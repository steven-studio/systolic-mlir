#!/usr/bin/env python3
"""
patch_buf_lutram.py -- 把 A_buf / B_buf 從暫存器陣列改成十六個小記憶體。

    python3 patch_buf_lutram.py hls/multi_200t/_fp_beat/rtl_fp/systolic_uart_fold_top.sv

問題
----
K_MAX=64 的 place_design 失敗:

    30575 slices available, unplaced instances require 31839 slices
    Control sets: 4227
    Luts:      106607 / 133800   (79.7%)
    Flip flops: 147360 / 267600  (55.0%)

LUT 和 FF 個別都沒滿,但每個 slice 只能容納一個 control set
(clock / reset / CE 的組合),所以 FF 沒辦法密集填進 slice,slice 先用完。

control set 的來源是 A_buf / B_buf。它們被合成成暫存器,每個 32-bit 字
帶自己的寫入致能:

    K_MAX=16 -> 8*16 + 16*8 =  256 個字
    K_MAX=64 -> 8*64 + 64*8 = 1024 個字

FF 實測差值 147360 - 122225 = 25135,對照 (1024-256)*32 = 24576,幾乎全部。

為什麼 ram_style = "distributed" 沒用
-------------------------------------
    WARNING: [Synth 8-7186] Applying attribute ram_style = "distributed"
    is ignored, object 'A_buf[0][0]' is not inferred as ram due to
    incorrect usage

二維陣列的寫入位址橫跨兩個維度(A_buf[rx_row][{rx_win, rx_col}]),
Vivado 看不到「一個記憶體、一個寫入位址」的形狀,整個 attribute 被忽略。

作法
----
拆成十六個一維記憶體:A 依 row 拆 8 個,B 依 col 拆 8 個。

    A_MEM[r]:  寫入位址 {rx_win, rx_col},致能 = (寫 A 且 rx_row == r)
               讀取位址 gk_a = feed_t - r
    B_MEM[c]:  寫入位址 {rx_win, rx_row},致能 = (寫 B 且 rx_col == c)
               讀取位址 gk_b = feed_t - c

每個記憶體一個寫入位址、一個讀取位址,正是 distributed RAM 的形狀。
餵入端本來就是每個 row 只讀一個位址,所以讀取仍是非同步的,不增加延遲,
週期公式 K+118 不變。

control set 從 1024 降到 16。FF 少 32768 個,換成約 512 個 LUT
(RAM64X1D 一顆 LUT 存 64x1,32 bit x 16 個記憶體)。

冪等。原檔備份到 <file>.bak_lutram。
"""

import sys
import os
import shutil

# ---------------------------------------------------------------- 1. 宣告

DECL_OLD_PLAIN = """    logic [31:0] A_buf [0:7][0:K_MAX-1];
    logic [31:0] B_buf [0:K_MAX-1][0:7];
"""

DECL_OLD_ATTR = """    (* ram_style = "distributed" *)
    logic [31:0] A_buf [0:7][0:K_MAX-1];

    (* ram_style = "distributed" *)
    logic [31:0] B_buf [0:K_MAX-1][0:7];
"""

DECL_NEW = """    /*
     * Held as sixteen small memories rather than one 2-D register
     * array: eight for A indexed by row, eight for B indexed by
     * column. The memories themselves are generated further down,
     * after the RX control signals they depend on exist.
     *
     * As a 2-D array these cannot be inferred as RAM at all. The
     * write address spans both dimensions, so there is no single
     * write address for the tool to recognise and it falls back to
     * flip-flops -- one write enable per 32-bit word. Control sets
     * then scale with K_MAX (256 at K_MAX=16, 1024 at K_MAX=64), and
     * since a slice holds exactly one control set the K_MAX=64 build
     * needed 31839 slices out of 30575 while LUTs sat at 80% and
     * flip-flops at 55%.
     *
     * Split per row and per column, each memory has one write address
     * and one read address. The feeder reads row r at gk = feed_t - r
     * and column c at gk = feed_t - c, so one read address per memory
     * per cycle is all it ever needs and the read stays asynchronous:
     * no extra feed latency, and the K+118 cycle formula is unchanged.
     */
    logic [K_W-1:0] a_raddr [0:7];
    logic [K_W-1:0] b_raddr [0:7];

    wire [31:0] a_rdata [0:7];
    wire [31:0] b_rdata [0:7];
"""


# ------------------------------------------------------- 2. 記憶體 generate

MEM_ANCHOR = """    always_ff @(posedge clk) begin
        if (rst) begin

            rx_state       <= RX_HUNT;
"""

MEM_NEW = """    /*
     * ============================================================
     * Operand memories
     * ============================================================
     *
     * One write port and one asynchronous read port each, which is
     * the shape distributed RAM wants. Writes are decoded here rather
     * than inside the RX state machine so that each memory sees a
     * single write address.
     * ============================================================
     */
    wire buf_wr =
        rx_valid &&
        (rx_state == RX_BODY) &&
        (byte_pos == 2'd3) &&
        hdr_done;

    wire [31:0] buf_wdata = {rx_byte, word_buf[23:0]};

    // Absolute k of the word being written. Each k window is exactly
    // 8 deep, so the concatenation is window*8 + offset with no adder.
    wire [K_W-1:0] a_waddr = {rx_win, rx_col};
    wire [K_W-1:0] b_waddr = {rx_win, rx_row};

    genvar gi;

    generate

        for (gi = 0; gi < 8; gi = gi + 1) begin : A_MEM

            (* ram_style = "distributed" *)
            logic [31:0] mem [0:K_MAX-1];

            always_ff @(posedge clk) begin
                if (buf_wr && !rx_is_b && rx_row == 3'(unsigned'(gi)))
                    mem[a_waddr] <= buf_wdata;
            end

            assign a_rdata[gi] = mem[a_raddr[gi]];

        end


        for (gi = 0; gi < 8; gi = gi + 1) begin : B_MEM

            (* ram_style = "distributed" *)
            logic [31:0] mem [0:K_MAX-1];

            always_ff @(posedge clk) begin
                if (buf_wr && rx_is_b && rx_col == 3'(unsigned'(gi)))
                    mem[b_waddr] <= buf_wdata;
            end

            assign b_rdata[gi] = mem[b_raddr[gi]];

        end

    endgenerate


""" + MEM_ANCHOR


# ------------------------------------------------------------- 3. 寫入路徑

WRITE_OLD = """                            /*
                             * Absolute k is {window, col} for A and
                             * {window, row} for B -- each k window is
                             * exactly 8 deep, so the concatenation is
                             * window*8 + offset with no adder.
                             *
                             * Payload is written optimistically,
                             * before the end marker has been seen. A
                             * frame that turns out to be spurious
                             * leaves stale operands behind, but they
                             * are overwritten by the next accepted
                             * frame before anything reads them --
                             * which is why no 1036-byte holding
                             * buffer is needed.
                             */
                            else if (rx_is_b) begin

                                B_buf[{rx_win, rx_row}]
                                     [rx_col]
                                    <= {
                                        rx_byte,
                                        word_buf[23:0]
                                    };

                            end
                            else begin

                                A_buf[rx_row]
                                     [{rx_win, rx_col}]
                                    <= {
                                        rx_byte,
                                        word_buf[23:0]
                                    };

                            end
"""

WRITE_NEW = """                            /*
                             * The operand write itself happens in the
                             * per-row and per-column memories above,
                             * gated by buf_wr. Keeping it out of this
                             * state machine is what gives each memory
                             * a single write address, without which
                             * the arrays cannot be inferred as RAM.
                             *
                             * Payload is written optimistically,
                             * before the end marker has been seen. A
                             * frame that turns out to be spurious
                             * leaves stale operands behind, but they
                             * are overwritten by the next accepted
                             * frame before anything reads them --
                             * which is why no 1036-byte holding
                             * buffer is needed.
                             */
"""


# ------------------------------------------------------------- 4. 讀取路徑

READ_A_OLD = """                    a_in[r] =
                        A_buf[r][gk_a];
"""

READ_A_NEW = """                    a_raddr[r] = K_W'(unsigned'(gk_a));
                    a_in[r]    = a_rdata[r];
"""

READ_B_OLD = """                    b_in[c] =
                        B_buf[gk_b][c];
"""

READ_B_NEW = """                    b_raddr[c] = K_W'(unsigned'(gk_b));
                    b_in[c]    = b_rdata[c];
"""

# 讀取位址要有預設值,否則 always_comb 會推出latch
RADDR_DEFAULT_OLD = """        for (int i = 0; i < 8; i++) begin

            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;
"""

RADDR_DEFAULT_NEW = """        for (int i = 0; i < 8; i++) begin

            a_raddr[i]       = '0;
            b_raddr[i]       = '0;

            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;
"""


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    path = sys.argv[1]
    if not os.path.isfile(path):
        print(f"ERROR: 找不到 {path}")
        return 1

    src = open(path, encoding="utf-8").read()

    if "A_MEM" in src:
        print("已經套用過(檔案裡已有 A_MEM),不做任何事。")
        return 0

    if "rx_state" not in src or "RX_BODY" not in src:
        print("ERROR: 這份檔案還沒套 patch_rx_framing.py。先做那個。")
        return 1

    # 宣告可能是原始的、也可能已加了 ram_style attribute
    if src.count(DECL_OLD_ATTR) == 1:
        decl_old = DECL_OLD_ATTR
    elif src.count(DECL_OLD_PLAIN) == 1:
        decl_old = DECL_OLD_PLAIN
    else:
        print("ERROR: 找不到 A_buf / B_buf 的宣告(或出現多次)。")
        return 1

    sites = [
        ("宣告", decl_old, DECL_NEW),
        ("記憶體 generate", MEM_ANCHOR, MEM_NEW),
        ("移除 FSM 內的寫入", WRITE_OLD, WRITE_NEW),
        ("讀取位址預設值", RADDR_DEFAULT_OLD, RADDR_DEFAULT_NEW),
        ("A 讀取", READ_A_OLD, READ_A_NEW),
        ("B 讀取", READ_B_OLD, READ_B_NEW),
    ]

    problems = []
    for name, old, _ in sites:
        n = src.count(old)
        if n != 1:
            problems.append(f"  {name}: 出現 {n} 次(需要剛好 1 次)")

    if problems:
        print("ERROR: 錨點對不上,沒有做任何修改。")
        print("\n".join(problems))
        return 1

    for _, old, new in sites:
        src = src.replace(old, new, 1)

    bak = path + ".bak_lutram"
    if not os.path.exists(bak):
        shutil.copy2(path, bak)

    open(path, "w", encoding="utf-8").write(src)

    print(f"已修改 {path}")
    print(f"備份    {bak}")
    print()
    for name, _, _ in sites:
        print(f"  ok  {name}")
    print()
    print("K_MAX=64 的預期變化:")
    print("  control sets  1024 -> 16   (A/B_buf 貢獻的部分)")
    print("  FF            -32768")
    print("  LUT as Memory +512 左右")
    print("  週期公式      K+118 不變(讀取仍是非同步)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
