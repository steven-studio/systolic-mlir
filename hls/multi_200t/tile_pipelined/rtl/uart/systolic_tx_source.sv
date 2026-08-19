/*
 * systolic_tx_source -- 一筆交易的 byte 流產生器。
 *
 * 擁有舊 top TX 區塊的全部「內容」邏輯:結果位址走訪(C0 -> C1 ->
 * cycle counter)、byte lane 選擇、breadcrumb marker 的優先權仲裁、
 * tx_send_started 的 rearm 語意。「協定」(怎麼把 byte 塞進 UART)
 * 完全不在這裡 -- 交給 uart_tx_streamer。
 *
 * 行為契約 = 與舊 FSM 的 byte 序列完全相同(tb_tx_equiv 驗證):
 *
 *   - marker 只在 stream 未進行時插隊。舊 FSM 的 512-byte burst
 *     中途從不回 TX_IDLE,marker 只能等整包送完 -- 這裡同樣只在
 *     SRC_IDLE 仲裁。優先序 A1..A5(低位元先)。
 *   - DEBUG_MARKERS=0 時 marker 分支為常數假,綜合後剪除,
 *     wire format 就是純 512(+4) bytes。
 *   - send_go(= state==ST_SEND)觸發一次 stream;完成後要等
 *     send_go 落下才重新武裝,同一次 ST_SEND 不會送兩輪。
 *   - all_done 在最後一個 byte「完全上線」後才脈衝(SRC_DRAIN 等
 *     streamer 的 ready 重新升起,見其時序契約),對齊舊
 *     tx_all_done 的時點。
 *
 * 之後 double buffer 只需要改這個模組(哪個 context 好了就先排水
 * 哪個),streamer 與 uart_tx 一行不動 -- 這正是切在這裡的理由。
 */
module systolic_tx_source #(
    parameter bit DEBUG_MARKERS = 1'b0,
    parameter bit CYCLE_COUNTER = 1'b0
)(
    input  logic        clk,
    input  logic        rst,

    // 主 FSM 介面
    input  logic        send_go,        // level: state == ST_SEND
    output logic        all_done,       // 1-cycle pulse

    // breadcrumb(捕捉在 top,送出在這裡)
    input  logic [4:0]  debug_pending,
    output logic [4:0]  debug_accept,

    // 內容
    input  logic [31:0] C0 [0:7][0:7],
    input  logic [31:0] C1 [0:7][0:7],
    input  logic [31:0] cyc_latched,

    // byte stream 出口
    output logic        m_valid,
    output logic [7:0]  m_data,
    input  logic        m_ready
);

    localparam int TX_BYTES       = 512;
    localparam int TX_TOTAL_BYTES = TX_BYTES + (CYCLE_COUNTER ? 4 : 0);
    localparam int TX_LAST        = TX_TOTAL_BYTES - 1;

    typedef enum logic [1:0] {
        SRC_IDLE,
        SRC_MARK,    // 送一個 marker byte
        SRC_DATA,    // 送結果 stream
        SRC_DRAIN    // 最後一個 byte 已交付,等它真正上線
    } src_t;

    src_t src;

    logic [9:0] cnt;
    logic [7:0] mark_byte;
    logic       send_started;

    /*
     * 結果 word 選擇 -- 逐字搬自舊 top:
     *   0..255 = C0([7:5]=row [4:2]=col [1:0]=byte)
     *   256..511 = C1(同 layout,索引扣 256)
     *   512..515 = cycle counter(little-endian;512 是 4 的倍數,
     *              byte lane 對齊)
     */
    logic [31:0] word;

    always_comb begin
        if (cnt < 10'd256)
            word = C0[cnt[7:5]][cnt[4:2]];
        else if (cnt < 10'd512)
            word = C1[(cnt - 10'd256) >> 5][((cnt - 10'd256) >> 2) & 7];
        else
            word = cyc_latched;
    end

    logic [7:0] data_byte;

    always_comb begin
        case (cnt[1:0])
            2'd0:    data_byte = word[7:0];
            2'd1:    data_byte = word[15:8];
            2'd2:    data_byte = word[23:16];
            default: data_byte = word[31:24];
        endcase
    end

    assign m_valid = (src == SRC_MARK) || (src == SRC_DATA);
    assign m_data  = (src == SRC_MARK) ? mark_byte : data_byte;

    always_ff @(posedge clk) begin
        if (rst) begin
            src          <= SRC_IDLE;
            cnt          <= '0;
            mark_byte    <= 8'h00;
            send_started <= 1'b0;
            all_done     <= 1'b0;
            debug_accept <= 5'b0;
        end
        else begin
            all_done     <= 1'b0;
            debug_accept <= 5'b0;

            case (src)

                SRC_IDLE: begin
                    /*
                     * 優先權與舊 TX_IDLE 完全相同:marker(低位先)
                     * 壓過結果 stream。
                     */
                    if (DEBUG_MARKERS && debug_pending[0]) begin
                        mark_byte       <= 8'hA1;
                        debug_accept[0] <= 1'b1;
                        src             <= SRC_MARK;
                    end
                    else if (DEBUG_MARKERS && debug_pending[1]) begin
                        mark_byte       <= 8'hA2;
                        debug_accept[1] <= 1'b1;
                        src             <= SRC_MARK;
                    end
                    else if (DEBUG_MARKERS && debug_pending[2]) begin
                        mark_byte       <= 8'hA3;
                        debug_accept[2] <= 1'b1;
                        src             <= SRC_MARK;
                    end
                    else if (DEBUG_MARKERS && debug_pending[3]) begin
                        mark_byte       <= 8'hA4;
                        debug_accept[3] <= 1'b1;
                        src             <= SRC_MARK;
                    end
                    else if (DEBUG_MARKERS && debug_pending[4]) begin
                        mark_byte       <= 8'hA5;
                        debug_accept[4] <= 1'b1;
                        src             <= SRC_MARK;
                    end
                    else if (send_go && !send_started) begin
                        send_started <= 1'b1;
                        cnt          <= '0;
                        src          <= SRC_DATA;
                    end

                    // rearm:與舊 FSM 相同,只在 idle 檢查。
                    if (!send_go)
                        send_started <= 1'b0;
                end

                SRC_MARK: begin
                    // marker 恰好一個 byte;交付即返回。若還有
                    // pending,下一輪 SRC_IDLE 會再送 -- streamer
                    // 要到前一個 byte 上線後才會再 ready,順序不亂。
                    if (m_ready)
                        src <= SRC_IDLE;
                end

                SRC_DATA: begin
                    if (m_ready) begin
                        if (cnt == TX_LAST[9:0]) begin
                            cnt <= '0;
                            src <= SRC_DRAIN;
                        end
                        else begin
                            cnt <= cnt + 1'b1;
                        end
                    end
                end

                SRC_DRAIN: begin
                    /*
                     * m_ready 再度升起 = streamer 把最後一個 byte
                     * 完整送上線。此時才 all_done,主 FSM 才離開
                     * ST_SEND -- 對齊舊 tx_all_done 的語意。
                     */
                    if (m_ready) begin
                        all_done <= 1'b1;
                        src      <= SRC_IDLE;
                    end
                end

                default: src <= SRC_IDLE;

            endcase
        end
    end

endmodule
