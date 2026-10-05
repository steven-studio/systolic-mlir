`timescale 1ns/1ps
`default_nettype none

module tb_dpti_mem_write_path;

    localparam integer AXI_DATA_W   = 128;
    localparam integer AXI_ADDR_W   = 29;
    localparam integer AXI_ID_W     = 2;
    localparam integer BURST_LEN    = 16;
    localparam integer PAYLOAD_BYTES = 1024;

    localparam logic [AXI_ADDR_W-1:0]
        BASE_ADDR = 29'h00010000;

    reg clk;
    reg rst_n;

    reg  [7:0] byte_data;
    reg        byte_valid;
    wire       byte_ready;

    wire busy;
    wire done;
    wire err_opcode;
    wire err_length;
    wire err_align;
    wire err_resp;

    wire [AXI_ID_W-1:0]     awid;
    wire [AXI_ADDR_W-1:0]   awaddr;
    wire [7:0]              awlen;
    wire [2:0]              awsize;
    wire [1:0]              awburst;
    wire [0:0]              awlock;
    wire [3:0]              awcache;
    wire [2:0]              awprot;
    wire [3:0]              awqos;
    wire                    awvalid;
    reg                     awready;

    wire [AXI_DATA_W-1:0]   wdata;
    wire [AXI_DATA_W/8-1:0] wstrb;
    wire                    wlast;
    wire                    wvalid;
    reg                     wready;

    reg  [AXI_ID_W-1:0]     bid;
    reg  [1:0]              bresp;
    reg                     bvalid;
    wire                    bready;

    integer cycle_count;
    integer burst_count;
    integer beat_count;
    integer beat_in_burst;
    integer timeout_cycles;

    reg response_pending;

    reg [7:0] memory [0:PAYLOAD_BYTES-1];

    dpti_mem_write_path #(
        .AXI_DATA_W   (AXI_DATA_W),
        .AXI_ADDR_W   (AXI_ADDR_W),
        .AXI_ID_W     (AXI_ID_W),
        .BURST_LEN    (BURST_LEN),
        .PAYLOAD_BYTES(PAYLOAD_BYTES)
    ) dut (
        .clk           (clk),
        .rst_n         (rst_n),

        .byte_data     (byte_data),
        .byte_valid    (byte_valid),
        .byte_ready    (byte_ready),

        .busy          (busy),
        .done          (done),
        .err_opcode    (err_opcode),
        .err_length    (err_length),
        .err_align     (err_align),
        .err_resp      (err_resp),

        .m_axi_awid    (awid),
        .m_axi_awaddr  (awaddr),
        .m_axi_awlen   (awlen),
        .m_axi_awsize  (awsize),
        .m_axi_awburst (awburst),
        .m_axi_awlock  (awlock),
        .m_axi_awcache (awcache),
        .m_axi_awprot  (awprot),
        .m_axi_awqos   (awqos),
        .m_axi_awvalid (awvalid),
        .m_axi_awready (awready),

        .m_axi_wdata   (wdata),
        .m_axi_wstrb   (wstrb),
        .m_axi_wlast   (wlast),
        .m_axi_wvalid  (wvalid),
        .m_axi_wready  (wready),

        .m_axi_bid     (bid),
        .m_axi_bresp   (bresp),
        .m_axi_bvalid  (bvalid),
        .m_axi_bready  (bready)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    // ------------------------------------------------------------------------
    // Input byte handshake.
    // ------------------------------------------------------------------------

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

    // ------------------------------------------------------------------------
    // AXI slave backpressure.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst_n) begin
            cycle_count <= 0;
            awready     <= 1'b0;
            wready      <= 1'b0;
        end
        else begin
            cycle_count <= cycle_count + 1;

            awready <= ((cycle_count % 3) != 0);
            wready  <= ((cycle_count % 5) != 0);
        end
    end

    // ------------------------------------------------------------------------
    // Address checking.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst_n) begin
            burst_count <= 0;
        end
        else if (awvalid && awready) begin

            if (awaddr !==
                BASE_ADDR +
                burst_count * BURST_LEN * 16) begin

                $display(
                    "FAIL: AW burst %0d address=%h expected=%h",
                    burst_count,
                    awaddr,
                    BASE_ADDR +
                    burst_count * BURST_LEN * 16
                );
                $finish(1);
            end

            if (awlen !== BURST_LEN - 1) begin
                $display(
                    "FAIL: AWLEN=%0d expected=%0d",
                    awlen,
                    BURST_LEN - 1
                );
                $finish(1);
            end

            $display(
                "AW burst %0d addr=%h",
                burst_count,
                awaddr
            );

            burst_count <= burst_count + 1;
        end
    end

    // ------------------------------------------------------------------------
    // AXI data sink.
    //
    // Store every accepted 128-bit beat into a byte-addressed memory model.
    // ------------------------------------------------------------------------

    integer j;

    always @(posedge clk) begin
        if (!rst_n) begin
            beat_count       <= 0;
            beat_in_burst    <= 0;
            response_pending <= 1'b0;
            bvalid           <= 1'b0;
            bid              <= '0;
            bresp            <= 2'b00;
        end
        else begin
            bvalid <= 1'b0;

            if (wvalid && wready) begin

                for (j = 0; j < 16; j = j + 1)
                    memory[beat_count * 16 + j]
                        <= wdata[8*j +: 8];

                if (wlast !==
                    (beat_in_burst == BURST_LEN - 1)) begin

                    $display(
                        "FAIL: beat %0d WLAST=%0b expected=%0b",
                        beat_count,
                        wlast,
                        (beat_in_burst == BURST_LEN - 1)
                    );
                    $finish(1);
                end

                beat_count <= beat_count + 1;

                if (beat_in_burst == BURST_LEN - 1) begin
                    beat_in_burst    <= 0;
                    response_pending <= 1'b1;
                end
                else begin
                    beat_in_burst <= beat_in_burst + 1;
                end
            end

            if (response_pending && !bvalid) begin
                bvalid           <= 1'b1;
                response_pending <= 1'b0;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Main.
    // ------------------------------------------------------------------------

    integer i;

    initial begin
        rst_n      = 1'b0;
        byte_data  = 8'h00;
        byte_valid = 1'b0;

        for (i = 0; i < PAYLOAD_BYTES; i = i + 1)
            memory[i] = 8'hxx;

        repeat (5)
            @(posedge clk);

        rst_n = 1'b1;

        repeat (2)
            @(posedge clk);

        // MEM_WRITE opcode.
        send_byte(8'h02);

        // address = 0x00010000.
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

        timeout_cycles = 0;

        while (!done && timeout_cycles < 2000) begin
            @(posedge clk);
            timeout_cycles = timeout_cycles + 1;
        end

        if (!done) begin
            $display("FAIL: timeout waiting for writer done");
            $finish(1);
        end

        // Let nonblocking memory writes settle.
        @(posedge clk);
        #1;

        if (err_opcode ||
            err_length ||
            err_align ||
            err_resp) begin

            $display(
                "FAIL: errors opcode=%0b length=%0b align=%0b resp=%0b",
                err_opcode,
                err_length,
                err_align,
                err_resp
            );
            $finish(1);
        end

        if (beat_count != 64) begin
            $display(
                "FAIL: beat_count=%0d expected=64",
                beat_count
            );
            $finish(1);
        end

        if (burst_count != 4) begin
            $display(
                "FAIL: burst_count=%0d expected=4",
                burst_count
            );
            $finish(1);
        end

        for (i = 0; i < PAYLOAD_BYTES; i = i + 1) begin
            if (memory[i] !== (i & 8'hff)) begin
                $display(
                    "FAIL: memory[%0d]=%02x expected=%02x",
                    i,
                    memory[i],
                    (i & 8'hff)
                );
                $finish(1);
            end
        end

        $display("----------------------------------------");
        $display("DPTI MEM WRITE PATH TEST: PASS");
        $display("bytes=1024 beats=64 bursts=4");
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
