// -----------------------------------------------------------------------------
// dma_seed_writer.sv -- fill a region of DDR3 with a known pattern, over AXI4.
//
// WHY THIS EXISTS
//   Board bring-up step 3a wants to prove that a known image travels
//   DRAM -> MIG -> dma_engine -> dma_operand_writer -> operand buffers.  Nothing
//   in this design can put a known image into DRAM: the read path is all that
//   has been built, and DDR3 after calibration holds whatever it holds.  Without
//   a seed, the only thing 3a could report is "something moved".
//
//   So this is the minimum AXI4 write master that seeds one: one burst at a
//   time, one outstanding, no scatter.  Speed does not matter -- it runs once at
//   startup and 16 KiB takes about two thousand cycles.
//
//   It is not a detour.  The writeback path (bring-up step 4, results going back
//   out to DRAM) needs an AXI write master anyway; this is that master's simple
//   ancestor, and its bench is the one that will grow.
//
// THE PATTERN
//   Word at byte address A holds A/4 - BASE/4, i.e. its own index within the
//   seeded region.  A word that ends up in the wrong operand slot therefore
//   names its own origin, which is the same trick the simulation benches use.
//
// WHAT IS DELIBERATELY NOT HERE
//   No bursts crossing 4 KiB (BEATS_PER_BURST * bytes-per-beat is 256 B and the
//   base is required to be burst-aligned -- checked, and refused if not), no
//   write strobes other than all-ones, no interleaving, no error recovery
//   beyond latching bresp.  Every one of those is a thing the writeback path
//   will need and this one does not.
// -----------------------------------------------------------------------------

`default_nettype none

module dma_seed_writer #(
  parameter integer AXI_DATA_W = 128,
  parameter integer AXI_ADDR_W = 29,
  parameter integer AXI_ID_W   = 2,
  parameter integer BURST_LEN  = 16,        // beats per burst
  parameter integer TOTAL_BEATS = 1024      // beats to write, multiple of BURST_LEN
) (
  input  wire                     clk,
  input  wire                     rst_n,

  input  wire                     start,          // one-cycle pulse
  input  wire [AXI_ADDR_W-1:0]    base_addr,      // must be BURST_LEN*bytes aligned
  output logic                    busy,
  output logic                    done,           // one-cycle pulse
  output logic                    err_align,
  output logic                    err_resp,

  // ---- AXI4 write address channel ----------------------------------------
  output wire  [AXI_ID_W-1:0]     m_axi_awid,
  output logic [AXI_ADDR_W-1:0]   m_axi_awaddr,
  output wire  [7:0]              m_axi_awlen,
  output wire  [2:0]              m_axi_awsize,
  output wire  [1:0]              m_axi_awburst,
  output wire  [0:0]              m_axi_awlock,
  output wire  [3:0]              m_axi_awcache,
  output wire  [2:0]              m_axi_awprot,
  output wire  [3:0]              m_axi_awqos,
  output logic                    m_axi_awvalid,
  input  wire                     m_axi_awready,

  // ---- AXI4 write data channel -------------------------------------------
  output logic [AXI_DATA_W-1:0]   m_axi_wdata,
  output wire  [AXI_DATA_W/8-1:0] m_axi_wstrb,
  output logic                    m_axi_wlast,
  output logic                    m_axi_wvalid,
  input  wire                     m_axi_wready,

  // ---- AXI4 write response channel ---------------------------------------
  input  wire  [AXI_ID_W-1:0]     m_axi_bid,
  input  wire  [1:0]              m_axi_bresp,
  input  wire                     m_axi_bvalid,
  output wire                     m_axi_bready
);

  localparam integer BYTES_PER_BEAT = AXI_DATA_W / 8;
  localparam integer LSB            = $clog2(BYTES_PER_BEAT);
  localparam integer BURST_BYTES    = BURST_LEN * BYTES_PER_BEAT;
  localparam integer N_BURSTS       = TOTAL_BEATS / BURST_LEN;
  localparam integer WPB            = AXI_DATA_W / 32;   // words per beat

  localparam [2:0] SIZE_FULL  = 3'(LSB);
  localparam [1:0] BURST_INCR = 2'b01;

  typedef enum logic [1:0] {S_IDLE, S_AW, S_W, S_B} state_t;
  state_t state;

  logic [31:0] burst_left;
  logic [31:0] beat_idx;        // global beat index within the seeded region
  logic [7:0]  beat_left;

  assign m_axi_awid    = '0;
  assign m_axi_awlen   = 8'(BURST_LEN - 1);
  assign m_axi_awsize  = SIZE_FULL;
  assign m_axi_awburst = BURST_INCR;
  assign m_axi_awlock  = 1'b0;
  assign m_axi_awcache = 4'b0011;
  assign m_axi_awprot  = 3'b000;
  assign m_axi_awqos   = 4'b0000;
  assign m_axi_wstrb   = '1;
  assign m_axi_bready  = 1'b1;

  assign busy = (state != S_IDLE);

  // The four (or more) 32-bit words of the beat at global index i are
  // i*WPB .. i*WPB + WPB-1, laid out low lane first -- byte order on the wire.
  always_comb begin
    m_axi_wdata = '0;
    for (int j = 0; j < WPB; j++)
      m_axi_wdata[32*j +: 32] = beat_idx * WPB + 32'(j);
  end

  always_ff @(posedge clk or negedge rst_n) begin
    if (!rst_n) begin
      state         <= S_IDLE;
      m_axi_awvalid <= 1'b0;
      m_axi_wvalid  <= 1'b0;
      m_axi_wlast   <= 1'b0;
      m_axi_awaddr  <= '0;
      burst_left    <= '0;
      beat_idx      <= '0;
      beat_left     <= 8'd0;
      done          <= 1'b0;
      err_align     <= 1'b0;
      err_resp      <= 1'b0;
    end else begin
      done <= 1'b0;

      if (m_axi_bvalid && (m_axi_bresp != 2'b00)) err_resp <= 1'b1;

      case (state)
        S_IDLE: begin
          if (start) begin
            // A base that is not burst-aligned would put a burst across a 4 KiB
            // boundary, which AXI4 forbids and MIG will not fix.  Refuse rather
            // than seed a region that is quietly wrong.
            if (base_addr[$clog2(BURST_BYTES)-1:0] != '0) begin
              err_align <= 1'b1;
            end else begin
              m_axi_awaddr  <= base_addr;
              burst_left    <= N_BURSTS;
              beat_idx      <= '0;
              m_axi_awvalid <= 1'b1;
              state         <= S_AW;
            end
          end
        end

        S_AW: begin
          if (m_axi_awready) begin
            m_axi_awvalid <= 1'b0;
            beat_left     <= 8'(BURST_LEN - 1);
            m_axi_wvalid  <= 1'b1;
            m_axi_wlast   <= (BURST_LEN == 1);
            state         <= S_W;
          end
        end

        S_W: begin
          if (m_axi_wready) begin
            beat_idx <= beat_idx + 1'b1;
            if (beat_left == 0) begin
              m_axi_wvalid <= 1'b0;
              m_axi_wlast  <= 1'b0;
              state        <= S_B;
            end else begin
              beat_left   <= beat_left - 1'b1;
              m_axi_wlast <= (beat_left == 1);
            end
          end
        end

        S_B: begin
          if (m_axi_bvalid) begin
            if (burst_left == 1) begin
              done  <= 1'b1;
              state <= S_IDLE;
            end else begin
              burst_left    <= burst_left - 1'b1;
              m_axi_awaddr  <= m_axi_awaddr + AXI_ADDR_W'(BURST_BYTES);
              m_axi_awvalid <= 1'b1;
              state         <= S_AW;
            end
          end
        end

        default: state <= S_IDLE;
      endcase
    end
  end

endmodule

`default_nettype wire
