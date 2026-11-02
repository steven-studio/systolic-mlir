//===- Lattice.cpp - Exact solution of linear equalities ------------------===//

#include "Systolic/Exact/Lattice.h"
#include "ExecutionCounters.h"

#include <utility>

namespace systolic::exact {

std::vector<Int>
EqualitySolution::toCoordinates(const std::vector<Int> &a) const {
  std::vector<Int> coordinates(size, Int(0));
  for (unsigned i = 0; i < size; ++i)
    for (unsigned j = 0; j < size; ++j)
      coordinates[j] += a[i] * unimodular[i][j];
  return coordinates;
}

std::vector<Int>
EqualitySolution::freeCoefficients(const std::vector<Int> &a) const {
  const std::vector<Int> coordinates = toCoordinates(a);
  return std::vector<Int>(coordinates.begin() + pivotValues.size(),
                          coordinates.end());
}

Rational EqualitySolution::constantAtOrigin(const std::vector<Int> &a,
                                            const Int &c) const {
  const std::vector<Int> coordinates = toCoordinates(a);
  Rational value(c);
  for (std::size_t k = 0; k < pivotValues.size(); ++k)
    value = value + Rational(coordinates[k]) * pivotValues[k];
  return value;
}

/// w with target[j] = sum_t w[t] * pivotCells[t][j] on the pivot columns. Each
/// pivot row is zero right of its pivot, so back substitution solves it.
std::vector<Rational>
EqualitySolution::pivotWeights(const std::vector<Int> &target) const {
  const std::size_t rank = pivotCells.size();
  std::vector<Rational> weights(rank);
  for (std::size_t j = rank; j-- > 0;) {
    Rational rest(target[j]);
    for (std::size_t t = j + 1; t < rank; ++t)
      rest = rest - weights[t] * Rational(pivotCells[t][j]);
    weights[j] = *rest.dividedBy(Rational(pivotCells[j][j]));
  }
  return weights;
}

std::vector<Rational>
EqualitySolution::multipliers(const std::vector<Int> &a) const {
  const std::vector<Rational> weights = pivotWeights(toCoordinates(a));
  std::vector<Rational> byInputRow(numRows);
  for (std::size_t t = 0; t < weights.size(); ++t)
    byInputRow[pivotInputRow[t]] = weights[t];
  return byInputRow;
}

std::vector<Rational>
EqualitySolution::point(const std::vector<Rational> &free) const {
  std::vector<Rational> coordinates = pivotValues;
  coordinates.insert(coordinates.end(), free.begin(), free.end());
  std::vector<Rational> x(size);
  for (unsigned i = 0; i < size; ++i)
    for (unsigned j = 0; j < size; ++j)
      x[i] = x[i] + Rational(unimodular[i][j]) * coordinates[j];
  return x;
}

/// Holds the working matrices of one run. A friend of EqualitySolution, whose
/// fields it fills.
struct LatticeSolver {
  LatticeSolver(unsigned numVariables, const std::vector<EqualityRow> &rows,
                Domain domain, WorkMeter &meter)
      : rows(rows), domain(domain), meter(meter), n(numVariables) {
    working.reserve(rows.size());
    for (const EqualityRow &row : rows)
      working.push_back(row.coefficients);
    solution.size = n;
    solution.numRows = rows.size();
    solution.unimodular.assign(n, std::vector<Int>(n, Int(0)));
    for (unsigned i = 0; i < n; ++i)
      solution.unimodular[i][i] = Int(1);
  }

  /// column[target] -= quotient * column[source], in rows from `first` on and
  /// in U. Rows above `first` are already zero in both columns.
  bool subtractColumn(std::size_t first, unsigned target, unsigned source,
                      const Int &quotient) {
    if (!meter.charge(working.size() - first + n))
      return false;
    for (std::size_t r = first; r < working.size(); ++r) {
      working[r][target] -= quotient * working[r][source];
      if (!meter.admit(working[r][target]))
        return false;
    }
    for (unsigned r = 0; r < n; ++r) {
      solution.unimodular[r][target] -=
          quotient * solution.unimodular[r][source];
      if (!meter.admit(solution.unimodular[r][target]))
        return false;
    }
    return true;
  }

  void swapColumns(std::size_t first, unsigned a, unsigned b) {
    for (std::size_t r = first; r < working.size(); ++r)
      std::swap(working[r][a], working[r][b]);
    for (unsigned r = 0; r < n; ++r)
      std::swap(solution.unimodular[r][a], solution.unimodular[r][b]);
  }

  /// Euclid on the columns right of the pivots, on row r. Leaves at most one
  /// nonzero there. Its column, which is returned, may be any of them.
  std::optional<unsigned> reduceRow(std::size_t r, bool &failed) {
    const unsigned rank = solution.pivotValues.size();
    std::vector<Int> &row = working[r];
    while (true) {
      std::optional<unsigned> best;
      for (unsigned j = rank; j < n; ++j)
        if (row[j] != Int(0) &&
            (!best || magnitude(row[j]) < magnitude(row[*best])))
          best = j;
      if (!best)
        return std::nullopt;
      bool otherLeft = false;
      for (unsigned j = rank; j < n; ++j) {
        if (j == *best || row[j] == Int(0))
          continue;
        const Int quotient = *divFloor(row[j], row[*best]);
        if (!subtractColumn(r, j, *best, quotient)) {
          failed = true;
          return std::nullopt;
        }
        otherLeft = otherLeft || row[j] != Int(0);
      }
      if (!otherLeft)
        return best;
    }
  }

  /// y for the integer Farkas certificate: the weights that isolate the newest
  /// pivot coordinate.
  EqualityConflict notIntegral() const {
    const std::size_t rank = solution.pivotCells.size();
    std::vector<Int> target(rank, Int(0));
    target[rank - 1] = Int(1);
    const std::vector<Rational> weights = solution.pivotWeights(target);
    EqualityConflict conflict;
    conflict.kind = EqualityConflict::Kind::NotIntegral;
    conflict.multipliers.assign(rows.size(), Rational());
    for (std::size_t t = 0; t < rank; ++t)
      conflict.multipliers[solution.pivotInputRow[t]] = weights[t];
    return conflict;
  }

  /// y for a row whose left side is a combination of earlier pivot rows.
  EqualityConflict inconsistent(std::size_t r) const {
    const std::vector<Rational> weights = solution.pivotWeights(working[r]);
    EqualityConflict conflict;
    conflict.kind = EqualityConflict::Kind::Inconsistent;
    conflict.multipliers.assign(rows.size(), Rational());
    conflict.multipliers[r] = Rational(Int(1));
    for (std::size_t t = 0; t < weights.size(); ++t)
      conflict.multipliers[solution.pivotInputRow[t]] = -weights[t];
    return conflict;
  }

  LatticeResult run() {
    LatticeResult result;
    for (std::size_t r = 0; r < rows.size(); ++r) {
      bool failed = false;
      const std::optional<unsigned> pivot = reduceRow(r, failed);
      if (failed)
        return result;
      const unsigned rank = solution.pivotValues.size();
      if (!meter.charge(rank + 1))
        return result;
      // The row's value with every known coordinate substituted.
      Rational residual(rows[r].constant);
      for (unsigned j = 0; j < rank; ++j)
        residual =
            residual + Rational(working[r][j]) * solution.pivotValues[j];
      if (!meter.admit(residual.numerator()) ||
          !meter.admit(residual.denominator()))
        return result;
      if (!pivot) {
        if (residual.sign() != 0) {
          result.status = LatticeResult::Status::Conflict;
          result.conflict = inconsistent(r);
          return result;
        }
        continue;
      }
      if (*pivot != rank)
        swapColumns(r, *pivot, rank);
      const Rational value =
          *(-residual).dividedBy(Rational(working[r][rank]));
      solution.pivotCells.push_back(working[r]);
      solution.pivotInputRow.push_back(r);
      solution.pivotValues.push_back(value);
      if (domain == Domain::Integer && !value.isInteger()) {
        result.status = LatticeResult::Status::Conflict;
        result.conflict = notIntegral();
        return result;
      }
    }
    // With no rows U is still the identity, and there is nothing to check.
    if (!rows.empty()) {
      const std::optional<bool> unit =
          hasUnitDeterminant(solution.unimodular, meter);
      if (!unit)
        return result;
      if (!*unit) {
        meter.stop(UnknownReason::CertificateRejected);
        return result;
      }
    }
    result.status = LatticeResult::Status::Solved;
    result.solution = std::move(solution);
    return result;
  }

  const std::vector<EqualityRow> &rows;
  Domain domain;
  WorkMeter &meter;
  unsigned n;
  std::vector<std::vector<Int>> working; // E U, by rows
  EqualitySolution solution;
};

std::optional<bool> hasUnitDeterminant(std::vector<std::vector<Int>> matrix,
                                       WorkMeter &meter) {
  const std::size_t n = matrix.size();
  Int previous(1);
  for (std::size_t k = 0; k < n; ++k) {
    if (!meter.charge(std::uint64_t(n - k) * (n - k)))
      return std::nullopt;
    std::size_t pivot = k;
    while (pivot < n && matrix[pivot][k] == Int(0))
      ++pivot;
    if (pivot == n)
      return false;
    std::swap(matrix[k], matrix[pivot]);
    for (std::size_t i = k + 1; i < n; ++i) {
      for (std::size_t j = k + 1; j < n; ++j) {
        // Exact: this is a minor, which `previous` divides (Bareiss, 1968).
        matrix[i][j] = (matrix[i][j] * matrix[k][k] -
                        matrix[i][k] * matrix[k][j]) /
                       previous;
        if (!meter.admit(matrix[i][j]))
          return std::nullopt;
      }
    }
    previous = matrix[k][k];
  }
  return n == 0 || magnitude(matrix[n - 1][n - 1]) == Int(1);
}

LatticeResult solveEqualities(unsigned numVariables,
                              const std::vector<EqualityRow> &rows,
                              Domain domain, WorkMeter &meter) {
  detail::noteExecution(detail::Stage::Lattice);
  return LatticeSolver(numVariables, rows, domain, meter).run();
}

} // namespace systolic::exact
