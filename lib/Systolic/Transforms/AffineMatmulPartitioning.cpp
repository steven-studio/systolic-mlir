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
                                    std::string *why,
                                    GemmMappingClass *mappingClass) {
  // Every refusal is classified (docs/AFFINE_GEMM_MAPPING.md section 9).
  auto failAs = [&](GemmMappingClass kind, const llvm::Twine &message) {
    if (mappingClass)
      *mappingClass = kind;
    return fail(why, message);
  };
  using Class = GemmMappingClass;
  const AffineLogicalNormalization &norm = matmul.normalization;
  if (!norm.domain) {
    // Overflow is reported only when normalizeLogicalCoordinates saw a
    // checked operation fail; every other nest without a normalized form
    // keeps the wording below.
    if (matmul.normalizationOverflow)
      return failAs(Class::Unknown, "the iteration domain's normalized form overflows "
                       "64-bit arithmetic");
    // An empty GEMM (M, N or K = 0): whether it should be offloaded with
    // zero invocations is open (SQ-2); refused, with this reason.
    if (matmul.normalizationEmptyLoop)
      return failAs(Class::ProvenValid, "a loop of the nest has no iteration (an empty GEMM: "
                       "M, N or K is 0)");
    // A bound on another loop's counter: the domain is not a box, which no
    // bijection onto the GEMM's box can come from (Theorem 3). A symbol: the
    // extent is unknown here (P1.4).
    return failAs(matmul.normalizationDetail.find("not a box") !=
                          std::string::npos
                      ? Class::ProvenInvalid
                      : Class::Unknown,
                  "the loop bounds are not static, so the iteration "
                  "domain has no static normalized extent");
  }

  // y = (P*, I, J, K): the rows of T are the batch coordinates P* (numP of
  // them, 0 for a plain GEMM), then I, J, K.
  const auto &T = norm.logicalTransformation;
  if (T.size() < 3 || norm.logicalOffset.size() != T.size())
    return failAs(Class::Unknown, "the logical mapping has fewer than three "
                                  "coordinates");
  const unsigned numP = T.size() - 3;

  const unsigned numSourceDims = norm.domain->getNumDimVars();
  // Any number of loops: a role of extent one needs none (a matrix-vector
  // product has two, docs/AFFINE_GEMM_MAPPING.md section 2).
  if (numSourceDims < 1)
    return failAs(Class::Unknown, "the iteration domain has no dimension");

  // Each loop of the nest that runs two or more times drives exactly one
  // logical coordinate (the recognizer has already required every loop's IV
  // to be used; this is necessary as well as sufficient for the logical
  // coordinates to be a bijection onto a box: docs/AFFINE_GEMM_MAPPING.md,
  // Theorem 3). A
  // coordinate may be driven by any number of loops -- a tiled or split loop
  // nest, at any depth -- as long as they enumerate it densely: sorted by
  // scale, the finest has scale s_0 and each next one the scale of the
  // previous times its extent (a mixed-radix numbering), so the coordinate
  // takes the values offset + s_0 * (0 .. product of extents - 1), each once.
  static constexpr const char *kName[] = {"I", "J", "K"};
  SmallVector<int64_t> extentOf(numSourceDims, 0);
  for (unsigned s = 0; s < numSourceDims; ++s) {
    std::optional<int64_t> lb = norm.domain->getConstantBound64(
        presburger::BoundType::LB, s);
    std::optional<int64_t> ub = norm.domain->getConstantBound64(
        presburger::BoundType::UB, s);
    if (!lb || !ub || *lb != 0 || *ub < 0)
      return failAs(Class::Unknown, "the normalized iteration domain is not a dense box");
    std::optional<int64_t> trips = llvm::checkedAdd(*ub, int64_t(1));
    if (!trips)
      return failAs(Class::Unknown, "loop " + llvm::Twine(s) +
                           "'s trip count overflows 64-bit arithmetic");
    extentOf[s] = *trips;
  }

  // A loop that runs exactly once has ordinal 0 at every point: its
  // coefficients do not change any coordinate's value, so it imposes nothing
  // (docs/AFFINE_GEMM_MAPPING.md, Theorem 3, degenerate loops) and drives no
  // coordinate. A coordinate driven by such loops only has one value -- M, N
  // or K = 1, like a constant subscript (SQ-4, decided 2026-10-15) -- whatever
  // their coefficients (a step of 2 on a single-iteration loop is no radix).
  // `d` indexes I, J, K; their rows of T follow the numP batch rows.
  auto drivesCoordinate = [&](unsigned d, unsigned s) {
    return T[numP + d][s] != 0 && extentOf[s] != 1;
  };
  // A batch loop: a nonzero coefficient in some P row and two or more
  // iterations (a loop that runs once is a constant on the domain: the
  // lowering folds its term into the batch index, as for I, J, K).
  auto isBatchLoop = [&](unsigned s) {
    if (extentOf[s] == 1)
      return false;
    for (unsigned p = 0; p < numP; ++p)
      if (T[p][s] != 0)
        return true;
    return false;
  };

  for (unsigned s = 0; s < numSourceDims; ++s) {
    unsigned drives = 0;
    for (unsigned d = 0; d < 3; ++d)
      drives += drivesCoordinate(d, s);
    // A loop of two or more iterations that drives a batch coordinate and
    // one of I, J, K couples the batches (C[b][b + i][j], affine_gemm_
    // partition_coupled.mlir): no box of (I, J, K) per batch value.
    if (drives > 1 || (drives == 1 && isBatchLoop(s))) {
      std::string coords;
      if (isBatchLoop(s))
        coords = "a batch coordinate";
      for (unsigned d = 0; d < 3; ++d)
        if (drivesCoordinate(d, s))
          coords += (coords.empty() ? "" : " and ") + std::string(kName[d]);
      return failAs(Class::ProvenInvalid, "two logical coordinates are driven by the same loop "
                       "(loop " + llvm::Twine(s) + " drives " + coords + ")");
    }
    if (drives == 0 && extentOf[s] != 1 && !isBatchLoop(s))
      return failAs(Class::ProvenInvalid, "loop " + llvm::Twine(s) +
                           " drives no logical coordinate");
  }

  DenseLogicalGemm dense;
  // The batch coordinates: their loops, their value range on the domain
  // (offset + sum over the loops of coefficient * ordinal, the ordinal in
  // [0, n_s): each term's extreme at 0 or n_s - 1), and how often the tiles
  // run. All of it checked.
  dense.numBatchDims = numP;
  for (unsigned s = 0; s < numSourceDims; ++s)
    if (isBatchLoop(s)) {
      dense.batchLoops.push_back(s);
      std::optional<int64_t> trips =
          llvm::checkedMul(dense.batchTrips, extentOf[s]);
      if (!trips)
        return failAs(Class::Unknown, "the batch loops' trip count overflows "
                                      "64-bit arithmetic");
      dense.batchTrips = *trips;
    }
  for (unsigned p = 0; p < numP; ++p) {
    std::optional<int64_t> lo = norm.logicalOffset[p], hi = lo;
    for (unsigned s = 0; s < numSourceDims && lo && hi; ++s) {
      if (T[p][s] == 0)
        continue;
      std::optional<int64_t> span = llvm::checkedMul(T[p][s], extentOf[s] - 1);
      if (!span) {
        lo = std::nullopt;
        break;
      }
      lo = llvm::checkedAdd(*lo, std::min<int64_t>(*span, 0));
      hi = hi ? llvm::checkedAdd(*hi, std::max<int64_t>(*span, 0)) : hi;
    }
    if (!lo || !hi)
      return failAs(Class::Unknown, "batch coordinate " + llvm::Twine(p) +
                                        "'s values overflow 64-bit arithmetic");
    dense.batchRange.push_back({*lo, *hi});
  }

  for (unsigned d = 0; d < 3; ++d) {
    SmallVector<unsigned, 2> loops;
    for (unsigned s = 0; s < numSourceDims; ++s)
      if (drivesCoordinate(d, s))
        loops.push_back(s);
    // A coordinate no loop of two or more iterations drives has one value:
    // M, N or K = 1 (C[0][j] += A[0][k] * B[k][j], a matrix-vector product's
    // missing column, or a coordinate of single-iteration loops). Its
    // operands read it through constant subscripts, or have no dimension for
    // it (SQ-4, decided 2026-10-15: offloaded). Its value on the domain is the
    // normalized offset (every ordinal is 0); each operand's view of it is a
    // size-1 dimension at that index.
    if (loops.empty()) {
      // Listed for the diagnostics: the single-iteration loops with a
      // nonzero coefficient, if any (they drive nothing).
      dense.sourceDims[d].clear();
      for (unsigned s = 0; s < numSourceDims; ++s)
        if (T[numP + d][s] != 0)
          dense.sourceDims[d].push_back(s);
      dense.sourceDim[d] = dense.sourceDims[d].empty() ? 0 : dense.sourceDims[d].front();
      dense.scale[d] = 1;
      dense.offset[d] = norm.logicalOffset[numP + d];
      dense.extent[d] = 1;
      continue;
    }
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
    int64_t offset = norm.logicalOffset[numP + d];
    for (unsigned s : loops) {
      if (T[numP + d][s] > 0) {
        magnitude[s] = T[numP + d][s];
        continue;
      }
      std::optional<int64_t> abs = llvm::checkedSub(int64_t(0), T[numP + d][s]);
      std::optional<int64_t> shift =
          llvm::checkedMul(T[numP + d][s], extentOf[s] - 1);
      std::optional<int64_t> shifted =
          shift ? llvm::checkedAdd(offset, *shift) : std::nullopt;
      if (!abs || !shifted)
        return failAs(Class::Unknown, llvm::Twine("logical coordinate ") + kName[d] +
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
        return failAs(Class::ProvenInvalid, llvm::Twine("logical coordinate ") + kName[d] +
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
        return failAs(Class::Unknown, llvm::Twine("logical coordinate ") + kName[d] +
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
      return failAs(Class::Unknown, llvm::Twine("logical coordinate ") + kName[d] +
                           "'s values overflow 64-bit arithmetic");
  }
  // A batched nest whose per-batch GEMM has two of M, N, K equal to one is a
  // vector scaled by a scalar per batch (C[i][k] += A[i][k] * B[k][j] with j
  // of one iteration: the reduction index in the output, which the
  // specification lists as not a GEMM). Correct to lower, but whether such a
  // nest is a GEMM to offload is an open specification question (SQ-5):
  // refused conservatively, as a valid mapping that is not lowered.
  if (numP > 0 && (dense.extent[0] == 1) + (dense.extent[1] == 1) +
                          (dense.extent[2] == 1) >= 2)
    return failAs(Class::ValidButLoweringUnsupported,
                  "a batched GEMM whose tiles are a vector scaled by a scalar "
                  "(two of M, N, K are 1) is not lowered pending spec question "
                  "SQ-5 (the reduction index in the output)");
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
  // y_d is (P*, I, J, K)'s last three rows; the batch coordinates P* are not
  // constrained: a region runs for every batch value (the batch loops are
  // re-emitted around the tiles).
  const unsigned numP = dense.numBatchDims;
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
        first ? llvm::checkedSub(c[numP + d], *first) : std::nullopt;
    std::optional<int64_t> nextMinusOne =
        next ? llvm::checkedSub(*next, int64_t(1)) : std::nullopt;
    std::optional<int64_t> upperConstant =
        nextMinusOne ? llvm::checkedSub(*nextMinusOne, c[numP + d]) : std::nullopt;
    if (!lowerConstant || !upperConstant)
      return fail(why, llvm::Twine("the bounds of logical coordinate ") +
                           kName[d] + " in a region overflow 64-bit "
                           "arithmetic");
    SmallVector<int64_t> lower(domain.getNumCols(), 0);
    SmallVector<int64_t> upper(domain.getNumCols(), 0);
    for (unsigned s = 0; s < numDims; ++s) {
      std::optional<int64_t> negated = llvm::checkedSub(int64_t(0), T[numP + d][s]);
      if (!negated)
        return fail(why, llvm::Twine("logical coordinate ") + kName[d] +
                             "'s coefficient overflows 64-bit arithmetic");
      lower[s] = T[numP + d][s];
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
