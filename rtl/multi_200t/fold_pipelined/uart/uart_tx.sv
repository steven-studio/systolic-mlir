module uart_tx #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200
) (
    input  logic       clk,
    input  logic       rst,

    input  logic       start,
    input  logic [7:0] data_in,

    output logic       tx,
    output logic       busy
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
    logic [7:0] tx_data;

    always_ff @(posedge clk) begin
        if (rst) begin
            state     <= IDLE;
            clk_count <= 0;
            bit_index <= 0;
            tx_data   <= 0;

            tx   <= 1'b1;
            busy <= 1'b0;
        end
        else begin
            case (state)

                IDLE: begin
                    tx        <= 1'b1;
                    busy      <= 1'b0;
                    clk_count <= 0;
                    bit_index <= 0;

                    if (start) begin
                        tx_data <= data_in;
                        busy    <= 1'b1;
                        state   <= START_BIT;
                    end
                end

                START_BIT: begin
                    tx   <= 1'b0;
                    busy <= 1'b1;

                    if (clk_count == CLKS_PER_BIT-1) begin
                        clk_count <= 0;
                        state <= DATA_BITS;
                    end
                    else begin
                        clk_count <= clk_count + 1'b1;
                    end
                end

                DATA_BITS: begin
                    tx   <= tx_data[bit_index];
                    busy <= 1'b1;

                    if (clk_count == CLKS_PER_BIT-1) begin
                        clk_count <= 0;

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
                    tx   <= 1'b1;
                    busy <= 1'b1;

                    if (clk_count == CLKS_PER_BIT-1) begin
                        clk_count <= 0;
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
