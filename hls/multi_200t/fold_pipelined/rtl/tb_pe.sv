`timescale 1ns/1ps

module tb_pe;

    localparam int DATA_W    = 32;
    localparam int ACC_BANKS = 16;

    logic clk;
    logic rst;

    logic valid_in;
    logic [3:0] acc_sel;

    logic [31:0] a_in;
    logic [31:0] b_in;

    logic valid_out;
    logic [31:0] a_out;
    logic [31:0] b_out;

    logic [31:0] dbg_acc [0:ACC_BANKS-1];

    systolic_pe dut (
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
     * 100 MHz
     */
    initial clk = 1'b0;
    always #5 clk = ~clk;


    integer i;
    integer errors;

    initial begin
        rst      = 1'b1;
        valid_in = 1'b0;
        acc_sel  = 4'd0;
        a_in     = 32'h00000000;
        b_in     = 32'h00000000;
        errors   = 0;

        /*
         * Hold reset for a few cycles.
         */
        repeat (5)
            @(posedge clk);

        rst = 1'b0;

        @(posedge clk);


        /*
         * Send 32 consecutive beats.
         *
         * IEEE-754:
         *
         *   1.0f = 0x3f800000
         *
         * acc_sel:
         *
         *   0,1,...15,
         *   0,1,...15
         *
         * Therefore each bank receives:
         *
         *   1*1 + 1*1 = 2.0
         */
        for (i = 0; i < 32; i = i + 1) begin
            valid_in <= 1'b1;
            acc_sel  <= i[3:0];

            a_in <= 32'h3f800000;
            b_in <= 32'h3f800000;

            $display(
                "[INPUT] cycle=%0d sel=%0d a=1.0 b=1.0",
                i,
                i[3:0]
            );

            @(posedge clk);
        end


        /*
         * Stop issuing inputs.
         */
        valid_in <= 1'b0;
        a_in     <= 32'h00000000;
        b_in     <= 32'h00000000;


        /*
         * Drain the whole datapath.
         *
         * Stage-0 register = 1
         * MUL              = 9
         * ADD              = 12
         *
         * Give generous margin.
         */
        repeat (40)
            @(posedge clk);


        $display("");
        $display("========================================");
        $display("ACCUMULATOR BANK RESULTS");
        $display("========================================");


        /*
         * IEEE-754 2.0f = 0x40000000
         */
        for (i = 0; i < ACC_BANKS; i = i + 1) begin

            $display(
                "bank[%0d] = 0x%08h",
                i,
                dbg_acc[i]
            );

            if (dbg_acc[i] !== 32'h40000000) begin
                $display(
                    "ERROR: bank[%0d] expected 2.0 (0x40000000)",
                    i
                );

                errors = errors + 1;
            end
        end


        $display("");

        if (errors == 0) begin
            $display("========================================");
            $display("PE ROTATING-ACCUMULATOR TEST PASSED");
            $display("========================================");
        end
        else begin
            $display("========================================");
            $display("PE TEST FAILED: %0d errors", errors);
            $display("========================================");
        end

        $finish;
    end

endmodule
