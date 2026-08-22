module systolic_uart_tile_top #(
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
    /*
     * ============================================================
     * N -- 陣列邊長(N x N 個 PE)
     * ============================================================
     *
     * 幾何參數,與 K_MAX(容量)正交。隨 N 縮放的東西:
     *   operand buffer 的 bank 數、feeder 的 skew 上限(N-1)、
     *   wire format 的 lane 數(每個 k 有 N+N 個 word)、
     *   TX 總量(2 * N*N * 4 bytes)。
     * 不隨 N 變的東西:fold 深度(ctx 每 8 個 k 切換)是協定常數,
     * k_dim 仍須為 8 的倍數;cycle counter、breadcrumb、UART 協定。
     *
     * N 必須是 2 的冪(位元切片依賴對齊),且 N*N*4 顆 DSP 要放得下
     * (xc7a200t 有 740:N=8 用 256,N=4 用 64,N=16 的 1024 放不下)。
     */
    parameter int N = 8,

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
    parameter bit DEBUG_MARKERS = 1'b0,

    /*
     * ============================================================
     * CYCLE_COUNTER -- 附加 4 bytes 的硬體週期計數
     * ============================================================
     *
     * 計數區間刻意與 tb_array_fold_kmax 的 total_cycles 定義一致：
     * 第一個 feed beat 到 ctx1 published，兩端皆含。這樣「模擬預測
     * 118，實機量到 118」才是逐一對應而非概略吻合。
     *
     * 四個 byte 接在 512 result bytes 之後（小端序，與協定中 K 的
     * 慣例相同），不是插在前面 -- DEBUG_MARKERS 的教訓是前置位元組
     * 會讓只讀 512 的 host 每個 float 都偏移。
     *
     * 預設關閉，wire format 維持 RX K_MAX*64 -> TX 512。
     * 開啟時 host 必須改讀 516 bytes。
     * ============================================================
     */
    parameter bit CYCLE_COUNTER = 1'b0
) (
    input  logic clk,
    input  logic rst,

    input  logic uart_rx,
    output logic uart_tx,

    /*
     * 板上狀態顯示。由 systolic_status 驅動 -- 那個模組只讀訊號、
     * 不寫回設計的任何一條線,所以這兩組埠的加入對既有行為的影響
     * 在結構上為零。
     *
     *   led    板上 LD0..LD7
     *   jb_led 外接 Pmod JB pin 4 / pin 10(裝在看得見的地方)
     *
     * 配置與判讀見 systolic_status.sv 的檔頭。
     */
    output logic [7:0] led,
    output logic [1:0] jb_led
);

    /*
     * ============================================================
     * Derived geometry -- all of it from K_MAX
     * ============================================================
     */

    // Absolute-k index width: k runs 0 .. K_MAX-1.
    localparam int K_W = $clog2(K_MAX);

    // Lane index width, and bytes per matrix chunk (one k-window of
    // one matrix): 8-deep window * N lanes * 4 bytes = 32*N.
    localparam int LANE_W  = $clog2(N);
    localparam int CHUNK_W = 5 + LANE_W;             // $clog2(32*N)

    // One transaction is K_MAX/8 (A,B) chunk pairs, 32*N bytes per
    // chunk, so K_MAX * 8 * N bytes in total. At N=8, K_MAX=16: 1024.
    localparam int RX_BYTES = K_MAX * 8 * N;
    localparam int RX_CNT_W = $clog2(RX_BYTES);      // == K_W + 3 + LANE_W

    // Last feed beat is (k_dim - 1) + max skew(N-1). Sized for the
    // largest k_dim the hardware can be handed, i.e. K_MAX.
    localparam int FEED_LAST = K_MAX + N - 2;
    localparam int FEED_W    = $clog2(FEED_LAST + 1);

    // TX is K_MAX-invariant: two accumulator contexts, 4*N*N B each.
    // (由 systolic_tx_source 自行推導;此處僅供文件與總量計算。)
    localparam int TX_BYTES = 8 * N * N;

    // 開啟 CYCLE_COUNTER 時附加 4 bytes 的計數值。
    localparam int TX_TOTAL_BYTES = TX_BYTES + (CYCLE_COUNTER ? 4 : 0);
    localparam int TX_LAST        = TX_TOTAL_BYTES - 1;

`ifndef SYNTHESIS
    initial begin
        if (K_MAX < 16)
            $fatal(1, "K_MAX must be >= 16 (got %0d)", K_MAX);
        if ((K_MAX % 8) != 0)
            $fatal(1, "K_MAX must be a multiple of 8 (got %0d)", K_MAX);
        if (N < 2 || (N & (N - 1)) != 0)
            $fatal(1, "N must be a power of two >= 2 (got %0d)", N);
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
    /* 組態後自動 reset。
     *
     * 7-series 的 FF 在 configuration 當下就會載入宣告初始值,所以
     * por_sr 開機時是全 0,por_rst 會維持高電位 16 拍再放開 —— 等於
     * 燒錄完成後硬體自己打了一次 reset。
     *
     * 在此之前,k_dim 只在外部 rst 時載入 K_MAX(見下方 reset 區塊),
     * 因此不按 BTNC 就送請求會完全沒有回應,症狀與 bitstream 損壞
     * 一模一樣。BTNC 現在退回「想手動重置時才按」。 */
    logic [15:0] por_sr = '0;
    wire         por_rst = ~por_sr[15];
    always_ff @(posedge clk)
        por_sr <= {por_sr[14:0], 1'b1};

    wire rst_i = rst | por_rst;

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
    logic [4:0] debug_set;
    logic [4:0] debug_pending_next;

    /* debug_tx_active / debug_tx_byte 已隨 TX 重構移入
     * systolic_tx_source;debug_accept 由該模組驅動。 */
    logic [4:0] debug_accept;

    uart_rx #(
        .CLK_HZ (CLK_HZ),
        .BAUD   (BAUD)
    ) u_uart_rx (
        .clk      (clk),
        .rst      (rst_i),
        .rx       (uart_rx),
        .data_out (rx_byte),
        .valid    (rx_valid)
    );

    uart_tx #(
        .CLK_HZ (CLK_HZ),
        .BAUD   (BAUD)
    ) u_uart_tx (
        .clk     (clk),
        .rst     (rst_i),
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
    /*
     * Held as sixteen small memories rather than one 2-D register
     * array: eight for A indexed by row, eight for B indexed by
     * column. Both groups are now instances of
     * systolic_operand_buffer, further down -- after the RX control
     * signals they depend on exist.
     *
     * As a 2-D array these cannot be inferred as RAM at all. The
     * write address spans both dimensions, so there is no single
     * write address for the tool to recognise and it falls back to
     * flip-flops -- one write enable per 32-bit word. Control sets
     * then scale with K_MAX (256 at K_MAX=16, 1024 at K_MAX=64), and
     * since a slice holds exactly one control set the K_MAX=64 build
     * needed 31839 slices out of 30575 while LUTs sat at 80% and
     * flip-flops at 55%.
     *
     * Split per row and per column, each memory has one write address
     * and one read address. The feeder reads row r at gk = feed_t - r
     * and column c at gk = feed_t - c, so one read address per memory
     * per cycle is all it ever needs.
     *
     * The read is SYNCHRONOUS: block RAM has no asynchronous read
     * port, so the address issued on beat t returns data on t+1. The
     * feeder already delays valid and accumulator context by that one
     * cycle to match; it is the sole reason the cost is K+119 rather
     * than K+118, and it does not depend on k_dim.
     */
    logic [K_W-1:0] a_raddr [0:N-1];
    logic [K_W-1:0] b_raddr [0:N-1];

    wire [31:0] a_rdata [0:N-1];
    wire [31:0] b_rdata [0:N-1];

    logic [RX_CNT_W-1:0] rx_count;
    logic [1:0]          byte_pos;
    logic [31:0]         word_buf;

    /*
     * Chunk layouts inherited from the row-major matrices:
     *   A chunk = A[row][k]:  word = lane*8 + koff  -> {lane, koff}
     *   B chunk = B[k][col]:  word = koff*N + lane  -> {koff, lane}
     * 在 N=8 時 lane 與 koff 同寬,四個欄位退化成 rx_row/rx_col
     * 兩個 -- 舊版就是這樣寫的。N != 8 時它們必須分開命名。
     */
    wire [RX_CNT_W-CHUNK_W-1:0] rx_mat  = rx_count[RX_CNT_W-1:CHUNK_W];
    wire                        rx_is_b = rx_mat[0];
    wire [K_W-4:0]              rx_win  = rx_mat[RX_CNT_W-CHUNK_W-1:1];

    wire [LANE_W-1:0] a_lane = rx_count[CHUNK_W-1:5];         // N=8: [7:5]
    wire [2:0]        a_koff = rx_count[4:2];
    wire [2:0]        b_koff = rx_count[CHUNK_W-1:CHUNK_W-3]; // N=8: [7:5]
    wire [LANE_W-1:0] b_lane = rx_count[LANE_W+1:2];          // N=8: [4:2]

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


    /*
     * ============================================================
     * RX framing
     * ============================================================
     *
     *   FRAME_START(4) | HDR(4) | PAYLOAD(RX_BYTES) | FRAME_END(4)
     *
     * The transaction used to be a bare fixed-length burst. With no
     * delimiter there is no way to tell where one request ends and
     * the next begins, so a host that sent the wrong number of bytes
     * left the byte counters pointing into the middle of a frame and
     * every later request was split across two of them. That state
     * survived until the board was reset by hand.
     *
     * The obvious cheap alternative -- treat a gap between bytes as
     * a boundary -- is not sound. One transaction takes 89 ms on the
     * wire, and any host-side stall inside that window would split a
     * VALID request. That trades "wrong length fails" for "correct
     * length sometimes fails", which is worse: it makes correctness
     * depend on the host's scheduler.
     *
     * sync_sr is a sliding window over the last four bytes, ordered
     * to match the little-endian word convention used everywhere
     * else on this interface. Matching on a sliding window is what
     * makes HUNT self-synchronising: whatever offset the receiver is
     * stuck at, it walks forward one byte at a time until the marker
     * lines up. No rewind and no buffer are needed.
     *
     * A false START inside float payload is possible -- any byte
     * value can occur -- but the END check then fails and the frame
     * is discarded, so the receiver converges within one frame. The
     * odds are 2^-32 per offset, about 2.4e-7 per frame, which is
     * why byte stuffing (SLIP/COBS) is not worth its cost here.
     * ============================================================
     */
    localparam logic [31:0] FRAME_START = 32'hA55A_C33C;
    localparam logic [31:0] FRAME_END   = 32'h5AA5_3CC3;

    typedef enum logic [1:0] {
        RX_HUNT,
        RX_BODY,
        RX_TAIL
    } rx_state_t;

    rx_state_t rx_state;

    logic [31:0] sync_sr;
    logic [1:0]  tail_cnt;

    /*
     * Oldest of the four bytes ends up in bits [7:0], matching
     * rx_word above, so a marker constant reads the same way a
     * header word does.
     */
    wire [31:0] sync_next = {rx_byte, sync_sr[31:8]};


    /*
     * ============================================================
     * Operand memories
     * ============================================================
     *
     * One write port and one synchronous read port each, which is the
     * shape block RAM wants. Writes are decoded here rather than
     * inside the RX state machine so that each memory sees a single
     * write address.
     *
     * A and B are the same hardware; the ONLY difference is which RX
     * field selects the bank and which forms the address, and that
     * swap is exactly the A/B transpose:
     *
     *     A:  bank = a_lane,  addr = {rx_win, a_koff}
     *     B:  bank = b_lane,  addr = {rx_win, b_koff}
     *
     * Written as two instances of one module the transpose is visible
     * on the port map. Written as two generate loops it could only be
     * found by diffing them line by line.
     * ============================================================
     */
    wire buf_wr =
        rx_valid &&
        (rx_state == RX_BODY) &&
        (byte_pos == 2'd3) &&
        hdr_done;

    wire [31:0] buf_wdata = {rx_byte, word_buf[23:0]};

    // Absolute k of the word being written. Each k window is exactly
    // 8 deep, so the concatenation is window*8 + offset with no adder.
    wire [K_W-1:0] a_waddr = {rx_win, a_koff};
    wire [K_W-1:0] b_waddr = {rx_win, b_koff};

    /* K_W 必須明確傳下去。漏傳時模組會用自己的 $clog2(K_MAX) 預設,
     * 與這裡相同,所以即使漏傳也不會錯位 —— 但寫出來才看得見契約。 */
    systolic_operand_buffer #(
        .K_MAX   (K_MAX),
        .K_W     (K_W),
        .N_BANKS (N)
    ) u_a_buf (
        .clk   (clk),

        .wr    (buf_wr && !rx_is_b),
        .wsel  (a_lane),
        .waddr (a_waddr),
        .wdata (buf_wdata),

        .raddr (a_raddr),
        .rdata (a_rdata)
    );


    systolic_operand_buffer #(
        .K_MAX   (K_MAX),
        .K_W     (K_W),
        .N_BANKS (N)
    ) u_b_buf (
        .clk   (clk),

        .wr    (buf_wr && rx_is_b),
        .wsel  (b_lane),
        .waddr (b_waddr),
        .wdata (buf_wdata),

        .raddr (b_raddr),
        .rdata (b_rdata)
    );


    always_ff @(posedge clk) begin
        if (rst_i) begin

            rx_state       <= RX_HUNT;
            sync_sr        <= 32'd0;
            tail_cnt       <= 2'd0;

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

                /*
                 * The sliding window advances on every byte, in
                 * every state. HUNT needs it to find a marker at an
                 * arbitrary offset; TAIL needs it to read the four
                 * bytes it is checking.
                 */
                sync_sr <= sync_next;

                case (rx_state)

                /*
                 * ----------------------------------------------------
                 * Walk forward one byte at a time until the start
                 * marker lines up. Everything before it is discarded,
                 * which is what recovers from a truncated or oversized
                 * previous transfer.
                 * ----------------------------------------------------
                 */
                RX_HUNT: begin

                    if (sync_next == FRAME_START) begin

                        rx_state <= RX_BODY;

                        rx_count <= '0;
                        byte_pos <= 2'd0;
                        hdr_done <= 1'b0;

                    end

                end


                /*
                 * ----------------------------------------------------
                 * Header word followed by the operand payload. This is
                 * the original receive path, unchanged: the framing
                 * states around it decide whether its results are
                 * allowed to start a computation.
                 * ----------------------------------------------------
                 */
                RX_BODY: begin

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
                             * The first complete word of a frame is
                             * the header, not operand data.
                             */
                            if (!hdr_done) begin

                                k_dim <= hdr_valid ? FEED_W'(hdr_k)
                                                   : FEED_W'(K_MAX);

                            end

                            /*
                             * The operand write itself happens in the
                             * per-row and per-column memories above,
                             * gated by buf_wr. Keeping it out of this
                             * state machine is what gives each memory
                             * a single write address, without which
                             * the arrays cannot be inferred as RAM.
                             *
                             * Payload is written optimistically,
                             * before the end marker has been seen. A
                             * frame that turns out to be spurious
                             * leaves stale operands behind, but they
                             * are overwritten by the next accepted
                             * frame before anything reads them --
                             * which is why no 1036-byte holding
                             * buffer is needed.
                             */

                        end

                    endcase


                    if (byte_pos == 2'd3)
                        byte_pos <= 2'd0;
                    else
                        byte_pos <= byte_pos + 1'b1;


                    /*
                     * Header bytes do not advance the payload
                     * counter, so rx_count still addresses operand
                     * storage exactly as before: the write at
                     * byte_pos == 3 sees rx_count = 4w+3 for
                     * payload word w.
                     */
                    if (!hdr_done) begin

                        if (byte_pos == 2'd3)
                            hdr_done <= 1'b1;

                    end
                    else if (rx_count == RX_CNT_W'(RX_BYTES - 1)) begin

                        rx_count <= '0;
                        hdr_done <= 1'b0;
                        tail_cnt <= 2'd0;

                        /*
                         * The payload is complete but not yet
                         * trusted. matrices_ready is asserted in
                         * RX_TAIL and only if the end marker
                         * matches.
                         */
                        rx_state <= RX_TAIL;

                    end
                    else begin

                        rx_count <= rx_count + 1'b1;

                    end

                end


                /*
                 * ----------------------------------------------------
                 * Four bytes of end marker. A match is the only thing
                 * that starts a computation; a mismatch means the
                 * start marker was spurious or the frame was mangled,
                 * so the whole frame is dropped and the receiver goes
                 * back to hunting.
                 * ----------------------------------------------------
                 */
                RX_TAIL: begin

                    if (tail_cnt == 2'd3) begin

                        if (sync_next == FRAME_END)
                            matrices_ready <= 1'b1;

                        rx_state <= RX_HUNT;

                    end
                    else begin

                        tail_cnt <= tail_cnt + 1'b1;

                    end

                end


                default: begin

                    rx_state <= RX_HUNT;

                end

                endcase

            end

        end
    end



    /*
     * ============================================================
     * Fold-pipelined array interface
     * ============================================================
     */
    logic [31:0] a_in [0:N-1];
    logic [31:0] b_in [0:N-1];

    logic a_valid_in [0:N-1];
    logic b_valid_in [0:N-1];

    logic accum_ctx_in_a [0:N-1];
    logic accum_ctx_in_b [0:N-1];

    logic        c_valid_out;
    logic        c_ctx_out;
    logic [31:0] c_out [0:N-1][0:N-1];

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

        if (rst_i) begin

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



    /* 直接實例化參數化的 array_tile。舊的 systolic_array_8x8_tile
     * 薄包裝已不再使用(tb 中的階層路徑 u_array.u_arr.* 需改為
     * u_array.*)。 */
    systolic_array_tile #(
        .N      (N),
        .DATA_W (32)
    ) u_array (
        .clk           (clk),
        .rst           (rst_i),

        .a_in          (a_in),
        .b_in          (b_in),

        .a_valid_in    (a_valid_in),
        .b_valid_in    (b_valid_in),

        .accum_ctx_in_a (accum_ctx_in_a),
        .accum_ctx_in_b (accum_ctx_in_b),

        .c_valid_out   (c_valid_out),
        .c_ctx_out     (c_ctx_out),
        .c_out         (c_out)
    );


    /*
     * ============================================================
     * Store final results
     * ============================================================
     */
    logic [31:0] C0 [0:N-1][0:N-1];
    logic [31:0] C1 [0:N-1][0:N-1];

    logic c0_done;
    logic c1_done;

    integer rr;
    integer cc;


    always_ff @(posedge clk) begin
        if (rst_i) begin

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

                    for (rr = 0; rr < N; rr = rr + 1)
                        for (cc = 0; cc < N; cc = cc + 1)
                            C0[rr][cc] <= c_out[rr][cc];

                    c0_done <= 1'b1;

                end
                else begin

                    for (rr = 0; rr < N; rr = rr + 1)
                        for (cc = 0; cc < N; cc = cc + 1)
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
    *   (k_dim - 1) + max skew(N-1)
    *   = k_dim + N - 2
    */
    systolic_tile_feeder #(
        .N      (N),
        .K_W    (K_W),
        .FEED_W ($bits(feed_t)),
        .KDIM_W ($bits(k_dim))
    ) u_feeder (
        .clk            (clk),
        .rst            (rst_i),
        .enable         (state == ST_FEED),

        .feed_t         (feed_t),
        .k_dim          (k_dim),

        .a_rdata        (a_rdata),
        .b_rdata        (b_rdata),

        .a_raddr        (a_raddr),
        .b_raddr        (b_raddr),

        .a_in           (a_in),
        .b_in           (b_in),

        .a_valid_in     (a_valid_in),
        .b_valid_in     (b_valid_in),
        .accum_ctx_in_a (accum_ctx_in_a),
        .accum_ctx_in_b (accum_ctx_in_b)
    );


    /*
     * ============================================================
     * Transaction cycle counter
     *
     * 起點：ST_FEED 的第一拍（feed_t == 0）
     * 終點：ctx1 published（c_valid_out && c_ctx_out）
     *
     * 兩端皆含，等同 tb 的 ctx1_cycle - first_valid_cycle + 1。
     * cyc_latched 在終點鎖存，TX 期間不再變動。
     * ============================================================
     */
    logic [31:0] cyc_count;
    logic [31:0] cyc_latched;
    logic        cyc_running;

    always_ff @(posedge clk) begin
        if (rst_i) begin

            cyc_count   <= '0;
            cyc_latched <= '0;
            cyc_running <= 1'b0;

        end
        else begin

            if (state == ST_FEED && feed_t == '0 && !cyc_running) begin

                cyc_running <= 1'b1;
                cyc_count   <= 32'd1;

            end
            else if (cyc_running) begin

                cyc_count <= cyc_count + 1'b1;

                if (c_valid_out && c_ctx_out) begin

                    cyc_running <= 1'b0;
                    cyc_latched <= cyc_count + 1'b1;

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
        if (rst_i) begin

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

                    if (feed_t == k_dim + FEED_W'(N - 2)) begin

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

    /*
     * Collect all debug events combinationally, then update
     * debug_pending with exactly one sequential assignment.
     *
     * This avoids overlapping nonblocking assignments to both the
     * whole debug_pending vector and individual bit-selects.
     */
    always_comb begin

        debug_set = 5'b0;

        if (matrices_ready)
            debug_set[0] = 1'b1;

        if (
            state == ST_WAIT_RESULT &&
            !state_was_wait_result
        )
            debug_set[1] = 1'b1;

        if (c_valid_out) begin

            if (c_ctx_out == 1'b0)
                debug_set[2] = 1'b1;
            else
                debug_set[3] = 1'b1;

        end

        if (
            state == ST_SEND &&
            !state_was_send
        )
            debug_set[4] = 1'b1;


        /*
         * Accepted UART breadcrumbs are cleared while newly
         * observed events are ORed in during the same cycle.
         */
        debug_pending_next =
            (debug_pending & ~debug_accept) | debug_set;

    end


    always_ff @(posedge clk) begin

        if (rst_i) begin

            debug_pending         <= 5'b0;
            state_was_wait_result <= 1'b0;
            state_was_send        <= 1'b0;

        end
        else begin

            debug_pending <= debug_pending_next;

            state_was_wait_result <=
                (state == ST_WAIT_RESULT);

            state_was_send <=
                (state == ST_SEND);

        end

    end


    /*
     * ============================================================
     * UART TX -- streaming 化(feat/tx-streaming)
     *
     * 舊版在這裡有一座四狀態 FSM,把三件事縫在一起:與 uart_tx 的
     * start/busy 握手、C0/C1/cycle-counter 的位址走訪、breadcrumb
     * 的優先權仲裁。現在沿縫切開:
     *
     *   systolic_tx_source   內容:位址走訪 + marker 仲裁 + rearm
     *   uart_tx_streamer     協定:valid/ready -> start/busy
     *
     * byte 序列與舊 FSM 完全相同(tb_tx_equiv 以逐字複製的舊邏輯
     * 為 golden,逐 byte 比對證明);wire format 不變:
     *
     *   bytes   0..255 = C0
     *   bytes 256..511 = C1
     *   (+4 bytes cycle counter,見 CYCLE_COUNTER)
     *
     * breadcrumb 的「捕捉」(debug_pending,上方)留在 top,因為它
     * 觀察的是 top 層事件;「送出」(仲裁 + 序列化)在 source 裡,
     * debug_accept 由 source 回灌給上方的 pending 更新式。
     *
     * 這個邊界是為 double buffer 預留的:屆時只改 source(哪個
     * context 好了就先排水哪個),streamer 與 uart_tx 不動。
     * ============================================================
     */
    logic       tx_m_valid;
    logic [7:0] tx_m_data;
    logic       tx_m_ready;

    systolic_tx_source #(
        .DEBUG_MARKERS (DEBUG_MARKERS),
        .CYCLE_COUNTER (CYCLE_COUNTER),
        .N             (N)
    ) u_tx_source (
        .clk           (clk),
        .rst           (rst_i),

        .send_go       (state == ST_SEND),
        .all_done      (tx_all_done),

        .debug_pending (debug_pending),
        .debug_accept  (debug_accept),

        .C0            (C0),
        .C1            (C1),
        .cyc_latched   (cyc_latched),

        .m_valid       (tx_m_valid),
        .m_data        (tx_m_data),
        .m_ready       (tx_m_ready)
    );

    uart_tx_streamer u_tx_streamer (
        .clk      (clk),
        .rst      (rst_i),

        .s_valid  (tx_m_valid),
        .s_data   (tx_m_data),
        .s_ready  (tx_m_ready),

        .tx_start (tx_start),
        .tx_data  (tx_byte),
        .tx_busy  (tx_busy)
    );


    /*
     * ============================================================
     * 板上狀態顯示
     * ============================================================
     *
     * 純觀測:所有連接都是這個模組的 input,它不驅動設計裡的任何
     * 訊號。這是刻意的紀律 -- 觀測工具不該有能力改變被觀測的對象。
     *
     * 與 DEBUG_MARKERS 的分工:
     *   DEBUG_MARKERS  一次性、經 UART、會改 wire format 與 placement
     *   LED            連續、獨立通道、協定與時序都不受影響
     * 兩者可以並存;日常用 LED,需要精確的事件順序時才開 markers。
     * ============================================================
     */
    systolic_status u_status (
        .clk            (clk),
        .rst            (rst_i),

        .frame_accepted (matrices_ready),
        .rx_active      (rx_state != RX_HUNT),
        .state          (state),
        .c0_done        (c0_done),
        .c1_done        (c1_done),

        .led            (led),
        .jb_led         (jb_led)
    );

endmodule