module uart_echo_top #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200
) (
    input  logic clk,
    input  logic rst,
    input  logic uart_rx,
    output logic uart_tx
);

    logic [7:0] rx_byte;
    logic       rx_valid;

    logic [7:0] tx_byte;
    logic       tx_start;
    logic       tx_busy;

    uart_rx #(
        .CLK_HZ(CLK_HZ),
        .BAUD(BAUD)
    ) u_rx (
        .clk(clk),
        .rst(rst),
        .rx(uart_rx),
        .data_out(rx_byte),
        .valid(rx_valid)
    );

    uart_tx #(
        .CLK_HZ(CLK_HZ),
        .BAUD(BAUD)
    ) u_tx (
        .clk(clk),
        .rst(rst),
        .start(tx_start),
        .data_in(tx_byte),
        .tx(uart_tx),
        .busy(tx_busy)
    );

    typedef enum logic [1:0] {
        WAIT_RX,
        START_TX,
        WAIT_BUSY,
        WAIT_DONE
    } state_t;

    state_t state;

    always_ff @(posedge clk) begin
        if (rst) begin
            state    <= WAIT_RX;
            tx_byte  <= 8'h00;
            tx_start <= 1'b0;
        end
        else begin
            tx_start <= 1'b0;

            case (state)

                WAIT_RX: begin
                    if (rx_valid) begin
                        tx_byte <= rx_byte;
                        state   <= START_TX;
                    end
                end

                START_TX: begin
                    if (!tx_busy) begin
                        tx_start <= 1'b1;
                        state    <= WAIT_BUSY;
                    end
                end

                WAIT_BUSY: begin
                    if (tx_busy)
                        state <= WAIT_DONE;
                end

                WAIT_DONE: begin
                    if (!tx_busy)
                        state <= WAIT_RX;
                end

                default:
                    state <= WAIT_RX;

            endcase
        end
    end

endmodule
