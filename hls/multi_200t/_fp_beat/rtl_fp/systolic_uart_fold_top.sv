module systolic_uart_fold_top #(
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
     * UART
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
     * Two independent 8x8 GEMMs
     *
     * UART input:
     *
     *   bytes    0..255  = A0
     *   bytes  256..511  = B0
     *   bytes  512..767  = A1
     *   bytes  768..1023 = B1
     *
     * All FP32 little-endian.
     * ============================================================
     */
    logic [31:0] A0 [0:7][0:7];
    logic [31:0] B0 [0:7][0:7];

    logic [31:0] A1 [0:7][0:7];
    logic [31:0] B1 [0:7][0:7];

    logic [10:0] rx_count;
    logic [1:0]  byte_pos;
    logic [31:0] word_buf;

    logic matrices_ready;


    /*
     * ============================================================
     * RX
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

                    2'd0:
                        word_buf[7:0] <= rx_byte;

                    2'd1:
                        word_buf[15:8] <= rx_byte;

                    2'd2:
                        word_buf[23:16] <= rx_byte;

                    2'd3: begin

                        word_buf[31:24] <= rx_byte;

                        /*
                         * A0
                         */
                        if (rx_count < 256) begin

                            A0[(rx_count >> 5) & 7]
                              [(rx_count >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                        /*
                         * B0
                         */
                        else if (rx_count < 512) begin

                            B0[((rx_count - 256) >> 5) & 7]
                              [((rx_count - 256) >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                        /*
                         * A1
                         */
                        else if (rx_count < 768) begin

                            A1[((rx_count - 512) >> 5) & 7]
                              [((rx_count - 512) >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                        /*
                         * B1
                         */
                        else begin

                            B1[((rx_count - 768) >> 5) & 7]
                              [((rx_count - 768) >> 2) & 7]
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


                /*
                 * 1024 bytes total.
                 */
                if (rx_count == 1023) begin

                    rx_count       <= 0;
                    matrices_ready <= 1'b1;

                end
                else begin

                    rx_count <= rx_count + 1'b1;

                end

            end

        end
    end


    /*
     * ============================================================
     * Fold-pipelined 8x8 array
     * ============================================================
     */
    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic [31:0]
        dbg_acc_ctx0 [0:7][0:7][0:15];

    logic [31:0]
        dbg_acc_ctx1 [0:7][0:7][0:15];


    systolic_array_8x8_fold u_array (
        .clk              (clk),
        .rst              (rst),

        .a_in             (a_in),
        .b_in             (b_in),

        .a_valid_in       (a_valid_in),
        .b_valid_in       (b_valid_in),

        .fold_ctx_in_a    (fold_ctx_in_a),
        .fold_ctx_in_b    (fold_ctx_in_b),

        .dbg_acc_ctx0     (dbg_acc_ctx0),
        .dbg_acc_ctx1     (dbg_acc_ctx1)
    );


    /*
     * ============================================================
     * Compute feeder
     *
     * Two folds:
     *
     *   fold 0: t = 0 .. 7
     *   fold 1: t = 8 .. 15
     *
     * With systolic skew, the boundary feeder runs through
     * t = 0 .. 22 so the final row/column injections occur.
     *
     * There is no bubble between fold 0 and fold 1 at PE[0][0].
     * ============================================================
     */

    typedef enum logic [2:0] {
        ST_IDLE,
        ST_FEED,
        ST_DRAIN,
        ST_SEND
    } state_t;

    state_t state;

    logic [5:0] feed_t;
    logic [5:0] drain_count;


    always_comb begin

        for (int i = 0; i < 8; i++) begin

            a_in[i] = 32'd0;
            b_in[i] = 32'd0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;

            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;

        end


        if (state == ST_FEED) begin

            /*
             * A-side skew.
             */
            for (int r = 0; r < 8; r++) begin

                integer gk;
                integer fold_id;
                integer k_idx;

                gk = feed_t - r;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    if (fold_id == 0)
                        a_in[r] = A0[r][k_idx];
                    else
                        a_in[r] = A1[r][k_idx];

                    a_valid_in[r] = 1'b1;
                    fold_ctx_in_a[r] = fold_id[0];

                end

            end


            /*
             * B-side skew.
             */
            for (int c = 0; c < 8; c++) begin

                integer gk;
                integer fold_id;
                integer k_idx;

                gk = feed_t - c;

                if ((gk >= 0) && (gk < 16)) begin

                    fold_id = gk >> 3;
                    k_idx   = gk & 7;

                    if (fold_id == 0)
                        b_in[c] = B0[k_idx][c];
                    else
                        b_in[c] = B1[k_idx][c];

                    b_valid_in[c] = 1'b1;
                    fold_ctx_in_b[c] = fold_id[0];

                end

            end

        end

    end


    /*
     * ============================================================
     * State progression
     * ============================================================
     */
    always_ff @(posedge clk) begin

        if (rst) begin

            state       <= ST_IDLE;
            feed_t      <= 0;
            drain_count <= 0;

        end
        else begin

            case (state)

                ST_IDLE: begin

                    if (matrices_ready) begin

                        feed_t <= 0;
                        state  <= ST_FEED;

                    end

                end


                ST_FEED: begin

                    /*
                     * Last useful boundary skew cycle:
                     *
                     * global_k max = 15
                     * max skew     = 7
                     *
                     * => 22
                     */
                    if (feed_t == 22) begin

                        drain_count <= 0;
                        state       <= ST_DRAIN;

                    end
                    else begin

                        feed_t <= feed_t + 1'b1;

                    end

                end


                ST_DRAIN: begin

                    /*
                     * Plenty of time for MUL(9) + ADD(12)
                     * writebacks to complete before reading banks.
                     */
                    if (drain_count == 40) begin

                        state <= ST_SEND;

                    end
                    else begin

                        drain_count <= drain_count + 1'b1;

                    end

                end


                ST_SEND: begin
                    /*
                     * Return to IDLE after the final UART byte
                     * has completely finished transmitting.
                     */
                    if (
                        tx_state == TX_WAIT_DONE
                        &&
                        !tx_busy
                        &&
                        tx_count == 8191
                    ) begin
                        state <= ST_IDLE;
                    end
                end

            endcase

        end

    end


    /*
     * ============================================================
     * UART TX
     *
     * Send raw accumulator banks:
     *
     * ctx0 first:
     *   8*8*16*4 = 4096 bytes
     *
     * ctx1 second:
     *   4096 bytes
     *
     * total:
     *   8192 bytes
     *
     * Host performs final 16-bank reduction.
     * ============================================================
     */

    logic [13:0] tx_count;

    logic [31:0] tx_word;

    logic [5:0] tx_pe;
    logic [3:0] tx_bank;

    logic tx_ctx;


    always_comb begin

        /*
        * tx_count layout:
        *
        * [12]   = context
        * [11:9] = PE row
        * [8:6]  = PE col
        * [5:2]  = accumulator bank
        * [1:0]  = byte within FP32 word
        */

        tx_ctx  = tx_count[12];
        tx_pe   = tx_count[11:6];
        tx_bank = tx_count[5:2];


        if (tx_ctx == 1'b0) begin

            tx_word =
                dbg_acc_ctx0
                    [tx_count[11:9]]
                    [tx_count[8:6]]
                    [tx_count[5:2]];

        end
        else begin

            tx_word =
                dbg_acc_ctx1
                    [tx_count[11:9]]
                    [tx_count[8:6]]
                    [tx_count[5:2]];

        end


        case (tx_count[1:0])

            2'd0:
                tx_byte = tx_word[7:0];

            2'd1:
                tx_byte = tx_word[15:8];

            2'd2:
                tx_byte = tx_word[23:16];

            2'd3:
                tx_byte = tx_word[31:24];

            default:
                tx_byte = 8'h00;

        endcase

    end


    typedef enum logic [1:0] {
        TX_IDLE,
        TX_START,
        TX_WAIT_BUSY,
        TX_WAIT_DONE
    } tx_state_t;

    tx_state_t tx_state;


    always_ff @(posedge clk) begin

        if (rst) begin

            tx_count <= 0;
            tx_start <= 1'b0;
            tx_state <= TX_IDLE;

        end
        else begin

            tx_start <= 1'b0;

            case (tx_state)

                TX_IDLE: begin

                    if (state == ST_SEND) begin

                        tx_count <= 0;
                        tx_state <= TX_START;

                    end

                end


                TX_START: begin

                    if (!tx_busy) begin

                        tx_start <= 1'b1;
                        tx_state <= TX_WAIT_BUSY;

                    end

                end


                TX_WAIT_BUSY: begin

                    if (tx_busy)
                        tx_state <= TX_WAIT_DONE;

                end


                TX_WAIT_DONE: begin

                    if (!tx_busy) begin

                        if (tx_count == 8191) begin

                            tx_count <= 0;
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
