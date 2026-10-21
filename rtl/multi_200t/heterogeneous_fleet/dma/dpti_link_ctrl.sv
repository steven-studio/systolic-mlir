`default_nettype none

// ============================================================================
// dpti_link_ctrl.sv
//
// Coordinated reset of the DPTI link: the clk_dpti domain and the ui_clk
// halves of the three DPTI clock-domain crossings, taken down and brought up
// together around MMCM LOCKED.
//
// WHY BOTH HALVES TOGETHER
//   dpti_cmd_async_fifo, dpti_output_cdc and dpti_mem_write_cdc are Gray-
//   pointer FIFOs whose two sides reset separately.  Resetting only the
//   clk_dpti side zeroes its write (or read) pointer while the ui_clk side
//   keeps its own, and the ui_clk side then sees entries that were never
//   written.  So the clk_dpti side is reset only while the ui_clk side is
//   held in reset as well (ui_link_rst), and the ui_clk side is released only
//   after the clk_dpti side has provably applied and released its reset.
//
// WHAT IS NOT RESET
//   The MEM_WRITE AXI engine (dpti_mem_write_engine) and the AXI4-Lite master
//   keep ui_rst_n only.  Resetting either in the middle of an AXI transaction
//   would break the protocol towards the MIG / the register bridge.  Instead:
//     * when the link drops, the clk_dpti domain is frozen (dpti_clk_gen holds
//       the MMCM in reset, so clk_dpti stops and the clk_dpti halves keep their
//       state) and the ui_clk side is allowed to DRAIN what already crossed:
//       pending WRITE32 commands execute, a MEM_WRITE whose beats all arrived
//       completes;
//     * if the MEM_WRITE engine is still busy after DRAIN_TIMEOUT_CYC -- its
//       command was cut off -- the link goes to FATAL: err_link_fatal is
//       latched (any_err / led[6], VIO), the clk_dpti domain and the ui_clk
//       halves stay in reset, the engine is left alone, and only ui_rst_n
//       (cpu_resetn) recovers.
//   Readback data still in flight to the host when the link drops is
//   discarded (the output CDC's ui_clk side accepts and drops it while held
//   in reset), so a later session never receives an earlier session's bytes.
//
// STATES (ui_clk)
//   DOWN_ASSERT   rst_req = 1, ui_link_rst = 1.  Leaves when LOCKED has been
//                 high for UP_DELAY_CYC cycles and the clk_dpti reset is seen
//                 asserted.  FATAL if the MEM_WRITE engine is busy (only a
//                 command started from a stale header can make it so).
//   DOWN_RELEASE  rst_req = 0, ui_link_rst = 1.  dpti_rst stays high for three
//                 clk_dpti edges after rst_req falls (every synchronous reset
//                 in the domain is applied on them); UP when its synchronized
//                 copy reads 0.  Back to DOWN_ASSERT if LOCKED drops.
//   UP            normal operation.  DRAIN when LOCKED drops.
//   DRAIN         hold_mmcm = 1.  DOWN_ASSERT once the ui_clk side has been
//                 quiet (MEM_WRITE engine idle, command FIFO empty, AXI4-Lite
//                 master idle) for QUIET_CYC consecutive cycles, but not before
//                 MIN_DRAIN_CYC cycles; FATAL after DRAIN_TIMEOUT_CYC.
//   FATAL         rst_req = 1, ui_link_rst = 1, err_link_fatal = 1; exit only
//                 through ui_rst_n.
//   ui_rst_n low resets the controller asynchronously into DOWN_ASSERT, so the
//   link is also taken down cleanly by cpu_resetn, with or without CLKOUT.
// ============================================================================

module dpti_link_ctrl #(
    parameter integer UP_DELAY_CYC      = 64,
    parameter integer MIN_DRAIN_CYC     = 64,
    parameter integer QUIET_CYC         = 32,
    parameter integer DRAIN_TIMEOUT_CYC = 65536
) (
    input  wire       ui_clk,
    input  wire       ui_rst_n,           // asynchronous assert

    input  wire       mmcm_locked,        // asynchronous

    // ui_clk-side activity (all ui_clk)
    input  wire       mem_writer_busy,
    input  wire       cmd_fifo_rd_valid,
    input  wire       cmd_master_idle,

    input  wire       clk_dpti,
    output wire       dpti_rst,           // clk_dpti: async assert, sync release

    output reg        ui_link_rst,        // ui_clk: ui_clk halves of the CDCs
    output reg        hold_mmcm,          // to dpti_clk_gen.hold_rst
    output reg        link_up,
    output reg        err_link_fatal,     // sticky until ui_rst_n
    output reg  [7:0] link_drop_count,    // saturating
    output wire       locked_ui,
    output wire [2:0] state_o
);

    localparam [2:0] S_DOWN_ASSERT  = 3'd0;
    localparam [2:0] S_DOWN_RELEASE = 3'd1;
    localparam [2:0] S_UP           = 3'd2;
    localparam [2:0] S_DRAIN        = 3'd3;
    localparam [2:0] S_FATAL        = 3'd4;

    localparam integer CW = $clog2(DRAIN_TIMEOUT_CYC + 1);

    reg [2:0]    state;
    reg          rst_req;
    reg [CW-1:0] cnt;
    reg [CW-1:0] quiet_cnt;

    assign state_o = state;

    // ------------------------------------------------------------------
    // clk_dpti side: reset synchronizer.  rst_req sets it at once (with or
    // without clk_dpti); after rst_req falls it shifts out over three edges.
    // ------------------------------------------------------------------
    (* ASYNC_REG = "TRUE" *) reg [2:0] dpti_rst_sync = 3'b111;

    always @(posedge clk_dpti or posedge rst_req) begin
        if (rst_req)
            dpti_rst_sync <= 3'b111;
        else
            dpti_rst_sync <= {dpti_rst_sync[1:0], 1'b0};
    end

    assign dpti_rst = dpti_rst_sync[2];

    // ------------------------------------------------------------------
    // ui_clk side: synchronizers.
    // ------------------------------------------------------------------
    (* ASYNC_REG = "TRUE" *) reg [1:0] locked_s;
    (* ASYNC_REG = "TRUE" *) reg [1:0] dpti_rst_s;

    always @(posedge ui_clk or negedge ui_rst_n) begin
        if (!ui_rst_n) begin
            locked_s   <= 2'b00;
            dpti_rst_s <= 2'b11;
        end
        else begin
            locked_s   <= {locked_s[0],   mmcm_locked};
            dpti_rst_s <= {dpti_rst_s[0], dpti_rst};
        end
    end

    assign locked_ui = locked_s[1];
    wire   dpti_rst_ui = dpti_rst_s[1];

    wire quiet = !mem_writer_busy && !cmd_fifo_rd_valid && cmd_master_idle;

    // ------------------------------------------------------------------
    // State machine.
    // ------------------------------------------------------------------
    always @(posedge ui_clk or negedge ui_rst_n) begin
        if (!ui_rst_n) begin
            state           <= S_DOWN_ASSERT;
            rst_req         <= 1'b1;
            ui_link_rst     <= 1'b1;
            hold_mmcm       <= 1'b0;
            link_up         <= 1'b0;
            err_link_fatal  <= 1'b0;
            link_drop_count <= 8'd0;
            cnt             <= {CW{1'b0}};
            quiet_cnt       <= {CW{1'b0}};
        end
        else begin
            case (state)

            S_DOWN_ASSERT: begin
                rst_req     <= 1'b1;
                ui_link_rst <= 1'b1;
                hold_mmcm   <= 1'b0;
                link_up     <= 1'b0;

                if (!locked_ui)
                    cnt <= {CW{1'b0}};
                else if (cnt != {CW{1'b1}})
                    cnt <= cnt + 1'b1;

                if (mem_writer_busy) begin
                    state          <= S_FATAL;
                    err_link_fatal <= 1'b1;
                end
                else if (locked_ui && cnt >= UP_DELAY_CYC[CW-1:0] && dpti_rst_ui) begin
                    rst_req <= 1'b0;
                    state   <= S_DOWN_RELEASE;
                end
            end

            S_DOWN_RELEASE: begin
                if (!locked_ui) begin
                    rst_req <= 1'b1;
                    cnt     <= {CW{1'b0}};
                    state   <= S_DOWN_ASSERT;
                end
                else if (!dpti_rst_ui) begin
                    ui_link_rst <= 1'b0;
                    link_up     <= 1'b1;
                    state       <= S_UP;
                end
            end

            S_UP: begin
                if (!locked_ui) begin
                    hold_mmcm <= 1'b1;
                    link_up   <= 1'b0;
                    cnt       <= {CW{1'b0}};
                    quiet_cnt <= {CW{1'b0}};
                    state     <= S_DRAIN;
                    if (link_drop_count != 8'hFF)
                        link_drop_count <= link_drop_count + 8'd1;
                end
            end

            S_DRAIN: begin
                hold_mmcm <= 1'b1;
                cnt       <= cnt + 1'b1;

                if (!quiet)
                    quiet_cnt <= {CW{1'b0}};
                else if (quiet_cnt != {CW{1'b1}})
                    quiet_cnt <= quiet_cnt + 1'b1;

                if (quiet && quiet_cnt >= QUIET_CYC[CW-1:0] &&
                    cnt >= MIN_DRAIN_CYC[CW-1:0]) begin
                    rst_req     <= 1'b1;
                    ui_link_rst <= 1'b1;
                    hold_mmcm   <= 1'b0;
                    cnt         <= {CW{1'b0}};
                    state       <= S_DOWN_ASSERT;
                end
                else if (cnt >= DRAIN_TIMEOUT_CYC[CW-1:0]) begin
                    rst_req        <= 1'b1;
                    ui_link_rst    <= 1'b1;
                    hold_mmcm      <= 1'b0;
                    err_link_fatal <= 1'b1;
                    state          <= S_FATAL;
                end
            end

            S_FATAL: begin
                rst_req        <= 1'b1;
                ui_link_rst    <= 1'b1;
                hold_mmcm      <= 1'b0;
                link_up        <= 1'b0;
                err_link_fatal <= 1'b1;
            end

            default: begin
                rst_req     <= 1'b1;
                ui_link_rst <= 1'b1;
                state       <= S_DOWN_ASSERT;
            end

            endcase
        end
    end

endmodule

`default_nettype wire
