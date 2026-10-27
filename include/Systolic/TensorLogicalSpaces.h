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

inline RatMatrix rationalize(
    const IntMatrix &matrix,
    std::size_t dimensions) {

  RatMatrix result;

  for (const auto &row : matrix) {
    if (row.size() != dimensions)
      throw std::invalid_argument(
          "Access row width differs from source dimension");

    RatRow converted;

    for (const auto &value : row)
      converted.emplace_back(value);

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
inline std::pair<RatMatrix, std::vector<std::size_t>>
rref(RatMatrix matrix, std::size_t columns) {

  for (const auto &row : matrix) {
    if (row.size() != columns)
      throw std::invalid_argument("Ragged matrix");
  }

  std::size_t pivotRow = 0;
  std::vector<std::size_t> pivots;

  for (std::size_t column = 0;
       column < columns && pivotRow < matrix.size();
       ++column) {

    std::size_t selected = pivotRow;

    while (selected < matrix.size() &&
           matrix[selected][column] == 0)
      ++selected;

    if (selected == matrix.size())
      continue;

    std::swap(matrix[selected], matrix[pivotRow]);

    Rat pivot = matrix[pivotRow][column];

    for (std::size_t j = column; j < columns; ++j)
      matrix[pivotRow][j] /= pivot;

    for (std::size_t row = 0;
         row < matrix.size(); ++row) {

      if (row == pivotRow ||
          matrix[row][column] == 0)
        continue;

      Rat factor = matrix[row][column];

      for (std::size_t j = column; j < columns; ++j)
        matrix[row][j] -=
            factor * matrix[pivotRow][j];
    }

    pivots.push_back(column);
    ++pivotRow;
  }

  matrix.resize(pivotRow);

  return {std::move(matrix), std::move(pivots)};
}

// Exact rational kernel of an m x n matrix.
inline RatMatrix kernel(
    const RatMatrix &matrix,
    std::size_t columns) {

  auto [reduced, pivots] = rref(matrix, columns);

  std::vector<bool> isPivot(columns, false);

  for (std::size_t pivot : pivots)
    isPivot[pivot] = true;

  RatMatrix result;

  for (std::size_t freeColumn = 0;
       freeColumn < columns; ++freeColumn) {

    if (isPivot[freeColumn])
      continue;

    RatRow vector(columns, Rat(0));

    vector[freeColumn] = Rat(1);

    for (std::size_t row = 0;
         row < pivots.size(); ++row)
      vector[pivots[row]] =
          -reduced[row][freeColumn];

    result.push_back(std::move(vector));
  }

  return result;
}

inline IntMatrix rowBasis(
    const IntMatrix &matrix,
    std::size_t dimensions) {

  auto [reduced, pivots] =
      rref(rationalize(matrix, dimensions), dimensions);

  (void)pivots;

  IntMatrix result;

  for (const auto &row : reduced)
    result.push_back(integralPrimitive(row));

  return result;
}

inline std::size_t rank(
    const IntMatrix &matrix,
    std::size_t dimensions) {

  return rowBasis(matrix, dimensions).size();
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
inline IntMatrix intersection(
    const IntMatrix &a,
    const IntMatrix &b,
    std::size_t dimensions) {

  IntMatrix A = rowBasis(a, dimensions);
  IntMatrix B = rowBasis(b, dimensions);

  const std::size_t p = A.size();
  const std::size_t q = B.size();

  RatMatrix relationMatrix(
      dimensions, RatRow(p + q, Rat(0)));

  for (std::size_t column = 0; column < p; ++column) {
    for (std::size_t row = 0;
         row < dimensions; ++row)
      relationMatrix[row][column] =
          Rat(A[column][row]);
  }

  for (std::size_t column = 0; column < q; ++column) {
    for (std::size_t row = 0;
         row < dimensions; ++row)
      relationMatrix[row][p + column] =
          -Rat(B[column][row]);
  }

  RatMatrix relations =
      kernel(relationMatrix, p + q);

  IntMatrix common;

  for (const auto &relation : relations) {
    RatRow vector(dimensions, Rat(0));

    for (std::size_t x = 0; x < p; ++x) {
      for (std::size_t y = 0;
           y < dimensions; ++y)
        vector[y] +=
            relation[x] * Rat(A[x][y]);
    }

    common.push_back(integralPrimitive(vector));
  }

  return rowBasis(common, dimensions);
}

// Select a rational linear complement:
//
//   larger = subspace (+) complement
//
// Requires subspace to be contained in larger.
inline IntMatrix complement(
    const IntMatrix &larger,
    const IntMatrix &subspace,
    std::size_t dimensions) {

  IntMatrix accumulated =
      rowBasis(subspace, dimensions);

  const std::size_t subspaceRank =
      accumulated.size();

  IntMatrix largerBasis =
      rowBasis(larger, dimensions);

  if (rank(
          concat(largerBasis, accumulated),
          dimensions) != largerBasis.size())
    throw std::invalid_argument(
        "Subspace is not contained in larger space");

  IntMatrix result;

  for (const auto &vector : largerBasis) {
    IntMatrix candidate = accumulated;
    candidate.push_back(vector);

    if (rank(candidate, dimensions) >
        accumulated.size()) {

      accumulated =
          rowBasis(candidate, dimensions);

      result.push_back(vector);
    }
  }

  if (result.size() + subspaceRank !=
      largerBasis.size())
    throw std::runtime_error(
        "Complement construction failed");

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

inline RankPartition analyzeRankPartition(
    const IntMatrix &a,
    const IntMatrix &b,
    const IntMatrix &c,
    std::size_t sourceDimension) {

  using namespace detail;

  const IntMatrix A = rowBasis(a, sourceDimension);
  const IntMatrix B = rowBasis(b, sourceDimension);
  const IntMatrix C = rowBasis(c, sourceDimension);

  const IntMatrix AB =
      intersection(A, B, sourceDimension);

  const IntMatrix AC =
      intersection(A, C, sourceDimension);

  const IntMatrix BC =
      intersection(B, C, sourceDimension);

  const IntMatrix T =
      intersection(AB, C, sourceDimension);

  RankPartition result;

  // (A intersect C) minus B
  result.i = complement(AC, T, sourceDimension);

  // (B intersect C) minus A
  result.j = complement(BC, T, sourceDimension);

  // (A intersect B) minus C
  result.k = complement(AB, T, sourceDimension);

  const IntMatrix IJK =
      concat(concat(result.i, result.j), result.k);

  const std::size_t ijkRank =
      rank(IJK, sourceDimension);

  const std::size_t expectedIJKRank =
      result.i.size() +
      result.j.size() +
      result.k.size();

  if (ijkRank != expectedIJKRank)
    throw std::runtime_error(
        "I/J/K are not linearly independent");

  // Build the complete source coordinate universe U.
  IntMatrix U;

  for (std::size_t dimension = 0;
       dimension < sourceDimension;
       ++dimension) {

    IntRow e(sourceDimension, BigInt(0));
    e[dimension] = 1;
    U.push_back(std::move(e));
  }

  // Every coordinate direction not selected by I/J/K
  // is assigned to Batch.
  result.batch = complement(
      U, IJK, sourceDimension);

  const IntMatrix complete =
      concat(IJK, result.batch);

  if (rank(complete, sourceDimension) != sourceDimension)
    throw std::runtime_error(
        "I/J/K/Batch do not span source space");

  if (expectedIJKRank + result.batch.size() !=
      sourceDimension)
    throw std::runtime_error(
        "Partition rank-nullity violated");

  result.sourceRank = sourceDimension;

  return result;
}

} // namespace systolic::logical
