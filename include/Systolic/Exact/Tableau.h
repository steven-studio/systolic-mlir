//===- Tableau.h - A bounds-based tableau for rational feasibility -*- C++ -*-//
//
// The general simplex of Dutertre and de Moura (2006). Every constraint is a
// bound on a variable, and the tableau itself holds only definitions, so it
// says nothing about feasibility until bounds are asserted against it.
//
// Variables are numbered. The first numStructural() are the variables of the
// problem. Each later one, a slack, is defined as a linear form over them:
// s = d . x, for a primitive d. At every moment each variable is either basic
// or nonbasic, and each basic variable has one row:
//
//   x_b = sum_j a_bj x_j, over the nonbasic x_j.
//
// Invariants, which hold between any two calls unless a call returned Stopped:
//
//   I1  The rows say the same thing as the definitions. Substituting the form
//       of each variable into a row gives an identity over the structural
//       variables.
//   I2  value() satisfies the rows. It is a linear map of the structural
//       values, so each variable's value is its form at those values.
//   I3  Every nonbasic variable is within its bounds. A basic one may not be.
//   I4  Each variable keeps its tightest asserted bound on each side, and a
//       lower bound never exceeds the upper bound on the same variable.
//
// step() restores I3 for basic variables one pivot at a time, by Bland's rule:
// the smallest basic variable out of bounds leaves, and the smallest nonbasic
// variable able to repair it enters. The order is by variable number, not by
// position in the tableau. With these two choices no sequence of pivots is
// infinite, whatever the bounds. The argument is above step() in Tableau.cpp.
//
// When no variable can repair the leaving one, its row is a Farkas combination
// of bounds whose sum is false, and conflict() returns it in terms of the input
// rows. Nothing here is trusted to be right: Session passes every witness and
// every combination to the independent checker.
//
// Limits: every step charges the WorkMeter, so a run stops within its budget.
// A call that returns Stopped may leave the tableau half changed, and the
// tableau must then be discarded. Session always works on a copy for this
// reason.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_TABLEAU_H
#define SYSTOLIC_EXACT_TABLEAU_H

#include "Systolic/Exact/Budget.h"
#include "Systolic/Exact/Certificate.h"
#include "Systolic/Exact/DeltaRational.h"

#include <map>
#include <optional>
#include <utility>

namespace systolic::exact {

/// A bound on a variable and the input row it came from. `scale`, which is
/// positive, makes scale * row equal x - value for a lower bound and value - x
/// for an upper bound, with the row's sign flipped when reason.negated is set.
struct Bound {
  DeltaRational value;
  RowRef reason;
  Rational scale;
};

/// One row of a Farkas combination, with a positive multiplier.
struct FarkasTerm {
  RowRef row;
  Rational multiplier;
};

class BoundsTableau {
public:
  enum class AssertOutcome { Applied, Conflict, Stopped };
  enum class StepOutcome { Feasible, Pivoted, Conflict, Stopped };

  explicit BoundsTableau(unsigned numStructural);

  /// The variable equal to `form`, which must be nonzero, primitive, and have a
  /// positive first nonzero entry. That is a structural variable for a unit
  /// form, and otherwise a slack, created on first use. std::nullopt when the
  /// meter stopped it.
  std::optional<unsigned> variableForForm(const std::vector<Int> &form,
                                          WorkMeter &meter);

  /// Keep the tighter of `bound` and the current bound, and the current one on
  /// a tie. The caller has admitted the row, and a bound is a reduced fraction
  /// of its numbers, so it needs no admission here. Conflict means the two sides
  /// of `variable` crossed, and conflict() explains it.
  AssertOutcome assertLower(unsigned variable, Bound bound, WorkMeter &meter);
  AssertOutcome assertUpper(unsigned variable, Bound bound, WorkMeter &meter);

  /// One round of the main loop. Feasible means I3 holds for every variable.
  StepOutcome step(WorkMeter &meter);

  /// The combination that proves the last Conflict.
  const std::vector<FarkasTerm> &conflict() const { return conflictTerms; }

  /// The structural values, with a positive rational in place of delta that
  /// keeps every bound. Meaningful after step() returns Feasible. std::nullopt
  /// when the meter stopped it.
  std::optional<std::vector<Rational>> concreteValues(WorkMeter &meter) const;

  // A read-only view, which the tests use to check the invariants from outside.
  unsigned numStructural() const { return structural; }
  unsigned numVariables() const { return structural + slackForms.size(); }
  /// The basic variable of each row.
  const std::vector<unsigned> &basicVariables() const { return basic; }
  /// The nonbasic variable of each column.
  const std::vector<unsigned> &nonbasicVariables() const { return nonbasic; }
  const std::vector<std::vector<Rational>> &rows() const { return cells; }
  const DeltaRational &value(unsigned variable) const {
    return values[variable];
  }
  const std::optional<Bound> &lower(unsigned variable) const {
    return lowers[variable];
  }
  const std::optional<Bound> &upper(unsigned variable) const {
    return uppers[variable];
  }
  /// The definition of a variable over the structural variables.
  std::vector<Int> form(unsigned variable) const;
  /// The pivot of the last step() that returned Pivoted: leaving, entering.
  std::optional<std::pair<unsigned, unsigned>> lastPivot() const {
    return latestPivot;
  }
  /// Pivots since this tableau was created.
  std::uint64_t numPivots() const { return pivots; }

  /// A text form of the whole state. Equal states give equal text.
  std::string stateText() const;

private:
  struct Place {
    bool isBasic = false;
    std::size_t position = 0;
  };

  bool isBelowLower(unsigned variable) const;
  bool isAboveUpper(unsigned variable) const;
  bool canIncrease(unsigned variable) const;
  bool canDecrease(unsigned variable) const;

  /// Moves a nonbasic variable to `target` and carries the basic ones with it.
  bool moveNonbasic(unsigned variable, const DeltaRational &target,
                    WorkMeter &meter);
  /// Moves the basic variable of `row` to `target` through `column`, and swaps
  /// the two.
  bool pivotAndUpdate(std::size_t row, std::size_t column,
                      const DeltaRational &target, WorkMeter &meter);
  bool pivot(std::size_t row, std::size_t column, WorkMeter &meter);

  void explainRow(std::size_t row, bool needsIncrease);
  void explainCrossing(const Bound &low, const Bound &high);
  void addTerm(const Bound &bound, const Rational &multiplier);

  unsigned structural;
  /// By slack. A structural variable is a unit form, which is not stored: that
  /// would be n squared integers before any limit had been read.
  std::vector<std::vector<Int>> slackForms;
  std::map<std::vector<Int>, unsigned> known; // slack by form
  std::vector<DeltaRational> values;          // by variable
  std::vector<std::optional<Bound>> lowers;   // by variable
  std::vector<std::optional<Bound>> uppers;   // by variable
  std::vector<Place> places;                  // by variable
  std::vector<unsigned> basic;                // by row
  std::vector<unsigned> nonbasic;             // by column
  std::vector<std::vector<Rational>> cells;   // by row, then column
  std::vector<FarkasTerm> conflictTerms;
  std::optional<std::pair<unsigned, unsigned>> latestPivot;
  std::uint64_t pivots = 0;
};

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_TABLEAU_H
