`timescale 1ns/1ps
module tb_led;
    logic clk = 0;
    always #5 clk = ~clk;
    logic [1:0] jb_led, led;
    led_test dut (.clk(clk), .jb_led(jb_led), .led(led));

    integer toggles = 0;
    logic   prev;
    integer i;
    initial begin
        @(posedge clk);
        prev = jb_led[1];
        for (i = 0; i < 90_000_000; i = i + 1) begin
            @(posedge clk);
            if (jb_led[0] !== 1'b1 || led[0] !== 1'b1)
                $fatal(1, "solid LED not high at cycle %0d", i);
            if (jb_led[1] !== led[1])
                $fatal(1, "external and on-board heartbeat disagree");
            if (jb_led[1] !== prev) begin
                toggles = toggles + 1;
                prev = jb_led[1];
                $display("  heartbeat toggle %0d at cycle %0d (%.3f s)",
                         toggles, i, i / 100.0e6);
            end
        end
        if (toggles >= 2) $display("PASS: solid stays high, heartbeat toggles %0d times", toggles);
        else              $display("FAIL: only %0d toggles", toggles);
        $finish;
    end
endmodule
