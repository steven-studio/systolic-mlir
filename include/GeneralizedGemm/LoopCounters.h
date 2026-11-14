//===- LoopCounters.h - Iteration counters as ghost state --------*- C++ -*-===//
//
// Phase 4 of the bit-vector adaptation (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
// integration item I1). The iteration count m of the thesis's specification (F.2)
// is made ordinary state: for every loop header h of the cut set, a fresh state
// variable c_h is added that is set to 0 on every transition into h from outside
// the natural loop of h and to c_h + 1 on every transition into h from inside it.
// Nothing reads c_h, so every run of the instrumented program performs the same
// transitions, events, memory effects and returns as the original: the
// extension is semantics-preserving by construction, and compareRuns checks it
// on concrete entries, which is test evidence for those entries.
//
// With c_h in the state, the coordinate search may choose it as a coordinate,
// the order proposers see a unit-step variable, and a proved closed form v = f(m)
// of a loop variable becomes the state identity v = f(c_h), which the domain
// proposer may use for guards and views. The checkers are not changed: c_h is
// state like any other, at its exact width, wrapping at 2^w iterations.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_LOOPCOUNTERS_H
#define GENERALIZEDGEMM_LOOPCOUNTERS_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/SemanticProgram.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct LoopCounter {
  unsigned header = 0;         // The cut location whose arrivals it counts.
  unsigned state = 0;          // The state id of c_h.
  std::vector<unsigned> body;  // The natural loop of the header.
};

struct CounterInstrumentation {
  std::vector<LoopCounter> counters;
  std::string reason; // Nonempty when nothing was added (unsupported, no cut set, no loop).
};

/// Adds one counter per cut of computeCutSet, at the index width (or 64 when
/// unspecified), and records them in program.iterationCounters. A program that
/// already has counters is left alone.
CounterInstrumentation addIterationCounters(SemanticProgram &program);

/// Runs `original` and `instrumented` side by side from one entry: same
/// transitions, same events, same returns and status, and at every arrival at a
/// header the counter equals the number of arrivals since the loop was entered,
/// which is the m of IndexSlice.h's specification. Evidence for this entry only.
struct CounterReplay {
  bool equal = false;
  std::string reason;
  uint64_t steps = 0, events = 0, headerArrivals = 0;
};
CounterReplay compareRuns(const SemanticProgram &original, const SemanticProgram &instrumented,
                          const CounterInstrumentation &instrumentation, const EntryState &entry,
                          const RunLimits &limits);

} // namespace mlir::generalized_gemm
#endif
