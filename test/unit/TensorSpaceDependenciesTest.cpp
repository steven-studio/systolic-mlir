//===- TensorSpaceDependenciesTest.cpp --------------------------*- C++ -*-===//
//
// Mathematical unit tests for three affine tensor access spaces.
//
// No compiler IR mutation, lowering, runtime, or hardware execution.
//
//===----------------------------------------------------------------------===//

#include <vector>
#include <algorithm>
#include <utility>
#include "Systolic/GeneralAffineMatmul.h"

#include <array>
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>

using namespace mlir::systolic;
using lattice::IntMatrix;
using lattice::IntVector;

namespace {

struct Expected {
  unsigned rankA;
  unsigned rankB;
  unsigned rankC;

  unsigned pairAB;
  unsigned pairAC;
  unsigned pairBC;
  unsigned triple;

  unsigned combinedRank;

  unsigned relations;
  unsigned pairGenerated;
  unsigned essential;
  unsigned combinedKernelDim;
};



// Independent exact-rank checker using fraction-free elimination.
// Does not call the production lattice rank implementation.
static unsigned independentExactRank(
    std::vector<std::vector<__int128>> a,
    unsigned columns) {

  const size_t rows = a.size();

  for (const auto &r : a) {
    if (r.size() != columns) {
      std::cerr << "FAIL: malformed rank-check matrix\n";
      std::exit(1);
    }
  }

  size_t pivotRow = 0;
  __int128 previousPivot = 1;

  for (unsigned col = 0;
       col < columns && pivotRow < rows;
       ++col) {

    size_t selected = pivotRow;

    while (selected < rows && a[selected][col] == 0)
      ++selected;

    if (selected == rows)
      continue;

    if (selected != pivotRow)
      std::swap(a[selected], a[pivotRow]);

    const __int128 pivot = a[pivotRow][col];

    for (size_t i = pivotRow + 1; i < rows; ++i) {
      for (unsigned j = col + 1; j < columns; ++j) {
        __int128 lhs, rhs, numerator;

        if (__builtin_mul_overflow(a[i][j], pivot, &lhs) ||
            __builtin_mul_overflow(
                a[i][col], a[pivotRow][j], &rhs) ||
            __builtin_sub_overflow(lhs, rhs, &numerator)) {
          std::cerr << "FAIL: rank-check arithmetic overflow\n";
          std::exit(1);
        }

        if (numerator % previousPivot != 0) {
          std::cerr << "FAIL: non-exact Bareiss division\n";
          std::exit(1);
        }

        a[i][j] = numerator / previousPivot;
      }

      a[i][col] = 0;
    }

    previousPivot = pivot;
    ++pivotRow;
  }

  return static_cast<unsigned>(pivotRow);
}

void verifyCombinedKernel(
    const char *name,
    unsigned n,
    const std::array<IntMatrix, 3> &matrices,
    const TensorContractionSpaces &spaces,
    unsigned expectedKernelDim) {

  const auto &kernel = spaces.combinedKernel;

  if (kernel.size() != expectedKernelDim) {
    std::cerr << "FAIL: " << name
              << " combinedKernel dimension: expected "
              << expectedKernelDim << ", got "
              << kernel.size() << '\n';
    std::exit(1);
  }

  if (spaces.combinedRank + kernel.size() != n) {
    std::cerr << "FAIL: " << name
              << " rank-nullity violated\n";
    std::exit(1);
  }

  for (const auto &v : kernel) {
    if (v.size() != n) {
      std::cerr << "FAIL: malformed kernel vector\n";
      std::exit(1);
    }

    for (unsigned a = 0; a < 3; ++a)
      for (const auto &row : matrices[a]) {
        __int128 dot = 0;

        for (unsigned d = 0; d < n; ++d)
          dot += static_cast<__int128>(row[d]) * v[d];

        if (dot != 0) {
          std::cerr << "FAIL: " << name
                    << " kernel vector does not annihilate operand "
                    << a << '\n';
          std::exit(1);
        }
      }
  }

  // Independently recompute the rank of the stacked access map F.
  std::vector<std::vector<__int128>> accessRows;

  for (const auto &operand : matrices) {
    for (const auto &r : operand) {
      if (r.size() != n) {
        std::cerr << "FAIL: malformed operand access matrix\n";
        std::exit(1);
      }

      accessRows.emplace_back(r.begin(), r.end());
    }
  }

  const unsigned actualAccessRank =
      independentExactRank(accessRows, n);

  if (actualAccessRank != spaces.combinedRank) {
    std::cerr << "FAIL: " << name
              << " combinedRank mismatch: independent="
              << actualAccessRank
              << ", production=" << spaces.combinedRank
              << '\n';
    std::exit(1);
  }

  // Independently check linear independence of kernel vectors.
  std::vector<std::vector<__int128>> kernelRows;

  for (const auto &v : kernel)
    kernelRows.emplace_back(v.begin(), v.end());

  const unsigned actualKernelRank =
      independentExactRank(kernelRows, n);

  if (actualKernelRank != kernel.size()) {
    std::cerr << "FAIL: " << name
              << " kernel basis is linearly dependent\n";
    std::exit(1);
  }

  // Full kernel certificate:
  //
  //   F*K^T = 0
  //   rank(K) = number of kernel basis vectors
  //   rank(F) + rank(K) = source dimension
  //
  if (actualAccessRank + actualKernelRank != n) {
    std::cerr << "FAIL: " << name
              << " independent rank-nullity check failed\n";
    std::exit(1);
  }

}

void verify(
    const char *name,
    unsigned n,
    const std::array<IntMatrix, 3> &matrices,
    const Expected &expected) {

  std::array<IntVector, 3> offsets;

  for (unsigned a = 0; a < 3; ++a)
    offsets[a] = IntVector(matrices[a].size(), 0);

  auto spaces =
      analyzeTensorContractionSpaces(matrices, offsets, n);

  assert(spaces && "TensorContractionSpaces analysis failed");

  assert(spaces->operands[0].rank == expected.rankA);
  assert(spaces->operands[1].rank == expected.rankB);
  assert(spaces->operands[2].rank == expected.rankC);

  assert(spaces->pairIntersections[0].size() == expected.pairAB);
  assert(spaces->pairIntersections[1].size() == expected.pairAC);
  assert(spaces->pairIntersections[2].size() == expected.pairBC);

  assert(spaces->tripleIntersection.size() == expected.triple);

  assert(spaces->combinedRank == expected.combinedRank);

  verifyCombinedKernel(
      name, n, matrices, *spaces, expected.combinedKernelDim);

  auto deps = analyzeTensorSpaceDependencies(*spaces, n);

  assert(deps && "TensorSpaceDependencies analysis failed");

  assert(deps->relationRank == expected.relations);

  assert(deps->pairGeneratedRank == expected.pairGenerated);

  assert(deps->essentialThreeWayRank == expected.essential);

  // Directly verify Phi * relation = 0.
  const auto &offset = deps->coefficientOffsets;
  const unsigned total = offset[3];

  IntMatrix phi(n, IntVector(total, 0));

  for (unsigned a = 0; a < 3; ++a) {
    const auto &basis = spaces->operands[a].rowBasis;

    for (unsigned r = 0; r < basis.size(); ++r)
      for (unsigned d = 0; d < n; ++d)
        phi[d][offset[a] + r] = basis[r][d];
  }

  auto checkRelations = [&](const IntMatrix &relations) {
    for (const auto &relation : relations) {
      assert(relation.size() == total);

      for (unsigned d = 0; d < n; ++d) {
        // Test data uses small coefficients.
        // Accumulate in a wider integer type to keep the test independent
        // from the production checked-int64 accumulation path.
        __int128 sum = 0;

        for (unsigned c = 0; c < total; ++c)
          sum += static_cast<__int128>(phi[d][c]) * relation[c];

        assert(sum == 0);
      }
    }
  };

  checkRelations(deps->allRelations);
  checkRelations(deps->pairGeneratedRelations);
  checkRelations(deps->essentialThreeWayRelations);

  assert(
      deps->pairGeneratedRank + deps->essentialThreeWayRank ==
      deps->relationRank);

  std::cout << "PASS: " << name << '\n';
}

} // namespace

int main() {

  // -------------------------------------------------------
  // Test 1: Standard GEMM
  // U = (i,j,k)
  // A = span(i,k)
  // B = span(j,k)
  // C = span(i,j)
  // -------------------------------------------------------

  verify(
      "standard GEMM",
      3,
      {{
          {{1,0,0}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,0});

  // -------------------------------------------------------
  // Test 2: Batched Tensor Contraction
  // U = (i,j,k,p)
  // -------------------------------------------------------

  verify(
      "batched tensor contraction",
      4,
      {{
          {{1,0,0,0}, {0,0,1,0}, {0,0,0,1}},
          {{0,0,1,0}, {0,1,0,0}, {0,0,0,1}},
          {{1,0,0,0}, {0,1,0,0}, {0,0,0,1}}
      }},
      {3,3,3, 2,2,2, 1, 4, 5,5,0,0});

  // -------------------------------------------------------
  // Test 3: A[i][k][k]
  // Tensor rank(A)=3, access rank(A)=2.
  // -------------------------------------------------------

  verify(
      "repeated tensor indices",
      3,
      {{
          {{1,0,0}, {0,0,1}, {0,0,1}},
          {{0,0,1}, {0,1,0}},
          {{1,0,0}, {0,1,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,0});

  // -------------------------------------------------------
  // Test 4: Genuine three-way dependency
  // A = span(i)
  // B = span(j)
  // C = span(i+j)
  // -------------------------------------------------------

  verify(
      "genuine three-way dependency",
      2,
      {{
          {{1,0}},
          {{0,1}},
          {{1,1}}
      }},
      {1,1,1, 0,0,0, 0, 2, 1,0,1,0});

  // -------------------------------------------------------
  // Test 5: Triple intersection + three-way dependency
  //
  // U = (i,j,p)
  // A = span(i,p)
  // B = span(j,p)
  // C = span(i+j,p)
  // -------------------------------------------------------

  verify(
      "triple intersection and three-way dependency",
      3,
      {{
          {{1,0,0}, {0,0,1}},
          {{0,1,0}, {0,0,1}},
          {{1,1,0}, {0,0,1}}
      }},
      {2,2,2, 1,1,1, 1, 3, 3,2,1,0});

  // -------------------------------------------------------
  // Test 6: Coupled Affine Tensor Access
  //
  // C[i][j] += A[2*i+k][i+2*k] * B[2*k+j][k+2*j]
  //
  // U = (i,j,k)
  //
  // A = span(i,k)
  // B = span(j,k)
  // C = span(i,j)
  //
  // Combined rank = 3.
  // All relations = 3.
  // Pair-generated relations = 3.
  // Essential three-way relations = 0.
  // -------------------------------------------------------

  verify(
      "coupled affine tensor access",
      3,
      {{
          {{2,0,1}, {1,0,2}},
          {{0,1,2}, {0,2,1}},
          {{1,0,0}, {0,1,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,0});

  // -------------------------------------------------------
  // Test 7: Coupled Two-dimensional Reduction
  //
  // C[i][j] += A[i][k+l] * B[k+l][j]
  //
  // U = (i,j,k,l)
  //
  // A = span(i,k+l)
  // B = span(j,k+l)
  // C = span(i,j)
  //
  // Combined rank = 3, source dimension = 4.
  // Combined source kernel = span(0,0,1,-1).
  //
  // Different source iterations can have identical
  // A/B/C addresses. Their MAC multiplicity must not
  // be discarded by a future partitioning algorithm.
  // -------------------------------------------------------

  verify(
      "coupled multidimensional reduction",
      4,
      {{
          {{1,0,0,0}, {0,0,1,1}},
          {{0,0,1,1}, {0,1,0,0}},
          {{1,0,0,0}, {0,1,0,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,1});


  // Test 8: An unobserved source variable p.
  // U = (p,i,j,k)
  // C[i][j] += A[i][k] * B[k][j]
  //
  // combinedKernel = span((1,0,0,0)).
  verify(
      "unobserved source variable",
      4,
      {{
          {{0,1,0,0}, {0,0,0,1}},
          {{0,0,0,1}, {0,0,1,0}},
          {{0,1,0,0}, {0,0,1,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,1});


  // Test 9: Two unobserved source variables.
  //
  // U = (p,q,i,j,k)
  // C[i][j] += A[i][k] * B[k][j]
  //
  // combinedKernel = span(e_p, e_q)
  // rank(combinedKernel) = 2.
  verify(
      "two-dimensional combined kernel",
      5,
      {{
          {{0,0,1,0,0}, {0,0,0,0,1}},
          {{0,0,0,0,1}, {0,0,0,1,0}},
          {{0,0,1,0,0}, {0,0,0,1,0}}
      }},
      {2,2,2, 1,1,1, 0, 3, 3,3,0,2});

  std::cout
      << "All TensorSpaceDependencies mathematical tests passed.\n";

  return 0;
}
