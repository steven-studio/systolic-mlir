#!/usr/bin/env bash
# commit_today.sh -- 把 2026-08-15 的改動分成邏輯上獨立的 commit。
#
#   cd ~/work/systolic-mlir
#   bash eval/scalesim/commit_today.sh
#
# 只 add 明確列出的路徑,不用 git add -A。每個 commit 前會印出即將提交的
# 檔案,任何一步失敗就停下來。
#
# 執行後請自己再看一次 git log -p,確認內容符合預期再 push。

set -euo pipefail

cd "$(git rev-parse --show-toplevel)"

RTL=hls/multi_200t/_fp_beat/rtl_fp

say() { echo; echo "=== $* ==="; }

commit_if_any() {
    local msg="$1"; shift
    local found=()
    for f in "$@"; do
        [ -e "$f" ] && found+=("$f")
    done
    if [ ${#found[@]} -eq 0 ]; then
        echo "  (略過:檔案都不存在)"
        return
    fi
    git add -- "${found[@]}"
    if git diff --cached --quiet; then
        echo "  (略過:沒有變更)"
        return
    fi
    git diff --cached --name-status | sed 's/^/    /'
    git commit -q -m "$msg"
    echo "  -> $(git log -1 --oneline)"
}


# ---------------------------------------------------------------------------
say "0. 先把建置產物與備份檔擋掉"

touch .gitignore
for pat in \
    'build_kmax/' \
    'rtl_fp_pe_test/' \
    'ip_local/' \
    '*.bit' \
    '*.dcp' \
    '*.jou' \
    '*.log' \
    '.Xil/' \
    '*.bak' \
    '*.bak_*' \
    '*.sv.tree'
do
    grep -qxF "$pat" .gitignore || echo "$pat" >> .gitignore
done

git add .gitignore
if ! git diff --cached --quiet; then
    git commit -q -m "chore: 忽略建置產物、bitstream 與修補備份檔

build_kmax/ 與 rtl_fp_pe_test/ 是本機 Vivado 產物,單次建置就上百 MB,
而且與機器綁定(IP 的 out-of-context 合成結果含 part 資訊),不該進版控。
*.bak_* 是本日各支 patch_*.py 留下的原檔備份,同理。"
    echo "  -> $(git log -1 --oneline)"
else
    echo "  (.gitignore 已是最新)"
fi


# ---------------------------------------------------------------------------
say "1. PE:歸約讀取位址的加法器移出組合路徑"

commit_if_any "perf(fold): 歸約讀取位址改為維護 reduce_j,加法器移出組合路徑

樹狀歸約的第二個運算元原本是

    fp_add_b = acc_bank[reduce_ctx][reduce_i + reduce_stride];

reduce_i + reduce_stride 是組合邏輯,結果要先算出來才能開始解 32:1
mux 的 select,整條路徑是

    FF -> 4-bit 加法器 -> select 解碼 -> mux -> FP IP 輸入

改為新增 reduce_j 暫存器,與 reduce_i 在同一個 always_ff 裡同步維護,
恆等於 reduce_i + reduce_stride,mux 直接吃它。每 PE 多 ACC_SEL_W 個
FF,週期數完全不變(xsim 四點 drain 仍為 111,errors=0)。

reduce_j 是冗餘狀態,走鐘的話歸約會安靜地讀錯 bank,所以附一條
\`ifndef SYNTHESIS 的 assertion 每拍檢查,合成時不產生邏輯。

順帶移除 reduce_level_last -- 宣告了但從未使用。" \
    "$RTL/systolic_pe_fold.sv"


# ---------------------------------------------------------------------------
say "2. XDC:UART 載入路徑改為多週期"

commit_if_any "perf(timing): UART 載入路徑改為多週期,回收時序餘裕

post-route 的關鍵路徑是 word_buf_reg -> A_buf_reg,logic levels = 0、
route 佔 95%,純粹是扇出與繞線問題:word_buf 的每個 bit 要驅動 256 顆
FF 的 D 腳(A_buf 8xK_MAX + B_buf K_MAXx8)。

這條路徑不需要單週期。word_buf[23:0] 在 byte_pos 0/1/2 被寫入,直到
byte_pos==3 才被讀去寫進運算元緩衝,中間隔了完整一個 byte 時間 --
115200 baud 下是 86.8 us,等於 8681 個 100 MHz 時脈。單週期是工具的
預設,不是設計的需求。

保守取 4 拍。setup N 必須配 hold N-1,否則工具會反過來收緊 hold 檢查。

效果(K_MAX=16):WNS +0.099 -> +0.802,LUT 94231 -> 86652。LUT 會少是
因為工具原本為了追這條路徑做了大量暫存器與 LUT 複製(報告裡看得到
word_buf_reg[22]_rep),約束下去之後那些複製就不必要了。" \
    "$RTL/nexys_video_uart.xdc"


# ---------------------------------------------------------------------------
say "3. build:加入 CYCLE_COUNTER generic"

commit_if_any "build: 合成時開啟 CYCLE_COUNTER

讓 bitstream 在 512 個結果位元組之後附加 4 bytes 的硬體週期計數,
host 才能拿實機拍數與 xsim 對照。" \
    "$RTL/build_kmax.tcl"


# ---------------------------------------------------------------------------
say "4. UART top:顯式 frame 邊界 + 運算元緩衝改為分散式記憶體"

commit_if_any "feat(uart): 顯式 START/END frame 邊界,運算元緩衝改為分散式記憶體

兩個改動都在同一個檔案,分不開提交,但解決的是兩個獨立問題。

一、frame 邊界
-------------
交易原本是固定長度的連續 burst,沒有分隔符,也沒有回報錯誤的通道。
主機送錯長度一次,rx_count / byte_pos / hdr_done 就停在交易中間,之後
每一筆請求都被切成兩半跨在兩筆交易上,而且錯誤會延續到手動 reset。

實例:板子是 K_MAX=16(一筆 1028 bytes),主機誤送 K_MAX=64 的 4100
bytes。硬體吃掉 3 筆完整交易,第 4 筆只收到 1016,還差 12。之後每次送
1028,前 12 補完殘缺交易然後回傳結果,剩 1016 又開始新的殘缺交易 --
殘量恆為 12,永遠對不回來。

改為

    FRAME_START(4) | HDR(4) | PAYLOAD(K_MAX*64) | FRAME_END(4)

RX_HUNT 用 32-bit 滑動視窗比對 START,卡在任何偏移都能自己走回邊界;
RX_TAIL 驗證 END,對了才 matrices_ready,不對就整筆丟棄回 HUNT。

payload 是任意 float32,任何 byte 值都可能出現,所以 START 有機會誤
觸發 -- 但那筆的 END 必定對不上,最多兩個 frame 內收斂,因此不需要
byte-stuffing。payload 採樂觀寫入,誤觸發留下的垃圾在被讀之前一定會
被下一個通過驗證的 frame 蓋掉,所以不需要 1036 bytes 的緩衝區。

刻意不用「byte 之間的間隔」當邊界:一筆交易在線上要 89 ms,主機只要
被排程器踢掉或觸發一次 GC 就會出現間隔,那會把合法的請求攔腰砍斷 --
從「送錯才錯」變成「送對也可能錯」。正確性不該押在 OS 排程行為上。

二、運算元緩衝
-------------
A_buf / B_buf 原本是二維暫存器陣列,每個 32-bit 字帶自己的寫入致能,
control set 隨 K_MAX 線性成長(K_MAX=16 為 256,K_MAX=64 為 1024)。
一個 slice 只能容納一個 control set,於是 K_MAX=64 的 place_design
失敗:

    30575 slices available, unplaced instances require 31839 slices
    Luts:      106607 / 133800  (79.7%)
    Flip flops: 147360 / 267600 (55.0%)

LUT 和 FF 個別都沒滿,slice 卻先用完。

ram_style = \"distributed\" 無效,因為寫入位址橫跨兩個維度
(A_buf[rx_row][{rx_win, rx_col}]),Vivado 看不到單一寫入位址的形狀,
attribute 被整個忽略。

改為十六個一維記憶體:A 依 row 拆 8 個、B 依 col 拆 8 個。位址完全
沒變,只是 row/col 從陣列索引變成「選哪一個記憶體」,於是每個記憶體
剛好一個寫入位址、一個讀取位址。餵入端本來就是每個 row 只讀
gk = feed_t - r 一個位址,所以讀取仍是非同步的,K+118 的週期公式不變。

K_MAX=64 實測:LUT 106607 -> 84514,FF 147360 -> 113939(-32589,對照
1024 字 x 32 bit = 32768),WNS +0.194,timing met。容量比原本的
K_MAX=16 大四倍,LUT 與 FF 反而都更少。" \
    "$RTL/systolic_uart_fold_top.sv"


# ---------------------------------------------------------------------------
say "5. host:frame 標記與週期讀取"

commit_if_any "test(uart): host 送出帶標記的 frame,並讀回硬體週期計數

request 從 HDR+payload 變成 START+HDR+payload+END,標記以小端序寫入,
與 header 中 k_dim 的慣例、以及 RTL 滑動視窗的位元組順序一致。

同時在 512 個結果位元組之後多讀 4 bytes 的硬體週期計數,與 xsim 的
k_dim + 118 對照。" \
    "$RTL/test_uart_kmax.py" \
    "$RTL/patch_kmax.py"


# ---------------------------------------------------------------------------
say "6. 工具與驗證資料"

commit_if_any "test: 本日改動所用的修補與驗證工具

各支 patch_*.py 都是冪等的、會驗證錨點、對不上就拒絕修改並保留原檔,
留著是為了讓這些改動可追溯:每一支的 docstring 記錄了當時的問題、
量測到的數字、以及為什麼選這個作法而不是別的。

patch_rx_resync.py 是被否決的方案(用 byte 間隔當 frame 邊界),
刻意保留並在檔頭寫明否決理由。" \
    eval/scalesim


# ---------------------------------------------------------------------------
say "完成"
git log --oneline -8
echo
echo "還沒被追蹤的檔案(自己確認要不要留):"
git status --porcelain | grep '^??' || echo "  無"
