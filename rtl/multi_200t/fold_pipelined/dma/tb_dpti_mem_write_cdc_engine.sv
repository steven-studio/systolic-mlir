`timescale 1ns/1ps
`default_nettype none

module tb_dpti_mem_write_cdc_engine;

    localparam integer AXI_DATA_W = 128;
    localparam integer AXI_ADDR_W = 29;
    localparam integer AXI_ID_W   = 2;
    localparam integer BURST_LEN  = 16;
    localparam integer TOTAL_BEATS = 64;
    localparam integer PAYLOAD_BYTES = 1024;

    localparam logic [AXI_ADDR_W-1:0]
        BASE_ADDR = 29'h00010000;

    // ------------------------------------------------------------------------
    // Two independent clock domains.
    // ------------------------------------------------------------------------

    reg src_clk;
    reg ui_clk;

    reg src_rst;
    reg ui_rst_n;

    // Source-side decoded MEM_WRITE stream.
    reg                       src_start;
    reg  [AXI_ADDR_W-1:0]     src_base_addr;
    reg  [AXI_DATA_W-1:0]     src_data;
    reg                       src_valid;
    wire                      src_ready;

    wire busy;
    wire done;
    wire err_protocol;
    wire err_align;
    wire err_resp;

    // AXI write channels.
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
    integer source_stalls;

    reg response_pending;

    reg [7:0] memory [0:PAYLOAD_BYTES-1];

    dpti_mem_write_cdc_engine #(
        .AXI_DATA_W  (AXI_DATA_W),
        .AXI_ADDR_W  (AXI_ADDR_W),
        .AXI_ID_W    (AXI_ID_W),
        .FIFO_DEPTH  (8),
        .BURST_LEN   (BURST_LEN),
        .TOTAL_BEATS (TOTAL_BEATS)
    ) dut (
        .src_clk       (src_clk),
        .src_rst       (src_rst),

        .src_start     (src_start),
        .src_base_addr (src_base_addr),

        .src_data      (src_data),
        .src_valid     (src_valid),
        .src_ready     (src_ready),

        .ui_clk        (ui_clk),
        .ui_rst_n      (ui_rst_n),

        .busy          (busy),
        .done          (done),
        .err_protocol  (err_protocol),
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

    // Source clock: 10 ns.
    initial begin
        src_clk = 1'b0;
        forever #5 src_clk = ~src_clk;
    end

    // AXI/ui clock: 14 ns.
    initial begin
        ui_clk = 1'b0;
        forever #7 ui_clk = ~ui_clk;
    end

    // ------------------------------------------------------------------------
    // AXI slave backpressure.
    // Same model used by the previously-passing path test.
    // ------------------------------------------------------------------------

    always @(posedge ui_clk) begin
        if (!ui_rst_n) begin
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
    // Address checker.
    // ------------------------------------------------------------------------

    always @(posedge ui_clk) begin
        if (!ui_rst_n) begin
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
    // AXI data sink / byte-addressed memory model.
    // ------------------------------------------------------------------------

    integer j;

    always @(posedge ui_clk) begin
        if (!ui_rst_n) begin
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
    // Source: one HEADER followed by 64 payload beats.
    // ------------------------------------------------------------------------

    integer beat;
    integer b;

    initial begin
        src_start     = 1'b0;
        src_base_addr = '0;
        src_data      = '0;
        src_valid     = 1'b0;
        source_stalls = 0;

        wait (!src_rst);

        // HEADER.
        @(negedge src_clk);
        src_base_addr = BASE_ADDR;
        src_start     = 1'b1;

        @(posedge src_clk);

        @(negedge src_clk);
        src_start = 1'b0;

        // Payload.
        for (beat = 0; beat < TOTAL_BEATS; beat = beat + 1) begin

            @(negedge src_clk);

            for (b = 0; b < 16; b = b + 1)
                src_data[8*b +: 8] =
                    ((beat * 16 + b) & 8'hff);

            src_valid = 1'b1;

            @(posedge src_clk);

            while (!src_ready) begin
                source_stalls = source_stalls + 1;
                @(posedge src_clk);
            end

            @(negedge src_clk);
            src_valid = 1'b0;
        end
    end

    // ------------------------------------------------------------------------
    // Reset + completion.
    // ------------------------------------------------------------------------

    integer i;

    initial begin
        src_rst  = 1'b1;
        ui_rst_n = 1'b0;

        for (i = 0; i < PAYLOAD_BYTES; i = i + 1)
            memory[i] = 8'hxx;

        repeat (5)
            @(posedge src_clk);

        @(negedge src_clk);
        src_rst = 1'b0;

        repeat (3)
            @(posedge ui_clk);

        @(negedge ui_clk);
        ui_rst_n = 1'b1;

        timeout_cycles = 0;

        while (!done && timeout_cycles < 5000) begin
            @(posedge ui_clk);
            timeout_cycles = timeout_cycles + 1;
        end

        if (!done) begin
            $display("FAIL: timeout waiting for writer done");
            $finish(1);
        end

        // Let memory-model NBAs settle.
        @(posedge ui_clk);
        #1;

        if (err_protocol ||
            err_align ||
            err_resp) begin

            $display(
                "FAIL: errors protocol=%0b align=%0b resp=%0b",
                err_protocol,
                err_align,
                err_resp
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
        $display("DPTI MEM WRITE CDC ENGINE TEST: PASS");
        $display(
            "bytes=1024 beats=%0d bursts=%0d source_stalls=%0d",
            beat_count,
            burst_count,
            source_stalls
        );
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
