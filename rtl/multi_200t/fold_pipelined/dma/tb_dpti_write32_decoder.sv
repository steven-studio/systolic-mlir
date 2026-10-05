`timescale 1ns/1ps
`default_nettype none

module tb_dpti_write32_decoder;

    reg clk;
    reg rst;

    reg  [7:0] byte_data;
    reg        byte_valid;
    wire       byte_ready;

    wire       cmd_valid;
    reg        cmd_ready;
    wire [7:0] cmd_addr;
    wire [31:0] cmd_data;

    wire err_opcode;

    dpti_write32_decoder dut (
        .clk        (clk),
        .rst        (rst),

        .byte_data  (byte_data),
        .byte_valid (byte_valid),
        .byte_ready (byte_ready),

        .cmd_valid  (cmd_valid),
        .cmd_ready  (cmd_ready),
        .cmd_addr   (cmd_addr),
        .cmd_data   (cmd_data),

        .err_opcode (err_opcode)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    task send_byte;
        input [7:0] value;
        begin
            @(negedge clk);
            byte_data  = value;
            byte_valid = 1'b1;

            @(posedge clk);
            while (!byte_ready)
                @(posedge clk);

            @(negedge clk);
            byte_valid = 1'b0;
            byte_data  = 8'h00;
        end
    endtask

    initial begin
        rst        = 1'b1;
        byte_data  = 8'h00;
        byte_valid = 1'b0;
        cmd_ready  = 1'b0;

        repeat (4)
            @(posedge clk);

        @(negedge clk);
        rst = 1'b0;

        // ------------------------------------------------------------
        // WRITE32:
        //
        // opcode = 01
        // addr   = 20
        // data   = 12345678, little endian on the wire.
        // ------------------------------------------------------------

        send_byte(8'h01);
        send_byte(8'h20);
        send_byte(8'h78);
        send_byte(8'h56);
        send_byte(8'h34);
        send_byte(8'h12);

        @(posedge clk);
        #1;

        if (!cmd_valid) begin
            $display("FAIL: cmd_valid not asserted");
            $finish(1);
        end

        if (cmd_addr !== 8'h20) begin
            $display(
                "FAIL: cmd_addr=%02x expected=20",
                cmd_addr
            );
            $finish(1);
        end

        if (cmd_data !== 32'h12345678) begin
            $display(
                "FAIL: cmd_data=%08x expected=12345678",
                cmd_data
            );
            $finish(1);
        end

        if (err_opcode) begin
            $display("FAIL: err_opcode unexpectedly asserted");
            $finish(1);
        end

        // ------------------------------------------------------------
        // Stall downstream.  Completed command must remain stable.
        // ------------------------------------------------------------

        repeat (5) begin
            @(posedge clk);
            #1;

            if (!cmd_valid) begin
                $display(
                    "FAIL: cmd_valid dropped while cmd_ready=0"
                );
                $finish(1);
            end

            if (cmd_addr !== 8'h20 ||
                cmd_data !== 32'h12345678) begin

                $display(
                    "FAIL: command changed while stalled"
                );
                $finish(1);
            end

            if (byte_ready) begin
                $display(
                    "FAIL: byte_ready asserted while command pending"
                );
                $finish(1);
            end
        end

        $display(
            "PASS: WRITE32 command held under backpressure."
        );

        // ------------------------------------------------------------
        // Consume command.
        // ------------------------------------------------------------

        @(negedge clk);
        cmd_ready = 1'b1;

        @(posedge clk);

        @(negedge clk);
        cmd_ready = 1'b0;

        @(posedge clk);
        #1;

        if (cmd_valid) begin
            $display(
                "FAIL: cmd_valid remained asserted after handshake"
            );
            $finish(1);
        end

        if (!byte_ready) begin
            $display(
                "FAIL: byte_ready not restored after command handshake"
            );
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI WRITE32 DECODER TEST: PASS");
        $display("addr=0x%02x data=0x%08x",
                 cmd_addr, cmd_data);
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
