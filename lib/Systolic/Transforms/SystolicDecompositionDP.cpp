#include "Systolic/SystolicDecompositionDP.h"

#include <algorithm>
#include <functional>
#include <map>
#include <numeric>
#include <set>
#include <utility>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

bool isPowerOfTwo(int64_t value) {
  return value > 0 &&
         (value & (value - 1)) == 0;
}

using CountVector = std::vector<int64_t>;

using DPResult = std::set<CountVector>;

} // namespace

llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsDP(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries) {

  llvm::SmallVector<SystolicGeometryMultiset> output;

  if (rows <= 0 ||
      columns <= 0 ||
      geometries.empty())
    return output;

  // ----------------------------------------------------------
  // Validate geometry assumptions.
  // ----------------------------------------------------------

  for (int64_t geometry : geometries) {
    if (!isPowerOfTwo(geometry))
      return output;
  }

  int64_t geometryGCD = 0;

  for (int64_t geometry : geometries)
    geometryGCD =
        std::gcd(
            geometryGCD,
            geometry);

  if (geometryGCD <= 0)
    return output;

  // A rectangle tiled entirely by the supported square
  // geometries must have both dimensions aligned to their
  // greatest common divisor.
  if (rows % geometryGCD != 0 ||
      columns % geometryGCD != 0)
    return output;

  // ----------------------------------------------------------
  // DP memo:
  //
  //   memo[{R, C}]
  //
  // stores every unique geometry-count vector that can exactly
  // cover an R x C rectangle.
  // ----------------------------------------------------------

  std::map<std::pair<int64_t, int64_t>, DPResult> memo;

  std::function<const DPResult &(int64_t, int64_t)> solve =
      [&](int64_t R, int64_t C) -> const DPResult & {

        // Square logical geometries are invariant under a
        // 90-degree rotation. Canonicalize every rectangle so
        // that R <= C, allowing R x C and C x R to share the
        // same DP state and recurrence expansion.
        if (R > C)
          std::swap(R, C);

        const std::pair<int64_t, int64_t> key{R, C};

        auto existing = memo.find(key);

        if (existing != memo.end())
          return existing->second;

        DPResult result;

        // ----------------------------------------------------
        // Case 1:
        //
        // The entire rectangle is directly covered by one
        // square systolic geometry.
        // ----------------------------------------------------

        if (R == C) {
          for (size_t geometryIndex = 0;
               geometryIndex < geometries.size();
               ++geometryIndex) {

            if (geometries[geometryIndex] != R)
              continue;

            CountVector counts(
                geometries.size(),
                0);

            counts[geometryIndex] = 1;

            result.insert(counts);
          }
        }

        // ----------------------------------------------------
        // Case 2:
        //
        // Horizontal split:
        //
        //      R x C
        //
        //   = r x C
        //     +
        //     (R-r) x C
        // ----------------------------------------------------

        for (int64_t r = geometryGCD;
             r <= R / 2;
             r += geometryGCD) {
          const DPResult &top =
              solve(r, C);

          const DPResult &bottom =
              solve(R - r, C);

          for (const CountVector &lhs : top) {
            for (const CountVector &rhs : bottom) {
              CountVector combined(
                  geometries.size(),
                  0);

              for (size_t i = 0;
                   i < geometries.size();
                   ++i) {
                combined[i] =
                    lhs[i] + rhs[i];
              }

              result.insert(combined);
            }
          }
        }

        // ----------------------------------------------------
        // Case 3:
        //
        // Vertical split:
        //
        //   R x C
        //
        //   = R x c  +  R x (C-c)
        // ----------------------------------------------------

        for (int64_t c = geometryGCD;
             c <= C / 2;
             c += geometryGCD) {
          const DPResult &left =
              solve(R, c);

          const DPResult &right =
              solve(R, C - c);

          for (const CountVector &lhs : left) {
            for (const CountVector &rhs : right) {
              CountVector combined(
                  geometries.size(),
                  0);

              for (size_t i = 0;
                   i < geometries.size();
                   ++i) {
                combined[i] =
                    lhs[i] + rhs[i];
              }

              result.insert(combined);
            }
          }
        }

        auto inserted =
            memo.emplace(
                key,
                std::move(result));

        return inserted.first->second;
      };

  const DPResult &result =
      solve(rows, columns);

  for (const CountVector &counts : result) {
    SystolicGeometryMultiset multiset;

    multiset.counts.assign(
        counts.begin(),
        counts.end());

    output.push_back(
        std::move(multiset));
  }

  return output;
}

} // namespace systolic
} // namespace mlir
