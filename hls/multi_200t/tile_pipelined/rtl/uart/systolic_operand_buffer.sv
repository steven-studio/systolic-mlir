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
 * ============================================================
 */
module systolic_operand_buffer #(
    parameter int K_MAX = 256,
    parameter int K_W   = 9
)(
    input  wire clk,

    input  wire           wr,      // 寫入脈衝(A/B 判別已在呼叫端折入)
    input  wire [2:0]     wsel,    // 寫哪一個 bank
    input  wire [K_W-1:0] waddr,
    input  wire [31:0]    wdata,

    input  wire [8*K_W-1:0] raddr_flat,
    output wire [8*32-1:0]  rdata_flat
);

    genvar gi;

    generate

        for (gi = 0; gi < K_W-1; gi = gi + 1) begin : BANK

            (* ram_style = "block" *)
            logic [31:0] mem [0:K_MAX-1];
            logic [31:0] rdata_q;

            always_ff @(posedge clk) begin
                if (wr && wsel == 3'(unsigned'(gi)))
                    mem[waddr] <= wdata;

                /* 同步讀:位址第 t 拍發出,資料第 t+1 拍有效。
                 * BRAM 沒有非同步讀取埠 -- 這一拍就是 K+118 變 K+119
                 * 的唯一來源,與 k_dim 無關。 */
                rdata_q <= mem[raddr_flat[gi*K_W +: K_W]];
            end

            assign rdata_flat[gi*32 +: 32] = rdata_q;

        end

    endgenerate

endmodule
