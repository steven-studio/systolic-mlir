`timescale 1ns/1ps
`default_nettype none

module tb_dpti_mem_write_decoder;

    localparam integer ADDR_W        = 29;
    localparam integer DATA_W        = 128;
    localparam integer PAYLOAD_BYTES = 1024;
    localparam integer TOTAL_BEATS   = PAYLOAD_BYTES / (DATA_W / 8);

    reg clk;
    reg rst_n;

    reg  [7:0] byte_data;
    reg        byte_valid;
    wire       byte_ready;

    wire                  start;
    wire [ADDR_W-1:0]     base_addr;

    wire [DATA_W-1:0]     out_data;
    wire                  out_valid;
    reg                   out_ready;

    wire                  done;
    wire                  err_opcode;
    wire                  err_length;

    integer cycle_count;
    integer beat_count;
    integer done_count;
    integer start_count;
    integer timeout_cycles;

    reg [127:0] expected;

    dpti_mem_write_decoder #(
        .ADDR_W        (ADDR_W),
        .DATA_W        (DATA_W),
        .PAYLOAD_BYTES (PAYLOAD_BYTES)
    ) dut (
        .clk        (clk),
        .rst_n      (rst_n),

        .byte_data  (byte_data),
        .byte_valid (byte_valid),
        .byte_ready (byte_ready),

        .start      (start),
        .base_addr  (base_addr),

        .out_data   (out_data),
        .out_valid  (out_valid),
        .out_ready  (out_ready),

        .done       (done),
        .err_opcode (err_opcode),
        .err_length (err_length)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    // ------------------------------------------------------------------------
    // Downstream backpressure.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst_n) begin
            cycle_count <= 0;
            out_ready   <= 1'b0;
        end
        else begin
            cycle_count <= cycle_count + 1;

            // Stall every fourth cycle.
            out_ready <= ((cycle_count % 4) != 0);
        end
    end

    // ------------------------------------------------------------------------
    // Send exactly one byte using valid/ready handshake.
    // ------------------------------------------------------------------------

    task send_byte;
        input [7:0] value;
        begin
            // Drive inputs away from the DUT's active posedge so data and
            // valid are stable before the sampling edge.
            @(negedge clk);
            byte_data  = value;
            byte_valid = 1'b1;

            // Hold valid until one rising edge observes ready.
            @(posedge clk);
            while (!byte_ready)
                @(posedge clk);

            // Withdraw only after the handshake edge has completed.
            @(negedge clk);
            byte_valid = 1'b0;
            byte_data  = 8'h00;
        end
    endtask

    // ------------------------------------------------------------------------
    // Check output beats.
    //
    // Payload byte i is i modulo 256.
    // Each 128-bit beat must therefore contain 16 consecutive bytes,
    // little-endian:
    //
    //   payload[16*n + 0]  -> bits [7:0]
    //   ...
    //   payload[16*n + 15] -> bits [127:120]
    // ------------------------------------------------------------------------

    integer j;

    always @(posedge clk) begin
        if (!rst_n) begin
            beat_count <= 0;
        end
        else if (out_valid && out_ready) begin
            expected = '0;

            for (j = 0; j < 16; j = j + 1) begin
                expected[8*j +: 8] =
                    (beat_count * 16 + j) & 8'hff;
            end

            if (out_data !== expected) begin
                $display(
                    "FAIL: beat %0d mismatch",
                    beat_count
                );
                $display(
                    "  got      = %032x",
                    out_data
                );
                $display(
                    "  expected = %032x",
                    expected
                );
                $finish(1);
            end

            beat_count <= beat_count + 1;
        end
    end

    always @(posedge clk) begin
        if (!rst_n) begin
            start_count <= 0;
            done_count  <= 0;
        end
        else begin
            if (start) begin
                start_count <= start_count + 1;

                if (base_addr !== 29'h00010000) begin
                    $display(
                        "FAIL: base_addr=%h expected=00010000",
                        base_addr
                    );
                    $finish(1);
                end
            end

            if (done)
                done_count <= done_count + 1;
        end
    end

    // ------------------------------------------------------------------------
    // Debug progress.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (rst_n && out_valid && out_ready) begin
            $display(
                "DBG beat=%0d state=%0d payload_count=%0d",
                beat_count,
                dut.state,
                dut.payload_count
            );
        end

        if (rst_n && done) begin
            $display(
                "DBG DONE state=%0d payload_count=%0d beat_count=%0d",
                dut.state,
                dut.payload_count,
                beat_count
            );
        end
    end

    // ------------------------------------------------------------------------
    // Debug progress.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (rst_n && out_valid && out_ready) begin
            $display(
                "DBG beat=%0d state=%0d payload_count=%0d",
                beat_count,
                dut.state,
                dut.payload_count
            );
        end

        if (rst_n && done) begin
            $display(
                "DBG DONE state=%0d payload_count=%0d beat_count=%0d",
                dut.state,
                dut.payload_count,
                beat_count
            );
        end
    end

    // ------------------------------------------------------------------------
    // Main test.
    // ------------------------------------------------------------------------

    integer i;

    initial begin
        rst_n      = 1'b0;
        byte_data  = 8'h00;
        byte_valid = 1'b0;

        repeat (5)
            @(posedge clk);

        rst_n = 1'b1;

        repeat (2)
            @(posedge clk);

        // opcode = MEM_WRITE
        send_byte(8'h02);

        // DDR address = 0x00010000, little-endian.
        send_byte(8'h00);
        send_byte(8'h00);
        send_byte(8'h01);
        send_byte(8'h00);

        // length = 1024 = 0x00000400, little-endian.
        send_byte(8'h00);
        send_byte(8'h04);
        send_byte(8'h00);
        send_byte(8'h00);

        // 1024-byte payload.
        for (i = 0; i < PAYLOAD_BYTES; i = i + 1)
            send_byte(i & 8'hff);

        timeout_cycles = 0;

        while ((done_count == 0) &&
               (timeout_cycles < 1000)) begin
            @(posedge clk);
            timeout_cycles = timeout_cycles + 1;
        end

        if (done_count != 1) begin
            $display(
                "DBG FINAL: state=%0d addr_tmp=%08x length_tmp=%08x",
                dut.state,
                dut.addr_tmp,
                dut.length_tmp
            );
            $display(
                "DBG FINAL: payload_count=%0d byte_index=%0d beat_count=%0d",
                dut.payload_count,
                dut.byte_index,
                beat_count
            );
            $display(
                "DBG FINAL: err_opcode=%0b err_length=%0b start_count=%0d",
                err_opcode,
                err_length,
                start_count
            );
            $display(
                "FAIL: done_count=%0d expected=1",
                done_count
            );
            $finish(1);
        end

        if (start_count != 1) begin
            $display(
                "FAIL: start_count=%0d expected=1",
                start_count
            );
            $finish(1);
        end

        if (beat_count != TOTAL_BEATS) begin
            $display(
                "FAIL: beat_count=%0d expected=%0d",
                beat_count,
                TOTAL_BEATS
            );
            $finish(1);
        end

        if (err_opcode) begin
            $display("FAIL: err_opcode asserted");
            $finish(1);
        end

        if (err_length) begin
            $display("FAIL: err_length asserted");
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI MEM WRITE DECODER TEST: PASS");
        $display(
            "base=0x%08x bytes=%0d beats=%0d",
            base_addr,
            PAYLOAD_BYTES,
            beat_count
        );
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
