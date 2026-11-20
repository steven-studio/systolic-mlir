#include "GeneralizedGemm/SemanticImport.h"

#include "mlir/Conversion/AffineToStandard/AffineToStandard.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlowOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Pass/PassManager.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ADT/TypeSwitch.h"
#include "llvm/Support/SHA256.h"
#include "llvm/Support/raw_ostream.h"

#include <algorithm>
#include <functional>
#include <set>

using namespace mlir;
using namespace mlir::generalized_gemm;
using llvm::APInt;

namespace {

/// Internal exit for a refusal. Caught at the API boundary, where it clears
/// everything but the reason (invariant I3).
struct Refusal {
  UnsupportedKind kind;
  std::string detail;
};
[[noreturn]] void refuse(UnsupportedKind kind, std::string detail) {
  throw Refusal{kind, std::move(detail)};
}

/// The attribute names `op` may carry. Anything else refuses the function, so
/// a future or unknown semantic attribute can never be ignored.
void checkAttributes(Operation *op,
                     std::initializer_list<llvm::StringRef> allowed) {
  for (NamedAttribute attr : op->getAttrs()) {
    llvm::StringRef name = attr.getName().strref();
    if (name == "operandSegmentSizes" || name == "resultSegmentSizes")
      continue;
    // These two are hints when false. True is a claim, so it stays refused.
    if (name == "nontemporal" || name == "invariant")
      if (auto flag = dyn_cast<BoolAttr>(attr.getValue()); flag && !flag.getValue())
        continue;
    if (std::find(allowed.begin(), allowed.end(), name) == allowed.end())
      refuse(UnsupportedKind::UnsupportedAttribute,
             op->getName().getStringRef().str() + ": " + name.str());
  }
}

void checkFunctionAttributes(func::FuncOp function) {
  for (NamedAttribute attr : function->getAttrs()) {
    llvm::StringRef name = attr.getName().strref();
    if (name == "sym_name" || name == "function_type" ||
        name == "sym_visibility" || name == "arg_attrs")
      continue;
    refuse(UnsupportedKind::UnsupportedAttribute, "func: " + name.str());
  }
  for (unsigned i = 0, e = function.getNumArguments(); i != e; ++i)
    if (DictionaryAttr attrs = function.getArgAttrDict(i))
      for (NamedAttribute attr : attrs)
        if (attr.getName().strref() != "llvm.noalias")
          refuse(UnsupportedKind::UnsupportedAttribute,
                 "func argument: " + attr.getName().strref().str());
}

std::string fingerprintOf(func::FuncOp function, const ImportOptions &options) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  Operation *top = function->getParentOp() ? function->getParentOp()
                                           : function.getOperation();
  top->print(stream, OpPrintingFlags().printGenericOpForm().enableDebugInfo());
  stream << "|function=" << function.getSymName()
         << "|index=" << options.indexBitWidth
         << "|structured=" << options.lowerStructured;
  stream.flush();
  llvm::SHA256 hash;
  hash.update(text);
  auto digest = hash.final();
  return llvm::toHex(digest, /*LowerCase=*/true);
}

/// True when the region has a cycle reachable from its entry.
bool hasReachableCycle(Region &region) {
  llvm::DenseMap<Block *, int> color;
  std::function<bool(Block *)> visit = [&](Block *b) {
    color[b] = 1;
    for (Block *s : b->getSuccessors()) {
      int c = color.lookup(s);
      if (c == 1 || (c == 0 && visit(s)))
        return true;
    }
    color[b] = 2;
    return false;
  };
  return visit(&region.front());
}

llvm::SetVector<Block *> reachableBlocks(Region &region) {
  llvm::SetVector<Block *> seen;
  llvm::SmallVector<Block *> work{&region.front()};
  while (!work.empty()) {
    Block *b = work.pop_back_val();
    if (!seen.insert(b))
      continue;
    for (Block *s : b->getSuccessors())
      work.push_back(s);
  }
  return seen;
}

/// Inlines one func.call on the private clone: split at the call, clone the
/// callee before the continuation, and route every return to it.
void inlineOneCall(func::CallOp call, func::FuncOp callee) {
  Block *before = call->getBlock();
  Region *region = before->getParent();
  Block *after = before->splitBlock(std::next(call->getIterator()));
  for (Value result : call.getResults()) {
    BlockArgument argument = after->addArgument(result.getType(), call.getLoc());
    result.replaceAllUsesWith(argument);
  }
  IRMapping mapping;
  for (auto [parameter, operand] :
       llvm::zip(callee.getArguments(), call.getOperands()))
    mapping.map(parameter, operand);
  callee.getBody().cloneInto(region, after->getIterator(), mapping);
  Block *clonedEntry = mapping.lookup(&callee.getBody().front());
  OpBuilder builder(call);
  builder.setInsertionPointToEnd(before);
  cf::BranchOp::create(builder, call.getLoc(), clonedEntry, ValueRange{});
  call.erase();
  for (Block &block : callee.getBody()) {
    Block *cloned = mapping.lookup(&block);
    if (auto ret = dyn_cast<func::ReturnOp>(cloned->getTerminator())) {
      builder.setInsertionPoint(ret);
      cf::BranchOp::create(builder, ret.getLoc(), after, ret.getOperands());
      ret.erase();
    }
  }
}

void checkCallGraph(func::FuncOp function, ModuleOp module) {
  llvm::DenseSet<Operation *> done, active;
  std::function<void(func::FuncOp)> visit = [&](func::FuncOp f) {
    if (done.count(f))
      return;
    if (!active.insert(f).second)
      refuse(UnsupportedKind::RecursiveCall, f.getSymName().str());
    f.walk([&](func::CallOp call) {
      if (auto callee = module.lookupSymbol<func::FuncOp>(call.getCallee()))
        if (!callee.isExternal())
          visit(callee);
    });
    active.erase(f);
    done.insert(f);
  };
  visit(function);
}

void inlineCalls(func::FuncOp function, ModuleOp module,
                 const ImportLimits &limits) {
  checkCallGraph(function, module);
  function->getContext()->getOrLoadDialect<cf::ControlFlowDialect>();
  for (unsigned count = 0;; ++count) {
    func::CallOp next;
    for (Block *block : reachableBlocks(function.getBody())) {
      for (Operation &op : *block)
        if (auto call = dyn_cast<func::CallOp>(op)) {
          next = call;
          break;
        }
      if (next)
        break;
    }
    if (!next)
      return;
    if (count >= limits.maxInlinedCalls)
      refuse(UnsupportedKind::ResourceLimit, "inlined calls");
    checkAttributes(next, {"callee"});
    auto callee = module.lookupSymbol<func::FuncOp>(next.getCallee());
    if (!callee || callee.isExternal())
      refuse(UnsupportedKind::ExternalCall, next.getCallee().str());
    if (hasReachableCycle(callee.getBody()))
      refuse(UnsupportedKind::CyclicCallee, next.getCallee().str());
    checkFunctionAttributes(callee);
    inlineOneCall(next, callee);
  }
}

/// A scalar in a block: its value and when that value is poison.
struct Symbolic {
  Term value = nullptr, poison = nullptr;
};

class Importer {
public:
  Importer(func::FuncOp function, const ImportOptions &options,
           SemanticProgram &program)
      : function(function), options(options), program(program),
        terms(*program.terms) {}

  void run();

private:
  // Layout.
  Sort sortOf(Type type);
  void checkMemrefType(MemRefType type);
  void layoutBlocks();
  void layoutParameters();
  void layoutState();
  void layoutAllocations();
  void layoutRootPairs();
  void layoutSort(Value root, MemRefType type, MemoryRoot &entry);
  bool inCycle(Block *block);

  // One block.
  void importBlock(Block *block);
  void importOperation(Operation &op);
  void finishTerminator(Operation *terminator);
  Symbolic lookup(Value value);
  void bind(Value value, Symbolic symbolic);
  unsigned rootOf(Value memref);
  void side(SideKind kind, const char *reason, Term violated);
  Term extent(const MemoryRoot &root, unsigned dimension);
  llvm::SmallVector<Term, 3> checkedIndices(Value memref, ValueRange indices);

  // Operation families.
  void importIntegerBinary(Operation &op, TermOp kind);
  void importDivision(Operation &op);
  void importShift(Operation &op, TermOp kind);
  void importCast(Operation &op);
  void importFloat(Operation &op, TermOp kind, arith::FastMathFlags flags);
  void importMinMax(Operation &op, Predicate choose);
  void importConstant(arith::ConstantOp op);
  void importLoad(memref::LoadOp op);
  void importStore(memref::StoreOp op);

  // After all blocks.
  void detectSites(Block *block);
  void computeObliviousness();

  func::FuncOp function;
  const ImportOptions &options;
  SemanticProgram &program;
  TermContext &terms;

  llvm::DenseMap<Block *, unsigned> locationOf;
  llvm::SmallVector<Block *> blocks;
  llvm::DenseMap<Block *, bool> cycleMemo;
  llvm::DenseMap<Value, unsigned> stateOf;
  llvm::DenseMap<Value, unsigned> parameterOf;
  llvm::DenseMap<Value, unsigned> roots;
  unsigned opCounter = 0;

  // Per block.
  llvm::DenseMap<Value, Symbolic> environment;
  llvm::DenseMap<unsigned, Term> memory;
  llvm::SmallVector<SideCondition, 0> sides;
  llvm::SmallVector<MemoryEffect, 0> effects;
  llvm::DenseMap<Value, unsigned> loadEffect;
  llvm::DenseMap<Operation *, unsigned> opIndex;
};

Sort Importer::sortOf(Type type) {
  if (auto integer = dyn_cast<IntegerType>(type)) {
    if (!integer.isSignless() || integer.getWidth() > 64)
      refuse(UnsupportedKind::UnsupportedType,
             "integers must be signless, of width 1..64");
    return Sort::bv(integer.getWidth());
  }
  if (isa<IndexType>(type)) {
    if (options.indexBitWidth == 0)
      refuse(UnsupportedKind::IndexWidthUnspecified,
             "`index` is used and no index width was given");
    return Sort::bv(options.indexBitWidth);
  }
  if (type.isF32())
    return Sort::flt(32);
  if (type.isF64())
    return Sort::flt(64);
  std::string text;
  llvm::raw_string_ostream stream(text);
  type.print(stream);
  refuse(UnsupportedKind::UnsupportedType, text);
}

void Importer::checkMemrefType(MemRefType type) {
  if (!type.getLayout().isIdentity())
    refuse(UnsupportedKind::NonIdentityLayout, "non-identity memref layout");
  if (type.getMemorySpace())
    refuse(UnsupportedKind::NonDefaultMemorySpace, "memref memory space");
}

bool Importer::inCycle(Block *block) {
  auto it = cycleMemo.find(block);
  if (it != cycleMemo.end())
    return it->second;
  llvm::DenseSet<Block *> seen;
  llvm::SmallVector<Block *> work(block->getSuccessors().begin(),
                                  block->getSuccessors().end());
  bool found = false;
  while (!work.empty() && !found) {
    Block *b = work.pop_back_val();
    if (b == block)
      found = true;
    else if (seen.insert(b).second)
      work.append(b->getSuccessors().begin(), b->getSuccessors().end());
  }
  return cycleMemo[block] = found;
}

void Importer::layoutBlocks() {
  Region &body = function.getBody();
  auto reachable = reachableBlocks(body);
  for (Block &block : body)
    if (reachable.contains(&block)) {
      locationOf[&block] = blocks.size();
      blocks.push_back(&block);
    }
  if (blocks.size() > options.limits.maxBlocks)
    refuse(UnsupportedKind::ResourceLimit, "blocks");
  program.locationCount = blocks.size();
  program.entryLocation = 0;
  unsigned count = 0;
  for (Block *block : blocks)
    for (Operation &op : *block) {
      opIndex[&op] = count++;
      if (op.getNumRegions())
        refuse(UnsupportedKind::NestedRegion,
               op.getName().getStringRef().str());
      // Before types are read: a function value has no sort, and the callee
      // being unknown is the reason worth reporting.
      if (isa<func::CallIndirectOp>(op))
        refuse(UnsupportedKind::IndirectCall, "func.call_indirect");
    }
  if (count > options.limits.maxOperations)
    refuse(UnsupportedKind::ResourceLimit, "operations");
}

void Importer::layoutSort(Value root, MemRefType type, MemoryRoot &entry) {
  (void)root;
  checkMemrefType(type);
  if (options.indexBitWidth == 0)
    refuse(UnsupportedKind::IndexWidthUnspecified,
           "memory addressing needs an index width");
  entry.sort = Sort::mem(entry.id, type.getRank(), options.indexBitWidth,
                         sortOf(type.getElementType()));
  for (int64_t extent : type.getShape()) {
    entry.extents.push_back(ShapedType::isDynamic(extent) ? -1 : extent);
    entry.dimensionParameter.push_back(-1);
  }
}

void Importer::layoutParameters() {
  Block &entry = *blocks.front();
  for (BlockArgument argument : entry.getArguments()) {
    unsigned number = argument.getArgNumber();
    if (auto type = dyn_cast<MemRefType>(argument.getType())) {
      MemoryRoot root;
      root.origin = MemoryRoot::FunctionArgument;
      root.id = program.roots.size();
      root.argument = number;
      root.restrictArgument = (bool)function.getArgAttr(number, "llvm.noalias");
      layoutSort(argument, type, root);
      roots[argument] = root.id;
      program.roots.push_back(root);
      continue;
    }
    if (isa<BaseMemRefType>(argument.getType()))
      refuse(UnsupportedKind::UnsupportedType, "unranked memref argument");
    Parameter parameter;
    parameter.kind = Parameter::Scalar;
    parameter.id = program.parameters.size();
    parameter.sort = sortOf(argument.getType());
    parameter.argument = number;
    parameterOf[argument] = parameter.id;
    program.parameters.push_back(parameter);
  }
  for (MemoryRoot &root : program.roots)
    for (unsigned d = 0; d < root.extents.size(); ++d)
      if (root.extents[d] < 0) {
        Parameter parameter;
        parameter.kind = Parameter::Dimension;
        parameter.id = program.parameters.size();
        parameter.sort = Sort::bv(options.indexBitWidth);
        parameter.argument = root.argument;
        parameter.dimension = d;
        root.dimensionParameter[d] = parameter.id;
        program.parameters.push_back(parameter);
      }
  for (Type type : function.getResultTypes()) {
    if (isa<BaseMemRefType>(type))
      refuse(UnsupportedKind::MemrefReturn, "memref result");
    program.returnSorts.push_back(sortOf(type));
  }
}

void Importer::layoutState() {
  for (Block *block : blocks) {
    unsigned location = locationOf[block];
    if (block != blocks.front())
      for (BlockArgument argument : block->getArguments()) {
        if (isa<BaseMemRefType>(argument.getType()))
          refuse(UnsupportedKind::MemrefBlockArgument,
                 "a memref block argument has no static root");
        StateVar var;
        var.kind = StateVar::BlockArgument;
        var.id = program.state.size();
        var.sort = sortOf(argument.getType());
        var.location = location;
        var.position = argument.getArgNumber();
        var.loopCarried = inCycle(block);
        stateOf[argument] = var.id;
        program.state.push_back(var);
      }
    for (Operation &op : *block)
      for (Value result : op.getResults()) {
        if (isa<BaseMemRefType>(result.getType()))
          continue;
        bool crosses = false;
        for (OpOperand &use : result.getUses()) {
          Block *user = use.getOwner()->getBlock();
          if (user != block && locationOf.count(user))
            crosses = true;
        }
        if (!crosses)
          continue;
        StateVar var;
        var.kind = StateVar::CrossBlockResult;
        var.id = program.state.size();
        var.sort = sortOf(result.getType());
        var.location = location;
        var.position = opIndex[&op];
        stateOf[result] = var.id;
        program.state.push_back(var);
      }
  }
  if (program.state.size() > options.limits.maxStates)
    refuse(UnsupportedKind::ResourceLimit, "state variables");
}

void Importer::layoutAllocations() {
  for (Block *block : blocks)
    for (Operation &op : *block) {
      if (!isa<memref::AllocOp, memref::AllocaOp>(op))
        continue;
      checkAttributes(&op, {});
      auto type = cast<MemRefType>(op.getResult(0).getType());
      if (inCycle(block))
        refuse(UnsupportedKind::AllocationInCycle,
               "an allocation inside a cycle has many dynamic instances");
      if (!type.hasStaticShape() || op.getNumOperands())
        refuse(UnsupportedKind::DynamicAllocation, "allocation shape");
      MemoryRoot root;
      root.origin = MemoryRoot::FreshAllocation;
      root.id = program.roots.size();
      root.definingOp = opIndex[&op];
      layoutSort(op.getResult(0), type, root);
      roots[op.getResult(0)] = root.id;
      program.roots.push_back(root);
    }
}

// Separation of two roots is what lets one array per root model memory. The
// rule is the compiler's policy (docs/AFFINE_GEMM_MAPPING.md section 7).
void Importer::layoutRootPairs() {
  for (unsigned i = 0; i < program.roots.size(); ++i)
    for (unsigned j = i + 1; j < program.roots.size(); ++j) {
      const MemoryRoot &a = program.roots[i], &b = program.roots[j];
      std::string name =
          "roots " + std::to_string(i) + " and " + std::to_string(j) + " are separate";
      if (a.origin == MemoryRoot::FreshAllocation ||
          b.origin == MemoryRoot::FreshAllocation)
        program.rootPairs.push_back(
            {i, j, ProofStatus::proven("fresh allocation")});
      else if (a.restrictArgument || b.restrictArgument)
        program.rootPairs.push_back(
            {i, j, ProofStatus::proven("llvm.noalias source contract")});
      else {
        ProofStatus status = ProofStatus::assumed(name);
        program.rootPairs.push_back({i, j, status});
        program.assumptions.push_back(status);
      }
    }
}

Symbolic Importer::lookup(Value value) {
  auto local = environment.find(value);
  if (local != environment.end())
    return local->second;
  auto parameter = parameterOf.find(value);
  if (parameter != parameterOf.end())
    return {terms.param(parameter->second,
                        program.parameters[parameter->second].sort),
            terms.boolean(false)};
  auto state = stateOf.find(value);
  if (state != stateOf.end())
    return {terms.state(state->second, program.state[state->second].sort),
            terms.shadow(state->second)};
  refuse(UnsupportedKind::UnsupportedOperation,
         "a value with no modelled definition");
}

void Importer::bind(Value value, Symbolic symbolic) {
  environment[value] = symbolic;
}

unsigned Importer::rootOf(Value memref) {
  while (auto cast = memref.getDefiningOp<memref::CastOp>())
    memref = cast.getSource();
  auto it = roots.find(memref);
  if (it == roots.end())
    refuse(UnsupportedKind::UnsupportedView, "a memref with no modelled root");
  return it->second;
}

void Importer::side(SideKind kind, const char *reason, Term violated) {
  if (violated->op == TermOp::Const && violated->value.isZero())
    return;
  sides.push_back({kind, reason, opCounter, violated});
}

Term Importer::extent(const MemoryRoot &root, unsigned d) {
  if (root.extents[d] >= 0) {
    if (!APInt(64, root.extents[d]).isIntN(options.indexBitWidth))
      refuse(UnsupportedKind::ConstantOutOfRange, "memref extent");
    return terms.bv(APInt(options.indexBitWidth, root.extents[d]));
  }
  int id = root.dimensionParameter[d];
  return terms.param(id, program.parameters[id].sort);
}

llvm::SmallVector<Term, 3> Importer::checkedIndices(Value memref,
                                                    ValueRange indices) {
  const MemoryRoot &root = program.roots[rootOf(memref)];
  llvm::SmallVector<Term, 3> result;
  Term poison = terms.boolean(false), outside = terms.boolean(false);
  for (auto [d, index] : llvm::enumerate(indices)) {
    Symbolic i = lookup(index);
    poison = terms.logicalOr(poison, i.poison);
    // Unsigned: a negative index is a huge one, so it is outside as well.
    outside = terms.logicalOr(
        outside, terms.icmp(Predicate::Uge, i.value, extent(root, d)));
    result.push_back(i.value);
  }
  side(SideKind::UndefinedBehavior, "poison memory index", poison);
  side(SideKind::UndefinedBehavior, "memory access out of bounds", outside);
  return result;
}

void Importer::importConstant(arith::ConstantOp op) {
  checkAttributes(op, {"value"});
  Type type = op.getType();
  Sort sort = sortOf(type);
  Term value;
  if (auto integer = dyn_cast<IntegerAttr>(op.getValue())) {
    APInt bits = integer.getValue();
    if (bits.getBitWidth() > sort.width) {
      if (!bits.isSignedIntN(sort.width))
        refuse(UnsupportedKind::ConstantOutOfRange,
               "index constant at the given width");
      bits = bits.trunc(sort.width);
    } else if (bits.getBitWidth() < sort.width) {
      bits = bits.sext(sort.width);
    }
    value = terms.bv(bits);
  } else if (auto real = dyn_cast<FloatAttr>(op.getValue())) {
    value = terms.fconst(real.getValue().bitcastToAPInt());
  } else {
    refuse(UnsupportedKind::UnsupportedOperation, "non-scalar constant");
  }
  bind(op.getResult(), {value, terms.boolean(false)});
}

static unsigned overflowBits(arith::IntegerOverflowFlags flags) {
  unsigned bits = 0;
  if (arith::bitEnumContainsAll(flags, arith::IntegerOverflowFlags::nsw))
    bits |= FlagNSW;
  if (arith::bitEnumContainsAll(flags, arith::IntegerOverflowFlags::nuw))
    bits |= FlagNUW;
  return bits;
}

/// The result of `kind` on a and b does not fit, computed at a width where the
/// mathematical result cannot wrap. `extend` picks the signedness.
static Term overflows(TermContext &terms, TermOp kind, Term a, Term b, Term result,
                      bool isSigned) {
  unsigned w = a->sort.width;
  unsigned wide = kind == TermOp::Mul ? 2 * w : w + 1;
  auto extend = [&](Term t) {
    return isSigned ? terms.sext(t, wide) : terms.zext(t, wide);
  };
  return terms.icmp(Predicate::Ne, terms.binary(kind, extend(a), extend(b)),
                    extend(result));
}

void Importer::importIntegerBinary(Operation &op, TermOp kind) {
  Symbolic a = lookup(op.getOperand(0)), b = lookup(op.getOperand(1));
  unsigned flags = 0;
  if (auto iface = dyn_cast<arith::ArithIntegerOverflowFlagsInterface>(op)) {
    checkAttributes(&op, {"overflowFlags"});
    flags = overflowBits(iface.getOverflowAttr().getValue());
  } else {
    checkAttributes(&op, {});
  }
  Term value = terms.binary(kind, a.value, b.value, flags);
  Term poison = terms.logicalOr(a.poison, b.poison);
  if (flags & FlagNSW)
    poison = terms.logicalOr(poison, overflows(terms, kind, a.value, b.value,
                                               value, /*isSigned=*/true));
  if (flags & FlagNUW)
    poison = terms.logicalOr(poison, overflows(terms, kind, a.value, b.value,
                                               value, /*isSigned=*/false));
  bind(op.getResult(0), {value, poison});
}

void Importer::importDivision(Operation &op) {
  Symbolic a = lookup(op.getOperand(0)), b = lookup(op.getOperand(1));
  unsigned w = a.value->sort.width;
  Term zero = terms.bv(APInt::getZero(w));
  Term minimum = terms.bv(APInt::getSignedMinValue(w));
  Term minusOne = terms.bv(APInt::getAllOnes(w));
  Term byZero = terms.icmp(Predicate::Eq, b.value, zero);
  Term byMinusOne = terms.icmp(Predicate::Eq, b.value, minusOne);
  // A poison dividend may be the minimum value, so it also overflows.
  Term overflow = terms.logicalAnd(
      byMinusOne, terms.logicalOr(a.poison, terms.icmp(Predicate::Eq, a.value,
                                                       minimum)));
  side(SideKind::UndefinedBehavior, "poison divisor", b.poison);
  side(SideKind::UndefinedBehavior, "division by zero", byZero);

  bool exact = false;
  TermOp kind;
  if (auto o = dyn_cast<arith::DivSIOp>(op)) {
    exact = o.getIsExact();
    kind = TermOp::SDiv;
    side(SideKind::UndefinedBehavior, "signed division overflow", overflow);
  } else if (auto o = dyn_cast<arith::DivUIOp>(op)) {
    exact = o.getIsExact();
    kind = TermOp::UDiv;
  } else if (isa<arith::RemSIOp>(op)) {
    kind = TermOp::SRem;
    // arith documents only division by zero; the lowering to llvm.srem makes
    // this case undefined. The two disagree, so no value is claimed.
    side(SideKind::Underspecified, "remsi of the minimum value by -1", overflow);
  } else {
    kind = TermOp::URem;
  }
  if (exact)
    checkAttributes(&op, {"isExact"});
  else
    checkAttributes(&op, {});
  Term value = terms.binary(kind, a.value, b.value, exact ? (unsigned)FlagExact : 0u);
  Term poison = a.poison;
  if (exact) {
    TermOp remainder = kind == TermOp::SDiv ? TermOp::SRem : TermOp::URem;
    poison = terms.logicalOr(
        poison, terms.icmp(Predicate::Ne,
                           terms.binary(remainder, a.value, b.value), zero));
  }
  bind(op.getResult(0), {value, poison});
}

void Importer::importShift(Operation &op, TermOp kind) {
  Symbolic a = lookup(op.getOperand(0)), b = lookup(op.getOperand(1));
  unsigned w = a.value->sort.width;
  unsigned flags = 0;
  if (auto shl = dyn_cast<arith::ShLIOp>(op)) {
    checkAttributes(&op, {"overflowFlags"});
    flags = overflowBits(shl.getOverflowFlags());
  } else {
    bool exact = isa<arith::ShRSIOp>(op) ? cast<arith::ShRSIOp>(op).getIsExact()
                                         : cast<arith::ShRUIOp>(op).getIsExact();
    if (exact) {
      checkAttributes(&op, {"isExact"});
      flags = FlagExact;
    } else {
      checkAttributes(&op, {});
    }
  }
  Term value = terms.binary(kind, a.value, b.value, flags);
  Term tooFar = terms.icmp(Predicate::Uge, b.value, terms.bv(APInt(w, w, false, true)));
  Term poison = terms.logicalOr(terms.logicalOr(a.poison, b.poison), tooFar);
  if (flags & FlagNSW)
    poison = terms.logicalOr(
        poison, terms.icmp(Predicate::Ne,
                           terms.binary(TermOp::AShr, value, b.value), a.value));
  if (flags & FlagNUW)
    poison = terms.logicalOr(
        poison, terms.icmp(Predicate::Ne,
                           terms.binary(TermOp::LShr, value, b.value), a.value));
  if (flags & FlagExact)
    poison = terms.logicalOr(
        poison, terms.icmp(Predicate::Ne,
                           terms.binary(TermOp::Shl, value, b.value), a.value));
  bind(op.getResult(0), {value, poison});
}

void Importer::importCast(Operation &op) {
  Symbolic a = lookup(op.getOperand(0));
  unsigned from = a.value->sort.width;
  unsigned to = sortOf(op.getResult(0).getType()).width;
  Term poison = a.poison;
  Term value = a.value;
  Term top = terms.icmp(Predicate::Slt, a.value, terms.bv(APInt::getZero(from)));

  auto widen = [&](bool isSigned) {
    if (to > from)
      value = isSigned ? terms.sext(a.value, to) : terms.zext(a.value, to);
    else if (to < from)
      value = terms.trunc(a.value, to);
  };

  if (isa<arith::ExtSIOp>(op)) {
    checkAttributes(&op, {});
    widen(true);
  } else if (auto ext = dyn_cast<arith::ExtUIOp>(op)) {
    bool nneg = ext.getNonNeg();
    if (nneg) {
      checkAttributes(&op, {"nonNeg"});
      poison = terms.logicalOr(poison, top);
    } else {
      checkAttributes(&op, {});
    }
    widen(false);
  } else if (auto narrow = dyn_cast<arith::TruncIOp>(op)) {
    checkAttributes(&op, {"overflowFlags"});
    unsigned flags = overflowBits(narrow.getOverflowFlags());
    widen(false);
    if (flags & FlagNSW)
      poison = terms.logicalOr(
          poison, terms.icmp(Predicate::Ne, terms.sext(value, from), a.value));
    if (flags & FlagNUW)
      poison = terms.logicalOr(
          poison, terms.icmp(Predicate::Ne, terms.zext(value, from), a.value));
  } else if (isa<arith::IndexCastOp>(op)) {
    checkAttributes(&op, {});
    widen(true);
  } else {
    auto cast = dyn_cast<arith::IndexCastUIOp>(op);
    bool nneg = cast.getNonNeg();
    if (nneg) {
      checkAttributes(&op, {"nonNeg"});
      poison = terms.logicalOr(poison, top);
    } else {
      checkAttributes(&op, {});
    }
    widen(false);
  }
  bind(op.getResult(0), {value, poison});
}

static unsigned fastMathBits(arith::FastMathFlags flags) {
  using F = arith::FastMathFlags;
  unsigned bits = 0;
  if (arith::bitEnumContainsAll(flags, F::reassoc)) bits |= FlagReassoc;
  if (arith::bitEnumContainsAll(flags, F::nnan)) bits |= FlagNNaN;
  if (arith::bitEnumContainsAll(flags, F::ninf)) bits |= FlagNInf;
  if (arith::bitEnumContainsAll(flags, F::nsz)) bits |= FlagNSZ;
  if (arith::bitEnumContainsAll(flags, F::arcp)) bits |= FlagARCFP;
  if (arith::bitEnumContainsAll(flags, F::contract)) bits |= FlagContract;
  if (arith::bitEnumContainsAll(flags, F::afn)) bits |= FlagAFN;
  return bits;
}

void Importer::importFloat(Operation &op, TermOp kind, arith::FastMathFlags fm) {
  // `roundingmode` is deliberately absent: only the default mode is modelled.
  checkAttributes(&op, {"fastmath"});
  Symbolic a = lookup(op.getOperand(0)), b = lookup(op.getOperand(1));
  unsigned flags = fastMathBits(fm);
  Term value = terms.fbinary(kind, a.value, b.value, flags);
  Term poison = terms.logicalOr(a.poison, b.poison);
  auto any = [&](TermOp cls) {
    return terms.logicalOr(terms.logicalOr(terms.fclass(cls, a.value),
                                           terms.fclass(cls, b.value)),
                           terms.fclass(cls, value));
  };
  if (flags & FlagNNaN)
    poison = terms.logicalOr(poison, any(TermOp::FIsNaN));
  if (flags & FlagNInf)
    poison = terms.logicalOr(poison, any(TermOp::FIsInf));
  Term live = terms.logicalNot(poison);
  // A NaN result's payload and sign are not determined by IEEE-754 or LLVM.
  side(SideKind::Underspecified, "NaN result: payload is not determined",
       terms.logicalAnd(live, terms.fclass(TermOp::FIsNaN, value)));
  if (flags & FlagNSZ)
    side(SideKind::Underspecified, "nsz: the sign of a zero is not determined",
         terms.logicalAnd(live, terms.fclass(TermOp::FIsZero, value)));
  bind(op.getResult(0), {value, poison});
}

void Importer::importMinMax(Operation &op, Predicate choose) {
  checkAttributes(&op, {});
  Symbolic a = lookup(op.getOperand(0)), b = lookup(op.getOperand(1));
  bind(op.getResult(0), {terms.ite(terms.icmp(choose, a.value, b.value), a.value,
                                   b.value),
                         terms.logicalOr(a.poison, b.poison)});
}

void Importer::importLoad(memref::LoadOp op) {
  checkAttributes(op, {});
  unsigned root = rootOf(op.getMemRef());
  auto index = checkedIndices(op.getMemRef(), op.getIndices());
  Term value = terms.load(memory[root], index);
  bind(op.getResult(), {value, terms.boolean(false)});
  loadEffect[op.getResult()] = effects.size();
  MemoryEffect effect;
  effect.kind = MemoryEffect::Read;
  effect.root = root;
  effect.opIndex = opCounter;
  effect.index = index;
  effect.value = value;
  effects.push_back(effect);
}

void Importer::importStore(memref::StoreOp op) {
  checkAttributes(op, {});
  unsigned root = rootOf(op.getMemRef());
  auto index = checkedIndices(op.getMemRef(), op.getIndices());
  Symbolic stored = lookup(op.getValue());
  side(SideKind::ModelLimit, "poison reaches memory", stored.poison);
  memory[root] = terms.store(memory[root], index, stored.value);
  MemoryEffect effect;
  effect.kind = MemoryEffect::Write;
  effect.root = root;
  effect.opIndex = opCounter;
  effect.index = index;
  effect.value = stored.value;
  effects.push_back(effect);
}

void Importer::importOperation(Operation &op) {
  opCounter = opIndex[&op];
  for (Value result : op.getResults())
    if (!isa<BaseMemRefType>(result.getType()))
      (void)sortOf(result.getType());
  llvm::TypeSwitch<Operation *>(&op)
      .Case([&](arith::ConstantOp o) { importConstant(o); })
      .Case([&](arith::AddIOp) { importIntegerBinary(op, TermOp::Add); })
      .Case([&](arith::SubIOp) { importIntegerBinary(op, TermOp::Sub); })
      .Case([&](arith::MulIOp) { importIntegerBinary(op, TermOp::Mul); })
      .Case([&](arith::AndIOp) { importIntegerBinary(op, TermOp::And); })
      .Case([&](arith::OrIOp) { importIntegerBinary(op, TermOp::Or); })
      .Case([&](arith::XOrIOp) { importIntegerBinary(op, TermOp::Xor); })
      .Case<arith::DivSIOp, arith::DivUIOp, arith::RemSIOp, arith::RemUIOp>(
          [&](Operation *) { importDivision(op); })
      .Case([&](arith::ShLIOp) { importShift(op, TermOp::Shl); })
      .Case([&](arith::ShRUIOp) { importShift(op, TermOp::LShr); })
      .Case([&](arith::ShRSIOp) { importShift(op, TermOp::AShr); })
      .Case<arith::ExtSIOp, arith::ExtUIOp, arith::TruncIOp, arith::IndexCastOp,
            arith::IndexCastUIOp>([&](Operation *) { importCast(op); })
      .Case([&](arith::AddFOp o) { importFloat(op, TermOp::FAdd, o.getFastmath()); })
      .Case([&](arith::MulFOp o) { importFloat(op, TermOp::FMul, o.getFastmath()); })
      .Case([&](arith::MinSIOp) { importMinMax(op, Predicate::Slt); })
      .Case([&](arith::MaxSIOp) { importMinMax(op, Predicate::Sgt); })
      .Case([&](arith::MinUIOp) { importMinMax(op, Predicate::Ult); })
      .Case([&](arith::MaxUIOp) { importMinMax(op, Predicate::Ugt); })
      .Case([&](arith::CmpIOp o) {
        checkAttributes(&op, {"predicate"});
        static const std::pair<arith::CmpIPredicate, Predicate> table[] = {
            {arith::CmpIPredicate::eq, Predicate::Eq},
            {arith::CmpIPredicate::ne, Predicate::Ne},
            {arith::CmpIPredicate::slt, Predicate::Slt},
            {arith::CmpIPredicate::sle, Predicate::Sle},
            {arith::CmpIPredicate::sgt, Predicate::Sgt},
            {arith::CmpIPredicate::sge, Predicate::Sge},
            {arith::CmpIPredicate::ult, Predicate::Ult},
            {arith::CmpIPredicate::ule, Predicate::Ule},
            {arith::CmpIPredicate::ugt, Predicate::Ugt},
            {arith::CmpIPredicate::uge, Predicate::Uge}};
        Symbolic a = lookup(o.getLhs()), b = lookup(o.getRhs());
        for (auto [source, target] : table)
          if (source == o.getPredicate()) {
            bind(o.getResult(), {terms.icmp(target, a.value, b.value),
                                 terms.logicalOr(a.poison, b.poison)});
            return;
          }
        refuse(UnsupportedKind::UnsupportedOperation, "comparison predicate");
      })
      .Case([&](arith::SelectOp o) {
        checkAttributes(&op, {});
        Symbolic c = lookup(o.getCondition()), a = lookup(o.getTrueValue()),
                 b = lookup(o.getFalseValue());
        // Only the selected arm's poison reaches the result.
        bind(o.getResult(),
             {terms.ite(c.value, a.value, b.value),
              terms.logicalOr(c.poison, terms.ite(c.value, a.poison, b.poison))});
      })
      .Case([&](memref::LoadOp o) { importLoad(o); })
      .Case([&](memref::StoreOp o) { importStore(o); })
      .Case<memref::AllocOp, memref::AllocaOp>([&](Operation *) {})
      .Case([&](memref::CastOp o) {
        checkAttributes(&op, {});
        auto source = dyn_cast<MemRefType>(o.getSource().getType());
        auto target = dyn_cast<MemRefType>(o.getResult().getType());
        if (!source || !target || !source.getLayout().isIdentity() ||
            !target.getLayout().isIdentity() || source.getMemorySpace() ||
            target.getMemorySpace() || source.getShape() != target.getShape())
          refuse(UnsupportedKind::UnsupportedView,
                 "memref.cast changes the shape, layout or memory space");
      })
      .Case([&](memref::DimOp o) {
        checkAttributes(&op, {});
        auto constant = o.getConstantIndex();
        if (!constant)
          refuse(UnsupportedKind::DynamicDimensionIndex, "memref.dim");
        const MemoryRoot &root = program.roots[rootOf(o.getSource())];
        if ((size_t)*constant >= root.extents.size())
          refuse(UnsupportedKind::DynamicDimensionIndex, "memref.dim range");
        bind(o.getResult(), {extent(root, *constant), terms.boolean(false)});
      })
      .Case<cf::BranchOp, cf::CondBranchOp, cf::SwitchOp, func::ReturnOp>(
          [&](Operation *) {})
      .Default([&](Operation *) {
        if (op.hasTrait<OpTrait::IsTerminator>())
          refuse(UnsupportedKind::UnsupportedTerminator,
                 op.getName().getStringRef().str());
        if (op.getNumResults() && isa<BaseMemRefType>(op.getResult(0).getType()))
          refuse(UnsupportedKind::UnsupportedView,
                 op.getName().getStringRef().str());
        refuse(UnsupportedKind::UnsupportedOperation,
               op.getName().getStringRef().str());
      });
}

void Importer::finishTerminator(Operation *terminator) {
  unsigned location = locationOf[terminator->getBlock()];
  opCounter = opIndex[terminator];

  struct Exit {
    unsigned to, slot;
    Term guard;
    ValueRange operands;
    Block *target;
  };
  llvm::SmallVector<Exit, 0> exits;
  Term yes = terms.boolean(true);

  if (auto br = dyn_cast<cf::BranchOp>(terminator)) {
    checkAttributes(br, {});
    exits.push_back({locationOf[br.getDest()], 0, yes, br.getDestOperands(),
                     br.getDest()});
  } else if (auto cond = dyn_cast<cf::CondBranchOp>(terminator)) {
    checkAttributes(cond, {});
    Symbolic c = lookup(cond.getCondition());
    side(SideKind::UndefinedBehavior, "branch on poison", c.poison);
    exits.push_back({locationOf[cond.getTrueDest()], 0, c.value,
                     cond.getTrueDestOperands(), cond.getTrueDest()});
    exits.push_back({locationOf[cond.getFalseDest()], 1,
                     terms.logicalNot(c.value), cond.getFalseDestOperands(),
                     cond.getFalseDest()});
  } else if (auto sw = dyn_cast<cf::SwitchOp>(terminator)) {
    checkAttributes(sw, {"case_values", "case_operand_segments"});
    Symbolic flag = lookup(sw.getFlag());
    side(SideKind::UndefinedBehavior, "switch on poison", flag.poison);
    Term noneSoFar = yes;
    llvm::SmallVector<Exit, 0> cases;
    if (auto values = sw.getCaseValues()) {
      unsigned slot = 1;
      // LLVM's verifier rejects a repeated case, so it has no lowered meaning.
      std::set<uint64_t> distinct;
      for (APInt value : values->getValues<APInt>())
        if (!distinct.insert(value.getZExtValue()).second)
          refuse(UnsupportedKind::UnsupportedTerminator,
                 "cf.switch with a repeated case");
      for (APInt value : values->getValues<APInt>()) {
        Term equal = terms.icmp(Predicate::Eq, flag.value, terms.bv(value));
        cases.push_back({locationOf[sw.getCaseDestinations()[slot - 1]], slot,
                         terms.logicalAnd(noneSoFar, equal),
                         sw.getCaseOperands(slot - 1),
                         sw.getCaseDestinations()[slot - 1]});
        noneSoFar = terms.logicalAnd(noneSoFar, terms.logicalNot(equal));
        ++slot;
      }
    }
    exits.push_back({locationOf[sw.getDefaultDestination()], 0, noneSoFar,
                     sw.getDefaultOperands(), sw.getDefaultDestination()});
    exits.append(cases.begin(), cases.end());
  } else if (auto ret = dyn_cast<func::ReturnOp>(terminator)) {
    checkAttributes(ret, {});
    exits.push_back({program.exitLocation(), 0, yes, ret.getOperands(), nullptr});
  } else {
    refuse(UnsupportedKind::UnsupportedTerminator,
           terminator->getName().getStringRef().str());
  }

  // Results that later blocks read, as of the end of this block.
  llvm::SmallVector<std::pair<unsigned, Symbolic>> crossing;
  for (Operation &op : *terminator->getBlock())
    for (Value result : op.getResults()) {
      auto state = stateOf.find(result);
      if (state != stateOf.end())
        crossing.push_back({state->second, environment.lookup(result)});
    }

  llvm::SmallVector<std::pair<unsigned, Term>> memoryUpdates;
  for (unsigned r = 0; r < program.roots.size(); ++r)
    if (memory[r]->op != TermOp::MemVar)
      memoryUpdates.push_back({r, memory[r]});

  for (Exit &exit : exits) {
    Transition t;
    t.from = location;
    t.slot = exit.slot;
    t.to = exit.to;
    t.guard = exit.guard;
    t.sideConditions = sides;
    t.effects = effects;
    t.memoryUpdates = memoryUpdates;
    for (auto &[id, symbolic] : crossing) {
      t.assignments.push_back({id, symbolic.value});
      t.shadowAssignments.push_back({id, symbolic.poison});
    }
    if (exit.target) {
      for (auto [argument, operand] :
           llvm::zip(exit.target->getArguments(), exit.operands)) {
        unsigned id = stateOf.lookup(argument);
        Symbolic value = lookup(operand);
        t.assignments.push_back({id, value.value});
        t.shadowAssignments.push_back({id, value.poison});
      }
    } else {
      for (Value operand : exit.operands) {
        Symbolic value = lookup(operand);
        t.returns.push_back(value.value);
        t.returnPoison.push_back(value.poison);
      }
    }
    program.transitions.push_back(std::move(t));
  }
}

void Importer::importBlock(Block *block) {
  environment.clear();
  memory.clear();
  sides.clear();
  effects.clear();
  loadEffect.clear();
  for (unsigned r = 0; r < program.roots.size(); ++r)
    memory[r] = terms.memVar(program.roots[r].sort);
  for (Operation &op : *block)
    importOperation(op);
  finishTerminator(block->getTerminator());
  detectSites(block);
}

/// C[h] := C[h'] + A[f] * B[g], or s := s + A[f] * B[g], in one block.
void Importer::detectSites(Block *block) {
  unsigned location = locationOf[block];

  struct Product {
    Value a, b;
    unsigned flags;
    ReductionOp op;
  };
  auto productOf = [&](Value value) -> std::optional<Product> {
    Operation *def = value.getDefiningOp();
    if (!def || def->getBlock() != block)
      return std::nullopt;
    Value lhs, rhs;
    unsigned flags;
    ReductionOp kind;
    if (auto m = dyn_cast<arith::MulFOp>(def)) {
      lhs = m.getLhs(); rhs = m.getRhs(); kind = ReductionOp::FAdd;
      flags = fastMathBits(m.getFastmath());
    } else if (auto m = dyn_cast<arith::MulIOp>(def)) {
      lhs = m.getLhs(); rhs = m.getRhs(); kind = ReductionOp::Add;
      flags = overflowBits(m.getOverflowFlags());
    } else {
      return std::nullopt;
    }
    if (!loadEffect.count(lhs) || !loadEffect.count(rhs))
      return std::nullopt;
    return Product{lhs, rhs, flags, kind};
  };
  auto access = [&](Value load) {
    auto op = cast<memref::LoadOp>(load.getDefiningOp());
    const MemoryEffect &effect = effects[loadEffect[load]];
    (void)op;
    return MacAccess{effect.root, effect.index};
  };

  auto fill = [&](MacSite &site, const Product &product, Operation *add) {
    site.id = program.sites.size();
    site.location = location;
    site.opIndex = opIndex[add];
    site.reduction = product.op;
    site.multiplyFlags = product.flags;
    if (auto a = dyn_cast<arith::AddFOp>(add))
      site.reductionFlags = fastMathBits(a.getFastmath());
    else
      site.reductionFlags = overflowBits(cast<arith::AddIOp>(add).getOverflowFlags());
    site.a = access(product.a);
    site.b = access(product.b);
    site.aValue = environment.lookup(product.a).value;
    site.bValue = environment.lookup(product.b).value;
    site.updateValue = environment.lookup(add->getResult(0)).value;
  };

  for (Operation &op : *block) {
    if (!isa<arith::AddFOp, arith::AddIOp>(op))
      continue;
    Value left = op.getOperand(0), right = op.getOperand(1);
    auto product = productOf(right);
    Value other = left;
    if (!product) {
      product = productOf(left);
      other = right;
    }
    if (!product ||
        (product->op == ReductionOp::FAdd) != isa<arith::AddFOp>(op))
      continue;

    // Direct: the sum is stored, and the other addend is a load.
    memref::StoreOp store;
    for (Operation *user : op.getResult(0).getUsers())
      if (auto s = dyn_cast<memref::StoreOp>(user))
        if (s->getBlock() == block && s.getValue() == op.getResult(0)) {
          store = s;
          break;
        }
    if (store && loadEffect.count(other)) {
      MacSite site;
      fill(site, *product, &op);
      site.form = ReductionForm::Direct;
      site.outputRead = access(other);
      site.accumulatedValue = environment.lookup(other).value;
      for (unsigned i = 0; i < effects.size(); ++i)
        if (effects[i].kind == MemoryEffect::Write &&
            effects[i].opIndex == opIndex[store]) {
          site.effectIndex = i;
          site.output = MacAccess{effects[i].root, effects[i].index};
        }
      program.sites.push_back(site);
      continue;
    }

    // Scalar accumulator: the other addend is loop-carried state.
    auto argument = dyn_cast<BlockArgument>(other);
    if (argument && stateOf.count(argument) &&
        program.state[stateOf[argument]].loopCarried) {
      MacSite site;
      fill(site, *product, &op);
      site.form = ReductionForm::ScalarAccumulator;
      site.accumulator = stateOf[argument];
      site.accumulatedValue = lookup(other).value;
      site.effectIndex =
          std::max(loadEffect[product->a], loadEffect[product->b]);
      program.sites.push_back(site);
    }
  }
}

void Importer::computeObliviousness() {
  std::vector<bool> tainted(program.state.size(), false);
  auto depends = [&](Term root) {
    if (containsOp(root, TermOp::Load))
      return true;
    for (unsigned id : stateSupport(root))
      if (tainted[id])
        return true;
    if (containsOp(root, TermOp::Shadow)) {
      llvm::DenseSet<Term> seen;
      std::function<bool(Term)> visit = [&](Term t) {
        if (!seen.insert(t).second)
          return false;
        if (t->op == TermOp::Shadow && tainted[t->index])
          return true;
        return llvm::any_of(t->args, visit);
      };
      return visit(root);
    }
    return false;
  };
  for (bool changed = true; changed;) {
    changed = false;
    for (const Transition &t : program.transitions) {
      auto scan = [&](const llvm::SmallVector<std::pair<unsigned, Term>> &list) {
        for (auto &[id, term] : list)
          if (!tainted[id] && depends(term)) {
            tainted[id] = true;
            changed = true;
          }
      };
      scan(t.assignments);
      scan(t.shadowAssignments);
    }
  }
  program.controlOblivious = true;
  program.addressOblivious = true;
  for (const Transition &t : program.transitions) {
    if (depends(t.guard))
      program.controlOblivious = false;
    for (const MemoryEffect &effect : t.effects)
      for (Term index : effect.index)
        if (depends(index))
          program.addressOblivious = false;
  }
  // Experimental: one level of gather from read-only roots. A root is read-only when no
  // block writes it; an address is a gather when its only memory dependence is a Load
  // from such a root's entry memory at an index that itself depends on nothing (no load,
  // no tainted state, no shadow).
  if (!program.addressOblivious) {
    std::vector<bool> written(program.roots.size(), false);
    for (const Transition &t : program.transitions) {
      for (const MemoryEffect &effect : t.effects)
        if (effect.kind == MemoryEffect::Write)
          written[effect.root] = true;
      for (const auto &[root, memoryTerm] : t.memoryUpdates)
        if (memoryTerm->op != TermOp::MemVar)
          written[root] = true;
    }
    std::set<unsigned> gathers;
    bool ok = true;
    std::function<bool(Term)> gatherOnly = [&](Term t) -> bool {
      if (t->op == TermOp::Load) {
        Term mem = t->args[0];
        if (mem->op != TermOp::MemVar || written[mem->sort.root])
          return false;
        for (unsigned i = 1; i < t->args.size(); ++i)
          if (depends(t->args[i]))
            return false;
        gathers.insert(mem->sort.root);
        return true;
      }
      if (t->op == TermOp::Shadow || t->op == TermOp::Store)
        return !depends(t);
      if (t->op == TermOp::State)
        return !tainted[t->index];
      return llvm::all_of(t->args, gatherOnly);
    };
    for (const Transition &t : program.transitions)
      for (const MemoryEffect &effect : t.effects)
        for (Term index : effect.index)
          if (depends(index) && !gatherOnly(index))
            ok = false;
    // dp[dp[i]] is refused above: the inner load's index depends on memory.
    program.addressGather = ok && !gathers.empty();
    if (program.addressGather)
      program.gatherRoots.assign(gathers.begin(), gathers.end());
  }
  for (MacSite &site : program.sites) {
    site.oblivious = program.controlOblivious && program.addressOblivious;
    std::set<unsigned> coordinates;
    auto add = [&](const MacAccess &access) {
      for (Term index : access.index)
        for (unsigned id : stateSupport(index))
          coordinates.insert(id);
    };
    add(site.a);
    add(site.b);
    if (site.output) add(*site.output);
    if (site.outputRead) add(*site.outputRead);
    site.coordinates.assign(coordinates.begin(), coordinates.end());
  }
}

void Importer::run() {
  layoutBlocks();
  layoutParameters();
  layoutAllocations();
  layoutRootPairs();
  layoutState();
  for (Block *block : blocks)
    importBlock(block);
  computeObliviousness();
}

} // namespace

namespace mlir::generalized_gemm {

SemanticProgram importFunction(func::FuncOp function,
                               const ImportOptions &options) {
  SemanticProgram program;
  program.terms = std::make_shared<TermContext>(options.limits.maxTerms);
  program.functionName = function.getSymName().str();
  program.indexBitWidth = options.indexBitWidth;
  program.structuredLoweringApplied = options.lowerStructured;
  program.admitGather = options.admitGather;
  program.fingerprint = fingerprintOf(function, options);

  auto fail = [&](UnsupportedKind kind, std::string detail) {
    SemanticProgram refused;
    refused.terms = program.terms;
    refused.functionName = program.functionName;
    refused.fingerprint = program.fingerprint;
    refused.indexBitWidth = program.indexBitWidth;
    refused.unsupported = UnsupportedReason{kind, std::move(detail)};
    return refused;
  };

  if (failed(mlir::verify(function)))
    return fail(UnsupportedKind::NotVerified, "the function does not verify");
  if (function.isExternal())
    return fail(UnsupportedKind::ExternalFunction, "no body");

  try {
    auto parent = function->getParentOfType<ModuleOp>();
    if (parent)
      for (NamedAttribute attr : parent->getAttrs())
        if (attr.getName().strref() != "sym_name")
          refuse(UnsupportedKind::UnsupportedAttribute,
                 "module: " + attr.getName().strref().str());
    checkFunctionAttributes(function);

    // Everything below rewrites this private copy only (invariant I7).
    OwningOpRef<ModuleOp> copy;
    func::FuncOp target;
    if (parent) {
      copy = parent.clone();
      target = copy->lookupSymbol<func::FuncOp>(function.getSymName());
    } else {
      copy = ModuleOp::create(function.getLoc());
      target = function.clone();
      copy->push_back(target);
    }
    if (options.lowerStructured) {
      PassManager manager(function->getContext(), "builtin.module");
      manager.enableVerifier(true);
      manager.addPass(createLowerAffinePass());
      manager.addPass(createSCFToControlFlowPass());
      if (failed(manager.run(*copy)))
        refuse(UnsupportedKind::LoweringFailed, "structured lowering failed");
    }
    inlineCalls(target, *copy, options.limits);
    if (failed(mlir::verify(*copy)))
      refuse(UnsupportedKind::LoweringFailed, "the rewritten copy does not verify");
    Importer(target, options, program).run();
    return program;
  } catch (const Refusal &refusal) {
    return fail(refusal.kind, refusal.detail);
  } catch (const TermError &error) {
    return fail(UnsupportedKind::ResourceLimit, error.what());
  }
}

bool gatherLoadsOnly(const SemanticProgram &program, Term term) {
  if (term->op == TermOp::Load) {
    Term mem = term->args[0];
    if (mem->op != TermOp::MemVar || !isGatherRoot(program, mem->sort.root))
      return false;
    for (unsigned i = 1; i < term->args.size(); ++i)
      if (containsOp(term->args[i], TermOp::Load))
        return false;
    return true;
  }
  return llvm::all_of(term->args, [&](Term arg) { return gatherLoadsOnly(program, arg); });
}

uint64_t gatherFillModulus(const SemanticProgram &program) {
  uint64_t modulus = 0;
  for (const Transition &t : program.transitions)
    for (const MemoryEffect &effect : t.effects)
      for (unsigned d = 0; d < effect.index.size(); ++d) {
        if (!containsOp(effect.index[d], TermOp::Load))
          continue;
        int64_t extent = program.roots[effect.root].extents[d];
        if (extent > 0 && (modulus == 0 || (uint64_t)extent < modulus))
          modulus = extent;
      }
  return modulus ? modulus : 1;
}

bool matchesSource(const SemanticProgram &program, func::FuncOp function,
                   const ImportOptions &options) {
  return program.fingerprint == fingerprintOf(function, options);
}

} // namespace mlir::generalized_gemm
