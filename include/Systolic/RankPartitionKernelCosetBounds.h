//===- RankPartitionKernelCosetBounds.h - static access bounds proof -----===//
//
// Phase F-KC-1 (2026-11-01): exact, fail-closed proof that every physical
// access of an operand lies inside its declared STATIC memref extents over the
// EXACT source iteration domain D -- the loop nest's index set with its affine
// guards -- not over a bounding box of the loops and not over the generated
// tiles. The exact kernel-coset emission reproduces the source accesses, so an
// out-of-bounds source access would be reproduced too; a bounded-looking
// generated loop proves nothing about it.
//
// For an operand with access map f(x) = M x + b (one row per memref dimension
// q) and static extents d_q, the requirement is
//
//     forall x in D :  0 <= f_q(x) < d_q      for every q,
//
// equivalently  D ∩ f^{-1}(Z^r \ B) = ∅  with B = prod_q [0, d_q). It is decided
// exactly, dimension by dimension and side by side, as the integer emptiness of
//
//     D ∩ { f_q(x) <= -1 }     and     D ∩ { f_q(x) >= d_q },
//
// with mlir::presburger::IntegerPolyhedron::isIntegerEmpty (exact; no point
// enumeration). Everything the proof cannot establish is Unknown, never
// InBounds: a dynamic (or negative) extent, symbols or locals in D, a row arity
// that does not match D, an access rank that does not match the memref rank.
// An empty D is vacuously in bounds (the emitter refuses empty domains anyway,
// finding F3).
//
//===----------------------------------------------------------------------===//
#pragma once

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DynamicAPInt.h"
#include "llvm/ADT/StringRef.h"

#include <cstdint>
#include <string>
#include <vector>

namespace systolic::logical {

enum class StaticBoundsVerdict { InBounds, OutOfBounds, Unknown };

struct StaticBoundsResult {
  StaticBoundsVerdict verdict = StaticBoundsVerdict::Unknown;
  std::string why;
  // The offending access dimension (OutOfBounds / Unknown with a dimension);
  // -1 when the verdict does not refer to one.
  int dim = -1;
};

/// `rows[q]` holds the n integer coefficients of access dimension q over the
/// source dimensions of `D` (no symbols, no locals), `offsets[q]` its constant,
/// `extents[q]` the memref's static extent (a negative value marks a dynamic
/// extent). `operand` names the operand in the diagnostics.
template <typename Matrix, typename Vector>
StaticBoundsResult
checkStaticAccessBounds(const mlir::presburger::IntegerPolyhedron &D,
                        const Matrix &rows, const Vector &offsets,
                        llvm::ArrayRef<int64_t> extents, llvm::StringRef operand) {
  using llvm::DynamicAPInt;
  using mlir::presburger::IntegerPolyhedron;
  StaticBoundsResult r;
  const std::string op = operand.str();
  if (D.getNumSymbolVars() != 0 || D.getNumLocalVars() != 0) {
    r.why = "the source domain carries symbols or locals; no static bounds "
            "proof for operand " + op;
    return r;
  }
  const unsigned n = D.getNumDimVars();
  if (rows.size() != extents.size() || offsets.size() != rows.size()) {
    r.why = "operand " + op + ": access rank " + std::to_string(rows.size()) +
            " differs from the memref rank " + std::to_string(extents.size());
    return r;
  }
  for (unsigned q = 0; q < rows.size(); ++q) {
    r.dim = static_cast<int>(q);
    if (rows[q].size() != n) {
      r.why = "operand " + op + " index " + std::to_string(q) +
              ": access row arity differs from the source rank";
      return r;
    }
    if (extents[q] < 0) {
      r.why = "operand " + op + " index " + std::to_string(q) +
              ": dynamic memref extent (no static bounds proof in this phase)";
      return r;
    }
    // Layout of an IntegerPolyhedron inequality: [dims..., constant] >= 0.
    std::vector<DynamicAPInt> ineq(n + 1, DynamicAPInt(0));
    // Lower side: D ∩ { f_q(x) <= -1 }  <=>  -f_q(x) - 1 >= 0.
    for (unsigned i = 0; i < n; ++i)
      ineq[i] = -DynamicAPInt(static_cast<int64_t>(rows[q][i]));
    ineq[n] = -DynamicAPInt(static_cast<int64_t>(offsets[q])) - DynamicAPInt(1);
    IntegerPolyhedron below(D);
    below.addInequality(ineq);
    if (!below.isIntegerEmpty()) {
      r.verdict = StaticBoundsVerdict::OutOfBounds;
      r.why = "operand " + op + " index " + std::to_string(q) +
              " is negative at some point of the source domain";
      return r;
    }
    // Upper side: D ∩ { f_q(x) >= d_q }  <=>  f_q(x) - d_q >= 0.
    for (unsigned i = 0; i < n; ++i)
      ineq[i] = DynamicAPInt(static_cast<int64_t>(rows[q][i]));
    ineq[n] = DynamicAPInt(static_cast<int64_t>(offsets[q])) -
              DynamicAPInt(static_cast<int64_t>(extents[q]));
    IntegerPolyhedron above(D);
    above.addInequality(ineq);
    if (!above.isIntegerEmpty()) {
      r.verdict = StaticBoundsVerdict::OutOfBounds;
      r.why = "operand " + op + " index " + std::to_string(q) +
              " reaches or exceeds the extent " + std::to_string(extents[q]) +
              " at some point of the source domain";
      return r;
    }
  }
  r.dim = -1;
  r.verdict = StaticBoundsVerdict::InBounds;
  r.why = "every access of operand " + op +
          " is inside its static extents over the exact source domain";
  return r;
}

} // namespace systolic::logical
