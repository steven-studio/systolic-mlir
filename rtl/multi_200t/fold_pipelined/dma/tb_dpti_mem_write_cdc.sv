`timescale 1ns/1ps
`default_nettype none

module tb_dpti_mem_write_cdc;

    localparam integer ADDR_W = 29;
    localparam integer DATA_W = 128;
    localparam integer DEPTH  = 8;
    localparam integer BEATS  = 64;

    reg src_clk;
    reg dst_clk;

    reg src_rst;
    reg dst_rst;

    reg                  src_start;
    reg  [ADDR_W-1:0]    src_base_addr;

    reg  [DATA_W-1:0]    src_data;
    reg                  src_valid;
    wire                 src_ready;

    wire                 dst_start;
    wire [ADDR_W-1:0]    dst_base_addr;

    wire [DATA_W-1:0]    dst_data;
    wire                 dst_valid;
    reg                  dst_ready;

    wire                 err_protocol;

    integer sent_count;
    integer recv_count;
    integer start_count;
    integer src_stall_count;
    integer dst_cycle;

    integer j;
    integer expected_byte;

    dpti_mem_write_cdc #(
        .ADDR_W (ADDR_W),
        .DATA_W (DATA_W),
        .DEPTH  (DEPTH)
    ) dut (
        .src_clk       (src_clk),
        .src_rst       (src_rst),

        .src_start     (src_start),
        .src_base_addr (src_base_addr),

        .src_data      (src_data),
        .src_valid     (src_valid),
        .src_ready     (src_ready),

        .dst_clk       (dst_clk),
        .dst_rst       (dst_rst),

        .dst_start     (dst_start),
        .dst_base_addr (dst_base_addr),

        .dst_data      (dst_data),
        .dst_valid     (dst_valid),
        .dst_ready     (dst_ready),

        .err_protocol  (err_protocol)
    );

    // Source clock: 10 ns.
    initial begin
        src_clk = 1'b0;
        forever #5 src_clk = ~src_clk;
    end

    // Destination clock: 14 ns.
    initial begin
        dst_clk = 1'b0;
        forever #7 dst_clk = ~dst_clk;
    end

    // ------------------------------------------------------------------------
    // Destination backpressure.
    // ------------------------------------------------------------------------

    always @(posedge dst_clk) begin
        if (dst_rst) begin
            dst_cycle <= 0;
            dst_ready <= 1'b0;
        end
        else begin
            dst_cycle <= dst_cycle + 1;

            // Accept only one out of every four destination cycles.
            // This deliberately fills the small CDC FIFO so that
            // backpressure must propagate into the source domain.
            dst_ready <= ((dst_cycle % 4) == 0);
        end
    end

    // ------------------------------------------------------------------------
    // Destination scoreboard.
    // ------------------------------------------------------------------------

    always @(posedge dst_clk) begin
        if (dst_rst) begin
            recv_count  <= 0;
            start_count <= 0;
        end
        else begin

            if (dst_start) begin
                start_count <= start_count + 1;

                if (recv_count != 0) begin
                    $display(
                        "FAIL: dst_start occurred after payload began, recv=%0d",
                        recv_count
                    );
                    $finish(1);
                end

                if (dst_base_addr !== 29'h00010000) begin
                    $display(
                        "FAIL: dst_base_addr=%h expected=00010000",
                        dst_base_addr
                    );
                    $finish(1);
                end

                $display(
                    "PASS: CDC header base=00010000"
                );
            end

            if (dst_valid && dst_ready) begin

                // dst_start may be observed on the same destination edge as
                // the first payload handshake.  start_count is updated using
                // a nonblocking assignment, so accept either a previously
                // counted header or dst_start asserted on this edge.
                if ((start_count == 0) && !dst_start) begin
                    $display(
                        "FAIL: payload arrived before header/start"
                    );
                    $finish(1);
                end

                for (j = 0; j < 16; j = j + 1) begin
                    expected_byte =
                        (recv_count * 16 + j) & 8'hff;

                    if (dst_data[8*j +: 8] !==
                        expected_byte[7:0]) begin

                        $display(
                            "FAIL: beat=%0d byte=%0d got=%02x expected=%02x",
                            recv_count,
                            j,
                            dst_data[8*j +: 8],
                            expected_byte[7:0]
                        );
                        $finish(1);
                    end
                end

                recv_count <= recv_count + 1;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Source.
    // ------------------------------------------------------------------------

    integer beat;
    integer b;

    initial begin
        src_start       = 1'b0;
        src_base_addr   = '0;
        src_data        = '0;
        src_valid       = 1'b0;

        sent_count      = 0;
        src_stall_count = 0;

        wait (!src_rst);

        // Emit command header.
        @(negedge src_clk);
        src_base_addr = 29'h00010000;
        src_start     = 1'b1;

        @(posedge src_clk);

        @(negedge src_clk);
        src_start = 1'b0;

        // The CDC bridge blocks payload until the HEADER has entered FIFO.
        for (beat = 0; beat < BEATS; beat = beat + 1) begin

            @(negedge src_clk);

            for (b = 0; b < 16; b = b + 1)
                src_data[8*b +: 8] =
                    ((beat * 16 + b) & 8'hff);

            src_valid = 1'b1;

            @(posedge src_clk);

            while (!src_ready) begin
                src_stall_count = src_stall_count + 1;
                @(posedge src_clk);
            end

            sent_count = sent_count + 1;

            @(negedge src_clk);
            src_valid = 1'b0;
        end
    end

    // ------------------------------------------------------------------------
    // Reset and completion.
    // ------------------------------------------------------------------------

    integer timeout;

    initial begin
        src_rst = 1'b1;
        dst_rst = 1'b1;

        repeat (5)
            @(posedge src_clk);

        @(negedge src_clk);
        src_rst = 1'b0;

        repeat (2)
            @(posedge dst_clk);

        @(negedge dst_clk);
        dst_rst = 1'b0;

        timeout = 0;

        while ((recv_count < BEATS) &&
               (timeout < 5000)) begin
            @(posedge dst_clk);
            timeout = timeout + 1;
        end

        if (recv_count != BEATS) begin
            $display(
                "FAIL: timeout recv_count=%0d expected=%0d",
                recv_count,
                BEATS
            );
            $finish(1);
        end

        repeat (3)
            @(posedge dst_clk);

        #1;

        if (sent_count != BEATS) begin
            $display(
                "FAIL: sent_count=%0d expected=%0d",
                sent_count,
                BEATS
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

        if (err_protocol) begin
            $display(
                "FAIL: err_protocol asserted"
            );
            $finish(1);
        end

        if (src_stall_count == 0) begin
            $display(
                "FAIL: CDC bridge never applied source backpressure"
            );
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI MEM WRITE CDC TEST: PASS");
        $display(
            "header=1 sent=%0d received=%0d source_stalls=%0d",
            sent_count,
            recv_count,
            src_stall_count
        );
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
