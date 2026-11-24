//===- Provenance.cpp - What was built, and what actually ran -------------===//

#include "Systolic/Exact/Provenance.h"
#include "ExecutionCounters.h"

// Generated at build time by GenerateIdentity.cmake.
#include "SolverIdentity.inc"

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/VCSRevision.h"

#include <atomic>

#ifndef LLVM_REVISION
#define LLVM_REVISION "UNKNOWN"
#endif

namespace systolic::exact {

namespace {
std::atomic<std::uint64_t> decideCalls{0};
std::atomic<std::uint64_t> latticeRuns{0};
std::atomic<std::uint64_t> normalizeRuns{0};
std::atomic<std::uint64_t> checkerRuns{0};
std::atomic<std::uint64_t> checkerRefusals{0};
std::atomic<std::uint64_t> rationalChecks{0};
std::atomic<std::uint64_t> tableauPivots{0};
std::atomic<std::uint64_t> integerChecks{0};
std::atomic<std::uint64_t> branchNodes{0};
} // namespace

void detail::noteExecution(detail::Stage stage) {
  switch (stage) {
  case detail::Stage::Decide:
    decideCalls.fetch_add(1);
    break;
  case detail::Stage::Lattice:
    latticeRuns.fetch_add(1);
    break;
  case detail::Stage::Normalize:
    normalizeRuns.fetch_add(1);
    break;
  case detail::Stage::Check:
    checkerRuns.fetch_add(1);
    break;
  case detail::Stage::CheckRefused:
    checkerRefusals.fetch_add(1);
    break;
  case detail::Stage::RationalCheck:
    rationalChecks.fetch_add(1);
    break;
  case detail::Stage::Pivot:
    tableauPivots.fetch_add(1);
    break;
  case detail::Stage::IntegerCheck:
    integerChecks.fetch_add(1);
    break;
  case detail::Stage::BranchNode:
    branchNodes.fetch_add(1);
    break;
  }
}

const BuildDeclaration &buildDeclaration() {
  static const BuildDeclaration declaration = {
      SolverImplementation::ProjectOwned,
      SolverImplementation::LlvmUpstream,
      "S2",
      "S3",
      SYSTOLIC_EXACT_SOURCE_SHA256,
      "ARITHMETIC_ONLY",
      SYSTOLIC_EXACT_PROJECT_COMMIT,
      TreeState::SYSTOLIC_EXACT_PROJECT_TREE,
      SYSTOLIC_EXACT_PROJECT_DIFF_SHA256,
      LLVM_REVISION,
      LLVM_VERSION_STRING,
      LlvmPatchStatus::SYSTOLIC_EXACT_LLVM_PATCH_STATUS,
      SYSTOLIC_EXACT_LLVM_PATCH_SCOPE,
      SYSTOLIC_EXACT_LLVM_SOURCE_DIR,
      SYSTOLIC_EXACT_COMPILER_ID,
      SYSTOLIC_EXACT_COMPILER_VERSION,
      SYSTOLIC_EXACT_BUILD_TYPE,
      SYSTOLIC_EXACT_FLAGS_SHA256,
#ifdef NDEBUG
      false,
#else
      true,
#endif
  };
  return declaration;
}

RuntimeEvidence runtimeEvidence() {
  RuntimeEvidence evidence;
  evidence.decideCalls = decideCalls.load();
  evidence.latticeRuns = latticeRuns.load();
  evidence.normalizeRuns = normalizeRuns.load();
  evidence.checkerRuns = checkerRuns.load();
  evidence.checkerRefusals = checkerRefusals.load();
  evidence.rationalChecks = rationalChecks.load();
  evidence.tableauPivots = tableauPivots.load();
  evidence.integerChecks = integerChecks.load();
  evidence.branchNodes = branchNodes.load();
  return evidence;
}

const char *implementationName(SolverImplementation implementation) {
  switch (implementation) {
  case SolverImplementation::ProjectOwned:
    return "PROJECT_OWNED";
  case SolverImplementation::LlvmUpstream:
    return "LLVM_UPSTREAM";
  case SolverImplementation::Other:
    return "OTHER";
  }
  return "INVALID";
}

const char *treeStateName(TreeState state) {
  switch (state) {
  case TreeState::Clean:
    return "CLEAN";
  case TreeState::Dirty:
    return "DIRTY";
  case TreeState::Unknown:
    return "UNKNOWN";
  }
  return "INVALID";
}

const char *llvmPatchStatusName(LlvmPatchStatus status) {
  switch (status) {
  case LlvmPatchStatus::NoneVerified:
    return "NONE_VERIFIED";
  case LlvmPatchStatus::Patched:
    return "PATCHED";
  case LlvmPatchStatus::Unknown:
    return "UNKNOWN";
  }
  return "INVALID";
}

/// A JSON string. Control characters do not occur in these fields, but a path
/// may hold a quote or a backslash.
static std::string quoted(const std::string &text) {
  std::string out = "\"";
  for (char c : text) {
    if (c == '"' || c == '\\')
      out += '\\';
    out += c;
  }
  out += '"';
  return out;
}

std::string provenanceJson() {
  const BuildDeclaration &d = buildDeclaration();
  const RuntimeEvidence e = runtimeEvidence();
  std::string json = "{\n";
  json += "  \"schema\": \"systolic-exact-provenance-1\",\n";
  json += "  \"compile_time_declaration\": {\n";
  json += "    \"solver\": {\"implementation\": " +
          quoted(implementationName(d.implementation)) +
          ", \"revision\": " + quoted(d.solverRevision) +
          ", \"integer_revision\": " + quoted(d.integerRevision) +
          ", \"source_sha256\": " + quoted(d.solverSourceSha256) +
          ", \"llvm_dependency\": " + quoted(d.llvmDependency) + "},\n";
  json += "    \"production\": {\"solver\": " +
          quoted(implementationName(d.productionSolver)) + "},\n";
  json += "    \"project\": {\"commit\": " + quoted(d.projectCommit) +
          ", \"tree\": " + quoted(treeStateName(d.projectTree)) +
          ", \"diff_sha256\": " + quoted(d.projectDiffSha256) + "},\n";
  json += "    \"llvm\": {\"revision\": " + quoted(d.llvmRevision) +
          ", \"version\": " + quoted(d.llvmVersion) +
          ", \"patch_status\": " + quoted(llvmPatchStatusName(d.llvmPatchStatus)) +
          ", \"patch_scope\": " + quoted(d.llvmPatchScope) +
          ", \"source_dir\": " + quoted(d.llvmSourceDir) + "},\n";
  json += "    \"build\": {\"compiler_id\": " + quoted(d.compilerId) +
          ", \"compiler_version\": " + quoted(d.compilerVersion) +
          ", \"build_type\": " + quoted(d.buildType) +
          ", \"flags_sha256\": " + quoted(d.compileFlagsSha256) +
          ", \"assertions\": " + (d.assertionsEnabled ? "true" : "false") +
          "}\n";
  json += "  },\n";
  json += "  \"runtime_evidence\": {\"decide_calls\": " +
          std::to_string(e.decideCalls) +
          ", \"lattice_runs\": " + std::to_string(e.latticeRuns) +
          ", \"normalize_runs\": " + std::to_string(e.normalizeRuns) +
          ", \"checker_runs\": " + std::to_string(e.checkerRuns) +
          ", \"checker_refusals\": " + std::to_string(e.checkerRefusals) +
          ", \"rational_checks\": " + std::to_string(e.rationalChecks) +
          ", \"tableau_pivots\": " + std::to_string(e.tableauPivots) +
          ", \"integer_checks\": " + std::to_string(e.integerChecks) +
          ", \"branch_nodes\": " + std::to_string(e.branchNodes) +
          "}\n";
  json += "}\n";
  return json;
}

} // namespace systolic::exact
