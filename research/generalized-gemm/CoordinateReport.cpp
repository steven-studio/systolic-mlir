//===- CoordinateReport.cpp - G3a driver ----------------------------------===//
//
// Prints a JSON array with one result per function, or per table with --table.
// Read-only. The termination proposers run here, and the enumerator re-checks
// their certificate: what they say is not taken on trust.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/CoordinateAnalysis.h"
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
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::generalized_gemm;
namespace cl = llvm::cl;
namespace json = llvm::json;

static cl::opt<std::string> input(cl::Positional, cl::init("-"));
static cl::opt<bool> tableMode("table", cl::init(false),
                               cl::desc("the input is a JSON array of event tables"));
static cl::opt<std::string> onlyFunction("function", cl::init(""));
static cl::opt<unsigned> indexWidth("index-bit-width", cl::init(0));
static cl::opt<bool> lowerStructured("lower-structured", cl::init(false));
static cl::opt<std::string> scopeText(
    "scope", cl::init(""), cl::desc("parameter:lower:upper,... as signed decimals"));
static cl::opt<unsigned> timeoutMs("timeout-ms", cl::init(10000));
static cl::opt<unsigned> resourceLimit("rlimit", cl::init(20000000));
static cl::opt<uint64_t> maxPoints("max-points", cl::init(4096));
static cl::opt<uint64_t> maxEvents("max-events", cl::init(1u << 22));
static cl::opt<uint64_t> fuel("fuel", cl::init(1u << 24));
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

static IndexTuple readTuple(const json::Array *array) {
  IndexTuple out;
  if (array)
    for (const json::Value &value : *array)
      out.push_back(llvm::APInt(64, (uint64_t)value.getAsInteger().value_or(0)));
  return out;
}

static std::vector<uint32_t> readLabels(const json::Object *object, const char *key) {
  std::vector<uint32_t> out;
  if (const json::Array *array = object->getArray(key))
    for (const json::Value &value : *array)
      out.push_back((uint32_t)value.getAsInteger().value_or(0));
  return out;
}

static int runTables() {
  auto buffer = llvm::MemoryBuffer::getFileOrSTDIN(input);
  if (!buffer) {
    llvm::errs() << "cannot read the input\n";
    return 2;
  }
  auto parsed = json::parse((*buffer)->getBuffer());
  const json::Array *tables = parsed ? parsed->getAsArray() : nullptr;
  if (!tables) {
    llvm::errs() << "the input is not a JSON array\n";
    llvm::consumeError(parsed.takeError());
    return 2;
  }
  bool first = true;
  llvm::outs() << "[\n";
  for (const json::Value &value : *tables) {
    const json::Object *object = value.getAsObject();
    const json::Array *events = object ? object->getArray("events") : nullptr;
    if (!events) {
      llvm::errs() << "a table has no events\n";
      return 2;
    }
    AccessTable table;
    table.coordinatesUniform = true;
    uint64_t step = 0;
    for (const json::Value &item : *events) {
      const json::Object *event = item.getAsObject();
      IndexTuple x = readTuple(event->getArray("x")), h = readTuple(event->getArray("h"));
      const json::Array *read = event->getArray("h_read");
      table.add((uint32_t)event->getInteger("site").value_or(0), step++, x, h,
                read ? readTuple(read) : h, readTuple(event->getArray("f")),
                readTuple(event->getArray("g")));
      if (table.coordinateNames.size() != x.size()) {
        table.coordinateNames.resize(x.size());
        for (unsigned n = 0; n < x.size(); ++n)
          table.coordinateNames[n] = n;
      }
    }
    CoordinateCandidate candidate;
    const json::Object *given = object->getObject("candidate");
    if (given) {
      candidate.p = readLabels(given, "p");
      candidate.i = readLabels(given, "i");
      candidate.j = readLabels(given, "j");
      candidate.k = readLabels(given, "k");
      candidate.origin = "given";
    }
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    llvm::outs() << tableToJSON(table, given ? &candidate : nullptr);
  }
  llvm::outs() << "\n]\n";
  return 0;
}

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "G3a coordinate recovery\n");
  if (tableMode)
    return runTables();
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
    EnumerationOptions options;
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
    options.limits.maxEvents = maxEvents;
    options.limits.fuel = fuel;
    options.referenceFill = referenceFill;
    std::optional<TerminationCertificate> certificate;
    if (!noCertificate && program.isSupported()) {
      auto hints = polynomialRankHints(program, function, importOptions);
      certificate = analyzeTermination(program, backend, termination, hints).certificate;
    }
    CoordinateResult result = recoverCoordinates(
        program, certificate ? &*certificate : nullptr, options, backend);
    if (!first)
      llvm::outs() << ",\n";
    first = false;
    llvm::outs() << toJSON(program, result, printEvents);
  }
  llvm::outs() << "\n]\n";
  return 0;
}
