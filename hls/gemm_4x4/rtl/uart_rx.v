// UART 接收模組:100MHz clock, 115200 baud
module uart_rx #(
    parameter CLK_FREQ  = 100_000_000,
    parameter BAUD_RATE = 115200
) (
    input  wire       clk,
    input  wire       rst,
    input  wire       rx,
    output reg  [7:0] data,
    output reg        valid
);
    localparam CLKS_PER_BIT = CLK_FREQ / BAUD_RATE;

    localparam IDLE  = 0, START = 1, DATA = 2, STOP = 3;
    reg [1:0] state = IDLE;
    reg [15:0] clk_cnt = 0;
    reg [2:0] bit_idx = 0;
    reg [7:0] shift_reg = 0;

    // 兩級同步器,避免 metastability
    reg rx_ff1 = 1, rx_ff2 = 1;
    always @(posedge clk) begin
        rx_ff1 <= rx;
        rx_ff2 <= rx_ff1;
    end

    always @(posedge clk) begin
        if (rst) begin
            state <= IDLE;
            valid <= 0;
            clk_cnt <= 0;
            bit_idx <= 0;
        end else begin
            valid <= 0;
            case (state)
                IDLE: begin
                    clk_cnt <= 0;
                    bit_idx <= 0;
                    if (rx_ff2 == 0) state <= START;
                end
                START: begin
                    if (clk_cnt == CLKS_PER_BIT/2) begin
                        if (rx_ff2 == 0) begin
                            clk_cnt <= 0;
                            state <= DATA;
                        end else begin
                            state <= IDLE;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1;
                    end
                end
                DATA: begin
                    if (clk_cnt == CLKS_PER_BIT-1) begin
                        clk_cnt <= 0;
                        shift_reg[bit_idx] <= rx_ff2;
                        if (bit_idx == 7) begin
                            bit_idx <= 0;
                            state <= STOP;
                        end else begin
                            bit_idx <= bit_idx + 1;
                        end
                    end else begin
                        clk_cnt <= clk_cnt + 1;
                    end
                end
                STOP: begin
                    if (clk_cnt == CLKS_PER_BIT-1) begin
                        clk_cnt <= 0;
                        data <= shift_reg;
                        valid <= 1;
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
