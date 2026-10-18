#include "Systolic/SystolicOptimizer.h"
#include "Systolic/CostModel.h"
#include "Systolic/SystolicTiling.h"

#include "llvm/ADT/SmallVector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

namespace mlir {
namespace systolic {

namespace {

constexpr int64_t kInt64Max = std::numeric_limits<int64_t>::max();

struct BinarySearchPlacement {
  SystolicTile tile;
  int64_t acceleratorId;
  int64_t startCycle;
  int64_t endCycle;
};

static bool checkedMultiply(int64_t a, int64_t b, int64_t &result) {
  if (a < 0 || b < 0)
    return false;
  return !__builtin_mul_overflow(a, b, &result);
}

static bool checkedCeilDiv(int64_t value, int64_t divisor, int64_t &result) {
  if (value <= 0 || divisor <= 0)
    return false;
  result = value / divisor + ((value % divisor) != 0 ? 1 : 0);
  return true;
}

/// One geometry of the fleet, priced by the cost model.
struct GeometryInfo {
  int64_t geometry = 0;
  int64_t blocks = 0;       // geometry / blockSize: its side in grid blocks
  int64_t cost = 0;         // one spatial tile, read + compute + write
  int64_t compute = 0;      // the compute term alone
  int64_t readBytes = 0;    // operand bytes one spatial tile reads
  llvm::SmallVector<int64_t> acceleratorIds;   // in fleet order

  // The stream of tiles one accelerator of this geometry runs, in order, on
  // the cost model's per-accelerator timeline (AcceleratorTimeline): tile n
  // of the stream holds the accelerator over [tileStart[n], tileEnd[n]).
  // Serialized (pipelineDepth = 1) that is [n * cost, (n + 1) * cost); on a
  // pipelined accelerator consecutive tiles overlap and the stream is
  // shorter. computeOnlyEnd[n]: the same stream with transfers free, where
  // start_cycle is measured. Long enough for every tile the search could
  // place on one accelerator.
  std::vector<int64_t> tileStart;
  std::vector<int64_t> tileEnd;
  std::vector<int64_t> computeOnlyEnd;

  /// Tiles one accelerator completes by `deadline`.
  int64_t tilesBy(int64_t deadline) const {
    return static_cast<int64_t>(
        std::upper_bound(tileEnd.begin(), tileEnd.end(), deadline) -
        tileEnd.begin());
  }
};

/// Everything a feasibility test needs, computed once per search.
struct SearchProblem {
  int64_t rows = 0;
  int64_t columns = 0;

  // The padded domain is a grid of blockSize x blockSize blocks, blockSize
  // = gcd of the geometries. Every tile has a side that is a multiple of
  // blockSize and starts on the first uncovered original cell, which is
  // always a block corner (all coverage so far is whole blocks), so the
  // grid places exactly the tiles a cell-by-cell scan would.
  int64_t blockSize = 0;
  int64_t rowBlocks = 0;           // padded domain
  int64_t columnBlocks = 0;
  int64_t originalRowBlocks = 0;   // blocks holding an original cell
  int64_t originalColumnBlocks = 0;
  int64_t smallestTileCount = 0;   // tiles of the smallest geometry to cover

  llvm::SmallVector<GeometryInfo> geometries;   // largest first
  double sharedReadBytesPerCycle = 0.0;
};

/// True if the two configurations price every tile of every reduction the
/// same way, i.e. the cost model cannot tell them apart.
static bool sameCostParameters(const ::systolic::ArrayConfig &a,
                               const ::systolic::ArrayConfig &b) {
  return a.rows == b.rows && a.cols == b.cols && a.kMax == b.kMax &&
         a.tileOverhead == b.tileOverhead &&
         a.readBytesPerCycle == b.readBytesPerCycle &&
         a.writeBytesPerCycle == b.writeBytesPerCycle &&
         a.readOverhead == b.readOverhead &&
         a.writeOverhead == b.writeOverhead &&
         a.operandLanes == b.operandLanes && a.kWindow == b.kWindow &&
         a.elementBytes == b.elementBytes &&
         a.pipelineDepth == b.pipelineDepth &&
         a.reduceCycles == b.reduceCycles && a.feedGap == b.feedGap;
}

static LogicalResult
buildSearchProblem(int64_t rows, int64_t columns, int64_t reduction,
                   llvm::ArrayRef<SystolicArrayResource> fleet,
                   const SystolicOptimizerOptions &options,
                   SearchProblem &problem) {
  if (rows <= 0 || columns <= 0 || reduction <= 0 || fleet.empty())
    return failure();
  if (!std::isfinite(options.sharedReadBytesPerCycle) ||
      options.sharedReadBytesPerCycle < 0.0)
    return failure();

  problem = SearchProblem{};
  problem.rows = rows;
  problem.columns = columns;
  problem.sharedReadBytesPerCycle = options.sharedReadBytesPerCycle;

  // ----------------------------------------------------------
  // Price one spatial tile per geometry with the cost model.
  // ----------------------------------------------------------

  for (size_t i = 0; i < fleet.size(); ++i) {
    const SystolicArrayResource &resource = fleet[i];
    if (resource.arraySize <= 0 ||
        resource.config.rows != resource.arraySize ||
        resource.config.cols != resource.arraySize)
      return failure();

    auto it = std::find_if(problem.geometries.begin(),
                           problem.geometries.end(),
                           [&](const GeometryInfo &g) {
                             return g.geometry == resource.arraySize;
                           });
    if (it != problem.geometries.end()) {
      // One cost per geometry: capacity is #accelerators(g) *
      // floor(T / cost(g)).
      const SystolicArrayResource *first = nullptr;
      for (const SystolicArrayResource &other : fleet)
        if (other.arraySize == resource.arraySize) {
          first = &other;
          break;
        }
      if (!first || !sameCostParameters(first->config, resource.config))
        return failure();
      it->acceleratorIds.push_back(resource.acceleratorId);
      continue;
    }

    const ::systolic::TileCost tile =
        ::systolic::estimateTileCost(reduction, resource.config);
    if (!tile.ok || tile.sequential() < 1)
      return failure();

    GeometryInfo info;
    info.geometry = resource.arraySize;
    info.cost = tile.sequential();
    info.compute = tile.compute;
    info.readBytes = tile.readBytes;
    info.acceleratorIds.push_back(resource.acceleratorId);
    problem.geometries.push_back(info);
  }

  std::sort(problem.geometries.begin(), problem.geometries.end(),
            [](const GeometryInfo &a, const GeometryInfo &b) {
              return a.geometry > b.geometry;
            });

  int64_t blockSize = 0;
  for (const GeometryInfo &g : problem.geometries)
    blockSize = std::gcd(blockSize, g.geometry);
  if (blockSize <= 0)
    return failure();
  problem.blockSize = blockSize;
  for (GeometryInfo &g : problem.geometries)
    g.blocks = g.geometry / blockSize;

  // ----------------------------------------------------------
  // Padded spatial domain: each side rounded up to a multiple of
  // the smallest geometry.
  // ----------------------------------------------------------

  const int64_t smallest = problem.geometries.back().geometry;
  int64_t rowTiles = 0, columnTiles = 0;
  if (!checkedCeilDiv(rows, smallest, rowTiles) ||
      !checkedCeilDiv(columns, smallest, columnTiles) ||
      !checkedMultiply(rowTiles, smallest / blockSize, problem.rowBlocks) ||
      !checkedMultiply(columnTiles, smallest / blockSize,
                       problem.columnBlocks) ||
      !checkedMultiply(rowTiles, columnTiles, problem.smallestTileCount))
    return failure();

  int64_t gridBlocks = 0;
  if (!checkedMultiply(problem.rowBlocks, problem.columnBlocks, gridBlocks) ||
      static_cast<uint64_t>(gridBlocks) >
          static_cast<uint64_t>(std::numeric_limits<size_t>::max()))
    return failure();

  if (!checkedCeilDiv(rows, blockSize, problem.originalRowBlocks) ||
      !checkedCeilDiv(columns, blockSize, problem.originalColumnBlocks))
    return failure();

  // ----------------------------------------------------------
  // Every geometry's tile stream, as long as any accelerator could ever
  // need: at most ceil(rowBlocks / side) * ceil(columnBlocks / side) tiles
  // of a geometry fit the padded domain, so no accelerator runs more.
  // ----------------------------------------------------------

  for (GeometryInfo &g : problem.geometries) {
    int64_t tilesPerRow = 0, tilesPerColumn = 0, maxTiles = 0;
    if (!checkedCeilDiv(problem.rowBlocks, g.blocks, tilesPerRow) ||
        !checkedCeilDiv(problem.columnBlocks, g.blocks, tilesPerColumn) ||
        !checkedMultiply(tilesPerRow, tilesPerColumn, maxTiles))
      return failure();
    const SystolicArrayResource *resource = nullptr;
    for (const SystolicArrayResource &r : fleet)
      if (r.arraySize == g.geometry) {
        resource = &r;
        break;
      }
    if (!resource)
      return failure();
    ::systolic::AcceleratorTimeline full(resource->config);
    ::systolic::AcceleratorTimeline computeOnly(resource->config,
                                                /*transfersFree=*/true);
    g.tileStart.reserve(static_cast<size_t>(maxTiles));
    g.tileEnd.reserve(static_cast<size_t>(maxTiles));
    g.computeOnlyEnd.reserve(static_cast<size_t>(maxTiles));
    for (int64_t n = 0; n < maxTiles; ++n) {
      int64_t start = 0, end = 0, cStart = 0, cEnd = 0;
      if (!full.pushTile(reduction, start, end) ||
          !computeOnly.pushTile(reduction, cStart, cEnd))
        return failure();
      g.tileStart.push_back(start);
      g.tileEnd.push_back(end);
      g.computeOnlyEnd.push_back(cEnd);
    }
  }

  return success();
}

/// Check whether the original rows x columns region can be decomposed and
/// executed completely by deadline T.
///
/// IMPORTANT:
///
/// The padded region is only an implementation domain. Feasibility
/// terminates when all ORIGINAL cells are covered, so a tile whose entire
/// area is padding can never be generated.
static bool binarySearchFeasible(
    const SearchProblem &problem,
    int64_t deadline,
    llvm::SmallVectorImpl<BinarySearchPlacement> *placements,
    int64_t *readBytesOut) {

  if (placements)
    placements->clear();
  if (deadline < 0)
    return false;

  const size_t numGeometries = problem.geometries.size();
  const bool shared = problem.sharedReadBytesPerCycle > 0.0;

  // ----------------------------------------------------------
  // Per-geometry deadline capacity.
  //
  //   tiles / accelerator = tiles of the geometry's stream that end by T
  //                         (floor(T / cost(g)) when nothing overlaps)
  //   total capacity      = #accelerators(g) * tiles / accelerator
  //
  // A capacity too large for int64 is unlimited for any real problem.
  // ----------------------------------------------------------

  llvm::SmallVector<int64_t> capacity(numGeometries, 0);
  for (size_t g = 0; g < numGeometries; ++g) {
    const GeometryInfo &info = problem.geometries[g];
    const int64_t perAccelerator = info.tilesBy(deadline);
    const int64_t count =
        static_cast<int64_t>(info.acceleratorIds.size());
    if (!checkedMultiply(count, perAccelerator, capacity[g]))
      capacity[g] = kInt64Max;
  }

  // ----------------------------------------------------------
  // Occupancy of the padded domain, in blocks. This is NOT the
  // completion condition.
  // ----------------------------------------------------------

  const int64_t rowBlocks = problem.rowBlocks;
  const int64_t columnBlocks = problem.columnBlocks;
  std::vector<uint8_t> covered(
      static_cast<size_t>(rowBlocks * columnBlocks), 0);
  const auto at = [columnBlocks](int64_t r, int64_t c) -> size_t {
    return static_cast<size_t>(r * columnBlocks + c);
  };

  llvm::SmallVector<int64_t> used(numGeometries, 0);
  int64_t readBytes = 0;

  // ----------------------------------------------------------
  // Row-major first uncovered ORIGINAL block.
  //
  // This is the key padding rule: the search covers only blocks that hold
  // an original cell, so a padding-only tile is never created. Tiles only
  // ever cover blocks, so the first uncovered original block moves forward
  // in row-major order and the scan resumes where it stopped instead of
  // restarting at (0, 0) for every tile.
  // ----------------------------------------------------------

  int64_t cursorRow = 0;
  int64_t cursorColumn = 0;

  while (true) {
    while (cursorRow < problem.originalRowBlocks) {
      if (cursorColumn >= problem.originalColumnBlocks) {
        ++cursorRow;
        cursorColumn = 0;
        continue;
      }
      if (covered[at(cursorRow, cursorColumn)] != 0) {
        ++cursorColumn;
        continue;
      }
      break;
    }
    if (cursorRow >= problem.originalRowBlocks)
      break;   // every original cell is covered

    const int64_t startRow = cursorRow;
    const int64_t startColumn = cursorColumn;
    bool placed = false;

    // --------------------------------------------------------
    // Largest geometry first.
    // --------------------------------------------------------

    for (size_t g = 0; g < numGeometries; ++g) {
      const GeometryInfo &info = problem.geometries[g];
      const int64_t side = info.blocks;

      // Spatial fit inside the padded domain.
      if (side > rowBlocks - startRow || side > columnBlocks - startColumn)
        continue;

      // Deadline capacity.
      if (used[g] >= capacity[g])
        continue;

      // Overlap.
      bool legal = true;
      for (int64_t r = startRow; r < startRow + side && legal; ++r)
        for (int64_t c = startColumn; c < startColumn + side; ++c)
          if (covered[at(r, c)] != 0) {
            legal = false;
            break;
          }
      if (!legal)
        continue;

      // ------------------------------------------------------
      // (startRow, startColumn) is an uncovered ORIGINAL block, so this
      // tile contains at least one original element.
      // ------------------------------------------------------

      const int64_t count =
          static_cast<int64_t>(info.acceleratorIds.size());
      const int64_t slot = used[g];
      const int64_t acceleratorIndex = slot % count;
      const int64_t executionRound = slot / count;

      // used[g] < capacity[g] puts this round inside the stream and by the
      // deadline; the interval is the stream's.
      if (executionRound >= static_cast<int64_t>(info.tileEnd.size()) ||
          info.tileEnd[static_cast<size_t>(executionRound)] > deadline)
        continue;
      const int64_t startCycle =
          info.tileStart[static_cast<size_t>(executionRound)];
      const int64_t endCycle =
          info.tileEnd[static_cast<size_t>(executionRound)];

      for (int64_t r = startRow; r < startRow + side; ++r)
        for (int64_t c = startColumn; c < startColumn + side; ++c)
          covered[at(r, c)] = 1;
      ++used[g];

      if (__builtin_add_overflow(readBytes, info.readBytes, &readBytes))
        return false;
      // The shared read bandwidth only ever loses ground as tiles are
      // added: stop as soon as the deadline cannot carry the bytes.
      if (shared && ::systolic::transferCycles(
                        readBytes, problem.sharedReadBytesPerCycle) > deadline)
        return false;

      if (placements) {
        SystolicTile tile;
        tile.row = startRow * problem.blockSize;
        tile.column = startColumn * problem.blockSize;
        tile.size = info.geometry;
        tile.acceleratorSize = info.geometry;
        tile.acceleratorId = info.acceleratorIds[acceleratorIndex];
        placements->push_back(BinarySearchPlacement{
            tile, tile.acceleratorId, startCycle, endCycle});
      }

      placed = true;
      break;
    }

    if (!placed)
      return false;
  }

  if (readBytesOut)
    *readBytesOut = readBytes;
  return true;
}

} // namespace

FailureOr<bool> isSystolicDeadlineFeasible(
    int64_t rows, int64_t columns, int64_t reduction,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const SystolicOptimizerOptions &options, int64_t deadline) {
  SearchProblem problem;
  if (failed(buildSearchProblem(rows, columns, reduction, fleet, options,
                                problem)))
    return failure();
  return binarySearchFeasible(problem, deadline, nullptr, nullptr);
}

FailureOr<ExactSystolicOptimizationResult>
optimizeSystolicBinarySearch(
    int64_t rows,
    int64_t columns,
    int64_t reduction,
    llvm::ArrayRef<SystolicArrayResource> fleet,
    const SystolicOptimizerOptions &options) {

  SearchProblem problem;
  if (failed(buildSearchProblem(rows, columns, reduction, fleet, options,
                                problem)))
    return failure();

  int64_t checks = 0;
  const auto feasible = [&](int64_t deadline) {
    ++checks;
    return binarySearchFeasible(problem, deadline, nullptr, nullptr);
  };

  // ----------------------------------------------------------
  // Binary search for the smallest feasible T in [0, high].
  //
  // feasible(T) is monotone in T when every geometry divides the larger
  // ones (8 and 4): increasing T never removes deadline capacity or shared
  // read bandwidth. Whatever the predicate, the search only ever moves
  // `high` to a deadline it found feasible, so if feasible(high) held on
  // entry, the T it returns is feasible.
  // ----------------------------------------------------------

  const auto search = [&](int64_t high) {
    int64_t low = 0;
    while (low < high) {
      const int64_t mid = low + (high - low) / 2;
      if (feasible(mid))
        high = mid;
      else
        low = mid + 1;
    }
    return low;
  };

  // ----------------------------------------------------------
  // Upper bound: the smallest geometry everywhere, those padded tiles
  // back to back on one accelerator.
  //
  // Without a shared read bandwidth and with geometries that divide each
  // other this deadline is always feasible. It need not be otherwise: the
  // greedy decomposition at that deadline may read more than a shared
  // bandwidth carries by then, or (5 and 3) leave a strip no tile fits.
  // Only then does the bound double until a deadline is feasible, and the
  // search runs again below it -- every result the plain search finds is
  // kept exactly as it was.
  // ----------------------------------------------------------

  int64_t high = 0;
  if (!checkedMultiply(problem.smallestTileCount,
                       problem.geometries.back().cost, high) ||
      high <= 0)
    return failure();

  int64_t low = search(high);
  if (!feasible(low)) {
    while (!feasible(high)) {
      if (high > kInt64Max / 2)
        return failure();
      high *= 2;
    }
    low = search(high);
  }

  // ----------------------------------------------------------
  // Reconstruct at the optimal deadline T*.
  // ----------------------------------------------------------

  llvm::SmallVector<BinarySearchPlacement> finalPlacements;
  int64_t readBytes = 0;
  ++checks;
  if (!binarySearchFeasible(problem, low, &finalPlacements, &readBytes))
    return failure();

  ExactSystolicOptimizationResult result;
  result.makespan = low;
  result.totalReadBytes = readBytes;
  if (problem.sharedReadBytesPerCycle > 0.0)
    result.sharedReadCycles = ::systolic::transferCycles(
        readBytes, problem.sharedReadBytesPerCycle);

  // Compute-only view of the same schedule, per accelerator: tiles placed on
  // the accelerator so far, and the geometry's compute-only stream end for
  // that many tiles.
  llvm::SmallVector<std::pair<int64_t, int64_t>> computeLoad;   // id, tiles

  for (const BinarySearchPlacement &placement : finalPlacements) {
    SystolicExecutionTask task;
    task.row = placement.tile.row;
    task.column = placement.tile.column;
    task.size = placement.tile.size;
    task.acceleratorSize = placement.tile.acceleratorSize;
    task.acceleratorId = placement.acceleratorId;
    result.decomposition.push_back(task);

    ScheduledSystolicTile scheduled;
    scheduled.tile = placement.tile;
    scheduled.startCycle = placement.startCycle;
    scheduled.endCycle = placement.endCycle;
    result.schedule.push_back(scheduled);
    result.scheduleEnd = std::max(result.scheduleEnd, placement.endCycle);

    const GeometryInfo *info = nullptr;
    for (const GeometryInfo &g : problem.geometries)
      if (g.geometry == placement.tile.size)
        info = &g;
    if (!info)
      return failure();
    auto load = std::find_if(computeLoad.begin(), computeLoad.end(),
                             [&](const std::pair<int64_t, int64_t> &e) {
                               return e.first == placement.acceleratorId;
                             });
    if (load == computeLoad.end()) {
      computeLoad.push_back({placement.acceleratorId, 0});
      load = std::prev(computeLoad.end());
    }
    const size_t tiles = static_cast<size_t>(load->second);
    if (tiles >= info->computeOnlyEnd.size())
      return failure();
    ++load->second;
    result.computeOnlyMakespan =
        std::max(result.computeOnlyMakespan, info->computeOnlyEnd[tiles]);
  }

  result.feasibilityChecks = checks;
  return result;
}

} // namespace systolic
} // namespace mlir
