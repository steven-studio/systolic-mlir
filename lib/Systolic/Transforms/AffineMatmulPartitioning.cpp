#include "Systolic/AffineMatmulPartitioning.h"

#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace mlir;
using namespace mlir::systolic;

static LogicalResult fail(std::string *why, const llvm::Twine &message) {
  if (why)
    *why = message.str();
  return failure();
}

FailureOr<DenseLogicalGemm>
mlir::systolic::getDenseLogicalGemm(const LogicalMatmul &matmul,
                                    std::string *why) {
  const AffineLogicalNormalization &norm = matmul.normalization;
  if (!norm.domain)
    return fail(why, "the loop bounds are not static, so the iteration "
                     "domain has no static normalized extent");

  // y = (P*, I, J, K): a nonempty P* is a batched GEMM.
  const auto &T = norm.logicalTransformation;
  if (T.size() != 3)
    return fail(why, "batched GEMM (a partition prefix P* of " +
                         llvm::Twine(T.size() >= 3 ? T.size() - 3 : 0) +
                         " coordinates) is not supported");

  const unsigned numSourceDims = norm.domain->getNumDimVars();
  if (numSourceDims != 3 || norm.logicalOffset.size() != 3)
    return fail(why, "the iteration domain is not three-dimensional");

  static constexpr const char *kName[] = {"I", "J", "K"};
  DenseLogicalGemm dense;
  std::array<bool, 3> used{};
  for (unsigned d = 0; d < 3; ++d) {
    int64_t scale = 0;
    unsigned sourceDim = 0;
    unsigned nonzero = 0;
    for (unsigned s = 0; s < numSourceDims; ++s) {
      if (T[d][s] == 0)
        continue;
      ++nonzero;
      scale = T[d][s];
      sourceDim = s;
    }
    if (nonzero != 1)
      return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                           " depends on " + llvm::Twine(nonzero) +
                           " loops; only one loop per coordinate is "
                           "supported");
    if (scale <= 0)
      return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                           " decreases along its loop");
    if (used[sourceDim])
      return fail(why, "two logical coordinates are driven by the same loop");
    used[sourceDim] = true;

    std::optional<int64_t> lb = norm.domain->getConstantBound64(
        presburger::BoundType::LB, sourceDim);
    std::optional<int64_t> ub = norm.domain->getConstantBound64(
        presburger::BoundType::UB, sourceDim);
    if (!lb || !ub || *lb != 0 || *ub < 0)
      return fail(why, "the normalized iteration domain is not a dense box");

    dense.sourceDim[d] = sourceDim;
    dense.scale[d] = scale;
    dense.offset[d] = norm.logicalOffset[d];
    dense.extent[d] = *ub + 1;
  }
  return dense;
}

FailureOr<SmallVector<LogicalPartition, 1>>
mlir::systolic::partitionLogicalMatmul(const LogicalMatmul &matmul) {
  if (!matmul.sourceDomain)
    return failure();

  // Default policy: preserve the complete logical execution region.
  // Hardware-specific boundaries (4x4, 8x8, k_max) are the Systolic Tiling
  // stage's; see the overload below.
  LogicalPartition whole{*matmul.sourceDomain, std::nullopt};
  if (auto dense = getDenseLogicalGemm(matmul); succeeded(dense)) {
    LogicalBox box;
    box.extent = dense->extent;
    whole.logicalBox = box;
  }

  SmallVector<LogicalPartition, 1> partitions;
  partitions.push_back(std::move(whole));
  return partitions;
}

/// The source iteration domain restricted to `box`. Normalized ordinal z_s
/// is source coordinate x_s = origin_s + step_s * z_s, so ordinals
/// [lo, lo + ext) are x_s in [origin_s + step_s * lo, origin_s + step_s *
/// (lo + ext)). The x interval is taken half-open on the next region's first
/// coordinate rather than closed on this region's last one, so consecutive
/// regions cover x_s without gaps whether or not the source domain encodes
/// the loop's step.
static affine::FlatAffineValueConstraints
restrictToBox(const LogicalMatmul &matmul, const DenseLogicalGemm &dense,
              const LogicalBox &box) {
  affine::FlatAffineValueConstraints domain = *matmul.sourceDomain;
  const AffineLogicalNormalization &norm = matmul.normalization;
  for (unsigned d = 0; d < 3; ++d) {
    const unsigned s = dense.sourceDim[d];
    const int64_t step = norm.sourceTransformation[s][s];
    const int64_t origin = norm.sourceOffset[s];
    domain.addBound(presburger::BoundType::LB, s,
                    origin + step * box.lower[d]);
    domain.addBound(presburger::BoundType::UB, s,
                    origin + step * (box.lower[d] + box.extent[d]) - 1);
  }
  return domain;
}

/// Every point of the dense box [0, extent) is in exactly one of `boxes`
/// (all already known to lie inside it).
///
/// The axes are cut at every box boundary; each cell of the resulting grid
/// is either inside or outside each box, so counting how many boxes cover
/// each cell decides exactness without enumerating iteration points. The
/// boxes of a systolic plan cover one or two cells per axis each, so the
/// work is about the number of cells.
static LogicalResult checkExactCover(const DenseLogicalGemm &dense,
                                     ArrayRef<LogicalBox> boxes,
                                     std::string *why) {
  std::array<std::vector<int64_t>, 3> cuts;
  for (unsigned d = 0; d < 3; ++d) {
    cuts[d].push_back(0);
    cuts[d].push_back(dense.extent[d]);
    for (const LogicalBox &box : boxes) {
      cuts[d].push_back(box.lower[d]);
      cuts[d].push_back(box.lower[d] + box.extent[d]);
    }
    llvm::sort(cuts[d]);
    cuts[d].erase(std::unique(cuts[d].begin(), cuts[d].end()),
                  cuts[d].end());
  }

  const uint64_t cellsI = cuts[0].size() - 1;
  const uint64_t cellsJ = cuts[1].size() - 1;
  const uint64_t cellsK = cuts[2].size() - 1;
  constexpr uint64_t kMaxCells = uint64_t(1) << 27;
  if (cellsI * cellsJ > kMaxCells || cellsI * cellsJ * cellsK > kMaxCells)
    return fail(why, "the partition has too many distinct boundaries to "
                     "verify (" +
                         llvm::Twine(cellsI) + " x " + llvm::Twine(cellsJ) +
                         " x " + llvm::Twine(cellsK) + " cells)");

  std::vector<uint8_t> coverCount(cellsI * cellsJ * cellsK, 0);
  auto cellRange = [&](unsigned d, const LogicalBox &box) {
    auto first = llvm::lower_bound(cuts[d], box.lower[d]);
    auto last = llvm::lower_bound(cuts[d], box.lower[d] + box.extent[d]);
    return std::make_pair<uint64_t, uint64_t>(first - cuts[d].begin(),
                                              last - cuts[d].begin());
  };
  for (auto [index, box] : llvm::enumerate(boxes)) {
    auto [i0, i1] = cellRange(0, box);
    auto [j0, j1] = cellRange(1, box);
    auto [k0, k1] = cellRange(2, box);
    for (uint64_t i = i0; i < i1; ++i)
      for (uint64_t j = j0; j < j1; ++j)
        for (uint64_t k = k0; k < k1; ++k) {
          uint8_t &count = coverCount[(i * cellsJ + j) * cellsK + k];
          if (count != 0)
            return fail(why, "region " + llvm::Twine(index) +
                                 " overlaps an earlier region at (I, J, K) "
                                 "= (" +
                                 llvm::Twine(cuts[0][i]) + ", " +
                                 llvm::Twine(cuts[1][j]) + ", " +
                                 llvm::Twine(cuts[2][k]) +
                                 "): those iterations would run twice");
          count = 1;
        }
  }
  for (uint64_t i = 0; i < cellsI; ++i)
    for (uint64_t j = 0; j < cellsJ; ++j)
      for (uint64_t k = 0; k < cellsK; ++k)
        if (coverCount[(i * cellsJ + j) * cellsK + k] == 0)
          return fail(why, "no region covers (I, J, K) = (" +
                               llvm::Twine(cuts[0][i]) + ", " +
                               llvm::Twine(cuts[1][j]) + ", " +
                               llvm::Twine(cuts[2][k]) +
                               "): those iterations would be lost");
  return success();
}

/// The same property again, in the source coordinates x and with the
/// Presburger library rather than the grid: the regions' iteration domains
/// cover the source domain, and no two of them share a point. This checks
/// the ordinal -> source mapping (restrictToBox) against the loop nest's own
/// domain. Pairwise intersection is quadratic, so it runs for plans of up to
/// kMaxPresburgerRegions regions; checkExactCover has already established
/// exactness in ordinal space for every plan.
static constexpr size_t kMaxPresburgerRegions = 64;

static LogicalResult
checkSourceCover(const LogicalMatmul &matmul,
                 ArrayRef<LogicalPartition> partitions, std::string *why) {
  if (partitions.size() > kMaxPresburgerRegions)
    return success();

  presburger::PresburgerSet covered = presburger::PresburgerSet::getEmpty(
      matmul.sourceDomain->getSpace());
  for (const LogicalPartition &partition : partitions)
    covered = covered.unionSet(
        presburger::PresburgerSet(partition.iterationDomain));

  presburger::PresburgerSet source(*matmul.sourceDomain);
  if (!source.subtract(covered).isIntegerEmpty())
    return fail(why, "the regions do not cover the source iteration domain");
  if (!covered.subtract(source).isIntegerEmpty())
    return fail(why, "a region extends outside the source iteration domain");

  for (size_t a = 0; a < partitions.size(); ++a)
    for (size_t b = 0; b < a; ++b) {
      // intersect, not append: each domain's local variables (the loop
      // steps' quotients) are its own existentials.
      presburger::IntegerRelation both =
          partitions[a].iterationDomain.intersect(
              partitions[b].iterationDomain);
      if (!both.isIntegerEmpty())
        return fail(why, "regions " + llvm::Twine(b) + " and " +
                             llvm::Twine(a) +
                             " share source iterations");
    }
  return success();
}

FailureOr<SmallVector<LogicalPartition, 1>>
mlir::systolic::partitionLogicalMatmul(const LogicalMatmul &matmul,
                                       ArrayRef<LogicalBox> boxes,
                                       std::string *why) {
  if (!matmul.sourceDomain)
    return fail(why, "the GEMM has no source iteration domain");

  FailureOr<DenseLogicalGemm> dense = getDenseLogicalGemm(matmul, why);
  if (failed(dense))
    return failure();

  if (boxes.empty())
    return fail(why, "no regions");

  for (auto [index, box] : llvm::enumerate(boxes))
    for (unsigned d = 0; d < 3; ++d)
      if (box.extent[d] < 1 || box.lower[d] < 0 ||
          box.lower[d] > dense->extent[d] - box.extent[d])
        return fail(why, "region " + llvm::Twine(index) +
                             " is empty or extends outside the iteration "
                             "domain");

  if (failed(checkExactCover(*dense, boxes, why)))
    return failure();

  SmallVector<LogicalPartition, 1> partitions;
  partitions.reserve(boxes.size());
  for (const LogicalBox &box : boxes)
    partitions.push_back(
        LogicalPartition{restrictToBox(matmul, *dense, box), box});

  if (failed(checkSourceCover(matmul, partitions, why)))
    return failure();

  return partitions;
}
