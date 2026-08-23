/*
 * uart_tx_streamer -- valid/ready byte 介面 -> uart_tx 的 start/busy 協定。
 *
 * 把「送一個 byte」的三段式握手(等 idle、脈衝 start、等 busy 起落)
 * 封裝成標準 stream 介面。上游只需要:
 *
 *     s_valid = 我有 byte
 *     s_ready = 我可以收
 *     握手成立(同一拍 valid && ready)即視為交付。
 *
 * uart_tx 本身一行不改。之後任何要往 host 送資料的東西(結果、
 * marker、未來 double buffer 的逐 context 排水)都掛在這個介面上,
 * 不再各自複製 start/busy 樣板 -- 舊 top 的 TX_START / TX_WAIT_BUSY /
 * TX_WAIT_DONE 三個狀態就是被收進這裡。
 *
 * 時序契約(systolic_tx_source 依賴這一條):
 *   s_ready 只在內部 idle 為高;接下一個 byte 後保持低,直到該
 *   byte 完全上線(busy 落下)為止。因此上游看到 ready 再度升起,
 *   等價於舊 FSM 的 TX_WAIT_DONE 完成。
 */
module uart_tx_streamer (
    input  logic       clk,
    input  logic       rst,

    // 上游 stream
    input  logic       s_valid,
    input  logic [7:0] s_data,
    output logic       s_ready,

    // 下游:既有 uart_tx,協定不變
    output logic       tx_start,
    output logic [7:0] tx_data,
    input  logic       tx_busy
);

    typedef enum logic [1:0] {
        S_IDLE,
        S_START,
        S_WAIT_BUSY,
        S_WAIT_DONE
    } st_t;

    st_t st;

    assign s_ready = (st == S_IDLE);

    always_ff @(posedge clk) begin
        if (rst) begin
            st       <= S_IDLE;
            tx_start <= 1'b0;
            tx_data  <= 8'h00;
        end
        else begin
            tx_start <= 1'b0;

            case (st)

                S_IDLE: begin
                    if (s_valid) begin
                        tx_data <= s_data;
                        st      <= S_START;
                    end
                end

                /*
                 * 與舊 FSM 相同的保守序列:等 uart_tx 不忙才打
                 * start,看到 busy 升起才相信它收到,busy 落下
                 * 才算送完。
                 */
                S_START: begin
                    if (!tx_busy) begin
                        tx_start <= 1'b1;
                        st       <= S_WAIT_BUSY;
                    end
                end

                S_WAIT_BUSY: begin
                    if (tx_busy)
                        st <= S_WAIT_DONE;
                end

                S_WAIT_DONE: begin
                    if (!tx_busy)
                        st <= S_IDLE;
                end

                default: st <= S_IDLE;

            endcase
        end
    end

endmodule
