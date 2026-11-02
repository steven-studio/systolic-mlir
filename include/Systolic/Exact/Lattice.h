//===- Lattice.h - Exact solution of linear equalities ----------*- C++ -*-===//
//
// Solves E x + f = 0 by unimodular column operations, the constructive proof
// that every integer matrix has a Hermite normal form (Schrijver, Theory of
// Linear and Integer Programming, chapter 4). A unimodular U maps Z^n onto
// itself, so x = U z loses no integer point. E U is lower triangular on its
// pivots, which fixes the pivot coordinates of z one at a time, and the rest of
// z stays free.
//
// Over the integers a pivot coordinate must come out integral. When it does
// not, the equalities have rational solutions but no integer one, and the
// multipliers found are the certificate of the integer Farkas lemma: y with
// y E integral and y f not (Schrijver, corollary 4.1a).
//
// U is built from steps that keep |det U| = 1, so the lattice step maps onto
// exactly the integer solutions. solveEqualities() still recomputes that
// determinant whenever it had rows to work on, so that a defect here stops the
// query instead of reaching the stages that rest on U.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_LATTICE_H
#define SYSTOLIC_EXACT_LATTICE_H

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Rational.h"

#include <optional>

namespace systolic::exact {

/// coefficients . x + constant == 0.
struct EqualityRow {
  std::vector<Int> coefficients;
  Int constant{0};
};

/// Why the equalities have no solution: one multiplier per input row.
struct EqualityConflict {
  enum class Kind {
    /// sum y_i a_i is zero and sum y_i c_i is not, in either domain.
    Inconsistent,
    /// sum y_i a_i is integral and sum y_i c_i is not, in the integer domain.
    NotIntegral,
  };
  Kind kind = Kind::Inconsistent;
  std::vector<Rational> multipliers;
};

/// The solutions of the equalities, as x = particular + basis * y. In the
/// domain it was solved for, each solution comes from exactly one y in that
/// domain, and each y gives a solution. An instance owns all its data.
class EqualitySolution {
public:
  unsigned numVariables() const { return size; }
  /// The number of free coordinates y.
  unsigned numFree() const { return size - pivotValues.size(); }

  /// The coefficients over y of the form a . x.
  std::vector<Int> freeCoefficients(const std::vector<Int> &a) const;
  /// The value of a . x + c at y = 0.
  Rational constantAtOrigin(const std::vector<Int> &a, const Int &c) const;
  /// w, one entry per input row, such that a . x - sum w_i (row i) has no pivot
  /// coordinate: it depends on y alone, and equals a . x on every solution.
  std::vector<Rational> multipliers(const std::vector<Int> &a) const;
  /// x for the given y.
  std::vector<Rational> point(const std::vector<Rational> &free) const;
  /// U, by rows, for x = U z, with the pivot coordinates of z first.
  const std::vector<std::vector<Int>> &transform() const { return unimodular; }

private:
  friend struct LatticeSolver;

  std::vector<Int> toCoordinates(const std::vector<Int> &a) const;
  std::vector<Rational> pivotWeights(const std::vector<Int> &target) const;

  unsigned size = 0;
  std::size_t numRows = 0;
  std::vector<std::vector<Int>> unimodular;  // U, by rows
  std::vector<std::vector<Int>> pivotCells;  // E U, one row per pivot
  std::vector<std::size_t> pivotInputRow;    // the input row of each pivot
  std::vector<Rational> pivotValues;         // z on the pivot coordinates
};

struct LatticeResult {
  enum class Status { Solved, Conflict, Stopped };
  Status status = Status::Stopped;
  EqualitySolution solution;  // when Solved
  EqualityConflict conflict;  // when Conflict
};

/// Whether a square matrix has determinant 1 or -1, by fraction-free
/// elimination, whose every intermediate value is a minor of the matrix.
/// std::nullopt when `meter`, borrowed for the call, stopped it first.
std::optional<bool> hasUnitDeterminant(std::vector<std::vector<Int>> matrix,
                                       WorkMeter &meter);

/// `meter` is borrowed for the call. Stopped leaves the reason in the meter. A
/// transform that fails its determinant check stops with CertificateRejected.
LatticeResult solveEqualities(unsigned numVariables,
                              const std::vector<EqualityRow> &rows,
                              Domain domain, WorkMeter &meter);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_LATTICE_H
