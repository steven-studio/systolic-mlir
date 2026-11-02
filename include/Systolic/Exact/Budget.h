//===- Budget.h - Deterministic work limits and cancellation ----*- C++ -*-===//
//
// Every limit is counted in work the solver does, never in time, so the same
// system under the same limits stops at the same point on every machine. A
// stop only ever produces Verdict::Unknown: no limit can change a Feasible or
// Infeasible answer into the other.
//
// Lifetime: a CancellationToken belongs to the caller and must outlive the
// query it is passed to. A WorkMeter borrows it and lives for one query.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_BUDGET_H
#define SYSTOLIC_EXACT_BUDGET_H

#include "Systolic/Exact/Int.h"
#include "Systolic/Exact/Verdict.h"

#include <atomic>
#include <cstdint>

namespace systolic::exact {

/// What one query may use. The defaults are far above every system seen in
/// the real query corpus and exist to bound adversarial input.
struct WorkLimits {
  /// Exact integer cell operations, as charged to the WorkMeter.
  std::uint64_t maxSteps = std::uint64_t(1) << 32;
  /// Rows of the system, counting all kinds together.
  std::uint64_t maxRows = 1 << 16;
  std::uint64_t maxVariables = 1 << 12;
  /// The largest bit length of an input value or a computed matrix entry.
  unsigned maxCoefficientBits = 1 << 16;
  /// Rows times columns of a tableau, which is what one Session query holds.
  std::uint64_t maxTableauCells = std::uint64_t(1) << 24;
  /// What a Session may hold across all its open scopes, from Session::
  /// heldCells(). The default is about 2 GB.
  std::uint64_t maxSessionCells = std::uint64_t(1) << 26;
};

/// A cooperative request to stop, polled once per charge to a WorkMeter. A
/// query can only see it between steps, and it then answers Unknown.
///
/// requestCancellation may be called from another thread, which is the one
/// non-deterministic input to a query; cancelAfterPolls is deterministic.
class CancellationToken {
public:
  CancellationToken() = default;
  CancellationToken(const CancellationToken &) = delete;
  CancellationToken &operator=(const CancellationToken &) = delete;

  void requestCancellation() { requested.store(true); }
  /// Cancels at the poll after `polls` polls have returned false.
  void cancelAfterPolls(std::uint64_t polls);

  /// Called by the solver. True once cancellation has been requested.
  bool poll();
  std::uint64_t pollCount() const { return polls.load(); }

private:
  std::atomic<bool> requested{false};
  std::atomic<bool> counting{false};
  std::atomic<std::uint64_t> remaining{0};
  std::atomic<std::uint64_t> polls{0};
};

/// The work of one query. The first limit reached stops it for good: every
/// later charge fails, and reason() keeps naming the first cause.
class WorkMeter {
public:
  explicit WorkMeter(const WorkLimits &limits,
                     CancellationToken *cancellation = nullptr)
      : limits(limits), cancellation(cancellation) {}

  /// Adds `units` steps. False when the query must stop.
  [[nodiscard]] bool charge(std::uint64_t units);
  /// Checks a computed value against the coefficient limit. False when the
  /// query must stop.
  [[nodiscard]] bool admit(const Int &value);
  /// Records a stop for a reason other than a charge, such as a row limit.
  void stop(UnknownReason why);

  bool stopped() const { return stopReason != UnknownReason::None; }
  UnknownReason reason() const { return stopReason; }
  std::uint64_t steps() const { return used; }
  const WorkLimits &workLimits() const { return limits; }

private:
  WorkLimits limits;
  CancellationToken *cancellation;
  std::uint64_t used = 0;
  UnknownReason stopReason = UnknownReason::None;
};

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_BUDGET_H
