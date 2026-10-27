#pragma once

// Phase 3.1: General Affine Tensor Contraction
// Logical Row-Space Decomposition.
//
// All calculations use exact rational arithmetic.
// No fixed tensor rank, source-IV count, or candidate enumeration limit.
//
// IMPORTANT:
// These are covectors in source-IV coordinates,
// not source iteration directions.

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/rational.hpp>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

using BigInt = boost::multiprecision::cpp_int;
using Rat = boost::rational<BigInt>;

using IntRow = std::vector<BigInt>;
using IntMatrix = std::vector<IntRow>;

using RatRow = std::vector<Rat>;
using RatMatrix = std::vector<RatRow>;

namespace detail {

inline BigInt abs(BigInt x) {
  return x < 0 ? -x : x;
}

inline BigInt gcd(BigInt a, BigInt b) {
  a = abs(a);
  b = abs(b);

  while (b != 0) {
    BigInt remainder = a % b;
    a = b;
    b = remainder;
  }

  return a;
}

inline BigInt lcm(const BigInt &a, const BigInt &b) {
  if (a == 0 || b == 0)
    return 0;

  return abs((a / gcd(a, b)) * b);
}

inline std::optional<RatMatrix> rationalize(
    const IntMatrix &matrix, std::size_t dimensions) {
  RatMatrix result;
  for (const auto &row : matrix) {
    if (row.size() != dimensions) return std::nullopt;
    RatRow converted;
    for (const auto &value : row) converted.emplace_back(value);
    result.push_back(std::move(converted));
  }
  return result;
}


// Convert a rational row to a primitive integer representative.
// This preserves its rational span, not necessarily an integer lattice basis.
inline IntRow integralPrimitive(const RatRow &row) {
  BigInt denominatorLCM = 1;

  for (const auto &value : row)
    denominatorLCM = lcm(
        denominatorLCM, value.denominator());

  IntRow result;
  BigInt divisor = 0;

  for (const auto &value : row) {
    BigInt integerValue =
        value.numerator() *
        (denominatorLCM / value.denominator());

    divisor = gcd(divisor, integerValue);
    result.push_back(std::move(integerValue));
  }

  if (divisor != 0) {
    for (auto &value : result)
      value /= divisor;
  }

  // Canonicalize the sign.
  for (const auto &value : result) {
    if (value == 0)
      continue;

    if (value < 0) {
      for (auto &entry : result)
        entry = -entry;
    }

    break;
  }

  return result;
}

// Exact reduced row-echelon form over Q.
inline std::optional<std::pair<RatMatrix, std::vector<std::size_t>>>
rref(RatMatrix matrix, std::size_t columns) {
  for (const auto &row : matrix)
    if (row.size() != columns) return std::nullopt;
  std::size_t pivotRow = 0;
  std::vector<std::size_t> pivots;
  for (std::size_t column = 0;
       column < columns && pivotRow < matrix.size(); ++column) {
    std::size_t selected = pivotRow;
    while (selected < matrix.size() && matrix[selected][column] == 0)
      ++selected;
    if (selected == matrix.size()) continue;
    std::swap(matrix[selected], matrix[pivotRow]);
    Rat pivot = matrix[pivotRow][column];
    for (std::size_t j = column; j < columns; ++j)
      matrix[pivotRow][j] /= pivot;
    for (std::size_t row = 0; row < matrix.size(); ++row) {
      if (row == pivotRow || matrix[row][column] == 0) continue;
      Rat factor = matrix[row][column];
      for (std::size_t j = column; j < columns; ++j)
        matrix[row][j] -= factor * matrix[pivotRow][j];
    }
    pivots.push_back(column);
    ++pivotRow;
  }
  matrix.resize(pivotRow);
  return std::make_pair(std::move(matrix), std::move(pivots));
}


// Exact rational kernel of an m x n matrix.
inline std::optional<RatMatrix> kernel(
    const RatMatrix &matrix, std::size_t columns) {
  auto reducedAndPivots = rref(matrix, columns);
  if (!reducedAndPivots) return std::nullopt;
  const auto &[reduced, pivots] = *reducedAndPivots;
  std::vector<bool> isPivot(columns, false);
  for (std::size_t pivot : pivots) isPivot[pivot] = true;
  RatMatrix result;
  for (std::size_t freeColumn = 0; freeColumn < columns; ++freeColumn) {
    if (isPivot[freeColumn]) continue;
    RatRow vector(columns, Rat(0));
    vector[freeColumn] = Rat(1);
    for (std::size_t row = 0; row < pivots.size(); ++row)
      vector[pivots[row]] = -reduced[row][freeColumn];
    result.push_back(std::move(vector));
  }
  return result;
}


inline std::optional<IntMatrix> rowBasis(
    const IntMatrix &matrix, std::size_t dimensions) {
  auto rational = rationalize(matrix, dimensions);
  if (!rational) return std::nullopt;
  auto reducedAndPivots = rref(std::move(*rational), dimensions);
  if (!reducedAndPivots) return std::nullopt;
  IntMatrix result;
  for (const auto &row : reducedAndPivots->first)
    result.push_back(integralPrimitive(row));
  return result;
}


inline std::optional<std::size_t> rank(
    const IntMatrix &matrix, std::size_t dimensions) {
  auto basis = rowBasis(matrix, dimensions);
  if (!basis) return std::nullopt;
  return basis->size();
}


inline IntMatrix concat(
    const IntMatrix &a,
    const IntMatrix &b) {

  IntMatrix result = a;

  result.insert(
      result.end(), b.begin(), b.end());

  return result;
}

// Intersection of rational row spaces A and B.
//
// Find coefficient relations:
//
//   alpha * A - beta * B = 0
//
// Each such relation supplies a vector in A intersect B.
inline std::optional<IntMatrix> intersection(
    const IntMatrix &a, const IntMatrix &b, std::size_t dimensions) {
  auto A = rowBasis(a, dimensions);
  auto B = rowBasis(b, dimensions);
  if (!A || !B) return std::nullopt;
  const std::size_t p = A->size(), q = B->size();
  RatMatrix relationMatrix(dimensions, RatRow(p + q, Rat(0)));
  for (std::size_t column = 0; column < p; ++column)
    for (std::size_t row = 0; row < dimensions; ++row)
      relationMatrix[row][column] = Rat((*A)[column][row]);
  for (std::size_t column = 0; column < q; ++column)
    for (std::size_t row = 0; row < dimensions; ++row)
      relationMatrix[row][p + column] = -Rat((*B)[column][row]);
  auto relations = kernel(relationMatrix, p + q);
  if (!relations) return std::nullopt;
  IntMatrix common;
  for (const auto &relation : *relations) {
    RatRow vector(dimensions, Rat(0));
    for (std::size_t x = 0; x < p; ++x)
      for (std::size_t y = 0; y < dimensions; ++y)
        vector[y] += relation[x] * Rat((*A)[x][y]);
    common.push_back(integralPrimitive(vector));
  }
  return rowBasis(common, dimensions);
}


// Select a rational linear complement:
//
//   larger = subspace (+) complement
//
// Requires subspace to be contained in larger.
inline std::optional<IntMatrix> complement(
    const IntMatrix &larger, const IntMatrix &subspace,
    std::size_t dimensions) {
  auto accumulated = rowBasis(subspace, dimensions);
  auto largerBasis = rowBasis(larger, dimensions);
  if (!accumulated || !largerBasis) return std::nullopt;
  const std::size_t subspaceRank = accumulated->size();
  auto jointRank = rank(concat(*largerBasis, *accumulated), dimensions);
  if (!jointRank || *jointRank != largerBasis->size()) return std::nullopt;
  IntMatrix result;
  for (const auto &vector : *largerBasis) {
    IntMatrix candidate = *accumulated;
    candidate.push_back(vector);
    auto candidateRank = rank(candidate, dimensions);
    if (!candidateRank) return std::nullopt;
    if (*candidateRank > accumulated->size()) {
      accumulated = rowBasis(candidate, dimensions);
      if (!accumulated) return std::nullopt;
      result.push_back(vector);
    }
  }
  if (result.size() + subspaceRank != largerBasis->size())
    return std::nullopt;
  return result;
}


} // namespace detail

// ============================================================
// Phase 3.2: Rank-based I/J/K/Batch Partition
//
// T = A intersect B intersect C
//
// I = complement(T, A intersect C)
// J = complement(T, B intersect C)
// K = complement(T, A intersect B)
//
// Batch = complement(span(I,J,K), U)
//
// All spaces are rational row spaces of source-IV covectors.
// Batch is the entire remaining coordinate complement;
// it is NOT merely the three-way intersection.
//
// No fixed tensor rank, source dimension, or candidate cap.
// ============================================================

struct RankPartition {
  IntMatrix i;
  IntMatrix j;
  IntMatrix k;
  IntMatrix batch;

  std::size_t sourceRank = 0;
};

inline std::optional<RankPartition> analyzeRankPartition(
    const IntMatrix &a, const IntMatrix &b, const IntMatrix &c,
    std::size_t sourceDimension) {
  using namespace detail;
  auto A = rowBasis(a, sourceDimension);
  auto B = rowBasis(b, sourceDimension);
  auto C = rowBasis(c, sourceDimension);
  if (!A || !B || !C) return std::nullopt;
  auto AB = intersection(*A, *B, sourceDimension);
  auto AC = intersection(*A, *C, sourceDimension);
  auto BC = intersection(*B, *C, sourceDimension);
  if (!AB || !AC || !BC) return std::nullopt;
  auto T = intersection(*AB, *C, sourceDimension);
  if (!T) return std::nullopt;
  auto I = complement(*AC, *T, sourceDimension);
  auto J = complement(*BC, *T, sourceDimension);
  auto K = complement(*AB, *T, sourceDimension);
  if (!I || !J || !K) return std::nullopt;
  RankPartition result;
  result.i = std::move(*I);
  result.j = std::move(*J);
  result.k = std::move(*K);
  const IntMatrix IJK = concat(concat(result.i, result.j), result.k);
  auto ijkRank = rank(IJK, sourceDimension);
  const std::size_t expectedIJKRank =
      result.i.size() + result.j.size() + result.k.size();
  if (!ijkRank || *ijkRank != expectedIJKRank) return std::nullopt;
  IntMatrix U;
  for (std::size_t d = 0; d < sourceDimension; ++d) {
    IntRow e(sourceDimension, BigInt(0));
    e[d] = 1;
    U.push_back(std::move(e));
  }
  auto batch = complement(U, IJK, sourceDimension);
  if (!batch) return std::nullopt;
  result.batch = std::move(*batch);
  const IntMatrix complete = concat(IJK, result.batch);
  auto completeRank = rank(complete, sourceDimension);
  if (!completeRank || *completeRank != sourceDimension ||
      expectedIJKRank + result.batch.size() != sourceDimension)
    return std::nullopt;
  result.sourceRank = sourceDimension;
  return result;
}


} // namespace systolic::logical
