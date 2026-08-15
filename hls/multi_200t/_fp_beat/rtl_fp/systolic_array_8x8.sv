module systolic_array_8x8 #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:7],
    input  logic [DATA_W-1:0] b_in [0:7],

    input  logic              a_valid_in [0:7],
    input  logic              b_valid_in [0:7],

    /*
     * 暫時保留。
     * 目前 systolic_pe 內部使用 local_acc_sel。
     */
    input  logic [3:0] acc_sel,

    /*
     * GEMM pipeline 完全 drain 後，
     * pulse 1 cycle 開始把每個 PE 的 banks 加起來。
     */
    input  logic reduce_start,

    /*
     * 所有 64 個 PE 同時 reduction。
     */
    output logic c_valid_out,

    /*
     * 真正的 8x8 GEMM result。
     */
    output logic [DATA_W-1:0] c_out [0:7][0:7],

    /*
     * Debug 用，暫時保留。
     */
    output logic [DATA_W-1:0] dbg_acc_out [0:7][0:7][0:15]
);

    logic [DATA_W-1:0] a_bus [0:7][0:8];
    logic [DATA_W-1:0] b_bus [0:8][0:7];

    logic a_valid_bus [0:7][0:8];
    logic b_valid_bus [0:8][0:7];

    logic reduce_valid [0:7][0:7];
    logic reduce_busy  [0:7][0:7];

    genvar r, c, k;


    /*
     * ============================================================
     * Array boundaries
     * ============================================================
     */
    generate

        for (r = 0; r < 8; r = r + 1) begin : INIT_A
            assign a_bus[r][0]       = a_in[r];
            assign a_valid_bus[r][0] = a_valid_in[r];
        end

        for (c = 0; c < 8; c = c + 1) begin : INIT_B
            assign b_bus[0][c]       = b_in[c];
            assign b_valid_bus[0][c] = b_valid_in[c];
        end

    endgenerate


    /*
     * ============================================================
     * 8 x 8 PE array
     * ============================================================
     */
    generate

        for (r = 0; r < 8; r = r + 1) begin : ROW

            for (c = 0; c < 8; c = c + 1) begin : COL

                logic [DATA_W-1:0] dbg_acc [0:15];

                systolic_pe u_pe (
                    .clk         (clk),
                    .rst         (rst),

                    .a_valid_in  (a_valid_bus[r][c]),
                    .b_valid_in  (b_valid_bus[r][c]),

                    .acc_sel     (acc_sel),

                    .a_in        (a_bus[r][c]),
                    .b_in        (b_bus[r][c]),

                    .a_valid_out (a_valid_bus[r][c+1]),
                    .b_valid_out (b_valid_bus[r+1][c]),

                    .a_out       (a_bus[r][c+1]),
                    .b_out       (b_bus[r+1][c]),

                    .dbg_acc     (dbg_acc)
                );


                /*
                 * Export accumulator banks for debugging.
                 */
                for (k = 0; k < 16; k = k + 1) begin : EXPORT_ACC
                    assign dbg_acc_out[r][c][k] = dbg_acc[k];
                end


                /*
                 * =================================================
                 * Final reduction:
                 *
                 * bank[0] + ... + bank[7] -> C[r][c]
                 * =================================================
                 */
                fp_reduce8 u_reduce (
                    .clk       (clk),
                    .rst       (rst),

                    .start     (reduce_start),

                    .acc       (dbg_acc[0:7]),

                    .busy      (reduce_busy[r][c]),
                    .valid_out (reduce_valid[r][c]),
                    .result    (c_out[r][c])
                );

            end
        end

    endgenerate


    /*
     * 所有 reducer 是完全同步啟動，
     * 因此用 PE[0][0] 的 valid 當整個矩陣的 valid。
     */
    assign c_valid_out = reduce_valid[0][0];

endmodule