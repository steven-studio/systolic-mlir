module systolic_array_tile #(
    /*
     * Array edge length. The array is square: N rows by N columns,
     * N*N processing elements.
     *
     * This used to be the literal 8, duplicated into a second file
     * for the 4x4 case. That copy stopped compiling somewhere around
     * the time the PE gained result_ctx0/result_ctx1 -- it was still
     * connecting dbg_acc_ctx0/dbg_acc_ctx1, ports the PE no longer
     * has. Two files that must be edited together will eventually
     * not be, so there is now one.
     *
     * Nothing here depends on the fold decomposition. The fold
     * context arrives at the boundary already computed, so N is
     * independent of the 8-deep k window the host uses.
     */
    parameter int N = 8,

    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:N-1],
    input  logic [DATA_W-1:0] b_in [0:N-1],

    input  logic a_valid_in [0:N-1],
    input  logic b_valid_in [0:N-1],

    input  logic accum_ctx_in_a [0:N-1],
    input  logic accum_ctx_in_b [0:N-1],

    output logic              c_valid_out,
    output logic              c_ctx_out,
    output logic [DATA_W-1:0] c_out [0:N-1][0:N-1]
);


    /*
     * ============================================================
     * Systolic operand buses
     * ============================================================
     */

    logic [DATA_W-1:0] a_bus [0:N-1][0:N];
    logic [DATA_W-1:0] b_bus [0:N][0:N-1];

    logic a_valid_bus [0:N-1][0:N];
    logic b_valid_bus [0:N][0:N-1];


    /*
     * ============================================================
     * Fold-context buses
     * ============================================================
     */

    logic accum_ctx_a_bus [0:N-1][0:N];
    logic accum_ctx_b_bus [0:N][0:N-1];


    /*
     * ============================================================
     * Final scalar result from every PE
     *
     * Each PE owns its accumulator implementation internally.
     * The array only sees one scalar result per fold/context.
     * ============================================================
     */

    logic [DATA_W-1:0]
        pe_result_ctx0 [0:N-1][0:N-1];

    logic [DATA_W-1:0]
        pe_result_ctx1 [0:N-1][0:N-1];

    logic
        pe_result_valid [0:N-1][0:N-1];


    genvar r;
    genvar c;


    /*
     * ============================================================
     * Array boundaries
     * ============================================================
     */

    generate

        for (r = 0; r < N; r = r + 1) begin : INIT_A

            assign a_bus[r][0] =
                a_in[r];

            assign a_valid_bus[r][0] =
                a_valid_in[r];

            assign accum_ctx_a_bus[r][0] =
                accum_ctx_in_a[r];

        end


        for (c = 0; c < N; c = c + 1) begin : INIT_B

            assign b_bus[0][c] =
                b_in[c];

            assign b_valid_bus[0][c] =
                b_valid_in[c];

            assign accum_ctx_b_bus[0][c] =
                accum_ctx_in_b[c];

        end

    endgenerate


    /*
     * ============================================================
     * N x N systolic array
     * ============================================================
     */

    generate

        for (r = 0; r < N; r = r + 1) begin : ROW

            for (c = 0; c < N; c = c + 1) begin : COL

                /*
                 * Matching A/B operands are expected to carry the
                 * same fold context when they meet at this PE.
                 *
                 * Current PE interface carries one fold context,
                 * therefore the A-side registered context is used.
                 */
                logic pe_accum_ctx;

                assign pe_accum_ctx =
                    accum_ctx_a_bus[r][c];


                systolic_pe_tile u_pe (
                    .clk          (clk),
                    .rst          (rst),

                    .a_valid_in   (
                        a_valid_bus[r][c]
                    ),

                    .b_valid_in   (
                        b_valid_bus[r][c]
                    ),

                    .accum_ctx_in  (
                        pe_accum_ctx
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

                    .accum_ctx_out (
                        accum_ctx_a_bus[r][c+1]
                    ),

                    .a_out        (
                        a_bus[r][c+1]
                    ),

                    .b_out        (
                        b_bus[r+1][c]
                    ),

                    .result_ctx0  (
                        pe_result_ctx0[r][c]
                    ),

                    .result_ctx1  (
                        pe_result_ctx1[r][c]
                    ),

                    .result_valid (
                        pe_result_valid[r][c]
                    )
                );


                /*
                 * Current PE has one fold-context output.
                 *
                 * Mirror the registered context downward so the
                 * B-side context follows the same systolic delay.
                 */
                assign accum_ctx_b_bus[r+1][c] =
                    accum_ctx_a_bus[r][c+1];

            end

        end

    endgenerate


    /*
     * ============================================================
     * Wait until every PE has finalized its result
     * ============================================================
     */

    logic all_results_valid;
    logic result_seen [0:N-1][0:N-1];
    logic clear_result_seen;

    always_ff @(posedge clk) begin

        if (rst) begin

            for (int rr = 0; rr < N; rr = rr + 1)
                for (int cc = 0; cc < N; cc = cc + 1)
                    result_seen[rr][cc] <= 1'b0;

        end
        else if (clear_result_seen) begin

            /*
             * Previous matrix transaction has been published.
             * Rearm completion tracking for the next transaction.
             */
            for (int rr = 0; rr < N; rr = rr + 1)
                for (int cc = 0; cc < N; cc = cc + 1)
                    result_seen[rr][cc] <= 1'b0;

        end
        else begin

            for (int rr = 0; rr < N; rr = rr + 1)
                for (int cc = 0; cc < N; cc = cc + 1)
                    if (pe_result_valid[rr][cc])
                        result_seen[rr][cc] <= 1'b1;

        end

    end

    always_comb begin

        all_results_valid = 1'b1;

        for (int rr = 0; rr < N; rr = rr + 1)
            for (int cc = 0; cc < N; cc = cc + 1)
                all_results_valid =
                    all_results_valid &&
                    result_seen[rr][cc];

    end


    /*
     * ============================================================
     * Output matrix selection
     *
     * c_ctx_out = 0 -> PE context 0 results
     * c_ctx_out = 1 -> PE context 1 results
     * ============================================================
     */

    always_comb begin

        for (int rr = 0; rr < N; rr = rr + 1) begin

            for (int cc = 0; cc < N; cc = cc + 1) begin

                if (c_ctx_out == 1'b0)
                    c_out[rr][cc] =
                        pe_result_ctx0[rr][cc];
                else
                    c_out[rr][cc] =
                        pe_result_ctx1[rr][cc];

            end

        end

    end


    /*
     * ============================================================
     * Result output controller
     *
     * Once every PE has both results:
     *
     *   cycle N   -> ctx0 valid
     *   cycle N+1 -> ctx1 valid
     *
     * Then wait until PE result_valid drops before accepting
     * another matrix transaction.
     * ============================================================
     */

    typedef enum logic [1:0] {
        OUT_WAIT_READY,
        OUT_CTX0,
        OUT_CTX1,
        OUT_WAIT_CLEAR
    } out_state_t;

    out_state_t out_state;


    always_ff @(posedge clk) begin

        if (rst) begin

            c_valid_out <=
                1'b0;

            c_ctx_out <=
                1'b0;

            out_state <=
                OUT_WAIT_READY;

            clear_result_seen <=
                1'b0;

        end
        else begin

            /*
             * Default: valid is a one-cycle pulse.
             */
            c_valid_out <=
                1'b0;

            clear_result_seen <=
                1'b0;


            case (out_state)


                /*
                 * ---------------------------------------------
                 * Wait for all N*N PE results.
                 * ---------------------------------------------
                 */
                OUT_WAIT_READY: begin

                    if (all_results_valid) begin

                        c_ctx_out <=
                            1'b0;

                        out_state <=
                            OUT_CTX0;

                    end

                end


                /*
                 * ---------------------------------------------
                 * Publish context 0 matrix.
                 * ---------------------------------------------
                 */
                OUT_CTX0: begin

                    c_ctx_out <=
                        1'b0;

                    c_valid_out <=
                        1'b1;

                    out_state <=
                        OUT_CTX1;

                end


                /*
                 * ---------------------------------------------
                 * Publish context 1 matrix.
                 * ---------------------------------------------
                 */
                OUT_CTX1: begin

                    c_ctx_out <=
                        1'b1;

                    c_valid_out <=
                        1'b1;

                    /*
                     * Both contexts have now been published.
                     * Clear sticky PE completion state so the
                     * controller can accept another transaction.
                     */
                    clear_result_seen <=
                        1'b1;

                    out_state <=
                        OUT_WAIT_CLEAR;

                end


                /*
                 * ---------------------------------------------
                 * result_valid inside each PE remains asserted
                 * until the next input transaction begins.
                 *
                 * Do not emit the same matrices repeatedly.
                 * ---------------------------------------------
                 */
                OUT_WAIT_CLEAR: begin

                    if (!all_results_valid) begin

                        c_ctx_out <=
                            1'b0;

                        out_state <=
                            OUT_WAIT_READY;

                    end

                end


                default: begin

                    c_ctx_out <=
                        1'b0;

                    out_state <=
                        OUT_WAIT_READY;

                end

            endcase

        end

    end


endmodule