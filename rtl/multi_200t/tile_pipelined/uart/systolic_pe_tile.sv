/*
 * ============================================================
 * systolic_pe_tile -- 一個處理單元 (PE)
 * ============================================================
 *
 * 它做什麼
 * --------
 * 每拍最多收到一組運算元 (a, b)。把 a*b 加進累加器,並且把 a、b
 * 原封不動往下一個 PE 傳(各延遲一拍)。收完一整串之後,把累加
 * 結果連同它的身分吐出來。
 *
 *   a_in ──┬──► [暫存一拍] ──► a_out ──► 右邊的 PE
 *          │
 *          ├──► fp_mul ──► fp_add(累加)──► acc_bank[set][]
 *          │                                      │
 *          │                fp_add(歸約)◄─────────┘
 *   b_in ──┴──► [暫存一拍] ──► b_out ──► 下面的 PE
 *
 *
 * 為什麼需要很多個 accumulator bank
 * ---------------------------------
 * fp_add 是管線化的,一個加法要 12 拍才出結果。只有一個累加器的話
 * 每 12 拍才能吃一個乘積。16 個 bank 輪流放,同一個 bank 被再次
 * 使用的間隔是 16 拍,大於 12 —— 所以每拍都能吃一個。
 *
 * 這是「結構上不可能衝突」,不是「偵測到衝突就擋下來」。
 * 沒有任何互鎖電路。
 *
 *
 * 為什麼有兩組 bank(乒乓)
 * -------------------------
 * 舊版只有一組。歸約要把 16 格併成 1 格,樹狀四層、層間要等加法器
 * 排空,總共約 63 拍:
 *
 *   (8+12) + (4+12) + (2+12) + (1+12) = 63
 *
 * 這 63 拍裡那 16 格正在被讀、被寫、被合併。下一個 tile 的乘積
 * 如果掉進來,會加進一個正在歸約的格子 —— 答案就錯了,而且不會
 * 有任何錯誤訊息。所以舊版的上層必須等歸約做完才能餵下一個 tile。
 *
 * 兩組 bank 之後:一組在累加、另一組在歸約,兩者讀寫的是不同的
 * 暫存器,互不相干。上層的 ST_FEED 就可以跟 ST_WAIT_RESULT 重疊。
 *
 *
 * 為什麼還要第二顆加法器
 * ----------------------
 * 因為衝突有兩種,乒乓只解決了其中一種:
 *
 *   資料衝突 —— 歸約要讀「已經定案」的格子。乒乓解決了:
 *                兩邊碰的根本不是同一組暫存器。
 *
 *   資源衝突 —— 累加全速時加法器佔用率是 100%(每拍一個乘積,
 *                每個乘積一次加法),沒有空的發射槽留給歸約。
 *                這個乒乓解決不了。
 *
 * 所以兩條路徑各給一顆加法器。它們永遠不會爭,因為它們根本不是
 * 同一個單元 —— 一樣是結構上消除,不是靠仲裁。
 *
 *
 * 剩下的那一段沒有被消除
 * ----------------------
 * PE 是靠「輸入停了而且管線排空了」來判斷一次交易結束的,那需要
 * 大約 MUL_LATENCY + ADD_LATENCY = 21 拍的靜默。所以兩個 tile
 * 之間仍然需要這段間隔,上層不能背靠背地餵。
 *
 * 尾巴從 21 + 63 = 84 拍縮到 21 拍。
 *
 * ⚠ 這 21 拍沒有任何硬體防線。上層若餵太早,PE 會把兩個 tile 當成
 *   同一次交易加在一起,而且不會有任何錯誤訊息 —— 症狀是結果偏大,
 *   不是掛掉。這條約束只存在於這段註解和 tb_pe_overlap 裡。
 *
 *   要讓硬體自己認得邊界,就得給每筆交易一個標籤,用「標籤變了」
 *   而不是「排空」當判準。那是下一步,不在這個檔案裡。
 *
 *
 * 測試
 * ----
 *   verilator --binary -Wall -Wno-fatal --top-module tb_pe_counters \
 *       tb/fp_model.sv systolic_pe_tile.sv tb/tb_pe_counters.sv -o tbrun
 *   verilator --binary -Wall -Wno-fatal --top-module tb_pe_reduce \
 *       tb/fp_model.sv systolic_pe_tile.sv tb/tb_pe_reduce.sv -o tbrun
 * ============================================================
 */
module systolic_pe_tile #(
    parameter int DATA_W    = 32,
    parameter int ACC_BANKS = 16,

    /*
     * 這兩個參數邏輯上用不到 —— PE 不靠「延遲幾拍」推算資料屬於
     * 哪個 bank,而是用計數器重播同一個序列。tb 把 fp_model 的 LAT
     * 改成 3/5 或 17/23 仍然通過,就是這件事的證明。
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

    /* bank 編號要幾個位元。ACC_BANKS=16 → 4 個位元 → 編號 0..15 */
    localparam int ACC_SEL_W = $clog2(ACC_BANKS);


    /* ============================================================
     * 1. 輸入級與 pass-through
     * ============================================================
     *
     * a 和 b 各自暫存一拍,然後送給下一個 PE。這一拍的延遲就是
     * systolic array 的「脈動」:資料每經過一個 PE 就晚一拍。
     */

    logic [DATA_W-1:0] a_reg, b_reg;
    logic              a_valid_reg, b_valid_reg;

    /* 這一拍「暫存級」有完整的一組運算元 —— 這組要餵給乘法器。
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
     * 2. 乘法器
     * ============================================================
     *
     * 丟一組 (a, b) 進去,過幾拍之後 product_valid 拉高、product
     * 就是答案。幾拍?不知道,也不需要知道。
     *
     * fp_mul 保證保序:先丟進去的先出來,而且一進一出。
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
     * 3. 兩組 accumulator bank
     * ============================================================
     *
     *   acc_set   現在正在累加的是哪一組
     *   red_set   現在正在歸約的是哪一組
     *
     * 這兩個永遠不相等 —— 歸約做的是累加剛剛離開的那一組,而累加
     * 在交棒的同一拍翻到另一組。所以兩條寫回路徑不可能撞在同一格。
     */

    logic [DATA_W-1:0] acc_bank [0:1][0:ACC_BANKS-1];
    logic              acc_set;
    logic              red_set;


    /* ============================================================
     * 4. 累加路徑
     * ============================================================
     *
     * product_bank   讀取端:乘積出來那一拍,它屬於哪一格
     * accum_wb_bank  寫回端:與 product_bank 相差一個加法器延遲
     *
     * 為什麼要兩個計數器而不是一個:乘積 P0 在第 t 拍出來、讀
     * acc_bank[0]、丟進加法器;結果要到 t+12 才回來,那時
     * product_bank 已經前進 12 次,指到 12 而不是 0。
     *
     * 為什麼不用 FIFO:bank 是依序指派的,而兩顆 IP 都保序,所以
     * 在輸出事件上遞增的計數器會重播出一模一樣的序列。有 ctx 的
     * 時候兩串序列會交錯,counter 重播不出來,那才是舊版非用 FIFO
     * 不可的原因。ctx 拿掉之後 FIFO 就退化成 counter。
     *
     * 兩個計數器都在交棒那一拍歸零。交棒的條件包含「加法器排空」,
     * 所以歸零的時候沒有任何寫回還在飛,不會錯位。
     */

    logic [ACC_SEL_W-1:0] product_bank;
    logic [ACC_SEL_W-1:0] accum_wb_bank;

    logic [7:0] mul_busy;         // 乘法器裡還有幾筆沒回來
    logic [7:0] accum_add_busy;   // 累加加法器裡還有幾筆沒回來
    logic [7:0] reduce_add_busy;  // 歸約加法器裡還有幾筆沒回來

    logic [DATA_W-1:0] accum_add_result;
    logic              accum_add_valid;

    fp_add u_fp_add_accum (
        .clk       (clk),
        .rst       (rst),
        .valid_in  (product_valid),
        .a         (acc_bank[acc_set][product_bank]),
        .b         (product),
        .valid_out (accum_add_valid),
        .result    (accum_add_result)
    );


    /* ============================================================
     * 5. 歸約路徑
     * ============================================================
     *
     * 樹狀,就地相加,每次距離減半:
     *
     *   stride=8 : bank[0..7]  += bank[8..15]
     *   stride=4 : bank[0..3]  += bank[4..7]
     *   stride=2 : bank[0..1]  += bank[2..3]
     *   stride=1 : bank[0]     += bank[1]
     *   → 答案在 bank[0]
     *
     * 同一層裡的加法互不相干,可以一拍發一個、不必等。
     * 但下一層要讀上一層寫回去的值,所以層與層之間必須等乾淨 ——
     * 那個「等」不是一個狀態,是「沒事做而且管線空了」這個條件。
     *
     * reduce_i 與 reduce_wb_i 又是一對讀寫索引,跟累加那一對是
     * 同一個模式:發射索引一路往前跑,回寫索引落後一個加法器延遲。
     * 同一個模式在這個檔案裡出現兩次,看懂一次就看懂全部。
     */

    logic [ACC_SEL_W-1:0] reduce_stride;   // 8 -> 4 -> 2 -> 1
    logic [ACC_SEL_W-1:0] reduce_i;        // 這一層「發射」到第幾個
    logic [ACC_SEL_W-1:0] reduce_wb_i;     // 這一層「寫回」到第幾個
    logic [ACC_SEL_W-1:0] reduce_todo;     // 這一層還剩幾個沒發

    typedef enum logic [1:0] {
        RED_IDLE,   // 沒事做,等交棒
        RED_RUN,    // 歸約中
        RED_DONE    // 結果出爐,送脈衝、清這一組
    } red_state_t;

    red_state_t red_state;

    wire reduce_issue = (red_state == RED_RUN) && (reduce_todo != 0);

    logic [DATA_W-1:0] reduce_add_result;
    logic              reduce_add_valid;

    fp_add u_fp_add_reduce (
        .clk       (clk),
        .rst       (rst),
        .valid_in  (reduce_issue),
        .a         (acc_bank[red_set][reduce_i]),
        .b         (acc_bank[red_set][reduce_i + reduce_stride]),
        .valid_out (reduce_add_valid),
        .result    (reduce_add_result)
    );


    /* ============================================================
     * 6. 三個 busy 計數器
     * ============================================================
     *
     * 同一拍一進一出就維持不變,所以只寫兩個單邊條件。
     * 8 位元足夠 —— 管線裡不可能同時有超過幾十筆。
     */

    always_ff @(posedge clk) begin
        if (rst) begin
            mul_busy        <= '0;
            accum_add_busy  <= '0;
            reduce_add_busy <= '0;
        end
        else begin
            if (pipe_pair_valid && !product_valid)      mul_busy <= mul_busy + 1'b1;
            else if (!pipe_pair_valid && product_valid) mul_busy <= mul_busy - 1'b1;

            if (product_valid && !accum_add_valid)      accum_add_busy <= accum_add_busy + 1'b1;
            else if (!product_valid && accum_add_valid) accum_add_busy <= accum_add_busy - 1'b1;

            if (reduce_issue && !reduce_add_valid)      reduce_add_busy <= reduce_add_busy + 1'b1;
            else if (!reduce_issue && reduce_add_valid) reduce_add_busy <= reduce_add_busy - 1'b1;
        end
    end


    /* ============================================================
     * 7. 累加狀態機與交棒
     * ============================================================
     *
     *   ACC_IDLE  等第一組運算元進到乘法器
     *   ACC_RUN   累加中
     *
     * 交棒(acc_handoff)要同時滿足四件事:
     *
     *   1. 輸入停了            !pipe_pair_valid
     *   2. 乘法器裡沒東西       mul_busy == 0
     *   3. 累加加法器裡沒東西   accum_add_busy == 0
     *   4. 另一組已經歸約完     red_state == RED_IDLE
     *
     * 第 2 條不能省:輸入結束的當下,第一個乘積可能都還沒從 fp_mul
     * 出來(k 很短的時候),這時 accum_add_busy 也是 0,看起來很乾淨,
     * 其實什麼都還沒算。
     *
     * 第 4 條是這個設計剩下的唯一序列化。它只有在「歸約比下一次
     * 累加還慢」的時候才會咬人 —— 也就是 k 小於約 63 的時候。
     * k 大的時候歸約早就做完了,這一條永遠成立。
     */

    typedef enum logic [0:0] {
        ACC_IDLE,
        ACC_RUN
    } acc_state_t;

    acc_state_t acc_state;

    wire acc_handoff = (acc_state == ACC_RUN)
                    && !pipe_pair_valid
                    && (mul_busy == 0)
                    && (accum_add_busy == 0)
                    && (red_state == RED_IDLE);

    always_ff @(posedge clk) begin
        if (rst) begin
            acc_state <= ACC_IDLE;
            acc_set   <= 1'b0;
        end
        else begin
            case (acc_state)
                ACC_IDLE:
                    if (pipe_pair_valid) acc_state <= ACC_RUN;

                ACC_RUN:
                    if (acc_handoff) begin
                        acc_state <= ACC_IDLE;
                        /* 翻到另一組。舊的那一組交給歸約 —— 下面的
                         * red_state 在同一拍把 red_set 記成舊的 acc_set,
                         * 非阻塞賦值,所以它讀到的是翻之前的值。 */
                        acc_set   <= ~acc_set;
                    end

                default: acc_state <= ACC_IDLE;
            endcase
        end
    end

    always_ff @(posedge clk) begin
        if (rst || acc_handoff) begin
            product_bank  <= '0;
            accum_wb_bank <= '0;
        end
        else begin
            if (product_valid)   product_bank  <= product_bank  + 1'b1;
            if (accum_add_valid) accum_wb_bank <= accum_wb_bank + 1'b1;
        end
    end


    /* ============================================================
     * 8. 歸約狀態機
     * ============================================================ */

    always_ff @(posedge clk) begin
        if (rst) begin
            red_state     <= RED_IDLE;
            red_set       <= 1'b0;
            reduce_stride <= '0;
            reduce_i      <= '0;
            reduce_wb_i   <= '0;
            reduce_todo   <= '0;
        end
        else begin

            /* 歸約的寫回索引。放在 case 之前,換層時 case 裡的歸零
             * 會蓋過它 —— 同一個 always 區塊裡後面的賦值贏。 */
            if (red_state == RED_RUN && reduce_add_valid)
                reduce_wb_i <= reduce_wb_i + 1'b1;

            case (red_state)

                RED_IDLE:
                    if (acc_handoff) begin
                        red_set       <= acc_set;      // 累加剛離開的那一組
                        reduce_stride <= ACC_SEL_W'(ACC_BANKS / 2);
                        reduce_todo   <= ACC_SEL_W'(ACC_BANKS / 2);
                        reduce_i      <= '0;
                        reduce_wb_i   <= '0;
                        red_state     <= RED_RUN;
                    end

                RED_RUN: begin
                    if (reduce_todo != 0) begin
                        /* 有事做:這一拍發一個加法,推進發射索引 */
                        reduce_todo <= reduce_todo - 1'b1;
                        reduce_i    <= reduce_i    + 1'b1;
                    end
                    else if (reduce_add_busy == 0) begin
                        /* 沒事做 + 管線空了 = 這一層全部落地。
                         * 這就是層間的 barrier —— 一個條件,不是一個狀態。 */
                        if (reduce_stride == ACC_SEL_W'(1)) begin
                            red_state <= RED_DONE;
                        end
                        else begin
                            reduce_stride <= reduce_stride >> 1;
                            reduce_todo   <= reduce_stride >> 1;  // 讀到舊的 stride
                            reduce_i      <= '0;
                            reduce_wb_i   <= '0;
                        end
                    end
                end

                RED_DONE: red_state <= RED_IDLE;   // 一拍就走

                default: red_state <= RED_IDLE;

            endcase
        end
    end


    /* ============================================================
     * 9. 寫回與清空
     * ============================================================
     *
     * 兩條寫回路徑各走各的,因為 acc_set 與 red_set 永遠不相等。
     *
     * 清空放在最後:同一個 always 區塊裡後面的賦值贏,所以歸約
     * 結束那一拍即使還有寫回,清空也會蓋過去 —— 那正是要的行為。
     * 一拍清完 16 格,for 在 always_ff 裡是展開不是迴圈。
     */

    always_ff @(posedge clk) begin
        if (rst) begin
            for (int s = 0; s < 2; s++)
                for (int i = 0; i < ACC_BANKS; i++)
                    acc_bank[s][i] <= '0;
        end
        else begin
            if (accum_add_valid)
                acc_bank[acc_set][accum_wb_bank] <= accum_add_result;

            if (reduce_add_valid)
                acc_bank[red_set][reduce_wb_i] <= reduce_add_result;

            if (red_state == RED_DONE)
                for (int i = 0; i < ACC_BANKS; i++)
                    acc_bank[red_set][i] <= '0;
        end
    end


    /* ============================================================
     * 10. 輸出
     * ============================================================
     *
     * 脈衝出現在 RED_DONE 的下一拍。同一拍清空也在清 acc_bank,
     * 但兩邊都是非阻塞賦值,右手邊取的是這一拍開始時的值 ——
     * 所以讀得到答案。
     */

    always_ff @(posedge clk) begin
        if (rst) begin
            acc_valid_out <= 1'b0;
            acc_out       <= '0;
        end
        else begin
            acc_valid_out <= (red_state == RED_DONE);

            if (red_state == RED_DONE)
                acc_out <= acc_bank[red_set][0];
        end
    end

endmodule