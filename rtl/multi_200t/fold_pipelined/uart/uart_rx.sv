module uart_rx #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200
) (
    input  logic       clk,
    input  logic       rst,
    input  logic       rx,

    output logic [7:0] data_out,
    output logic       valid
);

    localparam int CLKS_PER_BIT = CLK_HZ / BAUD;

    typedef enum logic [1:0] {
        IDLE,
        START_BIT,
        DATA_BITS,
        STOP_BIT
    } state_t;

    state_t state;

    logic [$clog2(CLKS_PER_BIT+1)-1:0] clk_count;
    logic [2:0] bit_index;
    logic [7:0] rx_data;

    // Synchronizer
    logic rx_meta;
    logic rx_sync;

    always_ff @(posedge clk) begin
        rx_meta <= rx;
        rx_sync <= rx_meta;
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            state     <= IDLE;
            clk_count <= 0;
            bit_index <= 0;
            rx_data   <= 0;
            data_out  <= 0;
            valid     <= 0;
        end
        else begin
            valid <= 0;

            case (state)

                IDLE: begin
                    clk_count <= 0;
                    bit_index <= 0;

                    if (rx_sync == 1'b0)
                        state <= START_BIT;
                end

                START_BIT: begin
                    // Sample middle of start bit
                    if (clk_count == (CLKS_PER_BIT-1)/2) begin
                        clk_count <= 0;

                        if (rx_sync == 1'b0)
                            state <= DATA_BITS;
                        else
                            state <= IDLE;
                    end
                    else begin
                        clk_count <= clk_count + 1'b1;
                    end
                end

                DATA_BITS: begin
                    if (clk_count == CLKS_PER_BIT-1) begin
                        clk_count <= 0;

                        rx_data[bit_index] <= rx_sync;

                        if (bit_index == 7) begin
                            bit_index <= 0;
                            state <= STOP_BIT;
                        end
                        else begin
                            bit_index <= bit_index + 1'b1;
                        end
                    end
                    else begin
                        clk_count <= clk_count + 1'b1;
                    end
                end

                STOP_BIT: begin
                    if (clk_count == CLKS_PER_BIT-1) begin
                        clk_count <= 0;

                        if (rx_sync == 1'b1) begin
                            data_out <= rx_data;
                            valid    <= 1'b1;
                        end

                        state <= IDLE;
                    end
                    else begin
                        clk_count <= clk_count + 1'b1;
                    end
                end

            endcase
        end
    end

endmodule
