//===- RankPartitionKernelCosetRanges.h - emitted value range proof ------===//
//
// Phase F-KC-5b-0: exact, fail-closed proof that every value the kernel-coset
// emission computes at run time fits in 64 bits.
//
// Fourier-Motzkin elimination multiplies coefficients: P*k <= i and j <= Q*k
// give P*j <= Q*i, so the j loop gets the face floor(Q*i / P). The projection is
// exact, every coefficient fits in 64 bits and the affine map is correct, but
// the value Q*i exists only at run time, where the map lowers to `mul nsw i64`.
// A source that computes nothing above 41 bits, with small in-bounds memrefs,
// could reach a 65-bit product: poison, in practice a wrapped bound, and a loop
// that silently loses its MACs. A check on coefficients cannot see this. It
// needs the range of the values the coefficients are multiplied by.
//
// This header answers that for one emitted nest, independent of how the nest
// was derived. Let v_1, ..., v_m be its loop variables, outermost first, with
// level l bounded by faces v_l >= ceil(f / d), v_l <= floor(g / e), where f and g
// are linear in the enclosing variables. Let R_l be the rational polytope cut
// out by the faces of levels 1 to l, each read as d * v_l >= f, e * v_l <= g.
//
//   M_l = max over R_l of |v_l|, computed exactly by linear programming.
//
// The check is, for every emitted linear form c_0 + sum c_j * v_j:
//
//   |c_0| + sum |c_j| * M_j  <=  kKernelCosetValueLimit.                    (*)
//
// Soundness, by induction on the nest depth. If no value at a level below l
// overflowed, the run-time variables are integers satisfying those faces, so they
// lie in R_l and |v_j| <= M_j. Then (*) bounds every product and every partial
// sum of a form at level l, in any evaluation order, because each is a sub-sum
// of the left-hand side. So the form is computed exactly, and with it the
// quotient, the loop bound and the next variable. R_l comes from the emitted
// faces alone, so a level scanned over a box, or over a rational shadow, or a
// prefix whose fiber turns out empty, is covered without further argument. An
// existence guard only removes iterations, so ignoring it here is conservative.
//
// The limit leaves headroom for what the lowering derives from a form of
// magnitude at most L = 2^61: ceil and floor (L + 1), a half-open bound (L + 2),
// a loop step (L + K), the difference of two quotients in a guard or a remainder
// count (2 * L + 3). All of them stay well inside int64. The price is that a
// nest with an emitted value between 2^61 and 2^63 is refused although some such
// nests would still be correct. No exact-range proof of those is attempted.
//
// Linear programming over the exact rationals, in arbitrary precision: nothing
// here can itself overflow. Coefficients are required to fit in 64 bits, as the
// emission does; anything the proof cannot establish is Unknown, never
// Representable. No enumeration, no cardinality, no approximation of a bound:
// the emitted faces are only measured, never changed.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "Systolic/RankPartitionKernelCosetImage.h"

#include "mlir/Analysis/Presburger/Fraction.h"
#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/Simplex.h"
#include "llvm/ADT/DynamicAPInt.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

/// The largest magnitude any emitted linear form may reach, 2^61.
inline constexpr std::int64_t kKernelCosetValueLimit = std::int64_t(1) << 61;

/// One emitted loop: reduced column `col`, its faces over `paramCols`.
struct KernelCosetEmittedLevel {
  unsigned col = 0;
  std::vector<unsigned> paramCols;
  std::vector<TileBoundFace> lowerCeil;  // col >= ceil((params.p + c) / d)
  std::vector<TileBoundFace> upperFloor; // col <= floor((params.p + c) / d)
};

/// What one emission computes at run time.
struct KernelCosetEmittedProgram {
  unsigned numCols = 0;
  /// Nest order, outermost first, the tile loop last.
  std::vector<KernelCosetEmittedLevel> levels;
  /// The existence guard, when the nest has one.
  const KernelCosetExistenceGuard *guard = nullptr;
  /// The block size. Above 1 the tile loop steps by it and tests a full block.
  std::int64_t blockK = 1;
  /// Physical addresses: coefficients over all columns, and the constant.
  std::vector<std::pair<IntRow, BigInt>> addresses;
};

enum class EmittedRangeVerdict { Representable, Exceeded, Unknown };

struct EmittedRangeResult {
  EmittedRangeVerdict verdict = EmittedRangeVerdict::Unknown;
  std::string why;
};

namespace kernel_coset_ranges_detail {

using llvm::DynamicAPInt;

inline std::string str(const DynamicAPInt &v) {
  std::string s;
  llvm::raw_string_ostream os(s);
  os << v;
  os.flush();
  return s;
}

/// |c_0| + sum |c_j| * M_j, or nullopt if a coefficient exceeds 64 bits or a
/// column has no bound yet.
inline std::optional<DynamicAPInt>
magnitudeOf(llvm::ArrayRef<unsigned> cols, llvm::ArrayRef<BigInt> coeffs,
            const BigInt &constant,
            llvm::ArrayRef<std::optional<DynamicAPInt>> bound) {
  if (cols.size() != coeffs.size())
    return std::nullopt;
  auto c = kernel_coset_detail::toAPInt(constant);
  if (!c)
    return std::nullopt;
  DynamicAPInt total = llvm::abs(*c);
  for (std::size_t j = 0; j < cols.size(); ++j) {
    auto a = kernel_coset_detail::toAPInt(coeffs[j]);
    if (!a)
      return std::nullopt;
    if (*a == DynamicAPInt(0))
      continue;
    if (cols[j] >= bound.size() || !bound[cols[j]])
      return std::nullopt;
    total += llvm::abs(*a) * *bound[cols[j]];
  }
  return total;
}

} // namespace kernel_coset_ranges_detail

/// Decide (*) of the file comment for every form of `program`.
inline EmittedRangeResult
checkKernelCosetEmittedRanges(const KernelCosetEmittedProgram &program) {
  using namespace kernel_coset_ranges_detail;
  using mlir::presburger::IntegerPolyhedron;
  using mlir::presburger::PresburgerSpace;
  using mlir::presburger::Simplex;
  const unsigned n = program.numCols;
  const DynamicAPInt limit(kKernelCosetValueLimit);
  const DynamicAPInt zero(0), one(1);

  auto unknown = [](std::string why) {
    return EmittedRangeResult{EmittedRangeVerdict::Unknown, std::move(why)};
  };
  auto exceeded = [&](const std::string &what, const DynamicAPInt &value) {
    return EmittedRangeResult{EmittedRangeVerdict::Exceeded,
                              what + " can reach magnitude " + str(value) +
                                  ", beyond the limit " + str(limit)};
  };

  std::vector<std::optional<DynamicAPInt>> bound(n);
  IntegerPolyhedron reach(PresburgerSpace::getSetSpace(n, 0, 0));

  // d * col >= params.p + c as a row of `reach`, or <= when `upper`.
  auto addFace = [&](const KernelCosetEmittedLevel &level,
                     const TileBoundFace &f, bool upper) -> bool {
    auto d = kernel_coset_detail::toAPInt(f.divisor);
    auto c = kernel_coset_detail::toAPInt(f.constant);
    if (!d || !c || *d <= zero || f.params.size() != level.paramCols.size())
      return false;
    llvm::SmallVector<DynamicAPInt, 8> row(n + 1, zero);
    row[level.col] = *d;
    for (std::size_t j = 0; j < f.params.size(); ++j) {
      auto a = kernel_coset_detail::toAPInt(f.params[j]);
      if (!a || level.paramCols[j] >= n)
        return false;
      row[level.paramCols[j]] -= *a;
    }
    row[n] = -*c;
    if (upper)
      for (DynamicAPInt &v : row)
        v = -v;
    reach.addInequality(row);
    return true;
  };

  for (const KernelCosetEmittedLevel &level : program.levels) {
    if (level.col >= n || bound[level.col])
      return unknown("malformed nest: column " + std::to_string(level.col));
    const std::string name = "a bound of reduced column " +
                             std::to_string(level.col);
    // The faces are evaluated at the enclosing variables.
    for (const auto *faces : {&level.lowerCeil, &level.upperFloor})
      for (const TileBoundFace &f : *faces) {
        auto m = magnitudeOf(level.paramCols, f.params, f.constant, bound);
        if (!m)
          return unknown(name + " is not measurable");
        if (*m > limit)
          return exceeded(name, *m);
      }
    if (level.lowerCeil.empty() || level.upperFloor.empty())
      return unknown(name + " is missing a side");
    for (const TileBoundFace &f : level.lowerCeil)
      if (!addFace(level, f, /*upper=*/false))
        return unknown(name + " does not fit the proof");
    for (const TileBoundFace &f : level.upperFloor)
      if (!addFace(level, f, /*upper=*/true))
        return unknown(name + " does not fit the proof");

    // M_l: the exact rational extremes of this variable over the nest so far.
    Simplex simplex(reach);
    if (simplex.isEmpty())
      return unknown("no iteration reaches reduced column " +
                     std::to_string(level.col));
    llvm::SmallVector<DynamicAPInt, 8> objective(n + 1, zero);
    objective[level.col] = one;
    auto up = simplex.computeOptimum(Simplex::Direction::Up, objective);
    auto down = simplex.computeOptimum(Simplex::Direction::Down, objective);
    if (!up.isBounded() || !down.isBounded())
      return unknown("reduced column " + std::to_string(level.col) +
                     " is unbounded");
    DynamicAPInt m = std::max(llvm::abs(mlir::presburger::ceil(*up)),
                              llvm::abs(mlir::presburger::floor(*down)));
    if (m > limit)
      return exceeded("reduced column " + std::to_string(level.col), m);
    bound[level.col] = std::move(m);
  }
  if (program.levels.empty())
    return unknown("empty nest");

  if (program.guard)
    for (const auto *faces :
         {&program.guard->lowerCeil, &program.guard->upperFloor})
      for (const TileBoundFace &f : *faces) {
        auto m = magnitudeOf(program.guard->paramCols, f.params, f.constant,
                             bound);
        if (!m)
          return unknown("the existence guard is not measurable");
        if (*m > limit)
          return exceeded("the existence guard", *m);
      }

  // The full-block predicate: num - d * t - d * (K - 1) >= 0, on the tile.
  if (program.blockK < 1)
    return unknown("block size below one");
  if (program.blockK > 1) {
    const KernelCosetEmittedLevel &tile = program.levels.back();
    const DynamicAPInt span(program.blockK - 1);
    for (const TileBoundFace &f : tile.upperFloor) {
      auto m = magnitudeOf(tile.paramCols, f.params, f.constant, bound);
      auto d = kernel_coset_detail::toAPInt(f.divisor);
      if (!m || !d || !bound[tile.col])
        return unknown("the full-block test is not measurable");
      DynamicAPInt total = *m + *d * *bound[tile.col] + *d * span;
      if (total > limit)
        return exceeded("the full-block test", total);
    }
  }

  std::vector<unsigned> allCols(n);
  for (unsigned c = 0; c < n; ++c)
    allCols[c] = c;
  for (const auto &[row, constant] : program.addresses) {
    auto m = magnitudeOf(allCols, row, constant, bound);
    if (!m)
      return unknown("a physical address is not measurable");
    if (*m > limit)
      return exceeded("a physical address", *m);
  }
  return EmittedRangeResult{EmittedRangeVerdict::Representable, std::string()};
}

} // namespace systolic::logical
