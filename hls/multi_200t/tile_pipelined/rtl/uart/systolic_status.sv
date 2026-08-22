`timescale 1ns / 1ps

/*
 * ============================================================
 * systolic_status -- 常駐的板上狀態顯示(純觀測)
 * ============================================================
 *
 * 這個模組只讀訊號、只驅動 LED,不寫回設計的任何一條線。
 * 因此它對既有行為的影響在結構上為零 -- 等價性由「沒有任何
 * 既有訊號被這裡指派」保證,不需要逐拍比對來證明。
 *
 * 為什麼需要它
 * ------------
 * DEBUG_MARKERS 的 breadcrumb 是有效的,但代價很重:它改變 TX
 * wire format(host 必須剝 byte)、而且加了 marker 邏輯之後
 * placement 會重擲 -- 2026-08-19 的 k16 事件裡,dbg 版能動而
 * clean 版不能動,正是這個副作用製造出來的假訊號。
 *
 * LED 沒有這些代價:協定不變、host 不用改、觀測是連續的而不是
 * 一次性的。接下來的 operand double-buffer 會引入死鎖類的錯誤
 * (RX 等不到空槽 / compute 等不到滿槽),症狀是板上完全沉默 --
 * 跟 RX 0/512 一模一樣。先把儀器裝好,再動待測物。
 *
 * 顯示配置
 * --------
 * 外接(裝在看得見的地方):
 *   jb_led[0]  心跳 ~1.5 Hz     時脈活著 + bitstream 有進去
 *   jb_led[1]  busy             state != ST_IDLE,遠遠一眼看出在算還是卡死
 *
 * 板上 LD0..LD7:
 *   led[0]  frame 被接受(閃一下)  對應 breadcrumb A1
 *   led[1]  RX 進行中             收 16KB payload 時會亮 1.4 秒,很明顯
 *   led[2]  ST_IDLE      \
 *   led[3]  ST_FEED       |  主 FSM one-hot。刻意不用二進位編碼 --
 *   led[4]  ST_WAIT_RESULT|  凌晨兩點盯著板子時,「哪顆亮就是卡在哪一級」
 *   led[5]  ST_SEND      /   不需要在腦袋裡解碼。
 *   led[6]  c0_done               對應 breadcrumb A3
 *   led[7]  c1_done               對應 breadcrumb A4
 *
 * 之後 double-buffer 上線時再加一個埠與一顆燈顯示 slot 佔用
 * (死鎖時直接看得到誰在等誰)。現在不預留未使用的埠 -- 空接的
 * 埠只會產生 lint 警告與閱讀時的疑問。
 * ============================================================
 */
module systolic_status #(
    /*
     * 心跳:cnt[BLINK_LOG2] 翻轉。100 MHz / 2^25 = 每 0.336 秒
     * 翻一次,約 1.5 Hz。
     */
    parameter int BLINK_LOG2 = 25,

    /*
     * 脈衝展寬長度。matrices_ready 這類一拍脈衝人眼看不到,
     * 拉成 2^22 拍 = 100 MHz 下約 42 ms。人眼可辨的最短亮燈
     * 約 20-50 ms,42 ms 剛好在區間內又不會拖成一團糊。
     */
    parameter int STRETCH_LOG2 = 22
)(
    input  logic       clk,
    input  logic       rst,

    // --- 被觀測的訊號(全部是 input,這裡不驅動任何一條) ---
    input  logic       frame_accepted,  // 一拍脈衝 (matrices_ready)
    input  logic       rx_active,       // RX framing 不在 HUNT
    input  logic [2:0] state,           // 主 FSM
    input  logic       c0_done,
    input  logic       c1_done,

    // --- 輸出 ---
    output logic [7:0] led,             // 板上 LD0..LD7
    output logic [1:0] jb_led           // 外接 Pmod JB pin 4 / pin 10
);

    /* 主 FSM 的編碼必須與 systolic_uart_tile_top 的 state_t 一致。
     * 這裡用 localparam 而不是共用 typedef,是為了讓這個模組能被
     * 單獨模擬而不必引入頂層的型別。數值變更時兩邊要一起改 --
     * 下方的 one-hot 解碼有 default 分支,編碼不符時四顆全暗,
     * 是一個看得見的失敗而不是誤導性的顯示。 */
    localparam logic [2:0] S_IDLE = 3'd0;
    localparam logic [2:0] S_FEED = 3'd1;
    localparam logic [2:0] S_WAIT = 3'd2;
    localparam logic [2:0] S_SEND = 3'd3;

    /*
     * ------------------------------------------------------------
     * 心跳
     *
     * 初始值 '0 在 7-series 有效:FF 於 configuration 當下載入
     * 宣告值。心跳因此不依賴 rst -- 就算 reset 路徑本身壞了,
     * 心跳仍會跳,這正是診斷時想要的性質。
     * ------------------------------------------------------------
     */
    logic [BLINK_LOG2:0] beat_cnt = '0;

    always_ff @(posedge clk)
        beat_cnt <= beat_cnt + 1'b1;


    /*
     * ------------------------------------------------------------
     * 脈衝展寬:載入計數器,倒數到零
     * ------------------------------------------------------------
     */
    logic [STRETCH_LOG2-1:0] frame_stretch;

    always_ff @(posedge clk) begin
        if (rst)
            frame_stretch <= '0;
        else if (frame_accepted)
            frame_stretch <= '1;
        else if (frame_stretch != '0)
            frame_stretch <= frame_stretch - 1'b1;
    end

    wire frame_flash = (frame_stretch != '0);


    /*
     * ------------------------------------------------------------
     * 輸出
     *
     * 全部由暫存器驅動:LED 是跨到 I/O 的長路徑,經一級 FF 可以
     * 讓 placer 自由擺放,不會把觀測邏輯拉進關鍵路徑。
     * ------------------------------------------------------------
     */
    always_ff @(posedge clk) begin
        if (rst) begin
            led    <= 8'b0;
            jb_led <= 2'b0;
        end
        else begin
            led[0] <= frame_flash;
            led[1] <= rx_active;

            led[2] <= (state == S_IDLE);
            led[3] <= (state == S_FEED);
            led[4] <= (state == S_WAIT);
            led[5] <= (state == S_SEND);

            led[6] <= c0_done;
            led[7] <= c1_done;

            jb_led[0] <= beat_cnt[BLINK_LOG2];
            jb_led[1] <= (state != S_IDLE);
        end
    end

`ifndef SYNTHESIS
    /* 編碼漂移的防呆:state 若出現預期外的值,one-hot 會全暗。
     * 那是刻意的(看得見的失敗),但模擬時要吵出來。 */
    always_ff @(posedge clk) begin
        if (!rst && state > S_SEND)
            $error("systolic_status: unexpected state %0d -- 編碼與頂層不同步?", state);
    end
`endif

endmodule
