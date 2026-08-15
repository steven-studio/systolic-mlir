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
        PE_REDUCE_DRAIN
    } pe_state_t;

    pe_state_t state;

    /*
    * Accumulator-bank selector width.
    *
    * ACC_BANKS = 16 -> ACC_SEL_W = 4
    * bank index = 0..15
    */
    localparam int ACC_SEL_W = $clog2(ACC_BANKS);


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

    logic [ACC_SEL_W-1:0] sel_reg;
    logic [ACC_SEL_W-1:0] local_acc_sel [0:1];

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

                if (local_acc_sel[fold_ctx_in] == ACC_BANKS-1)
                    local_acc_sel[fold_ctx_in] <= '0;
                else
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

    logic [ACC_SEL_W-1:0]
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
    wire [ACC_SEL_W-1:0] product_sel =
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

    /*
     * Tree reduction state.
     *
     * reduce_stride walks ACC_BANKS/2, ACC_BANKS/4, ... 1, then 0.
     * Within one stride every add is independent, so they are
     * issued back to back into the pipelined adder instead of
     * one-at-a-time.
     *
     *   acc[ctx][i] <= acc[ctx][i] + acc[ctx][i + stride]
     *
     * The add count is unchanged (ACC_BANKS-1 per context); only
     * the dependency chain between them is broken.
     */
    logic [ACC_SEL_W-1:0]
        reduce_stride;

    logic [ACC_SEL_W-1:0]
        reduce_i;

    /*
     * reduce_j is maintained to satisfy, at every point where the
     * reduction path reads a bank:
     *
     *     reduce_j == reduce_i + reduce_stride
     *
     * Keeping it as state instead of recomputing it combinationally
     * takes the 4-bit adder off the path that feeds the bank mux
     * select. Cost is ACC_SEL_W flops per PE; cycle count is
     * unchanged.
     */
    logic [ACC_SEL_W-1:0]
        reduce_j;

    /*
     * Reduction adds issued into fp_add whose result has not yet
     * returned. MAC uses outstanding_adds; the two never overlap
     * because reduction only starts after MAC has fully drained.
     */
    logic [7:0]
        reduce_outstanding;

    /*
     * High while PE_REDUCE_ISSUE still has an add to send this
     * cycle. Also gates the metadata push.
     */
    wire reduce_issue_fire =
        (state == PE_REDUCE_ISSUE);


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
                acc_bank
                    [reduce_ctx]
                    [reduce_i];

            fp_add_b =
                acc_bank
                    [reduce_ctx]
                    [reduce_j];

        end

    end


`ifndef SYNTHESIS
    /*
     * reduce_j is redundant state. If it ever drifts from the value
     * it stands in for, the reduction silently reads the wrong bank
     * and the result is merely wrong rather than obviously broken.
     * Check it every cycle a bank is actually read.
     */
    always_ff @(posedge clk) begin
        if (!rst && state == PE_REDUCE_ISSUE) begin
            if (reduce_j !== (reduce_i + reduce_stride)) begin
                $error({"reduce_j desync: j=%0d i=%0d stride=%0d ",
                        "expected=%0d"},
                       reduce_j, reduce_i, reduce_stride,
                       (reduce_i + reduce_stride));
            end
        end
    end
`endif


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

    logic [ACC_SEL_W-1:0]
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

    wire [ACC_SEL_W-1:0] writeback_sel =
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
                /*
                 * MAC writes back to the bank the product came
                 * from; a reduction add writes back to the low
                 * half of the pair it just consumed. Both use the
                 * same FIFO, so neither path needs to know the
                 * adder latency.
                 */
                add_meta_sel[add_meta_wr_ptr] <=
                    (state == PE_ACCUM) ? product_sel : reduce_i;

                add_meta_ctx[add_meta_wr_ptr] <=
                    (state == PE_ACCUM) ? product_ctx : reduce_ctx;

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
        else if (add_valid) begin

            /*
             * Writeback is now identical for MAC and reduction:
             * the FIFO carries the destination for both.
             */
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
                state == PE_REDUCE_DRAIN &&
                reduce_outstanding == 0 &&
                reduce_stride == ACC_SEL_W'(1)
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

            reduce_stride <=
                '0;

            reduce_i <=
                '0;

            reduce_j <=
                '0;

            reduce_outstanding <=
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


            /*
             * ----------------------------------------------------
             * Reduction-add accounting
             *
             * Increment when an add is issued from
             * PE_REDUCE_ISSUE, decrement when the FIFO says the
             * returning result belongs to a reduction.
             *
             * This is what lets a whole stride be issued back to
             * back: the FSM waits on a count reaching zero rather
             * than on one specific result.
             * ----------------------------------------------------
             */
            case ({
                reduce_issue_fire,
                add_valid &&
                !add_result_is_mac
            })

                2'b10:
                    reduce_outstanding <=
                        reduce_outstanding + 1'b1;

                2'b01:
                    reduce_outstanding <=
                        reduce_outstanding - 1'b1;

                default:
                    reduce_outstanding <=
                        reduce_outstanding;

            endcase


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
                     *  2. the multiplier holds no transaction,
                     *  3. every MAC ADD returned,
                     *  4. multiplier output is empty this cycle,
                     *  5. ADD output is empty this cycle.
                     *
                     * Condition 2 is not redundant: input_finished
                     * rises a few cycles after this PE's last
                     * operand pair, but the first product only
                     * leaves fp_mul after MUL_LATENCY, so a short
                     * reduction can see conditions 1/3/4/5 all true
                     * with nothing computed yet. mul_meta_count
                     * tracks exactly that.
                     */
                    if (
                        input_finished &&
                        mul_meta_count == 0 &&
                        outstanding_adds == 0 &&
                        !product_valid &&
                        !add_valid
                    ) begin

                        /*
                         * Begin the widest stride of the tree.
                         * ACC_BANKS=16 -> stride 8, 4, 2, 1.
                         */
                        reduce_stride <=
                            ACC_SEL_W'(ACC_BANKS / 2);

                        reduce_ctx <=
                            1'b0;

                        reduce_i <=
                            '0;

                        reduce_j <=
                            ACC_SEL_W'(ACC_BANKS / 2);

                        state <=
                            PE_REDUCE_ISSUE;

                    end

                end


                /*
                 * ------------------------------------------------
                 * Issue every add of the current stride, one per
                 * cycle, without waiting for any of them.
                 *
                 * Order: ctx0 i=0..stride-1, then ctx1 i=0..stride-1.
                 * That is 2*stride adds per level, all independent.
                 * ------------------------------------------------
                 */
                PE_REDUCE_ISSUE: begin

                    if (reduce_i == reduce_stride - 1'b1) begin

                        if (reduce_ctx == 1'b0) begin

                            reduce_ctx <=
                                1'b1;

                            reduce_i <=
                                '0;

                            reduce_j <=
                                reduce_stride;

                        end
                        else begin

                            state <=
                                PE_REDUCE_DRAIN;

                        end

                    end
                    else begin

                        reduce_i <=
                            reduce_i + 1'b1;

                        reduce_j <=
                            reduce_j + 1'b1;

                    end

                end


                /*
                 * ------------------------------------------------
                 * Wait for the whole stride to land before reading
                 * the banks again.
                 *
                 * This barrier is what makes the in-place update
                 * safe: within one stride every index is read once
                 * and written once, but the next stride reads
                 * results this one produced.
                 * ------------------------------------------------
                 */
                PE_REDUCE_DRAIN: begin

                    if (reduce_outstanding == 0) begin

                        if (reduce_stride == ACC_SEL_W'(1)) begin

                            /*
                             * Tree complete. Bank 0 of each context
                             * holds the sum of that context.
                             *
                             * clear_acc_banks is asserted combinationally
                             * on this same cycle; both reads below take
                             * the pre-clear values.
                             */
                            result_ctx0 <=
                                acc_bank[0][0];

                            result_ctx1 <=
                                acc_bank[1][0];

                            result_valid <=
                                1'b1;

                            state <=
                                PE_ACCUM;

                        end
                        else begin

                            reduce_stride <=
                                reduce_stride >> 1;

                            reduce_ctx <=
                                1'b0;

                            reduce_i <=
                                '0;

                            reduce_j <=
                                reduce_stride >> 1;

                            state <=
                                PE_REDUCE_ISSUE;

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
