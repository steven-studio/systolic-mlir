#pragma once

#include "Systolic/RankPartitionDependenceLegality.h"

#include "llvm/ADT/SmallVector.h"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.2b
//
// Check whether an integer affine schedule:
//
//   theta(x) = S*x
//
// strictly preserves every feasible ordered write dependence:
//
//   (x1, x2) in D => theta(x1) <lex theta(x2)
//
// A violation exists when:
//
//   theta(x2) <=lex theta(x1)
//
// This includes both reversed order and equal timestamps.
//
// Extended dependence relation layout:
//
//   [x1(n), z1(n), x2(n), z2(n), locals, constant]
//
// No source iteration enumeration.
// No assumption that Batch dimensions are output-independent.
//
// IMPORTANT:
//
// This checks ordered output-write dependences ONLY.
// It is not a complete lowering/parallelization certificate.

struct ScheduleLegalityResult {
  bool preservesOrderedWrites = false;

  std::size_t testedDependenceBranches = 0;
  std::size_t testedViolationBranches = 0;
  std::size_t feasibleViolationBranches = 0;
};

inline std::optional<ScheduleLegalityResult>
checkOrderedWriteSchedule(
    const OrderedWriteConflictAnalysis &conflicts,
    const std::vector<std::vector<BigInt>> &schedule,
    std::size_t sourceRank) {

  using mlir::presburger::IntegerRelation;

  if (sourceRank == 0 || schedule.empty())
    return std::nullopt;

  if (sourceRank >
      std::numeric_limits<unsigned>::max() / 4)
    return std::nullopt;

  // Validate schedule dimensions and coefficient ranges.
  //
  // Both +S and -S are required. Restrict each coefficient
  // to [-INT64_MAX, INT64_MAX] so negation remains safe.

  const BigInt maxCoefficient(
      std::numeric_limits<std::int64_t>::max());

  for (const auto &row : schedule) {
    if (row.size() != sourceRank)
      return std::nullopt;

    for (const BigInt &coefficient : row) {
      if (coefficient > maxCoefficient ||
          coefficient < -maxCoefficient)
        return std::nullopt;
    }
  }

  const std::size_t n = sourceRank;
  const std::size_t m = schedule.size();

  ScheduleLegalityResult result;

  result.testedDependenceBranches =
      conflicts.feasibleBranches.size();

  // Each dependence branch D is intersected with the
  // disjunction:
  //
  //   theta(x2) <lex theta(x1)
  //       OR
  //   theta(x2) = theta(x1)
  //
  // There are m strict reverse-order branches and
  // one all-equal branch.

  for (const IntegerRelation &dependence :
       conflicts.feasibleBranches) {

    if (dependence.getNumDomainVars() != 2 * n ||
        dependence.getNumRangeVars() != 2 * n ||
        dependence.getNumSymbolVars() != 0)
      return std::nullopt;

    const std::size_t locals =
        dependence.getNumLocalVars();

    const std::size_t columns = 4 * n + locals + 1;

    if (dependence.getNumCols() != columns)
      return std::nullopt;

    // Build theta_r(x1) - theta_r(x2).
    //
    // Source x1 coefficients begin at column 0.
    // Source x2 coefficients begin at column 2*n.
    //
    // Logical z coordinates and local variables
    // have zero coefficients.

    auto differenceRow = [&](std::size_t r) {
      llvm::SmallVector<llvm::DynamicAPInt> row(
          columns, llvm::DynamicAPInt(0));

      for (std::size_t i = 0; i < n; ++i) {
        const std::int64_t coefficient =
            schedule[r][i].convert_to<std::int64_t>();

        row[i] = llvm::DynamicAPInt(coefficient);

        row[2 * n + i] =
            llvm::DynamicAPInt(-coefficient);
      }

      return row;
    };

    // Reverse lexicographic violation:
    //
    // theta_0(x1) == theta_0(x2)
    // ...
    // theta_(r-1)(x1) == theta_(r-1)(x2)
    //
    // theta_r(x1) > theta_r(x2)
    //
    // Integer strict inequality:
    //
    // theta_r(x1) - theta_r(x2) - 1 >= 0.

    for (std::size_t r = 0; r < m; ++r) {
      IntegerRelation violation = dependence;

      for (std::size_t prefix = 0;
           prefix < r; ++prefix) {
        violation.addEquality(differenceRow(prefix));
      }

      auto reversed = differenceRow(r);

      reversed.back() = llvm::DynamicAPInt(-1);

      violation.addInequality(reversed);

      ++result.testedViolationBranches;

      if (!violation.isIntegerEmpty())
        ++result.feasibleViolationBranches;
    }

    // Equal schedule timestamps are also forbidden
    // for an ordered dependence, unless an independent
    // intra-timestamp ordering mechanism is certified.
    //
    // This phase provides no such mechanism.

    IntegerRelation tied = dependence;

    for (std::size_t r = 0; r < m; ++r)
      tied.addEquality(differenceRow(r));

    ++result.testedViolationBranches;

    if (!tied.isIntegerEmpty())
      ++result.feasibleViolationBranches;
  }

  result.preservesOrderedWrites =
      result.feasibleViolationBranches == 0;

  return result;
}

} // namespace systolic::logical
