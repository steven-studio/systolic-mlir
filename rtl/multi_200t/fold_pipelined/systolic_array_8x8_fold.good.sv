module systolic_array_8x8_fold #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:7],
    input  logic [DATA_W-1:0] b_in [0:7],

    input  logic a_valid_in [0:7],
    input  logic b_valid_in [0:7],

    /*
     * Fold context at array boundaries.
     *
     * Current systolic_pe_fold has ONE context input/output.
     * A/B are skewed such that matching operands carry the same
     * fold context when they meet at a PE.
     */
    input  logic fold_ctx_in_a [0:7],
    input  logic fold_ctx_in_b [0:7],

    /*
     * Debug exports for numerical verification.
     */
    output logic [DATA_W-1:0]
        dbg_acc_ctx0 [0:7][0:7][0:15],

    output logic [DATA_W-1:0]
        dbg_acc_ctx1 [0:7][0:7][0:15]
);


    /*
     * ============================================================
     * Systolic operand buses
     * ============================================================
     */
    logic [DATA_W-1:0] a_bus [0:7][0:8];
    logic [DATA_W-1:0] b_bus [0:8][0:7];

    logic a_valid_bus [0:7][0:8];
    logic b_valid_bus [0:8][0:7];


    /*
     * Fold-context buses.
     *
     * Current PE exposes only one fold_ctx_out.
     *
     * We use the A-side context as the PE's context tag and mirror
     * the registered context downward for the B-side path, exactly
     * like the already-tested 4x4 prototype.
     */
    logic fold_ctx_a_bus [0:7][0:8];
    logic fold_ctx_b_bus [0:8][0:7];


    genvar r;
    genvar c;
    genvar k;


    /*
     * ============================================================
     * Array boundaries
     * ============================================================
     */
    generate

        for (r = 0; r < 8; r = r + 1) begin : INIT_A

            assign a_bus[r][0] =
                a_in[r];

            assign a_valid_bus[r][0] =
                a_valid_in[r];

            assign fold_ctx_a_bus[r][0] =
                fold_ctx_in_a[r];

        end


        for (c = 0; c < 8; c = c + 1) begin : INIT_B

            assign b_bus[0][c] =
                b_in[c];

            assign b_valid_bus[0][c] =
                b_valid_in[c];

            assign fold_ctx_b_bus[0][c] =
                fold_ctx_in_b[c];

        end

    endgenerate


    /*
     * ============================================================
     * 8 x 8 fold-pipelined systolic array
     * ============================================================
     */
    generate

        for (r = 0; r < 8; r = r + 1) begin : ROW

            for (c = 0; c < 8; c = c + 1) begin : COL

                logic [DATA_W-1:0]
                    local_dbg_ctx0 [0:15];

                logic [DATA_W-1:0]
                    local_dbg_ctx1 [0:15];

                /*
                 * Matching A/B operands have the same context once
                 * skewed into this PE.
                 *
                 * Current prototype uses the A-side tag as the PE
                 * context.
                 */
                logic pe_fold_ctx;

                assign pe_fold_ctx =
                    fold_ctx_a_bus[r][c];


                systolic_pe_fold u_pe (
                    .clk          (clk),
                    .rst          (rst),

                    .a_valid_in   (
                        a_valid_bus[r][c]
                    ),

                    .b_valid_in   (
                        b_valid_bus[r][c]
                    ),

                    /*
                     * ONE fold context port in systolic_pe_fold.
                     */
                    .fold_ctx_in  (
                        pe_fold_ctx
                    ),

                    .a_in         (
                        a_bus[r][c]
                    ),

                    .b_in         (
                        b_bus[r][c]
                    ),

                    .a_valid_out  (
                        a_valid_bus[r][c+1]
                    ),

                    .b_valid_out  (
                        b_valid_bus[r+1][c]
                    ),

                    .fold_ctx_out (
                        fold_ctx_a_bus[r][c+1]
                    ),

                    .a_out        (
                        a_bus[r][c+1]
                    ),

                    .b_out        (
                        b_bus[r+1][c]
                    ),

                    .dbg_acc_ctx0 (
                        local_dbg_ctx0
                    ),

                    .dbg_acc_ctx1 (
                        local_dbg_ctx1
                    )
                );


                /*
                 * Mirror the same registered context downward.
                 *
                 * This is the same prototype structure used by the
                 * 4x4 fold array that already passed the 134-cycle
                 * and numerical-independence test.
                 */
                assign fold_ctx_b_bus[r+1][c] =
                    fold_ctx_a_bus[r][c+1];


                /*
                 * =================================================
                 * Export both accumulator contexts
                 * =================================================
                 */
                for (
                    k = 0;
                    k < 16;
                    k = k + 1
                ) begin : EXPORT_ACC

                    assign dbg_acc_ctx0[r][c][k] =
                        local_dbg_ctx0[k];

                    assign dbg_acc_ctx1[r][c][k] =
                        local_dbg_ctx1[k];

                end

            end

        end

    endgenerate

endmodule
