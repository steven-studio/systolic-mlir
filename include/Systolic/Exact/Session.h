//===- Session.h - Incremental rational feasibility -------------*- C++ -*-===//
//
// Decides whether rows have a common point, and keeps that work between queries.
// This is opt-in: it is separate from decide() in Decide.h, which it does not
// change, and nothing in the compiler calls it.
//
// There are two questions, and neither says anything about the other:
//
//   check()         Is there a rational point? This runs the tableau of
//                   Tableau.h. A congruence has no rational meaning, so a system
//                   holding one answers Unknown with UnsupportedConstruct.
//   checkInteger()  Is there an integer point? This runs the search of
//                   IntegerSearch.h, which reads every row, congruences too. An
//                   existential variable is an ordinary coordinate here, so a
//                   witness gives it a value as well.
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
// Contract of checkInteger():
//   Feasible    the Decision holds an IntegerWitness, one value per variable,
//               accepted by the checker against the rows now in force.
//   Infeasible  the Decision holds a BranchProof, accepted by the checker.
//   Unknown     nothing is asserted, and why() says why. A limit gives this, and
//               so would a certificate the checker refused.
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
// copy and is still given. A row is refused with SessionLimit, before anything
// is copied, when it would pass the limit given to setMemoryLimit(), and the
// session stays as it was. A push() itself allocates one pointer and is not
// limited. checkInteger() holds its own work apart from the session, under
// maxBranchNodes and maxSearchCells, and frees it before it returns, apart from
// the answer it keeps. It reads what the session holds first, and past
// maxSessionCells it answers Unknown with MemoryLimit. The proof it keeps can
// take the session past that limit by what one search may hold, and the next row
// is then refused.
//
// Termination. check() ends: the pivot rule cannot run forever, and the meter
// bounds the work besides. What the default limits do not promise is a short
// run. The step limit is 2^32 cell operations, and simplex with Bland's rule
// takes exponentially many pivots on some inputs, so a caller that needs a
// deadline must lower it.
//
// checkInteger() ends too, on any system: its cases form a finite tree, by the
// argument in IntegerSearch.h, and the limits bound it besides. The tree can be
// exponential in the input, and walking a form with a huge range takes a step per
// value, so the same advice holds. It has no lattice basis reduction, so a long
// thin set in a skewed basis can end in Unknown with NodeLimit. All the limits
// count work, so none of this is a bound in seconds. Nor do the limits cover the
// checker, which runs after the search, on a proof whose size they did bound. It
// also runs on an answer from the cache, which no limit stops.
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
  /// expr == 0 (mod modulus), for checkInteger().
  [[nodiscard]] RowError addCongruence(LinearExpr expr, Int modulus);

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
  /// The most that adding a row may leave this session holding, in those units.
  void setMemoryLimit(std::uint64_t cells) { memoryLimit = cells; }

  /// `cancellation` may be null, and otherwise must outlive the call.
  Decision check(const WorkLimits &limits = WorkLimits(),
                 CancellationToken *cancellation = nullptr);
  Decision checkInteger(const WorkLimits &limits = WorkLimits(),
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
  bool hasRoomForRow() const;

  std::shared_ptr<State> current;
  std::vector<std::shared_ptr<State>> scopes;
  std::uint64_t memoryLimit = WorkLimits().maxSessionCells;
};

/// Decides `system` over the rationals, with a Session of its own.
Decision solveRational(const System &system,
                       const WorkLimits &limits = WorkLimits(),
                       CancellationToken *cancellation = nullptr);

/// Decides `system` over the integers, with a Session of its own.
Decision solveInteger(const System &system,
                      const WorkLimits &limits = WorkLimits(),
                      CancellationToken *cancellation = nullptr);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_SESSION_H
