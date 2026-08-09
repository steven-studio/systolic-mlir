module systolic_pe_fold #(
    parameter int DATA_W      = 32,
    parameter int ACC_BANKS   = 16,
    parameter int MUL_LATENCY = 9,

    /*
     * Kept temporarily for source compatibility.
     *
     * IMPORTANT:
     * This implementation does NOT use ADD_LATENCY for
     * metadata alignment anymore.
     */
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
     * PE state
     * ============================================================
     */

    typedef enum logic [1:0] {
        PE_ACCUM,
        PE_REDUCE_ISSUE,
        PE_REDUCE_WAIT
    } pe_state_t;

    pe_state_t state;


    /*
     * ============================================================
     * Stage 0: PE-to-PE pipeline registers
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
        a_valid_in &&
        b_valid_in;

    wire pipe_pair_valid =
        a_valid_reg &&
        b_valid_reg;


    always_ff @(posedge clk) begin

        if (rst) begin

            a_reg <=
                '0;

            b_reg <=
                '0;

            a_valid_reg <=
                1'b0;

            b_valid_reg <=
                1'b0;

            fold_ctx_reg <=
                1'b0;

            sel_reg <=
                '0;

            local_acc_sel[0] <=
                '0;

            local_acc_sel[1] <=
                '0;

        end
        else begin

            a_valid_reg <=
                a_valid_in;

            b_valid_reg <=
                b_valid_in;


            if (a_valid_in)
                a_reg <=
                    a_in;

            if (b_valid_in)
                b_reg <=
                    b_in;


            if (pair_valid) begin

                fold_ctx_reg <=
                    fold_ctx_in;

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
     * FP MUL transaction metadata FIFO
     *
     * Metadata is pushed for every transaction entering fp_mul
     * and popped when the corresponding product_valid returns.
     *
     * This avoids depending on the implementation latency of the
     * floating-point multiplier.
     * ============================================================
     */

    localparam int MUL_META_DEPTH = 32;
    localparam int MUL_META_PTR_W = $clog2(MUL_META_DEPTH);

    logic [3:0]
        mul_meta_sel [0:MUL_META_DEPTH-1];

    logic
        mul_meta_ctx [0:MUL_META_DEPTH-1];

    logic [MUL_META_PTR_W-1:0]
        mul_meta_wr_ptr;

    logic [MUL_META_PTR_W-1:0]
        mul_meta_rd_ptr;

    logic [MUL_META_PTR_W:0]
        mul_meta_count;


    /*
     * Oldest outstanding multiplier transaction belongs to the
     * product currently returned by fp_mul.
     */
    wire [3:0] product_sel =
        mul_meta_sel[mul_meta_rd_ptr];

    wire product_ctx =
        mul_meta_ctx[mul_meta_rd_ptr];


    always_ff @(posedge clk) begin

        if (rst) begin

            mul_meta_wr_ptr <=
                '0;

            mul_meta_rd_ptr <=
                '0;

            mul_meta_count <=
                '0;

        end
        else begin

            /*
             * Push metadata belonging to the exact A/B transaction
             * entering fp_mul.
             */
            if (pipe_pair_valid) begin

                mul_meta_sel[mul_meta_wr_ptr] <=
                    sel_reg;

                mul_meta_ctx[mul_meta_wr_ptr] <=
                    fold_ctx_reg;

                mul_meta_wr_ptr <=
                    mul_meta_wr_ptr + 1'b1;

            end


            /*
             * fp_mul preserves transaction ordering.
             * Pop the metadata corresponding to this product.
             */
            if (product_valid) begin

                mul_meta_rd_ptr <=
                    mul_meta_rd_ptr + 1'b1;

            end


            case ({
                pipe_pair_valid,
                product_valid
            })

                2'b10:
                    mul_meta_count <=
                        mul_meta_count + 1'b1;

                2'b01:
                    mul_meta_count <=
                        mul_meta_count - 1'b1;

                default:
                    mul_meta_count <=
                        mul_meta_count;

            endcase

        end

    end


    /*
     * ============================================================
     * Two accumulator contexts
     * ============================================================
     */

    logic [DATA_W-1:0]
        acc_bank [0:1][0:ACC_BANKS-1];

    logic [DATA_W-1:0]
        selected_acc;


    always_comb begin

        selected_acc =
            acc_bank
                [product_ctx]
                [product_sel];

    end


    /*
     * ============================================================
     * Local result reduction state
     *
     * The same FP adder used by accumulation is reused after all
     * MAC traffic has drained.
     * ============================================================
     */

    logic reduce_ctx;

    logic [3:0]
        reduce_index;

    logic [DATA_W-1:0]
        reduce_running_sum;


    /*
     * ============================================================
     * Shared FP adder input mux
     * ============================================================
     */

    logic
        fp_add_valid_in;

    logic [DATA_W-1:0]
        fp_add_a;

    logic [DATA_W-1:0]
        fp_add_b;

    logic [DATA_W-1:0]
        add_result;

    logic
        add_valid;


    always_comb begin

        fp_add_valid_in =
            1'b0;

        fp_add_a =
            '0;

        fp_add_b =
            '0;


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
         * Exactly one add request is issued while the FSM is in
         * PE_REDUCE_ISSUE.
         */
        else if (state == PE_REDUCE_ISSUE) begin

            fp_add_valid_in =
                1'b1;

            fp_add_a =
                reduce_running_sum;

            fp_add_b =
                acc_bank
                    [reduce_ctx]
                    [reduce_index];

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
     * FP ADD transaction metadata FIFO
     *
     * Every request sent to fp_add pushes one metadata entry.
     *
     * Every add_valid pops exactly one metadata entry.
     *
     * This means the PE no longer needs to know or guess the
     * floating-point ADD latency.
     * ============================================================
     */

    localparam int ADD_META_DEPTH =
        32;

    localparam int ADD_META_PTR_W =
        $clog2(ADD_META_DEPTH);


    logic
        add_meta_is_mac [0:ADD_META_DEPTH-1];

    logic [3:0]
        add_meta_sel [0:ADD_META_DEPTH-1];

    logic
        add_meta_ctx [0:ADD_META_DEPTH-1];


    logic [ADD_META_PTR_W-1:0]
        add_meta_wr_ptr;

    logic [ADD_META_PTR_W-1:0]
        add_meta_rd_ptr;

    logic [ADD_META_PTR_W:0]
        add_meta_count;


    /*
     * The current read pointer identifies metadata belonging to
     * the add_result currently returning when add_valid == 1.
     *
     * These signals are meaningless when add_valid == 0 and are
     * therefore always consumed together with add_valid.
     */
    wire add_result_is_mac =
        add_meta_is_mac[add_meta_rd_ptr];

    wire [3:0] writeback_sel =
        add_meta_sel[add_meta_rd_ptr];

    wire writeback_ctx =
        add_meta_ctx[add_meta_rd_ptr];


    always_ff @(posedge clk) begin

        if (rst) begin

            add_meta_wr_ptr <=
                '0;

            add_meta_rd_ptr <=
                '0;

            add_meta_count <=
                '0;

        end
        else begin

            /*
             * Push metadata for every operation actually issued
             * into fp_add.
             */
            if (fp_add_valid_in) begin

                add_meta_is_mac[add_meta_wr_ptr] <=
                    (state == PE_ACCUM);

                /*
                 * sel/ctx are relevant only for MAC operations.
                 * For reduction entries they are don't-care.
                 */
                add_meta_sel[add_meta_wr_ptr] <=
                    product_sel;

                add_meta_ctx[add_meta_wr_ptr] <=
                    product_ctx;

                add_meta_wr_ptr <=
                    add_meta_wr_ptr + 1'b1;

            end


            /*
             * FP ADD preserves transaction ordering.
             * Therefore each returned result consumes the oldest
             * outstanding metadata entry.
             */
            if (add_valid) begin

                add_meta_rd_ptr <=
                    add_meta_rd_ptr + 1'b1;

            end


            /*
             * FIFO occupancy.
             */
            case ({
                fp_add_valid_in,
                add_valid
            })

                2'b10:
                    add_meta_count <=
                        add_meta_count + 1'b1;

                2'b01:
                    add_meta_count <=
                        add_meta_count - 1'b1;

                default:
                    add_meta_count <=
                        add_meta_count;

            endcase

        end

    end


    /*
     * ============================================================
     * Detect complete MAC transaction drain
     * ============================================================
     */

    logic
        pipe_pair_valid_d;

    logic
        transaction_seen;

    logic
        input_finished;

    /*
     * Number of MAC operations that entered fp_add but whose
     * corresponding MAC result has not returned yet.
     */
    logic [7:0]
        outstanding_adds;

    logic
        clear_acc_banks;


    always_ff @(posedge clk) begin

        if (rst) begin

            pipe_pair_valid_d <=
                1'b0;

            transaction_seen <=
                1'b0;

            input_finished <=
                1'b0;

            outstanding_adds <=
                '0;

        end
        else if (state == PE_ACCUM) begin

            pipe_pair_valid_d <=
                pipe_pair_valid;


            if (pipe_pair_valid) begin

                transaction_seen <=
                    1'b1;

            end


            /*
             * Falling edge of the local pair-valid stream marks
             * completion of input injection into this PE.
             */
            if (
                transaction_seen &&
                pipe_pair_valid_d &&
                !pipe_pair_valid
            ) begin

                input_finished <=
                    1'b1;

            end


            /*
             * MAC accounting.
             *
             * Increment:
             *   a multiplier result enters the ADD as a MAC.
             *
             * Decrement:
             *   the FIFO tells us that this returned ADD result
             *   belongs to a MAC transaction.
             */
            case ({
                product_valid,
                add_valid &&
                add_result_is_mac
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
             * MAC accounting is not used during local reduction.
             */
            outstanding_adds <=
                '0;

        end


        /*
         * Completion of ctx1 reduction ends the current matrix
         * transaction and prepares this PE for the next one.
         */
        if (clear_acc_banks) begin

            transaction_seen <=
                1'b0;

            input_finished <=
                1'b0;

            outstanding_adds <=
                '0;

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

                    acc_bank[ctx_i][bank_i] <=
                        '0;

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

                    acc_bank[ctx_i][bank_i] <=
                        '0;

                end

            end

        end
        else if (
            add_valid &&
            add_result_is_mac
        ) begin

            acc_bank
                [writeback_ctx]
                [writeback_sel]
                    <= add_result;

        end

    end


    /*
     * ============================================================
     * Final-result completion condition
     * ============================================================
     *
     * clear_acc_banks is asserted exactly when the final ctx1
     * reduction ADD returns.
     * ============================================================
     */

    always_comb begin

        clear_acc_banks =
            (
                state == PE_REDUCE_WAIT &&
                add_valid &&
                !add_result_is_mac &&
                reduce_ctx == 1'b1 &&
                reduce_index == ACC_BANKS-1
            );

    end


    /*
     * ============================================================
     * Final-result FSM
     * ============================================================
     */

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
             */
            result_valid <=
                1'b0;


            case (state)


                /*
                 * ------------------------------------------------
                 * Normal systolic accumulation
                 * ------------------------------------------------
                 */
                PE_ACCUM: begin

                    /*
                     * Start final reduction only after:
                     *
                     *  1. local input injection ended,
                     *  2. every MAC ADD returned,
                     *  3. multiplier output is empty,
                     *  4. ADD output is currently empty.
                     */
                    if (
                        input_finished &&
                        outstanding_adds == 0 &&
                        !product_valid &&
                        !add_valid
                    ) begin

                        /*
                         * Begin ctx0 reduction.
                         *
                         * Bank 0 becomes the initial running sum,
                         * so the first ADD consumes bank 1.
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
                 * ------------------------------------------------
                 * Issue exactly one reduction ADD
                 * ------------------------------------------------
                 */
                PE_REDUCE_ISSUE: begin

                    /*
                     * fp_add_valid_in is combinationally asserted
                     * while in this state.
                     *
                     * Move immediately to WAIT so exactly one
                     * request is issued.
                     */
                    state <=
                        PE_REDUCE_WAIT;

                end


                /*
                 * ------------------------------------------------
                 * Wait for that reduction ADD result
                 * ------------------------------------------------
                 */
                PE_REDUCE_WAIT: begin

                    /*
                     * Only consume ADD results whose FIFO metadata
                     * says they belong to reduction.
                     */
                    if (
                        add_valid &&
                        !add_result_is_mac
                    ) begin


                        /*
                         * More banks remain in this context.
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
                         * Current context reduction is complete.
                         */
                        else begin


                            /*
                             * ctx0 completed.
                             */
                            if (reduce_ctx == 1'b0) begin

                                result_ctx0 <=
                                    add_result;

                                /*
                                 * Immediately prepare ctx1.
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
                             * ctx1 completed.
                             *
                             * Both scalar results owned by this PE
                             * are now valid.
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