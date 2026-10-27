#include "TensorLogicalSpaces.h"

#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace systolic::logical;

static void check(
    const std::string &name,
    const IntMatrix &a,
    const IntMatrix &b,
    const IntMatrix &c,
    std::size_t n,
    std::array<std::size_t, 4> expected) {

  const auto p = analyzeRankPartition(a, b, c, n);

  const std::array<std::size_t, 4> actual = {
      p.i.size(),
      p.j.size(),
      p.k.size(),
      p.batch.size()
  };

  if (actual != expected)
    throw std::runtime_error(
        "FAIL: " + name + ": unexpected partition ranks");

  const IntMatrix ijk =
      detail::concat(detail::concat(p.i, p.j), p.k);

  const IntMatrix full =
      detail::concat(ijk, p.batch);

  if (detail::rank(full, n) != n)
    throw std::runtime_error(
        "FAIL: " + name + ": incomplete source basis");

  if (p.i.size() + p.j.size() +
      p.k.size() + p.batch.size() != n)
    throw std::runtime_error(
        "FAIL: " + name + ": non-direct decomposition");

  std::cout << "PASS: " << name << '\n';
}

int main() {
  try {
    // U=(i,j,k)
    check(
        "standard GEMM",
        {{1,0,0}, {0,0,1}},
        {{0,0,1}, {0,1,0}},
        {{1,0,0}, {0,1,0}},
        3,
        {1,1,1,0});

    // U=(p,i,j,k)
    // The common p coordinate belongs to Batch.
    check(
        "batched GEMM",
        {{1,0,0,0}, {0,1,0,0}, {0,0,0,1}},
        {{1,0,0,0}, {0,0,1,0}, {0,0,0,1}},
        {{1,0,0,0}, {0,1,0,0}, {0,0,1,0}},
        4,
        {1,1,1,1});

    // U=(i,j,k)
    check(
        "coupled affine indices",
        {{2,0,1}, {1,0,2}},
        {{0,1,2}, {0,2,1}},
        {{1,0,0}, {0,1,0}},
        3,
        {1,1,1,0});

    // U=(i,j,k,l)
    // One source dimension remains outside I/J/K.
    check(
        "k+l reduction",
        {{1,0,0,0}, {0,0,1,1}},
        {{0,0,1,1}, {0,1,0,0}},
        {{1,0,0,0}, {0,1,0,0}},
        4,
        {1,1,1,1});

    // U=(i,j)
    // Genuine three-way dependence is retained in Batch.
    check(
        "essential three-way relation",
        {{1,0}},
        {{0,1}},
        {{1,1}},
        2,
        {0,0,0,2});

    // U=(i,j,k0,k1)
    // A[i][k0][k1] * B[k0+k1][j]
    check(
        "asymmetric reduction ranks",
        {{1,0,0,0}, {0,0,1,0}, {0,0,0,1}},
        {{0,1,0,0}, {0,0,1,1}},
        {{1,0,0,0}, {0,1,0,0}},
        4,
        {1,1,1,1});

    // U=(p,q,i,j,k)
    check(
        "two unobserved source IVs",
        {{0,0,1,0,0}, {0,0,0,0,1}},
        {{0,0,0,0,1}, {0,0,0,1,0}},
        {{0,0,1,0,0}, {0,0,0,1,0}},
        5,
        {1,1,1,2});

    // Arbitrary-rank tensor operands.
    // U=(p,i0,i1,j0,j1,k0,k1)
    check(
        "multidimensional tensor contraction",
        {
          {1,0,0,0,0,0,0},
          {0,1,0,0,0,0,0},
          {0,0,1,0,0,0,0},
          {0,0,0,0,0,1,0},
          {0,0,0,0,0,0,1}
        },
        {
          {1,0,0,0,0,0,0},
          {0,0,0,1,0,0,0},
          {0,0,0,0,1,0,0},
          {0,0,0,0,0,1,0},
          {0,0,0,0,0,0,1}
        },
        {
          {1,0,0,0,0,0,0},
          {0,1,0,0,0,0,0},
          {0,0,1,0,0,0,0},
          {0,0,0,1,0,0,0},
          {0,0,0,0,1,0,0}
        },
        7,
        {2,2,2,1});

    std::cout << "All rank-based partition tests passed.\n";

    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
