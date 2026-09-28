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


llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsFrontierDP(
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

    if (geometry > rows ||
        geometry > columns)
      continue;
  }

  // ----------------------------------------------------------
  // Canonical orientation.
  //
  // The geometries are square, so rotating the whole rectangle
  // does not change the set of feasible geometry multisets.
  // Keep the narrower dimension as the skyline width.
  // ----------------------------------------------------------

  if (rows > columns)
    std::swap(rows, columns);

  // ----------------------------------------------------------
  // Frontier-state DP.
  //
  // A state is the skyline of the already-covered region:
  //
  //   heights[c] = covered height of column c.
  //
  // The invariant is that every column prefix
  //
  //   [0, heights[c])
  //
  // is occupied.
  //
  // At every state we choose the lowest-leftmost uncovered
  // cell.  Any valid square tiling must contain exactly one
  // square whose lower-left corner is this cell.
  //
  // Therefore this canonical choice removes tile-order
  // permutations without removing any valid tiling.
  //
  // memo[skyline] stores every distinct geometry count vector
  // reachable from that skyline.
  // ----------------------------------------------------------

  using Skyline = std::vector<int64_t>;

  std::map<Skyline, DPResult> memo;

  std::function<const DPResult &(const Skyline &)> solve =
      [&](const Skyline &heights) -> const DPResult & {

        auto existing = memo.find(heights);

        if (existing != memo.end())
          return existing->second;

        DPResult result;

        // ----------------------------------------------------
        // Terminal state:
        //
        // Every column is completely covered.
        // ----------------------------------------------------

        bool complete = true;

        for (int64_t height : heights) {
          if (height != rows) {
            complete = false;
            break;
          }
        }

        if (complete) {
          result.insert(
              CountVector(
                  geometries.size(),
                  0));

          auto inserted =
              memo.emplace(
                  heights,
                  std::move(result));

          return inserted.first->second;
        }

        // ----------------------------------------------------
        // Find the lowest-leftmost uncovered cell.
        // ----------------------------------------------------

        int64_t minimumHeight = rows;

        for (int64_t height : heights)
          minimumHeight =
              std::min(
                  minimumHeight,
                  height);

        size_t leftmost =
            heights.size();

        for (size_t column = 0;
             column < heights.size();
             ++column) {

          if (heights[column] ==
              minimumHeight) {
            leftmost = column;
            break;
          }
        }

        if (leftmost == heights.size()) {
          auto inserted =
              memo.emplace(
                  heights,
                  std::move(result));

          return inserted.first->second;
        }

        // ----------------------------------------------------
        // Try every supported square geometry.
        //
        // A square covering the canonical lowest-leftmost cell
        // must:
        //
        //   1. fit vertically,
        //   2. start at 'leftmost',
        //   3. cover columns whose skyline height is exactly
        //      minimumHeight.
        //
        // Otherwise it would overlap an already occupied cell.
        // ----------------------------------------------------

        for (size_t geometryIndex = 0;
             geometryIndex < geometries.size();
             ++geometryIndex) {

          const int64_t geometry =
              geometries[geometryIndex];

          if (geometry <= 0)
            continue;

          if (minimumHeight + geometry >
              rows)
            continue;

          if (leftmost +
                  static_cast<size_t>(geometry) >
              heights.size())
            continue;

          bool fits = true;

          for (size_t column = leftmost;
               column <
                   leftmost +
                       static_cast<size_t>(geometry);
               ++column) {

            if (heights[column] !=
                minimumHeight) {
              fits = false;
              break;
            }
          }

          if (!fits)
            continue;

          Skyline next =
              heights;

          for (size_t column = leftmost;
               column <
                   leftmost +
                       static_cast<size_t>(geometry);
               ++column) {
            next[column] += geometry;
          }

          const DPResult &suffix =
              solve(next);

          for (const CountVector &counts :
               suffix) {

            CountVector combined =
                counts;

            ++combined[geometryIndex];

            result.insert(
                std::move(combined));
          }
        }

        auto inserted =
            memo.emplace(
                heights,
                std::move(result));

        return inserted.first->second;
      };

  Skyline initial(
      static_cast<size_t>(columns),
      0);

  const DPResult &result =
      solve(initial);

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
