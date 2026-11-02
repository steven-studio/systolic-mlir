//===- Budget.cpp - Deterministic work limits and cancellation ------------===//

#include "Systolic/Exact/Budget.h"

namespace systolic::exact {

void CancellationToken::cancelAfterPolls(std::uint64_t count) {
  remaining.store(count);
  counting.store(true);
}

bool CancellationToken::poll() {
  polls.fetch_add(1);
  if (requested.load())
    return true;
  if (!counting.load())
    return false;
  if (remaining.load() == 0) {
    requested.store(true);
    return true;
  }
  remaining.fetch_sub(1);
  return false;
}

bool WorkMeter::charge(std::uint64_t units) {
  if (stopped())
    return false;
  if (cancellation && cancellation->poll()) {
    stop(UnknownReason::Cancelled);
    return false;
  }
  // A limit near 2^64 must not wrap past itself.
  if (units > limits.maxSteps - used) {
    used = limits.maxSteps;
    stop(UnknownReason::StepLimit);
    return false;
  }
  used += units;
  return true;
}

bool WorkMeter::admit(const Int &value) {
  if (stopped())
    return false;
  if (bitLength(value) > limits.maxCoefficientBits) {
    stop(UnknownReason::BitLimit);
    return false;
  }
  return true;
}

void WorkMeter::stop(UnknownReason why) {
  if (!stopped())
    stopReason = why;
}

} // namespace systolic::exact
