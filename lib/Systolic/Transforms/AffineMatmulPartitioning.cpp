#include "Systolic/AffineMatmulPartitioning.h"

#include "mlir/Analysis/Presburger/PresburgerRelation.h"

#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CheckedArithmetic.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <optional>
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
  if (!norm.domain) {
    // Overflow is reported only when normalizeLogicalCoordinates saw a
    // checked operation fail; every other nest without a normalized form
    // keeps the wording below.
    if (matmul.normalizationOverflow)
      return fail(why, "the iteration domain's normalized form overflows "
                       "64-bit arithmetic");
    // An empty GEMM (M, N or K = 0): whether it should be offloaded with
    // zero invocations is open (SQ-2); refused, with this reason.
    if (matmul.normalizationEmptyLoop)
      return fail(why, "a loop of the nest has no iteration (an empty GEMM: "
                       "M, N or K is 0)");
    return fail(why, "the loop bounds are not static, so the iteration "
                     "domain has no static normalized extent");
  }

  // y = (P*, I, J, K): a nonempty P* is a batched GEMM.
  const auto &T = norm.logicalTransformation;
  if (T.size() != 3)
    return fail(why, "batched GEMM (a partition prefix P* of " +
                         llvm::Twine(T.size() >= 3 ? T.size() - 3 : 0) +
                         " coordinates) is not supported");

  const unsigned numSourceDims = norm.domain->getNumDimVars();
  if (numSourceDims < 3 || norm.logicalOffset.size() != 3)
    return fail(why, "the iteration domain has fewer than three dimensions");

  // Each loop of the nest drives exactly one logical coordinate (the
  // recognizer has already required every loop's IV to be used). A
  // coordinate may be driven by any number of loops -- a tiled or split loop
  // nest, at any depth -- as long as they enumerate it densely: sorted by
  // scale, the finest has scale s_0 and each next one the scale of the
  // previous times its extent (a mixed-radix numbering), so the coordinate
  // takes the values offset + s_0 * (0 .. product of extents - 1), each once.
  static constexpr const char *kName[] = {"I", "J", "K"};
  SmallVector<int64_t> extentOf(numSourceDims, 0);
  for (unsigned s = 0; s < numSourceDims; ++s) {
    unsigned drives = 0;
    for (unsigned d = 0; d < 3; ++d)
      drives += T[d][s] != 0;
    if (drives > 1) {
      std::string coords;
      for (unsigned d = 0; d < 3; ++d)
        if (T[d][s] != 0)
          coords += (coords.empty() ? "" : " and ") + std::string(kName[d]);
      return fail(why, "two logical coordinates are driven by the same loop "
                       "(loop " + llvm::Twine(s) + " drives " + coords + ")");
    }
    if (drives == 0)
      return fail(why, "loop " + llvm::Twine(s) +
                           " drives no logical coordinate");
    std::optional<int64_t> lb = norm.domain->getConstantBound64(
        presburger::BoundType::LB, s);
    std::optional<int64_t> ub = norm.domain->getConstantBound64(
        presburger::BoundType::UB, s);
    if (!lb || !ub || *lb != 0 || *ub < 0)
      return fail(why, "the normalized iteration domain is not a dense box");
    std::optional<int64_t> trips = llvm::checkedAdd(*ub, int64_t(1));
    if (!trips)
      return fail(why, "loop " + llvm::Twine(s) +
                           "'s trip count overflows 64-bit arithmetic");
    extentOf[s] = *trips;
  }

  DenseLogicalGemm dense;
  for (unsigned d = 0; d < 3; ++d) {
    SmallVector<unsigned, 2> loops;
    for (unsigned s = 0; s < numSourceDims; ++s)
      if (T[d][s] != 0)
        loops.push_back(s);
    // A coordinate no loop drives is a constant subscript (C[0][j] with
    // A[0][k]): its extent would be 1, but whether such a nest is a GEMM to
    // offload with M, N or K = 1 is not decided by the specification
    // (test/gemm_recognition/SPEC_QUESTIONS.md, SQ-4). Refuse it with a
    // reason -- and never index loops.front() on an empty list.
    if (loops.empty())
      return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                           " is driven by no loop (a constant subscript)");
    // A loop along which the coordinate decreases (a negative coefficient:
    // a C loop that counts down, or a reversed subscript such as 7 - i)
    // enumerates the same values in the opposite order. With z in [0, n),
    //   T z = T (n - 1) + |T| (n - 1 - z),
    // so reading that loop's ordinal backwards gives the coordinate as
    // offset' + |T| z' with z' in [0, n): offset' adds T (n - 1) for every
    // such loop, and the density test below runs on the magnitudes. The set
    // of (I, J, K) points is unchanged; only the order the loop nest visits
    // them in differs -- immaterial for I and J (each point writes its own C
    // element), and for K the reordering of the sums that the systolic
    // lowering performs anyway (see AffineMatmulToSystolic.cpp, header).
    SmallVector<int64_t> magnitude(numSourceDims, 0);
    int64_t offset = norm.logicalOffset[d];
    for (unsigned s : loops) {
      if (T[d][s] > 0) {
        magnitude[s] = T[d][s];
        continue;
      }
      std::optional<int64_t> abs = llvm::checkedSub(int64_t(0), T[d][s]);
      std::optional<int64_t> shift =
          llvm::checkedMul(T[d][s], extentOf[s] - 1);
      std::optional<int64_t> shifted =
          shift ? llvm::checkedAdd(offset, *shift) : std::nullopt;
      if (!abs || !shifted)
        return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                             "'s scale or extent overflows 64-bit arithmetic");
      magnitude[s] = *abs;
      offset = *shifted;
    }
    llvm::stable_sort(loops, [&](unsigned a, unsigned b) {
      return magnitude[a] < magnitude[b];
    });
    int64_t expected = magnitude[loops.front()];
    int64_t extent = 1;
    for (unsigned s : loops) {
      if (magnitude[s] != expected)
        return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                             " is driven by " + llvm::Twine(loops.size()) +
                             " loops that do not enumerate it densely "
                             "(scales must grow by the finer loops' extents)");
      // The products are what the coordinate's values and the views built
      // from them are computed with; one that does not fit in 64 bits is
      // refused rather than wrapped around (a wrapped value could pass the
      // bounds checks downstream).
      std::optional<int64_t> nextExpected =
          llvm::checkedMul(expected, extentOf[s]);
      std::optional<int64_t> nextExtent = llvm::checkedMul(extent, extentOf[s]);
      if (!nextExpected || !nextExtent)
        return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                             "'s scale or extent overflows 64-bit arithmetic");
      expected = *nextExpected;
      extent = *nextExtent;
    }
    dense.sourceDim[d] = loops.front();
    dense.sourceDims[d] = loops;
    dense.scale[d] = magnitude[loops.front()];
    dense.offset[d] = offset;
    dense.extent[d] = extent;
    // The coordinate's last value, offset + scale * (extent - 1), must be
    // representable too: restrictToBox and the lowering's views compute it.
    std::optional<int64_t> span =
        llvm::checkedMul(dense.scale[d], dense.extent[d]);
    if (!span || !llvm::checkedAdd(dense.offset[d], *span))
      return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                           "'s values overflow 64-bit arithmetic");
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

/// The source iteration domain restricted to `box`. Logical coordinate
/// y_d = T_d x + c_d (LogicalMatmul::logicalMapping, in source coordinates
/// x) takes the values offset_d + scale_d * ordinal, so ordinals [lo, lo +
/// ext) are y_d in [offset_d + scale_d * lo, offset_d + scale_d * (lo +
/// ext)): one inequality per side, whether one loop drives the coordinate
/// or several. The interval is taken half-open on the next region's first
/// value rather than closed on this region's last one, so consecutive
/// regions cover y_d without gaps whether or not the source domain encodes
/// the loops' steps.
///
/// Every bound is formed in checked 64-bit arithmetic; failure (with the
/// reason in `why`) when one does not fit -- a wrapped-around bound would
/// describe a different region.
static FailureOr<affine::FlatAffineValueConstraints>
restrictToBox(const LogicalMatmul &matmul, const DenseLogicalGemm &dense,
              const LogicalBox &box, std::string *why) {
  affine::FlatAffineValueConstraints domain = *matmul.sourceDomain;
  const auto &T = matmul.logicalMapping.transformation;
  const auto &c = matmul.logicalMapping.offset;
  const unsigned numDims = domain.getNumDimVars();
  static constexpr const char *kName[] = {"I", "J", "K"};
  // y_d is (P*, I, J, K)'s last three rows: no P* in a dense GEMM.
  for (unsigned d = 0; d < 3; ++d) {
    std::optional<int64_t> nextOrdinal =
        llvm::checkedAdd(box.lower[d], box.extent[d]);
    std::optional<int64_t> firstStep =
        llvm::checkedMul(dense.scale[d], box.lower[d]);
    std::optional<int64_t> nextStep =
        nextOrdinal ? llvm::checkedMul(dense.scale[d], *nextOrdinal)
                    : std::nullopt;
    std::optional<int64_t> first =
        firstStep ? llvm::checkedAdd(dense.offset[d], *firstStep)
                  : std::nullopt;
    std::optional<int64_t> next =
        nextStep ? llvm::checkedAdd(dense.offset[d], *nextStep) : std::nullopt;
    // y_d - first >= 0 and next - 1 - y_d >= 0, with y_d = T_d x + c_d.
    std::optional<int64_t> lowerConstant =
        first ? llvm::checkedSub(c[d], *first) : std::nullopt;
    std::optional<int64_t> nextMinusOne =
        next ? llvm::checkedSub(*next, int64_t(1)) : std::nullopt;
    std::optional<int64_t> upperConstant =
        nextMinusOne ? llvm::checkedSub(*nextMinusOne, c[d]) : std::nullopt;
    if (!lowerConstant || !upperConstant)
      return fail(why, llvm::Twine("the bounds of logical coordinate ") +
                           kName[d] + " in a region overflow 64-bit "
                           "arithmetic");
    SmallVector<int64_t> lower(domain.getNumCols(), 0);
    SmallVector<int64_t> upper(domain.getNumCols(), 0);
    for (unsigned s = 0; s < numDims; ++s) {
      std::optional<int64_t> negated = llvm::checkedSub(int64_t(0), T[d][s]);
      if (!negated)
        return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                             "'s coefficient overflows 64-bit arithmetic");
      lower[s] = T[d][s];
      upper[s] = *negated;
    }
    lower.back() = *lowerConstant;
    upper.back() = *upperConstant;
    domain.addInequality(lower);
    domain.addInequality(upper);
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
  for (const LogicalBox &box : boxes) {
    FailureOr<affine::FlatAffineValueConstraints> region =
        restrictToBox(matmul, *dense, box, why);
    if (failed(region))
      return failure();
    partitions.push_back(LogicalPartition{std::move(*region), box});
  }

  if (failed(checkSourceCover(matmul, partitions, why)))
    return failure();

  return partitions;
}
