// tb_fold_new.v -- self-checking testbench for matmul_top_rk_fold.
//
// Case under test: K=64, Mt=2, Nt=2  (4 folds)
//
// Drives the UART line at 2 Mbaud from its own time base -- 500 ns per
// bit -- so the test does not depend on the DUT's internal clock, and
// a wrong CLKS_PER_BIT would show up as a framing failure rather than
// silently passing.
//
// Requires the MMCM behavioural model:
//     xelab tb_fold -s tb -L unisims_ver
`timescale 1ns / 1ps
module tb_fold;

    localparam real BIT_NS = 500.0;   // 2 Mbaud
    localparam integer KV  = 64;
    localparam integer MT  = 2;
    localparam integer NT  = 2;

    reg  clk100 = 0;
    reg  rst    = 1;
    reg  uart_rx = 1;
    wire uart_tx;
    wire led;

    always #5 clk100 = ~clk100;       // 100 MHz

    matmul_top_rk_fold dut (
        .clk_pin100(clk100), .btn_rst(rst),
        .uart_rx_pin(uart_rx), .uart_tx_pin(uart_tx),
        .led_done(led));

    // ---- byte / word transmit ------------------------------------
    task send_byte(input [7:0] b);
        integer bi;
        begin
            uart_rx = 0;  #(BIT_NS);                 // start
            for (bi = 0; bi < 8; bi = bi + 1) begin
                uart_rx = b[bi];  #(BIT_NS);         // LSB first
            end
            uart_rx = 1;  #(BIT_NS);                 // stop
        end
    endtask

    task send_word(input [31:0] w);
        begin
            send_byte(w[7:0]);   send_byte(w[15:8]);
            send_byte(w[23:16]); send_byte(w[31:24]);
        end
    endtask

    // ---- byte receive --------------------------------------------
    task recv_byte(output [7:0] b);
        integer bi;
        begin
            @(negedge uart_tx);
            #(BIT_NS * 1.5);                          // mid of bit 0
            for (bi = 0; bi < 8; bi = bi + 1) begin
                b[bi] = uart_tx;  #(BIT_NS);
            end
        end
    endtask

    integer fi, fj, bk, wd, i, j, errors;
    reg [7:0]  rb;
    reg [31:0] got, want, cyc;

    // Same value functions as the generator -- kept as expressions so a
    // failing case shows which term is off.
    function [31:0] a_val(input integer f, input integer b_, input integer w);
        a_val = 1000*f + 10*b_ + w;
    endfunction
    function [31:0] b_val(input integer f, input integer b_, input integer w);
        b_val = 2000*f + 20*b_ + w;
    endfunction
    function [31:0] c_val(input integer f1, input integer f2,
                          input integer r,  input integer c);
        c_val = 100000 + 10000*f1 + 1000*f2 + 100*r + c;
    endfunction

    initial begin
        errors = 0;
        repeat (2000) @(posedge clk100);   // MMCM lock
        rst = 0;
        repeat (100) @(posedge clk100);

        send_byte(8'h00);                  // device 0
        send_word(KV);
        send_word(MT);
        send_word(NT);

        for (fi = 0; fi < MT; fi = fi + 1)
          for (bk = 0; bk < 8; bk = bk + 1)
            for (wd = 0; wd < KV; wd = wd + 1)
              send_word(a_val(fi, bk, wd));

        for (fj = 0; fj < NT; fj = fj + 1)
          for (bk = 0; bk < 8; bk = bk + 1)
            for (wd = 0; wd < KV; wd = wd + 1)
              send_word(b_val(fj, bk, wd));

        for (fi = 0; fi < MT; fi = fi + 1)
          for (fj = 0; fj < NT; fj = fj + 1)
            for (i = 0; i < 8; i = i + 1)
              for (j = 0; j < 8; j = j + 1)
                send_word(c_val(fi, fj, i, j));

        $display("[tb] payload sent, waiting for %0d result blocks", MT*NT);

        for (fi = 0; fi < MT; fi = fi + 1)
          for (fj = 0; fj < NT; fj = fj + 1)
            for (i = 0; i < 8; i = i + 1)
              for (j = 0; j < 8; j = j + 1) begin
                recv_byte(rb); got[7:0]   = rb;
                recv_byte(rb); got[15:8]  = rb;
                recv_byte(rb); got[23:16] = rb;
                recv_byte(rb); got[31:24] = rb;
                want = c_val(fi,fj,i,j) + a_val(fi,i,0) + b_val(fj,j,0);
                if (got !== want) begin
                    if (errors < 10)
                        $display("[tb] MISMATCH fold(%0d,%0d) elem(%0d,%0d): got %0d want %0d", fi, fj, i, j, got, want);
                    errors = errors + 1;
                end
              end


        // trailing 4-byte hardware cycle count
        recv_byte(rb); cyc[7:0]   = rb;
        recv_byte(rb); cyc[15:8]  = rb;
        recv_byte(rb); cyc[23:16] = rb;
        recv_byte(rb); cyc[31:24] = rb;

        $display("[tb] cycles = %0d for %0d fold(s), K=%0d  -> %0d per fold", cyc, MT*NT, KV, cyc / (MT*NT));
        // Loose bounds only. The exact figure depends on the stub's
        // latency, and pinning it here would make the test brittle
        // without saying anything about the real array. What matters
        // is the slope across runs, which is for the human to read.
        if (cyc < MT*NT*KV) begin
            $display("[tb] IMPLAUSIBLE -- fewer cycles than reduction steps");
            errors = errors + 1;
        end
        if (cyc > MT*NT*(KV+200)) begin
            $display("[tb] IMPLAUSIBLE -- far more cycles than one invocation per fold");
            errors = errors + 1;
        end

        if (errors == 0)
            $display("[tb] PASS -- %0d blocks, %0d elements, all exact", MT*NT, MT*NT*64);
        else
            $display("[tb] FAIL -- %0d problem(s)", errors);
        $finish;
    end

    initial begin
        #200_000_000;
        $display("[tb] TIMEOUT -- FSM stuck, dut.state = %0d", dut.state);
        $finish;
    end

endmodule
