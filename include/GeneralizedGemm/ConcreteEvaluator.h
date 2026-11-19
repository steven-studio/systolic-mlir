//===- ConcreteEvaluator.h - Exact execution of a model ---------*- C++ -*-===//
//
// Deterministic execution from one entry state. A Returned result proves
// termination for that entry state only. A repeated full configuration proves
// divergence for that entry state only. Neither says anything about another
// parameter value. Out of fuel proves nothing.
//
// Floating point is evaluated strictly as written, IEEE-754 with
// round-to-nearest. Under reassoc or contract flags, this is one allowed
// behaviour, not the whole set, and RunResult reports it.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_CONCRETEEVALUATOR_H
#define GENERALIZEDGEMM_CONCRETEEVALUATOR_H

#include "GeneralizedGemm/SemanticProgram.h"

#include "llvm/ADT/DenseMap.h"

#include <optional>
#include <string>
#include <vector>

namespace mlir::generalized_gemm {

/// Dense row-major contents of one root. A missing cell is uninitialized.
struct RootData {
  llvm::SmallVector<uint64_t> shape;
  std::vector<std::optional<llvm::APInt>> cells;
};

struct EntryState {
  std::vector<llvm::APInt> parameters; // Parallel to program.parameters.
  std::vector<RootData> memory;        // Parallel to program.roots.
};

struct Configuration {
  unsigned location = 0;
  std::vector<std::optional<llvm::APInt>> state;
  std::vector<bool> shadow;
  std::vector<RootData> memory;
};

enum class RunStatus {
  Returned, UndefinedBehavior, Underspecified, ModelLimit, OutOfFuel,
  Diverged, InvalidEntry,
};
const char *stringifyRunStatus(RunStatus status);

struct MacEvent {
  unsigned site = 0;
  uint64_t step = 0;
  std::vector<llvm::APInt> coordinates;
  std::vector<llvm::APInt> f, g, h, hRead;
  llvm::APInt aValue, bValue;
  std::optional<llvm::APInt> accumulatedValue;
  llvm::APInt updateValue;
};

struct RunLimits {
  uint64_t fuel = 1u << 20;
  uint64_t maxCells = 1u << 20;
  // Configurations remembered for exact cycle detection. Zero disables it.
  uint64_t maxRecordedConfigurations = 4096;
};

struct RunResult {
  RunStatus status = RunStatus::InvalidEntry;
  std::string reason;
  uint64_t steps = 0;
  std::vector<MacEvent> events; // In execution order, with repeats.
  std::vector<llvm::APInt> returns;
  std::vector<bool> returnPoison;
  std::vector<RootData> memory;
  bool strictRefinementOfPermissiveFloat = false;
};

/// The initial configuration, or nullopt with `error` set.
std::optional<Configuration> initialConfiguration(const SemanticProgram &program,
                                                  const EntryState &entry,
                                                  const RunLimits &limits,
                                                  std::string &error);

/// One transition. On a non-None status the configuration is unchanged.
struct StepOutcome {
  std::optional<RunStatus> stopped;
  std::string reason;
  unsigned transition = 0;
  std::vector<MacEvent> events;
};
StepOutcome stepConfiguration(const SemanticProgram &program,
                              const EntryState &entry, Configuration &config,
                              std::vector<llvm::APInt> *returns = nullptr,
                              std::vector<bool> *returnPoison = nullptr);

RunResult runConcrete(const SemanticProgram &program, const EntryState &entry,
                      const RunLimits &limits = {});

/// Evidence for I8: ExactEnumerated for this parameter point when the run
/// returned; Unknown otherwise.
DomainEvidence domainEvidenceFromRun(const SemanticProgram &program,
                                     const EntryState &entry,
                                     const RunResult &run);

/// Evaluates a BV or Float term in a configuration. For cross-checks.
/// Nullopt when it reads an undefined state or an uninitialized cell.
std::optional<llvm::APInt> evaluateTerm(const SemanticProgram &program,
                                        const EntryState &entry,
                                        const Configuration &config, Term term);

} // namespace mlir::generalized_gemm
#endif
