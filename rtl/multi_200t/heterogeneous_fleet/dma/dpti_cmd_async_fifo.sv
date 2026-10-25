`default_nettype none

// ============================================================================
// dpti_cmd_async_fifo.sv
//
// Small asynchronous FIFO for crossing DPTI commands from the FT2232H
// dpti_clkout clock domain into the system ui_clk domain.
//
// FIFO payload:
//
//   [ADDR_W-1:0] address
//   [31:0]       data
//
// Write side:
//   dpti_clkout
//
// Read side:
//   ui_clk
//
// Standard Gray-pointer asynchronous FIFO.
// ============================================================================

module dpti_cmd_async_fifo #(
    parameter integer ADDR_W = 8,
    parameter integer DEPTH  = 4
)(
    // ------------------------------------------------------------------------
    // Write clock domain: DPTI
    // ------------------------------------------------------------------------

    input  wire                  wr_clk,
    input  wire                  wr_rst,

    input  wire                  wr_valid,
    output wire                  wr_ready,

    input  wire [ADDR_W-1:0]     wr_addr,
    input  wire [31:0]          wr_data,

    // ------------------------------------------------------------------------
    // Read clock domain: ui_clk
    // ------------------------------------------------------------------------

    input  wire                  rd_clk,
    input  wire                  rd_rst,

    output wire                  rd_valid,
    input  wire                  rd_ready,

    output wire [ADDR_W-1:0]     rd_addr,
    output wire [31:0]           rd_data
);

    localparam integer PTR_W = $clog2(DEPTH) + 1;

    reg [ADDR_W+31:0] mem [0:DEPTH-1];

    reg [PTR_W-1:0] wr_ptr_bin;
    reg [PTR_W-1:0] wr_ptr_gray;

    reg [PTR_W-1:0] rd_ptr_bin;
    reg [PTR_W-1:0] rd_ptr_gray;

    reg [PTR_W-1:0] rd_ptr_gray_wr1;
    reg [PTR_W-1:0] rd_ptr_gray_wr2;

    reg [PTR_W-1:0] wr_ptr_gray_rd1;
    reg [PTR_W-1:0] wr_ptr_gray_rd2;

    wire [PTR_W-1:0] wr_ptr_bin_next;
    wire [PTR_W-1:0] wr_ptr_gray_next;

    wire [PTR_W-1:0] rd_ptr_bin_next;
    wire [PTR_W-1:0] rd_ptr_gray_next;

    wire fifo_full;
    wire fifo_empty;

    // ------------------------------------------------------------------------
    // Gray-code conversion.
    // ------------------------------------------------------------------------

    function [PTR_W-1:0] bin_to_gray;
        input [PTR_W-1:0] bin;
        begin
            bin_to_gray = (bin >> 1) ^ bin;
        end
    endfunction

    // ------------------------------------------------------------------------
    // Full detection.
    //
    // For a power-of-two FIFO, full occurs when the next write pointer
    // equals the synchronized read pointer with the upper two Gray bits
    // inverted.
    // ------------------------------------------------------------------------

    // ------------------------------------------------------------------------
    // Full detection.
    //
    // IMPORTANT:
    //   wr_ready must not depend on wr_ptr_gray_next, because
    //   wr_ptr_gray_next depends on wr_ready through the write-enable.
    //
    //   Therefore compute the current full state from the current write
    //   pointer and synchronized read pointer.  This keeps wr_ready purely
    //   combinational from registered state and removes the combinational
    //   feedback loop:
    //
    //       wr_ready -> wr_ptr_bin_next -> fifo_full -> wr_ready
    //
    // ------------------------------------------------------------------------

    assign fifo_full =
        (wr_ptr_gray ==
         {
             ~rd_ptr_gray_wr2[PTR_W-1:PTR_W-2],
              rd_ptr_gray_wr2[PTR_W-3:0]
         });

    assign wr_ready = !fifo_full;

    assign wr_ptr_bin_next =
        wr_ptr_bin + ((wr_valid && wr_ready) ? 1'b1 : 1'b0);

    assign wr_ptr_gray_next =
        bin_to_gray(wr_ptr_bin_next);

    // ------------------------------------------------------------------------
    // Empty detection.
    // ------------------------------------------------------------------------

    assign rd_ptr_bin_next =
        rd_ptr_bin + ((rd_valid && rd_ready) ? 1'b1 : 1'b0);

    assign rd_ptr_gray_next =
        bin_to_gray(rd_ptr_bin_next);

    assign fifo_empty =
        (rd_ptr_gray == wr_ptr_gray_rd2);

    assign rd_valid = !fifo_empty;

    assign rd_addr = mem[rd_ptr_bin[PTR_W-2:0]][ADDR_W+31:32];

    assign rd_data = mem[rd_ptr_bin[PTR_W-2:0]][31:0];

    // ------------------------------------------------------------------------
    // Write clock domain.
    // ------------------------------------------------------------------------

    always @(posedge wr_clk) begin
        if (wr_rst) begin
            wr_ptr_bin  <= {PTR_W{1'b0}};
            wr_ptr_gray <= {PTR_W{1'b0}};

            rd_ptr_gray_wr1 <= {PTR_W{1'b0}};
            rd_ptr_gray_wr2 <= {PTR_W{1'b0}};
        end
        else begin

            // Synchronize read pointer into write clock domain.
            rd_ptr_gray_wr1 <= rd_ptr_gray;
            rd_ptr_gray_wr2 <= rd_ptr_gray_wr1;

            if (wr_valid && wr_ready) begin
                mem[wr_ptr_bin[PTR_W-2:0]] <= {
                    wr_addr,
                    wr_data
                };

                wr_ptr_bin  <= wr_ptr_bin_next;
                wr_ptr_gray <= wr_ptr_gray_next;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Read clock domain.
    // ------------------------------------------------------------------------

    always @(posedge rd_clk) begin
        if (rd_rst) begin
            rd_ptr_bin  <= {PTR_W{1'b0}};
            rd_ptr_gray <= {PTR_W{1'b0}};

            wr_ptr_gray_rd1 <= {PTR_W{1'b0}};
            wr_ptr_gray_rd2 <= {PTR_W{1'b0}};
        end
        else begin

            // Synchronize write pointer into read clock domain.
            wr_ptr_gray_rd1 <= wr_ptr_gray;
            wr_ptr_gray_rd2 <= wr_ptr_gray_rd1;

            if (rd_valid && rd_ready) begin
                rd_ptr_bin  <= rd_ptr_bin_next;
                rd_ptr_gray <= rd_ptr_gray_next;
            end
        end
    end

endmodule

`default_nettype wire
