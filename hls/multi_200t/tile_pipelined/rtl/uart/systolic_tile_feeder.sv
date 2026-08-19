`timescale 1ns / 1ps

/*
 * ============================================================
 * systolic_tile_feeder
 * ============================================================
 *
 * 自 systolic_uart_tile_top 原樣抽出的餵料邏輯。演算法、時序、
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
 * 位址由絕對 k 決定,fold 編號只用來挑 accumulator context ——
 * 這個分工是 fold 設計的核心,搬移過程中未更動。
 *
 * Last boundary injection: (k_dim - 1) + max skew(7) = k_dim + 6
 * ============================================================
 */
module systolic_tile_feeder #(
    parameter int K_W    = 9,
    parameter int FEED_W = 16,
    parameter int KDIM_W = 32
)(
    input  wire clk,
    input  wire rst,
    input  wire enable,

    input  wire [FEED_W-1:0] feed_t,
    input  wire [KDIM_W-1:0] k_dim,

    input  wire [31:0] a_rdata [0:7],
    input  wire [31:0] b_rdata [0:7],

    output logic [K_W-1:0] a_raddr [0:7],
    output logic [K_W-1:0] b_raddr [0:7],

    output logic [31:0] a_in [0:7],
    output logic [31:0] b_in [0:7],

    output logic a_valid_in     [0:7],
    output logic b_valid_in     [0:7],
    output logic accum_ctx_in_a [0:7],
    output logic accum_ctx_in_b [0:7]
);

    /* 同步讀之後,valid 與 accumulator context 必須與資料一起延後一拍。
     * 組合邏輯寫入 _c,經一級暫存後才驅動陣列 -- 位址路徑完全不變。
     *
     * 這一級是無條件更新的,因此 enable 最後一拍算出的 valid 會在下一拍
     * 送進陣列,剛好補上最後一筆運算元。呼叫端的 FSM 不需要任何改動。 */
    logic a_valid_c     [0:7];
    logic b_valid_c     [0:7];
    logic accum_ctx_a_c [0:7];
    logic accum_ctx_b_c [0:7];

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int i = 0; i < 8; i++) begin
                a_valid_in[i]     <= 1'b0;
                b_valid_in[i]     <= 1'b0;
                accum_ctx_in_a[i] <= 1'b0;
                accum_ctx_in_b[i] <= 1'b0;
            end
        end
        else begin
            for (int i = 0; i < 8; i++) begin
                a_valid_in[i]     <= a_valid_c[i];
                b_valid_in[i]     <= b_valid_c[i];
                accum_ctx_in_a[i] <= accum_ctx_a_c[i];
                accum_ctx_in_b[i] <= accum_ctx_b_c[i];
            end
        end
    end

    always_comb begin

        for (int i = 0; i < 8; i++) begin

            a_raddr[i]       = '0;
            b_raddr[i]       = '0;

            /* 同步讀:a_rdata 此刻是「前一拍位址」讀回的資料,不可依
             * 這一拍的邊界檢查清成 0,否則會抹掉仍然有效的上一筆。
             * 改為恆傳遞,由延後一拍的 valid 決定採不採用。 */
            a_in[i]          = a_rdata[i];
            b_in[i]          = b_rdata[i];

            a_valid_c[i]     = 1'b0;
            b_valid_c[i]     = 1'b0;

            accum_ctx_a_c[i] = 1'b0;
            accum_ctx_b_c[i] = 1'b0;

        end


        if (enable) begin

            /*
             * A-side skew
             */
            for (int r = 0; r < 8; r++) begin

                integer gk_a;
                integer fold_a;

                gk_a = int'(feed_t) - r;

                if ((gk_a >= 0) && (gk_a < int'(k_dim))) begin

                    /*
                     * The fold number is derived here, at run time,
                     * purely to pick the accumulator context. The
                     * buffer is addressed by absolute k.
                     */
                    fold_a = gk_a >> 3;

                    a_raddr[r] = K_W'(unsigned'(gk_a));

                    a_valid_c[r]     = 1'b1;
                    accum_ctx_a_c[r] = fold_a[0];

                end

            end


            /*
             * B-side skew
             */
            for (int c = 0; c < 8; c++) begin

                integer gk_b;
                integer fold_b;

                gk_b = int'(feed_t) - c;

                if ((gk_b >= 0) && (gk_b < int'(k_dim))) begin

                    fold_b = gk_b >> 3;

                    b_raddr[c] = K_W'(unsigned'(gk_b));

                    b_valid_c[c]     = 1'b1;
                    accum_ctx_b_c[c] = fold_b[0];

                end

            end

        end

    end

endmodule
