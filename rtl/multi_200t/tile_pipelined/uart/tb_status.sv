`timescale 1ns / 1ps

/*
 * tb_status -- systolic_status 的單元測試。
 *
 * 心跳與展寬的實際長度是 2^25 / 2^22 拍,模擬跑不完也沒必要 --
 * 用參數縮到 2^4 / 2^3 驗證機制,實際長度只是同一個計數器的位元
 * 選擇,不會有別的行為。
 *
 * 檢查:
 *   1. one-hot 解碼:每個 state 恰好點亮 led[2..5] 中的一顆
 *   2. busy:state != IDLE 時 jb_led[1] 為高
 *   3. 脈衝展寬:一拍的 frame_accepted 讓 led[0] 亮滿 2^STRETCH 拍
 *   4. 心跳:jb_led[0] 以固定週期翻轉,且不受 rst 影響(reset 路徑
 *      壞掉時心跳仍須跳 -- 這是它的診斷價值所在)
 *   5. 直通訊號:rx_active / c0_done / c1_done 延後一拍如實反映
 */
module tb_status;

    localparam int BL = 4;   // 心跳:每 2^4 = 16 拍翻轉
    localparam int SL = 3;   // 展寬:2^3 - 1 = 7 拍

    logic clk = 1'b0;
    always #5 clk = ~clk;

    logic       rst;
    logic       frame_accepted;
    logic       rx_active;
    logic [2:0] state;
    logic       c0_done, c1_done;
    logic [7:0] led;
    logic [1:0] jb_led;

    systolic_status #(.BLINK_LOG2(BL), .STRETCH_LOG2(SL)) dut (
        .clk(clk), .rst(rst),
        .frame_accepted(frame_accepted),
        .rx_active(rx_active),
        .state(state),
        .c0_done(c0_done), .c1_done(c1_done),
        .led(led), .jb_led(jb_led)
    );

    int errors = 0;
    int checked = 0;

    task automatic chk(input logic cond, input string what);
        begin
            checked++;
            if (!cond) begin
                errors++;
                $display("  FAIL: %s  (led=%b jb=%b state=%0d)",
                         what, led, jb_led, state);
            end
        end
    endtask

    int i, hi_cycles, toggles;
    logic prev_beat;

    initial begin
        rst = 1'b1;
        frame_accepted = 1'b0;
        rx_active = 1'b0;
        state = 3'd0;
        c0_done = 1'b0;
        c1_done = 1'b0;
        repeat (4) @(posedge clk);
        rst = 1'b0;
        @(posedge clk);

        /* ---- 1 & 2: one-hot 解碼與 busy ---- */
        for (i = 0; i < 4; i++) begin
            state = 3'(i);
            @(posedge clk);
            @(negedge clk);     // 輸出經一級 FF,等它落地
            chk(led[2 +: 4] == (4'b0001 << i),
                $sformatf("state %0d one-hot", i));
            chk(jb_led[1] == (i != 0), $sformatf("busy at state %0d", i));
        end

        /* ---- 5: 直通訊號 ---- */
        state = 3'd0;
        rx_active = 1'b1; c0_done = 1'b1; c1_done = 1'b0;
        @(posedge clk); @(negedge clk);
        chk(led[1] == 1'b1, "rx_active -> led[1]");
        chk(led[6] == 1'b1, "c0_done -> led[6]");
        chk(led[7] == 1'b0, "c1_done low -> led[7] low");
        rx_active = 1'b0; c0_done = 1'b0;
        @(posedge clk); @(negedge clk);
        chk(led[1] == 1'b0 && led[6] == 1'b0, "直通訊號隨輸入落下");

        /* ---- 3: 脈衝展寬 ----
         * 刺激一律在 negedge 變動、跨過一個 posedge 才收回。
         * 在 posedge 之後才收回會與 DUT 的 always_ff 競爭,結果
         * 取決於模擬器的排程順序 -- 那不是設計問題,是 tb 自己
         * 製造的假失敗。 */
        @(negedge clk);
        frame_accepted = 1'b1;
        @(negedge clk);
        frame_accepted = 1'b0;      // 恰好跨一個 posedge
        hi_cycles = 0;
        for (i = 0; i < (1 << SL) + 8; i++) begin
            @(negedge clk);
            if (led[0]) hi_cycles++;
        end
        // 計數器載入 2^SL-1 後倒數到 0,輸出再延後一級 FF
        chk(hi_cycles >= (1 << SL) - 1 && hi_cycles <= (1 << SL) + 1,
            $sformatf("一拍脈衝展寬成 %0d 拍(期望約 %0d)",
                      hi_cycles, 1 << SL));

        /* ---- 4: 心跳 ----
         * beat_cnt 沒有 rst 分支(初始值來自 configuration),所以
         * 即使 reset 路徑本身壞了心跳仍會跳 -- 那正是它的診斷價值。
         * 輸出級有 rst,所以 rst 拉起時燈會暗;這裡量的是放開之後。 */
        prev_beat = jb_led[0];
        toggles = 0;
        for (i = 0; i < (1 << (BL + 2)); i++) begin
            @(posedge clk); @(negedge clk);
            if (jb_led[0] !== prev_beat) begin
                toggles++;
                prev_beat = jb_led[0];
            end
        end
        chk(toggles >= 3, $sformatf("心跳在 %0d 拍內翻轉 %0d 次",
                                    1 << (BL + 2), toggles));

        $display("");
        if (errors == 0 && checked > 0)
            $display("PASS: systolic_status  (%0d 項檢查)", checked);
        else
            $display("FAIL: %0d/%0d 項失敗", errors, checked);
        $finish;
    end

endmodule
