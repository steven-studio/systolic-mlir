#include "Systolic/SystolicDialect.h"
#include "Systolic/SystolicOps.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/Linalg/IR/Linalg.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Tensor/IR/Tensor.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/Matchers.h"
#include "mlir/Parser/Parser.h"
#include "mlir/Support/FileUtilities.h"

#include "llvm/Support/CommandLine.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/ToolOutputFile.h"

using namespace mlir;

namespace cl = llvm::cl;

static cl::opt<std::string> inputFilename(cl::Positional,
                                           cl::desc("<input .mlir file>"),
                                           cl::init("-"));
static cl::opt<std::string> outputFilename("o", cl::desc("Output filename"),
                                            cl::init("-"));

namespace {

struct Emitter {
  llvm::raw_ostream &os;
  DenseMap<Value, std::string> names;
  DenseMap<Value, Value> streamSkip;

  explicit Emitter(llvm::raw_ostream &os) : os(os) {}

  Value resolve(Value v) {
    auto it = streamSkip.find(v);
    while (it != streamSkip.end()) {
      v = it->second;
      it = streamSkip.find(v);
    }
    return v;
  }

  std::string nameOf(Value v) {
    v = resolve(v);
    auto it = names.find(v);
    return it != names.end() ? it->second : std::string("<unresolved>");
  }
};

static int64_t getConstantIndex(Value v) {
  llvm::APInt val;
  bool matched = matchPattern(v, m_ConstantInt(&val));
  (void)matched;
  assert(matched && "expand-pe-array-to-mac 产生的迴圈边界应该都是常数");
  return val.getSExtValue();
}

static LogicalResult emitInnerLoop(Emitter &em, scf::ForOp kLoop,
                                    StringRef iName, StringRef jName,
                                    StringRef cName, int indent) {
  int64_t ub = getConstantIndex(kLoop.getUpperBound());
  std::string pad(indent, ' ');
  em.names[kLoop.getInductionVar()] = "k";

  em.os << pad << "for (int k = 0; k < " << ub << "; k++) {\n";
  em.os << pad << "  #pragma HLS PIPELINE II=1\n";

  tensor::ExtractOp aExtract, bExtract;
  systolic::MacOp macOp;
  for (Operation &op : kLoop.getBody()->getOperations()) {
    if (auto e = dyn_cast<tensor::ExtractOp>(op)) {
      if (!aExtract)
        aExtract = e;
      else
        bExtract = e;
      continue;
    }
    if (auto m = dyn_cast<systolic::MacOp>(op)) {
      macOp = m;
      continue;
    }
    if (isa<scf::YieldOp>(op))
      continue;
    return kLoop.emitError("内层迴圈里有 emitter 不认识的 op: ")
           << op.getName();
  }
  if (!aExtract || !bExtract || !macOp)
    return kLoop.emitError(
        "内层迴圈缺少预期的 tensor.extract x2 / systolic.mac");

  std::string aName = em.nameOf(aExtract.getTensor());
  std::string bName = em.nameOf(bExtract.getTensor());

  em.os << pad << "  acc = " << aName << "[" << iName << "][k] * " << bName
        << "[k][" << jName << "] + acc;\n";
  em.os << pad << "}\n";
  return success();
}

static LogicalResult emitMiddleLoop(Emitter &em, scf::ForOp jLoop,
                                     StringRef iName, StringRef cName,
                                     int indent) {
  int64_t ub = getConstantIndex(jLoop.getUpperBound());
  std::string pad(indent, ' ');
  em.names[jLoop.getInductionVar()] = "j";

  em.os << pad << "for (int j = 0; j < " << ub << "; j++) {\n";
  em.os << pad << "  #pragma HLS UNROLL\n";
  em.os << pad << "  float acc = " << cName << "[" << iName << "][j];\n";

  scf::ForOp kLoop;
  for (Operation &op : jLoop.getBody()->getOperations()) {
    if (auto f = dyn_cast<scf::ForOp>(op)) {
      kLoop = f;
      continue;
    }
    if (isa<tensor::ExtractOp, tensor::InsertOp, scf::YieldOp>(op))
      continue;
    return jLoop.emitError("中层迴圈里有 emitter 不认识的 op: ")
           << op.getName();
  }
  if (!kLoop)
    return jLoop.emitError("中层迴圈里找不到内层 scf.for (K)");

  if (failed(emitInnerLoop(em, kLoop, iName, "j", cName, indent + 2)))
    return failure();

  em.os << pad << "  " << cName << "[" << iName << "][j] = acc;\n";
  em.os << pad << "}\n";
  return success();
}

static LogicalResult emitOuterLoop(Emitter &em, scf::ForOp iLoop,
                                    StringRef cName, int indent) {
  int64_t ub = getConstantIndex(iLoop.getUpperBound());
  std::string pad(indent, ' ');
  em.names[iLoop.getInductionVar()] = "i";

  em.os << pad << "for (int i = 0; i < " << ub << "; i++) {\n";
  em.os << pad << "  #pragma HLS UNROLL\n";

  scf::ForOp jLoop;
  for (Operation &op : iLoop.getBody()->getOperations()) {
    if (auto f = dyn_cast<scf::ForOp>(op)) {
      jLoop = f;
      continue;
    }
    if (isa<scf::YieldOp>(op))
      continue;
    return iLoop.emitError("外层迴圈里有 emitter 不认识的 op: ")
           << op.getName();
  }
  if (!jLoop)
    return iLoop.emitError("外层迴圈里找不到中层 scf.for (col)");

  if (failed(emitMiddleLoop(em, jLoop, "i", cName, indent + 2)))
    return failure();

  em.os << pad << "}\n";
  return success();
}

static LogicalResult emitFunction(func::FuncOp func, llvm::raw_ostream &os) {
  Emitter em(os);

  SmallVector<std::string> paramDecls;
  for (BlockArgument arg : func.getArguments()) {
    auto ty = dyn_cast<RankedTensorType>(arg.getType());
    if (!ty || ty.getRank() != 2)
      return func.emitError("目前的 emitter 只支援 rank-2 tensor 参数");
    std::string name = ("arg" + Twine(arg.getArgNumber())).str();
    em.names[arg] = name;
    paramDecls.push_back(("float " + Twine(name) + "[" +
                           Twine(ty.getShape()[0]) + "][" +
                           Twine(ty.getShape()[1]) + "]")
                              .str());
  }

  os << "void " << func.getName() << "(";
  for (size_t i = 0; i < paramDecls.size(); ++i) {
    if (i)
      os << ", ";
    os << paramDecls[i];
  }
  os << ") {\n";

  scf::ForOp outer;
  for (Operation &op : func.getBody().front().getOperations()) {
    if (auto s = dyn_cast<systolic::StreamOp>(op)) {
      em.streamSkip[s.getResult()] = s.getData();
      continue;
    }
    if (auto f = dyn_cast<scf::ForOp>(op)) {
      outer = f;
      continue;
    }
    // scf.for 的迴圈边界常数(c0/c1/cRows 之类),函式最上层本来就会有
    // 这些,直接跳过,不影响我们找 scf.for 的结构。
    if (isa<arith::ConstantOp>(op))
      continue;
    if (isa<func::ReturnOp>(op))
      continue;
    // 真正不认识的 op(例如还没被阶段 2 转换、留着原始 linalg.matmul 的
    // function)——优雅跳过这个 function,而不是让整个工具失败。
    outer = nullptr;
    break;
  }

  if (!outer) {
    os << "  // 找不到 systolic 展开出来的迴圈结构,略过这个函式\n}\n\n";
    return success();
  }

  std::string cName = em.nameOf(outer.getInitArgs()[0]);
  if (failed(emitOuterLoop(em, outer, cName, /*indent=*/2)))
    return failure();

  os << "}\n\n";
  return success();
}

} // namespace

int main(int argc, char **argv) {
  cl::ParseCommandLineOptions(argc, argv, "Systolic dialect -> HLS C 翻译器\n");

  DialectRegistry registry;
  registry.insert<systolic::SystolicDialect, func::FuncDialect,
                   scf::SCFDialect, tensor::TensorDialect,
                   arith::ArithDialect, linalg::LinalgDialect>();
  MLIRContext context(registry);
  context.loadAllAvailableDialects();

  std::string errorMessage;
  auto input = openInputFile(inputFilename, &errorMessage);
  if (!input) {
    llvm::errs() << errorMessage << "\n";
    return 1;
  }

  llvm::SourceMgr sourceMgr;
  sourceMgr.AddNewSourceBuffer(std::move(input), llvm::SMLoc());
  OwningOpRef<ModuleOp> module =
      parseSourceFile<ModuleOp>(sourceMgr, &context);
  if (!module) {
    llvm::errs() << "解析输入的 MLIR 失败\n";
    return 1;
  }

  auto output = openOutputFile(outputFilename, &errorMessage);
  if (!output) {
    llvm::errs() << errorMessage << "\n";
    return 1;
  }

  for (auto func : module->getOps<func::FuncOp>()) {
    if (failed(emitFunction(func, output->os()))) {
      llvm::errs() << "codegen 失败: " << func.getName() << "\n";
      return 1;
    }
  }

  output->keep();
  return 0;
}
