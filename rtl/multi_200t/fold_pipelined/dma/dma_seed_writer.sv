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
// THE PATTERN -- AND WHY THERE ARE TWO OF THEM
//
//   SEED_MODE = 0 (bring-up step 3a, "did the bytes land in the right slot")
//     Word at byte address A holds A/4 - BASE/4, i.e. its own index within the
//     seeded region.  A word that ends up in the wrong operand slot therefore
//     names its own origin, which is the same trick the simulation benches use.
//     This is the pattern behind the recorded golden checksum 0x387fdc00, so it
//     is kept as mode 0 and that result stays reproducible.
//
//   SEED_MODE = 1 (bring-up step 3b, "does the array compute from them")
//     Mode 0 becomes USELESS the moment the systolic array is connected, and
//     quietly so.  Interpreted as fp32, the bit pattern 0x00000001 is 1.4e-45 --
//     every word of a 4096-word region lands in the denormal range.  A hardware
//     fp32 multiplier flushes denormal inputs to zero, so every product is zero,
//     every accumulation is zero, and a golden model computed from the same seed
//     agrees perfectly.  The test passes and distinguishes nothing: "the data
//     arrived and was multiplied correctly" and "no data arrived at all" produce
//     the identical all-zero result.
//
//     So mode 1 seeds the fp32 values of the integers 1..127, chosen for the two
//     properties a bit-exact comparison needs:
//
//       exact    every product is at most 127*127 = 16129 and every dot product
//                over K = 256 is at most 4,129,024 < 2^24, so every partial sum
//                is exactly representable.  The reduction tree's ADDITION ORDER
//                therefore cannot change the result, and a bit-exact comparison
//                is legitimate rather than a source of false alarms.
//
//       placed   the value is (word index mod 127) + 1.  127 is coprime with the
//                64-word chunk and with the 128-word k window, so two slots one
//                or more k windows apart never carry the same value (K_MAX = 256
//                gives 32 windows, far short of 127).  A misplaced word changes
//                the dot product it lands in.
//
//     Never zero, so no operand can silently drop out of a product.
//
//     tools/seed_ref.py generates the identical stream in software, so the UART
//     host can send a byte-identical payload and the two load paths can be
//     compared on the same bitstream.
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
  parameter integer TOTAL_BEATS = 1024,     // beats to write, multiple of BURST_LEN
  parameter integer SEED_MODE  = 0,         // 0 = word index (3a), 1 = fp32 1..MODULUS (3b)
  parameter integer MODULUS    = 127        // mode 1 values are 1..MODULUS
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
  localparam integer MOD_W          = $clog2(MODULUS + 1);

  localparam [2:0] SIZE_FULL  = 3'(LSB);
  localparam [1:0] BURST_INCR = 2'b01;

  typedef enum logic [1:0] {S_IDLE, S_AW, S_W, S_B} state_t;
  state_t state;

  logic [31:0] burst_left;
  logic [31:0] beat_idx;        // global beat index within the seeded region
  logic [7:0]  beat_left;
  logic [MOD_W-1:0] vbase;      // (beat_idx * WPB) mod MODULUS, mode 1 only

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

  // ---- fp32 of a small positive integer ----------------------------------
  // v is at most MODULUS (127 by default), so the leading one is within seven
  // bits: priority-encode its position e, bias the exponent, and left-align the
  // bits below it.  Shifting v left by (23 - e) puts that leading one at bit 23,
  // which the 23-bit mantissa slice drops -- exactly the implicit-one rule.
  //
  //   v = 1   -> 0x3F800000 = 1.0        v = 3   -> 0x40400000 = 3.0
  //   v = 2   -> 0x40000000 = 2.0        v = 127 -> 0x42FE0000 = 127.0
  //
  // No rounding is possible: a 7-bit integer needs 6 mantissa bits and fp32 has
  // 23, so this is exact by construction, not by luck.
  function automatic logic [31:0] fp32_small(input logic [6:0] v);
    logic [2:0]  e;
    logic [31:0] shifted;
    begin
      if      (v[6]) e = 3'd6;
      else if (v[5]) e = 3'd5;
      else if (v[4]) e = 3'd4;
      else if (v[3]) e = 3'd3;
      else if (v[2]) e = 3'd2;
      else if (v[1]) e = 3'd1;
      else           e = 3'd0;          // v = 1; v = 0 never occurs
      // Masks and shifts rather than a part-select, purely to keep this
      // readable next to the exponent arithmetic.
      //
      // Icarus prints "sorry: constant selects in always_* processes are not
      // currently supported (all bits will be included)" six times here, once
      // per v[6]..v[1] above.  That message is about the process's implicit
      // SENSITIVITY LIST, not about the value: it cannot make the block
      // sensitive to one bit, so it makes it sensitive to all of v.  For an
      // always_comb that is what we want anyway.  The values are unaffected --
      // tb_dma_path checks every seeded DRAM word against an independently
      // written model and passes across all 127 values, which is the evidence,
      // not this comment.
      shifted    = 32'(v) << (5'd23 - 5'(e));
      fp32_small = ((32'd127 + 32'(e)) << 23) | (shifted & 32'h007F_FFFF);
    end
  endfunction

  // ---- the beat's WPB words ----------------------------------------------
  // Mode 0: word i carries i.  Mode 1: word i carries fp32((i mod MODULUS) + 1),
  // where vbase tracks (beat_idx * WPB) mod MODULUS so no divider is needed.
  // vbase < MODULUS and j < WPB <= 4, so vbase + j < MODULUS + 4 and a single
  // conditional subtract brings it back into range.
  // Plain integer arithmetic, no width casts -- the casts this replaced added
  // nothing but noise here.  vbase < MODULUS and j < WPB, and MODULUS is a
  // constant, so synthesis trims these back to the handful of bits they
  // actually need; this block runs once at startup, so its area is irrelevant.
  integer vsum [0:WPB-1];
  always_comb begin
    m_axi_wdata = '0;
    for (int j = 0; j < WPB; j++) begin
      vsum[j] = vbase + j;
      if (vsum[j] >= MODULUS) vsum[j] = vsum[j] - MODULUS;
      m_axi_wdata[32*j +: 32] = (SEED_MODE == 0)
                              ? (beat_idx * WPB + j)
                              : fp32_small(vsum[j] + 1);
    end
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
      vbase         <= '0;
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
              vbase         <= '0;
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
            // vbase advances by one beat's worth of words, mod MODULUS.  It is
            // updated in lockstep with beat_idx, so the two never disagree about
            // which words the next beat carries.
            vbase <= ((MOD_W+1)'(vbase) + (MOD_W+1)'(WPB) >= (MOD_W+1)'(MODULUS))
                   ? MOD_W'((MOD_W+1)'(vbase) + (MOD_W+1)'(WPB) - (MOD_W+1)'(MODULUS))
                   : MOD_W'((MOD_W+1)'(vbase) + (MOD_W+1)'(WPB));
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

  // MODULUS must fit the 7-bit fp32_small input, and WPB must be small enough
  // that one conditional subtract closes the modulo.  Both hold by default;
  // catch a bad override at elaboration rather than on the board.
  initial begin
    if (SEED_MODE == 1) begin
      if (MODULUS > 127 || MODULUS < 2)
        $fatal(1, "dma_seed_writer: MODULUS must be 2..127, got %0d", MODULUS);
      if (WPB > MODULUS)
        $fatal(1, "dma_seed_writer: WPB %0d exceeds MODULUS %0d", WPB, MODULUS);
    end
  end

endmodule

`default_nettype wire
