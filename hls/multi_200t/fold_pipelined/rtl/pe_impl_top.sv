module pe_impl_top (
    input  logic        clk,
    input  logic        rst,

    input  logic        valid_in,
    input  logic [3:0]  acc_sel,

    input  logic [31:0] a_in,
    input  logic [31:0] b_in,

    output logic        valid_out,
    output logic [31:0] a_out,
    output logic [31:0] b_out,

    // Observable value derived from ALL accumulator banks.
    // This prevents Vivado from sweeping away the FP datapath.
    output logic [31:0] acc_checksum
);

    logic [31:0] dbg_acc [0:15];

    /*
     * Keep hierarchy mainly to make reports/debugging easier.
     * The important part for preventing dead-code elimination
     * is that dbg_acc[] reaches acc_checksum below.
     */
    (* KEEP_HIERARCHY = "yes" *)
    systolic_pe #(
        .DATA_W      (32),
        .ACC_BANKS   (16),
        .MUL_LATENCY (9),
        .ADD_LATENCY (12)
    ) u_pe (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (valid_in),
        .acc_sel   (acc_sel),

        .a_in      (a_in),
        .b_in      (b_in),

        .valid_out (valid_out),
        .a_out     (a_out),
        .b_out     (b_out),

        .dbg_acc   (dbg_acc)
    );


    /*
     * XOR reduction of all 16 accumulator banks.
     *
     * Keep this combinational:
     *   - every accumulator bit becomes observable
     *   - Vivado cannot remove the accumulator datapath
     *   - no extra registered timing path is introduced
     */
    integer i;

    always_comb begin
        acc_checksum = 32'b0;

        for (i = 0; i < 16; i = i + 1)
            acc_checksum = acc_checksum ^ dbg_acc[i];
    end

endmodule