//===- RankPartitionKernelCosetBudget.h - bounded general-kappa analysis --===//
//
// Phase F-KC-5c-1: work limits and query gates for kernel-coset analysis at any
// number kappa of reduction coordinates. analyzeKernelCosetWork computes the
// exact set of output coordinates of a source domain, or refuses, and nothing
// that can grow or search starts before a limit admits it. It is an analysis
// only: no pass calls it, it emits no IR, it takes its inputs by const
// reference, and a refusal leaves the source exactly as it was. The committed
// refusal of kappa > 2 in the lowering stands until a lowering exists.
//
// Two kinds of step occur here, and a counter does not turn one into the other.
//
//   Bounded. The work is a fixed function of the shape of the system, known
//   before the step starts: the shape scans, the row checks of the preflight,
//   the row prediction, the certificate test, removeTrivialRedundancy, and one
//   elimination of a column that has the certificate. Unit coefficients make
//   that elimination L + U for each lower L and upper U, or one substitution by
//   an equality, so its rows are predicted below and no entry can more than
//   double.
//
//   Gated. The step has no bound that can be stated from its shape.
//   buildIntegerKernelBasis computes an adjugate, whose entries can grow far
//   past the input. removeRedundantConstraints is a Simplex. isIntegerEmpty is
//   a Simplex under generalized basis reduction with branching. None of them
//   takes an iteration budget, a deadline or an interrupt, so once one starts
//   it runs until it finishes. A limit on queries bounds how many start and
//   says nothing about how long one takes. Each gate is a judgement on shape,
//   made before the start: it is why the step is expected to be cheap, not a
//   proof that it is. The values are measurements on sampled families, and a
//   harder family may exist. Neither a time bound nor a memory bound is claimed
//   for a gated step, and a wall-clock limit is not used, because the answer
//   must not depend on the machine.
//
// Nothing limits kappa. Every limit is a resource, read from the system at
// hand, and a caller can raise any of them. An integer query runs on the output
// image, after every kernel column is gone, so kappa adds eliminations and
// never adds dimensions to the query.
//
// Soundness. U is unimodular, so D' = { y | U*y in D } has the integer points
// of D, one for one, and its emptiness is that of D. Each elimination goes
// through eliminateExact, whose certificate makes the projection exact over the
// integers. A row reduction tightens rows by their gcd and removes rows implied
// by the others, which keeps every integer point. So once every kernel column
// is gone the system has exactly the integer points of the projection of D'
// onto the output columns: nothing is approximated, and the image is empty just
// when D is. A limit only ever stops the analysis. It changes no row, so it
// cannot turn an inexact result into an accepted one, and every verdict except
// ExactImage is a refusal that returns no image.
//
// The analysis avoids what has no shape gate: getConstantBound (projectOut of
// every other column), countKernelCosetCounts and buildGuardedPrelude (ungated
// queries), and certifyKernelCosetSourceOrder, whose verdict the kernel-coset
// lowering does not use.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "Systolic/RankPartitionKernelCosetRanges.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "llvm/ADT/DynamicAPInt.h"
#include "llvm/Support/CheckedArithmetic.h"

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

/// What the analysis of one statement may use. Each field is a resource, none
/// is a number of reduction coordinates, and a caller may raise any of them.
struct KernelCosetWorkLimits {
  /// The gate on the kernel basis: (rows + n) * n of the C matrix and its U,
  /// the rows of C, those rows times the columns they touch, and those rows
  /// times the bit length of the largest entry, which is how U's entries grow.
  std::uint64_t maxMatrixCells = 4352;
  std::uint64_t maxMatrixRows = 4;
  std::uint64_t maxMatrixTouchedCells = 64;
  std::uint64_t maxMatrixBits = 24;
  /// Rows of any system, predicted before the step that would create it.
  std::uint64_t maxSystemRows = 128;
  /// Rows times columns of any system, likewise.
  std::uint64_t maxSystemCells = 8192;
  /// Largest magnitude of any entry of any system, likewise.
  std::int64_t maxEntry = kKernelCosetValueLimit;
  /// Eliminations started, and the rows they generate between them.
  std::uint64_t maxEliminations = 512;
  std::uint64_t maxGeneratedRows = 16384;
  /// Row reductions started, and the gate on one: rows times columns, and the
  /// largest coefficient in a non-box row. Each is one Simplex.
  std::uint64_t maxReductions = 512;
  std::uint64_t maxReductionCells = 512;
  std::int64_t maxReductionCoefficient = std::int64_t(1) << 20;
  /// Emptiness queries started. This limits how many begin, not how long one
  /// runs.
  std::uint64_t maxEmptinessQueries = 512;
  /// The gate on one query: the variables its non-box rows touch, and the
  /// largest coefficient in such a row.
  std::uint64_t maxQueryVariables = 6;
  std::int64_t maxQueryCoefficient = 7;
};

/// Which limit stopped the analysis.
enum class KernelCosetLimit {
  None,
  MatrixCells,
  MatrixRows,
  MatrixTouchedCells,
  MatrixBits,
  SystemRows,
  SystemCells,
  Entry,
  Eliminations,
  GeneratedRows,
  Reductions,
  ReductionCells,
  ReductionCoefficient,
  EmptinessQueries,
  QueryVariables,
  QueryCoefficient,
  Arithmetic // a resource estimate does not fit in 64 bits
};

/// Work started so far. Each count is raised before its step runs.
struct KernelCosetWorkCounters {
  std::uint64_t eliminations = 0;
  std::uint64_t generatedRows = 0;
  std::uint64_t reductions = 0;
  std::uint64_t emptinessQueries = 0;
};

/// What the shape scan reads from the physical C matrix, with no search.
struct KernelCosetMatrixShape {
  std::uint64_t rows = 0;
  std::uint64_t columns = 0;
  /// Columns with a nonzero entry.
  std::uint64_t touchedColumns = 0;
  /// Bit length of the largest magnitude of an entry.
  std::uint64_t entryBits = 0;
};

/// What the shape scan reads from a system, with no search.
struct KernelCosetSystemShape {
  std::uint64_t rows = 0;
  std::uint64_t columns = 0;
  /// Rows with two or more variables.
  std::uint64_t nonBoxRows = 0;
  /// Variables with a nonzero coefficient in a non-box row.
  std::uint64_t coupledVariables = 0;
  /// Largest |a| over the variable coefficients of the non-box rows.
  llvm::DynamicAPInt largestCoefficient{0};
  /// Largest magnitude of any entry, the constant column included.
  llvm::DynamicAPInt largestEntry{0};
};

/// The limits of one statement and the work charged against them. The first
/// refusal is kept, and after it nothing else may start.
class KernelCosetWorkBudget {
public:
  explicit KernelCosetWorkBudget(
      KernelCosetWorkLimits limits = KernelCosetWorkLimits())
      : limits_(limits) {}

  const KernelCosetWorkLimits &limits() const { return limits_; }
  const KernelCosetWorkCounters &counters() const { return counters_; }
  KernelCosetLimit refusedBy() const { return refusedBy_; }
  bool exhausted() const { return refusedBy_ != KernelCosetLimit::None; }
  const std::string &why() const { return why_; }

  /// Record a refusal. Always false, so a caller can return it.
  bool refuse(KernelCosetLimit which, std::string why) {
    if (!exhausted()) {
      refusedBy_ = which;
      why_ = std::move(why);
    }
    return false;
  }

  /// The limit a kernel basis of this C matrix would exceed, or None. This is a
  /// gate too: the basis computes an adjugate with no budget of its own.
  KernelCosetLimit checkMatrix(const KernelCosetMatrixShape &shape) const {
    auto height = llvm::checkedAddUnsigned(shape.rows, shape.columns);
    auto cells = height ? llvm::checkedMulUnsigned(*height, shape.columns)
                        : std::nullopt;
    if (!cells)
      return KernelCosetLimit::Arithmetic;
    if (*cells > limits_.maxMatrixCells)
      return KernelCosetLimit::MatrixCells;
    if (shape.rows > limits_.maxMatrixRows)
      return KernelCosetLimit::MatrixRows;
    auto touched = llvm::checkedMulUnsigned(shape.rows, shape.touchedColumns);
    if (!touched)
      return KernelCosetLimit::Arithmetic;
    if (*touched > limits_.maxMatrixTouchedCells)
      return KernelCosetLimit::MatrixTouchedCells;
    auto bits = llvm::checkedMulUnsigned(shape.rows, shape.entryBits);
    if (!bits)
      return KernelCosetLimit::Arithmetic;
    if (*bits > limits_.maxMatrixBits)
      return KernelCosetLimit::MatrixBits;
    return KernelCosetLimit::None;
  }

  /// The limit a system of this size would exceed, or None.
  KernelCosetLimit checkSystem(std::uint64_t rows, std::uint64_t columns,
                               const llvm::DynamicAPInt &entry) const {
    if (rows > limits_.maxSystemRows)
      return KernelCosetLimit::SystemRows;
    auto cells = llvm::checkedMulUnsigned(rows, columns);
    if (!cells)
      return KernelCosetLimit::Arithmetic;
    if (*cells > limits_.maxSystemCells)
      return KernelCosetLimit::SystemCells;
    if (entry > llvm::DynamicAPInt(limits_.maxEntry))
      return KernelCosetLimit::Entry;
    return KernelCosetLimit::None;
  }

  /// The limit a query on a system of this shape would exceed, or None. This is
  /// the gate of the file comment, and it says nothing about the query's time.
  KernelCosetLimit checkQuery(const KernelCosetSystemShape &shape) const {
    if (shape.coupledVariables > limits_.maxQueryVariables)
      return KernelCosetLimit::QueryVariables;
    if (shape.largestCoefficient >
        llvm::DynamicAPInt(limits_.maxQueryCoefficient))
      return KernelCosetLimit::QueryCoefficient;
    return KernelCosetLimit::None;
  }

  /// Charge one elimination that generates up to `predictedRows`.
  bool startElimination(std::uint64_t predictedRows) {
    auto steps = next(counters_.eliminations, 1, limits_.maxEliminations,
                      KernelCosetLimit::Eliminations, "eliminations");
    if (!steps)
      return false;
    auto rows = next(counters_.generatedRows, predictedRows,
                     limits_.maxGeneratedRows, KernelCosetLimit::GeneratedRows,
                     "generated rows");
    if (!rows)
      return false;
    counters_.eliminations = *steps;
    counters_.generatedRows = *rows;
    return true;
  }

  /// Charge one row reduction of a system of this shape.
  bool startReduction(const KernelCosetSystemShape &shape) {
    auto reductions = next(counters_.reductions, 1, limits_.maxReductions,
                           KernelCosetLimit::Reductions, "row reductions");
    if (!reductions)
      return false;
    auto cells = llvm::checkedMulUnsigned(shape.rows, shape.columns);
    if (!cells)
      return refuse(KernelCosetLimit::Arithmetic,
                    "limit reached: the size of a row reduction does not fit "
                    "in 64 bits");
    if (*cells > limits_.maxReductionCells)
      return refuse(KernelCosetLimit::ReductionCells,
                    "limit reached: a row reduction of " +
                        std::to_string(*cells) + " cells, limit " +
                        std::to_string(limits_.maxReductionCells));
    if (shape.largestCoefficient >
        llvm::DynamicAPInt(limits_.maxReductionCoefficient))
      return refuse(KernelCosetLimit::ReductionCoefficient,
                    "limit reached: a row reduction with a coefficient past " +
                        std::to_string(limits_.maxReductionCoefficient));
    counters_.reductions = *reductions;
    return true;
  }

  /// Charge one emptiness query.
  bool startEmptinessQuery() {
    auto queries =
        next(counters_.emptinessQueries, 1, limits_.maxEmptinessQueries,
             KernelCosetLimit::EmptinessQueries, "emptiness queries");
    if (!queries)
      return false;
    counters_.emptinessQueries = *queries;
    return true;
  }

private:
  /// counter + amount when that stays within `limit`, otherwise a refusal.
  std::optional<std::uint64_t> next(std::uint64_t counter, std::uint64_t amount,
                                    std::uint64_t limit, KernelCosetLimit which,
                                    const char *name) {
    if (exhausted())
      return std::nullopt;
    auto total = llvm::checkedAddUnsigned(counter, amount);
    if (!total) {
      refuse(KernelCosetLimit::Arithmetic,
             std::string("limit reached: the count of ") + name +
                 " does not fit in 64 bits");
      return std::nullopt;
    }
    if (*total > limit) {
      refuse(which, std::string("limit reached: ") + name + " would reach " +
                        std::to_string(*total) + ", limit " +
                        std::to_string(limit));
      return std::nullopt;
    }
    return total;
  }

  KernelCosetWorkLimits limits_;
  KernelCosetWorkCounters counters_;
  KernelCosetLimit refusedBy_ = KernelCosetLimit::None;
  std::string why_;
};

/// How the rows of a system meet one column.
struct KernelCosetColumnRows {
  std::uint64_t lowers = 0;    // inequalities with a positive coefficient
  std::uint64_t uppers = 0;    // inequalities with a negative coefficient
  std::uint64_t unrelated = 0; // inequalities with a zero coefficient
  std::uint64_t pinning = 0;   // equalities with a nonzero coefficient
  std::uint64_t otherEqualities = 0;
};

/// The rows eliminating this column can leave, before any row is dropped as
/// redundant: lowers * uppers + the rows without the column, or, when an
/// equality substitutes it, every row but that equality. nullopt if it does not
/// fit in 64 bits.
inline std::optional<std::uint64_t>
predictKernelCosetEliminationRows(const KernelCosetColumnRows &c) {
  if (c.pinning > 0) {
    auto rows = llvm::checkedAddUnsigned(c.lowers, c.uppers);
    if (rows)
      rows = llvm::checkedAddUnsigned(*rows, c.unrelated);
    if (rows)
      rows = llvm::checkedAddUnsigned(*rows, c.pinning - 1);
    if (rows)
      rows = llvm::checkedAddUnsigned(*rows, c.otherEqualities);
    return rows;
  }
  auto rows = llvm::checkedMulAddUnsigned(c.lowers, c.uppers, c.unrelated);
  return rows ? llvm::checkedAddUnsigned(*rows, c.otherEqualities) : rows;
}

/// The shape of a C matrix whose rows all have `n` entries. Linear in them.
inline KernelCosetMatrixShape measureKernelCosetMatrix(const IntMatrix &Crows,
                                                       unsigned n) {
  KernelCosetMatrixShape shape;
  shape.rows = Crows.size();
  shape.columns = n;
  std::vector<bool> touched(n, false);
  BigInt largest = 0;
  for (const IntRow &row : Crows)
    for (unsigned j = 0; j < n; ++j)
      if (row[j] != 0) {
        touched[j] = true;
        largest =
            std::max<BigInt>(largest, kernel_coset_detail::bigAbs(row[j]));
      }
  shape.touchedColumns = std::count(touched.begin(), touched.end(), true);
  if (largest != 0)
    shape.entryBits = boost::multiprecision::msb(largest) + 1;
  return shape;
}

/// The shape of `P`. Linear in its entries, with no search.
inline KernelCosetSystemShape
measureKernelCosetSystem(const mlir::presburger::IntegerPolyhedron &P) {
  using llvm::DynamicAPInt;
  KernelCosetSystemShape shape;
  const unsigned vars = P.getNumVars();
  shape.columns = P.getNumCols();
  shape.rows = std::uint64_t(P.getNumInequalities()) + P.getNumEqualities();
  std::vector<bool> coupled(vars, false);
  const DynamicAPInt zero(0);
  auto scan = [&](llvm::ArrayRef<DynamicAPInt> row) {
    unsigned nonzero = 0;
    for (unsigned j = 0; j < vars; ++j)
      nonzero += row[j] != zero;
    for (const DynamicAPInt &v : row)
      shape.largestEntry = std::max(shape.largestEntry, llvm::abs(v));
    if (nonzero < 2)
      return;
    ++shape.nonBoxRows;
    for (unsigned j = 0; j < vars; ++j)
      if (row[j] != zero) {
        coupled[j] = true;
        shape.largestCoefficient =
            std::max(shape.largestCoefficient, llvm::abs(row[j]));
      }
  };
  for (unsigned r = 0; r < P.getNumInequalities(); ++r)
    scan(P.getInequality(r));
  for (unsigned r = 0; r < P.getNumEqualities(); ++r)
    scan(P.getEquality(r));
  shape.coupledVariables = std::count(coupled.begin(), coupled.end(), true);
  return shape;
}

enum class KernelCosetEmptiness { Empty, NonEmpty, Unknown };

struct KernelCosetEmptinessResult {
  KernelCosetEmptiness verdict = KernelCosetEmptiness::Unknown;
  /// What stopped the query when the verdict is Unknown; None if the system has
  /// symbols or locals, which the gate does not measure.
  KernelCosetLimit limit = KernelCosetLimit::None;
  std::string why;
};

namespace kernel_coset_budget_detail {

using llvm::DynamicAPInt;
using mlir::presburger::IntegerPolyhedron;

/// The integer interval of each variable under the rows with one variable.
struct VariableIntervals {
  std::vector<std::optional<DynamicAPInt>> lower, upper;
  /// A row with no variable fails, an equality has no integer solution, or an
  /// interval is empty. The intervals mean nothing when this is set.
  bool contradiction = false;
};

inline unsigned countVariables(llvm::ArrayRef<DynamicAPInt> row, unsigned vars,
                               unsigned &last) {
  unsigned count = 0;
  for (unsigned j = 0; j < vars; ++j)
    if (row[j] != DynamicAPInt(0)) {
      ++count;
      last = j;
    }
  return count;
}

/// Linear in the entries of `P`, with no search. P must have no locals.
inline VariableIntervals readVariableIntervals(const IntegerPolyhedron &P) {
  const unsigned vars = P.getNumVars();
  const DynamicAPInt zero(0);
  VariableIntervals out;
  out.lower.resize(vars);
  out.upper.resize(vars);
  out.contradiction = P.isEmptyByGCDTest();
  auto raiseLower = [&](unsigned v, const DynamicAPInt &value) {
    if (!out.lower[v] || *out.lower[v] < value)
      out.lower[v] = value;
  };
  auto dropUpper = [&](unsigned v, const DynamicAPInt &value) {
    if (!out.upper[v] || value < *out.upper[v])
      out.upper[v] = value;
  };
  for (unsigned r = 0; r < P.getNumInequalities(); ++r) {
    llvm::ArrayRef<DynamicAPInt> row = P.getInequality(r);
    unsigned v = 0;
    const unsigned count = countVariables(row, vars, v);
    const DynamicAPInt &c = row[vars];
    if (count == 0 && c < zero)
      out.contradiction = true;
    if (count != 1)
      continue;
    if (row[v] > zero) // a*v + c >= 0
      raiseLower(v, ceilDiv(-c, row[v]));
    else
      dropUpper(v, floorDiv(c, -row[v]));
  }
  for (unsigned r = 0; r < P.getNumEqualities(); ++r) {
    llvm::ArrayRef<DynamicAPInt> row = P.getEquality(r);
    unsigned v = 0;
    const unsigned count = countVariables(row, vars, v);
    const DynamicAPInt &c = row[vars];
    if (count == 0 && c != zero)
      out.contradiction = true;
    if (count != 1)
      continue;
    const DynamicAPInt value = -c / row[v]; // exact, or the gcd test has failed
    raiseLower(v, value);
    dropUpper(v, value);
  }
  for (unsigned v = 0; v < vars; ++v)
    if (out.lower[v] && out.upper[v] && *out.upper[v] < *out.lower[v])
      out.contradiction = true;
  return out;
}

/// `P` restricted to the variables of its non-box rows. Each other variable has
/// only rows of its own and, the intervals being nonempty, a value independent
/// of the rest, so the two systems are empty together. Bounds are written as
/// unit rows, the same integer set.
inline IntegerPolyhedron restrictToCoupled(const IntegerPolyhedron &P,
                                           const VariableIntervals &intervals) {
  using mlir::presburger::PresburgerSpace;
  const unsigned vars = P.getNumVars();
  const DynamicAPInt zero(0), one(1);
  std::vector<bool> keep(vars, false);
  auto mark = [&](llvm::ArrayRef<DynamicAPInt> row) {
    unsigned last = 0;
    if (countVariables(row, vars, last) < 2)
      return;
    for (unsigned j = 0; j < vars; ++j)
      if (row[j] != zero)
        keep[j] = true;
  };
  for (unsigned r = 0; r < P.getNumInequalities(); ++r)
    mark(P.getInequality(r));
  for (unsigned r = 0; r < P.getNumEqualities(); ++r)
    mark(P.getEquality(r));

  std::vector<unsigned> index(vars, 0);
  unsigned kept = 0;
  for (unsigned j = 0; j < vars; ++j)
    if (keep[j])
      index[j] = kept++;

  IntegerPolyhedron Q(PresburgerSpace::getSetSpace(kept, 0, 0));
  auto narrow = [&](llvm::ArrayRef<DynamicAPInt> row) {
    llvm::SmallVector<DynamicAPInt, 8> out(kept + 1, zero);
    for (unsigned j = 0; j < vars; ++j)
      if (keep[j])
        out[index[j]] = row[j];
    out[kept] = row[vars];
    return out;
  };
  for (unsigned r = 0; r < P.getNumInequalities(); ++r) {
    unsigned last = 0;
    if (countVariables(P.getInequality(r), vars, last) >= 2)
      Q.addInequality(narrow(P.getInequality(r)));
  }
  for (unsigned r = 0; r < P.getNumEqualities(); ++r) {
    unsigned last = 0;
    if (countVariables(P.getEquality(r), vars, last) >= 2)
      Q.addEquality(narrow(P.getEquality(r)));
  }
  for (unsigned j = 0; j < vars; ++j) {
    if (!keep[j])
      continue;
    if (intervals.lower[j]) { // v - lower >= 0
      llvm::SmallVector<DynamicAPInt, 8> row(kept + 1, zero);
      row[index[j]] = one;
      row[kept] = -*intervals.lower[j];
      Q.addInequality(row);
    }
    if (intervals.upper[j]) { // upper - v >= 0
      llvm::SmallVector<DynamicAPInt, 8> row(kept + 1, zero);
      row[index[j]] = -one;
      row[kept] = *intervals.upper[j];
      Q.addInequality(row);
    }
  }
  return Q;
}

/// Does column `col` of `D` have a row on each side? A column missing one is
/// unbounded in that direction, or the domain is empty.
inline bool boundedOnBothSides(const IntegerPolyhedron &D, unsigned col) {
  const DynamicAPInt zero(0);
  bool lower = false, upper = false;
  for (unsigned r = 0; r < D.getNumInequalities(); ++r) {
    const DynamicAPInt &a = D.atIneq(r, col);
    lower = lower || a > zero;
    upper = upper || a < zero;
  }
  for (unsigned r = 0; r < D.getNumEqualities(); ++r)
    if (D.atEq(r, col) != zero)
      lower = upper = true;
  return lower && upper;
}

inline KernelCosetColumnRows
countColumnRows(const kernel_coset_image_detail::Projected &pr, unsigned pos) {
  const DynamicAPInt zero(0);
  KernelCosetColumnRows c;
  for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r) {
    const DynamicAPInt &a = pr.P.atIneq(r, pos);
    if (a > zero)
      ++c.lowers;
    else if (a < zero)
      ++c.uppers;
    else
      ++c.unrelated;
  }
  for (unsigned r = 0; r < pr.P.getNumEqualities(); ++r) {
    if (pr.P.atEq(r, pos) != zero)
      ++c.pinning;
    else
      ++c.otherEqualities;
  }
  return c;
}

struct CertifiedColumn {
  unsigned col = 0;
  std::uint64_t predictedRows = 0;
};

/// The certified column among `pending` whose elimination can leave the fewest
/// rows, the lowest column on a tie.
inline std::optional<CertifiedColumn>
chooseCertifiedColumn(const kernel_coset_image_detail::Projected &pr,
                      const std::vector<unsigned> &pending) {
  using namespace kernel_coset_image_detail;
  std::optional<CertifiedColumn> best;
  for (unsigned col : pending) {
    auto pos = positionOf(pr, col);
    if (!pos || !unitEverywhere(pr, col))
      continue;
    auto rows = predictKernelCosetEliminationRows(countColumnRows(pr, *pos));
    if (rows && (!best || *rows < best->predictedRows))
      best = CertifiedColumn{col, *rows};
  }
  return best;
}

inline const char *limitName(KernelCosetLimit limit) {
  switch (limit) {
  case KernelCosetLimit::None:
    return "no limit";
  case KernelCosetLimit::MatrixCells:
    return "kernel basis size";
  case KernelCosetLimit::MatrixRows:
    return "kernel basis rows";
  case KernelCosetLimit::MatrixTouchedCells:
    return "kernel basis touched cells";
  case KernelCosetLimit::MatrixBits:
    return "kernel basis entry bits";
  case KernelCosetLimit::SystemRows:
    return "rows of a system";
  case KernelCosetLimit::SystemCells:
    return "cells of a system";
  case KernelCosetLimit::Entry:
    return "entry magnitude";
  case KernelCosetLimit::Eliminations:
    return "eliminations";
  case KernelCosetLimit::GeneratedRows:
    return "generated rows";
  case KernelCosetLimit::Reductions:
    return "row reductions";
  case KernelCosetLimit::ReductionCells:
    return "row reduction size";
  case KernelCosetLimit::ReductionCoefficient:
    return "row reduction coefficient";
  case KernelCosetLimit::EmptinessQueries:
    return "emptiness queries";
  case KernelCosetLimit::QueryVariables:
    return "query variables";
  case KernelCosetLimit::QueryCoefficient:
    return "query coefficient";
  case KernelCosetLimit::Arithmetic:
    return "a resource estimate overflowed 64 bits";
  }
  return "unknown limit";
}

} // namespace kernel_coset_budget_detail

/// Is `P` empty over the integers? The rows are read first, and most systems
/// end there with no query. Otherwise one isIntegerEmpty runs if the gate and
/// the budget both admit it. A query refused by its shape is not charged and
/// does not exhaust the budget: the caller may ask again about a smaller
/// system.
inline KernelCosetEmptinessResult
gatedIsIntegerEmpty(const mlir::presburger::IntegerPolyhedron &P,
                    KernelCosetWorkBudget &budget) {
  using namespace kernel_coset_budget_detail;
  KernelCosetEmptinessResult out;
  if (P.getNumSymbolVars() != 0 || P.getNumLocalVars() != 0) {
    out.why = "UNKNOWN: the system carries symbols or locals";
    return out;
  }
  const VariableIntervals intervals = readVariableIntervals(P);
  if (intervals.contradiction) {
    out.verdict = KernelCosetEmptiness::Empty;
    return out;
  }
  const KernelCosetSystemShape shape = measureKernelCosetSystem(P);
  if (shape.nonBoxRows == 0) {
    // A product of nonempty integer intervals.
    out.verdict = KernelCosetEmptiness::NonEmpty;
    return out;
  }
  out.limit = budget.checkQuery(shape);
  if (out.limit != KernelCosetLimit::None) {
    out.why = std::string("limit reached: ") + limitName(out.limit) +
              " refuses a query with " + std::to_string(shape.nonBoxRows) +
              " non-box rows over " + std::to_string(shape.coupledVariables) +
              " variables";
    return out;
  }
  if (!budget.startEmptinessQuery()) {
    out.limit = budget.refusedBy();
    out.why = budget.why();
    return out;
  }
  out.verdict = restrictToCoupled(P, intervals).isIntegerEmpty()
                    ? KernelCosetEmptiness::Empty
                    : KernelCosetEmptiness::NonEmpty;
  return out;
}

enum class KernelCosetWorkVerdict {
  ExactImage,   // outputImage is exactly the output coordinates of the domain.
  Empty,        // the domain has no integer point.
  NotCertified, // no remaining kernel column has the exactness certificate.
  LimitReached, // a limit stopped the analysis before the step it guards.
  Unsupported   // the input is outside what this analysis models.
};

/// One elimination, as predicted and as it came out.
struct KernelCosetEliminationStep {
  unsigned col = 0;
  std::uint64_t predictedRows = 0;
  std::uint64_t actualRows = 0;
};

struct KernelCosetWorkResult {
  KernelCosetWorkVerdict verdict = KernelCosetWorkVerdict::Unsupported;
  std::string why;
  /// Set by the step that computes them, whatever the verdict. They describe
  /// the exact domain: nothing here is an approximation.
  IntMatrix U, Uinv;
  unsigned outputRank = 0, kernelRank = 0;
  std::vector<unsigned> outputCols, kernelCols;
  std::optional<mlir::presburger::IntegerPolyhedron> reducedDomain;
  std::vector<KernelCosetEliminationStep> eliminations;
  /// Only with ExactImage: { o | (o, z) in D' for some integer z }, whose dims
  /// are outputCols in order.
  std::optional<mlir::presburger::IntegerPolyhedron> outputImage;
};

/// Compute the exact output image of the domain `D` under the physical C rows
/// `Crows`, within `budget`. See the file comment. A refusal returns no image
/// and `why` says what stopped it. The exact image needs no emptiness query of
/// D itself: an exact projection is empty just when D is, so the one query runs
/// on the image, in rho variables whatever kappa is.
inline KernelCosetWorkResult
analyzeKernelCosetWork(const IntMatrix &Crows,
                       const mlir::presburger::IntegerPolyhedron &D, unsigned n,
                       KernelCosetWorkBudget &budget) {
  using namespace kernel_coset_image_detail;
  using namespace kernel_coset_budget_detail;
  using Verdict = KernelCosetWorkVerdict;

  KernelCosetWorkResult out;
  auto stop = [&](Verdict verdict, std::string why) {
    out.verdict = verdict;
    out.why = std::move(why);
    return std::move(out);
  };
  auto limited = [&] { return stop(Verdict::LimitReached, budget.why()); };

  // ---- What this analysis does not model ---------------------------------
  if (n == 0)
    return stop(Verdict::Unsupported, "UNKNOWN: degenerate source rank");
  if (D.getNumDimVars() != n || D.getNumSymbolVars() != 0 ||
      D.getNumLocalVars() != 0)
    return stop(Verdict::Unsupported,
                "UNKNOWN: the source domain carries symbols or locals "
                "(congruence)");
  if (Crows.empty())
    return stop(Verdict::Unsupported, "UNKNOWN: no physical C access");
  for (const IntRow &row : Crows)
    if (row.size() != n)
      return stop(Verdict::Unsupported, "UNKNOWN: access row arity");
  if (budget.exhausted())
    return limited();

  // ---- Preflight: no search, and sound for every input --------------------
  auto admit = [&](KernelCosetLimit limit, const std::string &what) {
    return limit == KernelCosetLimit::None ||
           budget.refuse(limit, "limit reached: " +
                                    std::string(limitName(limit)) + " of " +
                                    what);
  };
  const KernelCosetSystemShape sourceShape = measureKernelCosetSystem(D);
  if (!admit(budget.checkMatrix(measureKernelCosetMatrix(Crows, n)),
             "the C matrix") ||
      !admit(budget.checkSystem(sourceShape.rows, sourceShape.columns,
                                sourceShape.largestEntry),
             "the source domain"))
    return limited();
  if (readVariableIntervals(D).contradiction)
    return stop(Verdict::Empty, "the source domain is empty on inspection");
  for (unsigned j = 0; j < n; ++j)
    if (!boundedOnBothSides(D, j))
      return stop(Verdict::Unsupported,
                  "UNKNOWN: source variable " + std::to_string(j) +
                      " has no bound on one side");

  // ---- The reduced domain -------------------------------------------------
  auto kb = buildIntegerKernelBasis(Crows, n);
  if (!kb)
    return stop(Verdict::Unsupported,
                "UNKNOWN: integer kernel basis (overflow or malformed C rows)");
  if (kb->outputRank == 0)
    return stop(Verdict::Unsupported, "UNKNOWN: constant physical C (rank 0)");
  if (kb->kernelRank == 0)
    return stop(Verdict::Unsupported,
                "UNKNOWN: no reduction coordinate (kappa == 0)");
  out.outputRank = kb->outputRank;
  out.kernelRank = kb->kernelRank;
  out.kernelCols = kb->kernelColIndices;
  {
    std::vector<bool> isKernel(n, false);
    for (unsigned c : out.kernelCols)
      isKernel[c] = true;
    for (unsigned j = 0; j < n; ++j)
      if (!isKernel[j])
        out.outputCols.push_back(j);
  }
  auto reduced = buildReducedDomain(kb->U, D, n);
  if (!reduced)
    return stop(Verdict::Unsupported, "UNKNOWN: reduced domain (i64 overflow)");
  out.U = std::move(kb->U);
  out.Uinv = std::move(kb->Uinv);
  const KernelCosetSystemShape reducedShape =
      measureKernelCosetSystem(*reduced);
  if (!admit(budget.checkSystem(reducedShape.rows, reducedShape.columns,
                                reducedShape.largestEntry),
             "the reduced domain"))
    return limited();
  out.reducedDomain = *reduced;

  // ---- Certified eliminations, cheapest first -----------------------------
  // A reduction runs only when the analysis cannot otherwise go on: it is the
  // one step of this loop that is a search.
  Projected pr{std::move(*reduced), std::vector<unsigned>(n)};
  for (unsigned j = 0; j < n; ++j)
    pr.cols[j] = j;
  pr.P.removeTrivialRedundancy();
  std::vector<unsigned> pending = out.kernelCols;
  bool isReduced = false;
  auto reduce = [&] {
    if (!budget.startReduction(measureKernelCosetSystem(pr.P)))
      return false;
    pr.P.removeRedundantConstraints();
    isReduced = true;
    return true;
  };
  while (!pending.empty()) {
    const auto choice = chooseCertifiedColumn(pr, pending);
    const llvm::DynamicAPInt entry =
        measureKernelCosetSystem(pr.P).largestEntry;
    const llvm::DynamicAPInt nextEntry = llvm::DynamicAPInt(2) * entry;
    const KernelCosetLimit blocked =
        choice ? budget.checkSystem(choice->predictedRows,
                                    pr.P.getNumCols() - 1, nextEntry)
               : KernelCosetLimit::None;
    if ((!choice || blocked != KernelCosetLimit::None) && !isReduced) {
      if (!reduce())
        return limited();
      continue;
    }
    if (!choice)
      return stop(Verdict::NotCertified,
                  "UNKNOWN: no remaining reduction coordinate has unit "
                  "coefficients in every row -- the projection is not "
                  "certified integer-exact");
    if (blocked != KernelCosetLimit::None) {
      budget.refuse(blocked, "limit reached: " +
                                 std::string(limitName(blocked)) +
                                 " of the elimination of reduced column " +
                                 std::to_string(choice->col) + " (predicted " +
                                 std::to_string(choice->predictedRows) +
                                 " rows)");
      return limited();
    }
    if (!budget.startElimination(choice->predictedRows))
      return limited();
    std::string why;
    if (!eliminateExact(pr, choice->col, why))
      return stop(Verdict::Unsupported, why);
    isReduced = false;
    pending.erase(std::find(pending.begin(), pending.end(), choice->col));
    const KernelCosetSystemShape after = measureKernelCosetSystem(pr.P);
    if (after.rows > choice->predictedRows || after.largestEntry > nextEntry)
      return stop(Verdict::Unsupported,
                  "UNKNOWN: an elimination exceeded its prediction");
    out.eliminations.push_back(
        {choice->col, choice->predictedRows, after.rows});
  }
  if (pr.cols != out.outputCols)
    return stop(Verdict::Unsupported,
                "UNKNOWN: kernel/output column bookkeeping");

  // ---- Emptiness, on the image --------------------------------------------
  auto emptiness = gatedIsIntegerEmpty(pr.P, budget);
  if (emptiness.verdict == KernelCosetEmptiness::Unknown && !isReduced &&
      !budget.exhausted()) {
    if (!reduce())
      return limited();
    emptiness = gatedIsIntegerEmpty(pr.P, budget);
  }
  if (emptiness.verdict == KernelCosetEmptiness::Unknown) {
    budget.refuse(emptiness.limit, emptiness.why);
    return stop(Verdict::LimitReached, emptiness.why);
  }
  if (emptiness.verdict == KernelCosetEmptiness::Empty)
    return stop(Verdict::Empty, "the source domain has no integer point");
  out.outputImage = std::move(pr.P);
  return stop(Verdict::ExactImage, std::string());
}

} // namespace systolic::logical
