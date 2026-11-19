//===- CoordinateGrammar.h - Untrusted atom enumeration ---------*- C++ -*-===//
//
// Bottom-up enumeration of terms over live state and parameters, in the program's
// own TermContext. Everything here is a guess. Deduplication by kernel over the
// sampled events is an economy of the search: two atoms equal on the samples may
// differ elsewhere, which only costs the search a candidate.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_COORDINATEGRAMMAR_H
#define GENERALIZEDGEMM_COORDINATEGRAMMAR_H

#include "GeneralizedGemm/CoordinateFamily.h"
#include "GeneralizedGemm/FiniteDomain.h"

#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// A configurable grammar. No fixed grammar expresses every program's functions.
struct GrammarSpec {
  unsigned level = 0;
  bool addition = false, subtraction = false, constantProduct = false;
  bool product = false, division = false, remainder = false;
  bool comparison = false, conditional = false, bits = false;
  unsigned maxSize = 1, maxOperands = 300, maxAtoms = 40;
  std::vector<llvm::APInt> extraConstants;
  // Statement programs (IndexProgram.h): the bit-vector adaptation of the thesis's
  // statement grammar, loop-free in this phase. A program is `if B then v := E
  // else v := E ; return v` over the operands of the level, nested up to
  // maxConditionals, lowered to a term and offered like any other. Off at every
  // level: SynthesisOptions turns it on, and the default output does not change.
  bool statements = false;
  unsigned maxConditionals = 1, maxConditions = 400, maxPrograms = 4000;
  // Program atoms have their own cap, so they never crowd out the terms.
  unsigned maxProgramAtoms = 24;
  // Operands are told apart by their values at a thin sample of the events, and an operand
  // equal to an earlier one there is dropped. With exactLeaves, a state variable equal to an
  // earlier operand on the sample is compared on every event, and kept unless they are equal
  // there: a sample that meets an innermost loop at one
  // phase sees its counter constant, equal there to a constant held in state (an scf.for bound
  // defined before the nest), and would lose the counter. Without it, such leaves are counted
  // (AtomBank::lostLeaves). The search turns it on only when its search with the thin sample lost
  // such a state variable and verified no candidate at the symbolic tier, on a scope enumerated
  // whole, and keeps what it finds only when it verifies one there (SynthesisAnalysis.cpp,
  // synthesizeGroup).
  bool exactLeaves = false;

  /// 0: state and access subterms. 1: add, subtract, constant multiples.
  /// 2: products, division, remainder. 3: comparison, if-then-else. 4: bits.
  static GrammarSpec atLevel(unsigned level);
  static constexpr unsigned MaxLevel = 4;
};

struct Atom {
  Term term = nullptr;
  unsigned size = 1;
  // The statement program this term was lowered from, in the text of
  // formatIndexProgram, when there is one. Documentation of the proposal, not
  // evidence: the checkers judge the term.
  std::string program;
  // One vector per domain, one value per event, compared for equality only.
  std::vector<std::vector<uint64_t>> values;
};

struct AtomBank {
  std::vector<Atom> atoms;
  unsigned operands = 0;
  unsigned programs = 0;  // Statement programs lowered and offered.
  // State variables the thin sample took for an earlier operand they differ from at some
  // event, and dropped (without GrammarSpec::exactLeaves; with it, they are kept).
  unsigned lostLeaves = 0;
  std::string truncated; // Nonempty: a limit cut the enumeration short.
};

AtomBank buildAtoms(const SemanticProgram &program, const SiteGroup &group,
                    llvm::ArrayRef<const AccessDomain *> domains, const GrammarSpec &spec);

} // namespace mlir::generalized_gemm
#endif
