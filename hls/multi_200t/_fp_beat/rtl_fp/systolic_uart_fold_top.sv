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

    /*
     * DEBUG:
     * Once all 1024 input bytes have been received,
     * send one 0xA1 marker through UART.
     */
    /*
     * Hardware breadcrumb UART markers:
     *
     *   A1 = matrices_ready
     *   A2 = entered ST_WAIT_RESULT
     *   A3 = c_valid_out ctx0
     *   A4 = c_valid_out ctx1
     *   A5 = entered ST_SEND
     */
    logic [4:0] debug_pending;

    logic       debug_tx_active;
    logic [7:0] debug_tx_byte;
    logic [4:0] debug_accept;

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
     * Input matrices
     *
     * bytes    0..255  = A0
     * bytes  256..511  = B0
     * bytes  512..767  = A1
     * bytes  768..1023 = B1
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


    always_ff @(posedge clk) begin
        if (rst) begin

            rx_count       <= 11'd0;
            byte_pos       <= 2'd0;
            word_buf       <= 32'd0;
            matrices_ready <= 1'b0;

        end
        else begin

            matrices_ready <= 1'b0;

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

                            A0[rx_count[7:5]]
                              [rx_count[4:2]]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                        /*
                         * B0
                         */
                        else if (rx_count < 512) begin

                            B0[(rx_count - 256) >> 5]
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

                            A1[(rx_count - 512) >> 5]
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

                            B1[(rx_count - 768) >> 5]
                              [((rx_count - 768) >> 2) & 7]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                    end

                endcase


                if (byte_pos == 2'd3)
                    byte_pos <= 2'd0;
                else
                    byte_pos <= byte_pos + 1'b1;


                if (rx_count == 11'd1023) begin

                    rx_count       <= 11'd0;
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
     * Fold-pipelined array interface
     * ============================================================
     */
    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic fold_ctx_in_a [0:7];
    logic fold_ctx_in_b [0:7];

    logic        c_valid_out;
    logic        c_ctx_out;
    logic [31:0] c_out [0:7][0:7];

    /*
     * ============================================================
     * Accelerator latency measurement
     *
     * Count from matrices_ready until ctx1 result is valid.
     * ============================================================
     */
    logic [31:0] accel_cycles;
    logic [31:0] last_accel_cycles;
    logic        accel_counting;


    always_ff @(posedge clk) begin

        if (rst) begin

            accel_cycles      <= 32'd0;
            last_accel_cycles <= 32'd0;
            accel_counting    <= 1'b0;

        end
        else begin

            /*
             * Complete 1024-byte request has arrived.
             */
            if (matrices_ready) begin

                accel_cycles   <= 32'd0;
                accel_counting <= 1'b1;

            end
            else if (accel_counting) begin

                accel_cycles <= accel_cycles + 1'b1;

            end

            /*
             * ctx1 is the final result context.
             */
            if (
                accel_counting &&
                c_valid_out &&
                c_ctx_out == 1'b1
            ) begin

                last_accel_cycles <= accel_cycles + 1'b1;
                accel_counting    <= 1'b0;

            end

        end

    end



    systolic_array_8x8_fold u_array (
        .clk           (clk),
        .rst           (rst),

        .a_in          (a_in),
        .b_in          (b_in),

        .a_valid_in    (a_valid_in),
        .b_valid_in    (b_valid_in),

        .fold_ctx_in_a (fold_ctx_in_a),
        .fold_ctx_in_b (fold_ctx_in_b),

        .c_valid_out   (c_valid_out),
        .c_ctx_out     (c_ctx_out),
        .c_out         (c_out)
    );


    /*
     * ============================================================
     * Store final results
     * ============================================================
     */
    logic [31:0] C0 [0:7][0:7];
    logic [31:0] C1 [0:7][0:7];

    logic c0_done;
    logic c1_done;

    integer rr;
    integer cc;


    always_ff @(posedge clk) begin
        if (rst) begin

            c0_done <= 1'b0;
            c1_done <= 1'b0;

        end
        else begin

            /*
             * New transaction starts.
             */
            if (matrices_ready) begin
                c0_done <= 1'b0;
                c1_done <= 1'b0;
            end


            /*
             * Array itself tells us when a reduced C matrix
             * is ready.
             */
            if (c_valid_out) begin

                if (c_ctx_out == 1'b0) begin

                    for (rr = 0; rr < 8; rr = rr + 1)
                        for (cc = 0; cc < 8; cc = cc + 1)
                            C0[rr][cc] <= c_out[rr][cc];

                    c0_done <= 1'b1;

                end
                else begin

                    for (rr = 0; rr < 8; rr = rr + 1)
                        for (cc = 0; cc < 8; cc = cc + 1)
                            C1[rr][cc] <= c_out[rr][cc];

                    c1_done <= 1'b1;

                end

            end

        end
    end


    /*
     * ============================================================
     * Main state machine
     * ============================================================
     */
    typedef enum logic [2:0] {
        ST_IDLE,
        ST_FEED,
        ST_WAIT_RESULT,
        ST_SEND
    } state_t;

    state_t state;

    logic [5:0] feed_t;
    logic       tx_all_done;


    /*
     * ============================================================
     * Feed two K=8 folds continuously
     *
     * global k:
     *
     *   0..7   -> ctx0
     *   8..15  -> ctx1
     *
     * Last boundary injection:
     *
     *   15 + max skew(7) = 22
     * ============================================================
     */
    always_comb begin

        for (int i = 0; i < 8; i++) begin

            a_in[i]          = 32'd0;
            b_in[i]          = 32'd0;

            a_valid_in[i]    = 1'b0;
            b_valid_in[i]    = 1'b0;

            fold_ctx_in_a[i] = 1'b0;
            fold_ctx_in_b[i] = 1'b0;

        end


        if (state == ST_FEED) begin

            /*
             * A-side skew
             */
            for (int r = 0; r < 8; r++) begin

                integer gk_a;
                integer fold_a;
                integer k_a;

                gk_a = feed_t - r;

                if ((gk_a >= 0) && (gk_a < 16)) begin

                    fold_a = gk_a >> 3;
                    k_a    = gk_a & 7;

                    if (fold_a == 0)
                        a_in[r] = A0[r][k_a];
                    else
                        a_in[r] = A1[r][k_a];

                    a_valid_in[r]    = 1'b1;
                    fold_ctx_in_a[r] = fold_a[0];

                end

            end


            /*
             * B-side skew
             */
            for (int c = 0; c < 8; c++) begin

                integer gk_b;
                integer fold_b;
                integer k_b;

                gk_b = feed_t - c;

                if ((gk_b >= 0) && (gk_b < 16)) begin

                    fold_b = gk_b >> 3;
                    k_b    = gk_b & 7;

                    if (fold_b == 0)
                        b_in[c] = B0[k_b][c];
                    else
                        b_in[c] = B1[k_b][c];

                    b_valid_in[c]    = 1'b1;
                    fold_ctx_in_b[c] = fold_b[0];

                end

            end

        end

    end


    /*
     * ============================================================
     * Main state progression
     * ============================================================
     */
    always_ff @(posedge clk) begin
        if (rst) begin

            state  <= ST_IDLE;
            feed_t <= 6'd0;

        end
        else begin

            case (state)

                ST_IDLE: begin

                    feed_t <= 6'd0;

                    if (matrices_ready) begin
                        state  <= ST_FEED;
                        feed_t <= 6'd0;
                    end

                end


                ST_FEED: begin

                    if (feed_t == 6'd22) begin

                        state <= ST_WAIT_RESULT;

                    end
                    else begin

                        feed_t <= feed_t + 1'b1;

                    end

                end


                ST_WAIT_RESULT: begin

                    /*
                     * No hard-coded drain cycle count.
                     *
                     * Wait for the accelerator itself to report
                     * that both reduced result contexts exist.
                     */
                    if (c0_done && c1_done) begin
                        state <= ST_SEND;
                    end

                end


                ST_SEND: begin

                    if (tx_all_done) begin
                        state <= ST_IDLE;
                    end

                end


                default: begin
                    state <= ST_IDLE;
                end

            endcase

        end
    end



    /*
     * ============================================================
     * Hardware breadcrumb event capture
     * ============================================================
     *
     * Sticky pending bits ensure short one-cycle events survive
     * until UART becomes available.
     *
     * debug_pending[0] -> A1 matrices_ready
     * debug_pending[1] -> A2 ST_WAIT_RESULT entry
     * debug_pending[2] -> A3 ctx0 result
     * debug_pending[3] -> A4 ctx1 result
     * debug_pending[4] -> A5 ST_SEND entry
     * ============================================================
     */

    logic state_was_wait_result;
    logic state_was_send;

    always_ff @(posedge clk) begin

        if (rst) begin

            debug_pending         <= 5'b0;
            state_was_wait_result <= 1'b0;
            state_was_send        <= 1'b0;

        end
        else begin

            /*
             * Clear markers explicitly accepted by the UART FSM.
             */
            debug_pending <=
                debug_pending & ~debug_accept;


            /*
             * A1: all 1024 input bytes were received.
             */
            if (matrices_ready)
                debug_pending[0] <= 1'b1;


            /*
             * A2: first cycle in ST_WAIT_RESULT.
             */
            if (
                state == ST_WAIT_RESULT &&
                !state_was_wait_result
            )
                debug_pending[1] <= 1'b1;


            /*
             * A3 / A4: array publishes the two result contexts.
             */
            if (c_valid_out) begin

                if (c_ctx_out == 1'b0)
                    debug_pending[2] <= 1'b1;
                else
                    debug_pending[3] <= 1'b1;

            end


            /*
             * A5: first cycle in ST_SEND.
             */
            if (
                state == ST_SEND &&
                !state_was_send
            )
                debug_pending[4] <= 1'b1;


            state_was_wait_result <=
                (state == ST_WAIT_RESULT);

            state_was_send <=
                (state == ST_SEND);

        end

    end


    /*
     * ============================================================
     * UART TX
     *
     * bytes   0..255 = C0
     * bytes 256..511 = C1
     *
     * Total = 512 bytes
     * ============================================================
     */
    logic [8:0]  tx_count;
    logic [31:0] tx_word;
    logic tx_send_started;

    always_comb begin

        /*
         * tx_count layout inside each matrix:
         *
         * [7:5] = row
         * [4:2] = col
         * [1:0] = byte
         */

        if (tx_count < 256) begin

            tx_word =
                C0[tx_count[7:5]]
                  [tx_count[4:2]];

        end
        else begin

            tx_word =
                C1[(tx_count - 256) >> 5]
                  [((tx_count - 256) >> 2) & 7];

        end


        if (debug_tx_active) begin

            tx_byte = debug_tx_byte;

        end
        else begin

            case (tx_count[1:0])

                2'd0:
                    tx_byte = tx_word[7:0];

                2'd1:
                    tx_byte = tx_word[15:8];

                2'd2:
                    tx_byte = tx_word[23:16];

                default:
                    tx_byte = tx_word[31:24];

            endcase

        end

    end


    /*
     * ============================================================
     * UART TX FSM
     * ============================================================
     */
    typedef enum logic [1:0] {
        TX_IDLE,
        TX_START,
        TX_WAIT_BUSY,
        TX_WAIT_DONE
    } tx_state_t;

    tx_state_t tx_state;


    always_ff @(posedge clk) begin
        if (rst) begin

            tx_count         <= 9'd0;
            tx_start         <= 1'b0;
            tx_state         <= TX_IDLE;
            tx_all_done      <= 1'b0;
            debug_tx_active <= 1'b0;
            debug_tx_byte   <= 8'h00;
            debug_accept    <= 5'b0;
            tx_send_started <= 1'b0;

        end
        else begin

            tx_start    <= 1'b0;
            tx_all_done <= 1'b0;
            debug_accept <= 5'b0;

            case (tx_state)

                TX_IDLE: begin

                    /*
                     * Breadcrumb markers have priority over the
                     * normal 512-byte result stream.
                     *
                     * Lowest-number marker is sent first.
                     */
                    if (debug_pending[0]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA1;
                        debug_accept[0] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (debug_pending[1]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA2;
                        debug_accept[1] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (debug_pending[2]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA3;
                        debug_accept[2] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (debug_pending[3]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA4;
                        debug_accept[3] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (debug_pending[4]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA5;
                        debug_accept[4] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (
                        state == ST_SEND &&
                        !tx_send_started
                    ) begin

                        debug_tx_active <= 1'b0;
                        tx_send_started <= 1'b1;

                        tx_count <= 9'd0;
                        tx_state <= TX_START;

                    end


                    /*
                     * Rearm for the next transaction only after
                     * the main FSM has actually left ST_SEND.
                     */
                    if (state != ST_SEND)
                        tx_send_started <= 1'b0;

                end


                TX_START: begin

                    if (!tx_busy) begin

                        tx_start <= 1'b1;
                        tx_state <= TX_WAIT_BUSY;

                    end

                end


                TX_WAIT_BUSY: begin

                    if (tx_busy) begin
                        tx_state <= TX_WAIT_DONE;
                    end

                end


                TX_WAIT_DONE: begin

                    if (!tx_busy) begin

                        /*
                         * Debug transaction is exactly one byte.
                         */
                        if (debug_tx_active) begin

                            /*
                             * Debug transaction is exactly one byte.
                             */
                            debug_tx_active <= 1'b0;
                            tx_state        <= TX_IDLE;

                        end
                        else if (tx_count == 9'd511) begin

                            tx_count    <= 9'd0;
                            tx_state    <= TX_IDLE;
                            tx_all_done <= 1'b1;

                        end
                        else begin

                            tx_count <= tx_count + 1'b1;
                            tx_state <= TX_START;

                        end

                    end

                end


                default: begin
                    tx_state <= TX_IDLE;
                end

            endcase

        end
    end

endmodule