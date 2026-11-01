//===- TerminationReport.cpp - G2a driver ---------------------------------===//
//
// Prints a JSON array with one termination result per function. Read-only.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/PolynomialAdapter.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/TerminationAnalysis.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::generalized_gemm;
namespace cl = llvm::cl;

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<bool> lowerStructured("lower-structured", cl::init(false));
static cl::opt<std::string> scopeText(
    "scope", cl::init(""),
    cl::desc("parameter:lower:upper,... as signed decimals"));
static cl::opt<bool> polynomialHints("polynomial-hints", cl::init(true),
                                     cl::desc("try P2b candidates as ranks"));
static cl::opt<bool> printScripts("print-scripts", cl::init(false));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(10000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(20000000));
static cl::opt<unsigned> proposerQueries("max-proposer-queries", cl::init(2500));
static cl::opt<unsigned> checkerQueries("max-checker-queries", cl::init(4096));
static cl::opt<unsigned> maxPaths("max-paths", cl::init(512));
static cl::opt<uint64_t> replayFuel("replay-fuel", cl::init(1u << 18));
static cl::opt<std::string> authorize(
    "authorize", cl::init(""), cl::desc("comma separated assumption names"));

static std::optional<ParameterScope> parseScope(const SemanticProgram &program) {
  ParameterScope scope;
  if (scopeText.empty())
    return scope;
  scope.allParameters = false;
  llvm::SmallVector<llvm::StringRef> items;
  llvm::StringRef(scopeText).split(items, ',');
  for (llvm::StringRef item : items) {
    llvm::SmallVector<llvm::StringRef> parts;
    item.split(parts, ':');
    unsigned id;
    if (parts.size() != 3 || parts[0].getAsInteger(10, id) ||
        id >= program.parameters.size())
      return std::nullopt;
    unsigned width = program.parameters[id].sort.width;
    int64_t lower, upper;
    if (parts[1].getAsInteger(10, lower) || parts[2].getAsInteger(10, upper))
      return std::nullopt;
    // A bound the type cannot hold is a mistake, not something to truncate.
    llvm::APInt low(64, lower, /*isSigned=*/true), high(64, upper, /*isSigned=*/true);
    if (!low.isSignedIntN(width) || !high.isSignedIntN(width))
      return std::nullopt;
    scope.ranges.push_back({id, low.trunc(width), high.trunc(width)});
  }
  return scope;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "G2a termination analysis\n");
  DialectRegistry registry;
  registry.insert<affine::AffineDialect, arith::ArithDialect,
                  cf::ControlFlowDialect, func::FuncDialect, LLVM::LLVMDialect,
                  memref::MemRefDialect, scf::SCFDialect>();
  MLIRContext context(registry);
  context.allowUnregisteredDialects();
  context.loadAllAvailableDialects();
  OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(input, &context);
  if (!module)
    return 2;

  ImportOptions importOptions;
  importOptions.indexBitWidth = indexWidth;
  importOptions.lowerStructured = lowerStructured;
  Z3Backend backend;
  bool first = true;
  llvm::outs() << "[\n";
  for (auto function : module->getOps<func::FuncOp>()) {
    if (function.isExternal())
      continue;
    if (!onlyFunction.empty() && function.getSymName() != onlyFunction)
      continue;
    SemanticProgram program = importFunction(function, importOptions);
    TerminationOptions options;
    if (program.isSupported()) {
      auto scope = parseScope(program);
      if (!scope) {
        llvm::errs() << "malformed --scope for " << program.functionName << "\n";
        return 2;
      }
      options.scope = *scope;
    }
    options.limits.solver.timeoutMilliseconds = timeoutMs;
    options.limits.solver.resourceLimit = resourceLimit;
    options.limits.maxProposerQueries = proposerQueries;
    options.limits.maxCheckerQueries = checkerQueries;
    options.limits.paths.maxPaths = maxPaths;
    options.limits.replayFuel = replayFuel;
    llvm::SmallVector<llvm::StringRef> names;
    llvm::StringRef(authorize).split(names, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef name : names)
      options.authorized.insert(name.str());
    std::vector<RankHint> hints;
    if (polynomialHints && program.isSupported())
      hints = polynomialRankHints(program, function, importOptions);
    TerminationResult result = analyzeTermination(program, backend, options, hints);
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    llvm::outs() << toJSON(program, result, printScripts);
  }
  llvm::outs() << "\n]\n";
  return 0;
}
