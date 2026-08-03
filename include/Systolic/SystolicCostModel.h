//===- SystolicCostModel.h - Calibrated cost model for the PE array -------===//
//
// Deliberately free of MLIR dependencies so it can be unit-tested against
// the raw HLS sweep numbers without building the dialect. The pass is just
// one caller.
//
// LATENCY
//   PE(i,j) performs its k-th MAC on beat i+j+k, so the time loop runs
//   K_t + R + C - 2 beats. At an initiation interval of II that costs
//
//       cycles_tile = II * (K_t + R + C - 2) + depth
//
//   Calibrated by C/RTL co-simulation over 14 (R, C, K_t) configurations:
//   II = 1, depth = 6, residual exactly zero at every point. TIME_STEPS
//   spanned 8..70; R+C spanned 4..34. Crucially the sweep included
//   non-square arrays (8x8, 16x4, 32x2 all hold 64 PEs while R+C goes
//   16 -> 20 -> 34), which separates the geometric term from array size,
//   and three transposed pairs that produced identical cycle counts --
//   so the fill/drain cost really does depend on R+C alone.
//
//   depth is not derived; it absorbs the init/drain loops and the
//   interface handshake, and is a fitted constant.
//
// RESOURCES
//       DSP = kDspPerPE * R * C          (kDspPerPE = 5 for fp32)
//
//   Exact at all 15 synthesised points. Five is the DSP48 cost of one
//   fp32 multiplier; the fp32 adder is mapped to LUTs and so does not
//   appear. LUT usage also grows with K_t (the west/north edge PEs need
//   a wider mux to select A[i][k]) and is not modelled here -- DSP is
//   the binding resource in every configuration measured, with LUT
//   staying under 20%.
//
// SCOPE
//   These constants describe the fp32 output-stationary kernel in
//   hls/gemm_4x4/design.cpp built at II=1. A different element type,
//   a different II directive, or a different part invalidates them --
//   hence CostModelParams rather than hard-coded literals.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_COST_MODEL_H
#define SYSTOLIC_COST_MODEL_H

#include <cstdint>
#include <limits>
#include <vector>

namespace systolic {
namespace cost {

inline int64_t ceilDiv(int64_t a, int64_t b) { return (a + b - 1) / b; }

/// Calibrated constants. Defaults are the fp32 / II=1 / xc7a200t figures.
struct CostModelParams {
  int64_t ii = 1;         ///< achieved initiation interval of the time loop
  int64_t depth = 6;      ///< fitted constant: init + drain + handshake
  int64_t dspPerPE = 5;   ///< DSP48s per PE (fp32 multiplier)
  int64_t dspBudget = 740;///< available DSP48s on the target part

  /// Per-tile dispatch cost in accelerator-clock cycles. Zero by default:
  /// the model then describes the accelerator in isolation. On the current
  /// UART link one 4x4 tile costs 48 bytes out plus 16 back at 115200 baud
  /// -- about 5.6 ms, or ~556000 cycles at 100 MHz, which is four orders
  /// of magnitude above the compute. Set this when ranking end-to-end
  /// wall-clock rather than accelerator work, and expect it to dominate.
  int64_t dispatchCyclesPerTile = 0;
};

/// Beats of the time loop for one tile: K_t + R + C - 2.
inline int64_t timeSteps(int64_t r, int64_t c, int64_t kt) {
  return kt + r + c - 2;
}

/// Cycles to execute one R x C x K_t tile.
inline int64_t tileCycles(int64_t r, int64_t c, int64_t kt,
                          const CostModelParams &p = {}) {
  return p.ii * timeSteps(r, c, kt) + p.depth;
}

/// Number of tiles an M x K by K x N product decomposes into.
inline int64_t tileCount(int64_t m, int64_t n, int64_t k,
                         int64_t r, int64_t c, int64_t kt) {
  return ceilDiv(m, r) * ceilDiv(n, c) * ceilDiv(k, kt);
}

/// Total cycles for the whole GEMM under a given array shape.
inline int64_t gemmCycles(int64_t m, int64_t n, int64_t k,
                          int64_t r, int64_t c, int64_t kt,
                          const CostModelParams &p = {}) {
  return tileCount(m, n, k, r, c, kt) *
         (tileCycles(r, c, kt, p) + p.dispatchCyclesPerTile);
}

inline int64_t dspUsage(int64_t r, int64_t c, const CostModelParams &p = {}) {
  return p.dspPerPE * r * c;
}

inline bool fitsBudget(int64_t r, int64_t c, const CostModelParams &p = {}) {
  return dspUsage(r, c, p) <= p.dspBudget;
}

/// im2col maps conv2d onto a GEMM. NHWC input, HWCF filter.
///   M = N_batch * H_out * W_out,  N = C_out,  K = C_in * K_h * K_w
struct GemmShape {
  int64_t m, n, k;
};

inline int64_t convOutDim(int64_t in, int64_t padLo, int64_t padHi,
                          int64_t kern, int64_t dilation, int64_t stride) {
  int64_t eff = (kern - 1) * dilation + 1;
  return (in + padLo + padHi - eff) / stride + 1;
}

struct ArrayConfig {
  int64_t r = 0, c = 0, kt = 0;
  int64_t cycles = 0;
  int64_t dsp = 0;

  bool valid() const { return r > 0; }
};

/// Exhaustive search over the candidate shapes that fit the DSP budget.
///
/// Note this is NOT simply "make the array as large as possible". Growing
/// R+C raises the per-tile fill/drain cost, while growing R or C cuts the
/// tile count only when it actually divides M or N -- so for a skewed GEMM
/// a non-square array can beat the largest square one that fits. That
/// trade-off is the whole reason the pass consults a cost model instead of
/// hard-coding a shape.
inline ArrayConfig
selectBest(const GemmShape &g, const std::vector<int64_t> &rCandidates,
           const std::vector<int64_t> &cCandidates,
           const std::vector<int64_t> &ktCandidates,
           const CostModelParams &p = {}) {
  ArrayConfig best;
  int64_t bestCycles = std::numeric_limits<int64_t>::max();
  for (int64_t r : rCandidates) {
    for (int64_t c : cCandidates) {
      if (!fitsBudget(r, c, p))
        continue;
      for (int64_t kt : ktCandidates) {
        int64_t cyc = gemmCycles(g.m, g.n, g.k, r, c, kt, p);
        if (cyc < bestCycles) {
          bestCycles = cyc;
          best = ArrayConfig{r, c, kt, cyc, dspUsage(r, c, p)};
        }
      }
    }
  }
  return best;
}

} // namespace cost
} // namespace systolic

#endif // SYSTOLIC_COST_MODEL_H
