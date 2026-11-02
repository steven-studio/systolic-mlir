//===- SynthesisReport.cpp - G3b driver -----------------------------------===//
//
// Prints a JSON array with one result per function. Read-only. Candidates are
// proposals until the checkers have spoken, and no result authorizes lowering.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/PolynomialAdapter.h"
#include "GeneralizedGemm/SemanticImport.h"
#include "GeneralizedGemm/SynthesisAnalysis.h"
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
    "scope", cl::init(""), cl::desc("parameter:lower:upper,... as signed decimals"));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(10000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(20000000));
static cl::opt<uint64_t> maxPoints("max-points", cl::init(256));
static cl::opt<unsigned> maxLevel("max-level", cl::init(3));
static cl::opt<int> fixedLevel("grammar-level", cl::init(-1),
                               cl::desc("use only this grammar level"));
static cl::opt<unsigned> maxCandidates("max-candidates", cl::init(5));
static cl::opt<unsigned> maxRounds("max-rounds", cl::init(300));
static cl::opt<uint64_t> referenceFill("reference-fill", cl::init(0));
static cl::opt<bool> printEvents("print-events", cl::init(false));
static cl::opt<bool> noCertificate("no-certificate", cl::init(false),
                                   cl::desc("withhold the certificate, as a test"));
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
    llvm::APInt low(64, lower, /*isSigned=*/true), high(64, upper, /*isSigned=*/true);
    if (!low.isSignedIntN(width) || !high.isSignedIntN(width))
      return std::nullopt;
    scope.ranges.push_back({id, low.trunc(width), high.trunc(width)});
  }
  return scope;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "G3b coordinate synthesis\n");
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
    TerminationOptions termination;
    SynthesisOptions options;
    if (program.isSupported()) {
      auto scope = parseScope(program);
      if (!scope) {
        llvm::errs() << "malformed --scope for " << program.functionName << "\n";
        return 2;
      }
      termination.scope = options.scope = *scope;
    }
    termination.limits.solver.timeoutMilliseconds = timeoutMs;
    termination.limits.solver.resourceLimit = resourceLimit;
    llvm::SmallVector<llvm::StringRef> names;
    llvm::StringRef(authorize).split(names, ',', -1, /*KeepEmpty=*/false);
    for (llvm::StringRef name : names)
      termination.authorized.insert(name.str());
    options.termination = termination.limits;
    options.authorized = termination.authorized;
    options.limits.maxPoints = maxPoints;
    options.maxLevel = maxLevel;
    options.synthesis.maxCandidates = maxCandidates;
    options.synthesis.maxRounds = maxRounds;
    options.synthesis.solver = termination.limits.solver;
    options.referenceFill = referenceFill;
    if (fixedLevel >= 0)
      options.grammar = GrammarSpec::atLevel(fixedLevel);
    std::optional<TerminationCertificate> certificate;
    if (!noCertificate && program.isSupported()) {
      auto hints = polynomialRankHints(program, function, importOptions);
      certificate = analyzeTermination(program, backend, termination, hints).certificate;
    }
    SynthesisResult result = synthesizeProgram(
        program, certificate ? &*certificate : nullptr, options, backend);
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    llvm::outs() << toJSON(program, result, printEvents);
  }
  llvm::outs() << "\n]\n";
  return 0;
}
