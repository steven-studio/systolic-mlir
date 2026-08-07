#!/usr/bin/env python3
"""Emit a stub array + self-checking testbench for matmul_top_rk_fold.

    python3 gen_fold_tb_new.py            # writes stub + tb into ./

WHAT THIS TESTS, AND WHY IT USES A STUB

Everything new in matmul_top_rk_fold.v is control: header parsing, the
word/bank/fold fill counters, fold iteration, C-buffer addressing, and the
order blocks come back in. None of it needs a real floating-point array, and
pulling one in would mean dragging the Xilinx FP IP netlists and unisims into
the simulation for no diagnostic value -- a failure there tells you nothing
about the FSM.

So matmul_8x8x8 is replaced by a stub with the same port list that computes

    Cinout[i][j]_o = Cinout[i][j]_i + A[fold_i][bank i][word 0]
                                    + B[fold_j][bank j][word 0]

in plain integer arithmetic. That single expression is enough to catch every
addressing mistake the rewrite could plausibly make:

  * C_init reaching the wrong fold        -> wrong base term
  * fold_i not reaching the A RAMs        -> A term identical across fi
  * fold_j not reaching the B RAMs        -> B term identical across fj
  * result stored to the wrong fold       -> blocks swapped on the wire
  * blocks transmitted in the wrong order -> same, detected positionally
  * the S_LOAD_A/S_LOAD_D off-by-one      -> C_init lags one fold behind

The stub reads through the real ap_memory ports with the real one-cycle
latency, so the {fold, word} concatenation in the iface is exercised for
real. It ties address0 to 0 because word 0 is all the check needs; the rest
of the reduction is what the actual array is for, and that path is already
validated bit-exactly on hardware.

WHAT THIS DOES NOT TEST

Arithmetic. The stub adds integers. Numerical correctness of the systolic
array is unchanged by this work -- matmul_8x8x8 is byte-identical to the
version in the known-good bitstream -- and is covered by the existing
hardware vectors. Run those, at Mt=Nt=1, before trusting multi-fold.

TEST VECTORS

Values are chosen so that every term is separable by eye when a case fails:

    A[fi][bank][word]     = 1000*fi + 10*bank + word
    B[fj][bank][word]     = 2000*fj + 20*bank + word
    C_init[fi][fj][i][j]  = 100000 + 10000*fi + 1000*fj + 100*i + j

so an expected value of 123456 that comes back as 122456 says "fi was one
too low" rather than just "mismatch".
"""

import sys

N = 8
KMAX = 64


def a_val(fi, bank, word):
    return 1000 * fi + 10 * bank + word


def b_val(fj, bank, word):
    return 2000 * fj + 20 * bank + word


def c_val(fi, fj, i, j):
    return 100000 + 10000 * fi + 1000 * fj + 100 * i + j


def expected(fi, fj, i, j):
    return c_val(fi, fj, i, j) + a_val(fi, i, 0) + b_val(fj, j, 0)


# --------------------------------------------------------------------------
def gen_stub():
    L = []
    a = L.append
    a("// matmul_8x8x8 -- SIMULATION STUB. Not for synthesis.")
    a("//")
    a("// Same port list as the HLS core, integer arithmetic instead of fp32:")
    a("//   Cinout[i][j]_o = Cinout[i][j]_i + A_i_q0 + B_j_q0")
    a("// with address0 tied to 0, so the operand terms are word 0 of the")
    a("// currently selected fold. Latency mimics the real core (K + 14 + 11)")
    a("// so the controller's ap_start/ap_done handshake sees realistic")
    a("// timing rather than a combinational done.")
    a("`timescale 1ns / 1ps")
    a("module matmul_8x8x8 (")
    a("    input  wire        ap_clk,")
    a("    input  wire        ap_rst,")
    a("    input  wire        ap_start,")
    a("    output reg         ap_done,")
    a("    output wire        ap_idle,")
    a("    output wire        ap_ready,")
    a("    input  wire [31:0] K,")
    for tag in ("A", "B"):
        for i in range(N):
            a(f"    output wire [5:0]  {tag}_{i}_address0,")
            a(f"    output wire        {tag}_{i}_ce0,")
            a(f"    input  wire [31:0] {tag}_{i}_q0,")
    ports = []
    for i in range(N):
        for j in range(N):
            ports.append(f"    input  wire [31:0] Cinout_{i}_{j}_i")
            ports.append(f"    output wire [31:0] Cinout_{i}_{j}_o")
            ports.append(f"    output wire        Cinout_{i}_{j}_o_ap_vld")
    a(",\n".join(ports))
    a(");")
    a("")
    for tag in ("A", "B"):
        for i in range(N):
            a(f"    assign {tag}_{i}_address0 = 6'd0;")
            a(f"    assign {tag}_{i}_ce0      = 1'b1;")
    a("")
    a("    reg  [31:0] cnt;")
    a("    reg         busy;")
    a("    assign ap_idle  = ~busy;")
    a("    assign ap_ready = ap_done;")
    a("")
    a("    always @(posedge ap_clk) begin")
    a("        if (ap_rst) begin")
    a("            busy <= 0; cnt <= 0; ap_done <= 0;")
    a("        end else begin")
    a("            ap_done <= 0;")
    a("            if (!busy && ap_start) begin")
    a("                busy <= 1;")
    a("                cnt  <= K + 32'd25;      // K + (R+C-2) + c0")
    a("            end else if (busy) begin")
    a("                if (cnt == 0) begin")
    a("                    busy <= 0; ap_done <= 1;")
    a("                end else begin")
    a("                    cnt <= cnt - 1;")
    a("                end")
    a("            end")
    a("        end")
    a("    end")
    a("")
    for i in range(N):
        for j in range(N):
            a(f"    assign Cinout_{i}_{j}_o = Cinout_{i}_{j}_i "
              f"+ A_{i}_q0 + B_{j}_q0;")
            a(f"    assign Cinout_{i}_{j}_o_ap_vld = ap_done;")
    a("")
    a("endmodule")
    return "\n".join(L) + "\n"


# --------------------------------------------------------------------------
def gen_tb(k, mt, nt):
    nblk = mt * nt
    L = []
    a = L.append
    a("// tb_fold_new.v -- self-checking testbench for matmul_top_rk_fold.")
    a("//")
    a(f"// Case under test: K={k}, Mt={mt}, Nt={nt}  ({nblk} fold"
      f"{'s' if nblk > 1 else ''})")
    a("//")
    a("// Drives the UART line at 2 Mbaud from its own time base -- 500 ns per")
    a("// bit -- so the test does not depend on the DUT's internal clock, and")
    a("// a wrong CLKS_PER_BIT would show up as a framing failure rather than")
    a("// silently passing.")
    a("//")
    a("// Requires the MMCM behavioural model:")
    a("//     xelab tb_fold -s tb -L unisims_ver")
    a("`timescale 1ns / 1ps")
    a("module tb_fold;")
    a("")
    a("    localparam real BIT_NS = 500.0;   // 2 Mbaud")
    a(f"    localparam integer KV  = {k};")
    a(f"    localparam integer MT  = {mt};")
    a(f"    localparam integer NT  = {nt};")
    a("")
    a("    reg  clk100 = 0;")
    a("    reg  rst    = 1;")
    a("    reg  uart_rx = 1;")
    a("    wire uart_tx;")
    a("    wire led;")
    a("")
    a("    always #5 clk100 = ~clk100;       // 100 MHz")
    a("")
    a("    matmul_top_rk_fold dut (")
    a("        .clk_pin100(clk100), .btn_rst(rst),")
    a("        .uart_rx_pin(uart_rx), .uart_tx_pin(uart_tx),")
    a("        .led_done(led));")
    a("")
    a("    // ---- byte / word transmit ------------------------------------")
    a("    task send_byte(input [7:0] b);")
    a("        integer bi;")
    a("        begin")
    a("            uart_rx = 0;  #(BIT_NS);                 // start")
    a("            for (bi = 0; bi < 8; bi = bi + 1) begin")
    a("                uart_rx = b[bi];  #(BIT_NS);         // LSB first")
    a("            end")
    a("            uart_rx = 1;  #(BIT_NS);                 // stop")
    a("        end")
    a("    endtask")
    a("")
    a("    task send_word(input [31:0] w);")
    a("        begin")
    a("            send_byte(w[7:0]);   send_byte(w[15:8]);")
    a("            send_byte(w[23:16]); send_byte(w[31:24]);")
    a("        end")
    a("    endtask")
    a("")
    a("    // ---- byte receive --------------------------------------------")
    a("    task recv_byte(output [7:0] b);")
    a("        integer bi;")
    a("        begin")
    a("            @(negedge uart_tx);")
    a("            #(BIT_NS * 1.5);                          // mid of bit 0")
    a("            for (bi = 0; bi < 8; bi = bi + 1) begin")
    a("                b[bi] = uart_tx;  #(BIT_NS);")
    a("            end")
    a("        end")
    a("    endtask")
    a("")
    a("    integer fi, fj, bk, wd, i, j, errors;")
    a("    reg [7:0]  rb;")
    a("    reg [31:0] got, want;")
    a("")
    a("    // Same value functions as the generator -- kept as expressions so a")
    a("    // failing case shows which term is off.")
    a("    function [31:0] a_val(input integer f, input integer b_,"
      " input integer w);")
    a("        a_val = 1000*f + 10*b_ + w;")
    a("    endfunction")
    a("    function [31:0] b_val(input integer f, input integer b_,"
      " input integer w);")
    a("        b_val = 2000*f + 20*b_ + w;")
    a("    endfunction")
    a("    function [31:0] c_val(input integer f1, input integer f2,")
    a("                          input integer r,  input integer c);")
    a("        c_val = 100000 + 10000*f1 + 1000*f2 + 100*r + c;")
    a("    endfunction")
    a("")
    a("    initial begin")
    a("        errors = 0;")
    a("        repeat (2000) @(posedge clk100);   // MMCM lock")
    a("        rst = 0;")
    a("        repeat (100) @(posedge clk100);")
    a("")
    a("        send_byte(8'h00);                  // device 0")
    a("        send_word(KV);")
    a("        send_word(MT);")
    a("        send_word(NT);")
    a("")
    a("        for (fi = 0; fi < MT; fi = fi + 1)")
    a("          for (bk = 0; bk < 8; bk = bk + 1)")
    a("            for (wd = 0; wd < KV; wd = wd + 1)")
    a("              send_word(a_val(fi, bk, wd));")
    a("")
    a("        for (fj = 0; fj < NT; fj = fj + 1)")
    a("          for (bk = 0; bk < 8; bk = bk + 1)")
    a("            for (wd = 0; wd < KV; wd = wd + 1)")
    a("              send_word(b_val(fj, bk, wd));")
    a("")
    a("        for (fi = 0; fi < MT; fi = fi + 1)")
    a("          for (fj = 0; fj < NT; fj = fj + 1)")
    a("            for (i = 0; i < 8; i = i + 1)")
    a("              for (j = 0; j < 8; j = j + 1)")
    a("                send_word(c_val(fi, fj, i, j));")
    a("")
    a("        $display(\"[tb] payload sent, waiting for %0d result blocks\","
      " MT*NT);")
    a("")
    a("        for (fi = 0; fi < MT; fi = fi + 1)")
    a("          for (fj = 0; fj < NT; fj = fj + 1)")
    a("            for (i = 0; i < 8; i = i + 1)")
    a("              for (j = 0; j < 8; j = j + 1) begin")
    a("                recv_byte(rb); got[7:0]   = rb;")
    a("                recv_byte(rb); got[15:8]  = rb;")
    a("                recv_byte(rb); got[23:16] = rb;")
    a("                recv_byte(rb); got[31:24] = rb;")
    a("                want = c_val(fi,fj,i,j) + a_val(fi,i,0) + b_val(fj,j,0);")
    a("                if (got !== want) begin")
    a("                    if (errors < 10)")
    a("                        $display(\"[tb] MISMATCH fold(%0d,%0d) "
      "elem(%0d,%0d): got %0d want %0d\", fi, fj, i, j, got, want);")
    a("                    errors = errors + 1;")
    a("                end")
    a("              end")
    a("")
    a("        if (errors == 0)")
    a("            $display(\"[tb] PASS -- %0d blocks, %0d elements, all"
      " exact\", MT*NT, MT*NT*64);")
    a("        else")
    a("            $display(\"[tb] FAIL -- %0d/%0d elements wrong\","
      " errors, MT*NT*64);")
    a("        $finish;")
    a("    end")
    a("")
    a("    initial begin")
    a("        #200_000_000;")
    a("        $display(\"[tb] TIMEOUT -- FSM stuck, dut.state = %0d\","
      " dut.state);")
    a("        $finish;")
    a("    end")
    a("")
    a("endmodule")
    return "\n".join(L) + "\n"


if __name__ == "__main__":
    k = int(sys.argv[1]) if len(sys.argv) > 1 else 2
    mt = int(sys.argv[2]) if len(sys.argv) > 2 else 2
    nt = int(sys.argv[3]) if len(sys.argv) > 3 else 2

    open("matmul_8x8x8_stub.v", "w").write(gen_stub())
    open("tb_fold_new.v", "w").write(gen_tb(k, mt, nt))

    nblk = mt * nt
    tx = 1 + 12 + mt * 8 * k * 4 + nt * 8 * k * 4 + nblk * 256
    print(f"wrote matmul_8x8x8_stub.v, tb_fold_new.v   (K={k}, Mt={mt}, Nt={nt})")
    print(f"  {tx} bytes out, {nblk*256} back, "
          f"~{(tx + nblk*256) * 10 * 0.5 / 1000:.1f} ms of simulated time")
    print()
    print("  xvlog matmul_top_rk_fold_new.v matmul_iface_rk_fold.v \\")
    print("        uart_rx.v uart_tx.v clk_gen.v \\")
    print("        matmul_8x8x8_stub.v tb_fold_new.v")
    print("  xelab tb_fold -s tb -L unisims_ver")
    print("  xsim tb -R")