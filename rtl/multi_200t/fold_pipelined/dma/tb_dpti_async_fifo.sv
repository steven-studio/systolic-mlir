`timescale 1ns/1ps
`default_nettype none

module tb_dpti_async_fifo;

    localparam integer DATA_W = 32;
    localparam integer DEPTH  = 8;
    localparam integer COUNT  = 32;

    reg wr_clk;
    reg rd_clk;

    reg wr_rst;
    reg rd_rst;

    reg  [DATA_W-1:0] wr_data;
    reg               wr_valid;
    wire              wr_ready;

    wire [DATA_W-1:0] rd_data;
    wire              rd_valid;
    reg               rd_ready;

    integer write_count;
    integer read_count;
    integer wr_stall_count;
    integer rd_cycle;

    dpti_async_fifo #(
        .DATA_W (DATA_W),
        .DEPTH  (DEPTH)
    ) dut (
        .wr_clk   (wr_clk),
        .wr_rst   (wr_rst),
        .wr_valid (wr_valid),
        .wr_ready (wr_ready),
        .wr_data  (wr_data),

        .rd_clk   (rd_clk),
        .rd_rst   (rd_rst),
        .rd_valid (rd_valid),
        .rd_ready (rd_ready),
        .rd_data  (rd_data)
    );

    // Write clock: 10 ns.
    initial begin
        wr_clk = 1'b0;
        forever #5 wr_clk = ~wr_clk;
    end

    // Read clock: 14 ns.
    initial begin
        rd_clk = 1'b0;
        forever #7 rd_clk = ~rd_clk;
    end

    // ------------------------------------------------------------------------
    // Writer.
    //
    // Keep valid asserted until each word is accepted.
    // ------------------------------------------------------------------------

    initial begin
        wr_valid      = 1'b0;
        wr_data       = '0;
        write_count   = 0;
        wr_stall_count = 0;

        wait (!wr_rst);

        @(negedge wr_clk);

        while (write_count < COUNT) begin
            wr_valid = 1'b1;
            wr_data  = 32'h10000000 + write_count;

            @(posedge wr_clk);

            if (wr_ready) begin
                write_count = write_count + 1;

                @(negedge wr_clk);
            end
            else begin
                wr_stall_count = wr_stall_count + 1;

                // Keep wr_valid/wr_data stable.
                @(negedge wr_clk);
            end
        end

        wr_valid = 1'b0;
        wr_data  = '0;
    end

    // ------------------------------------------------------------------------
    // Reader.
    //
    // Add extra periodic stalls on top of the slower read clock.
    // ------------------------------------------------------------------------

    always @(posedge rd_clk) begin
        if (rd_rst) begin
            rd_cycle <= 0;
            rd_ready <= 1'b0;
        end
        else begin
            rd_cycle <= rd_cycle + 1;

            // Stall every third read-domain cycle.
            rd_ready <= ((rd_cycle % 3) != 0);
        end
    end

    // ------------------------------------------------------------------------
    // Scoreboard.
    // ------------------------------------------------------------------------

    always @(posedge rd_clk) begin
        if (rd_rst) begin
            read_count <= 0;
        end
        else if (rd_valid && rd_ready) begin

            if (rd_data !==
                (32'h10000000 + read_count)) begin

                $display(
                    "FAIL: read #%0d data=%08x expected=%08x",
                    read_count,
                    rd_data,
                    (32'h10000000 + read_count)
                );
                $finish(1);
            end

            read_count <= read_count + 1;
        end
    end

    // ------------------------------------------------------------------------
    // Reset + completion.
    // ------------------------------------------------------------------------

    integer timeout;

    initial begin
        wr_rst = 1'b1;
        rd_rst = 1'b1;

        repeat (5)
            @(posedge wr_clk);

        @(negedge wr_clk);
        wr_rst = 1'b0;

        repeat (2)
            @(posedge rd_clk);

        @(negedge rd_clk);
        rd_rst = 1'b0;

        timeout = 0;

        while ((read_count < COUNT) &&
               (timeout < 2000)) begin
            @(posedge rd_clk);
            timeout = timeout + 1;
        end

        if (read_count != COUNT) begin
            $display(
                "FAIL: timeout read_count=%0d expected=%0d",
                read_count,
                COUNT
            );
            $finish(1);
        end

        // Allow final nonblocking assignments to settle.
        @(posedge rd_clk);
        #1;

        if (write_count != COUNT) begin
            $display(
                "FAIL: write_count=%0d expected=%0d",
                write_count,
                COUNT
            );
            $finish(1);
        end

        if (wr_stall_count == 0) begin
            $display(
                "FAIL: FIFO never applied write-side backpressure"
            );
            $finish(1);
        end

        $display("----------------------------------------");
        $display("DPTI ASYNC FIFO TEST: PASS");
        $display(
            "written=%0d read=%0d write_stalls=%0d",
            write_count,
            read_count,
            wr_stall_count
        );
        $display("----------------------------------------");

        $finish(0);
    end

endmodule

`default_nettype wire
