//===- GeneralAffineMatmul.h - Finite affine MAC analysis ---------*- C++
//-*-===//
#ifndef SYSTOLIC_GENERAL_AFFINE_MATMUL_H
#define SYSTOLIC_GENERAL_AFFINE_MATMUL_H

#include "Systolic/AffineMatmulAnalysis.h"
#include "mlir/Analysis/Presburger/PresburgerRelation.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include <memory>

namespace mlir::systolic {
/// Row spaces are over Q, represented by independent primitive integer rows.
/// Constants are NOT coordinates of these spaces. Quotient complements are
/// deterministic representatives, not set differences or canonical bases.
struct OperandSubspaces {
  std::array<lattice::IntMatrix, 3> operand; // A, B, C
  std::array<lattice::IntMatrix, 3> pair;    // AB, AC, BC
  lattice::IntMatrix shared;
  // Quotient representatives A/(A intersect (B+C)), likewise B and C.
  std::array<lattice::IntMatrix, 3> exclusive;
  // Dual ambient complement and primal annihilator are not interchangeable.
  lattice::IntMatrix unusedComplement;
  lattice::IntMatrix unobservedKernel;
  std::array<lattice::IntMatrix, 3> core; // AC/shared, BC/shared, AB/shared
  lattice::IntMatrix residual;            // ambient / span(core, shared)
  lattice::IntMatrix outputKernel;
  lattice::IntMatrix
      coordinateFrame; // independent core/shared + residual; full rank
  std::array<unsigned, 7> ranks{}; // A,B,C,ABsum,ACsum,BCsum,ABCsum
  bool independentCore = false;
  // x=V(P*,I,J,K). Inverse rows use exact positive denominators. The stored
  // integer logical point is the inverse numerator; no divisibility is assumed.
  lattice::IntMatrix parametricSourceFrame;
  std::array<lattice::IntMatrix, 3> parametricOperandMaps;
  SmallVector<int64_t> frameDenominators;
  bool hasParametricCore = false;
};

struct GeneralAffinePoint {
  SmallVector<int64_t> source;  // full lexical SSA universe, even unused IVs
  SmallVector<int64_t> logical; // exact integer coordinateFrame * source
  std::array<SmallVector<int64_t>, 3> address; // exact physical A,B,C indices
};

/// A finite exact decomposition. Every source point occurs once in points,
/// and once in fibers. Each fiber has one physical C address and strictly
/// increasing source-execution ordinals. No cross-fiber write dependences.
/// sourceToJobs retains source witnesses (hence multiplicity and lattices),
/// rather than projecting to a bounding box or deduplicating MAC statements.
struct GeneralAffineMatmul {
  Operation *anchor = nullptr;
  SmallVector<affine::AffineForOp> loops;
  std::array<affine::AffineLoadOp, 3> loads; // A, B, C
  affine::AffineStoreOp store;
  arith::MulFOp multiply;
  arith::AddFOp add;
  OperandSubspaces spaces;
  std::array<AffineMap, 3> access;
  std::array<lattice::IntVector, 3>
      accessOffsets; // constants, separate from rows
  std::shared_ptr<affine::FlatAffineValueConstraints> sourceDomain;
  std::shared_ptr<presburger::PresburgerRelation> sourceToJobs;
  std::shared_ptr<presburger::PresburgerRelation> sourceToLogical;
  SmallVector<GeneralAffinePoint, 0> points;
  SmallVector<SmallVector<unsigned>> fibers;
};

/// Analysis only: never mutates IR. Bounded exact decision procedure for one
/// linear-index MAC in a finite unit-step affine loop/then-guard chain. Unknown
/// symbols, non-linear access forms, overflow, ambiguity and budgets refuse.
FailureOr<GeneralAffineMatmul>
analyzeGeneralAffineMatmul(Operation *anchor, std::string &why,
                           unsigned pointBudget = 512);
void describeOperandSubspaces(const OperandSubspaces &spaces,
                              llvm::raw_ostream &os);
} // namespace mlir::systolic
#endif
