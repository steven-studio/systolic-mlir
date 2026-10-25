`default_nettype none
`timescale 1ns / 1ps

// ============================================================================
// ft2232h_sync245_model.sv -- BENCH ONLY.
//
// Behavioural FT2232H channel A in FT245 synchronous-FIFO mode, seen from its
// own pins.  Every timing value is tagged:
//   [DS]   FTDI DS_FT2232H FT_000061 v2.10 (2025-11-21), Table 4.1 / Table 3.6 /
//          section 4.4 / Figure 4.4
//   [ASM]  assumption -- not in the datasheet; set by plusarg, swept by the run
//          script, never reported as a measured value.
//
//   t1   CLKOUT period 16.67                                           [DS]
//   t2/3 CLKOUT high/low 7.5 .. 9.17   (+DUTY, default 0.5)            [DS]
//   CLKOUT jitter +-JIT, per edge, non-accumulating (+JIT, default 0)  [ASM]
//   t4/t5/t11 CLKOUT -> RXF# / read DATA / TXE#: 1 .. 7.15             [DS]
//        +TCO_MODE: 0 random in [1, 7.15] per event (default), 1 always
//        7.15, 2 always 1.  The pin is X from edge+1 until it settles.
//   t6   OE# low -> read DATA valid: 1 .. 7.15 (X from +1 until valid) [DS]
//   OE# high -> D released (high-Z): +TOFF, default 7.15             [ASM]
//        (Table 4.1 has no such parameter.)
//   t7/t9/t12/t14 OE#, RD#, write DATA, WR# setup 8; holds 0          [DS]
//
// Protocol (Table 3.6, section 4.4) enforced as errors:
//   * RD# sampled low  => OE# sampled low at this edge and the previous one.
//   * only a read or a write cycle at any one time: WR# low with OE# or RD#
//     low is an error.
//   * a read transfer happens at an edge iff RXF# (as driven before the edge)
//     and RD# are both low; after the last byte RXF# goes high and the bus
//     carries X ("data ... after RXF# is high, is invalid").
//   * a write transfer happens iff TXE# (as driven before the edge) and WR#
//     are both low; D must be valid (no X/Z) at such an edge.
//   * both ends driving D at once (contention) is an error.
//
// Host side (USB): host_push() makes bytes available to RD#; the TX buffer
// holds TXCAP bytes and drains one byte per edge with probability DRAIN_PCT.
// TXE# may additionally be forced high for single clocks (GLITCH_PERMILLE) to
// stress accept/reject ordering -- [ASM], the datasheet neither promises nor
// excludes this.
// ============================================================================

module ft2232h_sync245_model (
    input  wire        clk_run,       // 0: CLKOUT held low (chip not in sync mode)
    output reg         clkout,

    input  wire        rd_n,          // all as seen at the FT2232H pins
    input  wire        oe_n,
    input  wire        wr_n,
    input  wire [7:0]  d_pin,         // resolved bus at the FT2232H pins
    input  wire [7:0]  fpga_d_at_ft,  // the FPGA's drive as it arrives here (z = off)
    output wire [7:0]  ft_d_drv,      // the chip's own drive (z = off)
    output reg         rxf_n,
    output reg         txe_n,

    // one toggle per completed transfer, with the byte
    output reg         rx_pop_evt  = 1'b0,
    output reg  [7:0]  rx_pop_byte = 8'h00,
    output reg         tx_acc_evt  = 1'b0,
    output reg  [7:0]  tx_acc_byte = 8'h00
);

    // ---- [DS] Table 4.1 ----
    real T_CLK    = 16.667;
    real TCO_MIN  = 1.0;
    real TCO_MAX  = 7.15;
    real T6_MIN   = 1.0;
    real T6_MAX   = 7.15;
    real TSU      = 8.0;
    // ---- [ASM] ----
    real TOFF     = 7.15;
    real JIT      = 0.0;
    real DUTY     = 0.5;
    integer TCO_MODE = 0;

    // ---- bench knobs (set hierarchically by the testbench) ----
    integer TXCAP           = 64;
    integer DRAIN_PCT       = 100;
    integer GLITCH_PERMILLE = 0;
    integer checks_on       = 0;      // protocol checks armed (after reset)
    integer seed            = 1;

    // ---- error counters ----
    integer err_proto   = 0;
    integer err_setup   = 0;
    integer err_contend = 0;
    integer err_wdata   = 0;
    integer n_rx_pop    = 0;
    integer n_tx_acc    = 0;
    integer n_tx_rej    = 0;
    integer n_rd_strobe_empty = 0;
    integer max_report  = 20;
    integer reported    = 0;

    // ---- state ----
    real        ideal_rise = 0.0;   // n*T of the current edge, without jitter
    logic [7:0] avail [$];          // bytes the host has made readable
    integer     txbuf  = 0;
    reg         rxf_state = 1'b1;   // as driven during the current cycle
    reg         txe_state = 1'b1;
    reg         prev_oe_low = 1'b0;
    reg         ft_drv_en = 1'b0;
    reg  [7:0]  ft_dval   = 8'hxx;
    reg  [7:0]  presented = 8'hxx;
    realtime    t_rd = 0, t_oe = 0, t_wr = 0, t_d = 0;
    realtime    t_edge = 0;

    assign ft_d_drv = ft_drv_en ? ft_dval : 8'bz;

    initial begin
        clkout      = 1'b0;
        rxf_n       = 1'b1;
        txe_n       = 1'b1;
        if ($value$plusargs("TOFF=%f", TOFF)) ;
        if ($value$plusargs("JIT=%f", JIT)) ;
        if ($value$plusargs("DUTY=%f", DUTY)) ;
        if ($value$plusargs("TCO_MODE=%d", TCO_MODE)) ;
        if ($value$plusargs("SEED=%d", seed)) ;
    end

    function automatic real urand_real(input real lo, input real hi);
        integer r;
        begin
            r = $random(seed);
            if (r < 0) r = -r;
            urand_real = lo + (hi - lo) * ((r % 10001) / 10000.0);
        end
    endfunction

    function automatic integer urand_pct();
        integer r;
        begin
            r = $random(seed);
            if (r < 0) r = -r;
            urand_pct = r % 100;
        end
    endfunction

    function automatic real tco();
        begin
            case (TCO_MODE)
                1:       tco = TCO_MAX;
                2:       tco = TCO_MIN;
                default: tco = urand_real(TCO_MIN, TCO_MAX);
            endcase
        end
    endfunction

    task automatic report(input [8*16-1:0] kind, input [8*96-1:0] msg);
        begin
            if (reported < max_report)
                $display("FT2232H %0s @%0.3f ns: %0s", kind, $realtime, msg);
            reported = reported + 1;
        end
    endtask

    task automatic host_push(input [7:0] b);
        begin
            avail.push_back(b);
        end
    endtask

    function automatic integer host_level();
        host_level = avail.size();
    endfunction

    // ---------------- CLKOUT ([DS] t1..t3, [ASM] jitter) ----------------
    // Rising edge n at n*T + e_n, |e_n| <= JIT; high time DUTY*T.
    initial begin : clockgen
        real    next_rise;
        real    e;
        integer n;
        n = 1;
        forever begin
            if (!clk_run) begin
                clkout = 1'b0;
                @(posedge clk_run);
                n = $rtoi($realtime / T_CLK) + 2;
            end
            e = (JIT > 0.0) ? urand_real(-JIT, JIT) : 0.0;
            next_rise = n * T_CLK + e;
            if (next_rise > $realtime)
                #(next_rise - $realtime);
            if (clk_run) begin
                ideal_rise = n * T_CLK;
                clkout = 1'b1;
                #(DUTY * T_CLK);
                clkout = 1'b0;
            end
            n = n + 1;
        end
    end

    // ---------------- last-change stamps for setup checks ----------------
    always @(rd_n)  t_rd = $realtime;
    always @(oe_n)  t_oe = $realtime;
    always @(wr_n)  t_wr = $realtime;
    always @(d_pin) t_d  = $realtime;

    // ---------------- D drive: OE# -> data ([DS] t6, [ASM] TOFF) ----------
    always @(negedge oe_n) begin
        ft_drv_en <= #(T6_MIN) 1'b1;
        ft_dval   <= #(T6_MIN) 8'hxx;
        ft_dval   <= #(urand_real(T6_MIN, T6_MAX)) (rxf_state ? 8'hxx : presented);
    end
    always @(posedge oe_n) begin
        ft_drv_en <= #(TOFF) 1'b0;
    end

    // ---------------- contention at this end ----------------
    always @(ft_drv_en or ft_dval or fpga_d_at_ft) begin : contention
        integer i;
        if (checks_on != 0 && ft_drv_en) begin
            for (i = 0; i < 8; i = i + 1) begin
                if (fpga_d_at_ft[i] !== 1'bz) begin
                    err_contend = err_contend + 1;
                    report("CONTENTION", "chip drives D (OE# low) while the FPGA drive reaches the chip pins");
                    disable contention;
                end
            end
        end
    end

    // ---------------- one CLKOUT edge ----------------
    always @(posedge clkout) begin : edge_blk
        reg s_rd, s_oe, s_wr;
        reg [7:0] s_d;
        reg [7:0] newpres;
        real tc;
        s_rd = rd_n;
        s_oe = oe_n;
        s_wr = wr_n;
        s_d  = d_pin;
        t_edge = $realtime;

        if (checks_on != 0) begin
            // [DS] t7 / t9 / t14 setup (holds are 0)
            if ($realtime - t_oe < TSU) begin err_setup = err_setup + 1; report("SETUP", "OE# changed less than t7 = 8 ns before CLKOUT"); end
            if ($realtime - t_rd < TSU) begin err_setup = err_setup + 1; report("SETUP", "RD# changed less than t9 = 8 ns before CLKOUT"); end
            if ($realtime - t_wr < TSU) begin err_setup = err_setup + 1; report("SETUP", "WR# changed less than t14 = 8 ns before CLKOUT"); end
            if (s_rd !== 1'b0 && s_rd !== 1'b1) begin err_proto = err_proto + 1; report("PROTOCOL", "RD# is X/Z at CLKOUT"); end
            if (s_oe !== 1'b0 && s_oe !== 1'b1) begin err_proto = err_proto + 1; report("PROTOCOL", "OE# is X/Z at CLKOUT"); end
            if (s_wr !== 1'b0 && s_wr !== 1'b1) begin err_proto = err_proto + 1; report("PROTOCOL", "WR# is X/Z at CLKOUT"); end
            // [DS] Table 3.6: OE# low at least 1 clock before RD# low
            if (s_rd === 1'b0 && !(s_oe === 1'b0 && prev_oe_low)) begin
                err_proto = err_proto + 1;
                report("PROTOCOL", "RD# sampled low without OE# low at this and the previous CLKOUT edge");
            end
            // [DS] 4.4: only a read or a write cycle at any one time
            if (s_wr === 1'b0 && (s_oe === 1'b0 || s_rd === 1'b0)) begin
                err_proto = err_proto + 1;
                report("PROTOCOL", "WR# sampled low while OE# or RD# is low");
            end
            if (s_wr === 1'b0) begin
                if ($realtime - t_d < TSU) begin err_setup = err_setup + 1; report("SETUP", "write DATA changed less than t12 = 8 ns before CLKOUT"); end
                if ((^s_d) === 1'bx) begin err_wdata = err_wdata + 1; report("WDATA", "WR# low with X/Z on D"); end
            end
        end

        // ---- read transfer ----
        if (s_rd === 1'b0 && rxf_state == 1'b0) begin
            rx_pop_byte = avail.pop_front();
            rx_pop_evt  = ~rx_pop_evt;
            n_rx_pop    = n_rx_pop + 1;
        end
        else if (s_rd === 1'b0) begin
            n_rd_strobe_empty = n_rd_strobe_empty + 1;
        end

        // ---- write transfer ----
        if (s_wr === 1'b0 && txe_state == 1'b0) begin
            tx_acc_byte = s_d;
            tx_acc_evt  = ~tx_acc_evt;
            n_tx_acc    = n_tx_acc + 1;
            txbuf       = txbuf + 1;
        end
        else if (s_wr === 1'b0) begin
            n_tx_rej = n_tx_rej + 1;
        end

        // ---- USB side drains the TX buffer ----
        if (txbuf > 0 && urand_pct() < DRAIN_PCT)
            txbuf = txbuf - 1;

        prev_oe_low = (s_oe === 1'b0);

        // ---- next state of RXF# / TXE# ([DS] t4 / t11) ----
        if ((avail.size() == 0) != rxf_state) begin
            rxf_state = (avail.size() == 0);
            tc = tco();
            rxf_n <= #(TCO_MIN) 1'bx;
            rxf_n <= #(tc) rxf_state;
        end
        begin : txe_next
            reg nx;
            integer g;
            g = $random(seed); if (g < 0) g = -g;
            nx = (txbuf >= TXCAP) || ((g % 1000) < GLITCH_PERMILLE);
            if (nx != txe_state) begin
                txe_state = nx;
                tc = tco();
                txe_n <= #(TCO_MIN) 1'bx;
                txe_n <= #(tc) nx;
            end
        end

        // ---- read DATA ([DS] t5): head of the FIFO, X once empty ----
        newpres = rxf_state ? 8'hxx : avail[0];
        if (newpres !== presented) begin
            presented = newpres;
            tc = tco();
            ft_dval <= #(TCO_MIN) 8'hxx;
            ft_dval <= #(tc) newpres;
        end
    end

endmodule

`default_nettype wire
