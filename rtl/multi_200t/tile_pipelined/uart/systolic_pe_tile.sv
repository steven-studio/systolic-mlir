/*
 * ============================================================
 * systolic_pe_tile -- 一個處理單元 (PE)
 * ============================================================
 *
 * 它做什麼
 * --------
 * 這個 PE 每拍最多收到一組運算元 (a, b)。它把 a*b 加進一個
 * 累加器,並且把 a、b 原封不動往下一個 PE 傳(各延遲一拍)。
 *
 * 收完一整串之後,把累加結果吐出來:acc_out + acc_valid_out。
 *
 *   a_in ──┬──► [暫存一拍] ──► a_out ──► 右邊的 PE
 *          │
 *          ├──► fp_mul ──► fp_add ──► acc_bank[]
 *          │
 *   b_in ──┴──► [暫存一拍] ──► b_out ──► 下面的 PE
 *
 *
 * 為什麼需要很多個 accumulator bank
 * ---------------------------------
 * fp_add 是管線化的,一個加法要 12 拍才出結果。
 *
 * 如果只有一個累加器:
 *   第 t 拍   acc = acc + p0     ← 結果在 t+12 才有
 *   第 t+1 拍 acc = acc + p1     ← 但 acc 還沒更新,必須等
 *   → 每 12 拍才能吃一個乘積
 *
 * 如果有 16 個 bank,乘積依序輪流放:
 *   第 t 拍    bank0  += p0
 *   第 t+1 拍  bank1  += p1      ← 不同的暫存器,不用等
 *   ...
 *   第 t+16 拍 bank0  += p16     ← 回到 bank0,而 t 那筆在 t+12 已完成
 *   → 每 1 拍吃一個乘積
 *
 * 關鍵數字:同一個 bank 被再次使用的間隔 = ACC_BANKS = 16,
 * 必須大於加法器延遲 12。這是「結構上不可能衝突」,不是
 * 「偵測到衝突就擋下來」—— 所以不需要任何互鎖電路。
 *
 *
 * 最後怎麼把 16 個 bank 併成一個數
 * --------------------------------
 * 樹狀,就地相加,每次距離減半:
 *
 *   stride=8 : bank[0..7]  += bank[8..15]
 *   stride=4 : bank[0..3]  += bank[4..7]
 *   stride=2 : bank[0..1]  += bank[2..3]
 *   stride=1 : bank[0]     += bank[1]
 *   → 答案在 bank[0]
 *
 * 同一層裡的加法互不相干,可以一拍發一個、不必等。
 * 但下一層要讀上一層寫回去的值,所以層與層之間必須等乾淨。
 *
 * ============================================================
 * 本檔目前的進度
 *
 *   [完成] 輸入級與 pass-through
 *   [完成] fp_mul / fp_add 實例化
 *   [進行] TODO 1  bank 計數器 —— 讀取端已完成,寫回端與
 *                  busy 計數器未完成
 *   [待做] TODO 2  加法器輸入 mux
 *   [待做] TODO 3  寫回 bank 與歸零
 *   [待做] TODO 4  排空判準
 *   [待做] TODO 5  歸約 FSM
 *   [待做] TODO 6  輸出脈衝
 *
 * 每個 TODO 都寫了:要做什麼、要滿足什麼性質、怎麼檢查。
 * 一次做一個,做完就能編譯、能跑、能 commit。
 *
 * 測試:tb_pe_counters.sv + fp_model.sv
 *   verilator --binary -Wno-fatal --top-module tb_pe_counters \
 *       tb_pe_counters.sv fp_model.sv systolic_pe_tile.sv -o tbrun
 *   ./obj_dir/tbrun
 * ============================================================
 */
module systolic_pe_tile #(
    parameter int DATA_W    = 32,
    parameter int ACC_BANKS = 16,

    /*
     * 這兩個參數新設計用不到 —— PE 不再靠「延遲幾拍」推算資料
     * 屬於哪個 bank,而是用計數器重播同一個序列(見 TODO 1)。
     *
     * tb 把 fp_model 的 LAT 改成 3/5 仍然通過,就是這件事的證明。
     * 全部寫完、確認沒有任何一處引用之後刪掉。
     */
    parameter int MUL_LATENCY = 9,
    parameter int ADD_LATENCY = 12
) (
    input  logic clk,
    input  logic rst,

    // --- 運算元進來 ---
    input  logic              a_valid_in,
    input  logic              b_valid_in,
    input  logic [DATA_W-1:0] a_in,
    input  logic [DATA_W-1:0] b_in,

    // --- 運算元往下一個 PE(各延遲一拍)---
    output logic              a_valid_out,
    output logic              b_valid_out,
    output logic [DATA_W-1:0] a_out,
    output logic [DATA_W-1:0] b_out,

    // --- 這個 PE 的最終結果 ---
    output logic              acc_valid_out,   // 一拍脈衝
    output logic [DATA_W-1:0] acc_out
);

    /*
     * bank 編號要幾個位元。ACC_BANKS=16 → 4 個位元 → 編號 0..15
     */
    localparam int ACC_SEL_W = $clog2(ACC_BANKS);


    /* ============================================================
     * 1. 輸入級與 pass-through  [已完成]
     * ============================================================
     *
     * a 和 b 各自暫存一拍,然後送給下一個 PE。這一拍的延遲就是
     * systolic array 的「脈動」:資料每經過一個 PE 就晚一拍。
     */

    logic [DATA_W-1:0] a_reg, b_reg;
    logic              a_valid_reg, b_valid_reg;

    /* 這一拍「暫存級」有完整的一組運算元 —— 這組要餵給乘法器。
     *
     * 輸入端的 pair_valid 已經刪掉:新設計裡沒有任何邏輯需要它。
     * 所有判斷都發生在暫存級之後,因為那才是真正進到乘法器的東西。 */
    wire pipe_pair_valid = a_valid_reg && b_valid_reg;

    always_ff @(posedge clk) begin
        if (rst) begin
            a_reg       <= '0;
            b_reg       <= '0;
            a_valid_reg <= 1'b0;
            b_valid_reg <= 1'b0;
        end
        else begin
            a_valid_reg <= a_valid_in;
            b_valid_reg <= b_valid_in;

            // 只在 valid 的時候擷取。無效時保持舊值,不做無謂的翻轉。
            if (a_valid_in) a_reg <= a_in;
            if (b_valid_in) b_reg <= b_in;
        end
    end

    assign a_out       = a_reg;
    assign b_out       = b_reg;
    assign a_valid_out = a_valid_reg;
    assign b_valid_out = b_valid_reg;


    /* ============================================================
     * 2. 乘法器  [已完成]
     * ============================================================
     *
     * 丟一組 (a, b) 進去,過幾拍之後 product_valid 拉高、product
     * 就是答案。幾拍?不知道,也不需要知道 —— 見 [TODO 1]。
     *
     * fp_mul 保證「保序」:先丟進去的先出來,而且一進一出。
     */

    logic [DATA_W-1:0] product;
    logic              product_valid;

    fp_mul u_fp_mul (
        .clk       (clk),
        .rst       (rst),
        .valid_in  (pipe_pair_valid),
        .a         (a_reg),
        .b         (b_reg),
        .valid_out (product_valid),
        .result    (product)
    );


    /* ============================================================
     * [TODO 1] bank 編號與 busy 計數器   ── 進行中
     * ============================================================
     *
     * 問題:乘積從 fp_mul 出來的時候,它該加進哪個 bank?
     *
     * 舊版開一個 FIFO,發射時把 bank 編號存進去、出來時讀出來。
     * 那是對的,但沒必要:bank 是「依序」指派的,而 fp_mul 保序,
     * 所以出來的順序也是同一個序列。
     *
     *   發射端  0, 1, 2, ..., 15, 0, 1, ...
     *   出來端  0, 1, 2, ..., 15, 0, 1, ...   ← 同一個序列
     *
     * 一個計數器就能重現它,不需要記錄任何東西。
     *
     * 這個簡化以「移除 ctx」為前提。有兩個 accumulator context 的
     * 時候,單一計數器分不出回來的乘積屬於哪一個 context,那才是
     * 舊版非用 FIFO 不可的原因。
     *
     * ------------------------------------------------------------
     * 已決定:不要 issue_bank
     *
     * 發射端的 bank 編號沒有任何邏輯會用到 —— 乘法器不需要知道
     * 資料要加去哪裡。只有讀取端與寫回端需要。
     *
     * ------------------------------------------------------------
     * 已完成:product_bank(讀取端)
     *
     * 在 product_valid 上前進。乘積出來的那一拍,它就是這個乘積
     * 該加進去的 bank 編號。
     *
     * 4 位元的計數器數到 15 再 +1 自然回捲到 0,不需要 if。
     * 前提是 ACC_BANKS 是 2 的冪 —— 目前沒有任何機制擋住有人傳
     * 12 進來,值得補一個 initial 斷言。
     *
     * ------------------------------------------------------------
     * 還沒完成 (a):寫回端的計數器
     *
     * 讀取和寫回不在同一拍。乘積 P0 在第 t 拍出來、讀 acc_bank[0]、
     * 丟進加法器;結果要到 t+12 才回來,那時 product_bank 已經
     * 前進 12 次,指到 12 而不是 0。
     *
     * 所以寫回需要自己的計數器,在 add_valid 上前進。fp_add 一樣
     * 保序,所以它重播的是同一個序列,只是整體晚了一個加法器延遲。
     *
     *   ⚠ 陷阱:add_valid 在「歸約期間」也會拉高(見 TODO 5)。
     *     如果不擋,寫回計數器會多前進 ACC_BANKS-1 次,下一次
     *     交易就整個錯位 —— 而症狀是「第一次對、第二次開始錯」。
     *     等 TODO 5 有了 state 之後,要加上只在累加期間前進的條件。
     *
     * ------------------------------------------------------------
     * 還沒完成 (b):兩個 busy 計數器
     *
     * TODO 4 判斷管線排空要用:
     *
     *   mul_busy   pipe_pair_valid 時 +1,product_valid 時 -1
     *   add_busy   fp_add_valid_in 時 +1,add_valid     時 -1
     *
     * 同一拍一進一出就維持不變。8 位元足夠 —— 管線裡不可能同時
     * 有超過幾十個交易。
     *
     * ------------------------------------------------------------
     * 檢查方式:tb_pe_counters
     *
     * tb 自己養一份 expect_bank、用同樣的規則前進,兩邊獨立算出
     * 同一個序列才算數。目前 305 項檢查通過;把 fp_model 的 LAT
     * 從 9/12 改成 3/5 仍然通過 —— 那證明這裡沒有把延遲寫死。
     * ============================================================
     */

    /* 讀取端:乘積出來那一拍,它屬於哪一格 */
    logic [ACC_SEL_W-1:0] product_bank;

    /* 寫回端:累加的加法結果回來時,要寫回哪一格。
     * 與 product_bank 相差一個加法器延遲,所以必須是獨立的計數器。
     * 只在 PE_ACCUM 前進 —— 歸約期間的 add_valid 不屬於它。 */
    logic [ACC_SEL_W-1:0] accum_wb_bank;

    /* 兩個管線裡各還有幾筆沒回來 */
    logic [7:0] mul_busy;
    logic [7:0] add_busy;

    always_ff @(posedge clk) begin
        if (rst || clear_all) begin
            product_bank  <= '0;
            accum_wb_bank <= '0;
        end
        else begin
            if (product_valid)
                product_bank <= product_bank + 1'b1;

            if (add_valid && state == PE_ACCUM)
                accum_wb_bank <= accum_wb_bank + 1'b1;
        end
    end

    always_ff @(posedge clk) begin
        if (rst) begin
            mul_busy <= '0;
            add_busy <= '0;
        end
        else begin
            /* 同一拍一進一出就維持不變,所以只寫兩個單邊條件 */
            if (pipe_pair_valid && !product_valid)      mul_busy <= mul_busy + 1'b1;
            else if (!pipe_pair_valid && product_valid) mul_busy <= mul_busy - 1'b1;

            if (fp_add_valid_in && !add_valid)      add_busy <= add_busy + 1'b1;
            else if (!fp_add_valid_in && add_valid) add_busy <= add_busy - 1'b1;
        end
    end


    /* ============================================================
     * 3. 累加器  [宣告完成,寫入邏輯見 TODO 3]
     * ============================================================
     */

    logic [DATA_W-1:0] acc_bank [0:ACC_BANKS-1];

    /* ============================================================
     * PE 狀態
     * ============================================================
     *
     *        ┌──────────────────────────────────────┐
     *        ▼                                      │
     *   PE_IDLE ──第一組運算元進來──► PE_ACCUM       │
     *                                    │           │
     *                       管線排空                 │
     *                                    ▼           │
     *                                PE_REDUCE       │
     *                                    │           │
     *                          stride 走完 1         │
     *                                    ▼           │
     *                                 PE_DONE ───────┘
     *
     * PE_IDLE    還沒開始。等第一次 pipe_pair_valid
     * PE_ACCUM   累加中。乘積經 fp_add 寫進 acc_bank
     * PE_REDUCE  歸約中。樹狀相加,層間等管線空
     * PE_DONE    結果出爐。送出 acc_valid_out 脈衝,清乾淨所有 bank
     *
     * 這個 enum 不用寫死數值 —— state 不出這個模組,沒有第二份
     * 定義會漂移。(頂層那個 FSM 要寫死,是因為 systolic_status.sv
     * 複製了一份編碼去解 LED。)
     */

    typedef enum logic [1:0] {
        PE_IDLE,
        PE_ACCUM,
        PE_REDUCE,
        PE_DONE
    } pe_state_t;

    pe_state_t state;


    /* ============================================================
     * [TODO 2] 加法器的兩個輸入要接什麼
     * ============================================================
     *
     * fp_add 被兩件事共用:
     *
     *   累加期間:  a = acc_bank[product_bank]   b = product
     *              什麼時候發?product_valid 的時候
     *
     *   歸約期間:  a = acc_bank[reduce_i]       b = acc_bank[reduce_i + stride]
     *              什麼時候發?還有加法沒發完的時候(見 TODO 5)
     *
     * 兩者不會同時發生 —— 歸約只在累加完全結束之後才開始。
     *
     * 目前先綁死成不發,讓檔案可以編譯。
     *
     * 檢查方式:[TODO 3] 一起驗。
     */

    logic              fp_add_valid_in;
    logic [DATA_W-1:0] fp_add_a, fp_add_b;

    always_comb begin
        if (state == PE_REDUCE) begin
            /* 歸約:距離 stride 的兩格相加,寫回低位那格 */
            fp_add_valid_in = (reduce_todo != 0);
            fp_add_a        = acc_bank[reduce_i];
            fp_add_b        = acc_bank[reduce_i + reduce_stride];
        end
        else begin
            /* 累加:乘積回來就發。
             * PE_IDLE / PE_DONE 時 product_valid 恆為 0,所以這個
             * 分支也涵蓋那兩個狀態,不需要額外的 case。 */
            fp_add_valid_in = product_valid;
            fp_add_a        = acc_bank[product_bank];
            fp_add_b        = product;
        end
    end


    /* ============================================================
     * 4. 加法器  [已完成]
     * ============================================================
     *
     * 跟 fp_mul 一樣:保序、一進一出、延遲不需要知道。
     */

    logic [DATA_W-1:0] add_result;
    logic              add_valid;

    fp_add u_fp_add (
        .clk       (clk),
        .rst       (rst),
        .valid_in  (fp_add_valid_in),
        .a         (fp_add_a),
        .b         (fp_add_b),
        .valid_out (add_valid),
        .result    (add_result)
    );


    /* ============================================================
     * [TODO 3] 寫回 accumulator bank
     * ============================================================
     *
     * add_valid 拉高的那一拍,add_result 就是答案,要寫進某個 bank:
     *
     *   累加期間 → 寫回「寫回端計數器」(TODO 1 (a),尚未建立)
     *              也就是那個乘積原本所屬的 bank
     *   歸約期間 → 寫回這一層正在算的那個低位索引
     *
     * 另外還需要一個「全部歸零」的路徑:一次交易結束、結果送出
     * 之後,所有 bank 要清成 0,下一次交易才不會累加到舊資料。
     *
     * 檢查方式:
     *   餵 16 組小整數(例如 a 全是 1、b 依序 1..16),讓它跑完
     *   累加,然後在 tb 裡用階層路徑讀 dut.acc_bank[i],應該看到
     *   bank[i] == 第 i 個乘積。全部加起來 = 136。
     */

    /* 交易結束那一拍把所有 bank 清成 0 */
    wire clear_all = (state == PE_DONE);

    always_ff @(posedge clk) begin
        if (rst || clear_all) begin
            /* 清零優先於單格寫入:否則交易結束那一拍若剛好有一個
             * add_valid,那一格會留下髒資料給下一次交易。 */
            for (int i = 0; i < ACC_BANKS; i++)
                acc_bank[i] <= '0;
        end
        else if (add_valid) begin
            /* 兩個階段共用同一條寫回路徑,差別只在索引 */
            acc_bank[(state == PE_REDUCE) ? reduce_wb_i : accum_wb_bank]
                <= add_result;
        end
    end


    /* ============================================================
     * [TODO 4] 什麼時候算「輸入結束、管線排空」
     * ============================================================
     *
     * 歸約不能太早開始 —— 還有乘積在管線裡飛的話,它們會加到
     * 已經被歸約過的 bank 上,答案就錯了。
     *
     * 要滿足三件事才算乾淨:
     *
     *   1. 輸入真的結束了
     *      → pipe_pair_valid 從 1 變 0 的那一刻(而且之前至少
     *        有過一次 1,否則重置後就會誤判)
     *
     *   2. 乘法器裡沒有東西      → mul_busy == 0
     *   3. 加法器裡沒有東西      → add_busy == 0
     *
     * 注意第 2 條不能省。輸入結束的當下,第一個乘積可能都還沒
     * 從 fp_mul 出來(k 很短的時候),這時 add_busy 也是 0,
     * 看起來很乾淨,其實什麼都還沒算。
     *
     * 檢查方式:
     *   在 tb 裡對這個「乾淨」訊號設一個斷言:它拉高的那一拍,
     *   mul_busy 和 add_busy 必須都是 0,而且整個交易只能拉高一次。
     */

    /* 判準寫在 FSM 的 PE_ACCUM 分支裡:
     *
     *     !pipe_pair_valid && mul_busy == 0 && add_busy == 0
     *
     * 有了 PE_IDLE 之後就不需要 transaction_seen —— 狀態本身就是
     * 「這次交易已經開始過」這個旗標。
     */


    /* ============================================================
     * [TODO 5] 歸約:樹狀,就地,一個狀態
     * ============================================================
     *
     * 狀態機只有兩個狀態:
     *
     *   PE_ACCUM   累加中。[TODO 4] 的條件成立就跳到 PE_REDUCE
     *   PE_REDUCE  歸約中。做完跳回 PE_ACCUM
     *
     * PE_REDUCE 裡面只有一個 if-else:
     *
     *   if (這一層還有加法沒發出去) begin
     *       發一個,推進索引
     *   end
     *   else if (add_busy == 0) begin        ← 這就是「層間的等」
     *       if (stride == 1)  完成,輸出結果
     *       else              stride 減半,重新裝填這一層的加法數
     *   end
     *
     * 「等」不是一個狀態,是「沒事做而且管線空了」。
     *
     * 需要的狀態變數:
     *   reduce_stride   8 → 4 → 2 → 1
     *   reduce_i        這一層發到第幾個(0 .. stride-1)
     *   reduce_todo     這一層還剩幾個沒發(裝填時 = stride)
     *
     * 提示:stride 減半的時候,新的一層有 stride>>1 個加法。
     *       用非阻塞賦值的話,reduce_todo <= reduce_stride >> 1
     *       和 reduce_stride <= reduce_stride >> 1 可以並排寫,
     *       兩邊讀到的都是舊的 reduce_stride。
     *
     * 檢查方式:
     *   接續 [TODO 3] 的刺激,跑完歸約之後 dut.acc_bank[0]
     *   應該等於 136(bit-exact,因為都是小整數)。
     */

    /* 樹狀歸約的狀態變數 */
    logic [ACC_SEL_W-1:0] reduce_stride;   // 8 -> 4 -> 2 -> 1
    logic [ACC_SEL_W-1:0] reduce_i;        // 這一層「發射」到第幾個
    logic [ACC_SEL_W-1:0] reduce_wb_i;     // 這一層「寫回」到第幾個
    logic [ACC_SEL_W-1:0] reduce_todo;     // 這一層還剩幾個沒發

    always_ff @(posedge clk) begin
        if (rst) begin
            state         <= PE_IDLE;
            reduce_stride <= '0;
            reduce_i      <= '0;
            reduce_wb_i   <= '0;
            reduce_todo   <= '0;
        end
        else begin

            /* 歸約的寫回索引:與 reduce_i 是一對,差一個加法器延遲。
             * 每一層重新從 0 開始 —— 換層的條件是 add_busy == 0,
             * 表示上一層的寫回都已經落地,所以歸零是安全的。 */
            if (state == PE_REDUCE && add_valid)
                reduce_wb_i <= reduce_wb_i + 1'b1;

            case (state)

                /* 等第一組運算元進到乘法器 */
                PE_IDLE: begin
                    if (pipe_pair_valid)
                        state <= PE_ACCUM;
                end

                /* 輸入停了、而且兩個管線都空了
                 * → 所有 bank 的值已經定案,可以開始歸約。
                 *
                 * 這裡假設運算元串流是連續的:中間若有超過乘法器
                 * 深度的空檔,mul_busy 會歸零而誤觸。feeder 目前
                 * 連續餵 k,成立;哪天加了 back-pressure 要重看。 */
                PE_ACCUM: begin
                    if (!pipe_pair_valid && mul_busy == 0 && add_busy == 0) begin
                        reduce_stride <= ACC_SEL_W'(ACC_BANKS / 2);
                        reduce_todo   <= ACC_SEL_W'(ACC_BANKS / 2);
                        reduce_i      <= '0;
                        reduce_wb_i   <= '0;
                        state         <= PE_REDUCE;
                    end
                end

                PE_REDUCE: begin
                    if (reduce_todo != 0) begin
                        /* 有事做:這一拍發一個加法,推進發射索引。
                         * 實際的發射由 fp_add_valid_in 負責。 */
                        reduce_todo <= reduce_todo - 1'b1;
                        reduce_i    <= reduce_i    + 1'b1;
                    end
                    else if (add_busy == 0) begin
                        /* 沒事做 + 管線空了 = 這一層全部落地。
                         * 這就是層間的 barrier —— 一個條件,不是一個狀態。 */
                        if (reduce_stride == ACC_SEL_W'(1)) begin
                            state <= PE_DONE;
                        end
                        else begin
                            reduce_stride <= reduce_stride >> 1;
                            reduce_todo   <= reduce_stride >> 1;  // 讀到舊的 stride
                            reduce_i      <= '0;
                            reduce_wb_i   <= '0;
                        end
                    end
                end

                /* 送出脈衝、清乾淨,一拍就走 */
                PE_DONE: begin
                    state <= PE_IDLE;
                end

                default: state <= PE_IDLE;

            endcase
        end
    end


    /* ============================================================
     * [TODO 6] 輸出
     * ============================================================
     *
     * 歸約完成的那一拍:
     *   acc_out       <= acc_bank[0]
     *   acc_valid_out <= 1'b1     ← 只有這一拍,下一拍要自己回 0
     *
     * 同時把所有 bank 清成 0(見 [TODO 3] 的歸零路徑),並且把
     * [TODO 4] 的「輸入結束」旗標也清掉,準備接下一次交易。
     *
     * 檢查方式:
     *   連續跑兩次交易,第二次的結果必須正確 —— 如果沒清乾淨,
     *   第二次會等於兩次的和。
     */

    always_ff @(posedge clk) begin
        if (rst) begin
            acc_valid_out <= 1'b0;
            acc_out       <= '0;
        end
        else begin
            /* 一拍脈衝,與 acc_out 同時出現在 PE_DONE 的下一拍 */
            acc_valid_out <= (state == PE_DONE);

            /* 同一拍 clear_all 也在清 bank,但兩邊都是非阻塞賦值,
             * 右手邊取的是這一拍開始時的值 —— 所以讀得到答案。 */
            if (state == PE_DONE)
                acc_out <= acc_bank[0];
        end
    end

endmodule