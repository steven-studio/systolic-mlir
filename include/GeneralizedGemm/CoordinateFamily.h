//===- CoordinateFamily.h - Coordinates as terms ----------------*- C++ -*-===//
//
// G3b. A family gives P, I, J, K as tuples of terms over state and parameters,
// so one family has a meaning at every parameter point. A proposal is data: only
// SymbolicCandidateChecker can say anything about it. See
// docs/GENERALIZED_GEMM_G3B.md.
//
// Terms use G1's total operators at exact widths. A term's value says nothing
// about whether its source-level reading is defined, so `definedness` is separate
// and the checker demands it on the domain. Labels are only ever compared for
// equality, at the exact width, so wrapping cannot hide an error.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COORDINATEFAMILY_H
#define GENERALIZEDGEMM_COORDINATEFAMILY_H

#include "GeneralizedGemm/ConcreteEvaluator.h"
#include "GeneralizedGemm/CoordinateModel.h"
#include "GeneralizedGemm/SemanticProgram.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

enum AxisIndex { AxisP = 0, AxisI = 1, AxisJ = 2, AxisK = 3 };

struct CoordinateFamily {
  std::array<std::vector<Term>, 4> axes; // P, I, J, K. A tuple per axis.
  std::string origin;                    // Who proposed it. Not evidence.
};

/// One level of a lexicographic enumeration: lower <= value < upper, unsigned.
/// `lower` and `upper` read parameters only; `upperOuter` adds the value of an
/// earlier level to `upper`, which is how a triangular nest is written.
struct OrderLevel {
  Term value = nullptr, lower = nullptr, upper = nullptr;
  int upperOuter = -1;
};

/// Levels from slowest to fastest. Untrusted, like a family.
struct EnumerationOrder {
  std::vector<OrderLevel> levels;
};

/// True when every division has a nonzero divisor, no signed division overflows
/// and every shift is below the width.
Term definedness(TermContext &terms, Term term);

/// Evaluates terms at the events of one exact domain. x is state by name, so a
/// term reading a variable outside the table has no value.
class EventEvaluator {
public:
  EventEvaluator(const SemanticProgram &program, const AccessDomain &domain);
  std::optional<llvm::APInt> value(Term term, size_t event);
  size_t size() const { return domain.table().events().size(); }

private:
  const SemanticProgram &program;
  const AccessDomain &domain;
  EntryState entry;
  Configuration config;
  size_t loaded = SIZE_MAX;
};

/// Per-event labels for the family. Nullopt with `why` when a term has no value
/// or is undefined at an event, which names that event.
std::optional<CoordinateCandidate> familyLabels(const SemanticProgram &program,
                                                const AccessDomain &domain,
                                                const CoordinateFamily &family,
                                                std::string &why);

} // namespace mlir::generalized_gemm
#endif
