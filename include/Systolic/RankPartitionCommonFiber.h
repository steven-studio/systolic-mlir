#pragma once

#include "Systolic/RankPartitionIterationPlan.h"
#include "Systolic/TensorLogicalSpaces.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"

#include <cstdint>
#include <limits>
#include <optional>
#include <vector>

namespace systolic::logical {

// Phase 5.4.3d-3b-8 (docs/AFFINE_GEMM_MAPPING.md section 29):
//
// Reduction fibers of a conjunctive logical domain.
//
// For a domain D' over the logical coordinates (z_0, z_1, z_2) -- one
// polyhedron, no locals, no symbols -- with the roles I, J, K among the
// coordinates, the reduction fiber of an output coordinate (i, j) is
//
//   K(i, j) = { k : (i, j, k) in D' },
//
// an integer interval: for fixed (i, j) every constraint of D' is a linear
// bound on k. The common fiber of an output block G = [i0, i1] x [j0, j1]
// is the intersection of its cells' fibers,
//
//   K_common(G) = { k : for all (i, j) in G, (i, j, k) in D' },
//
// also an interval, decided exactly without projection: a facet
// a_I i + a_J j + a_K k + c >= 0 holds on the whole block iff it holds at
// the vertex of the block that minimizes a_I i + a_J j (chosen by the signs
// of a_I and a_J: an affine function on a box attains its minimum at a
// vertex, and the vertices are integer points of the block). That vertex
// gives one bound on k per facet (ceil / floor division for |a_K| > 1); a
// facet without k must hold at its vertex, else the block has a cell with
// an empty fiber and the common fiber is empty. An equality is its two
// facets. Everything is exact integer arithmetic (BigInt), no sampling, no
// approximation.

struct OutputBlock {
  std::int64_t rowLower = 0, rowUpper = -1;       // I values, inclusive
  std::int64_t columnLower = 0, columnUpper = -1; // J values, inclusive
};

struct ReductionInterval {
  bool empty = true;
  std::int64_t lower = 0, upper = -1; // inclusive when not empty
  std::int64_t length() const { return empty ? 0 : upper - lower + 1; }
};

struct FiberRoles {
  unsigned i = 0, j = 1, k = 2; // the logical coordinate of each role
};

namespace fiber_detail {

inline BigInt floorDiv(const BigInt &a, const BigInt &b) { // b > 0
  BigInt q = a / b;
  if (a % b != 0 && a < 0)
    q -= 1;
  return q;
}

inline BigInt ceilDiv(const BigInt &a, const BigInt &b) { // b > 0
  BigInt q = a / b;
  if (a % b != 0 && a > 0)
    q += 1;
  return q;
}

inline std::optional<std::int64_t> toI64(const llvm::DynamicAPInt &value) {
  const llvm::DynamicAPInt minimum(std::numeric_limits<std::int64_t>::min());
  const llvm::DynamicAPInt maximum(std::numeric_limits<std::int64_t>::max());
  if (value < minimum || value > maximum)
    return std::nullopt;
  return static_cast<std::int64_t>(value);
}

inline std::optional<std::int64_t> toI64(const BigInt &value) {
  if (value < BigInt(std::numeric_limits<std::int64_t>::min()) ||
      value > BigInt(std::numeric_limits<std::int64_t>::max()))
    return std::nullopt;
  return value.convert_to<std::int64_t>();
}

// One constraint a . z + c >= 0 as big integers.
struct Facet {
  BigInt coefficients[3];
  BigInt constant;
};

inline std::optional<std::vector<Facet>>
facetsOf(const mlir::presburger::IntegerRelation &domain) {
  if (domain.getNumVars() != 3 || domain.getNumLocalVars() != 0 ||
      domain.getNumSymbolVars() != 0)
    return std::nullopt;
  std::vector<Facet> facets;
  auto push = [&](llvm::ArrayRef<llvm::DynamicAPInt> row, bool negate) {
    Facet facet;
    for (unsigned v = 0; v < 3; ++v) {
      auto value = toI64(row[v]);
      if (!value)
        return false;
      facet.coefficients[v] = negate ? -BigInt(*value) : BigInt(*value);
    }
    auto constant = toI64(row[3]);
    if (!constant)
      return false;
    facet.constant = negate ? -BigInt(*constant) : BigInt(*constant);
    facets.push_back(std::move(facet));
    return true;
  };
  for (unsigned r = 0; r < domain.getNumInequalities(); ++r)
    if (!push(domain.getInequality(r), false))
      return std::nullopt;
  for (unsigned r = 0; r < domain.getNumEqualities(); ++r)
    if (!push(domain.getEquality(r), false) ||
        !push(domain.getEquality(r), true))
      return std::nullopt;
  return facets;
}

// The k interval on which every facet holds at the given (i, j) values
// (iLower == iUpper and jLower == jUpper for one cell; the block's bounds
// for the common fiber: each facet is then evaluated at its minimizing
// vertex). Clamped to [kLower, kUpper].
inline std::optional<ReductionInterval>
fiberOnBox(const std::vector<Facet> &facets, FiberRoles roles,
           std::int64_t iLower, std::int64_t iUpper, std::int64_t jLower,
           std::int64_t jUpper, std::int64_t kLower, std::int64_t kUpper) {
  if (iLower > iUpper || jLower > jUpper || kLower > kUpper ||
      roles.i > 2 || roles.j > 2 || roles.k > 2 || roles.i == roles.j ||
      roles.i == roles.k || roles.j == roles.k)
    return std::nullopt;
  BigInt lower(kLower), upper(kUpper);
  for (const Facet &facet : facets) {
    const BigInt &aI = facet.coefficients[roles.i];
    const BigInt &aJ = facet.coefficients[roles.j];
    const BigInt &aK = facet.coefficients[roles.k];
    const BigInt iStar(aI >= 0 ? iLower : iUpper);
    const BigInt jStar(aJ >= 0 ? jLower : jUpper);
    const BigInt base = aI * iStar + aJ * jStar + facet.constant;
    if (aK == 0) {
      if (base < 0)
        return ReductionInterval{}; // a cell violates the facet: empty
      continue;
    }
    if (aK > 0) {
      // aK k >= -base
      const BigInt bound = ceilDiv(-base, aK);
      if (bound > lower)
        lower = bound;
    } else {
      // (-aK) k <= base
      const BigInt bound = floorDiv(base, -aK);
      if (bound < upper)
        upper = bound;
    }
  }
  if (lower > upper)
    return ReductionInterval{};
  auto lo = toI64(lower), hi = toI64(upper);
  if (!lo || !hi)
    return std::nullopt;
  ReductionInterval interval;
  interval.empty = false;
  interval.lower = *lo;
  interval.upper = *hi;
  return interval;
}

} // namespace fiber_detail

// The common reduction fiber of `block` within the K range [kLower,
// kUpper]. nullopt: the domain is not a conjunctive three-dimensional
// polyhedron without locals or a value does not fit i64 (callers refuse).
inline std::optional<ReductionInterval>
commonReductionFiber(const mlir::presburger::IntegerRelation &domain,
                     FiberRoles roles, const OutputBlock &block,
                     std::int64_t kLower, std::int64_t kUpper) {
  auto facets = fiber_detail::facetsOf(domain);
  if (!facets)
    return std::nullopt;
  return fiber_detail::fiberOnBox(*facets, roles, block.rowLower,
                                  block.rowUpper, block.columnLower,
                                  block.columnUpper, kLower, kUpper);
}

// The fiber of one output cell.
inline std::optional<ReductionInterval>
cellReductionFiber(const mlir::presburger::IntegerRelation &domain,
                   FiberRoles roles, std::int64_t i, std::int64_t j,
                   std::int64_t kLower, std::int64_t kUpper) {
  auto facets = fiber_detail::facetsOf(domain);
  if (!facets)
    return std::nullopt;
  return fiber_detail::fiberOnBox(*facets, roles, i, i, j, j, kLower,
                                  kUpper);
}

// Exact point counts: total = the sum of every cell's fiber length over
// the output box; device = the sum over the blocks of |block| x |common
// fiber|; remainder = total - device. The blocks must lie inside the output
// box and be pairwise disjoint (the caller's partition); nullopt otherwise
// or when the domain is not representable.
struct FiberCounts {
  BigInt total = 0;
  BigInt device = 0;
  BigInt remainder = 0;
};

inline std::optional<FiberCounts>
countReductionFibers(const mlir::presburger::IntegerRelation &domain,
                     FiberRoles roles, const std::vector<OutputBlock> &blocks,
                     std::int64_t iLower, std::int64_t iUpper,
                     std::int64_t jLower, std::int64_t jUpper,
                     std::int64_t kLower, std::int64_t kUpper) {
  auto facets = fiber_detail::facetsOf(domain);
  if (!facets || iLower > iUpper || jLower > jUpper)
    return std::nullopt;
  FiberCounts counts;
  for (std::int64_t i = iLower; i <= iUpper; ++i)
    for (std::int64_t j = jLower; j <= jUpper; ++j) {
      auto fiber = fiber_detail::fiberOnBox(*facets, roles, i, i, j, j, kLower,
                                            kUpper);
      if (!fiber)
        return std::nullopt;
      counts.total += BigInt(fiber->length());
    }
  for (std::size_t b = 0; b < blocks.size(); ++b) {
    const OutputBlock &block = blocks[b];
    if (block.rowLower < iLower || block.rowUpper > iUpper ||
        block.columnLower < jLower || block.columnUpper > jUpper ||
        block.rowLower > block.rowUpper ||
        block.columnLower > block.columnUpper)
      return std::nullopt;
    for (std::size_t o = 0; o < b; ++o) {
      const OutputBlock &other = blocks[o];
      const bool rows = block.rowLower <= other.rowUpper &&
                        other.rowLower <= block.rowUpper;
      const bool columns = block.columnLower <= other.columnUpper &&
                           other.columnLower <= block.columnUpper;
      if (rows && columns)
        return std::nullopt;
    }
    auto fiber = fiber_detail::fiberOnBox(
        *facets, roles, block.rowLower, block.rowUpper, block.columnLower,
        block.columnUpper, kLower, kUpper);
    if (!fiber)
      return std::nullopt;
    counts.device += BigInt(block.rowUpper - block.rowLower + 1) *
                     BigInt(block.columnUpper - block.columnLower + 1) *
                     BigInt(fiber->length());
  }
  counts.remainder = counts.total - counts.device;
  return counts;
}

// Phase 5.4.3d-3b-8 M2b-1 (docs/AFFINE_GEMM_MAPPING.md section 29.8):
//
// The host remainder of an output block, in pieces with exact bounds.
//
// For a block G with the common fiber F = [f0, f1] within the K range
// [k0, k1] the remainder R(G) = {(i, j, k) in D' : (i, j) in G, k not in F}
// is the disjoint union of the lower piece P- = D' ∩ (G × [k0, f0 - 1]) and
// the upper piece P+ = D' ∩ (G × [f1 + 1, k1]); without a common fiber the
// single piece is D' ∩ (G × [k0, k1]). Each piece is one conjunctive
// polyhedron (D' with six bounds added). A non-empty piece is iterated by one
// guarded nest over its exact bounding box: per coordinate the least and the
// greatest value an integer point of the piece attains. The least value is
// the least t for which the piece has an integer point with z_v <= t: that
// predicate is monotone in t, false below the least value and true from it
// on, so bisection over the piece's known range decides it with
// O(log range) conjunctive integer emptiness checks -- no rational
// relaxation (whose bound can be looser than the integer one), no sampling.
// The box only bounds the sweep: the nest's guard is the piece itself, every
// point of the piece lies in its box by definition, and the iteration plan
// re-certifies the inclusion (buildSymbolicIterationPlan). A piece without
// an establishable bound is refused (nullopt: the caller answers UNKNOWN).

struct RemainderPiece {
  mlir::presburger::IntegerPolyhedron set; // non-empty, no locals
  std::vector<ClosedIntegerInterval> box; // exact bounds, coordinate order
};

namespace fiber_detail {

// The least value of coordinate `v` over the integer points of `piece`, a
// non-empty polyhedron on which low <= z_v <= high holds. Invariant of the
// bisection: piece ∩ {z_v <= hi} is non-empty (at first: it is the piece)
// and piece ∩ {z_v <= lo - 1} is empty (at first: z_v >= low on the piece);
// it ends with lo == hi, the least value.
inline std::optional<std::int64_t>
leastCoordinate(const mlir::presburger::IntegerPolyhedron &piece, unsigned v,
                std::int64_t low, std::int64_t high) {
  using namespace mlir::presburger;
  if (low > high || v >= piece.getNumVars())
    return std::nullopt;
  BigInt lo(low), hi(high);
  while (lo < hi) {
    const BigInt mid = floorDiv(lo + hi, BigInt(2)); // lo <= mid < hi
    auto m = toI64(mid);
    if (!m)
      return std::nullopt;
    IntegerPolyhedron cut(piece);
    cut.addBound(BoundType::UB, v, llvm::DynamicAPInt(*m));
    if (cut.isIntegerEmpty())
      lo = mid + 1;
    else
      hi = mid;
  }
  return toI64(lo);
}

// The greatest value, symmetrically: piece ∩ {z_v >= lo} is non-empty and
// piece ∩ {z_v >= hi + 1} is empty throughout.
inline std::optional<std::int64_t>
greatestCoordinate(const mlir::presburger::IntegerPolyhedron &piece,
                   unsigned v, std::int64_t low, std::int64_t high) {
  using namespace mlir::presburger;
  if (low > high || v >= piece.getNumVars())
    return std::nullopt;
  BigInt lo(low), hi(high);
  while (lo < hi) {
    const BigInt mid = ceilDiv(lo + hi, BigInt(2)); // lo < mid <= hi
    auto m = toI64(mid);
    if (!m)
      return std::nullopt;
    IntegerPolyhedron cut(piece);
    cut.addBound(BoundType::LB, v, llvm::DynamicAPInt(*m));
    if (cut.isIntegerEmpty())
      hi = mid - 1;
    else
      lo = mid;
  }
  return toI64(lo);
}

} // namespace fiber_detail

// The exact bounding box of the integer points of `piece`: a non-empty
// polyhedron without locals or symbols whose constraints include the bounds
// `within` (one interval per coordinate). nullopt: the preconditions fail
// or a bound does not fit i64. On an empty piece the result is meaningless;
// callers decide emptiness first.
inline std::optional<std::vector<ClosedIntegerInterval>>
exactBoundingBox(const mlir::presburger::IntegerPolyhedron &piece,
                 const std::vector<ClosedIntegerInterval> &within) {
  if (piece.getNumVars() != within.size() || piece.getNumLocalVars() != 0 ||
      piece.getNumSymbolVars() != 0 || within.empty())
    return std::nullopt;
  std::vector<ClosedIntegerInterval> box(within.size());
  for (unsigned v = 0; v < within.size(); ++v) {
    if (within[v].lower > within[v].upper)
      return std::nullopt;
    auto least = fiber_detail::leastCoordinate(piece, v, within[v].lower,
                                               within[v].upper);
    auto greatest = fiber_detail::greatestCoordinate(
        piece, v, within[v].lower, within[v].upper);
    if (!least || !greatest || *least > *greatest)
      return std::nullopt;
    box[v] = {*least, *greatest};
  }
  return box;
}

// The remainder pieces of `block` whose common fiber is `fiber` (empty: the
// whole block is the host's) within the K range [kLower, kUpper]: at most
// two, each non-empty, each with its exact bounding box; an empty vector
// when the block has no remainder. nullopt: the domain is not a conjunctive
// three-dimensional polyhedron without locals or symbols, the roles or the
// block are malformed, or a bound does not fit i64 (callers refuse).
inline std::optional<std::vector<RemainderPiece>>
remainderPieces(const mlir::presburger::IntegerRelation &domain,
                FiberRoles roles, const OutputBlock &block,
                const ReductionInterval &fiber, std::int64_t kLower,
                std::int64_t kUpper) {
  using namespace mlir::presburger;
  if (domain.getNumVars() != 3 || domain.getNumLocalVars() != 0 ||
      domain.getNumSymbolVars() != 0 || domain.getNumDomainVars() != 0 ||
      roles.i > 2 || roles.j > 2 || roles.k > 2 || roles.i == roles.j ||
      roles.i == roles.k || roles.j == roles.k ||
      block.rowLower > block.rowUpper ||
      block.columnLower > block.columnUpper || kLower > kUpper)
    return std::nullopt;
  if (!fiber.empty && (fiber.lower > fiber.upper || fiber.lower < kLower ||
                       fiber.upper > kUpper))
    return std::nullopt;
  std::vector<RemainderPiece> pieces;
  auto piece = [&](std::int64_t kFrom, std::int64_t kTo) -> bool {
    IntegerPolyhedron set(domain);
    set.addBound(BoundType::LB, roles.i, llvm::DynamicAPInt(block.rowLower));
    set.addBound(BoundType::UB, roles.i, llvm::DynamicAPInt(block.rowUpper));
    set.addBound(BoundType::LB, roles.j,
                 llvm::DynamicAPInt(block.columnLower));
    set.addBound(BoundType::UB, roles.j,
                 llvm::DynamicAPInt(block.columnUpper));
    set.addBound(BoundType::LB, roles.k, llvm::DynamicAPInt(kFrom));
    set.addBound(BoundType::UB, roles.k, llvm::DynamicAPInt(kTo));
    if (set.isIntegerEmpty())
      return true; // nothing to iterate
    std::vector<ClosedIntegerInterval> within(3);
    within[roles.i] = {block.rowLower, block.rowUpper};
    within[roles.j] = {block.columnLower, block.columnUpper};
    within[roles.k] = {kFrom, kTo};
    auto box = exactBoundingBox(set, within);
    if (!box)
      return false;
    pieces.push_back({std::move(set), std::move(*box)});
    return true;
  };
  if (fiber.empty) {
    if (!piece(kLower, kUpper))
      return std::nullopt;
  } else {
    // fiber.lower > kLower keeps fiber.lower - 1 in range; likewise above.
    if (fiber.lower > kLower && !piece(kLower, fiber.lower - 1))
      return std::nullopt;
    if (fiber.upper < kUpper && !piece(fiber.upper + 1, kUpper))
      return std::nullopt;
  }
  return pieces;
}

} // namespace systolic::logical
