//===- NestBinding.cpp - Which loop nest a record is about ---------------===//
//
// See NestBinding.h. The research build compiles this file as it is
// (research/generalized-gemm/CMakeLists.txt), so the research tool and the production
// pass compute one binding with one piece of code. It reads the function and changes
// nothing in it.
//
//===----------------------------------------------------------------------===//
#include "Systolic/NestBinding.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/OwningOpRef.h"
#include "mlir/Interfaces/LoopLikeInterface.h"

#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;

namespace {

std::string printed(Type type) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  type.print(stream);
  return text;
}

bool isAllocation(Operation *op) { return isa<memref::AllocOp, memref::AllocaOp>(op); }

} // namespace

namespace mlir::systolic {

FunctionRoots numberFunctionRoots(func::FuncOp function) {
  FunctionRoots out;
  if (function.isExternal())
    return out;
  Block &entry = function.getBody().front();
  for (BlockArgument argument : entry.getArguments()) {
    if (!isa<MemRefType>(argument.getType()))
      continue;
    FunctionRoot root;
    root.argument = true;
    root.argNumber = argument.getArgNumber();
    root.noalias = (bool)function.getArgAttr(root.argNumber, "llvm.noalias");
    root.type = printed(argument.getType());
    out.number[argument] = out.roots.size();
    out.roots.push_back(std::move(root));
  }
  function.walk<WalkOrder::PreOrder>([&](Operation *op) {
    if (!isAllocation(op))
      return;
    FunctionRoot root;
    root.type = printed(op->getResult(0).getType());
    out.number[op->getResult(0)] = out.roots.size();
    out.roots.push_back(std::move(root));
  });
  return out;
}

SmallVector<Operation *> outermostLoops(func::FuncOp function) {
  SmallVector<Operation *> out;
  if (function.isExternal())
    return out;
  function.getBody().walk<WalkOrder::PreOrder>([&](Operation *op) -> WalkResult {
    if (isa<LoopLikeOpInterface>(op)) {
      out.push_back(op);
      return WalkResult::skip();
    }
    return WalkResult::advance();
  });
  return out;
}

FailureOr<NestBinding> computeNestBinding(func::FuncOp function, Operation *loop, unsigned ordinal,
                                          const FunctionRoots &roots, std::string &why) {
  // What the loop reads and does not define, in order of first use.
  SmallVector<Value> liveIns;
  llvm::DenseSet<Value> seen;
  auto inside = [&](Value value) {
    Block *block = value.getParentBlock();
    Operation *parent = block ? block->getParentOp() : nullptr;
    return parent && loop->isAncestor(parent);
  };
  loop->walk<WalkOrder::PreOrder>([&](Operation *op) {
    for (Value value : op->getOperands())
      if (!inside(value) && seen.insert(value).second)
        liveIns.push_back(value);
  });

  // Roots by their number, then the function's scalar arguments by their number, then the
  // values the function computes, by first use. Constants are copied in.
  SmallVector<std::pair<unsigned, Value>> rootInputs;
  SmallVector<Value> argumentScalars, computedScalars, constants;
  Block *entry = function.isExternal() ? nullptr : &function.getBody().front();
  for (Value value : liveIns) {
    Type type = value.getType();
    if (isa<BaseMemRefType>(type)) {
      auto found = roots.number.find(value);
      if (found == roots.number.end()) {
        why = "the loop reads a memref that is no root of the function";
        return failure();
      }
      rootInputs.push_back({found->second, value});
      continue;
    }
    if (!isa<IntegerType, IndexType, FloatType>(type)) {
      why = "the loop reads a value of type " + printed(type) + ", which is neither a memref nor a scalar";
      return failure();
    }
    Operation *def = value.getDefiningOp();
    if (def && isa<arith::ConstantOp>(def))
      constants.push_back(value);
    else if (!def && value.getParentBlock() == entry)
      argumentScalars.push_back(value);
    else
      computedScalars.push_back(value);
  }
  llvm::sort(rootInputs, [](const auto &a, const auto &b) { return a.first < b.first; });
  llvm::stable_sort(argumentScalars, [](Value a, Value b) {
    return cast<BlockArgument>(a).getArgNumber() < cast<BlockArgument>(b).getArgNumber();
  });

  std::string text;
  llvm::raw_string_ostream os(text);
  os << kNestBindingScheme << "\n"
     << "function: " << function.getSymName() << "\n"
     << "nest: " << ordinal << "\n";
  SmallVector<Value> inputs;
  for (auto [number, value] : rootInputs) {
    const FunctionRoot &root = roots.roots[number];
    os << "argument " << inputs.size() << ": root " << number << ", ";
    if (root.argument)
      os << "argument " << root.argNumber << (root.noalias ? ", llvm.noalias" : "");
    else
      os << "allocation";
    os << " : " << printed(value.getType()) << "\n";
    inputs.push_back(value);
  }
  for (Value value : argumentScalars) {
    os << "argument " << inputs.size() << ": scalar, argument " << cast<BlockArgument>(value).getArgNumber()
       << " : " << printed(value.getType()) << "\n";
    inputs.push_back(value);
  }
  for (Value value : computedScalars) {
    Operation *def = value.getDefiningOp();
    os << "argument " << inputs.size() << ": scalar, computed by "
       << (def ? def->getName().getStringRef() : StringRef("a block argument")) << " : "
       << printed(value.getType()) << "\n";
    inputs.push_back(value);
  }

  // The loop in a function of its own, printed in the generic form without locations.
  MLIRContext *context = function->getContext();
  Location location = UnknownLoc::get(context);
  OpBuilder builder(context);
  OwningOpRef<ModuleOp> module(ModuleOp::create(location));
  SmallVector<Type> types;
  for (Value value : inputs)
    types.push_back(value.getType());
  func::FuncOp copy = func::FuncOp::create(location, "nest", builder.getFunctionType(types, {}));
  module->push_back(copy);
  Block *body = copy.addEntryBlock();
  IRMapping mapping;
  for (auto [index, value] : llvm::enumerate(inputs))
    mapping.map(value, body->getArgument(index));
  builder.setInsertionPointToEnd(body);
  for (Value value : constants) {
    Operation *constant = builder.clone(*value.getDefiningOp());
    mapping.map(value, constant->getResult(0));
  }
  builder.clone(*loop, mapping);
  func::ReturnOp::create(builder, location);
  os << "\n";
  module.get()->print(os, OpPrintingFlags().printGenericOpForm());

  NestBinding out;
  out.text = std::move(text);
  out.sha256 = llvm::toHex(llvm::SHA256::hash(llvm::arrayRefFromStringRef(out.text)), /*LowerCase=*/true);
  return out;
}

} // namespace mlir::systolic
