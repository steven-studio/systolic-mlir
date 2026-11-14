//===- IndexProgramSemgus.h - SemGuS-Lib style text ------------*- C++ -*-===//
//
// The three parts of a semantics-guided synthesis problem (Appendix G): the
// grammar as term types with one constructor per production, the semantics as
// one recursive Boolean function per nonterminal, and the specification as
// constraints, here the input-output examples of the sampled events and, when a
// candidate is given, that candidate as a defined term.
//
// Status, stated in the text itself: the output follows the structure the thesis
// describes for the SemGuS-Lib format, and NO external SemGuS parser or solver has
// read it. Nothing here talks to a solver. The only mechanical check is the
// round trip below: a candidate printed in constructor form is parsed back and
// runs the same, which checks this file against itself and nothing else.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_INDEXPROGRAMSEMGUS_H
#define GENERALIZEDGEMM_INDEXPROGRAMSEMGUS_H

#include "GeneralizedGemm/IndexProgram.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

struct SemgusExample {
  std::vector<llvm::APInt> inputs;
  llvm::APInt output;
};

/// Phase 5 (G.5): an array of the problem's grammar, typed by rank and extents.
/// `extents` are SMT-LIB texts over the input names, one per dimension.
struct SemgusArray {
  unsigned rank = 1;
  bool input = false;
  std::vector<std::string> extents;
};

struct SemgusProblem {
  unsigned width = 0;                   // One width for every integer.
  std::vector<std::string> inputNames;  // x0, x1, ... by default.
  unsigned locals = 1;
  std::vector<llvm::APInt> constants;   // The K of the grammar, at `width`.
  std::vector<llvm::APInt> divisors;    // The C of div and mod, nonzero.
  std::vector<SemgusArray> arrays;      // T0, T1, ... (phase 5).
  std::vector<SemgusExample> examples;
  // Pre-rendered constraint bodies (SMT-LIB over the declared variables), e.g.
  // the inductive specification of G.3 from semgusInductiveConstraints.
  std::vector<std::string> constraints;
  std::vector<std::string> declaredVariables; // (declare-var name sort) pairs as "name sort".
  const IndexProgram *candidate = nullptr; // Optional: printed as a define-fun.
};

/// The candidate in constructor form, e.g. (return (if (eq x0 (_ bv0 8)) ...) v0);
/// arrays as (read T0 e...) and (write T0 e... e).
std::string semgusTerm(const IndexProgram &program);

/// Parses constructor form over the given inputs and locals. Nullopt with `why`
/// on malformed text, an unknown symbol or an ill-sorted node.
std::optional<IndexProgram> parseSemgusTerm(const std::string &text, llvm::ArrayRef<Term> inputs,
                                            llvm::ArrayRef<unsigned> localWidths,
                                            std::string &why);

/// Phase 5: the same over a skeleton that also declares the arrays (its body and
/// result are replaced; inputs, locals and arrays are kept).
std::optional<IndexProgram> parseSemgusTerm(const std::string &text, const IndexProgram &skeleton,
                                            std::string &why);

/// The whole problem as text, with the status header.
std::string exportSemgusProblem(const SemgusProblem &problem);

/// Phase 5, a faithfulness self-check: every constructor the candidate uses
/// ($add, $read_T0, $assign_v0, ...) appears in the exported declare-term-types
/// and has a clause in the exported define-funs-rec; the missing ones are
/// returned. A syntactic check of this file's output against the candidate; it
/// does not parse the semantics and is not an external validation.
std::vector<std::string> semgusExportMissing(const IndexProgram &candidate, const std::string &text);

/// The constructors a program uses, in the export's names.
std::vector<std::string> semgusConstructors(const IndexProgram &program);

/// A BV term as SMT-LIB text, with `leafName` for Param and State leaves; a
/// poison shadow reads as false (the specification describes runs with no
/// undefined behaviour); nullopt for an operator the export does not carry
/// (memory, floats, shifts, width changes).
std::optional<std::string> semgusTermText(Term term, const std::function<std::string(Term)> &leafName);

/// Whether the parentheses of `text` balance and every top-level form is one
/// of the commands the export writes. A syntax check of this file's own output,
/// not a SemGuS-Lib parser.
bool semgusExportWellFormed(const std::string &text, std::string &why);

} // namespace mlir::generalized_gemm
#endif
