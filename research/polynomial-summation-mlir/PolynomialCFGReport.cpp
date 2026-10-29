#include "Systolic/MLIRPolynomialRecurrence.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/AsmState.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

using namespace systolic::polynomial;

static llvm::cl::opt<std::string> input(llvm::cl::Positional, llvm::cl::init("-"),
                                       llvm::cl::desc("MLIR input"));
static llvm::cl::opt<std::string> mode("mode", llvm::cl::init("modular"));
static llvm::cl::opt<std::string> horizon("horizon", llvm::cl::init(""));
static llvm::cl::opt<std::string> assumption("assume-no-overflow", llvm::cl::init(""));
static llvm::cl::opt<std::string> selectedFunction("function", llvm::cl::init(""));
static llvm::cl::opt<std::string> evaluateAt("evaluate", llvm::cl::init(""));
static llvm::cl::opt<bool> printIR("print-ir", llvm::cl::init(false));
static llvm::cl::opt<bool> symbolic("symbolic", llvm::cl::init(false));
static llvm::cl::list<std::string> parameterRanges("parameter-range",
    llvm::cl::desc("P2a argument:lower:upper (signed representatives)"));
static llvm::cl::list<std::string> parameterValues("parameter-value",
    llvm::cl::desc("P2a parameter:value for --evaluate"));

static std::string snapshot(mlir::Operation *op) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  op->print(stream, mlir::OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  return text;
}

int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc, argv, "Read-only MLIR polynomial recurrence analysis\n");
  try {
    cfg::Options options;
    if (mode == "modular") options.mode = IntegerMode::Modular;
    else if (mode == "signed") options.mode = IntegerMode::SignedNoOverflow;
    else if (mode == "mathematical") options.mode = IntegerMode::Mathematical;
    else throw std::invalid_argument("mode must be modular, signed, or mathematical");
    if (!horizon.empty()) options.maxIterations = Integer(horizon.getValue());
    options.noOverflowAssumption = assumption;
    cfg::SymbolicOptions symbolicOptions;
    symbolicOptions.integers = options;
    SymbolicPolynomial::Assignment assignments;
    if (!symbolic && (!parameterRanges.empty() || !parameterValues.empty()))
      throw std::invalid_argument("parameter options require --symbolic");
    for (const std::string &range : parameterRanges) {
      auto a = range.find(':'), b = range.find(':', a == std::string::npos ? 0 : a+1);
      if (a == std::string::npos || b == std::string::npos)
        throw std::invalid_argument("expected argument:lower:upper");
      std::string index = range.substr(0,a);
      if (index.empty() || index.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument("invalid parameter argument number");
      unsigned long number = std::stoul(index);
      if (number > 4096 || !symbolicOptions.parameterRanges.emplace(static_cast<unsigned>(number),
          std::make_pair(Integer(range.substr(a+1,b-a-1)), Integer(range.substr(b+1)))).second)
        throw std::invalid_argument("duplicate or excessive parameter argument number");
    }
    for (const std::string &binding : parameterValues) {
      auto colon = binding.find(':');
      if (colon == std::string::npos ||
          !assignments.emplace(binding.substr(0,colon), Integer(binding.substr(colon+1))).second)
        throw std::invalid_argument("expected unique parameter:value");
    }
    mlir::DialectRegistry registry;
    registry.insert<mlir::arith::ArithDialect, mlir::cf::ControlFlowDialect,
                    mlir::func::FuncDialect, mlir::memref::MemRefDialect, mlir::scf::SCFDialect>();
    mlir::MLIRContext context(registry);
    context.allowUnregisteredDialects();
    auto module = mlir::parseSourceFile<mlir::ModuleOp>(input, &context);
    if (!module) {
      llvm::outs() << "{\"status\":\"UNKNOWN\",\"reason\":\"MLIR parsing/verification failed\",\"states\":[]}\n";
      return 1;
    }
    std::string before = snapshot(module.get());
    llvm::json::Array reports;
    for (auto function : module->getOps<mlir::func::FuncOp>()) {
      if (!selectedFunction.empty() && function.getSymName() != selectedFunction.getValue())
        continue;
      std::string report;
      llvm::json::Array values;
      bool evaluated = false;
      if (symbolic) {
        auto result = cfg::analyzeSymbolic(function, symbolicOptions);
        report = cfg::formatResult(result);
        if (report != cfg::formatResult(cfg::analyzeSymbolic(function, symbolicOptions)))
          throw std::logic_error("symbolic analysis report is not deterministic");
        if (!evaluateAt.empty() && result.status != Status::Unknown) {
          for (const auto &state : result.states)
            values.push_back(result.valueAt(state.name, Integer(evaluateAt.getValue()), assignments).convert_to<std::string>());
          evaluated = true;
        }
      } else {
        auto result = cfg::analyze(function, options);
        report = cfg::formatResult(result);
        if (report != cfg::formatResult(cfg::analyze(function, options)))
          throw std::logic_error("analysis report is not deterministic");
        if (!evaluateAt.empty() && result.status != Status::Unknown) {
          for (const auto &state : result.states)
            values.push_back(result.solution.valueAt(state.name, Integer(evaluateAt.getValue())).convert_to<std::string>());
          evaluated = true;
        }
      }
      auto parsed = llvm::json::parse(report);
      if (!parsed) throw std::logic_error("internal JSON report is invalid");
      llvm::json::Object item;
      item["function"] = function.getSymName().str();
      item["analysis"] = std::move(*parsed);
      if (evaluated) {
        Integer t(evaluateAt.getValue());
        item["header_iteration"] = t.convert_to<std::string>();
        item["values"] = std::move(values);
      }
      reports.push_back(std::move(item));
    }
    if (snapshot(module.get()) != before)
      throw std::logic_error("analysis modified input MLIR");
    if (reports.empty()) throw std::invalid_argument("no matching func.func");
    if (printIR) {
      module->print(llvm::outs());
      llvm::outs() << '\n';
    } else {
      llvm::json::Object report;
      report["read_only"] = true;
      report["deterministic"] = true;
      report["functions"] = std::move(reports);
      llvm::outs() << llvm::json::Value(std::move(report)) << '\n';
    }
    return 0;
  } catch (const std::exception &error) {
    llvm::json::Object failure;
    failure["status"] = "UNKNOWN";
    failure["reason"] = error.what();
    failure["states"] = llvm::json::Array();
    llvm::outs() << llvm::json::Value(std::move(failure)) << '\n';
    return 1;
  }
}
