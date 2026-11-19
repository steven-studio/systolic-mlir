#include "GeneralizedGemm/PolynomialAdapter.h"

#include "GeneralizedGemm/ControlStructure.h"
#include "GeneralizedGemm/TerminationObligations.h"

#include "Systolic/MLIRPolynomialTermination.h"

#include "llvm/ADT/DenseSet.h"

#include <functional>

using namespace mlir::generalized_gemm;
using llvm::APInt;
namespace p2b = systolic::polynomial::termination;

namespace mlir::generalized_gemm {

std::vector<RankHint> polynomialRankHints(const SemanticProgram &program,
                                          func::FuncOp function,
                                          const ImportOptions &options) {
  std::vector<RankHint> hints;
  if (!program.isSupported() || !matchesSource(program, function, options))
    return hints;
  auto cuts = computeCutSet(program);
  if (cuts.size() != 1)
    return hints;
  TermContext &terms = *program.terms;
  // The model's comparisons at the header, as candidates for the bound.
  std::vector<Term> comparisons;
  llvm::DenseSet<Term> seen;
  std::function<void(Term)> visit = [&](Term t) {
    if (!seen.insert(t).second)
      return;
    if (t->op == TermOp::ICmp && !readsMemory(t))
      comparisons.push_back(t);
    for (Term a : t->args)
      visit(a);
  };
  for (const Transition &t : program.transitions)
    if (t.from == cuts[0])
      visit(t.guard);

  for (const p2b::Certificate &candidate : p2b::generateCandidates(function)) {
    if (candidate.kind != p2b::CertificateKind::LinearRanking)
      continue;
    Term counter = nullptr;
    for (const StateVar &s : program.state)
      if (s.kind == StateVar::BlockArgument && s.location == cuts[0] &&
          s.position == candidate.counter && s.sort.kind == Sort::BV)
        counter = terms.state(s.id, s.sort);
    if (!counter)
      continue;
    unsigned width = counter->sort.width + 2;
    auto wide = [&](Term t) {
      return candidate.signedValues ? terms.sext(t, width) : terms.zext(t, width);
    };
    for (Term comparison : comparisons) {
      Term bound = comparison->args[0] == counter   ? comparison->args[1]
                   : comparison->args[1] == counter ? comparison->args[0]
                                                    : nullptr;
      if (!bound || bound->sort != counter->sort)
        continue;
      Term distance = candidate.ascending
                          ? terms.binary(TermOp::Sub, wide(bound), wide(counter))
                          : terms.binary(TermOp::Sub, wide(counter), wide(bound));
      APInt offset(width, (int64_t)candidate.offset, /*isSigned=*/true);
      hints.push_back({terms.binary(TermOp::Add, distance, terms.bv(offset)),
                       "P2b candidate"});
    }
  }
  return hints;
}

} // namespace mlir::generalized_gemm
