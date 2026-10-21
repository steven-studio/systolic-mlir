`timescale 1ns / 1ps

/*
 * ============================================================
 * systolic_operand_buffer -- 八 bank 的運算元緩衝
 * ============================================================
 *
 * 自 systolic_uart_tile_top 原樣抽出。原本 A 側與 B 側是兩段
 * 幾乎相同的 generate 迴圈,差別只在「誰選 bank、誰當位址」:
 *
 *     A:  bank = rx_row,  addr = {rx_win, rx_col}
 *     B:  bank = rx_col,  addr = {rx_win, rx_row}
 *
 * 那個對調就是 A/B 的轉置。抽成單一模組之後,轉置寫在實例化的
 * 埠上,讀的人不必逐行比對兩段程式碼才看得出來。
 *
 * 為什麼是八個獨立記憶體而不是一個二維陣列:二維陣列的寫入位址
 * 橫跨兩個維度,工具無法辨識成 RAM,會退化成每個 32-bit 字各自
 * 一個 write enable,control set 隨 K_MAX 線性成長,slice 放不下。
 * 每個 bank 一個寫入位址、一個讀取位址,問題就消失。
 *
 * ------------------------------------------------------------
 * 埠為 unpacked array,不是打平的向量
 * ------------------------------------------------------------
 * systolic_array_tile 與 systolic_tile_feeder 的運算元埠都已經是
 * unpacked array,Vivado 合成這個專案時一直都吃得下。打平成
 * [8*K_W-1:0] 會多出兩段 pack/unpack 的 generate,而且一旦呼叫端
 * 與這裡的 K_W 不一致,埠連接會靜默地補零或截斷 —— 位址錯位而
 * 合成無警告。用 unpacked array 時同樣的不一致是寬度不符,工具
 * 會講話。
 * ============================================================
 */
module systolic_operand_buffer #(
    parameter int K_MAX = 256,

    /* 預設由 K_MAX 導出,呼叫端漏傳也不會兩者脫鉤。
     * 舊版預設寫死 9,而 $clog2(256) 是 8 —— 那個預設本身就是錯的。
     * K_MAX 可以是任何正整數(不必是 2 的冪);K_MAX = 1 時 $clog2 是 0,
     * 位址仍需 1 bit,所以下限是 1。 */
    parameter int K_W   = (K_MAX > 1) ? $clog2(K_MAX) : 1,

    /* Bank 數 = 陣列邊長 N(每列/每行一個 bank)。這是幾何參數,
     * 與位址寬度 K_W 無關 -- 上面那段「曾寫成 K_W-1」的事故正是
     * 把兩者混為一談的後果。預設 8,既有 8x8 呼叫端不需改動。
     * 注意:fold 深度(8-deep k window)是協定常數,不隨 N_BANKS 變。 */
    parameter int N_BANKS = 8,
    parameter int SEL_W   = $clog2(N_BANKS)
)(
    input  wire clk,

    input  wire             wr,      // 寫入脈衝(A/B 判別已在呼叫端折入)
    input  wire [SEL_W-1:0] wsel,    // 寫哪一個 bank
    input  wire [K_W-1:0]   waddr,
    input  wire [31:0]      wdata,

    input  wire  [K_W-1:0] raddr [0:N_BANKS-1],
    output wire  [31:0]    rdata [0:N_BANKS-1]
);

    genvar gi;

    generate

        /* Bank 數是幾何常數 N_BANKS:陣列一邊有 N 列 / N 行,與位址
         * 寬度無關。這裡曾經寫成 K_W-1,只在 K_W == 9 時碰巧等於 8;
         * 頂層傳進來的是 $clog2(K_MAX),K_MAX=256 得 8、K_MAX=16 得 4,
         * 於是只生出 7 個或 3 個 bank。缺的 bank 其 rdata 未驅動,
         * 合成後接地,對應的列/行永遠讀回 0。 */
        for (gi = 0; gi < N_BANKS; gi = gi + 1) begin : BANK

            (* ram_style = "block" *)
            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            always_ff @(posedge clk) begin
                if (wr && wsel == SEL_W'(unsigned'(gi)))
                    mem[waddr] <= wdata;

                /* 同步讀:位址第 t 拍發出,資料第 t+1 拍有效。
                 * BRAM 沒有非同步讀取埠 -- 這一拍就是 K+118 變 K+119
                 * 的唯一來源,與 k_dim 無關。 */
                rdata_q <= mem[raddr[gi]];
            end

            assign rdata[gi] = rdata_q;

        end

    endgenerate

`ifndef SYNTHESIS
    /* 深度不是 2 的冪時,K_W 位元的位址可以表示 >= K_MAX 的值,而
     * mem[0:K_MAX-1] 沒有那些格子。呼叫端(systolic_accel_context)在寫入
     * 前就丟掉 >= K_MAX 的 slab 補零格,feeder 也只讀 k < k_dim <= K_MAX;
     * 這裡在每一次模擬裡確認兩者都成立,而不是讓越界寫入在模擬器裡
     * 被默默忽略、在硬體上落到哪裡算哪裡。 */
    always_ff @(posedge clk) begin
        if (wr && (int'(waddr) >= K_MAX))
            $fatal(1, "systolic_operand_buffer: write to depth %0d of a %0d-deep buffer",
                   waddr, K_MAX);
        for (int bi = 0; bi < N_BANKS; bi++)
            if (int'(raddr[bi]) >= K_MAX)
                $fatal(1, "systolic_operand_buffer: bank %0d reads depth %0d of a %0d-deep buffer",
                       bi, raddr[bi], K_MAX);
    end
`endif

endmodule
