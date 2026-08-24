`timescale 1ns/1ps
/*
 * tb_pe_overlap -- 兩個 tile 之間最短要隔多久?
 *
 * 乒乓 bank + 第二顆加法器的目的,是讓 tile T 的歸約跟 tile T+1
 * 的累加重疊。這支測試量的就是「重疊了多少」:
 *
 *   餵 tile 0 → 空 GAP 拍 → 餵 tile 1 → 兩個結果都要對
 *
 * GAP 從小往大掃,第一個會過的值就是這個設計的最短間隔。
 *
 *   乒乓之前:歸約佔著那 16 格,GAP 必須大於 排空(21) + 歸約(63)
 *   乒乓之後:兩組 bank 各做各的,GAP 只需要大於 排空(21)
 *
 * 21 這一段拿不掉,因為 PE 是靠「輸入停了而且管線排空」判斷交易
 * 結束的。要連它也拿掉,就得給每筆交易一個標籤,用「標籤變了」當邊界。
 *
 * GAP 太小的時候,PE 分不出兩個 tile 的邊界,會把它們當成同一次
 * 交易加在一起 —— 症狀是只有一個脈衝、值是 136+392。硬體沒有防線,
 * 這支測試就是那條約束唯一的守門員。
 *
 *   verilator --binary -Wall -Wno-fatal --top-module tb_pe_overlap \
 *       -GGAP=21 fp_model.sv systolic_pe_tile.sv tb_pe_overlap.sv
 */
module tb_pe_overlap;

    parameter int GAP = 22;      // 兩個 tile 之間空幾拍(21 是實測的最小值)

    logic clk = 0;
    always #5 clk = ~clk;

    logic rst;
    logic        av, bv;
    logic [31:0] a, b;
    logic        avo, bvo;
    logic [31:0] ao, bo;
    logic        accv;
    logic [31:0] acco;

    systolic_pe dut (
        .clk(clk), .rst(rst),
        .a_valid_in(av), .b_valid_in(bv), .a_in(a), .b_in(b),
        .a_valid_out(avo), .b_valid_out(bvo), .a_out(ao), .b_out(bo),
        .acc_valid_out(accv), .acc_out(acco)
    );

    /* ---------- 收集脈衝 ---------- */
    int cyc = 0;
    int n_pulse = 0;
    int got_val  [0:3];
    int got_cyc  [0:3];

    always @(posedge clk) begin
        if (!rst) begin
            cyc <= cyc + 1;
            if (accv) begin
                if (n_pulse < 4) begin
                    got_val[n_pulse] <= int'(acco);
                    got_cyc[n_pulse] <= cyc;
                end
                n_pulse <= n_pulse + 1;
            end
        end
    end

    int errors = 0;
    int checks = 0;
    task automatic chk(input string what, input int got, input int want);
        checks++;
        if (got !== want) begin
            errors++;
            $display("  [FAIL] %-30s got %0d  want %0d", what, got, want);
        end
    endtask

    /* 餵一個 tile:a 全是 1,b 是 base+1 .. base+16
     * 總和 = 16*base + 136 */
    task automatic feed_tile(input int base);
        for (int t = 0; t < 16; t++) begin
            @(negedge clk);
            av = 1; bv = 1;
            a  = 32'd1;
            b  = 32'(base + t + 1);
        end
        @(negedge clk);
        av = 0; bv = 0; a = 0; b = 0;
    endtask

    initial begin
        rst = 1; av = 0; bv = 0; a = 0; b = 0;
        repeat (4) @(negedge clk);
        rst = 0;

        feed_tile(0);               // 1..16   -> 136
        repeat (GAP) @(negedge clk);
        feed_tile(16);              // 17..32  -> 392

        repeat (400) @(negedge clk);

        $display("GAP = %0d", GAP);
        chk("脈衝數", n_pulse, 2);

        if (n_pulse >= 2) begin
            chk("tile0 結果", got_val[0], 136);
            chk("tile1 結果", got_val[1], 392);
            $display("  脈衝落在 cycle %0d 與 %0d(相距 %0d 拍)",
                     got_cyc[0], got_cyc[1], got_cyc[1] - got_cyc[0]);
        end

        if (errors == 0) $display("PASS  GAP=%0d  (%0d 項)", GAP, checks);
        else             $display("FAIL  GAP=%0d  (%0d / %0d 項不符)", GAP, errors, checks);
        $finish;
    end

endmodule
