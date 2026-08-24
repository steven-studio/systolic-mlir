`timescale 1ns / 1ps

/*
 * ============================================================
 * systolic_tile_feeder
 * ============================================================
 *
 * 自 systolic_uart_top 原樣抽出的餵料邏輯。演算法、時序、
 * 訊號語意一律未改;唯一的差別是原本的
 *
 *     if (state == ST_FEED)
 *
 * 改為由 enable 埠傳入。頂層目前仍接 state == ST_FEED,行為完全
 * 相同;解耦的目的是讓 tile-pipelined 能在排空尚未結束時就啟動
 * 下一塊 tile 的餵料 -- 只要換掉 enable 的來源即可,不必再動這裡。
 *
 * 契約
 * ----
 * enable 為真的那一拍,依 feed_t 算出每列/每行的絕對 k,送出
 * *_raddr。運算元緩衝是 block RAM(同步讀),資料下一拍才回來,
 * 所以 valid 與 accumulator context 也一併延後一拍才驅動陣列。
 *
 * 位址由絕對 k 決定。
 *
 * 舊版這裡還算了一個 fold 編號(gk >> 3),用途只有一個:挑
 * accumulator context。ctx 移除之後那個編號沒有任何讀者,所以
 * 整段跟著消失 —— 位址路徑一個字都沒改,因為它本來就只看絕對 k。
 *
 * Last boundary injection: (k_dim - 1) + max skew(N-1) = k_dim + N - 2
 * ============================================================
 */
module systolic_tile_feeder #(
    /* 陣列邊長。skew 上限 = N-1,運算元埠各 N 路。 */
    parameter int N      = 8,

    parameter int K_W    = 9,
    parameter int FEED_W = 16,
    parameter int KDIM_W = 32
)(
    input  wire clk,
    input  wire rst,
    input  wire enable,

    input  wire [FEED_W-1:0] feed_t,
    input  wire [KDIM_W-1:0] k_dim,

    input  wire [31:0] a_rdata [0:N-1],
    input  wire [31:0] b_rdata [0:N-1],

    output logic [K_W-1:0] a_raddr [0:N-1],
    output logic [K_W-1:0] b_raddr [0:N-1],

    output logic [31:0] a_in [0:N-1],
    output logic [31:0] b_in [0:N-1],

    output logic a_valid_in [0:N-1],
    output logic b_valid_in [0:N-1]
);

    /* 同步讀之後,valid 必須與資料一起延後一拍。
     * 組合邏輯寫入 _c,經一級暫存後才驅動陣列 -- 位址路徑完全不變。
     *
     * 這一級是無條件更新的,因此 enable 最後一拍算出的 valid 會在下一拍
     * 送進陣列,剛好補上最後一筆運算元。呼叫端的 FSM 不需要任何改動。 */
    logic a_valid_c [0:N-1];
    logic b_valid_c [0:N-1];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < N; i++) begin
                a_valid_in[i] <= 1'b0;
                b_valid_in[i] <= 1'b0;
            end
        end
        else begin
            for (int i = 0; i < N; i++) begin
                a_valid_in[i] <= a_valid_c[i];
                b_valid_in[i] <= b_valid_c[i];
            end
        end
    end

    always_comb begin

        for (int i = 0; i < N; i++) begin

            a_raddr[i]       = '0;
            b_raddr[i]       = '0;

            /* 同步讀:a_rdata 此刻是「前一拍位址」讀回的資料,不可依
             * 這一拍的邊界檢查清成 0,否則會抹掉仍然有效的上一筆。
             * 改為恆傳遞,由延後一拍的 valid 決定採不採用。 */
            a_in[i]          = a_rdata[i];
            b_in[i]          = b_rdata[i];

            a_valid_c[i]     = 1'b0;
            b_valid_c[i]     = 1'b0;

        end


        if (enable) begin

            /*
             * A-side skew
             */
            for (int r = 0; r < N; r++) begin

                integer gk_a;

                gk_a = int'(feed_t) - r;

                if ((gk_a >= 0) && (gk_a < int'(k_dim))) begin
                    a_raddr[r]   = K_W'(unsigned'(gk_a));
                    a_valid_c[r] = 1'b1;
                end

            end


            /*
             * B-side skew
             */
            for (int c = 0; c < N; c++) begin

                integer gk_b;

                gk_b = int'(feed_t) - c;

                if ((gk_b >= 0) && (gk_b < int'(k_dim))) begin
                    b_raddr[c]   = K_W'(unsigned'(gk_b));
                    b_valid_c[c] = 1'b1;
                end

            end

        end

    end

endmodule
