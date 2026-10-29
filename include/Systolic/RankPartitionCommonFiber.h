#pragma once

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

} // namespace systolic::logical
