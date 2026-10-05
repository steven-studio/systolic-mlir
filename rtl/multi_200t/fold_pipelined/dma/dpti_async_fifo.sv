`default_nettype none

// ============================================================================
// dpti_async_fifo.sv
//
// Generic asynchronous FIFO.
//
// Based on the already-used Gray-pointer CDC structure in
// dpti_cmd_async_fifo.sv.
//
// Requirements:
//   DEPTH must be a power of two and at least 4.
// ============================================================================

module dpti_async_fifo #(
    parameter integer DATA_W = 128,
    parameter integer DEPTH  = 8
) (
    input  wire                  wr_clk,
    input  wire                  wr_rst,

    input  wire                  wr_valid,
    output wire                  wr_ready,
    input  wire [DATA_W-1:0]     wr_data,

    input  wire                  rd_clk,
    input  wire                  rd_rst,

    output wire                  rd_valid,
    input  wire                  rd_ready,
    output wire [DATA_W-1:0]     rd_data
);

    localparam integer ADDR_W = $clog2(DEPTH);
    localparam integer PTR_W  = ADDR_W + 1;

    reg [DATA_W-1:0] mem [0:DEPTH-1];

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

    function [PTR_W-1:0] bin_to_gray;
        input [PTR_W-1:0] bin;
        begin
            bin_to_gray = (bin >> 1) ^ bin;
        end
    endfunction

    assign fifo_full =
        (wr_ptr_gray ==
         {
             ~rd_ptr_gray_wr2[PTR_W-1:PTR_W-2],
              rd_ptr_gray_wr2[PTR_W-3:0]
         });

    assign wr_ready = !fifo_full;

    assign wr_ptr_bin_next =
        wr_ptr_bin +
        ((wr_valid && wr_ready) ? 1'b1 : 1'b0);

    assign wr_ptr_gray_next =
        bin_to_gray(wr_ptr_bin_next);

    assign fifo_empty =
        (rd_ptr_gray == wr_ptr_gray_rd2);

    assign rd_valid = !fifo_empty;

    assign rd_data =
        mem[rd_ptr_bin[ADDR_W-1:0]];

    assign rd_ptr_bin_next =
        rd_ptr_bin +
        ((rd_valid && rd_ready) ? 1'b1 : 1'b0);

    assign rd_ptr_gray_next =
        bin_to_gray(rd_ptr_bin_next);

    // ------------------------------------------------------------------------
    // Write clock domain.
    // ------------------------------------------------------------------------

    always @(posedge wr_clk) begin
        if (wr_rst) begin
            wr_ptr_bin       <= {PTR_W{1'b0}};
            wr_ptr_gray      <= {PTR_W{1'b0}};

            rd_ptr_gray_wr1  <= {PTR_W{1'b0}};
            rd_ptr_gray_wr2  <= {PTR_W{1'b0}};
        end
        else begin
            rd_ptr_gray_wr1 <= rd_ptr_gray;
            rd_ptr_gray_wr2 <= rd_ptr_gray_wr1;

            if (wr_valid && wr_ready) begin
                mem[wr_ptr_bin[ADDR_W-1:0]] <= wr_data;

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
            rd_ptr_bin       <= {PTR_W{1'b0}};
            rd_ptr_gray      <= {PTR_W{1'b0}};

            wr_ptr_gray_rd1  <= {PTR_W{1'b0}};
            wr_ptr_gray_rd2  <= {PTR_W{1'b0}};
        end
        else begin
            wr_ptr_gray_rd1 <= wr_ptr_gray;
            wr_ptr_gray_rd2 <= wr_ptr_gray_rd1;

            if (rd_valid && rd_ready) begin
                rd_ptr_bin  <= rd_ptr_bin_next;
                rd_ptr_gray <= rd_ptr_gray_next;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Elaboration checks.
    // ------------------------------------------------------------------------

    initial begin
        if (DEPTH < 4)
            $fatal(
                1,
                "dpti_async_fifo: DEPTH must be at least 4"
            );

        if ((DEPTH & (DEPTH - 1)) != 0)
            $fatal(
                1,
                "dpti_async_fifo: DEPTH must be a power of two"
            );

        if (DATA_W <= 0)
            $fatal(
                1,
                "dpti_async_fifo: DATA_W must be positive"
            );
    end

endmodule

`default_nettype wire
