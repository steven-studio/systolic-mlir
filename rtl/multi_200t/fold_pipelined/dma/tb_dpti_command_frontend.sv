`timescale 1ns/1ps
`default_nettype none

module tb_dpti_command_frontend;

    localparam integer PAYLOAD_BYTES = 1024;

    reg clk;
    reg rst;

    reg  [7:0] byte_data;
    reg        byte_valid;
    wire       byte_ready;

    wire       cmd_valid;
    reg        cmd_ready;
    wire [7:0] cmd_addr;
    wire [31:0] cmd_data;

    wire       mem_start;
    wire [28:0] mem_base_addr;
    wire [127:0] mem_data;
    wire       mem_valid;
    reg        mem_ready;
    wire       mem_done;

    wire err_opcode;
    wire err_write32_opcode;
    wire err_mem_opcode;
    wire err_mem_length;

    integer write32_count;
    integer mem_start_count;
    integer mem_done_count;
    integer mem_beat_count;
    integer cycle_count;

    dpti_command_frontend #(
        .ADDR_W        (8),
        .MEM_ADDR_W    (29),
        .MEM_DATA_W    (128),
        .PAYLOAD_BYTES (PAYLOAD_BYTES)
    ) dut (
        .clk                (clk),
        .rst                (rst),

        .byte_data          (byte_data),
        .byte_valid         (byte_valid),
        .byte_ready         (byte_ready),

        .cmd_valid          (cmd_valid),
        .cmd_ready          (cmd_ready),
        .cmd_addr           (cmd_addr),
        .cmd_data           (cmd_data),

        .mem_start          (mem_start),
        .mem_base_addr      (mem_base_addr),

        .mem_data           (mem_data),
        .mem_valid          (mem_valid),
        .mem_ready          (mem_ready),

        .mem_done           (mem_done),

        .err_opcode         (err_opcode),
        .err_write32_opcode (err_write32_opcode),
        .err_mem_opcode     (err_mem_opcode),
        .err_mem_length     (err_mem_length)
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

    task send_write32;
        input [7:0] addr;
        input [31:0] data;
        begin
            send_byte(8'h01);
            send_byte(addr);
            send_byte(data[7:0]);
            send_byte(data[15:8]);
            send_byte(data[23:16]);
            send_byte(data[31:24]);
        end
    endtask

    // ------------------------------------------------------------------------
    // Downstream backpressure.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (rst) begin
            cycle_count <= 0;
            mem_ready   <= 1'b0;
        end
        else begin
            cycle_count <= cycle_count + 1;

            // Periodically stall the MEM_WRITE output.
            mem_ready <= ((cycle_count % 4) != 0);
        end
    end

    // ------------------------------------------------------------------------
    // WRITE32 checker.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (rst) begin
            write32_count <= 0;
        end
        else if (cmd_valid && cmd_ready) begin

            if (write32_count == 0) begin
                if (cmd_addr !== 8'h20 ||
                    cmd_data !== 32'h12345678) begin

                    $display(
                        "FAIL: WRITE32 #1 addr=%02x data=%08x",
                        cmd_addr,
                        cmd_data
                    );
                    $finish(1);
                end

                $display(
                    "PASS: WRITE32 #1 addr=20 data=12345678"
                );
            end
            else if (write32_count == 1) begin
                if (cmd_addr !== 8'h24 ||
                    cmd_data !== 32'hDEADBEEF) begin

                    $display(
                        "FAIL: WRITE32 #2 addr=%02x data=%08x",
                        cmd_addr,
                        cmd_data
                    );
                    $finish(1);
                end

                $display(
                    "PASS: WRITE32 #2 addr=24 data=DEADBEEF"
                );
            end
            else begin
                $display(
                    "FAIL: unexpected extra WRITE32 command"
                );
                $finish(1);
            end

            write32_count <= write32_count + 1;
        end
    end

    // ------------------------------------------------------------------------
    // MEM_WRITE checker.
    // ------------------------------------------------------------------------

    integer j;
    integer expected_byte;

    always @(posedge clk) begin
        if (rst) begin
            mem_start_count <= 0;
            mem_done_count  <= 0;
            mem_beat_count  <= 0;
        end
        else begin

            if (mem_start) begin
                mem_start_count <= mem_start_count + 1;

                if (mem_base_addr !== 29'h00010000) begin
                    $display(
                        "FAIL: MEM_WRITE base=%h expected=00010000",
                        mem_base_addr
                    );
                    $finish(1);
                end

                $display(
                    "PASS: MEM_WRITE start base=00010000"
                );
            end

            if (mem_valid && mem_ready) begin

                for (j = 0; j < 16; j = j + 1) begin
                    expected_byte =
                        (mem_beat_count * 16 + j) & 8'hff;

                    if (mem_data[8*j +: 8] !==
                        expected_byte[7:0]) begin

                        $display(
                            "FAIL: MEM beat=%0d byte=%0d got=%02x expected=%02x",
                            mem_beat_count,
                            j,
                            mem_data[8*j +: 8],
                            expected_byte[7:0]
                        );
                        $finish(1);
                    end
                end

                mem_beat_count <= mem_beat_count + 1;
            end

            if (mem_done) begin
                mem_done_count <= mem_done_count + 1;
                $display(
                    "PASS: MEM_WRITE done beats=%0d",
                    mem_beat_count
                );
            end
        end
    end

    // ------------------------------------------------------------------------
    // Main.
    // ------------------------------------------------------------------------

    integer i;

    initial begin
        rst        = 1'b1;
        byte_data  = 8'h00;
        byte_valid = 1'b0;

        // Keep WRITE32 downstream accepting commands.
        cmd_ready = 1'b1;

        repeat (5)
            @(posedge clk);

        @(negedge clk);
        rst = 1'b0;

        // WRITE32 #1.
        send_write32(
            8'h20,
            32'h12345678
        );

        // Wait until frontend releases WRITE32 ownership.
        wait (write32_count == 1);

        // MEM_WRITE opcode.
        send_byte(8'h02);

        // DDR address = 0x00010000.
        send_byte(8'h00);
        send_byte(8'h00);
        send_byte(8'h01);
        send_byte(8'h00);

        // length = 1024 = 0x00000400.
        send_byte(8'h00);
        send_byte(8'h04);
        send_byte(8'h00);
        send_byte(8'h00);

        // Deterministic payload.
        for (i = 0; i < PAYLOAD_BYTES; i = i + 1)
            send_byte(i & 8'hff);

        wait (mem_done_count == 1);

        // WRITE32 #2.
        send_write32(
            8'h24,
            32'hDEADBEEF
        );

        wait (write32_count == 2);

        repeat (3)
            @(posedge clk);

        if (mem_start_count != 1) begin
            $display(
                "FAIL: mem_start_count=%0d expected=1",
                mem_start_count
            );
            $finish(1);
        end

        if (mem_done_count != 1) begin
            $display(
                "FAIL: mem_done_count=%0d expected=1",
                mem_done_count
            );
            $finish(1);
        end

        if (mem_beat_count != 64) begin
            $display(
                "FAIL: mem_beat_count=%0d expected=64",
                mem_beat_count
            );
            $finish(1);
        end

        if (err_opcode ||
            err_write32_opcode ||
            err_mem_opcode ||
            err_mem_length) begin

            $display(
                "FAIL: errors frontend=%0b write32=%0b mem=%0b length=%0b",
                err_opcode,
                err_write32_opcode,
                err_mem_opcode,
                err_mem_length
            );
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI COMMAND FRONTEND TEST: PASS");
        $display("WRITE32 commands=2");
        $display("MEM_WRITE commands=1 beats=64 bytes=1024");
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
