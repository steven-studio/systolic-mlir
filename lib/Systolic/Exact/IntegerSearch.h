//===- IntegerSearch.h - Exact integer feasibility by cases -----*- C++ -*-===//
//
// Private to lib/Systolic/Exact. Session::checkInteger() is the public way in.
//
// The search walks a tree of cases. At each node it knows the input rows and the
// rows the path assumes, and it does this:
//
//   1. Solves the equalities over the integers, with one quotient column per
//      congruence, and rewrites every row over the free coordinates y.
//   2. Normalizes the rows, which tightens each by its gcd. Two rows that meet
//      at one value are a new equality, so it goes back to 1.
//   3. Solves the rational relaxation with solveRational(). A Farkas proof over
//      the tightened rows closes the node.
//   4. Takes the rational point if it is integral, or rounds it if the rounded
//      point keeps every row.
//   5. Otherwise splits on an integer form, and walks both cases.
//
// Which form to split on is what decides whether the search ends. It uses forms
// that are bounded on the polyhedron P. A two-sided row is bounded. For the rest,
// let C = { d : a_i . d >= 0 } be the recession cone of P. A row that vanishes on
// all of C has a finite range on P, because P is a polytope plus C, and one
// rational query finds such a row whenever there is one. So is any form in the
// span of bounded rows, which is how a coordinate of the lattice can be known to
// be bounded.
//
// The choice among bounded forms decides how fast it ends. A step of one in a
// form with huge coefficients is a tiny step in space, so splitting on such a row
// only peels slivers off a vertex, and a coordinate can have a huge range in a
// skewed basis. The search takes, in this order:
//
//     a two-sided row at a fraction, when its range is small: a flat direction;
//     a bounded coordinate y_j at a fraction, a form of norm 1 on the lattice;
//     a bounded row.
//
// A walk along one form leaves a row of that form per step, and a node reads only
// the tightest, so the cost of a node does not grow with the length of the walk.
//
//   - If there is such a form, the search splits on it. If its value at the
//     rational point is a fraction, the split is around it. If it is an integer,
//     the split leaves that value alone on one side, where the row meets at one
//     value and step 2 removes a dimension. Either way the form's range shrinks,
//     and at a fixed dimension the forms in play are a finite set.
//   - If no row is bounded, C has a point inside every row, so P holds arbitrarily
//     large balls. Moving each row in by half the 1-norm of its coefficients then
//     leaves a nonempty set, and rounding a point of it stays inside P, since
//     rounding moves a . y by at most that much.
//
// So on any system the search ends if its budgets let it: each node either closes,
// finds a point, or splits on a form with a finite integer range, and pinning such
// a form lowers the dimension. How long that takes can be exponential in the input.
//
// Nothing here is trusted. A witness and a proof both go to the checker in
// Checker.h, and the proof is written over the original columns, so the checker
// sees none of the coordinates above. A row over y is a row of the system minus a
// combination of equalities, and because the lattice step is unimodular the gcd
// of that row over the columns is its gcd over y. That is what lets a tightening
// over y be written as a Tighten over the columns.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_INTEGERSEARCH_H
#define SYSTOLIC_EXACT_INTEGERSEARCH_H

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Certificate.h"
#include "Systolic/Exact/System.h"

namespace systolic::exact::detail {

struct SearchOutcome {
  enum class Kind { Feasible, Infeasible, Stopped };
  Kind kind = Kind::Stopped;
  IntegerWitness witness; // Feasible
  BranchProof proof;      // Infeasible
  /// What `proof` holds, in the units of WorkLimits::maxSearchCells.
  std::uint64_t proofCells = 0;
  UnknownReason reason = UnknownReason::None; // Stopped
  std::uint64_t steps = 0;
  std::uint64_t pivots = 0;
  std::uint64_t nodes = 0;
  bool ranTableau = false;
};

/// `system` is read for the length of the call. `cancellation` may be null.
SearchOutcome searchIntegerPoint(const System &system, const WorkLimits &limits,
                                 CancellationToken *cancellation);

} // namespace systolic::exact::detail

#endif // SYSTOLIC_EXACT_INTEGERSEARCH_H
