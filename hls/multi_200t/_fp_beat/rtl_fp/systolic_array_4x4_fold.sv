module systolic_array_4x4_fold #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:3],
    input  logic [DATA_W-1:0] b_in [0:3],

    input  logic a_valid_in [0:3],
    input  logic b_valid_in [0:3],

    /*
     * Fold context injected at the array boundary.
     * For this prototype:
     *   0 = fold context 0
     *   1 = fold context 1
     */
    input  logic fold_ctx_in_a [0:3],
    input  logic fold_ctx_in_b [0:3],

    output logic [DATA_W-1:0] dbg_acc_ctx0 [0:3][0:3][0:15],
    output logic [DATA_W-1:0] dbg_acc_ctx1 [0:3][0:3][0:15]
);

    logic [DATA_W-1:0] a_bus [0:3][0:4];
    logic [DATA_W-1:0] b_bus [0:4][0:3];

    logic a_valid_bus [0:3][0:4];
    logic b_valid_bus [0:4][0:3];

    /*
     * Fold context propagates with A and B.
     */
    logic fold_ctx_a_bus [0:3][0:4];
    logic fold_ctx_b_bus [0:4][0:3];

    genvar r, c, k;

    /*
     * ============================================================
     * Array boundaries
     * ============================================================
     */
    generate

        for (r = 0; r < 4; r = r + 1) begin : INIT_A
            assign a_bus[r][0]        = a_in[r];
            assign a_valid_bus[r][0]  = a_valid_in[r];
            assign fold_ctx_a_bus[r][0] = fold_ctx_in_a[r];
        end

        for (c = 0; c < 4; c = c + 1) begin : INIT_B
            assign b_bus[0][c]        = b_in[c];
            assign b_valid_bus[0][c]  = b_valid_in[c];
            assign fold_ctx_b_bus[0][c] = fold_ctx_in_b[c];
        end

    endgenerate


    /*
     * ============================================================
     * 4x4 fold-pipelined PE array
     * ============================================================
     */
    generate

        for (r = 0; r < 4; r = r + 1) begin : ROW

            for (c = 0; c < 4; c = c + 1) begin : COL

                logic [DATA_W-1:0] dbg0 [0:15];
                logic [DATA_W-1:0] dbg1 [0:15];

                /*
                 * A and B should carry the same fold context when
                 * they meet at the PE.
                 *
                 * For the actual PE context we use the A-side tag.
                 * The TB will verify A/B contexts remain aligned.
                 */
                logic pe_fold_ctx;

                assign pe_fold_ctx = fold_ctx_a_bus[r][c];

                systolic_pe_fold u_pe (
                    .clk          (clk),
                    .rst          (rst),

                    .a_valid_in   (a_valid_bus[r][c]),
                    .b_valid_in   (b_valid_bus[r][c]),

                    .fold_ctx_in  (pe_fold_ctx),

                    .a_in         (a_bus[r][c]),
                    .b_in         (b_bus[r][c]),

                    .a_valid_out  (a_valid_bus[r][c+1]),
                    .b_valid_out  (b_valid_bus[r+1][c]),

                    .fold_ctx_out (fold_ctx_a_bus[r][c+1]),

                    .a_out        (a_bus[r][c+1]),
                    .b_out        (b_bus[r+1][c]),

                    .dbg_acc_ctx0 (dbg0),
                    .dbg_acc_ctx1 (dbg1)
                );

                /*
                 * B-side context must propagate downward as well.
                 *
                 * Since systolic_pe_fold currently has only one
                 * fold_ctx_out port, mirror the same registered
                 * context onto the B context path.
                 */
                assign fold_ctx_b_bus[r+1][c] =
                    fold_ctx_a_bus[r][c+1];

                for (k = 0; k < 16; k = k + 1) begin : EXPORT_ACC
                    assign dbg_acc_ctx0[r][c][k] = dbg0[k];
                    assign dbg_acc_ctx1[r][c][k] = dbg1[k];
                end

            end
        end

    endgenerate

endmodule