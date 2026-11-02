//===- RankPartitionKernelCosetGap.h - exact integer output membership ----===//
//
// Phase F-KC-5c-2: exact gap certificates. For the reduced domain D' over
// output coordinates y and kernel coordinates t, with any existential locals q,
//
//   P = { y | exists integer t, q : (y, t, q) in D' }.
//
// Rational projection contains P and can be strictly larger: i = 2 * k projects
// to every i, while P holds only the even ones. A point of the shadow outside P
// is a gap. This header decides, for one integer y, whether y is in P. It never
// reads rational feasibility as integer feasibility. It is an analysis only: no
// pass calls it and it emits no IR.
//
// The algorithm has one plan per statement and one decision per candidate. Let
// z collect the existential coordinates (t, q).
//
//   1. Equalities. Let E * z = g(y) be the equalities that touch z. With V
//      unimodular and E * V = [H | 0], substitute z = V * (s, w). Then
//      H * s = g and w is free. H has full column rank, so s is the unique
//      rational solution, and a row of H adds at most one unknown, so forward
//      substitution gives s_k = (nu_k . g) / m_k, and nu . g = 0 for each
//      dependent row. An integer solution exists exactly when m_k divides
//      nu_k . g for every k and each dependent row holds. These are the
//      congruences of the image.
//
//   2. Exact elimination. The inequalities become rows over (y, s, w). A column
//      of w is eliminated by Fourier-Motzkin only when all its lower
//      coefficients or all its upper coefficients are 1, which is exact over
//      the integers for integer values of the other columns: the bound on the
//      unit side is an integer, and the real shadow says it fits. Elimination
//      stops with one column left, or when no column has that certificate.
//
//   3. The last column. With y and s fixed, one column remains under rows
//      a * w >= L and b * w <= U, and an integer w exists exactly when
//      max ceil(L / a) <= min floor(U / b). This test is where a gap shows: the
//      rational interval can hold points when the integer interval holds none.
//
//   4. Two or more columns left. No local certificate applies. One gated
//      findIntegerSample decides the fiber at this y, or the answer is Unknown.
//
// A Member answer always carries a witness. It is rebuilt level by level, each
// level having an integer value because its elimination was exact, and it is
// then substituted into the rows of D' and, through U, of the source domain D.
// A failed substitution gives Unknown, never Member, so no Member answer rests
// on a library claim. A NotMember answer is a proof, and the proof says which:
// a false row, a congruence with its combination of equalities, an empty
// interval, or LibraryEmpty, which is the one case that trusts
// findIntegerSample.
//
// Bounded and uninterruptible work stay apart, as in the budget header.
// Deciding a candidate in classes Pinned and Interval is local arithmetic, a
// fixed function of the plan's shape and of the size of y, and it never calls a
// search. The plan itself needs buildIntegerKernelBasis, and on demand
// removeRedundantConstraints. Class Search calls findIntegerSample once per
// candidate. Those three take no budget, deadline or interrupt: their gates
// decide whether they start, and no time or memory bound is claimed for them.
//
// One LLVM 24 defect is worse than a missing bound. Simplex::addInequality
// keeps pivoting after the tableau is marked empty, where its anti-cycling rule
// no longer applies, so constructing a Simplex from a rationally infeasible
// system can loop forever: 8 rows over 5 variables are enough.
// findIntegerSample, isIntegerEmpty and removeRedundantConstraints all
// construct one. So no system reaches them here before rationallyEmpty has
// added its rows one at a time and stopped at the first that empties the
// tableau. This avoids that defect. It is not a time bound.
//
// Arithmetic is exact: every value is a DynamicAPInt, so nothing here can
// overflow. No enumeration of the domain, the image or a fiber occurs anywhere,
// and the fiber size is returned only where it has a closed form: 1 for a
// pinned fiber, an interval length for one free coordinate. Beyond that it
// would need a lattice point count, and it stays empty. With locals only the
// pinned case is reported, because an interval then counts (t, q) rather
// than t.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "Systolic/RankPartitionKernelCosetBudget.h"

#include "mlir/Analysis/Presburger/IntegerRelation.h"
#include "mlir/Analysis/Presburger/Simplex.h"
#include "llvm/ADT/DynamicAPInt.h"

#include <algorithm>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace systolic::logical {

/// How a candidate of this statement is decided once the lattice holds.
enum class KernelCosetGapClass {
  Pinned,   // no free coordinate: the fiber is at most one point.
  Interval, // one column left: local arithmetic, no search.
  Search    // two or more columns left: one gated query per candidate.
};

/// One consequence of the equalities, as the affine form v(y) = form . (y, 1).
/// With modulus m > 0, a pivot of s is v / m, and m must divide v. With modulus
/// 0 the row is dependent, and v must vanish.
struct KernelCosetLatticeCondition {
  /// nu, over `kernelEqualities`.
  std::vector<llvm::DynamicAPInt> combination;
  llvm::DynamicAPInt modulus{0};
  /// One coefficient per output coordinate, then the constant.
  std::vector<llvm::DynamicAPInt> form;
};

/// A system before one exact elimination, kept to rebuild the witness.
struct KernelCosetGapLevel {
  unsigned col = 0;
  kernel_coset_image_detail::Projected system;
};

/// What certifyKernelCosetCandidate needs for every candidate of one statement.
/// Column ids of `system` and `levels`: rho outputs, then one per entry of
/// `pivotCols`, then one per entry of `freeCols`.
struct KernelCosetGapPlan {
  unsigned numLocals = 0;
  IntMatrix U, Uinv;
  std::vector<unsigned> outputCols, kernelCols;
  mlir::presburger::IntegerPolyhedron source, reduced;
  /// The equalities of `reduced` that touch an existential coordinate.
  std::vector<unsigned> kernelEqualities;
  /// V, over the existential coordinates: kernel columns, then locals.
  std::vector<std::vector<llvm::DynamicAPInt>> lattice;
  std::vector<unsigned> pivotCols, freeCols;
  std::vector<KernelCosetLatticeCondition> pivots, dependent;
  std::vector<KernelCosetGapLevel> levels;
  kernel_coset_image_detail::Projected system;
  /// The free columns of `system` that were not eliminated.
  std::vector<unsigned> remaining;
  KernelCosetGapClass gapClass = KernelCosetGapClass::Search;

  KernelCosetGapPlan(mlir::presburger::IntegerPolyhedron src,
                     mlir::presburger::IntegerPolyhedron red,
                     kernel_coset_image_detail::Projected sys)
      : source(std::move(src)), reduced(std::move(red)),
        system(std::move(sys)) {}
};

enum class KernelCosetGapPlanVerdict { Ready, LimitReached, Unsupported };

struct KernelCosetGapPlanResult {
  KernelCosetGapPlanVerdict verdict = KernelCosetGapPlanVerdict::Unsupported;
  std::string why;
  std::optional<KernelCosetGapPlan> plan;
};

enum class KernelCosetMembership { Member, NotMember, Unknown };

/// Why a candidate has its answer.
enum class KernelCosetGapProof {
  None,             // Unknown.
  Witness,          // Member: a point substituted into D' and D.
  FalseRow,         // A row with no remaining coordinate fails at y.
  Congruence,       // m does not divide nu . g(y), yet m divides nu . E.
  Inconsistent,     // nu . E = 0 and nu . g(y) != 0.
  EmptyInterval,    // ceil(L / a) > floor(U / b) on the last column.
  ConcreteConflict, // The fiber fails on inspection, with no query.
  LibraryEmpty      // findIntegerSample found no point. Trusted, not rechecked.
};

struct KernelCosetCandidateResult {
  KernelCosetMembership membership = KernelCosetMembership::Unknown;
  KernelCosetGapProof proof = KernelCosetGapProof::None;
  std::string why;
  /// Member: the kernel point, the locals and the source point it maps to.
  std::vector<llvm::DynamicAPInt> kernelWitness, localWitness, sourceWitness;
  /// Member: the number of source points at y, where it has a closed form.
  std::optional<llvm::DynamicAPInt> multiplicity;
  /// Congruence and Inconsistent: nu, and m (0 for Inconsistent).
  std::vector<llvm::DynamicAPInt> combination;
  llvm::DynamicAPInt modulus{0};
  /// EmptyInterval: the integer bounds, lower > upper.
  llvm::DynamicAPInt lower{0}, upper{0};
  /// EmptyInterval: the last column has a rational value and no integer one.
  bool rationalGap = false;
};

/// A gated findIntegerSample: `sample` holds a point when the verdict is
/// NonEmpty.
struct KernelCosetSampleResult {
  KernelCosetEmptiness verdict = KernelCosetEmptiness::Unknown;
  KernelCosetLimit limit = KernelCosetLimit::None;
  std::string why;
  /// Whether the library ran, as against an answer read from the rows.
  bool queried = false;
  std::vector<llvm::DynamicAPInt> sample;
};

namespace kernel_coset_gap_detail {

using kernel_coset_image_detail::Projected;
using llvm::DynamicAPInt;
using mlir::presburger::IntegerPolyhedron;
using APMatrix = std::vector<std::vector<DynamicAPInt>>;

inline bool toAPMatrix(const IntMatrix &in, APMatrix &out) {
  out.assign(in.size(), {});
  for (std::size_t i = 0; i < in.size(); ++i)
    for (const BigInt &v : in[i]) {
      auto a = kernel_coset_detail::toAPInt(v);
      if (!a)
        return false;
      out[i].push_back(*a);
    }
  return true;
}

/// form . (y, 1), for a form with one coefficient per entry of y and a
/// constant.
inline DynamicAPInt evaluateForm(llvm::ArrayRef<DynamicAPInt> form,
                                 llvm::ArrayRef<DynamicAPInt> y) {
  DynamicAPInt total = form[y.size()];
  for (std::size_t j = 0; j < y.size(); ++j)
    total += form[j] * y[j];
  return total;
}

/// Is `P`, with no symbols or locals, empty over the rationals? See the file
/// comment for why the rows go in one at a time. An equality is its two rows.
inline bool rationallyEmpty(const IntegerPolyhedron &P) {
  mlir::presburger::Simplex simplex(P.getNumVars());
  for (unsigned r = 0; r < P.getNumInequalities(); ++r) {
    simplex.addInequality(P.getInequality(r));
    if (simplex.isEmpty())
      return true;
  }
  for (unsigned r = 0; r < P.getNumEqualities(); ++r) {
    llvm::SmallVector<DynamicAPInt, 8> row(P.getEquality(r).begin(),
                                           P.getEquality(r).end());
    simplex.addInequality(row);
    if (simplex.isEmpty())
      return true;
    for (DynamicAPInt &v : row)
      v = -v;
    simplex.addInequality(row);
    if (simplex.isEmpty())
      return true;
  }
  return false;
}

/// removeRedundantConstraints, unless `P` is rationally empty, which it must
/// not see. false then, with `P` unchanged.
inline bool reduceRows(IntegerPolyhedron &P) {
  if (rationallyEmpty(P))
    return false;
  P.removeRedundantConstraints();
  return true;
}

/// Solve H * s = g symbolically by forward substitution, over the columns
/// `pivotCols` of H, in the order of the rows. See step 1 of the file comment.
/// A row of a Hermite normal form adds at most one unknown. false when that
/// does not hold, which leaves a pivot unset or the identity below unsatisfied.
inline bool solveLattice(const APMatrix &H,
                         const std::vector<unsigned> &pivotCols,
                         std::vector<KernelCosetLatticeCondition> &pivots,
                         std::vector<KernelCosetLatticeCondition> &dependent) {
  const unsigned rows = H.size(), rank = pivotCols.size();
  const DynamicAPInt zero(0), one(1);
  std::vector<bool> known(rank, false);
  pivots.assign(rank, {});
  dependent.clear();
  for (unsigned i = 0; i < rows; ++i) {
    std::optional<unsigned> fresh;
    DynamicAPInt scale = one;
    for (unsigned k = 0; k < rank; ++k) {
      if (H[i][pivotCols[k]] == zero)
        continue;
      if (known[k])
        scale = llvm::lcm(scale, pivots[k].modulus);
      else
        fresh = k;
    }
    std::vector<DynamicAPInt> nu(rows, zero);
    nu[i] = scale;
    for (unsigned k = 0; k < rank; ++k) {
      if (!known[k] || H[i][pivotCols[k]] == zero)
        continue;
      const DynamicAPInt factor =
          H[i][pivotCols[k]] * (scale / pivots[k].modulus);
      for (unsigned e = 0; e < rows; ++e)
        nu[e] -= factor * pivots[k].combination[e];
    }
    KernelCosetLatticeCondition condition;
    if (fresh)
      condition.modulus = scale * llvm::abs(H[i][pivotCols[*fresh]]);
    DynamicAPInt divisor = condition.modulus;
    for (const DynamicAPInt &v : nu)
      divisor = llvm::gcd(divisor, llvm::abs(v));
    if (divisor > zero) {
      condition.modulus /= divisor;
      for (DynamicAPInt &v : nu)
        v /= divisor;
    }
    condition.combination = std::move(nu);
    if (fresh) {
      known[*fresh] = true;
      pivots[*fresh] = std::move(condition);
    } else {
      dependent.push_back(std::move(condition));
    }
  }
  if (std::find(known.begin(), known.end(), false) != known.end())
    return false;
  // The identity the proof rests on: nu_k . H = m_k * e_k, and nu . H = 0.
  auto check = [&](const KernelCosetLatticeCondition &c,
                   std::optional<unsigned> pivot) {
    for (unsigned k = 0; k < rank; ++k) {
      DynamicAPInt total(0);
      for (unsigned e = 0; e < rows; ++e)
        total += c.combination[e] * H[e][pivotCols[k]];
      if (total != (pivot && *pivot == k ? c.modulus : zero))
        return false;
    }
    return true;
  };
  for (unsigned k = 0; k < rank; ++k)
    if (!check(pivots[k], k))
      return false;
  for (const KernelCosetLatticeCondition &c : dependent)
    if (!check(c, std::nullopt))
      return false;
  return true;
}

struct ExactColumn {
  unsigned col = 0;
  std::uint64_t predictedRows = 0;
  /// a + b for the largest lower and upper coefficients: the new entries are at
  /// most this multiple of the largest entry.
  DynamicAPInt growth{1};
};

/// The column among `pending` that has the one-sided unit certificate and can
/// leave the fewest rows, the lowest column on a tie.
inline std::optional<ExactColumn>
chooseExactColumn(const Projected &pr, const std::vector<unsigned> &pending) {
  using namespace kernel_coset_image_detail;
  const DynamicAPInt zero(0), one(1);
  std::optional<ExactColumn> best;
  for (unsigned col : pending) {
    auto pos = positionOf(pr, col);
    if (!pos)
      continue;
    DynamicAPInt lower(0), upper(0);
    for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r) {
      const DynamicAPInt a = pr.P.atIneq(r, *pos);
      lower = std::max(lower, a);
      upper = std::max(upper, -a);
    }
    if (lower > one && upper > one)
      continue;
    const KernelCosetColumnRows rows =
        kernel_coset_budget_detail::countColumnRows(pr, *pos);
    if (rows.pinning != 0)
      continue;
    auto predicted = predictKernelCosetEliminationRows(rows);
    if (predicted && (!best || *predicted < best->predictedRows))
      best = ExactColumn{col, *predicted, std::max(one, lower + upper)};
  }
  return best;
}

/// What the rows of a system say about one column at known values of the
/// others.
struct ColumnBounds {
  std::optional<DynamicAPInt> lower, upper;
  /// The rational bounds, as numerator over a positive denominator.
  DynamicAPInt lowerNum{0}, lowerDen{1}, upperNum{0}, upperDen{1};
  /// A row without the column fails.
  bool falseRow = false;
};

/// `col` is the column left open, or none when every column has a value.
inline ColumnBounds columnBounds(const Projected &pr,
                                 std::optional<unsigned> col,
                                 llvm::ArrayRef<DynamicAPInt> values) {
  const DynamicAPInt zero(0);
  const unsigned constant = pr.P.getNumCols() - 1;
  ColumnBounds out;
  bool anyLower = false, anyUpper = false;
  // row . values, without the open column, and its coefficient.
  auto split = [&](llvm::ArrayRef<DynamicAPInt> row, DynamicAPInt &a) {
    DynamicAPInt rest = row[constant];
    a = zero;
    for (unsigned p = 0; p < pr.cols.size(); ++p) {
      if (col && pr.cols[p] == *col)
        a = row[p];
      else
        rest += row[p] * values[pr.cols[p]];
    }
    return rest;
  };
  // The equalities of a plan are over the outputs alone.
  for (unsigned r = 0; r < pr.P.getNumEqualities(); ++r) {
    DynamicAPInt a(0);
    if (split(pr.P.getEquality(r), a) != zero)
      out.falseRow = true;
  }
  for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r) {
    DynamicAPInt a(0);
    const DynamicAPInt rest = split(pr.P.getInequality(r), a);
    if (a == zero) {
      if (rest < zero)
        out.falseRow = true;
    } else if (a > zero) { // col >= -rest / a
      const DynamicAPInt num = -rest;
      const DynamicAPInt bound = ceilDiv(num, a);
      if (!out.lower || *out.lower < bound)
        out.lower = bound;
      if (!anyLower || num * out.lowerDen > out.lowerNum * a) {
        out.lowerNum = num;
        out.lowerDen = a;
        anyLower = true;
      }
    } else { // col <= rest / -a
      const DynamicAPInt den = -a;
      const DynamicAPInt bound = floorDiv(rest, den);
      if (!out.upper || bound < *out.upper)
        out.upper = bound;
      if (!anyUpper || rest * out.upperDen < out.upperNum * den) {
        out.upperNum = rest;
        out.upperDen = den;
        anyUpper = true;
      }
    }
  }
  return out;
}

} // namespace kernel_coset_gap_detail

/// Find an integer point of `P`, which must have no symbols or locals. The rows
/// are read first. Otherwise one findIntegerSample runs if the gate and the
/// budget both admit it. As with gatedIsIntegerEmpty, a refusal on shape is not
/// charged.
inline KernelCosetSampleResult
gatedFindIntegerSample(const mlir::presburger::IntegerPolyhedron &P,
                       KernelCosetWorkBudget &budget) {
  using namespace kernel_coset_budget_detail;
  using llvm::DynamicAPInt;
  KernelCosetSampleResult out;
  if (P.getNumSymbolVars() != 0 || P.getNumLocalVars() != 0) {
    out.why = "UNKNOWN: the system carries symbols or locals";
    return out;
  }
  const VariableIntervals intervals = readVariableIntervals(P);
  if (intervals.contradiction) {
    out.verdict = KernelCosetEmptiness::Empty;
    return out;
  }
  const unsigned vars = P.getNumVars();
  const KernelCosetSystemShape shape = measureKernelCosetSystem(P);
  // A variable outside every non-box row takes any value of its interval.
  out.sample.assign(vars, DynamicAPInt(0));
  for (unsigned j = 0; j < vars; ++j) {
    if (intervals.lower[j])
      out.sample[j] = *intervals.lower[j];
    else if (intervals.upper[j])
      out.sample[j] = *intervals.upper[j];
  }
  if (shape.nonBoxRows == 0) {
    out.verdict = KernelCosetEmptiness::NonEmpty;
    return out;
  }
  out.sample.clear();
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
  out.queried = true;
  const IntegerPolyhedron restricted = restrictToCoupled(P, intervals);
  auto point = kernel_coset_gap_detail::rationallyEmpty(restricted)
                   ? std::nullopt
                   : restricted.findIntegerSample();
  if (!point) {
    out.verdict = KernelCosetEmptiness::Empty;
    return out;
  }
  // The coupled variables, in the order restrictToCoupled gives them.
  std::vector<bool> coupled(vars, false);
  auto mark = [&](llvm::ArrayRef<DynamicAPInt> row) {
    unsigned last = 0;
    if (countVariables(row, vars, last) < 2)
      return;
    for (unsigned j = 0; j < vars; ++j)
      if (row[j] != DynamicAPInt(0))
        coupled[j] = true;
  };
  for (unsigned r = 0; r < P.getNumInequalities(); ++r)
    mark(P.getInequality(r));
  for (unsigned r = 0; r < P.getNumEqualities(); ++r)
    mark(P.getEquality(r));
  out.sample.assign(vars, DynamicAPInt(0));
  unsigned next = 0;
  for (unsigned j = 0; j < vars; ++j) {
    if (coupled[j])
      out.sample[j] = (*point)[next++];
    else if (intervals.lower[j])
      out.sample[j] = *intervals.lower[j];
    else if (intervals.upper[j])
      out.sample[j] = *intervals.upper[j];
  }
  out.verdict = KernelCosetEmptiness::NonEmpty;
  return out;
}

/// Build the plan of the statement with physical C rows `Crows` over the source
/// domain `D`, which may carry locals. See the file comment. Nothing here is an
/// approximation: a limit refuses the plan, and a nest with no certified
/// columns is still planned, as class Search.
inline KernelCosetGapPlanResult
buildKernelCosetGapPlan(const IntMatrix &Crows,
                        const mlir::presburger::IntegerPolyhedron &D,
                        unsigned n, KernelCosetWorkBudget &budget) {
  using namespace kernel_coset_gap_detail;
  using namespace kernel_coset_budget_detail;
  using mlir::presburger::PresburgerSpace;
  using Verdict = KernelCosetGapPlanVerdict;

  KernelCosetGapPlanResult out;
  auto stop = [&](Verdict verdict, std::string why) {
    out.verdict = verdict;
    out.why = std::move(why);
    return std::move(out);
  };
  auto limited = [&] { return stop(Verdict::LimitReached, budget.why()); };

  // ---- What this analysis does not model ---------------------------------
  if (n == 0)
    return stop(Verdict::Unsupported, "UNKNOWN: degenerate source rank");
  if (D.getNumDimVars() != n || D.getNumSymbolVars() != 0)
    return stop(Verdict::Unsupported,
                "UNKNOWN: the source domain carries symbols");
  if (Crows.empty())
    return stop(Verdict::Unsupported, "UNKNOWN: no physical C access");
  for (const IntRow &row : Crows)
    if (row.size() != n)
      return stop(Verdict::Unsupported, "UNKNOWN: access row arity");
  if (budget.exhausted())
    return limited();

  // ---- The reduced domain -------------------------------------------------
  auto admit = [&](KernelCosetLimit limit, const std::string &what) {
    return limit == KernelCosetLimit::None ||
           budget.refuse(limit, "limit reached: " +
                                    std::string(limitName(limit)) + " of " +
                                    what);
  };
  auto admitSystem = [&](const IntegerPolyhedron &P, const std::string &what) {
    const KernelCosetSystemShape shape = measureKernelCosetSystem(P);
    return admit(budget.checkSystem(shape.rows, shape.columns,
                                    shape.largestEntry),
                 what);
  };
  if (!admit(budget.checkMatrix(measureKernelCosetMatrix(Crows, n)),
             "the C matrix") ||
      !admitSystem(D, "the source domain"))
    return limited();
  auto kb = buildIntegerKernelBasis(Crows, n);
  if (!kb)
    return stop(Verdict::Unsupported,
                "UNKNOWN: integer kernel basis (overflow or malformed C rows)");
  if (kb->outputRank == 0)
    return stop(Verdict::Unsupported, "UNKNOWN: constant physical C (rank 0)");
  if (kb->kernelRank == 0)
    return stop(Verdict::Unsupported,
                "UNKNOWN: no reduction coordinate (kappa == 0)");
  auto reduced = buildReducedDomain(kb->U, D, n);
  if (!reduced)
    return stop(Verdict::Unsupported, "UNKNOWN: reduced domain (i64 overflow)");
  if (!admitSystem(*reduced, "the reduced domain"))
    return limited();

  std::vector<unsigned> outputCols, kernelCols = kb->kernelColIndices;
  {
    std::vector<bool> isKernel(n, false);
    for (unsigned c : kernelCols)
      isKernel[c] = true;
    for (unsigned j = 0; j < n; ++j)
      if (!isKernel[j])
        outputCols.push_back(j);
  }
  const unsigned rho = outputCols.size(), locals = D.getNumLocalVars();
  const unsigned constant = n + locals;
  // The existential coordinates: kernel columns, then locals.
  std::vector<unsigned> existential = kernelCols;
  for (unsigned l = 0; l < locals; ++l)
    existential.push_back(n + l);
  const unsigned e = existential.size();
  const DynamicAPInt zero(0);

  // ---- Step 1: the lattice of the equalities ------------------------------
  std::vector<unsigned> kernelEqualities;
  IntMatrix Erows;
  for (unsigned r = 0; r < reduced->getNumEqualities(); ++r) {
    llvm::ArrayRef<DynamicAPInt> row = reduced->getEquality(r);
    IntRow out64;
    bool touches = false;
    for (unsigned c : existential) {
      touches = touches || row[c] != zero;
      auto v = kernel_coset_detail::fromAPInt(row[c]);
      if (!v)
        return stop(Verdict::Unsupported, "UNKNOWN: equality past 64 bits");
      out64.push_back(*v);
    }
    if (touches) {
      kernelEqualities.push_back(r);
      Erows.push_back(std::move(out64));
    }
  }
  APMatrix lattice(e, std::vector<DynamicAPInt>(e, zero));
  std::vector<unsigned> pivotCols, freeCols;
  std::vector<KernelCosetLatticeCondition> pivots, dependent;
  if (Erows.empty()) {
    for (unsigned j = 0; j < e; ++j) {
      lattice[j][j] = DynamicAPInt(1);
      freeCols.push_back(j);
    }
  } else {
    if (!admit(budget.checkMatrix(measureKernelCosetMatrix(Erows, e)),
               "the kernel equalities"))
      return limited();
    auto basis = buildIntegerKernelBasis(Erows, e);
    if (!basis || !toAPMatrix(basis->U, lattice))
      return stop(Verdict::Unsupported,
                  "UNKNOWN: lattice of the kernel equalities");
    freeCols = basis->kernelColIndices;
    for (unsigned j = 0; j < e; ++j)
      if (std::find(freeCols.begin(), freeCols.end(), j) == freeCols.end())
        pivotCols.push_back(j);
    APMatrix E;
    if (!toAPMatrix(Erows, E))
      return stop(Verdict::Unsupported,
                  "UNKNOWN: lattice of the kernel equalities");
    APMatrix H(E.size(), std::vector<DynamicAPInt>(e, zero));
    for (unsigned i = 0; i < E.size(); ++i)
      for (unsigned c = 0; c < e; ++c)
        for (unsigned j = 0; j < e; ++j)
          H[i][c] += E[i][j] * lattice[j][c];
    for (unsigned c : freeCols)
      for (unsigned i = 0; i < E.size(); ++i)
        if (H[i][c] != zero)
          return stop(Verdict::Unsupported,
                      "UNKNOWN: a free column is not in the lattice");
    if (!solveLattice(H, pivotCols, pivots, dependent))
      return stop(Verdict::Unsupported,
                  "UNKNOWN: the equalities did not solve triangularly");
    // v(y) = nu . g(y), with g(y) = -(alpha . y + gamma) on each equality.
    auto fillForm = [&](KernelCosetLatticeCondition &c) {
      c.form.assign(rho + 1, zero);
      for (unsigned i = 0; i < kernelEqualities.size(); ++i) {
        llvm::ArrayRef<DynamicAPInt> row =
            reduced->getEquality(kernelEqualities[i]);
        for (unsigned j = 0; j < rho; ++j)
          c.form[j] -= c.combination[i] * row[outputCols[j]];
        c.form[rho] -= c.combination[i] * row[constant];
      }
    };
    for (KernelCosetLatticeCondition &c : pivots)
      fillForm(c);
    for (KernelCosetLatticeCondition &c : dependent)
      fillForm(c);
  }

  // ---- The rows over (y, s, w) --------------------------------------------
  const unsigned rank = pivotCols.size(), freeCount = freeCols.size();
  const unsigned columns = rho + rank + freeCount;
  IntegerPolyhedron W(PresburgerSpace::getSetSpace(columns, 0, 0));
  auto compose = [&](llvm::ArrayRef<DynamicAPInt> row) {
    llvm::SmallVector<DynamicAPInt, 16> composed(columns + 1, zero);
    for (unsigned j = 0; j < rho; ++j)
      composed[j] = row[outputCols[j]];
    auto coefficient = [&](unsigned c) {
      DynamicAPInt total(0);
      for (unsigned j = 0; j < e; ++j)
        total += row[existential[j]] * lattice[j][c];
      return total;
    };
    for (unsigned k = 0; k < rank; ++k)
      composed[rho + k] = coefficient(pivotCols[k]);
    for (unsigned j = 0; j < freeCount; ++j)
      composed[rho + rank + j] = coefficient(freeCols[j]);
    composed[columns] = row[constant];
    return composed;
  };
  for (unsigned r = 0; r < reduced->getNumInequalities(); ++r)
    W.addInequality(compose(reduced->getInequality(r)));
  for (unsigned r = 0; r < reduced->getNumEqualities(); ++r)
    if (std::find(kernelEqualities.begin(), kernelEqualities.end(), r) ==
        kernelEqualities.end())
      W.addEquality(compose(reduced->getEquality(r)));
  if (!admitSystem(W, "the lattice system"))
    return limited();

  // ---- Step 2: exact eliminations, cheapest first -------------------------
  // A reduction runs only when a step would otherwise exceed a limit. With no
  // certified column left the plan is complete: class Search, not a refusal.
  Projected pr{std::move(W), std::vector<unsigned>(columns)};
  for (unsigned j = 0; j < columns; ++j)
    pr.cols[j] = j;
  pr.P.removeTrivialRedundancy();
  std::vector<unsigned> pending;
  for (unsigned j = 0; j < freeCount; ++j)
    pending.push_back(rho + rank + j);
  std::vector<KernelCosetGapLevel> levels;
  bool isReduced = false;
  while (pending.size() > 1) {
    const auto choice = chooseExactColumn(pr, pending);
    if (!choice)
      break;
    const DynamicAPInt nextEntry =
        choice->growth * measureKernelCosetSystem(pr.P).largestEntry;
    const KernelCosetLimit blocked = budget.checkSystem(
        choice->predictedRows, pr.P.getNumCols() - 1, nextEntry);
    if (blocked != KernelCosetLimit::None) {
      if (!isReduced) {
        if (!budget.startReduction(measureKernelCosetSystem(pr.P)))
          return limited();
        reduceRows(pr.P);
        isReduced = true;
        continue;
      }
      budget.refuse(blocked, "limit reached: " +
                                 std::string(limitName(blocked)) +
                                 " of an exact elimination (predicted " +
                                 std::to_string(choice->predictedRows) +
                                 " rows)");
      return limited();
    }
    if (!budget.startElimination(choice->predictedRows))
      return limited();
    levels.push_back(KernelCosetGapLevel{choice->col, pr});
    auto pos = kernel_coset_image_detail::positionOf(pr, choice->col);
    pr.P.projectOut(*pos, 1);
    pr.cols.erase(pr.cols.begin() + *pos);
    if (pr.P.getNumLocalVars() != 0)
      return stop(Verdict::Unsupported,
                  "UNKNOWN: the projection introduced locals");
    isReduced = false;
    pending.erase(std::find(pending.begin(), pending.end(), choice->col));
    const KernelCosetSystemShape after = measureKernelCosetSystem(pr.P);
    if (after.rows > choice->predictedRows || after.largestEntry > nextEntry)
      return stop(Verdict::Unsupported,
                  "UNKNOWN: an elimination exceeded its prediction");
  }

  KernelCosetGapPlan plan(D, *reduced, std::move(pr));
  plan.numLocals = locals;
  plan.U = std::move(kb->U);
  plan.Uinv = std::move(kb->Uinv);
  plan.outputCols = std::move(outputCols);
  plan.kernelCols = std::move(kernelCols);
  plan.kernelEqualities = std::move(kernelEqualities);
  plan.lattice = std::move(lattice);
  plan.pivotCols = std::move(pivotCols);
  plan.freeCols = std::move(freeCols);
  plan.pivots = std::move(pivots);
  plan.dependent = std::move(dependent);
  plan.levels = std::move(levels);
  plan.remaining = std::move(pending);
  plan.gapClass = plan.remaining.empty()       ? KernelCosetGapClass::Pinned
                  : plan.remaining.size() == 1 ? KernelCosetGapClass::Interval
                                               : KernelCosetGapClass::Search;
  out.plan = std::move(plan);
  return stop(Verdict::Ready, std::string());
}

/// Is the integer output `y`, one value per output coordinate, in P? See the
/// file comment. Unknown when the exact answer cannot be established, never a
/// guess.
inline KernelCosetCandidateResult
certifyKernelCosetCandidate(const KernelCosetGapPlan &plan,
                            llvm::ArrayRef<llvm::DynamicAPInt> y,
                            KernelCosetWorkBudget &budget) {
  using namespace kernel_coset_gap_detail;
  using mlir::presburger::PresburgerSpace;
  KernelCosetCandidateResult out;
  const unsigned rho = plan.outputCols.size();
  const unsigned rank = plan.pivotCols.size();
  const unsigned columns = rho + rank + plan.freeCols.size();
  if (y.size() != rho) {
    out.why = "UNKNOWN: candidate arity";
    return out;
  }
  const DynamicAPInt zero(0), one(1);
  auto notMember = [&](KernelCosetGapProof proof, std::string why) {
    out.membership = KernelCosetMembership::NotMember;
    out.proof = proof;
    out.why = std::move(why);
    return std::move(out);
  };
  auto unknown = [&](std::string why) {
    out.why = std::move(why);
    return std::move(out);
  };

  // ---- Step 1: the congruences --------------------------------------------
  std::vector<DynamicAPInt> values(columns, zero);
  std::copy(y.begin(), y.end(), values.begin());
  for (unsigned k = 0; k < rank; ++k) {
    const KernelCosetLatticeCondition &c = plan.pivots[k];
    const DynamicAPInt v = evaluateForm(c.form, y);
    if (v % c.modulus != zero) {
      out.combination = c.combination;
      out.modulus = c.modulus;
      return notMember(KernelCosetGapProof::Congruence,
                       "no integer solution of the kernel equalities: the "
                       "modulus does not divide this output");
    }
    values[rho + k] = v / c.modulus;
  }
  for (const KernelCosetLatticeCondition &c : plan.dependent)
    if (evaluateForm(c.form, y) != zero) {
      out.combination = c.combination;
      return notMember(KernelCosetGapProof::Inconsistent,
                       "the kernel equalities disagree at this output");
    }

  // ---- Steps 3 and 4: the columns left ------------------------------------
  if (plan.gapClass == KernelCosetGapClass::Search) {
    const unsigned left = plan.remaining.size();
    const Projected &pr = plan.system;
    mlir::presburger::IntegerPolyhedron fiber(
        PresburgerSpace::getSetSpace(left, 0, 0));
    std::vector<std::optional<unsigned>> leftIndex(columns);
    for (unsigned i = 0; i < left; ++i)
      leftIndex[plan.remaining[i]] = i;
    const unsigned constant = pr.P.getNumCols() - 1;
    auto concrete = [&](llvm::ArrayRef<DynamicAPInt> row) {
      llvm::SmallVector<DynamicAPInt, 8> c(left + 1, zero);
      c[left] = row[constant];
      for (unsigned p = 0; p < pr.cols.size(); ++p) {
        if (leftIndex[pr.cols[p]])
          c[*leftIndex[pr.cols[p]]] = row[p];
        else
          c[left] += row[p] * values[pr.cols[p]];
      }
      return c;
    };
    for (unsigned r = 0; r < pr.P.getNumInequalities(); ++r)
      fiber.addInequality(concrete(pr.P.getInequality(r)));
    for (unsigned r = 0; r < pr.P.getNumEqualities(); ++r)
      fiber.addEquality(concrete(pr.P.getEquality(r)));
    KernelCosetSampleResult found = gatedFindIntegerSample(fiber, budget);
    if (found.verdict == KernelCosetEmptiness::Unknown)
      return unknown(found.why);
    if (found.verdict == KernelCosetEmptiness::Empty)
      return notMember(found.queried ? KernelCosetGapProof::LibraryEmpty
                                     : KernelCosetGapProof::ConcreteConflict,
                       found.queried
                           ? "findIntegerSample found no point in the fiber"
                           : "the fiber fails on inspection");
    for (unsigned i = 0; i < left; ++i)
      values[plan.remaining[i]] = found.sample[i];
  } else {
    const std::optional<unsigned> last =
        plan.remaining.empty() ? std::nullopt
                               : std::optional<unsigned>(plan.remaining[0]);
    const ColumnBounds bounds = columnBounds(plan.system, last, values);
    if (bounds.falseRow)
      return notMember(KernelCosetGapProof::FalseRow,
                       "a row without a free coordinate fails at this output");
    if (bounds.lower && bounds.upper && *bounds.upper < *bounds.lower) {
      out.lower = *bounds.lower;
      out.upper = *bounds.upper;
      out.rationalGap = bounds.lowerNum * bounds.upperDen <=
                        bounds.upperNum * bounds.lowerDen;
      return notMember(KernelCosetGapProof::EmptyInterval,
                       "no integer value lies between the last bounds");
    }
    if (last) {
      values[*last] = bounds.lower   ? *bounds.lower
                      : bounds.upper ? *bounds.upper
                                     : zero;
      if (plan.numLocals == 0 && plan.levels.empty() && bounds.lower &&
          bounds.upper)
        out.multiplicity = *bounds.upper - *bounds.lower + one;
    } else {
      out.multiplicity = one;
    }
  }

  // ---- The witness --------------------------------------------------------
  // Each level has an integer value because its elimination was exact, and the
  // substitution below rejects any point for which that failed.
  for (auto level = plan.levels.rbegin(); level != plan.levels.rend();
       ++level) {
    const ColumnBounds bounds = columnBounds(level->system, level->col, values);
    values[level->col] = bounds.lower   ? *bounds.lower
                         : bounds.upper ? *bounds.upper
                                        : zero;
  }
  const unsigned e = plan.lattice.size();
  std::vector<DynamicAPInt> z(e, zero);
  for (unsigned i = 0; i < e; ++i) {
    for (unsigned k = 0; k < rank; ++k)
      z[i] += plan.lattice[i][plan.pivotCols[k]] * values[rho + k];
    for (unsigned j = 0; j < plan.freeCols.size(); ++j)
      z[i] += plan.lattice[i][plan.freeCols[j]] * values[rho + rank + j];
  }
  const unsigned n = plan.U.size();
  std::vector<DynamicAPInt> point(n + plan.numLocals, zero);
  for (unsigned i = 0; i < rho; ++i)
    point[plan.outputCols[i]] = y[i];
  for (unsigned i = 0; i < plan.kernelCols.size(); ++i)
    point[plan.kernelCols[i]] = z[i];
  for (unsigned l = 0; l < plan.numLocals; ++l)
    point[n + l] = z[plan.kernelCols.size() + l];
  std::vector<DynamicAPInt> source(n + plan.numLocals, zero);
  for (unsigned l = 0; l < plan.numLocals; ++l)
    source[n + l] = point[n + l];
  for (unsigned i = 0; i < n; ++i)
    for (unsigned j = 0; j < n; ++j) {
      auto u = kernel_coset_detail::toAPInt(plan.U[i][j]);
      if (!u)
        return unknown("UNKNOWN: a coordinate change past 64 bits");
      source[i] += *u * point[j];
    }
  if (!plan.reduced.containsPoint(point) ||
      !plan.source.containsPoint(source)) {
    out.multiplicity.reset();
    return unknown("UNKNOWN: a witness failed its substitution");
  }
  out.membership = KernelCosetMembership::Member;
  out.proof = KernelCosetGapProof::Witness;
  out.kernelWitness.assign(z.begin(), z.begin() + plan.kernelCols.size());
  out.localWitness.assign(z.begin() + plan.kernelCols.size(), z.end());
  source.resize(n);
  out.sourceWitness = std::move(source);
  return out;
}

} // namespace systolic::logical
