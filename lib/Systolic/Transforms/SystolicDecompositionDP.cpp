#include "Systolic/SystolicDecompositionDP.h"

#include <array>
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

  // Packed CountVector.
  //
  // Each geometry count receives 20 bits.
  //
  // This supports counts up to:
  //
  //   2^20 - 1 = 1,048,575
  //
  // which is sufficient for the current generalized
  // 512x1000 case:
  //
  //   max count of 2x2 tiles
  //       = 512000 / 4
  //       = 128000.
  //
  // Seven geometries therefore require:
  //
  //   7 * 20 = 140 bits
  //
  // and are stored in three uint64_t words.
  //
  // The representation is independent of geometry order:
  // counts[i] occupies bits [20*i, 20*i+19].
  // ----------------------------------------------------------

  constexpr unsigned countBits = 20;
  constexpr uint64_t countMask =
      (uint64_t{1} << countBits) - 1;

  struct PackedCountVector {
    std::array<uint64_t, 3> words{};

    bool operator<(const PackedCountVector &other) const {
      if (words[0] != other.words[0])
        return words[0] < other.words[0];

      if (words[1] != other.words[1])
        return words[1] < other.words[1];

      return words[2] < other.words[2];
    }
  };

  auto getCount =
      [](const PackedCountVector &value,
         size_t index) -> uint64_t {
    const unsigned bit =
        static_cast<unsigned>(index) * countBits;

    const size_t word =
        bit / 64;

    const unsigned offset =
        bit % 64;

    if (offset + countBits <= 64) {
      return
          (value.words[word] >> offset) &
          countMask;
    }

    // A 20-bit field can cross a 64-bit word boundary.
    const unsigned lowBits =
        64 - offset;

    const unsigned highBits =
        countBits - lowBits;

    const uint64_t low =
        value.words[word] >> offset;

    const uint64_t highMask =
        (uint64_t{1} << highBits) - 1;

    const uint64_t high =
        value.words[word + 1] &
        highMask;

    return
        low |
        (high << lowBits);
  };

  auto incrementCount =
      [](PackedCountVector value,
         size_t index)
      -> PackedCountVector {

    const unsigned bit =
        static_cast<unsigned>(index) * countBits;

    const size_t word =
        bit / 64;

    const unsigned offset =
        bit % 64;

    const uint64_t current =
        (value.words[word] >> offset) &
        countMask;

    if (current >= countMask)
      return value;

    const uint64_t next =
        current + 1;

    const uint64_t clearMask =
        countMask << offset;

    if (offset + countBits <= 64) {

      value.words[word] =
          (value.words[word] & ~clearMask) |
          (next << offset);

      return value;
    }

    // Field crosses a 64-bit boundary.
    const unsigned lowBits =
        64 - offset;

    const unsigned highBits =
        countBits - lowBits;

    const uint64_t lowMask =
        (uint64_t{1} << lowBits) - 1;

    const uint64_t lowPart =
        next & lowMask;

    const uint64_t highPart =
        next >> lowBits;

    value.words[word] =
        (value.words[word] &
         ~(lowMask << offset)) |
        (lowPart << offset);

    const uint64_t highMask =
        (uint64_t{1} << highBits) - 1;

    value.words[word + 1] =
        (value.words[word + 1] &
         ~highMask) |
        highPart;

    return value;
  };

  using Skyline = std::vector<int64_t>;
  using PackedDPResult =
      std::set<PackedCountVector>;

  // ----------------------------------------------------------
  // Skyline -> packed count vectors.
  // ----------------------------------------------------------

  std::map<Skyline, PackedDPResult> memo;

  std::function<
      const PackedDPResult &(const Skyline &)>
      solve =
      [&](const Skyline &heights)
          -> const PackedDPResult & {

        auto existing =
            memo.find(heights);

        if (existing != memo.end())
          return existing->second;

        PackedDPResult result;

        // ----------------------------------------------------
        // Terminal state.
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
              PackedCountVector{});

          auto inserted =
              memo.emplace(
                  heights,
                  std::move(result));

          return inserted.first->second;
        }

        // ----------------------------------------------------
        // Lowest-leftmost uncovered cell.
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
        // Try every geometry.
        // ----------------------------------------------------

        for (size_t geometryIndex = 0;
             geometryIndex < geometries.size();
             ++geometryIndex) {

          const int64_t geometry =
              geometries[geometryIndex];

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

          const PackedDPResult &suffix =
              solve(next);

          for (const PackedCountVector &counts :
               suffix) {

            PackedCountVector combined =
                incrementCount(
                    counts,
                    geometryIndex);

            result.insert(
                combined);
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

  const PackedDPResult &result =
      solve(initial);

  // ----------------------------------------------------------
  // Decode packed representation back into the public API.
  // ----------------------------------------------------------

  for (const PackedCountVector &packed :
       result) {

    SystolicGeometryMultiset multiset;

    multiset.counts.resize(
        geometries.size());

    for (size_t i = 0;
         i < geometries.size();
         ++i) {

      multiset.counts[i] =
          static_cast<int64_t>(
              getCount(packed, i));
    }

    output.push_back(
        std::move(multiset));
  }

  return output;
}


llvm::SmallVector<SystolicGeometryMultiset>
enumerateSystolicGeometryMultisetsAreaFirstDP(
    int64_t rows,
    int64_t columns,
    llvm::ArrayRef<int64_t> geometries) {

  llvm::SmallVector<SystolicGeometryMultiset> output;

  if (rows <= 0 ||
      columns <= 0 ||
      geometries.empty())
    return output;

  // ----------------------------------------------------------
  // Validate geometries and compute the target area.
  // ----------------------------------------------------------

  for (int64_t geometry : geometries) {
    if (!isPowerOfTwo(geometry))
      return output;
  }

  if (rows >
      std::numeric_limits<int64_t>::max() /
          columns)
    return output;

  const int64_t targetArea =
      rows * columns;

  llvm::SmallVector<int64_t> tileAreas;

  tileAreas.reserve(
      geometries.size());

  for (int64_t geometry : geometries) {

    if (geometry >
            std::numeric_limits<int64_t>::max() /
                geometry)
      return output;

    tileAreas.push_back(
        geometry * geometry);
  }

  // ----------------------------------------------------------
  // Canonical orientation.
  //
  // All geometries are square, so rotation preserves tileability.
  // ----------------------------------------------------------

  if (rows > columns)
    std::swap(rows, columns);

  // ----------------------------------------------------------
  // Stage 1:
  //
  // Enumerate every CountVector satisfying:
  //
  //   sum_i counts[i] * area_i = targetArea
  //
  // This is a necessary condition, not a sufficient one.
  //
  // Therefore these candidates are subsequently checked by the
  // exact skyline tileability search below.
  // ----------------------------------------------------------

  llvm::SmallVector<CountVector> areaCandidates;

  CountVector currentCounts(
      geometries.size(),
      0);

  std::function<void(size_t, int64_t)> enumerateCounts =
      [&](size_t index,
          int64_t remainingArea) {

        if (index == geometries.size()) {

          if (remainingArea == 0)
            areaCandidates.push_back(
                currentCounts);

          return;
        }

        const int64_t tileArea =
            tileAreas[index];

        if (tileArea <= 0)
          return;

        const int64_t maxCount =
            remainingArea / tileArea;

        for (int64_t count = 0;
             count <= maxCount;
             ++count) {

          currentCounts[index] =
              count;

          enumerateCounts(
              index + 1,
              remainingArea -
                  count * tileArea);
        }

        currentCounts[index] = 0;
      };

  enumerateCounts(
      0,
      targetArea);

  // ----------------------------------------------------------
  // Stage 2:
  //
  // Exact tileability test for one CountVector.
  //
  // We use the same canonical lowest-leftmost skyline rule as
  // FrontierDP.  The difference is that the CountVector is fixed:
  //
  //   remainingCounts[i]
  //
  // records how many tiles of geometry i still have to be placed.
  //
  // A state is therefore:
  //
  //   (skyline heights, remaining geometry counts)
  //
  // A failed state is memoized.
  // ----------------------------------------------------------

  struct TileabilityState {
    std::vector<int64_t> heights;
    CountVector remainingCounts;

    bool operator<(const TileabilityState &other) const {
      if (heights != other.heights)
        return heights < other.heights;

      return remainingCounts <
             other.remainingCounts;
    }
  };

  std::function<bool(
      const CountVector &)>
      isTileable =
          [&](const CountVector &counts) {

    std::vector<int64_t> initialHeights(
        static_cast<size_t>(columns),
        0);

    std::set<TileabilityState> failedStates;

    std::function<bool(
        const std::vector<int64_t> &,
        const CountVector &)>
        solve =
            [&](const std::vector<int64_t> &heights,
                const CountVector &remainingCounts)
                -> bool {

      // ------------------------------------------------------
      // Terminal condition.
      // ------------------------------------------------------

      bool complete = true;

      for (int64_t height : heights) {
        if (height != rows) {
          complete = false;
          break;
        }
      }

      if (complete) {
        for (int64_t count :
             remainingCounts) {
          if (count != 0)
            return false;
        }

        return true;
      }

      // ------------------------------------------------------
      // Memoize failed states.
      // ------------------------------------------------------

      TileabilityState state{
          heights,
          remainingCounts};

      if (failedStates.find(state) !=
          failedStates.end())
        return false;

      // ------------------------------------------------------
      // Find the lowest-leftmost uncovered cell.
      // ------------------------------------------------------

      int64_t minimumHeight =
          rows;

      for (int64_t height :
           heights) {
        minimumHeight =
            std::min(
                minimumHeight,
                height);
      }

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
        failedStates.insert(
            std::move(state));
        return false;
      }

      // ------------------------------------------------------
      // Try every geometry that still has remaining copies.
      // ------------------------------------------------------

      for (size_t geometryIndex = 0;
           geometryIndex < geometries.size();
           ++geometryIndex) {

        if (remainingCounts[geometryIndex] <= 0)
          continue;

        const int64_t geometry =
            geometries[geometryIndex];

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

        std::vector<int64_t> nextHeights =
            heights;

        for (size_t column = leftmost;
             column <
                 leftmost +
                     static_cast<size_t>(geometry);
             ++column) {

          nextHeights[column] +=
              geometry;
        }

        CountVector nextCounts =
            remainingCounts;

        --nextCounts[geometryIndex];

        if (solve(
                nextHeights,
                nextCounts))
          return true;
      }

      failedStates.insert(
          std::move(state));

      return false;
    };

    return solve(
        initialHeights,
        counts);
  };

  // ----------------------------------------------------------
  // Filter the area-feasible CountVectors using the exact
  // tileability oracle.
  // ----------------------------------------------------------

  for (const CountVector &counts :
       areaCandidates) {

    if (!isTileable(counts))
      continue;

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
