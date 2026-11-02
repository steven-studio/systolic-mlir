//===- ExecutionCounters.h - Evidence that the code really ran --*- C++ -*-===//
//
// Private to lib/Systolic/Exact. Each stage calls noteExecution from inside its
// own code, so a count can only come from the stage having run.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_EXECUTIONCOUNTERS_H
#define SYSTOLIC_EXACT_EXECUTIONCOUNTERS_H

namespace systolic::exact::detail {

enum class Stage {
  Decide,
  Lattice,
  Normalize,
  Check,
  CheckRefused,
  RationalCheck,
  Pivot,
};

void noteExecution(Stage stage);

} // namespace systolic::exact::detail

#endif // SYSTOLIC_EXACT_EXECUTIONCOUNTERS_H
