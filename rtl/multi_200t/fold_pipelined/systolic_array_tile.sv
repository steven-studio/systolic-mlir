/*
 * ============================================================
 * systolic_array_tile -- N x N 的 PE 陣列
 * ============================================================
 *
 * a 從左邊推進去,b 從上面推進去,在 N x N 個 PE 裡交會相乘累加。
 * 每一格都算完之後,把整片 N x N 的結果發佈出去。
 *
 *        b_in[0] b_in[1] ...
 *           │       │
 *           ▼       ▼
 * a_in[0]─►PE(0,0)─►PE(0,1)─► ...
 *           │       │
 *           ▼       ▼
 * a_in[1]─►PE(1,0)─►PE(1,1)─► ...
 *           │       │
 *           ▼       ▼
 *
 *
 * 這一版拿掉了什麼
 * ----------------
 *   兩組 accum_ctx 輸入埠、c_ctx_out 輸出埠
 *   兩條 ctx 匯流排與每個 PE 的 ctx 接線
 *   第二份結果矩陣、輸出選擇的 always_comb
 *   四個輸出狀態變兩個
 *
 * 對外唯一的協定改變:一次交易發佈一片矩陣,不是兩片。
 * TX 因此從 8*N*N bytes 變成 4*N*N bytes。
 *
 *
 * 編譯器抓不到的一件事
 * --------------------
 * 舊 PE 的 result_valid 持續拉高到下一次交易;
 * 新 PE 的 acc_valid_out 是一拍脈衝。
 *
 * 這是語意改變,不是接線改變。下面 (A)(B) 兩段是它的後果。
 * ============================================================
 */

module systolic_array_tile #(
    /*
     * 陣列邊長。陣列是正方形:N 列 x N 行,共 N*N 個 PE。
     *
     * host 送幾個 k、怎麼切,對這個檔案完全不可見 —— 它只看到
     * 運算元一拍一拍流進來,以及每個 PE 說「我算完了」。
     */
    parameter int N = 8,

    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    /* 左邊界(a)與上邊界(b)的運算元 */
    input  logic [DATA_W-1:0] a_in [0:N-1],
    input  logic [DATA_W-1:0] b_in [0:N-1],

    input  logic a_valid_in [0:N-1],
    input  logic b_valid_in [0:N-1],

    /* 整片結果。c_valid_out 是一拍脈衝 */
    output logic              c_valid_out,
    output logic [DATA_W-1:0] c_out [0:N-1][0:N-1]
);


    /*
     * ============================================================
     * 運算元匯流排
     *
     * a 往右走,所以行的索引多一格(0..N);b 往下走,列多一格。
     * 多出來的那一格是陣列右邊 / 下面的出口,沒有人接。
     * ============================================================
     */

    logic [DATA_W-1:0] a_bus [0:N-1][0:N];
    logic [DATA_W-1:0] b_bus [0:N][0:N-1];

    logic a_valid_bus [0:N-1][0:N];
    logic b_valid_bus [0:N][0:N-1];


    /* 每個 PE 的最終結果。一格一個純量,沒有 context。 */
    logic [DATA_W-1:0] pe_acc       [0:N-1][0:N-1];
    logic              pe_acc_valid [0:N-1][0:N-1];



    genvar r;
    genvar c;


    /* ---------- 邊界:輸入接到第 0 行 / 第 0 列 ---------- */

    generate

        for (r = 0; r < N; r = r + 1) begin : INIT_A
            assign a_bus[r][0]       = a_in[r];
            assign a_valid_bus[r][0] = a_valid_in[r];
        end

        for (c = 0; c < N; c = c + 1) begin : INIT_B
            assign b_bus[0][c]       = b_in[c];
            assign b_valid_bus[0][c] = b_valid_in[c];
        end

    endgenerate


    /* ---------- N x N 陣列本體 ---------- */

    generate

        for (r = 0; r < N; r = r + 1) begin : ROW
            for (c = 0; c < N; c = c + 1) begin : COL

                systolic_pe_tile #(
                    .DATA_W (DATA_W)
                ) u_pe (
                    .clk           (clk),
                    .rst           (rst),

                    .a_valid_in    (a_valid_bus[r][c]),
                    .b_valid_in    (b_valid_bus[r][c]),
                    .a_in          (a_bus[r][c]),
                    .b_in          (b_bus[r][c]),

                    .a_valid_out   (a_valid_bus[r][c+1]),
                    .b_valid_out   (b_valid_bus[r+1][c]),
                    .a_out         (a_bus[r][c+1]),
                    .b_out         (b_bus[r+1][c]),

                    .acc_valid_out (pe_acc_valid[r][c]),
                    .acc_out       (pe_acc[r][c])
                );

            end
        end

    endgenerate


    /*
     * ============================================================
     * (A) 逐格的「已抵達」旗標
     *
     * acc_valid_out 是一拍脈衝,一定要鎖存 —— 錯過那一拍就再也
     * 讀不到了。舊版 result_valid 持續拉高,那時候這個旗標只是
     * 方便,現在它是必要的。
     *
     * 優先權:脈衝 > 清除。
     *   脈衝只有一拍,錯過就沒了;清除隨時可以再做。
     *   如果清除贏,而某個 PE 剛好同拍完成,那一格的結果會
     *   靜靜地掉,而且不會有任何錯誤訊息。
     * ============================================================
     */

    logic acc_arrived [0:N-1][0:N-1];
    logic clear_arrived;

    always_ff @(posedge clk) begin
        for (int rr = 0; rr < N; rr = rr + 1) begin
            for (int cc = 0; cc < N; cc = cc + 1) begin
                if (rst)                       acc_arrived[rr][cc] <= 1'b0;
                else if (pe_acc_valid[rr][cc]) acc_arrived[rr][cc] <= 1'b1;
                else if (clear_arrived)        acc_arrived[rr][cc] <= 1'b0;
            end
        end
    end


    /* 每一格都到齊了嗎 */
    logic all_arrived;

    always_comb begin
        all_arrived = 1'b1;
        for (int rr = 0; rr < N; rr = rr + 1)
            for (int cc = 0; cc < N; cc = cc + 1)
                all_arrived = all_arrived && acc_arrived[rr][cc];
    end


    /*
     * ============================================================
     * 結果矩陣
     *
     * 直接接 PE 的 acc_out,不另外抄一份暫存器。
     *
     * PE 的 acc_out 只在它自己的 PE_DONE 那一拍更新,之後一直
     * 抱著 —— 也就是 PE 已經鎖存過了。陣列再抄一份是重複付錢
     * (N*N*DATA_W 個 FF,N=8 時 2048 個)。
     *
     * 這是一條跨模組的約定,編譯器不檢查,所以 tb_array_pulse
     * 有一項專門驗它:脈衝過後晾著,c_out 不能變。
     *
     * 交易進行中 c_out 會隨各格陸續完成而逐格變動(每個 PE 完成
     * 的時間差了 r+c 拍)。只有 c_valid_out 拉高那一拍,整片才
     * 保證是對的。
     * ============================================================
     */

    generate
        for (r = 0; r < N; r = r + 1) begin : OUT_ROW
            for (c = 0; c < N; c = c + 1) begin : OUT_COL
                assign c_out[r][c] = pe_acc[r][c];
            end
        end
    endgenerate


    /*
     * ============================================================
     * 輸出控制器
     *
     *   OUT_WAIT         還有格子沒到齊
     *   OUT_CAN_PUBLISH  全部到齊了,可以發
     *
     * 名字是「可以發」而不是「發」:在這個狀態裡 c_valid_out 還是 0,
     * 脈衝要下一拍才出現。這跟 PE 裡 acc_valid_out <= (state == PE_DONE)
     * 是同一個寫法 —— 狀態是條件,脈衝是它的下一拍。
     *
     * 這兩個狀態之間沒有任何重疊:每一格到齊之前不發佈,發佈的
     * 同一拍把旗標清空,下一次交易從頭來過。整個模組沒有一處是
     * 兩件事同時在跑的。
     * ============================================================
     */


    typedef enum logic [0:0] {
        OUT_WAIT,
        OUT_CAN_PUBLISH
    } out_state_t;

    out_state_t out_state;


    /*
     * (B) 清除訊號必須是組合邏輯,不能是暫存的。
     *
     * 寫成暫存的話清除會晚一拍生效。那一拍裡 all_arrived 還是 1,
     * 而狀態已經回到 OUT_WAIT —— 同一組結果會被發佈兩次。
     */
    assign clear_arrived = (out_state == OUT_CAN_PUBLISH);


    always_ff @(posedge clk) begin

        if (rst) begin
            c_valid_out <= 1'b0;
            out_state   <= OUT_WAIT;
        end
        else begin

            /* 一拍脈衝,出現在「可以發」的下一拍 */
            c_valid_out <= (out_state == OUT_CAN_PUBLISH);

            case (out_state)
                OUT_WAIT:        if (all_arrived) out_state <= OUT_CAN_PUBLISH;
                OUT_CAN_PUBLISH:                  out_state <= OUT_WAIT;
                default:                          out_state <= OUT_WAIT;
            endcase

        end

    end


endmodule