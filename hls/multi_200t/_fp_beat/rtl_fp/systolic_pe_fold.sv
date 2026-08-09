module systolic_pe_fold #(
    parameter int DATA_W      = 32,
    parameter int ACC_BANKS   = 16,
    parameter int MUL_LATENCY = 9,
    parameter int ADD_LATENCY = 12
) (
    input  logic              clk,
    input  logic              rst,

    input  logic              a_valid_in,
    input  logic              b_valid_in,
    input  logic              fold_ctx_in,

    input  logic [DATA_W-1:0] a_in,
    input  logic [DATA_W-1:0] b_in,

    output logic              a_valid_out,
    output logic              b_valid_out,
    output logic              fold_ctx_out,

    output logic [DATA_W-1:0] a_out,
    output logic [DATA_W-1:0] b_out,

    output logic [DATA_W-1:0] result_ctx0,
    output logic [DATA_W-1:0] result_ctx1,
    output logic              result_valid
);


    /*
     * ============================================================
     * PE-to-PE pipeline register
     * ============================================================
     */

    logic [DATA_W-1:0] a_reg;
    logic [DATA_W-1:0] b_reg;

    logic a_valid_reg;
    logic b_valid_reg;

    logic fold_ctx_reg;

    logic [3:0] sel_reg;
    logic [3:0] local_acc_sel [0:1];

    wire pair_valid =
        a_valid_in && b_valid_in;

    wire pipe_pair_valid =
        a_valid_reg && b_valid_reg;


    /*
     * ============================================================
     * Local control
     * ============================================================
     */

    typedef enum logic [1:0] {
        PE_ACCUM,
        PE_REDUCE_ISSUE,
        PE_REDUCE_WAIT
    } pe_state_t;

    pe_state_t state;

    logic clear_acc_banks;


    /*
     * ============================================================
     * Stage 0
     * ============================================================
     */

    always_ff @(posedge clk) begin

        if (rst) begin

            a_reg       <= '0;
            b_reg       <= '0;

            a_valid_reg <= 1'b0;
            b_valid_reg <= 1'b0;

            fold_ctx_reg <= 1'b0;

            sel_reg <= '0;

            local_acc_sel[0] <= '0;
            local_acc_sel[1] <= '0;

        end
        else begin

            a_valid_reg <= a_valid_in;
            b_valid_reg <= b_valid_in;

            if (a_valid_in)
                a_reg <= a_in;

            if (b_valid_in)
                b_reg <= b_in;


            /*
             * A completed operation has consumed both local
             * accumulator contexts. Prepare selectors for the
             * next matrix transaction.
             */
            if (clear_acc_banks) begin

                local_acc_sel[0] <= '0;
                local_acc_sel[1] <= '0;

            end
            else if (pair_valid) begin

                fold_ctx_reg <= fold_ctx_in;

                sel_reg <=
                    local_acc_sel[fold_ctx_in];

                local_acc_sel[fold_ctx_in] <=
                    local_acc_sel[fold_ctx_in] + 1'b1;

            end

        end

    end


    assign a_out =
        a_reg;

    assign b_out =
        b_reg;

    assign a_valid_out =
        a_valid_reg;

    assign b_valid_out =
        b_valid_reg;

    assign fold_ctx_out =
        fold_ctx_reg;


    /*
     * ============================================================
     * FP multiplier
     * ============================================================
     */

    logic [DATA_W-1:0] product;
    logic              product_valid;


    fp_mul u_fp_mul (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (pipe_pair_valid),

        .a         (a_reg),
        .b         (b_reg),

        .valid_out (product_valid),
        .result    (product)
    );


    /*
     * ============================================================
     * Multiplier metadata pipeline
     * ============================================================
     */

    logic [3:0] mul_sel_pipe [0:MUL_LATENCY-1];
    logic       mul_ctx_pipe [0:MUL_LATENCY-1];

    integer m;


    always_ff @(posedge clk) begin

        if (rst) begin

            for (
                m = 0;
                m < MUL_LATENCY;
                m = m + 1
            ) begin

                mul_sel_pipe[m] <= '0;
                mul_ctx_pipe[m] <= 1'b0;

            end

        end
        else begin

            mul_sel_pipe[0] <=
                sel_reg;

            mul_ctx_pipe[0] <=
                fold_ctx_reg;

            for (
                m = 1;
                m < MUL_LATENCY;
                m = m + 1
            ) begin

                mul_sel_pipe[m] <=
                    mul_sel_pipe[m-1];

                mul_ctx_pipe[m] <=
                    mul_ctx_pipe[m-1];

            end

        end

    end


    wire [3:0] product_sel =
        mul_sel_pipe[MUL_LATENCY-1];

    wire product_ctx =
        mul_ctx_pipe[MUL_LATENCY-1];


    /*
     * ============================================================
     * Local accumulator banks
     *
     * These are PRIVATE PE state.
     * They are no longer exported to the array.
     * ============================================================
     */

    logic [DATA_W-1:0]
        acc_bank [0:1][0:ACC_BANKS-1];

    logic [DATA_W-1:0]
        selected_acc;


    always_comb begin

        selected_acc =
            acc_bank[product_ctx][product_sel];

    end


    /*
     * ============================================================
     * Local result reduction state
     * ============================================================
     *
     * After all MAC writebacks have drained, the same fp_add
     * used for accumulation is reused to reduce:
     *
     *   acc_bank[ctx][0..15]
     *
     * into one scalar result.
     * ============================================================
     */

    logic       reduce_ctx;
    logic [3:0] reduce_index;

    logic [DATA_W-1:0]
        reduce_running_sum;


    /*
     * ============================================================
     * Shared FP adder input mux
     * ============================================================
     */

    logic              fp_add_valid_in;
    logic [DATA_W-1:0] fp_add_a;
    logic [DATA_W-1:0] fp_add_b;

    logic [DATA_W-1:0] add_result;
    logic              add_valid;


    always_comb begin

        fp_add_valid_in = 1'b0;

        fp_add_a = '0;
        fp_add_b = '0;


        /*
         * Normal MAC accumulation.
         */
        if (state == PE_ACCUM) begin

            fp_add_valid_in =
                product_valid;

            fp_add_a =
                selected_acc;

            fp_add_b =
                product;

        end


        /*
         * Final local reduction.
         *
         * Exactly one FP add is issued while in this state.
         */
        else if (state == PE_REDUCE_ISSUE) begin

            fp_add_valid_in =
                1'b1;

            fp_add_a =
                reduce_running_sum;

            fp_add_b =
                acc_bank[reduce_ctx][reduce_index];

        end

    end


    fp_add u_fp_add (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (fp_add_valid_in),

        .a         (fp_add_a),
        .b         (fp_add_b),

        .valid_out (add_valid),
        .result    (add_result)
    );


    /*
     * ============================================================
     * Accumulation writeback metadata pipeline
     * ============================================================
     */

    logic [3:0]
        add_sel_pipe [0:ADD_LATENCY-1];

    logic
        add_ctx_pipe [0:ADD_LATENCY-1];

    logic add_accum_tag_pipe [0:ADD_LATENCY-1];

    integer ap;


    always_ff @(posedge clk) begin

        if (rst) begin

            for (
                ap = 0;
                ap < ADD_LATENCY;
                ap = ap + 1
            ) begin

                add_sel_pipe[ap] <= '0;
                add_ctx_pipe[ap] <= 1'b0;

            end

        end
        else begin

            add_sel_pipe[0] <=
                product_sel;

            add_ctx_pipe[0] <=
                product_ctx;

            add_accum_tag_pipe[0] <=
                (state == PE_ACCUM) &&
                fp_add_valid_in;

            for (
                ap = 1;
                ap < ADD_LATENCY;
                ap = ap + 1
            ) begin

                add_sel_pipe[ap] <=
                    add_sel_pipe[ap-1];

                add_ctx_pipe[ap] <=
                    add_ctx_pipe[ap-1];

                add_accum_tag_pipe[ap] <=
                    add_accum_tag_pipe[ap-1];

            end

        end

    end

    wire add_result_is_accum =
        add_accum_tag_pipe[ADD_LATENCY-2];

    /*
     * Keep the currently verified latency alignment.
     */
    wire [3:0] writeback_sel =
        add_sel_pipe[ADD_LATENCY-3];

    wire writeback_ctx =
        add_ctx_pipe[ADD_LATENCY-3];


    /*
     * ============================================================
     * Detect when the complete input transaction has drained
     * ============================================================
     */

    logic pipe_pair_valid_d;
    logic transaction_seen;
    logic input_finished;

    /*
     * Number of products that have entered the FP ADD but have
     * not yet returned.
     */
    logic [7:0] outstanding_adds;


    always_ff @(posedge clk) begin

        if (rst) begin

            pipe_pair_valid_d <= 1'b0;

            transaction_seen <= 1'b0;
            input_finished   <= 1'b0;

            outstanding_adds <= '0;

        end
        else if (state == PE_ACCUM) begin

            pipe_pair_valid_d <=
                pipe_pair_valid;


            if (pipe_pair_valid) begin

                transaction_seen <= 1'b1;

                /*
                 * Beginning of a new transaction.
                 */
                if (result_valid)
                    input_finished <= 1'b0;

            end


            /*
             * The two folds are contiguous, therefore the falling
             * edge after a transaction marks the end of injection
             * into this PE.
             */
            if (
                transaction_seen &&
                pipe_pair_valid_d &&
                !pipe_pair_valid
            ) begin

                input_finished <= 1'b1;

            end


            /*
             * Track products waiting for FP ADD writeback.
             */
            case ({
                product_valid,
                add_valid && add_result_is_accum
            })

                2'b10:
                    outstanding_adds <=
                        outstanding_adds + 1'b1;

                2'b01:
                    outstanding_adds <=
                        outstanding_adds - 1'b1;

                default:
                    outstanding_adds <=
                        outstanding_adds;

            endcase

        end
        else begin

            /*
             * Not used during local reduction.
             */
            outstanding_adds <= '0;

        end


        if (clear_acc_banks) begin

            transaction_seen <= 1'b0;
            input_finished   <= 1'b0;
            outstanding_adds <= '0;

        end

    end


    /*
     * ============================================================
     * Accumulator-bank writeback
     * ============================================================
     */

    integer ctx_i;
    integer bank_i;


    always_ff @(posedge clk) begin

        if (rst) begin

            for (
                ctx_i = 0;
                ctx_i < 2;
                ctx_i = ctx_i + 1
            ) begin

                for (
                    bank_i = 0;
                    bank_i < ACC_BANKS;
                    bank_i = bank_i + 1
                ) begin

                    acc_bank[ctx_i][bank_i] <= '0;

                end

            end

        end
        else if (clear_acc_banks) begin

            for (
                ctx_i = 0;
                ctx_i < 2;
                ctx_i = ctx_i + 1
            ) begin

                for (
                    bank_i = 0;
                    bank_i < ACC_BANKS;
                    bank_i = bank_i + 1
                ) begin

                    acc_bank[ctx_i][bank_i] <= '0;

                end

            end

        end
        else if (
            add_valid && add_result_is_accum
        ) begin

            acc_bank
                [writeback_ctx]
                [writeback_sel]
                    <= add_result;

        end

    end


    /*
     * ============================================================
     * Final-result FSM
     * ============================================================
     */

    always_comb begin

        clear_acc_banks =
            (
                state == PE_REDUCE_WAIT &&
                add_valid &&
                reduce_ctx == 1'b1 &&
                reduce_index == ACC_BANKS-1
            );

    end

    always_ff @(posedge clk) begin
        if (!rst &&
            state == PE_ACCUM &&
            add_valid &&
            !product_valid &&
            outstanding_adds == 0) begin

            $display(
                "OUTSTANDING UNDERFLOW time=%0t pair=%b finished=%b prod=%b add=%b",
                $time,
                pipe_pair_valid,
                input_finished,
                product_valid,
                add_valid
            );

        end
    end

    always_ff @(posedge clk) begin

        if (rst) begin

            state <=
                PE_ACCUM;

            reduce_ctx <=
                1'b0;

            reduce_index <=
                4'd1;

            reduce_running_sum <=
                '0;

            result_ctx0 <=
                '0;

            result_ctx1 <=
                '0;

            result_valid <=
                1'b0;

        end
        else begin

            /*
            * result_valid is a one-cycle pulse.
            * It is asserted only when ctx1 reduction finishes.
            */
            result_valid <= 1'b0;

            case (state)


                /*
                 * ---------------------------------------------
                 * Normal systolic accumulation.
                 * ---------------------------------------------
                 */
                PE_ACCUM: begin

                    /*
                     * Wait until:
                     *
                     *  1. the input stream ended,
                     *  2. multiplier has no new product,
                     *  3. all accumulator ADDs have returned.
                     */
                    if (
                        input_finished &&
                        outstanding_adds == 0 &&
                        !product_valid &&
                        !add_valid
                    ) begin

                        /*
                         * Start ctx0 reduction.
                         *
                         * bank0 is the initial running value.
                         */
                        reduce_ctx <=
                            1'b0;

                        reduce_index <=
                            4'd1;

                        reduce_running_sum <=
                            acc_bank[0][0];

                        state <=
                            PE_REDUCE_ISSUE;

                    end

                end


                /*
                 * ---------------------------------------------
                 * Issue one local reduction ADD.
                 * ---------------------------------------------
                 */
                PE_REDUCE_ISSUE: begin

                    state <=
                        PE_REDUCE_WAIT;

                end


                /*
                 * ---------------------------------------------
                 * Wait for that ADD to return.
                 * ---------------------------------------------
                 */
                PE_REDUCE_WAIT: begin

                    if (add_valid) begin


                        /*
                         * More banks remain in the current context.
                         */
                        if (
                            reduce_index !=
                            ACC_BANKS-1
                        ) begin

                            reduce_running_sum <=
                                add_result;

                            reduce_index <=
                                reduce_index + 1'b1;

                            state <=
                                PE_REDUCE_ISSUE;

                        end


                        /*
                         * Current context is complete.
                         */
                        else begin


                            /*
                             * ctx0 finished.
                             */
                            if (reduce_ctx == 1'b0) begin

                                result_ctx0 <=
                                    add_result;

                                /*
                                 * Immediately begin ctx1.
                                 */
                                reduce_ctx <=
                                    1'b1;

                                reduce_index <=
                                    4'd1;

                                reduce_running_sum <=
                                    acc_bank[1][0];

                                state <=
                                    PE_REDUCE_ISSUE;

                            end


                            /*
                             * ctx1 finished: this PE now owns
                             * two final scalar results.
                             */
                            else begin

                                result_ctx1 <=
                                    add_result;

                                result_valid <=
                                    1'b1;

                                state <=
                                    PE_ACCUM;

                            end

                        end

                    end

                end


                default: begin

                    state <=
                        PE_ACCUM;

                end

            endcase

        end

    end


endmodule