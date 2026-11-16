//===- NestReport.cpp - One record per loop nest -------------------------===//
//
// Stage 1 of docs/GENERALIZED_GEMM_SCOPE.md. Prints a JSON array with one object per
// function: its roots, the order of its statements, how its nests meet on memory, and
// one record per outermost loop. Each nest is analysed on a private copy, by the
// importer and the checkers the domain driver uses, from any memory at its entry.
// Read-only: the input is not changed, and no record authorizes a lowering.
//
// Names given to --authorize and arguments given to --scope are the function's: roots
// as the importer numbers them in the whole function, and argument numbers.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/NestRecord.h"
#include "GeneralizedGemm/Z3Backend.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::generalized_gemm;
namespace cl = llvm::cl;

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<std::string> scopeText(
    "scope", cl::init(""), cl::desc("argument:lower:upper,... over the function's arguments, as signed decimals"));
static cl::opt<std::string> authorize(
    "authorize", cl::init(""), cl::desc("comma separated separation names over the function's roots"));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(10000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(20000000));
static cl::opt<uint64_t> maxPoints("max-points", cl::init(256));
static cl::opt<unsigned> maxLevel("max-level", cl::init(3));
static cl::opt<unsigned> maxCandidates("max-candidates", cl::init(5));
static cl::opt<unsigned> maxRounds("max-rounds", cl::init(300));
static cl::opt<uint64_t> referenceFill("reference-fill", cl::init(0));
static cl::opt<uint64_t> maxBox("max-box", cl::init(1u << 20));
static cl::opt<unsigned> candidatesPerGroup("candidates-per-group", cl::init(2));
static cl::opt<uint64_t> replaySteps("replay-steps", cl::init(1u << 22));
static cl::opt<uint64_t> replayNodes("replay-nodes", cl::init(1u << 23));
static cl::opt<unsigned> maxQueries("max-queries", cl::init(4000));
static cl::opt<bool> outlineOnly(
    "outline-only", cl::init(false),
    cl::desc("print each nest's copy, or why it has none, and analyse nothing"));

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "Stage 1: one record per loop nest\n");
  DialectRegistry registry;
  registry.insert<affine::AffineDialect, arith::ArithDialect, cf::ControlFlowDialect, func::FuncDialect,
                  LLVM::LLVMDialect, memref::MemRefDialect, scf::SCFDialect>();
  MLIRContext context(registry);
  context.allowUnregisteredDialects();
  context.loadAllAvailableDialects();
  OwningOpRef<ModuleOp> module = parseSourceFile<ModuleOp>(input, &context);
  if (!module)
    return 2;

  NestAnalysisOptions options;
  options.indexBitWidth = indexWidth;
  options.timeoutMs = timeoutMs;
  options.resourceLimit = resourceLimit;
  options.maxPoints = maxPoints;
  options.maxBox = maxBox;
  options.replaySteps = replaySteps;
  options.replayNodes = replayNodes;
  options.referenceFill = referenceFill;
  options.maxLevel = maxLevel;
  options.maxCandidates = maxCandidates;
  options.maxRounds = maxRounds;
  options.candidatesPerGroup = candidatesPerGroup;
  options.maxQueries = maxQueries;
  {
    llvm::SmallVector<llvm::StringRef> items;
    llvm::StringRef(scopeText).split(items, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef item : items) {
      llvm::SmallVector<llvm::StringRef> parts;
      item.split(parts, ':');
      NestScopeEntry entry;
      if (parts.size() != 3 || parts[0].getAsInteger(10, entry.argument) ||
          parts[1].getAsInteger(10, entry.lower) || parts[2].getAsInteger(10, entry.upper)) {
        llvm::errs() << "malformed --scope\n";
        return 2;
      }
      options.scope.push_back(entry);
    }
  }
  Authorizations authorized;
  {
    llvm::SmallVector<llvm::StringRef> names;
    llvm::StringRef(authorize).split(names, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef name : names)
      authorized.insert(name.str());
  }

  Z3Backend backend;
  llvm::json::Array functions;
  for (auto function : module->getOps<func::FuncOp>()) {
    if (function.isExternal())
      continue;
    if (!onlyFunction.empty() && function.getSymName() != onlyFunction)
      continue;
    FunctionOutline outline = outlineNests(function, authorized);
    if (outlineOnly) {
      llvm::outs() << "// function " << outline.function;
      if (!outline.unsupported.empty())
        llvm::outs() << ": not outlined: " << outline.unsupported;
      llvm::outs() << "\n";
      for (const OutlinedNest &nest : outline.nests) {
        llvm::outs() << "// nest " << nest.ordinal << " (" << nest.loop << ", statement " << nest.statement << ")";
        if (!nest.unsupported.empty()) {
          llvm::outs() << ": no copy: " << nest.unsupported << "\n";
          continue;
        }
        llvm::outs() << "\n";
        nest.module.get().print(llvm::outs());
      }
      continue;
    }
    std::vector<NestRecord> records;
    for (const OutlinedNest &nest : outline.nests)
      records.push_back(analyzeNest(nest, options, backend));
    functions.push_back(toJSON(outline, records));
  }
  if (!outlineOnly)
    llvm::outs() << llvm::formatv("{0:2}", llvm::json::Value(std::move(functions))) << "\n";
  return 0;
}
