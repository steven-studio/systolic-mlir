module systolic_uart_fold_top #(
    parameter int CLK_HZ = 100_000_000,
    parameter int BAUD   = 115200,

    /*
     * ============================================================
     * K_MAX -- synthesis-time hardware capacity
     * ============================================================
     *
     * The deepest reduction the on-chip operand buffers can hold.
     * This is the ONLY hardware design-space knob here.
     *
     *   K_MAX   synthesis-time hardware capacity   <- this parameter
     *   k_dim   runtime workload reduction length  <- from software
     *   folds   k_dim / 8, derived at runtime      <- NOT hardware
     *
     * A fold COUNT is deliberately not a parameter. Folds are a
     * scheduling quantity software derives from k_dim; baking one
     * into the RTL was the mistake this replaced. Note that the
     * operand buffers below are indexed by ABSOLUTE k, so the fold
     * decomposition does not appear in storage at all -- it
     * survives only as the accumulator-context bit, which is
     * derived per beat at run time.
     *
     * K_MAX must be a multiple of 8 and at least 16.
     *
     * Everything scaling with K_MAX lives in this module: the 64
     * PEs, both FP IP cores and the entire reduction path are
     * K_MAX-invariant.
     * ============================================================
     */
    parameter int K_MAX = 16,

    /*
     * ============================================================
     * DEBUG_MARKERS -- emit the 0xA1..0xA5 breadcrumb bytes
     * ============================================================
     *
     * The breadcrumb markers (see below) were unconditional, and
     * they take priority over the result stream in TX_IDLE. That
     * puts up to five extra bytes ahead of the 512 result bytes,
     * so a host reading exactly 512 bytes -- which is what
     * test_uart_fold8x8.py does -- receives
     *
     *     5 marker bytes + the first 507 result bytes
     *
     * and every float it decodes is shifted by five bytes.
     *
     * The markers are a bring-up aid, not part of the protocol, so
     * they now default OFF and the wire format is exactly
     *
     *     RX 1024 bytes  ->  TX 512 bytes
     *
     * Set DEBUG_MARKERS = 1 to get them back for debugging, and
     * remember that a host must then consume them explicitly.
     *
     * This parameter does not affect the datapath: no accumulator,
     * feed, fold-context or reduction behaviour depends on it.
     * ============================================================
     */
    parameter bit DEBUG_MARKERS = 1'b0
) (
    input  logic clk,
    input  logic rst,

    input  logic uart_rx,
    output logic uart_tx
);

    /*
     * ============================================================
     * Derived geometry -- all of it from K_MAX
     * ============================================================
     */

    // Absolute-k index width: k runs 0 .. K_MAX-1.
    localparam int K_W = $clog2(K_MAX);

    // One transaction is K_MAX/8 (A,B) pairs, 256 bytes per matrix,
    // so K_MAX * 64 bytes in total. At K_MAX=16 that is 1024.
    localparam int RX_BYTES = K_MAX * 64;
    localparam int RX_CNT_W = $clog2(RX_BYTES);      // == K_W + 6

    // Last feed beat is (k_dim - 1) + max skew(7). Sized for the
    // largest k_dim the hardware can be handed, i.e. K_MAX.
    localparam int FEED_LAST = K_MAX + 6;
    localparam int FEED_W    = $clog2(FEED_LAST + 1);

    // TX is K_MAX-invariant: two accumulator contexts, 256 B each.
    localparam int TX_BYTES = 512;

`ifndef SYNTHESIS
    initial begin
        if (K_MAX < 16)
            $fatal(1, "K_MAX must be >= 16 (got %0d)", K_MAX);
        if ((K_MAX % 8) != 0)
            $fatal(1, "K_MAX must be a multiple of 8 (got %0d)", K_MAX);
    end
`endif

    /*
     * ============================================================
     * k_dim -- runtime valid reduction length of this invocation
     * ============================================================
     *
     * The feed-length register. K_MAX is what the buffers can hold;
     * k_dim is how much of that this particular invocation actually
     * reduces. The host writes it once per transaction through the
     * request header below, so a workload K that is not a multiple
     * of K_MAX can issue its remainder invocation at its true depth
     * rather than padding up to capacity.
     *
     *   K_MAX = 64,  K = 100  ->  k_dim = 64, then k_dim = 36
     *
     * Reset value is K_MAX so a design that is reset and then driven
     * by a host which never updates the header still behaves as the
     * fixed-capacity baseline did.
     */
    logic [FEED_W-1:0] k_dim;

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
     * Matrices arrive interleaved, 256 bytes each, one (A,B) pair
     * per 8-deep k window:
     *
     *   A[k 0..7] B[k 0..7] A[k 8..15] B[k 8..15] ...
     *
     * At K_MAX = 16 this is byte-for-byte the original layout:
     *
     *   bytes    0..255  = A0
     *   bytes  256..511  = B0
     *   bytes  512..767  = A1
     *   bytes  768..1023 = B1
     *
     * STORAGE IS INDEXED BY ABSOLUTE k, not by fold. A_buf is
     * [row][k] and B_buf is [k][col], both k = 0..K_MAX-1, so the
     * 8-deep windowing exists only in the wire format and the fold
     * decomposition never appears in the buffers.
     *
     * The byte counter decomposes with no arithmetic:
     *
     *   rx_count[RX_CNT_W-1:8] = matrix index
     *      matrix[0]           = 0 -> A, 1 -> B
     *      matrix >> 1         = k window
     *   rx_count[7:5]          = row
     *   rx_count[4:2]          = col
     *   rx_count[1:0]          = byte within word
     *
     * and absolute k is {window, col} for A, {window, row} for B,
     * since each window is exactly 8 deep.
     * ============================================================
     */
    logic [31:0] A_buf [0:7][0:K_MAX-1];
    logic [31:0] B_buf [0:K_MAX-1][0:7];

    logic [RX_CNT_W-1:0] rx_count;
    logic [1:0]          byte_pos;
    logic [31:0]         word_buf;

    wire [RX_CNT_W-9:0] rx_mat  = rx_count[RX_CNT_W-1:8];
    wire                rx_is_b = rx_mat[0];
    wire [K_W-4:0]      rx_win  = rx_mat[RX_CNT_W-9:1];
    wire [2:0]          rx_row  = rx_count[7:5];
    wire [2:0]          rx_col  = rx_count[4:2];

    /*
     * ============================================================
     * Request header
     * ============================================================
     *
     * Every transaction is now
     *
     *   [ k_dim : 4 bytes, little-endian ] [ A/B payload ]
     *
     * so one request is HDR_BYTES + RX_BYTES bytes total.
     *
     * The header is a full 32-bit word on purpose: the byte
     * assembler below already builds words out of four bytes, so a
     * word-sized header needs no separate path and leaves the
     * payload's word alignment untouched.
     *
     * The payload length does NOT shrink with k_dim. Operand
     * storage and the RX framing are still sized by K_MAX, and
     * positions at k >= k_dim are simply never read by the feeder.
     * Making the transfer itself shorter is a separate change to
     * the framing, deliberately not bundled here.
     */
    localparam int HDR_BYTES = 4;

    logic hdr_done;

    // The word currently being completed, LSB-first on the wire.
    wire [31:0] rx_word = {rx_byte, word_buf[23:0]};

    /*
     * Out-of-range requests are clamped to K_MAX rather than
     * honoured or flagged. There is no status channel to report an
     * error on, and the two failure modes this prevents are worse
     * than a clamp: k_dim = 0 would terminate the feed loop before
     * injecting anything and hang the design waiting for results
     * that cannot arrive, and k_dim > K_MAX would read operand
     * positions the host never wrote.
     */
    wire [31:0] hdr_k     = rx_word;
    wire        hdr_valid = (hdr_k != 32'd0) && (hdr_k <= 32'(K_MAX));

    logic matrices_ready;


    always_ff @(posedge clk) begin
        if (rst) begin

            rx_count       <= '0;
            byte_pos       <= 2'd0;
            word_buf       <= 32'd0;
            matrices_ready <= 1'b0;
            hdr_done       <= 1'b0;
            k_dim          <= FEED_W'(K_MAX);

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
                         * The first complete word of a transaction
                         * is the header, not operand data.
                         */
                        if (!hdr_done) begin

                            k_dim <= hdr_valid ? FEED_W'(hdr_k)
                                               : FEED_W'(K_MAX);

                        end

                        /*
                         * Absolute k is {window, col} for A and
                         * {window, row} for B -- each k window is
                         * exactly 8 deep, so the concatenation is
                         * window*8 + offset with no adder.
                         */
                        else if (rx_is_b) begin

                            B_buf[{rx_win, rx_row}]
                                 [rx_col]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end
                        else begin

                            A_buf[rx_row]
                                 [{rx_win, rx_col}]
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


                /*
                 * Header bytes do not advance the payload counter,
                 * so rx_count still addresses operand storage
                 * exactly as before: the write at byte_pos == 3
                 * sees rx_count = 4w+3 for payload word w.
                 *
                 * hdr_done is cleared at end of transaction, which
                 * rearms the header for the next request -- every
                 * invocation therefore carries its own k_dim.
                 */
                if (!hdr_done) begin

                    if (byte_pos == 2'd3)
                        hdr_done <= 1'b1;

                end
                else if (rx_count == RX_CNT_W'(RX_BYTES - 1)) begin

                    rx_count       <= '0;
                    hdr_done       <= 1'b0;
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

    logic [FEED_W-1:0] feed_t;
    logic              tx_all_done;


    /*
    * Feed variable K dimension continuously.
    *
    * Fold context alternates every 8 K elements:
    *
    *   fold0 -> ctx0
    *   fold1 -> ctx1
    *   fold2 -> ctx0
    *   fold3 -> ctx1
    *   ...
    *
    * fold = global_k >> 3
    * ctx  = fold[0]
    *
    * Last boundary injection:
    *
    *   (k_dim - 1) + max skew(7)
    *   = k_dim + 6
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

                gk_a = int'(feed_t) - r;

                if ((gk_a >= 0) && (gk_a < int'(k_dim))) begin

                    /*
                     * The fold number is derived here, at run time,
                     * purely to pick the accumulator context. The
                     * buffer is addressed by absolute k.
                     */
                    fold_a = gk_a >> 3;

                    a_in[r] =
                        A_buf[r][gk_a];

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

                gk_b = int'(feed_t) - c;

                if ((gk_b >= 0) && (gk_b < int'(k_dim))) begin

                    fold_b = gk_b >> 3;

                    b_in[c] =
                        B_buf[gk_b][c];

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
            feed_t <= '0;

        end
        else begin

            case (state)

                ST_IDLE: begin

                    feed_t <= '0;

                    if (matrices_ready) begin
                        state  <= ST_FEED;
                        feed_t <= '0;
                    end

                end


                ST_FEED: begin

                    if (feed_t == k_dim + 6) begin

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
                     *
                     * Gated by DEBUG_MARKERS. When it is 0 these
                     * five branches are constant-false, nothing
                     * ever sets debug_tx_active, and the sticky
                     * debug_pending bits become unread and are
                     * trimmed -- so the TX stream is exactly the
                     * 512 result bytes and nothing else.
                     */
                    if (DEBUG_MARKERS && debug_pending[0]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA1;
                        debug_accept[0] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (DEBUG_MARKERS && debug_pending[1]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA2;
                        debug_accept[1] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (DEBUG_MARKERS && debug_pending[2]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA3;
                        debug_accept[2] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (DEBUG_MARKERS && debug_pending[3]) begin

                        debug_tx_active <= 1'b1;
                        debug_tx_byte   <= 8'hA4;
                        debug_accept[3] <= 1'b1;
                        tx_state        <= TX_START;

                    end
                    else if (DEBUG_MARKERS && debug_pending[4]) begin

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