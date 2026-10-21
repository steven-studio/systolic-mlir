`default_nettype none
`timescale 1ns / 1ps

// ============================================================================
// tb_dpti_sync245_phy.sv -- standalone bench for dpti_sync245_phy.
//
// Does NOT instantiate systolic_dma_top.  Compile with +define+LEGACY to put
// the existing dpti_byte_rx + dpti_byte_tx (via tb/dpti_legacy_phy_wrap.sv,
// both unmodified) behind the same pins instead.
//
// Run: tb/run_dpti_phy_sim.sh (commands and the scenario matrix are there).
//
// Board / FPGA timing model.  All [ASM] unless noted; plusargs, ns.
//   TC     FT2232H CLKOUT pin -> FPGA clock pin trace
//   TD     signal trace, both directions, every signal
//   T_INS  FPGA clock pin -> internal clock edge (MMCM-compensated: ~0)
//   CLK_TRACK 0: internal clock = ideal n*T + TC + T_INS (an MMCM filters the
//            chip's edge jitter); 1: internal clock = CLKOUT delayed by
//            TC + T_INS, jitter included (a BUFG, as today)
//   D_IN   FPGA input pin -> capture register
//   D_RD / D_OE / D_WR / D_DAT  register (or, for the legacy combinational
//            outputs, register input) -> FPGA output pin, per port; T_CO sets
//            all four
//   TSU_F / TH_F  capture-register setup / hold, checked at the register input
//            against the internal clock edge (CHK_FPGA_IN=1)
//
// Checks (each counted; any non-zero count is a FAIL):
//   chip side (ft2232h_sync245_model): OE#/RD#/WR#/write-DATA setup [DS],
//     RD# without OE# >= 1 clock before, read and write at once, WR# with X on
//     D, bus contention at the chip pins.
//   bench side: contention at the FPGA pins, FPGA input-register window, RX and
//     TX scoreboards (exact byte sequence: no loss, duplicate, reorder or
//     corruption), valid/ready rules, X on DUT outputs, PHY error flags,
//     outputs safe during reset without a clock, drain/liveness at the end.
// ============================================================================

module tb_dpti_sync245_phy;

    // ------------------------------------------------------------------
    // Configuration
    // ------------------------------------------------------------------
    real    T_CLK  = 16.667;
    real    TC     = 0.0;
    real    TD     = 0.0;
    real    T_INS  = 0.0;
    real    D_IN   = 1.454;     // measured IBUF delay (K16 STA, dpti_rxf_n), used as an estimate
    real    T_CO   = 5.0;       // [ASM] IOB register clock-to-out + OBUF 3.73 (STA) + margin
    real    D_RD, D_OE, D_WR, D_DAT;
    real    TSU_F  = 0.5;
    real    TH_F   = 0.5;
    integer CLK_TRACK   = 0;
    integer CHK_FPGA_IN = 1;
    integer SEED        = 1;
    integer NRX         = 3000;
    integer NTX         = 3000;
    integer READY_PCT   = 100;
    integer TXV_PCT     = 100;
    integer TIMEOUT_CYC = 400000;
    reg [8*24-1:0] TEST = "bidir";

    // ------------------------------------------------------------------
    // Nets
    // ------------------------------------------------------------------
    reg         clk_run = 1'b1;
    wire        clkout;
    reg         clk_fpga = 1'b0;
    reg         arst = 1'b1;

    // FPGA internal (DUT) side
    wire [7:0]  pad_d_o, pad_d_t;
    wire        pad_rd_n, pad_oe_n, pad_wr_n;
    reg  [7:0]  pad_d_i_r;
    reg         pad_rxf_n_r = 1'b1, pad_txe_n_r = 1'b1;

    // chip pins
    reg         rd_ft = 1'b1, oe_ft = 1'b1, wr_ft = 1'b1;
    wire [7:0]  ft_d_drv;
    reg  [7:0]  fpga_at_ft = 8'bz;
    wire [7:0]  d_ft_pin;
    wire        rxf_ft, txe_ft;

    // FPGA pins
    reg  [7:0]  ft_at_fpga = 8'bz;
    reg  [7:0]  fpga_at_fpga = 8'bz;
    wire [7:0]  d_fpga_pin;
    reg         rxf_fpga = 1'b1, txe_fpga = 1'b1;

    assign d_ft_pin   = ft_d_drv;
    assign d_ft_pin   = fpga_at_ft;
    assign d_fpga_pin = ft_at_fpga;
    assign d_fpga_pin = fpga_at_fpga;

    // streams
    wire [7:0]  rx_data;
    wire        rx_valid;
    reg         rx_ready = 1'b0;
    reg  [7:0]  tx_data  = 8'h00;
    reg         tx_valid = 1'b0;
    wire        tx_ready;

    wire        rx_pop_evt, tx_acc_evt;
    wire [7:0]  rx_pop_byte, tx_acc_byte;

    // ------------------------------------------------------------------
    // Chip model
    // ------------------------------------------------------------------
    ft2232h_sync245_model u_ft (
        .clk_run      (clk_run),
        .clkout       (clkout),
        .rd_n         (rd_ft),
        .oe_n         (oe_ft),
        .wr_n         (wr_ft),
        .d_pin        (d_ft_pin),
        .fpga_d_at_ft (fpga_at_ft),
        .ft_d_drv     (ft_d_drv),
        .rxf_n        (rxf_ft),
        .txe_n        (txe_ft),
        .rx_pop_evt   (rx_pop_evt),
        .rx_pop_byte  (rx_pop_byte),
        .tx_acc_evt   (tx_acc_evt),
        .tx_acc_byte  (tx_acc_byte)
    );

    // ------------------------------------------------------------------
    // DUT
    // ------------------------------------------------------------------
`ifdef LEGACY
    dpti_legacy_phy_wrap dut (
`else
    wire [1:0]  bus_mode;
    wire        err_rx_overflow;
    wire [31:0] rx_xfer_count, tx_accept_count, tx_reject_count;
    dpti_sync245_phy dut (
`endif
        .clk       (clk_fpga),
        .arst      (arst),
        .pad_d_i   (pad_d_i_r),
        .pad_d_o   (pad_d_o),
        .pad_d_t   (pad_d_t),
        .pad_rxf_n (pad_rxf_n_r),
        .pad_txe_n (pad_txe_n_r),
        .pad_rd_n  (pad_rd_n),
        .pad_oe_n  (pad_oe_n),
        .pad_wr_n  (pad_wr_n),
        .rx_data   (rx_data),
        .rx_valid  (rx_valid),
        .rx_ready  (rx_ready),
        .tx_data   (tx_data),
        .tx_valid  (tx_valid),
`ifdef LEGACY
        .tx_ready  (tx_ready)
`else
        .tx_ready  (tx_ready),
        .bus_mode        (bus_mode),
        .err_rx_overflow (err_rx_overflow),
        .rx_xfer_count   (rx_xfer_count),
        .tx_accept_count (tx_accept_count),
        .tx_reject_count (tx_reject_count)
`endif
    );

    // ------------------------------------------------------------------
    // Delays (transport: nonblocking with intra-assignment delay)
    // ------------------------------------------------------------------
    wire [7:0] fpga_drv;
    genvar gi;
    generate
        for (gi = 0; gi < 8; gi = gi + 1) begin : g_drv
            assign fpga_drv[gi] = (pad_d_t[gi] === 1'b0) ? pad_d_o[gi] :
                                  (pad_d_t[gi] === 1'b1) ? 1'bz : 1'bx;
        end
    endgenerate

    always @(pad_rd_n)  rd_ft        <= #(D_RD + TD)  pad_rd_n;
    always @(pad_oe_n)  oe_ft        <= #(D_OE + TD)  pad_oe_n;
    always @(pad_wr_n)  wr_ft        <= #(D_WR + TD)  pad_wr_n;
    always @(fpga_drv)  fpga_at_ft   <= #(D_DAT + TD) fpga_drv;
    always @(fpga_drv)  fpga_at_fpga <= #(D_DAT)      fpga_drv;
    always @(ft_d_drv)  ft_at_fpga   <= #(TD)         ft_d_drv;
    always @(rxf_ft)    rxf_fpga     <= #(TD)         rxf_ft;
    always @(txe_ft)    txe_fpga     <= #(TD)         txe_ft;
    always @(d_fpga_pin) pad_d_i_r   <= #(D_IN)       d_fpga_pin;
    always @(rxf_fpga)  pad_rxf_n_r  <= #(D_IN)       rxf_fpga;
    always @(txe_fpga)  pad_txe_n_r  <= #(D_IN)       txe_fpga;

    // Internal FPGA clock
    always @(posedge clkout) begin : fpga_clock
        real d;
        if (CLK_TRACK != 0)
            d = TC + T_INS;
        else
            d = u_ft.ideal_rise + TC + T_INS - $realtime;
        if (d < 0.0) begin
            $display("TB CONFIG ERROR: internal clock would precede CLKOUT (d=%0.3f); need TC+T_INS >= JIT", d);
            $finish;
        end
        clk_fpga <= #(d) 1'b1;
        clk_fpga <= #(d + T_CLK / 2.0) 1'b0;
    end

    // ------------------------------------------------------------------
    // Error bookkeeping
    // ------------------------------------------------------------------
    integer err_sb_rx = 0, err_sb_tx = 0, err_x = 0, err_hs = 0;
    integer err_fin = 0, err_fsetup = 0, err_fhold = 0, err_contend_f = 0;
    integer err_reset = 0, err_flag = 0, err_drain = 0;
    integer tb_reported = 0;
    integer checks_on = 0;
    integer sb_ignore = 0;
    integer lost_rx_reset = 0, lost_tx_reset = 0;

    task automatic tb_err(input [8*16-1:0] kind, input [8*120-1:0] msg);
        begin
            if (tb_reported < 30)
                $display("TB %0s @%0.3f ns: %0s", kind, $realtime, msg);
            tb_reported = tb_reported + 1;
        end
    endtask

    // ------------------------------------------------------------------
    // Contention at the FPGA pins
    // ------------------------------------------------------------------
    always @(ft_at_fpga or fpga_at_fpga) begin : contention_f
        integer i;
        if (checks_on != 0) begin
            for (i = 0; i < 8; i = i + 1) begin
                if (ft_at_fpga[i] !== 1'bz && fpga_at_fpga[i] !== 1'bz) begin
                    err_contend_f = err_contend_f + 1;
                    tb_err("CONTENTION", "chip drive and FPGA drive overlap at the FPGA pins");
                    disable contention_f;
                end
            end
        end
    end

    // ------------------------------------------------------------------
    // FPGA capture-register window (inputs at the register, vs clk_fpga)
    // ------------------------------------------------------------------
    realtime t_rxf_chg = 0, t_txe_chg = 0, t_d_chg = 0, t_fedge = -100.0, t_dcrit = -100.0;
    realtime cap_q [$];
    logic [7:0] cap_v [$];

    always @(pad_rxf_n_r) begin
        t_rxf_chg = $realtime;
        if (checks_on != 0 && CHK_FPGA_IN != 0 && $realtime - t_fedge < TH_F) begin
            err_fhold = err_fhold + 1; tb_err("FPGA-HOLD", "RXF# changed inside the capture hold window");
        end
    end
    always @(pad_txe_n_r) begin
        t_txe_chg = $realtime;
        if (checks_on != 0 && CHK_FPGA_IN != 0 && $realtime - t_fedge < TH_F) begin
            err_fhold = err_fhold + 1; tb_err("FPGA-HOLD", "TXE# changed inside the capture hold window");
        end
    end
    always @(pad_d_i_r) begin
        t_d_chg = $realtime;
        if (checks_on != 0 && CHK_FPGA_IN != 0 && $realtime - t_dcrit < TH_F) begin
            err_fhold = err_fhold + 1; tb_err("FPGA-HOLD", "D changed inside the hold window of a capture that carries a byte");
        end
    end

    // Expected capture edges of read bytes: the internal edge that follows the
    // chip edge of the transfer by TC + T_INS.
    always @(rx_pop_evt) if ($realtime > 0.0) begin
        cap_q.push_back(u_ft.t_edge + TC + T_INS);
        cap_v.push_back(rx_pop_byte);
    end

    always @(posedge clk_fpga) begin : fwin
        realtime te;
        reg [7:0] v;
        t_fedge = $realtime;
        if (checks_on != 0 && CHK_FPGA_IN != 0) begin
            if ($realtime - t_rxf_chg < TSU_F || (pad_rxf_n_r !== 1'b0 && pad_rxf_n_r !== 1'b1)) begin
                err_fsetup = err_fsetup + 1; tb_err("FPGA-SETUP", "RXF# not stable for TSU_F before the capture edge");
            end
            if ($realtime - t_txe_chg < TSU_F || (pad_txe_n_r !== 1'b0 && pad_txe_n_r !== 1'b1)) begin
                err_fsetup = err_fsetup + 1; tb_err("FPGA-SETUP", "TXE# not stable for TSU_F before the capture edge");
            end
        end
        while (cap_q.size() > 0 && cap_q[0] < $realtime - T_CLK / 2.0) begin
            te = cap_q.pop_front(); v = cap_v.pop_front();
        end
        if (cap_q.size() > 0 && cap_q[0] <= $realtime + T_CLK / 2.0) begin
            te = cap_q.pop_front(); v = cap_v.pop_front();
            t_dcrit = $realtime;
            if (checks_on != 0 && CHK_FPGA_IN != 0) begin
                if ($realtime - t_d_chg < TSU_F) begin
                    err_fsetup = err_fsetup + 1; tb_err("FPGA-SETUP", "D not stable for TSU_F before the edge that captures a transferred byte");
                end
                if (pad_d_i_r !== v) begin
                    err_fsetup = err_fsetup + 1; tb_err("FPGA-SETUP", "D at the capture register differs from the byte the chip transferred");
                end
            end
        end
    end

    // ------------------------------------------------------------------
    // Byte streams and scoreboards
    // ------------------------------------------------------------------
    logic [7:0] rx_exp [$];
    logic [7:0] tx_exp [$];
    integer n_rx_got = 0, n_tx_sent = 0, n_tx_ft = 0, n_host = 0;
    integer stall_left = 0;
    integer rx_go = 0, tx_go = 0;
    integer rseed;

    function automatic [7:0] pat(input integer i, input integer stream);
        integer k;
        begin
            k = i + stream * 7919;
            // runs of 00 / FF every 1000 bytes, otherwise a mixing function
            if ((i % 1000) >= 990)      pat = ((i / 1000) % 2) ? 8'hFF : 8'h00;
            else                        pat = (k * 37 + (k >> 8) * 101 + (k >> 3)) & 8'hFF;
        end
    endfunction

    function automatic integer rnd(input integer lo, input integer hi);
        integer r;
        begin
            r = $random(rseed); if (r < 0) r = -r;
            rnd = lo + (r % (hi - lo + 1));
        end
    endfunction

    always @(rx_pop_evt) if ($realtime > 0.0) begin
        if (sb_ignore != 0) lost_rx_reset = lost_rx_reset + 1;
        else                rx_exp.push_back(rx_pop_byte);
    end

    // TX scoreboard.  The chip's acceptances are queued and matched in order
    // against the bytes the PHY took from the source.  For a pass-through PHY
    // (the legacy one) the acceptance and the source handshake fall on the same
    // edge, so matching is deferred to the next source edge; more than one
    // acceptance ahead of the source is an error in its own right.
    logic [7:0] tx_got [$];
    always @(tx_acc_evt) if ($realtime > 0.0) begin
        n_tx_ft = n_tx_ft + 1;
        if (sb_ignore != 0) lost_tx_reset = lost_tx_reset + 1;
        else                tx_got.push_back(tx_acc_byte);
    end

    task automatic tx_match(input integer final_check);
        reg [7:0] e, g;
        begin
            while (tx_got.size() > 0 && tx_exp.size() > 0) begin
                e = tx_exp.pop_front();
                g = tx_got.pop_front();
                if (g !== e) begin
                    err_sb_tx = err_sb_tx + 1;
                    tb_err("SB-TX", "chip accepted a byte out of order / corrupted");
                    if (err_sb_tx < 5) $display("      expected %02h got %02h", e, g);
                end
            end
            if (tx_got.size() > ((final_check != 0) ? 0 : 1)) begin
                err_sb_tx = err_sb_tx + tx_got.size();
                tb_err("SB-TX", "chip accepted byte(s) that were never offered (duplicate or garbage)");
                tx_got.delete();
            end
        end
    endtask

    // RX sink + valid/ready rules
    reg       prev_vnr = 1'b0;
    reg [7:0] prev_rx_data;
    always @(posedge clk_fpga) begin : rx_sink
        reg [7:0] e;
        if (checks_on != 0) begin
            if (rx_valid !== 1'b0 && rx_valid !== 1'b1) begin err_x = err_x + 1; tb_err("X", "rx_valid is X"); end
            if (rx_valid === 1'b1 && (^rx_data) === 1'bx) begin err_x = err_x + 1; tb_err("X", "rx_data is X while rx_valid"); end
            if (prev_vnr && arst === 1'b0 && (rx_valid !== 1'b1 || rx_data !== prev_rx_data)) begin
                err_hs = err_hs + 1; tb_err("HANDSHAKE", "rx_valid dropped or rx_data changed before rx_ready");
            end
        end
        if (rx_valid === 1'b1 && rx_ready === 1'b1 && arst === 1'b0) begin
            n_rx_got = n_rx_got + 1;
            if (rx_exp.size() == 0) begin
                err_sb_rx = err_sb_rx + 1;
                tb_err("SB-RX", "PHY delivered a byte the chip never transferred (duplicate or garbage)");
            end
            else begin
                e = rx_exp.pop_front();
                if (rx_data !== e) begin
                    err_sb_rx = err_sb_rx + 1;
                    tb_err("SB-RX", "PHY delivered a byte out of order / corrupted");
                    if (err_sb_rx < 5) $display("      rx #%0d expected %02h got %02h", n_rx_got, e, rx_data);
                end
            end
        end
        prev_vnr     = (rx_valid === 1'b1 && rx_ready !== 1'b1 && arst === 1'b0);
        prev_rx_data = rx_data;
        // next ready
        if (stall_left > 0) begin
            stall_left = stall_left - 1;
            rx_ready <= 1'b0;
        end
        else begin
            rx_ready <= (rnd(0, 99) < READY_PCT);
        end
    end

    // TX source (valid/ready: data held until accepted)
    always @(posedge clk_fpga) begin : tx_src
        reg taken;
        taken = (tx_valid === 1'b1 && tx_ready === 1'b1 && arst === 1'b0);
        if (taken) begin
            if (sb_ignore == 0) tx_exp.push_back(tx_data);
            n_tx_sent = n_tx_sent + 1;
        end
        tx_match(0);
        if (checks_on != 0 && tx_ready !== 1'b0 && tx_ready !== 1'b1) begin
            err_x = err_x + 1; tb_err("X", "tx_ready is X");
        end
        if (arst === 1'b1) begin
            tx_valid <= 1'b0;
        end
        else if (tx_valid !== 1'b1 || taken) begin
            if (tx_go != 0 && n_tx_sent < NTX && rnd(0, 99) < TXV_PCT) begin
                tx_valid <= 1'b1;
                tx_data  <= pat(n_tx_sent, 1);
            end
            else begin
                tx_valid <= 1'b0;
            end
        end
    end

    // Host feeding the chip (packets with gaps, or trickle)
    integer feed_mode = 0;     // 0 packets, 1 trickle (single bytes, gaps), 2 all at once
    initial begin : host_feed
        integer pk, j;
        forever begin
            if (rx_go != 0 && n_host < NRX) begin
                if (feed_mode == 2) begin
                    while (n_host < NRX) begin
                        u_ft.host_push(pat(n_host, 0)); n_host = n_host + 1;
                    end
                end
                else if (feed_mode == 1) begin
                    u_ft.host_push(pat(n_host, 0)); n_host = n_host + 1;
                    repeat (rnd(0, 6)) @(posedge clkout);
                end
                else begin
                    pk = rnd(1, 512);
                    for (j = 0; j < pk && n_host < NRX; j = j + 1) begin
                        u_ft.host_push(pat(n_host, 0)); n_host = n_host + 1;
                    end
                    repeat (rnd(0, 200)) @(posedge clkout);
                end
            end
            else begin
                @(posedge clkout);
            end
        end
    end

    // DUT output X / flag checks
    always @(posedge clk_fpga) begin
        if (checks_on != 0) begin
            if ((pad_rd_n !== 1'b0 && pad_rd_n !== 1'b1) ||
                (pad_oe_n !== 1'b0 && pad_oe_n !== 1'b1) ||
                (pad_wr_n !== 1'b0 && pad_wr_n !== 1'b1) ||
                ((^pad_d_t) === 1'bx)) begin
                err_x = err_x + 1; tb_err("X", "X on a pad control output");
            end
`ifndef LEGACY
            if (err_rx_overflow !== 1'b0) begin
                err_flag = err_flag + 1; tb_err("FLAG", "err_rx_overflow set");
            end
`endif
        end
    end

    // ------------------------------------------------------------------
    // Coverage / throughput (observation only)
    // ------------------------------------------------------------------
    realtime t_rx_first = -1.0, t_rx_last = 0, t_tx_first = -1.0, t_tx_last = 0;
    always @(rx_pop_evt) if ($realtime > 0.0) begin
        if (t_rx_first < 0.0) t_rx_first = $realtime;
        t_rx_last = $realtime;
    end
    always @(tx_acc_evt) if ($realtime > 0.0) begin
        if (t_tx_first < 0.0) t_tx_first = $realtime;
        t_tx_last = $realtime;
    end
`ifndef LEGACY
    integer cov_rx2idle = 0, cov_tx2idle = 0, cov_both_want = 0, cov_rx_hiwater = 0;
    integer cov_rx_full_stall = 0, cov_tx_ready_low = 0;
    reg [1:0] prev_mode = 2'd0;
    always @(posedge clk_fpga) begin
        if (checks_on != 0) begin
            if (prev_mode == 2'd1 && dut.mode == 2'd0) cov_rx2idle = cov_rx2idle + 1;
            if (prev_mode == 2'd2 && dut.mode == 2'd0) cov_tx2idle = cov_tx2idle + 1;
            if (dut.rx_want === 1'b1 && dut.tx_want === 1'b1) cov_both_want = cov_both_want + 1;
            if (dut.rx_count > cov_rx_hiwater) cov_rx_hiwater = dut.rx_count;
            if (dut.mode == 2'd1 && dut.rxf_q === 1'b0 && dut.rx_space === 1'b0) cov_rx_full_stall = cov_rx_full_stall + 1;
            if (tx_valid === 1'b1 && tx_ready === 1'b0) cov_tx_ready_low = cov_tx_ready_low + 1;
        end
        prev_mode = dut.mode;
    end
`endif

    // ------------------------------------------------------------------
    // Sequences
    // ------------------------------------------------------------------
    task automatic do_reset(input integer cycles);
        begin
            arst = 1'b1;
            repeat (cycles) @(posedge clkout);
            #3.0 arst = 1'b0;
            repeat (8) @(posedge clkout);
        end
    endtask

    // Outputs must be inactive while arst is high, even with no clock.
    task automatic check_safe(input [8*40-1:0] where);
        begin
            if (pad_rd_n !== 1'b1 || pad_oe_n !== 1'b1 || pad_wr_n !== 1'b1 || pad_d_t !== 8'hFF) begin
                err_reset = err_reset + 1;
                tb_err("RESET", where);
                $display("      rd_n=%b oe_n=%b wr_n=%b d_t=%b", pad_rd_n, pad_oe_n, pad_wr_n, pad_d_t);
            end
        end
    endtask

    task automatic wait_drain(input integer max_cycles);
        integer c;
        begin
            c = 0;
            while (c < max_cycles &&
                   !(n_host >= NRX && u_ft.host_level() == 0 && rx_exp.size() == 0 &&
                     n_tx_sent >= NTX && tx_valid === 1'b0 && tx_exp.size() == 0 &&
                     tx_got.size() == 0)) begin
                @(posedge clkout);
                c = c + 1;
            end
            tx_match(1);
            if (c >= max_cycles) begin
                err_drain = err_drain + 1;
                $display("TB DRAIN timeout: host_left=%0d rx_exp=%0d tx_exp=%0d tx_got=%0d tx_sent=%0d/%0d",
                         u_ft.host_level(), rx_exp.size(), tx_exp.size(), tx_got.size(), n_tx_sent, NTX);
            end
        end
    endtask

    initial begin : main
        integer i;
        if ($value$plusargs("TC=%f", TC)) ;
        if ($value$plusargs("TD=%f", TD)) ;
        if ($value$plusargs("T_INS=%f", T_INS)) ;
        if ($value$plusargs("D_IN=%f", D_IN)) ;
        if ($value$plusargs("T_CO=%f", T_CO)) ;
        D_RD = T_CO; D_OE = T_CO; D_WR = T_CO; D_DAT = T_CO;
        if ($value$plusargs("D_RD=%f", D_RD)) ;
        if ($value$plusargs("D_OE=%f", D_OE)) ;
        if ($value$plusargs("D_WR=%f", D_WR)) ;
        if ($value$plusargs("D_DAT=%f", D_DAT)) ;
        if ($value$plusargs("TSU_F=%f", TSU_F)) ;
        if ($value$plusargs("TH_F=%f", TH_F)) ;
        if ($value$plusargs("CLK_TRACK=%d", CLK_TRACK)) ;
        if ($value$plusargs("CHK_FPGA_IN=%d", CHK_FPGA_IN)) ;
        if ($value$plusargs("SEED=%d", SEED)) ;
        if ($value$plusargs("NRX=%d", NRX)) ;
        if ($value$plusargs("NTX=%d", NTX)) ;
        if ($value$plusargs("READY_PCT=%d", READY_PCT)) ;
        if ($value$plusargs("TXV_PCT=%d", TXV_PCT)) ;
        if ($value$plusargs("TXCAP=%d", u_ft.TXCAP)) ;
        if ($value$plusargs("DRAIN_PCT=%d", u_ft.DRAIN_PCT)) ;
        if ($value$plusargs("GLITCH=%d", u_ft.GLITCH_PERMILLE)) ;
        if ($value$plusargs("FEED=%d", feed_mode)) ;
        if ($value$plusargs("TEST=%s", TEST)) ;
        rseed = SEED * 7 + 3;

        $display("CFG test=%0s legacy=%0d TC=%0.3f TD=%0.3f T_INS=%0.3f D_IN=%0.3f D_RD=%0.3f D_OE=%0.3f D_WR=%0.3f D_DAT=%0.3f TSU_F=%0.2f TH_F=%0.2f CLK_TRACK=%0d JIT=%0.2f DUTY=%0.2f TCO_MODE=%0d TOFF=%0.2f SEED=%0d NRX=%0d NTX=%0d READY=%0d TXV=%0d TXCAP=%0d DRAIN=%0d GLITCH=%0d FEED=%0d",
`ifdef LEGACY
                 TEST, 1,
`else
                 TEST, 0,
`endif
                 TC, TD, T_INS, D_IN, D_RD, D_OE, D_WR, D_DAT, TSU_F, TH_F, CLK_TRACK,
                 u_ft.JIT, u_ft.DUTY, u_ft.TCO_MODE, u_ft.TOFF, SEED, NRX, NTX,
                 READY_PCT, TXV_PCT, u_ft.TXCAP, u_ft.DRAIN_PCT, u_ft.GLITCH_PERMILLE, feed_mode);

        // ---- power-on reset with the clock running ----
        arst = 1'b1;
        repeat (4) @(posedge clkout);
        #1.0 check_safe("outputs not inactive during power-on reset");
        do_reset(16);
        checks_on    = 1;
        u_ft.checks_on = 1;

        if (TEST == "reset") begin
            // (a) arst with the clock STOPPED must still force the outputs
            //     inactive (the MMCM-unlock case); (b) traffic, then arst in
            //     the middle of it, then fresh traffic must be exact.
            NRX = 600; NTX = 600;
            rx_go = 1; tx_go = 1;
            repeat (300) @(posedge clkout);
            // stop CLKOUT mid-traffic, then assert arst with no clock
            sb_ignore = 1;
            clk_run = 1'b0;
            #40.0 arst = 1'b1;
            #0.5 check_safe("outputs not inactive 0.5 ns after arst with CLKOUT stopped");
            #200.0 check_safe("outputs not inactive 200 ns after arst with CLKOUT stopped");
            clk_run = 1'b1;
            repeat (20) @(posedge clkout);
            check_safe("outputs not inactive while arst held with CLKOUT running");
            // everything in flight is gone by definition of reset
            lost_rx_reset = lost_rx_reset + rx_exp.size();
            lost_tx_reset = lost_tx_reset + tx_exp.size();
            rx_exp.delete(); tx_exp.delete(); tx_got.delete();
            // The PHY is still in reset and holds nothing; from here on every
            // byte must be accounted for again.
            sb_ignore = 0;
            #3.0 arst = 1'b0;
            repeat (8) @(posedge clkout);
            // second phase: arst with the clock running, mid-traffic
            repeat (300) @(posedge clkout);
            sb_ignore = 1;
            #2.0 arst = 1'b1;
            #0.5 check_safe("outputs not inactive 0.5 ns after arst, clock running");
            repeat (10) @(posedge clkout);
            lost_rx_reset = lost_rx_reset + rx_exp.size();
            lost_tx_reset = lost_tx_reset + tx_exp.size();
            rx_exp.delete(); tx_exp.delete(); tx_got.delete();
            // The PHY is still in reset and holds nothing; from here on every
            // byte must be accounted for again.
            sb_ignore = 0;
            #3.0 arst = 1'b0;
            repeat (8) @(posedge clkout);
            NRX = n_host + 1500; NTX = n_tx_sent + 1500;
            wait_drain(TIMEOUT_CYC);
        end
        else begin
            if (TEST == "rx" || TEST == "bidir" || TEST == "rx_bp") rx_go = 1;
            if (TEST == "tx" || TEST == "bidir")                    tx_go = 1;
            if (rx_go == 0) NRX = 0;
            if (tx_go == 0) NTX = 0;
            if (TEST == "rx_bp") begin
                // long hard stalls of the sink while the host keeps feeding
                for (i = 0; i < 6; i = i + 1) begin
                    repeat (rnd(50, 400)) @(posedge clkout);
                    stall_left = rnd(100, 800);
                end
            end
            wait_drain(TIMEOUT_CYC);
        end

        repeat (40) @(posedge clkout);
        checks_on = 0;
        u_ft.checks_on = 0;

        begin : summary
            integer total;
            total = u_ft.err_proto + u_ft.err_setup + u_ft.err_contend + u_ft.err_wdata +
                    err_sb_rx + err_sb_tx + err_x + err_hs + err_fsetup + err_fhold +
                    err_contend_f + err_reset + err_flag + err_drain;
            $display("COUNTS rx_host=%0d rx_popped=%0d rx_delivered=%0d rd_strobes_on_empty=%0d tx_offered=%0d tx_chip_accepted=%0d tx_chip_rejected=%0d lost_in_reset rx=%0d tx=%0d",
                     n_host, u_ft.n_rx_pop, n_rx_got, u_ft.n_rd_strobe_empty, n_tx_sent, n_tx_ft, u_ft.n_tx_rej,
                     lost_rx_reset, lost_tx_reset);
            $display("RATE rx_bytes_per_clk=%0.3f tx_bytes_per_clk=%0.3f",
                     (u_ft.n_rx_pop > 1) ? (u_ft.n_rx_pop - 1) / ((t_rx_last - t_rx_first) / T_CLK) : 0.0,
                     (n_tx_ft > 1) ? (n_tx_ft - 1) / ((t_tx_last - t_tx_first) / T_CLK) : 0.0);
`ifndef LEGACY
            $display("COVER rx_to_idle=%0d tx_to_idle=%0d both_requesting_cycles=%0d rx_fifo_hiwater=%0d/%0d rx_stalled_by_space=%0d tx_ready_low_cycles=%0d phy_rx_xfer=%0d phy_tx_acc=%0d phy_tx_rej=%0d",
                     cov_rx2idle, cov_tx2idle, cov_both_want, cov_rx_hiwater, dut.RX_FIFO_DEPTH, cov_rx_full_stall,
                     cov_tx_ready_low, rx_xfer_count, tx_accept_count, tx_reject_count);
`endif
            $display("ERRORS ft_proto=%0d ft_setup=%0d ft_contention=%0d ft_wdata=%0d fpga_contention=%0d fpga_in_setup=%0d fpga_in_hold=%0d sb_rx=%0d sb_tx=%0d handshake=%0d x=%0d reset=%0d flag=%0d drain=%0d",
                     u_ft.err_proto, u_ft.err_setup, u_ft.err_contend, u_ft.err_wdata, err_contend_f,
                     err_fsetup, err_fhold, err_sb_rx, err_sb_tx, err_hs, err_x, err_reset, err_flag, err_drain);
            $display("RESULT %0s test=%0s total_errors=%0d", (total == 0) ? "PASS" : "FAIL", TEST, total);
        end
        $finish;
    end

    // hard stop
    initial begin
        #(T_CLK * 2000000.0);
        $display("RESULT FAIL test=%0s total_errors=-1 (simulation time limit)", TEST);
        $finish;
    end

endmodule

`default_nettype wire
