`default_nettype none
`timescale 1ns / 1ps

/*
 * mig_axi_model.sv -- a behavioural stand-in for mig_7series_0.  BENCH ONLY.
 *
 * It answers one question that no unit bench can: with the seeder and the
 * write-back engine sharing one AXI write channel, does the memory see a clean
 * sequence of bursts from exactly one master at a time?  So the model is not
 * only storage -- it is a monitor:
 *
 *   - a W beat with no burst open is an error, not a dropped beat;
 *   - a burst that crosses a 4 KiB page is an error;
 *   - wlast in the wrong beat is an error;
 *   - and, the point of this file, once a write has landed in the write-back
 *     region no further write may land in the operand region.  That is the
 *     ownership assumption of systolic_dma_top stated from the far side of the
 *     bus, where a mux that hands over early or late would show up as
 *     interleaving even if every individual burst were well formed.
 *
 * ui_clk is sys_clk_i: the real controller's 4:1 PHY ratio against 800 Mbps
 * DDR3 gives 100 MHz, which is the frequency the bench drives in.
 */

module mig_7series_0 #(
  parameter integer MEM_WORDS = 4096,      // 32-bit words
  parameter integer WB_REGION = 4096       // byte address where results start
) (
  output wire [14:0] ddr3_addr, output wire [2:0] ddr3_ba,
  output wire ddr3_cas_n, output wire [0:0] ddr3_ck_n, output wire [0:0] ddr3_ck_p,
  output wire [0:0] ddr3_cke, output wire ddr3_ras_n, output wire ddr3_reset_n,
  output wire ddr3_we_n, inout wire [15:0] ddr3_dq,
  inout wire [1:0] ddr3_dqs_n, inout wire [1:0] ddr3_dqs_p,
  output logic init_calib_complete, output wire [1:0] ddr3_dm, output wire [0:0] ddr3_odt,

  output wire  ui_clk, output logic ui_clk_sync_rst,
  output wire  ui_addn_clk_0, output wire ui_addn_clk_1, output wire ui_addn_clk_2,
  output wire  ui_addn_clk_3, output wire ui_addn_clk_4,
  output wire  mmcm_locked, input wire aresetn,
  input  wire  app_sr_req, input wire app_ref_req, input wire app_zq_req,
  output wire  app_sr_active, output wire app_ref_ack, output wire app_zq_ack,

  input  wire [1:0]   s_axi_awid,   input wire [28:0] s_axi_awaddr,
  input  wire [7:0]   s_axi_awlen,  input wire [2:0]  s_axi_awsize,
  input  wire [1:0]   s_axi_awburst,input wire [0:0]  s_axi_awlock,
  input  wire [3:0]   s_axi_awcache,input wire [2:0]  s_axi_awprot,
  input  wire [3:0]   s_axi_awqos,  input wire        s_axi_awvalid,
  output logic        s_axi_awready,
  input  wire [127:0] s_axi_wdata,  input wire [15:0] s_axi_wstrb,
  input  wire         s_axi_wlast,  input wire        s_axi_wvalid,
  output logic        s_axi_wready,
  output logic [1:0]  s_axi_bid,    output logic [1:0] s_axi_bresp,
  output logic        s_axi_bvalid, input  wire        s_axi_bready,

  input  wire [1:0]   s_axi_arid,   input wire [28:0] s_axi_araddr,
  input  wire [7:0]   s_axi_arlen,  input wire [2:0]  s_axi_arsize,
  input  wire [1:0]   s_axi_arburst,input wire [0:0]  s_axi_arlock,
  input  wire [3:0]   s_axi_arcache,input wire [2:0]  s_axi_arprot,
  input  wire [3:0]   s_axi_arqos,  input wire        s_axi_arvalid,
  output logic        s_axi_arready,
  output logic [1:0]  s_axi_rid,    output logic [127:0] s_axi_rdata,
  output logic [1:0]  s_axi_rresp,  output logic       s_axi_rlast,
  output logic        s_axi_rvalid, input  wire        s_axi_rready,

  input  wire sys_clk_i, input wire clk_ref_i, input wire sys_rst
);

  localparam integer BPB = 16;   // bytes per 128-bit beat

  assign ui_clk = sys_clk_i;
  assign mmcm_locked = 1'b1;
  assign {ddr3_addr, ddr3_ba, ddr3_cas_n, ddr3_ck_n, ddr3_ck_p, ddr3_cke,
          ddr3_ras_n, ddr3_reset_n, ddr3_we_n, ddr3_dm, ddr3_odt} = '0;
  assign {ui_addn_clk_0, ui_addn_clk_1, ui_addn_clk_2, ui_addn_clk_3,
          ui_addn_clk_4, app_sr_active, app_ref_ack, app_zq_ack} = '0;

  logic [31:0] mem [0:MEM_WORDS-1];
  integer      errors = 0;
  integer      wb_seen = 0;      // a write has landed at or above WB_REGION

  // ---- reset and calibration ---------------------------------------------
  integer calib_ctr;
  initial begin
    ui_clk_sync_rst     = 1'b1;
    init_calib_complete = 1'b0;
    calib_ctr           = 0;
    for (int i = 0; i < MEM_WORDS; i++) mem[i] = 32'hDEAD_0000 | i[15:0];
  end
  always_ff @(posedge ui_clk) begin
    if (!sys_rst) begin
      ui_clk_sync_rst <= 1'b1; calib_ctr <= 0; init_calib_complete <= 1'b0;
    end else begin
      calib_ctr <= calib_ctr + 1;
      if (calib_ctr == 8)  ui_clk_sync_rst     <= 1'b0;
      if (calib_ctr == 40) init_calib_complete <= 1'b1;
    end
  end

  // ---- write channel ------------------------------------------------------
  int   w_addr, w_left, w_burst_base, w_burst_len;
  logic w_open = 1'b0;

  initial begin
    s_axi_awready = 1'b1; s_axi_wready = 1'b1;
    s_axi_bvalid  = 1'b0; s_axi_bresp  = 2'b00; s_axi_bid = 2'b00;
  end

  always_ff @(posedge ui_clk) begin
    if (ui_clk_sync_rst) begin
      w_open <= 1'b0; s_axi_bvalid <= 1'b0;
    end else begin
      if (s_axi_bvalid && s_axi_bready) s_axi_bvalid <= 1'b0;

      if (s_axi_awvalid && s_axi_awready) begin
        if (w_open) begin
          $display("  FAIL: AW accepted while a burst was still open");
          errors <= errors + 1;
        end
        w_burst_base <= int'(s_axi_awaddr);
        w_burst_len  <= int'(s_axi_awlen) + 1;
        w_addr       <= int'(s_axi_awaddr);
        w_left       <= int'(s_axi_awlen) + 1;
        w_open       <= 1'b1;
        if ((int'(s_axi_awaddr) % 4096) + (int'(s_axi_awlen) + 1) * BPB > 4096) begin
          $display("  FAIL: write burst crosses a 4 KiB page: addr %0d len %0d",
                   s_axi_awaddr, s_axi_awlen + 1);
          errors <= errors + 1;
        end
      end

      if (s_axi_wvalid && s_axi_wready) begin
        if (!w_open) begin
          $display("  FAIL: W beat with no burst open");
          errors <= errors + 1;
        end else begin
          if (s_axi_wlast !== (w_left == 1)) begin
            $display("  FAIL: wlast=%0b with %0d beat(s) left", s_axi_wlast, w_left);
            errors <= errors + 1;
          end
          if (w_addr >= WB_REGION) wb_seen <= 1;
          else if (wb_seen != 0) begin
            $display("  FAIL: a write returned to the operand region (addr %0d) after the write-back region was touched -- the two masters interleaved",
                     w_addr);
            errors <= errors + 1;
          end
          for (int j = 0; j < 4; j++)
            if (s_axi_wstrb[4*j +: 4] == 4'hF)
              mem[w_addr/4 + j] = s_axi_wdata[32*j +: 32];
          w_addr <= w_addr + BPB;
          w_left <= w_left - 1;
          if (s_axi_wlast) begin
            w_open       <= 1'b0;
            s_axi_bvalid <= 1'b1;
          end
        end
      end
    end
  end

  // ---- read channel -------------------------------------------------------
  int q_addr[$], q_len[$];
  int r_addr, r_left;

  function automatic logic [127:0] beat_at(input int byte_addr);
    int wi;
    begin
      wi = byte_addr / 4;
      beat_at = { mem[wi+3], mem[wi+2], mem[wi+1], mem[wi] };
    end
  endfunction

  initial begin
    s_axi_arready = 1'b1; s_axi_rvalid = 1'b0; s_axi_rlast = 1'b0;
    s_axi_rresp = 2'b00; s_axi_rid = 2'b00; s_axi_rdata = '0;
  end

  always_ff @(posedge ui_clk) begin
    if (ui_clk_sync_rst) begin
      s_axi_rvalid <= 1'b0; s_axi_rlast <= 1'b0;
      q_addr.delete(); q_len.delete(); r_addr <= 0; r_left <= 0;
    end else begin
      if (s_axi_arvalid && s_axi_arready) begin
        q_addr.push_back(int'(s_axi_araddr));
        q_len.push_back(int'(s_axi_arlen) + 1);
      end
      if (!s_axi_rvalid || s_axi_rready) begin
        if (s_axi_rvalid && s_axi_rready && (r_left > 1)) begin
          r_addr       <= r_addr + BPB;
          r_left       <= r_left - 1;
          s_axi_rdata  <= beat_at(r_addr + BPB);
          s_axi_rlast  <= (r_left == 2);
          s_axi_rvalid <= 1'b1;
        end else if (q_addr.size() > 0) begin
          r_addr       <= q_addr[0];
          r_left       <= q_len[0];
          s_axi_rdata  <= beat_at(q_addr[0]);
          s_axi_rlast  <= (q_len[0] == 1);
          s_axi_rvalid <= 1'b1;
          q_addr.delete(0);
          q_len.delete(0);
        end else begin
          s_axi_rvalid <= 1'b0;
          s_axi_rlast  <= 1'b0;
        end
      end
    end
  end

endmodule

`default_nettype wire
