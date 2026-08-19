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
     * 舊版預設寫死 9,而 $clog2(256) 是 8 —— 那個預設本身就是錯的。 */
    parameter int K_W   = $clog2(K_MAX)
)(
    input  wire clk,

    input  wire           wr,      // 寫入脈衝(A/B 判別已在呼叫端折入)
    input  wire [2:0]     wsel,    // 寫哪一個 bank
    input  wire [K_W-1:0] waddr,
    input  wire [31:0]    wdata,

    input  wire  [K_W-1:0] raddr [0:7],
    output wire  [31:0]    rdata [0:7]
);

    genvar gi;

    generate

        /* Bank 數是幾何常數 8:陣列一邊有 8 列 / 8 行,與位址寬度
         * 無關。這裡曾經寫成 K_W-1,只在 K_W == 9 時碰巧等於 8;
         * 頂層傳進來的是 $clog2(K_MAX),K_MAX=256 得 8、K_MAX=16 得 4,
         * 於是只生出 7 個或 3 個 bank。缺的 bank 其 rdata 未驅動,
         * 合成後接地,對應的列/行永遠讀回 0。 */
        for (gi = 0; gi < 8; gi = gi + 1) begin : BANK

            (* ram_style = "block" *)
            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            always_ff @(posedge clk) begin
                if (wr && wsel == 3'(unsigned'(gi)))
                    mem[waddr] <= wdata;

                /* 同步讀:位址第 t 拍發出,資料第 t+1 拍有效。
                 * BRAM 沒有非同步讀取埠 -- 這一拍就是 K+118 變 K+119
                 * 的唯一來源,與 k_dim 無關。 */
                rdata_q <= mem[raddr[gi]];
            end

            assign rdata[gi] = rdata_q;

        end

    endgenerate

endmodule