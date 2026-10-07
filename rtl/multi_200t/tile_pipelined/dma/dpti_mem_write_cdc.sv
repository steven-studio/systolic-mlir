`default_nettype none

module dpti_mem_write_cdc #(
    parameter integer ADDR_W = 29,
    parameter integer DATA_W = 128,
    parameter integer DEPTH  = 8
) (
    // Source domain.
    input  wire                 src_clk,
    input  wire                 src_rst,

    input  wire                 src_start,
    input  wire [ADDR_W-1:0]    src_base_addr,

    input  wire [DATA_W-1:0]    src_data,
    input  wire                 src_valid,
    output wire                 src_ready,

    // Destination domain.
    input  wire                 dst_clk,
    input  wire                 dst_rst,

    output reg                  dst_start,
    output reg  [ADDR_W-1:0]    dst_base_addr,

    output wire [DATA_W-1:0]    dst_data,
    output wire                 dst_valid,
    input  wire                 dst_ready,

    output reg                  err_protocol
);

    localparam integer ENTRY_W =
        1 + ADDR_W + DATA_W;

    localparam integer TYPE_BIT =
        ENTRY_W - 1;

    wire [ENTRY_W-1:0] fifo_wr_data;
    wire               fifo_wr_valid;
    wire               fifo_wr_ready;

    wire [ENTRY_W-1:0] fifo_rd_data;
    wire               fifo_rd_valid;
    wire               fifo_rd_ready;

    reg header_pending;
    reg [ADDR_W-1:0] header_addr;

    // ------------------------------------------------------------------------
    // Source domain.
    //
    // src_start is converted into one HEADER entry.  Payload cannot advance
    // until that HEADER has entered the FIFO.
    // ------------------------------------------------------------------------

    assign fifo_wr_valid =
        header_pending || src_valid;

    assign fifo_wr_data =
        header_pending
            ? {
                1'b1,
                header_addr,
                {DATA_W{1'b0}}
              }
            : {
                1'b0,
                {ADDR_W{1'b0}},
                src_data
              };

    assign src_ready =
        !header_pending &&
        fifo_wr_ready;

    always @(posedge src_clk) begin
        if (src_rst) begin
            header_pending <= 1'b0;
            header_addr    <= {ADDR_W{1'b0}};
        end
        else begin
            if (src_start) begin
                header_pending <= 1'b1;
                header_addr    <= src_base_addr;
            end

            if (header_pending &&
                fifo_wr_ready) begin
                header_pending <= 1'b0;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Generic asynchronous FIFO.
    // ------------------------------------------------------------------------

    dpti_async_fifo #(
        .DATA_W (ENTRY_W),
        .DEPTH  (DEPTH)
    ) u_fifo (
        .wr_clk   (src_clk),
        .wr_rst   (src_rst),

        .wr_valid (fifo_wr_valid),
        .wr_ready (fifo_wr_ready),
        .wr_data  (fifo_wr_data),

        .rd_clk   (dst_clk),
        .rd_rst   (dst_rst),

        .rd_valid (fifo_rd_valid),
        .rd_ready (fifo_rd_ready),
        .rd_data  (fifo_rd_data)
    );

    // ------------------------------------------------------------------------
    // Destination domain.
    //
    // HEADER entries are consumed internally and generate dst_start.
    // DATA entries are exposed through valid/ready.
    // ------------------------------------------------------------------------

    wire fifo_is_header =
        fifo_rd_data[TYPE_BIT];

    wire [ADDR_W-1:0] fifo_header_addr =
        fifo_rd_data[DATA_W +: ADDR_W];

    assign dst_data =
        fifo_rd_data[DATA_W-1:0];

    assign dst_valid =
        fifo_rd_valid &&
        !fifo_is_header;

    assign fifo_rd_ready =
        fifo_rd_valid
            ? (
                fifo_is_header
                    ? 1'b1
                    : dst_ready
              )
            : 1'b0;

    always @(posedge dst_clk) begin
        if (dst_rst) begin
            dst_start    <= 1'b0;
            dst_base_addr <= {ADDR_W{1'b0}};
            err_protocol <= 1'b0;
        end
        else begin
            dst_start <= 1'b0;

            if (fifo_rd_valid &&
                fifo_rd_ready &&
                fifo_is_header) begin

                dst_base_addr <= fifo_header_addr;
                dst_start     <= 1'b1;
            end
        end
    end

endmodule

`default_nettype wire
