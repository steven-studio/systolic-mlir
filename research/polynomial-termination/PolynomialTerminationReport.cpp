#include "Systolic/MLIRPolynomialTermination.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/Parser/Parser.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/JSON.h"
#include "llvm/Support/raw_ostream.h"

using namespace systolic::polynomial;
static llvm::cl::opt<std::string> input(llvm::cl::Positional,llvm::cl::init("-"));
static llvm::cl::opt<std::string> function("function",llvm::cl::init(""));
static llvm::cl::list<std::string> ranges("parameter-range");
static llvm::cl::opt<unsigned> timeout("timeout-ms",llvm::cl::init(2000));
static llvm::cl::opt<unsigned> rlimit("solver-resource-limit",llvm::cl::init(5000000));
static llvm::cl::opt<unsigned> queries("max-queries",llvm::cl::init(16));
static llvm::cl::opt<bool> printIR("print-ir",llvm::cl::init(false));
static std::string snapshot(mlir::Operation *op) {
  std::string text; llvm::raw_string_ostream stream(text);
  op->print(stream,mlir::OpPrintingFlags().printGenericOpForm().enableDebugInfo()); return text;
}
int main(int argc, char **argv) {
  llvm::cl::ParseCommandLineOptions(argc,argv,"Read-only polynomial CFG termination proofs\n");
  try {
    termination::Options options;
    options.limits.timeoutMilliseconds=timeout; options.limits.solverResourceLimit=rlimit; options.limits.maxQueries=queries;
    for (const auto &range : ranges) {
      auto a=range.find(':'),b=range.find(':',a==std::string::npos?0:a+1);
      auto index=range.substr(0,a);
      if (a==std::string::npos || b==std::string::npos || index.empty() || index.find_first_not_of("0123456789")!=std::string::npos)
        throw std::invalid_argument("expected argument:lower:upper");
      unsigned long n=std::stoul(index);
      if(n>4096 || !options.parameterRanges.emplace(static_cast<unsigned>(n),std::make_pair(Integer(range.substr(a+1,b-a-1)),Integer(range.substr(b+1)))).second)
        throw std::invalid_argument("duplicate or excessive parameter index");
    }
    mlir::DialectRegistry registry;
    registry.insert<mlir::arith::ArithDialect,mlir::cf::ControlFlowDialect,mlir::func::FuncDialect,mlir::memref::MemRefDialect,mlir::scf::SCFDialect>();
    mlir::MLIRContext context(registry); context.allowUnregisteredDialects();
    auto module=mlir::parseSourceFile<mlir::ModuleOp>(input,&context);
    if(!module) throw std::invalid_argument("MLIR parsing/verification failed");
    auto before=snapshot(module.get());
    llvm::json::Array functions;
    for(auto f:module->getOps<mlir::func::FuncOp>()) {
      if(!function.empty() && f.getSymName()!=function.getValue()) continue;
      auto r=termination::analyze(f,options);
      std::string report=termination::formatResult(r);
      auto parsed=llvm::json::parse(report);
      if(!parsed) throw std::logic_error("invalid internal JSON");
      llvm::json::Object item; item["function"]=f.getSymName().str(); item["analysis"]=std::move(*parsed);
      functions.push_back(std::move(item));
    }
    if(functions.empty()) throw std::invalid_argument("no matching function");
    if(snapshot(module.get())!=before) throw std::logic_error("analysis modified input IR");
    if(printIR) { module->print(llvm::outs()); llvm::outs()<<'\n'; }
    else {
      llvm::json::Object report; report["read_only"]=true; report["functions"]=std::move(functions);
      llvm::outs()<<llvm::json::Value(std::move(report))<<'\n';
    }
    return 0;
  } catch(const std::exception &e) {
    llvm::json::Object error; error["status"]="UNKNOWN"; error["reason"]=e.what(); error["certificate"]=nullptr;
    llvm::outs()<<llvm::json::Value(std::move(error))<<'\n'; return 1;
  }
}
