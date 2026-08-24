from pathlib import Path
import shutil

src = Path("systolic_pe_tile.sv")
dst = Path("systolic_pe_bram.sv")

shutil.copy2(src, dst)

text = dst.read_text()

old = """\
    logic [DATA_W-1:0] acc_bank [0:1][0:ACC_BANKS-1];
    logic              acc_set;
    logic              red_set;
"""

new = """\
    /*
     * 每個 logical accumulator set 保存兩份相同的 BRAM copy。
     *
     * copy0 / copy1 的內容永遠相同。
     * reduction 因此可以同時取得兩個 read port，
     * writeback 則同時寫回兩份。
     */
    (* ram_style = "block" *)
    logic [DATA_W-1:0] acc_ram0 [0:1][0:ACC_BANKS-1];

    (* ram_style = "block" *)
    logic [DATA_W-1:0] acc_ram1 [0:1][0:ACC_BANKS-1];

    /*
     * BRAM 本身不做整顆 clear。
     * valid=0 時，該 bank 在邏輯上視為 0。
     */
    logic acc_bank_valid [0:1][0:ACC_BANKS-1];

    logic acc_set;
    logic red_set;
"""

if old not in text:
    raise RuntimeError("找不到 acc_bank 宣告，停止修改")

text = text.replace(old, new)

dst.write_text(text)

print(f"產生：{dst}")
