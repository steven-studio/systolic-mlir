//===- Session.h - Incremental rational feasibility -------------*- C++ -*-===//
//
// Decides whether rows over rational variables have a common point, with the
// tableau of Tableau.h, and keeps that work between queries. This is opt-in: it
// is separate from decide() in Decide.h, which it does not change, and nothing
// in the compiler calls it.
//
// It answers over the rationals only. A rational point says nothing about
// integer points, so Feasible here is no claim about the integers. The integer
// search is planned and does not exist yet. A congruence has no rational
// meaning, so a system holding one answers Unknown with UnsupportedConstruct.
//
// Contract of check():
//   Feasible    the Decision holds a RationalWitness, accepted by the checker
//               against the rows now in force, strict ones included.
//   Infeasible  the Decision holds a proof, accepted by the checker: a sum of
//               those rows with positive multipliers, the signs of equalities
//               chosen freely, that is a false row. That is a Farkas
//               certificate.
//   Unknown     nothing is asserted, and why() says why.
//
// Both definite answers pass through the checker on every call, also when they
// are answered from the cache. A refused answer is Unknown with
// CertificateRejected and is not kept.
//
// Transactions. Each operation either happens or leaves the state as it was:
//   - A refused row changes nothing.
//   - check() works on a copy and keeps it only when it reaches an answer the
//     checker accepted. A limit, a cancellation or a rejected certificate
//     leaves the session exactly as it was, so a query can stop in the middle
//     of a pivot and be retried with a larger budget.
//   - pop() restores the state at the matching push() exactly: the rows, the
//     bounds, the basis, the tableau, the assignment, the status, the witness
//     and the proof. A scope keeps that state, so there is no undo log to get
//     wrong, and stateText() is byte-identical afterwards.
//
// Cost. A push() costs nothing until the next change, which copies the state
// once. So a session of depth d holds up to d + 1 copies of the state, and when
// each scope adds rows, that grows with the square of d. check() adds up what
// the session holds, with the copy it is about to make, and past maxSessionCells
// it answers Unknown with TableauLimit and changes nothing. A pop() frees what
// its scope held, so a retry can then succeed. An answer already known needs no
// copy and is still given. Only check() looks at limits: a caller that adds rows
// and scopes and never queries is limited by nothing here.
//
// Termination. check() ends: the pivot rule cannot run forever, and the meter
// bounds the work besides. What the default limits do not promise is a short
// run. The step limit is 2^32 cell operations, and simplex with Bland's rule
// takes exponentially many pivots on some inputs, so a caller that needs a
// deadline must lower it.
//
// A Session is not thread safe. It holds no reference to its arguments.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_SESSION_H
#define SYSTOLIC_EXACT_SESSION_H

#include "Systolic/Exact/Decide.h"
#include "Systolic/Exact/Tableau.h"

#include <memory>

namespace systolic::exact {

class Session {
public:
  /// No rows, over `numVariables` variables of kind Problem.
  explicit Session(unsigned numVariables);
  /// Starts from the rows of `system`.
  explicit Session(System system);
  ~Session();

  Session(const Session &) = delete;
  Session &operator=(const Session &) = delete;

  [[nodiscard]] RowError addInequality(LinearExpr expr);       // expr >= 0
  [[nodiscard]] RowError addStrictInequality(LinearExpr expr); // expr > 0
  [[nodiscard]] RowError addEquality(LinearExpr expr);         // expr == 0

  /// Opens a scope.
  void push();
  /// Closes the newest scope and restores the state at its push(). False, with
  /// nothing changed, when no scope is open.
  [[nodiscard]] bool pop();
  /// The number of open scopes.
  std::size_t depth() const { return scopes.size(); }
  /// What this session holds now, over all open scopes, in units of about 30
  /// bytes. Scopes that share a state count it once.
  std::uint64_t heldCells() const;

  /// `cancellation` may be null, and otherwise must outlive the call.
  Decision check(const WorkLimits &limits = WorkLimits(),
                 CancellationToken *cancellation = nullptr);

  /// The rows in force.
  const System &system() const;
  /// The basis, bounds and assignment, for inspection. It never changes an
  /// answer.
  const BoundsTableau &tableau() const;
  /// A text form of the whole state. Equal states give equal text.
  std::string stateText() const;

private:
  struct State;
  State &writable();

  std::shared_ptr<State> current;
  std::vector<std::shared_ptr<State>> scopes;
};

/// Decides `system` over the rationals, with a Session of its own.
Decision solveRational(const System &system,
                       const WorkLimits &limits = WorkLimits(),
                       CancellationToken *cancellation = nullptr);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_SESSION_H
