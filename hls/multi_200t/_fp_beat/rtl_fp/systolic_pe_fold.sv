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

    output logic [DATA_W-1:0] dbg_acc_ctx0 [0:ACC_BANKS-1],
    output logic [DATA_W-1:0] dbg_acc_ctx1 [0:ACC_BANKS-1]
);

    /*
     * ============================================================
     * Stage 0: PE-to-PE registers
     * ============================================================
     */
    logic [DATA_W-1:0] a_reg;
    logic [DATA_W-1:0] b_reg;

    logic a_valid_reg;
    logic b_valid_reg;

    logic fold_ctx_reg;

    logic [3:0] sel_reg;
    logic [3:0] local_acc_sel;

    wire pair_valid = a_valid_in && b_valid_in;

    always_ff @(posedge clk) begin
        if (rst) begin
            a_reg         <= '0;
            b_reg         <= '0;

            a_valid_reg   <= 1'b0;
            b_valid_reg   <= 1'b0;

            fold_ctx_reg  <= 1'b0;

            sel_reg       <= '0;
            local_acc_sel <= '0;
        end
        else begin
            a_valid_reg <= a_valid_in;
            b_valid_reg <= b_valid_in;

            if (a_valid_in)
                a_reg <= a_in;

            if (b_valid_in)
                b_reg <= b_in;

            if (pair_valid) begin
                fold_ctx_reg <= fold_ctx_in;

                sel_reg       <= local_acc_sel;
                local_acc_sel <= local_acc_sel + 1'b1;
            end
        end
    end

    assign a_out        = a_reg;
    assign b_out        = b_reg;
    assign a_valid_out  = a_valid_reg;
    assign b_valid_out  = b_valid_reg;
    assign fold_ctx_out = fold_ctx_reg;


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

        .valid_in  (a_valid_reg && b_valid_reg),
        .a         (a_reg),
        .b         (b_reg),

        .valid_out (product_valid),
        .result    (product)
    );


    /*
     * Delay bank selector through multiplier pipeline.
     */
    logic [3:0] mul_sel_pipe [0:MUL_LATENCY-1];

    /*
     * Delay fold context through multiplier pipeline.
     */
    logic mul_ctx_pipe [0:MUL_LATENCY-1];

    integer m;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (m = 0; m < MUL_LATENCY; m = m + 1) begin
                mul_sel_pipe[m] <= '0;
                mul_ctx_pipe[m] <= 1'b0;
            end
        end
        else begin
            mul_sel_pipe[0] <= sel_reg;
            mul_ctx_pipe[0] <= fold_ctx_reg;

            for (m = 1; m < MUL_LATENCY; m = m + 1) begin
                mul_sel_pipe[m] <= mul_sel_pipe[m-1];
                mul_ctx_pipe[m] <= mul_ctx_pipe[m-1];
            end
        end
    end

    wire [3:0] product_sel =
        mul_sel_pipe[MUL_LATENCY-2];

    wire product_ctx =
        mul_ctx_pipe[MUL_LATENCY-2];


    /*
     * ============================================================
     * Two accumulator contexts.
     *
     * Context 0 and context 1 are independent output-fold states.
     * Each context still has 16 rotating banks to hide FP ADD
     * feedback latency.
     * ============================================================
     */
    logic [DATA_W-1:0] acc_bank [0:1][0:ACC_BANKS-1];

    logic [DATA_W-1:0] selected_acc;

    always_comb begin
        selected_acc = acc_bank[product_ctx][product_sel];
    end


    /*
     * ============================================================
     * ONE pipelined FP adder
     * ============================================================
     */
    logic [DATA_W-1:0] add_result;
    logic              add_valid;

    fp_add u_fp_add (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (product_valid),
        .a         (selected_acc),
        .b         (product),

        .valid_out (add_valid),
        .result    (add_result)
    );


    /*
     * Delay destination bank + fold context through adder.
     */
    logic [3:0] add_sel_pipe [0:ADD_LATENCY-1];
    logic       add_ctx_pipe [0:ADD_LATENCY-1];

    integer a;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (a = 0; a < ADD_LATENCY; a = a + 1) begin
                add_sel_pipe[a] <= '0;
                add_ctx_pipe[a] <= 1'b0;
            end
        end
        else begin
            add_sel_pipe[0] <= product_sel;
            add_ctx_pipe[0] <= product_ctx;

            for (a = 1; a < ADD_LATENCY; a = a + 1) begin
                add_sel_pipe[a] <= add_sel_pipe[a-1];
                add_ctx_pipe[a] <= add_ctx_pipe[a-1];
            end
        end
    end

    wire [3:0] writeback_sel =
        add_sel_pipe[ADD_LATENCY-2];

    wire writeback_ctx =
        add_ctx_pipe[ADD_LATENCY-2];


    /*
     * ============================================================
     * Accumulator writeback
     * ============================================================
     */
    integer c;
    integer b;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (c = 0; c < 2; c = c + 1) begin
                for (b = 0; b < ACC_BANKS; b = b + 1) begin
                    acc_bank[c][b] <= '0;
                end
            end
        end
        else begin
            if (add_valid)
                acc_bank[writeback_ctx][writeback_sel] <= add_result;
        end
    end


    /*
     * ============================================================
     * Debug export
     * ============================================================
     */
    genvar g;

    generate
        for (g = 0; g < ACC_BANKS; g = g + 1) begin : DBG_ACC
            assign dbg_acc_ctx0[g] = acc_bank[0][g];
            assign dbg_acc_ctx1[g] = acc_bank[1][g];
        end
    endgenerate

endmodule