`timescale 1ns/1ps

module tb_pe_ii1;

    logic clk;
    logic rst;

    logic a_valid_in;
    logic b_valid_in;
    logic [3:0] acc_sel;

    logic [31:0] a_in;
    logic [31:0] b_in;

    logic a_valid_out;
    logic b_valid_out;
    logic [31:0] a_out;
    logic [31:0] b_out;

    logic [31:0] dbg_acc [0:15];

    systolic_pe dut (
        .clk         (clk),
        .rst         (rst),

        .a_valid_in  (a_valid_in),
        .b_valid_in  (b_valid_in),
        .acc_sel     (acc_sel),

        .a_in        (a_in),
        .b_in        (b_in),

        .a_valid_out (a_valid_out),
        .b_valid_out (b_valid_out),
        .a_out       (a_out),
        .b_out       (b_out),

        .dbg_acc     (dbg_acc)
    );

    initial clk = 0;
    always #5 clk = ~clk;

    integer cycle;
    integer write_count;

    logic [3:0] prev_sel;
    logic seen_write;

    initial begin
        cycle       = 0;
        write_count = 0;
        seen_write  = 0;

        rst         = 1;
        a_valid_in  = 0;
        b_valid_in  = 0;
        acc_sel     = 0;
        a_in        = 32'h3f800000; // 1.0
        b_in        = 32'h3f800000; // 1.0

        repeat (2) @(posedge clk);

        @(negedge clk);
        rst = 0;

        // 連續 64 beats，完全不插 bubble
        repeat (64) begin
            @(negedge clk);
            a_valid_in = 1;
            b_valid_in = 1;
        end

        @(negedge clk);
        a_valid_in = 0;
        b_valid_in = 0;

        // 等 pipeline drain
        repeat (40) @(posedge clk);

        $display("");
        $display("Total writebacks = %0d", write_count);

        if (write_count == 64)
            $display("PASS: 64 inputs produced 64 writebacks.");
        else
            $fatal("FAIL: expected 64 writebacks, got %0d", write_count);

        $finish;
    end


    always @(posedge clk) begin
        if (rst) begin
            cycle      <= 0;
            seen_write <= 0;
        end
        else begin
            cycle <= cycle + 1;

            if (dut.add_valid) begin

                $display(
                    "cycle=%0d add_valid=1 wbsel=%0d result=%h",
                    cycle,
                    dut.writeback_sel,
                    dut.add_result
                );

                write_count <= write_count + 1;

                if (seen_write) begin
                    if (dut.writeback_sel !== (prev_sel + 1'b1)) begin
                        $fatal(
                            "FAIL: bank sequence broken: prev=%0d now=%0d",
                            prev_sel,
                            dut.writeback_sel
                        );
                    end
                end

                prev_sel   <= dut.writeback_sel;
                seen_write <= 1;
            end
        end
    end

endmodule
