//===- Provenance.h - What was built, and what actually ran -----*- C++ -*-===//
//
// Two different kinds of claim, kept apart on purpose.
//
// A BuildDeclaration is what the build recorded about itself. It is fixed at
// compile time, so it proves nothing about what a process did. Any field that
// could not be verified when it was recorded reads UNKNOWN.
//
// RuntimeEvidence is what this process did. Each counter is incremented from
// inside the stage it counts, so it can only move if that code ran. A printed
// name cannot move it, and neither can reading the declaration. The
// ExecutionReceipt in Decide.h goes further and binds one query's input to its
// output.
//
// This library is project-owned, and it is not what the compiler uses. Until a
// cutover milestone says otherwise, production decides through LLVM upstream,
// and BuildDeclaration says so.
//
// Project-owned does not mean free of LLVM. The arithmetic is LLVM's, as Int.h
// explains, and BuildDeclaration says that too.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_EXACT_PROVENANCE_H
#define SYSTOLIC_EXACT_PROVENANCE_H

#include <cstdint>
#include <string>

namespace systolic::exact {

enum class SolverImplementation { ProjectOwned, LlvmUpstream, Other };

enum class TreeState { Clean, Dirty, Unknown };

/// Whether the LLVM this was built against carries local changes.
enum class LlvmPatchStatus {
  /// The source tree is at the built revision, with no local change in
  /// llvmPatchScope. It says nothing about files outside that scope, or about a
  /// library built from a tree that has since been restored.
  NoneVerified,
  /// The source tree is at the built revision, with local changes in scope.
  Patched,
  /// There is no source tree, or it is at another revision, so what was built
  /// cannot be told from it.
  Unknown,
};

struct BuildDeclaration {
  SolverImplementation implementation;
  /// The solver production uses. Unchanged by this library existing.
  SolverImplementation productionSolver;
  /// The milestone this library implements.
  std::string solverRevision;
  /// SHA-256 of the text `sha256sum`, in sorted order, prints for
  /// include/Systolic/Exact/* and lib/Systolic/Exact/*, by repository path.
  std::string solverSourceSha256;
  /// What this library takes from LLVM at link time. ARITHMETIC_ONLY means
  /// llvm::DynamicAPInt and the APInt code under it, and no solver.
  std::string llvmDependency;

  std::string projectCommit;
  TreeState projectTree;
  /// SHA-256 of `git diff`.
  std::string projectDiffSha256;

  std::string llvmRevision;
  std::string llvmVersion;
  LlvmPatchStatus llvmPatchStatus;
  std::string llvmPatchScope;
  std::string llvmSourceDir;

  std::string compilerId;
  std::string compilerVersion;
  std::string buildType;
  /// SHA-256 of the C++ flags of this build type.
  std::string compileFlagsSha256;
  /// Whether this library was compiled with assertions.
  bool assertionsEnabled;
};

/// The same object for the life of the process.
const BuildDeclaration &buildDeclaration();

/// Counts since the process started. Thread safe.
struct RuntimeEvidence {
  std::uint64_t decideCalls = 0;
  std::uint64_t latticeRuns = 0;
  std::uint64_t normalizeRuns = 0;
  std::uint64_t checkerRuns = 0;
  std::uint64_t checkerRefusals = 0;
  /// Calls of Session::check(), the project-owned rational solver.
  std::uint64_t rationalChecks = 0;
  /// Pivots of the project-owned tableau.
  std::uint64_t tableauPivots = 0;
};

RuntimeEvidence runtimeEvidence();

/// Both, as JSON with a fixed key order. They are separate objects, named
/// compile_time_declaration and runtime_evidence.
std::string provenanceJson();

const char *implementationName(SolverImplementation implementation);
const char *treeStateName(TreeState state);
const char *llvmPatchStatusName(LlvmPatchStatus status);

} // namespace systolic::exact

#endif // SYSTOLIC_EXACT_PROVENANCE_H
