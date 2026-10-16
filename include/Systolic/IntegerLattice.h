//===- IntegerLattice.h - Exact integer linear algebra ----------*- C++ -*-===//
//
// Exact integer linear algebra for the affine GEMM recognizer
// (docs/AFFINE_GEMM_MAPPING.md section 19): rank, |determinant| and a basis
// of the integer kernel by unimodular column operations, primitive vectors, a
// common direction of several linear forms, signed mixed radixes and the
// injectivity of a two-coordinate index map on a box.
//
// Everything is checked 64-bit arithmetic: a result that does not fit is
// std::nullopt, never a wrapped value. Nothing here decides that a nest is a
// GEMM; these are the tools the screening and the recognizer are built from.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_INTEGERLATTICE_H
#define SYSTOLIC_INTEGERLATTICE_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/SmallVector.h"

#include <cstdint>
#include <optional>

namespace mlir::systolic::lattice {

using IntVector = llvm::SmallVector<int64_t>;
/// A matrix as its rows.
using IntMatrix = llvm::SmallVector<IntVector>;

/// |v|; nullopt for INT64_MIN.
std::optional<int64_t> checkedAbs(int64_t v);

/// gcd(|a|, |b|), gcd(0, 0) = 0; nullopt when |a| or |b| does not fit.
std::optional<int64_t> gcd(int64_t a, int64_t b);

/// The column echelon form of an integer matrix m (rows x cols) under
/// unimodular column operations, m U = E:
///   rank      -- the rank of m over Q (equal to its rank over Z);
///   kernel    -- a basis of the integer kernel {x in Z^cols : m x = 0}, as
///                rows, in Hermite normal form (every row's first nonzero
///                entry positive, the entries of the other rows in that
///                column reduced into [0, pivot)): unique for the lattice;
///   absDeterminant -- |det m| for a square m.
struct ColumnEchelon {
  unsigned rank = 0;
  IntMatrix kernel;
  std::optional<int64_t> absDeterminant;
};

/// nullopt when a row of m does not have `cols` entries or on overflow.
std::optional<ColumnEchelon> columnEchelon(const IntMatrix &m, unsigned cols);

/// v divided by the gcd of its entries, its first nonzero entry positive; the
/// zero vector stays zero. nullopt on overflow.
std::optional<IntVector> primitive(llvm::ArrayRef<int64_t> v);

/// A common direction of linear forms of the same length: a primitive g and
/// one integer multiplier per form with form = multiplier * g (a zero form
/// has multiplier 0). nullopt when two nonzero forms are not proportional,
/// when every form is zero, or on overflow.
struct CommonDirection {
  IntVector direction;
  IntVector multipliers;
};
std::optional<CommonDirection>
commonDirection(llvm::ArrayRef<IntVector> forms);

/// Theorem 3 (S2) for one coordinate: the map z -> sum_s c_s z_s on
/// prod_s [0, n_s) is a bijection onto consecutive integers iff the
/// magnitudes |c_s|, sorted ascending with their extents, are 1, n_(1),
/// n_(1) n_(2), ... Digits of extent 1 are constants and ignored. Returns
/// the number of values (the product of the extents) when it is; nullopt when
/// it is not (a zero coefficient on a digit of extent >= 2 included), when an
/// extent is below 1, or on overflow.
std::optional<int64_t> mixedRadixExtent(llvm::ArrayRef<int64_t> coefficients,
                                        llvm::ArrayRef<int64_t> extents);

/// Injectivity of (I, J) -> lamI * I + lamJ * J (vectors of one length) on
/// the box [0, M) x [0, N), M, N >= 1:
///   collision -- nullopt when the map is injective; otherwise a nonzero
///                (dI, dJ) with |dI| < M, |dJ| < N and lamI dI + lamJ dJ = 0
///                (two points of the box with the same image).
/// The criterion (docs section 19): rank [lamI lamJ] = 2 is injective; a
/// zero lamI needs M = 1 (likewise lamJ, N); for lamI = a u, lamJ = b u
/// (u primitive, a, b != 0, g = gcd(a, b)) the smallest collision is
/// (b/g, -a/g), so it is injective iff |b|/g >= M or |a|/g >= N.
struct BoxInjectivity {
  std::optional<std::pair<int64_t, int64_t>> collision;
  bool injective() const { return !collision; }
};
std::optional<BoxInjectivity> injectiveOnBox(llvm::ArrayRef<int64_t> lamI,
                                             llvm::ArrayRef<int64_t> lamJ,
                                             int64_t M, int64_t N);

} // namespace mlir::systolic::lattice

#endif // SYSTOLIC_INTEGERLATTICE_H
