`timescale 1ns/1ps
`default_nettype none

module tb_dpti_mem_write_engine;

    localparam integer AXI_DATA_W  = 128;
    localparam integer AXI_ADDR_W  = 29;
    localparam integer AXI_ID_W    = 2;
    localparam integer BURST_LEN   = 16;
    localparam integer TOTAL_BEATS = 64;

    localparam logic [AXI_ADDR_W-1:0] BASE_ADDR = 29'h00010000;

    reg clk;
    reg rst_n;

    reg                         start;
    reg  [AXI_ADDR_W-1:0]      base_addr;

    reg                         src_valid;
    reg  [AXI_DATA_W-1:0]      src_data;
    wire                        src_ready;

    wire                        busy;
    wire                        done;
    wire                        err_align;
    wire                        err_resp;

    wire [AXI_ID_W-1:0]         awid;
    wire [AXI_ADDR_W-1:0]       awaddr;
    wire [7:0]                  awlen;
    wire [2:0]                  awsize;
    wire [1:0]                  awburst;
    wire [0:0]                  awlock;
    wire [3:0]                  awcache;
    wire [2:0]                  awprot;
    wire [3:0]                  awqos;
    wire                        awvalid;
    reg                         awready;

    wire [AXI_DATA_W-1:0]       wdata;
    wire [AXI_DATA_W/8-1:0]     wstrb;
    wire                        wlast;
    wire                        wvalid;
    reg                         wready;

    reg  [AXI_ID_W-1:0]         bid;
    reg  [1:0]                  bresp;
    reg                         bvalid;
    wire                        bready;

    integer source_index;
    integer received_beats;
    integer burst_index;
    integer beat_in_burst;
    integer timeout_cycles;

    reg response_pending;

    dpti_mem_write_engine #(
        .AXI_DATA_W  (AXI_DATA_W),
        .AXI_ADDR_W  (AXI_ADDR_W),
        .AXI_ID_W    (AXI_ID_W),
        .BURST_LEN   (BURST_LEN),
        .TOTAL_BEATS (TOTAL_BEATS)
    ) dut (
        .clk            (clk),
        .rst_n          (rst_n),

        .start          (start),
        .base_addr      (base_addr),

        .src_valid      (src_valid),
        .src_data       (src_data),
        .src_ready      (src_ready),

        .busy           (busy),
        .done           (done),
        .err_align      (err_align),
        .err_resp       (err_resp),

        .m_axi_awid     (awid),
        .m_axi_awaddr   (awaddr),
        .m_axi_awlen    (awlen),
        .m_axi_awsize   (awsize),
        .m_axi_awburst  (awburst),
        .m_axi_awlock   (awlock),
        .m_axi_awcache  (awcache),
        .m_axi_awprot   (awprot),
        .m_axi_awqos    (awqos),
        .m_axi_awvalid  (awvalid),
        .m_axi_awready  (awready),

        .m_axi_wdata    (wdata),
        .m_axi_wstrb    (wstrb),
        .m_axi_wlast    (wlast),
        .m_axi_wvalid   (wvalid),
        .m_axi_wready   (wready),

        .m_axi_bid      (bid),
        .m_axi_bresp    (bresp),
        .m_axi_bvalid   (bvalid),
        .m_axi_bready   (bready)
    );

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    // ------------------------------------------------------------------------
    // Source stream.
    //
    // Each 128-bit beat contains its source index repeated in four 32-bit
    // words.  src_valid remains asserted until src_ready consumes the beat.
    // ------------------------------------------------------------------------

    always_comb begin
        src_valid = (source_index < TOTAL_BEATS);

        src_data = {
            32'(source_index),
            32'(source_index),
            32'(source_index),
            32'(source_index)
        };
    end

    always @(posedge clk) begin
        if (!rst_n)
            source_index <= 0;
        else if (src_valid && src_ready)
            source_index <= source_index + 1;
    end

    // ------------------------------------------------------------------------
    // AXI slave model.
    //
    // AWREADY and WREADY deliberately stall periodically.
    // ------------------------------------------------------------------------

    integer cycle_count;

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
    // Check AW transactions.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst_n) begin
            burst_index <= 0;
        end
        else if (awvalid && awready) begin
            if (awaddr !==
                BASE_ADDR + burst_index * (BURST_LEN * 16)) begin
                $display(
                    "FAIL: burst %0d AWADDR=%h expected=%h",
                    burst_index,
                    awaddr,
                    BASE_ADDR + burst_index * (BURST_LEN * 16)
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
                burst_index,
                awaddr
            );

            burst_index <= burst_index + 1;
        end
    end

    // ------------------------------------------------------------------------
    // Check W stream and generate one B response after every 16 accepted beats.
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst_n) begin
            received_beats   <= 0;
            beat_in_burst    <= 0;
            response_pending <= 1'b0;
            bvalid           <= 1'b0;
            bid              <= '0;
            bresp            <= 2'b00;
        end
        else begin
            bvalid <= 1'b0;

            if (wvalid && wready) begin
                if (wdata !== {
                    32'(received_beats),
                    32'(received_beats),
                    32'(received_beats),
                    32'(received_beats)
                }) begin
                    $display(
                        "FAIL: beat %0d data mismatch",
                        received_beats
                    );
                    $finish(1);
                end

                if (wlast !==
                    (beat_in_burst == BURST_LEN - 1)) begin
                    $display(
                        "FAIL: beat %0d WLAST=%0b expected=%0b",
                        received_beats,
                        wlast,
                        (beat_in_burst == BURST_LEN - 1)
                    );
                    $finish(1);
                end

                received_beats <= received_beats + 1;

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
    // Main test.
    // ------------------------------------------------------------------------

    initial begin
        rst_n     = 1'b0;
        start     = 1'b0;
        base_addr = BASE_ADDR;

        repeat (5)
            @(posedge clk);

        rst_n = 1'b1;

        repeat (2)
            @(posedge clk);

        start = 1'b1;
        @(posedge clk);
        start = 1'b0;

        timeout_cycles = 0;

        while (!done && timeout_cycles < 2000) begin
            @(posedge clk);
            timeout_cycles = timeout_cycles + 1;
        end

        if (!done) begin
            $display("FAIL: timeout waiting for done");
            $finish(1);
        end

        if (err_align) begin
            $display("FAIL: err_align asserted");
            $finish(1);
        end

        if (err_resp) begin
            $display("FAIL: err_resp asserted");
            $finish(1);
        end

        if (received_beats != TOTAL_BEATS) begin
            $display(
                "FAIL: received %0d beats expected %0d",
                received_beats,
                TOTAL_BEATS
            );
            $finish(1);
        end

        if (burst_index != TOTAL_BEATS / BURST_LEN) begin
            $display(
                "FAIL: received %0d bursts expected %0d",
                burst_index,
                TOTAL_BEATS / BURST_LEN
            );
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI MEM WRITE ENGINE TEST: PASS");
        $display("beats=%0d bursts=%0d", received_beats, burst_index);
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
