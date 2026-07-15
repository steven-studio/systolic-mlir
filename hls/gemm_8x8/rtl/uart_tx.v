// UART 傳送模組:100MHz clock, 115200 baud
module uart_tx #(
    parameter CLK_FREQ  = 100_000_000,
    parameter BAUD_RATE = 115200
) (
    input  wire       clk,
    input  wire       rst,
    input  wire [7:0] data,
    input  wire       start,
    output reg        tx = 1,
    output reg        busy = 0
);
    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;
    localparam IDLE = 0, START = 1, DATA = 2, STOP = 3;
    reg [1:0] state = IDLE;
    reg [15:0] clk_cnt = 0;
    reg [2:0] bit_idx = 0;
    reg [7:0] shift_reg = 0;

    always @(posedge clk) begin
        if (rst) begin
            state <= IDLE;
            tx <= 1;
            busy <= 0;
        end else begin
            case (state)
                IDLE: begin
                    tx <= 1;
                    if (start) begin
                        shift_reg <= data;
                        busy <= 1;
                        clk_cnt <= 0;
                        state <= START;
                    end else begin
                        busy <= 0;
                    end
                end
                START: begin
                    tx <= 0;
                    if (clk_cnt == CLKS_PER_BIT-1) begin
                        clk_cnt <= 0;
                        bit_idx <= 0;
                        state <= DATA;
                    end else begin
                        clk_cnt <= clk_cnt + 1;
                    end
                end
                DATA: begin
                    tx <= shift_reg[bit_idx];
                    if (clk_cnt == CLKS_PER_BIT-1) begin
                        clk_cnt <= 0;
                        if (bit_idx == 7) begin
                            state <= STOP;
                        end else begin
                            bit_idx <= bit_idx + 1;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1;
                    end
                end
                STOP: begin
                    tx <= 1;
                    if (clk_cnt == CLKS_PER_BIT-1) begin
                        clk_cnt <= 0;
                        busy <= 0;
                        state <= IDLE;
                    end else begin
                        clk_cnt <= clk_cnt + 1;
                    end
                end
                default: state <= IDLE;
            endcase
        end
    end
endmodule
