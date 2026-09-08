// -----------------------------------------------------------------------------
// uart_tx.sv -- minimal 8N1 transmitter.
//
// Deliberately standalone: it does not touch the existing UART on the array
// side, so bringing DDR3 up cannot break the path that carries CYCLE_COUNTER.
// One byte in, one frame out; hold `valid` until `ready` is seen low.
//
// DIV = CLK_HZ / BAUD must be >= 2.  At 200 MHz / 115200 that is 1736.
// -----------------------------------------------------------------------------

`default_nettype none

module uart_tx #(
  parameter int unsigned CLK_HZ = 200_000_000,
  parameter int unsigned BAUD   = 115_200
) (
  input  wire       clk,
  input  wire       rst_n,
  input  wire [7:0] data,
  input  wire       valid,     // assert with data; accepted when ready is high
  output wire       ready,     // 1 = idle, can take a byte this cycle
  output logic      tx
);

  localparam int unsigned DIV   = (CLK_HZ / BAUD) < 2 ? 2 : (CLK_HZ / BAUD);
  localparam int unsigned DIV_W = $clog2(DIV);

  logic [DIV_W-1:0] baud_cnt;
  logic [3:0]       bit_idx;
  logic [9:0]       shifter;   // {stop, data[7:0], start}
  logic             busy;

  assign ready = ~busy;

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      busy     <= 1'b0;
      tx       <= 1'b1;
      baud_cnt <= '0;
      bit_idx  <= '0;
      shifter  <= 10'h3FF;
    end else if (busy) begin
      if (baud_cnt == DIV_W'(DIV - 1)) begin
        baud_cnt <= '0;
        tx       <= shifter[0];
        shifter  <= {1'b1, shifter[9:1]};
        bit_idx  <= bit_idx + 4'd1;
        if (bit_idx == 4'd9) busy <= 1'b0;   // start + 8 data + stop = 10 bits
      end else begin
        baud_cnt <= baud_cnt + 1'b1;
      end
    end else begin
      tx <= 1'b1;                            // idle high
      if (valid) begin
        shifter  <= {1'b1, data, 1'b0};
        busy     <= 1'b1;
        bit_idx  <= '0;
        baud_cnt <= '0;
      end
    end
  end

endmodule

`default_nettype wire
