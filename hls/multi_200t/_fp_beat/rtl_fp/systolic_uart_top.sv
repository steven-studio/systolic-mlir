module systolic_uart_top #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200
) (
    input  logic clk,
    input  logic rst,

    input  logic uart_rx,
    output logic uart_tx
);

    /*
     * ============================================================
     * UART RX/TX byte interface
     * ============================================================
     */
    logic [7:0] rx_byte;
    logic       rx_valid;

    logic [7:0] tx_byte;
    logic       tx_start;
    logic       tx_busy;

    uart_rx #(
        .CLK_HZ (CLK_HZ),
        .BAUD   (BAUD)
    ) u_uart_rx (
        .clk      (clk),
        .rst      (rst),
        .rx       (uart_rx),
        .data_out (rx_byte),
        .valid    (rx_valid)
    );

    uart_tx #(
        .CLK_HZ (CLK_HZ),
        .BAUD   (BAUD)
    ) u_uart_tx (
        .clk     (clk),
        .rst     (rst),
        .start   (tx_start),
        .data_in (tx_byte),
        .tx      (uart_tx),
        .busy    (tx_busy)
    );


    /*
     * ============================================================
     * Matrix storage
     *
     * A = 64 FP32 = 256 bytes
     * B = 64 FP32 = 256 bytes
     * ============================================================
     */
    logic [31:0] A [0:7][0:7];
    logic [31:0] B [0:7][0:7];
    logic [31:0] C [0:7][0:7];

    logic [8:0] rx_count;

    logic [1:0] byte_pos;
    logic [31:0] word_buf;

    logic matrices_ready;


    /*
     * ============================================================
     * Receive 512 bytes:
     *
     * byte 0..255   -> A
     * byte 256..511 -> B
     *
     * little-endian FP32
     * ============================================================
     */
    always_ff @(posedge clk) begin
        if (rst) begin
            rx_count       <= 0;
            byte_pos       <= 0;
            word_buf       <= 0;
            matrices_ready <= 0;
        end
        else begin
            matrices_ready <= 0;

            if (rx_valid) begin
                case (byte_pos)
                    2'd0: word_buf[7:0]   <= rx_byte;
                    2'd1: word_buf[15:8]  <= rx_byte;
                    2'd2: word_buf[23:16] <= rx_byte;

                    2'd3: begin
                        word_buf[31:24] <= rx_byte;

                        if (rx_count < 256) begin
                            A[(rx_count >> 5) & 7]
                             [(rx_count >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };
                        end
                        else begin
                            B[((rx_count - 256) >> 5) & 7]
                             [((rx_count - 256) >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };
                        end
                    end
                endcase

                if (byte_pos == 3)
                    byte_pos <= 0;
                else
                    byte_pos <= byte_pos + 1'b1;

                if (rx_count == 511) begin
                    rx_count       <= 0;
                    matrices_ready <= 1;
                end
                else begin
                    rx_count <= rx_count + 1'b1;
                end
            end
        end
    end


    /*
     * ============================================================
     * GEMM controller <-> systolic array
     * ============================================================
     */
    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic [3:0] acc_sel;

    logic reduce_start;
    logic c_valid_out;

    logic [31:0] c_out [0:7][0:7];

    logic [31:0] dbg_acc_out [0:7][0:7][0:15];

    logic gemm_start;
    logic gemm_done;

    assign gemm_start = matrices_ready;

    gemm_controller u_gemm_controller (
        .clk          (clk),
        .rst          (rst),

        .start        (gemm_start),

        .A            (A),
        .B            (B),

        .a_in         (a_in),
        .b_in         (b_in),

        .a_valid_in   (a_valid_in),
        .b_valid_in   (b_valid_in),

        .acc_sel      (acc_sel),
        .reduce_start (reduce_start),

        .c_valid_in   (c_valid_out),
        .c_in         (c_out),

        .done         (gemm_done),

        .C            (C)
    );


    systolic_array_8x8 u_array (
        .clk          (clk),
        .rst          (rst),

        .a_in         (a_in),
        .b_in         (b_in),

        .a_valid_in   (a_valid_in),
        .b_valid_in   (b_valid_in),

        .acc_sel      (acc_sel),

        .reduce_start (reduce_start),

        .c_valid_out  (c_valid_out),
        .c_out        (c_out),

        .dbg_acc_out  (dbg_acc_out)
    );


    /*
     * ============================================================
     * UART transmit C matrix
     *
     * 64 FP32 = 256 bytes
     * ============================================================
     */

    logic       sending;
    logic [8:0] tx_count;
    logic [31:0] tx_word;

    typedef enum logic [1:0] {
        TX_IDLE,
        TX_START,
        TX_WAIT_BUSY,
        TX_WAIT_DONE
    } tx_state_t;

    tx_state_t tx_state;


    /*
     * Select current byte from C matrix.
     */
    always_comb begin
        tx_word =
            C[(tx_count >> 5) & 7]
             [(tx_count >> 2) & 7];

        case (tx_count[1:0])
            2'd0: tx_byte = tx_word[7:0];
            2'd1: tx_byte = tx_word[15:8];
            2'd2: tx_byte = tx_word[23:16];
            2'd3: tx_byte = tx_word[31:24];
        endcase
    end


    /*
     * UART TX handshake.
     *
     * Important:
     * Do NOT increment tx_count merely because tx_busy is low.
     *
     * tx_start is registered, therefore uart_tx sees it one clock
     * later. We explicitly wait for busy to assert, and then wait
     * for busy to deassert before advancing to the next byte.
     */
    always_ff @(posedge clk) begin
        if (rst) begin
            sending  <= 1'b0;
            tx_count <= 9'd0;
            tx_start <= 1'b0;
            tx_state <= TX_IDLE;
        end
        else begin
            tx_start <= 1'b0;

            case (tx_state)

                TX_IDLE: begin
                    if (gemm_done) begin
                        sending  <= 1'b1;
                        tx_count <= 9'd0;
                        tx_state <= TX_START;
                    end
                end


                /*
                 * Pulse start for exactly one clock.
                 */
                TX_START: begin
                    if (!tx_busy) begin
                        tx_start <= 1'b1;
                        tx_state <= TX_WAIT_BUSY;
                    end
                end


                /*
                 * uart_tx receives tx_start one clock later.
                 * Wait until it acknowledges by raising busy.
                 */
                TX_WAIT_BUSY: begin
                    if (tx_busy) begin
                        tx_state <= TX_WAIT_DONE;
                    end
                end


                /*
                 * Wait until the complete UART byte,
                 * including stop bit, has been transmitted.
                 */
                TX_WAIT_DONE: begin
                    if (!tx_busy) begin

                        if (tx_count == 9'd255) begin
                            sending  <= 1'b0;
                            tx_count <= 9'd0;
                            tx_state <= TX_IDLE;
                        end
                        else begin
                            tx_count <= tx_count + 1'b1;
                            tx_state <= TX_START;
                        end

                    end
                end

            endcase
        end
    end

endmodule
