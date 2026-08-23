module systolic_pe #(
    parameter int DATA_W      = 32,
    parameter int ACC_BANKS   = 16,
    parameter int MUL_LATENCY = 9,
    parameter int ADD_LATENCY = 12
) (
    input  logic              clk,
    input  logic              rst,

    input  logic              a_valid_in,
    input  logic              b_valid_in,
    input  logic [3:0]        acc_sel,

    input  logic [DATA_W-1:0] a_in,
    input  logic [DATA_W-1:0] b_in,

    output logic              a_valid_out,
    output logic              b_valid_out,
    output logic [DATA_W-1:0] a_out,
    output logic [DATA_W-1:0] b_out,

    output logic [DATA_W-1:0] dbg_acc [0:ACC_BANKS-1]
);

    /*
     * ============================================================
     * Stage 0: PE-to-PE registers
     * ============================================================
     */
    logic [DATA_W-1:0] a_reg;
    logic [DATA_W-1:0] b_reg;

    logic              a_valid_reg;
    logic              b_valid_reg;

    logic [3:0]        sel_reg;
    logic [3:0]        local_acc_sel;

    wire pair_valid = a_valid_in && b_valid_in;

    always_ff @(posedge clk) begin
        if (rst) begin
            a_reg     <= '0;
            b_reg     <= '0;
            a_valid_reg <= 1'b0;
            b_valid_reg <= 1'b0;
            sel_reg   <= '0;
            local_acc_sel <= '0;
        end
        else begin
            a_valid_reg <= a_valid_in;
            b_valid_reg <= b_valid_in;

            if (a_valid_in)
                a_reg   <= a_in;

            if (b_valid_in)
                b_reg   <= b_in;

            if (pair_valid) begin
                sel_reg <= local_acc_sel;
                local_acc_sel <= local_acc_sel + 1'b1;
            end
        end
    end

    assign a_out     = a_reg;
    assign b_out     = b_reg;
    assign a_valid_out = a_valid_reg;
    assign b_valid_out = b_valid_reg;


    /*
     * ============================================================
     * FP multiplier
     *
     * Vivado IP:
     *   Operation = Multiply
     *   Rate      = 1
     *   Latency   = 9
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
     * Delay acc_sel alongside the multiplier.
     */
    logic [3:0] mul_sel_pipe [0:MUL_LATENCY-1];

    integer m;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (m = 0; m < MUL_LATENCY; m = m + 1)
                mul_sel_pipe[m] <= '0;
        end
        else begin
            mul_sel_pipe[0] <= sel_reg;

            for (m = 1; m < MUL_LATENCY; m = m + 1)
                mul_sel_pipe[m] <= mul_sel_pipe[m-1];
        end
    end

    wire [3:0] product_sel =
        mul_sel_pipe[MUL_LATENCY-2];


    /*
     * ============================================================
     * 16 rotating accumulator banks
     *
     * Same bank is reused every 16 input beats.
     *
     * ADD latency is 12, therefore:
     *
     *     reuse distance 16 > feedback latency 12
     *
     * so previous result has returned before that bank is
     * selected again.
     * ============================================================
     */
    logic [DATA_W-1:0] acc_bank [0:ACC_BANKS-1];


    /*
     * Select current accumulator value for the product.
     *
     * This is a 16:1 mux. For the first correctness/timing
     * experiment we accept it explicitly and let Vivado place
     * registers around the FP IP.
     */
    logic [DATA_W-1:0] selected_acc;

    always_comb begin
        selected_acc = acc_bank[product_sel];
    end


    /*
     * ============================================================
     * ONE pipelined FP adder
     *
     * Vivado IP:
     *   Operation = Add
     *   Rate      = 1
     *   Latency   = 12
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
     * The destination bank must travel through the adder
     * pipeline as well.
     */
    logic [3:0] add_sel_pipe [0:ADD_LATENCY-1];

    integer a;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (a = 0; a < ADD_LATENCY; a = a + 1)
                add_sel_pipe[a] <= '0;
        end
        else begin
            add_sel_pipe[0] <= product_sel;

            for (a = 1; a < ADD_LATENCY; a = a + 1)
                add_sel_pipe[a] <= add_sel_pipe[a-1];
        end
    end

    wire [3:0] writeback_sel =
        add_sel_pipe[ADD_LATENCY-2];


    /*
     * ============================================================
     * Accumulator writeback
     * ============================================================
     */
    integer b;

    always_ff @(posedge clk) begin
        if (rst) begin
            for (b = 0; b < ACC_BANKS; b = b + 1)
                acc_bank[b] <= '0;
        end
        else begin
            if (add_valid)
                acc_bank[writeback_sel] <= add_result;
        end
    end

    genvar g;
    generate
        for (g = 0; g < ACC_BANKS; g = g + 1) begin : DBG_ACC
            assign dbg_acc[g] = acc_bank[g];
        end
    endgenerate

// `ifndef SYNTHESIS

// integer dbg_cycle = 0;

// always_ff @(posedge clk) begin
//     if (rst) begin
//         dbg_cycle <= 0;
//     end
//     else begin

//         $display(
//             "[CYCLE %0d] avin=%b bvin=%b sel_in=%0d | avreg=%b bvreg=%b sel_reg=%0d | pvalid=%b psel=%0d product=%h | addvalid=%b wbsel=%0d add=%h",
//             dbg_cycle,
//             a_valid_in,
//             b_valid_in,
//             acc_sel,
//             a_valid_reg,
//             b_valid_reg,
//             sel_reg,
//             product_valid,
//             product_sel,
//             product,
//             add_valid,
//             writeback_sel,
//             add_result
//         );

//         if (add_valid) begin
//             $display(
//                 "    >>> WRITE bank[%0d] <= %h",
//                 writeback_sel,
//                 add_result
//             );
//         end

//         dbg_cycle <= dbg_cycle + 1;
//     end
// end

// `endif

endmodule