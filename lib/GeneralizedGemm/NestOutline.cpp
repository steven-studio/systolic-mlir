//===- NestOutline.cpp - One analysis function per loop nest -------------===//
//
// See NestOutline.h. Nothing here judges a program: it copies a nest, says what the copy
// stands for, and refuses in words what it cannot copy faithfully.
//
//===----------------------------------------------------------------------===//
#include "GeneralizedGemm/NestOutline.h"

#include "Systolic/NestBinding.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/CallInterfaces.h"
#include "mlir/Interfaces/LoopLikeInterface.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/raw_ostream.h"

using namespace mlir;
using namespace mlir::generalized_gemm;

namespace {

std::string typeText(Type type) {
  std::string text;
  llvm::raw_string_ostream stream(text);
  type.print(stream);
  return text;
}

/// A file position as file:line:column, and any other location as it prints.
std::string locationText(Location location, unsigned *line = nullptr, unsigned *column = nullptr) {
  if (auto position = dyn_cast<FileLineColLoc>(location)) {
    if (line)
      *line = position.getLine();
    if (column)
      *column = position.getColumn();
    return (position.getFilename().str() + ":" + std::to_string(position.getLine()) + ":" +
            std::to_string(position.getColumn()));
  }
  std::string text;
  llvm::raw_string_ostream stream(text);
  location.print(stream);
  return text;
}

bool isLoop(Operation *op) { return isa<LoopLikeOpInterface>(op); }
bool isAllocation(Operation *op) { return isa<memref::AllocOp, memref::AllocaOp>(op); }

/// What `op` does to the memref it takes as operand `index`, by the operation alone.
/// Anything that is no load, store or shape query may read and write it.
unsigned accessOf(Operation *op, unsigned index) {
  if (isa<memref::LoadOp, affine::AffineLoadOp>(op))
    return NestReads;
  if (isa<memref::StoreOp, affine::AffineStoreOp>(op))
    return index == 1 ? NestWrites : NestReads | NestWrites;
  if (isa<memref::DimOp, memref::RankOp>(op))
    return 0;
  return NestReads | NestWrites;
}

/// Whether a single operation, with no region, may touch memory in a way its memref
/// operands do not show.
bool hasOpaqueEffect(Operation *op) {
  if (isa<memref::LoadOp, affine::AffineLoadOp, memref::StoreOp, affine::AffineStoreOp>(op))
    return false;
  if (isAllocation(op) || isa<memref::DimOp, memref::RankOp>(op))
    return false;
  if (op->hasTrait<OpTrait::IsTerminator>())
    return false;
  return !isMemoryEffectFree(op);
}

struct Outliner {
  func::FuncOp function;
  const Authorizations &authorized;
  FunctionOutline &out;
  llvm::DenseMap<Value, unsigned> rootOf;
  std::vector<Operation *> loops;
  std::vector<bool> opaque; // Per nest: effects that are no access to a root.

  Outliner(func::FuncOp function, const Authorizations &authorized, FunctionOutline &out)
      : function(function), authorized(authorized), out(out) {}

  void numberRoots() {
    Block &entry = function.getBody().front();
    for (BlockArgument argument : entry.getArguments()) {
      if (!isa<MemRefType>(argument.getType()))
        continue;
      FunctionRoot root;
      root.origin = FunctionRoot::FunctionArgument;
      root.id = out.roots.size();
      root.argument = argument.getArgNumber();
      root.restrictArgument = (bool)function.getArgAttr(root.argument, "llvm.noalias");
      root.type = typeText(argument.getType());
      root.location = "argument " + std::to_string(root.argument);
      rootOf[argument] = root.id;
      out.roots.push_back(std::move(root));
    }
    function.walk<WalkOrder::PreOrder>([&](Operation *op) {
      if (!isAllocation(op))
        return;
      FunctionRoot root;
      root.origin = FunctionRoot::FreshAllocation;
      root.id = out.roots.size();
      root.type = typeText(op->getResult(0).getType());
      root.location = locationText(op->getLoc());
      for (Operation *parent = op->getParentOp(); parent && parent != function.getOperation();
           parent = parent->getParentOp())
        root.insideLoop = root.insideLoop || isLoop(parent);
      rootOf[op->getResult(0)] = root.id;
      out.roots.push_back(std::move(root));
    });
  }

  NestSeparation separation(unsigned a, unsigned b) const {
    NestSeparation s;
    s.first = std::min(a, b);
    s.second = std::max(a, b);
    s.name = separationName(s.first, s.second);
    const FunctionRoot &x = out.roots[s.first], &y = out.roots[s.second];
    if (x.origin == FunctionRoot::FreshAllocation || y.origin == FunctionRoot::FreshAllocation) {
      s.apart = true;
      s.proof = true;
      s.basis = "fresh allocation";
    } else if (authorized.count(s.name)) {
      s.apart = true;
      s.basis = x.restrictArgument || y.restrictArgument
                    ? "authorized, with an llvm.noalias source contract"
                    : "authorized";
    }
    return s;
  }

  /// The body's statements in order: the nests, and what touches memory outside them.
  /// False when a region has more than one block, where loops are no operations.
  bool visit(Block &block, unsigned statement, unsigned conditions, bool top) {
    unsigned index = 0;
    for (Operation &op : block) {
      unsigned at = top ? index++ : statement;
      OrderEntry entry;
      entry.statement = at;
      entry.conditions = conditions;
      entry.operation = op.getName().getStringRef().str();
      entry.location = locationText(op.getLoc());
      if (isLoop(&op)) {
        entry.kind = OrderEntry::Nest;
        entry.nest = loops.size();
        loops.push_back(&op);
        out.order.push_back(std::move(entry));
        continue;
      }
      if (op.getNumRegions()) {
        for (Region &region : op.getRegions()) {
          if (region.empty())
            continue;
          if (!region.hasOneBlock())
            return false;
          if (!visit(region.front(), at, conditions + 1, false))
            return false;
        }
        continue;
      }
      if (isAllocation(&op)) {
        entry.kind = OrderEntry::Allocation;
        entry.root = rootOf.lookup(op.getResult(0));
        out.order.push_back(std::move(entry));
        continue;
      }
      bool listed = false;
      for (unsigned i = 0, e = op.getNumOperands(); i != e; ++i) {
        auto found = rootOf.find(op.getOperand(i));
        if (found == rootOf.end())
          continue;
        unsigned access = accessOf(&op, i);
        if (!access)
          continue;
        OrderEntry one = entry;
        one.kind = OrderEntry::Access;
        one.root = found->second;
        one.access = access;
        out.order.push_back(std::move(one));
        listed = true;
      }
      if (hasOpaqueEffect(&op) || (!listed && !isMemoryEffectFree(&op) &&
                                   !op.hasTrait<OpTrait::IsTerminator>())) {
        entry.kind = OrderEntry::Opaque;
        out.order.push_back(std::move(entry));
      }
    }
    return true;
  }

  void outline(Operation *loop, OutlinedNest &nest) {
    // Everything the nest reads and does not define, in order of first use, and what
    // its operations do to each root.
    std::vector<Value> liveIns;
    llvm::DenseSet<Value> seen;
    llvm::DenseMap<unsigned, unsigned> accessByRoot;
    std::string call, foreignMemref, madeMemref, parallel;
    bool effects = false;
    auto inside = [&](Value value) {
      Block *block = value.getParentBlock();
      Operation *parent = block ? block->getParentOp() : nullptr;
      return parent && loop->isAncestor(parent);
    };
    auto use = [&](Operation *op) {
      for (unsigned i = 0, e = op->getNumOperands(); i != e; ++i) {
        Value value = op->getOperand(i);
        if (inside(value))
          continue;
        if (seen.insert(value).second)
          liveIns.push_back(value);
        auto found = rootOf.find(value);
        if (found != rootOf.end())
          accessByRoot[found->second] |= op == loop ? NestReads | NestWrites : accessOf(op, i);
      }
    };
    loop->walk<WalkOrder::PreOrder>([&](Operation *op) {
      use(op);
      // The iterations of a parallel loop have no order, and every claim here is about
      // the nest's own order.
      llvm::StringRef name = op->getName().getStringRef();
      if ((name == "scf.parallel" || name == "scf.forall" || name == "affine.parallel") && parallel.empty())
        parallel = name.str();
      // A memref the nest makes for itself, other than by allocating it, stands for
      // memory nobody here can name: a view, a global, a memref carried round the loop.
      for (Region &region : op->getRegions())
        for (Block &block : region)
          for (BlockArgument argument : block.getArguments())
            if (isa<BaseMemRefType>(argument.getType()) && madeMemref.empty())
              madeMemref = "a block argument";
      if (op == loop)
        return;
      for (Value result : op->getResults())
        if (isa<BaseMemRefType>(result.getType()) && !isAllocation(op) && madeMemref.empty())
          madeMemref = "the result of " + op->getName().getStringRef().str();
      if (isa<CallOpInterface>(op) && call.empty())
        call = op->getName().getStringRef().str();
      if (!op->getNumRegions() && hasOpaqueEffect(op))
        effects = true;
    });

    // The roots first, in the function's order, then the scalars: arguments of the
    // function in their order, then the values computed before the nest.
    std::vector<Value> rootValues(out.roots.size()), argumentScalars, computedScalars, constants;
    std::string refusal;
    Block &entryOfFunction = function.getBody().front();
    for (Value value : liveIns) {
      Type type = value.getType();
      if (isa<BaseMemRefType>(type)) {
        auto found = rootOf.find(value);
        if (found == rootOf.end()) {
          Operation *def = value.getDefiningOp();
          foreignMemref = def ? "the result of " + def->getName().getStringRef().str()
                              : std::string("a block argument that is no argument of the function");
          if (isa<UnrankedMemRefType>(type))
            foreignMemref = "unranked";
          continue;
        }
        rootValues[found->second] = value;
        continue;
      }
      if (!isa<IntegerType, IndexType, FloatType>(type)) {
        if (refusal.empty())
          refusal = "a value of type " + typeText(type) + " enters the nest, and only scalars and memory roots are inputs";
        continue;
      }
      Operation *def = value.getDefiningOp();
      if (def && isa<arith::ConstantOp>(def))
        constants.push_back(value);
      else if (!def && value.getParentBlock() == &entryOfFunction)
        argumentScalars.push_back(value);
      else
        computedScalars.push_back(value);
    }
    llvm::stable_sort(argumentScalars, [](Value a, Value b) {
      return cast<BlockArgument>(a).getArgNumber() < cast<BlockArgument>(b).getArgNumber();
    });

    for (unsigned id = 0; id < out.roots.size(); ++id) {
      if (!rootValues[id])
        continue;
      NestRoot root;
      root.root = id;
      root.outlined = nest.roots.size();
      root.access = accessByRoot.lookup(id);
      nest.roots.push_back(root);
    }
    for (unsigned a = 0; a < nest.roots.size(); ++a)
      for (unsigned b = a + 1; b < nest.roots.size(); ++b) {
        NestSeparation s = separation(nest.roots[a].root, nest.roots[b].root);
        s.outlinedName = separationName(nest.roots[a].outlined, nest.roots[b].outlined);
        nest.separations.push_back(std::move(s));
      }
    opaque.push_back(effects || !call.empty() || !foreignMemref.empty() || !madeMemref.empty());

    // What cannot be kept.
    for (OpResult result : loop->getResults())
      if (!result.use_empty()) {
        nest.unsupported = "the loop yields a value that is used after it (result " +
                           std::to_string(result.getResultNumber()) + " of " + nest.loop +
                           "): the nest's effect is not on memory alone";
        return;
      }
    if (!parallel.empty()) {
      nest.unsupported = "the nest holds a parallel loop (" + parallel +
                         "), whose iterations have no order of their own";
      return;
    }
    if (!call.empty()) {
      nest.unsupported = "the nest calls a function (" + call + "), and a callee is not outlined";
      return;
    }
    if (!foreignMemref.empty()) {
      nest.unsupported = foreignMemref == "unranked"
                             ? std::string("the nest uses an unranked memref, which is no root of the function")
                             : "the nest uses a memref that is " + foreignMemref +
                                   ", which is no root of the function";
      return;
    }
    if (!madeMemref.empty()) {
      nest.unsupported = "the nest makes a memref that is " + madeMemref +
                         " and no allocation, which is no root of the function";
      return;
    }
    if (!refusal.empty()) {
      nest.unsupported = refusal;
      return;
    }

    // The copy.
    MLIRContext *context = function->getContext();
    Location location = loop->getLoc();
    std::vector<Type> inputs;
    std::vector<Value> inputValues;
    for (const NestRoot &root : nest.roots) {
      inputs.push_back(rootValues[root.root].getType());
      inputValues.push_back(rootValues[root.root]);
    }
    for (Value value : argumentScalars) {
      NestScalar scalar;
      scalar.source = NestScalar::FunctionArgument;
      scalar.argument = cast<BlockArgument>(value).getArgNumber();
      scalar.outlined = inputs.size();
      scalar.type = typeText(value.getType());
      scalar.definition = "argument " + std::to_string(scalar.argument);
      nest.scalars.push_back(std::move(scalar));
      inputs.push_back(value.getType());
      inputValues.push_back(value);
    }
    for (Value value : computedScalars) {
      NestScalar scalar;
      scalar.source = NestScalar::ComputedValue;
      scalar.outlined = inputs.size();
      scalar.type = typeText(value.getType());
      if (Operation *def = value.getDefiningOp()) {
        scalar.definition = def->getName().getStringRef().str();
        scalar.location = locationText(def->getLoc());
      } else {
        scalar.definition = "a block argument";
      }
      nest.scalars.push_back(std::move(scalar));
      inputs.push_back(value.getType());
      inputValues.push_back(value);
    }

    nest.outlinedName = nest.function + "__nest" + std::to_string(nest.ordinal);
    OwningOpRef<ModuleOp> module(ModuleOp::create(location));
    // The module's and the function's own attributes go with the copy, so that the
    // importer's refusal of an attribute it does not know still applies to the nest.
    if (auto parent = function->getParentOfType<ModuleOp>())
      for (NamedAttribute attr : parent->getAttrs())
        module->getOperation()->setAttr(attr.getName(), attr.getValue());
    func::FuncOp copy =
        func::FuncOp::create(location, nest.outlinedName, FunctionType::get(context, inputs, {}));
    for (NamedAttribute attr : function->getAttrs()) {
      llvm::StringRef name = attr.getName().strref();
      if (name == "sym_name" || name == "function_type" || name == "arg_attrs" || name == "res_attrs")
        continue;
      copy->setAttr(attr.getName(), attr.getValue());
    }
    module->push_back(copy);
    Block *entry = copy.addEntryBlock();
    IRMapping mapping;
    for (unsigned i = 0; i < inputValues.size(); ++i) {
      mapping.map(inputValues[i], entry->getArgument(i));
      if (auto argument = dyn_cast<BlockArgument>(inputValues[i]))
        if (DictionaryAttr attrs = function.getArgAttrDict(argument.getArgNumber()))
          copy.setArgAttrs(i, attrs);
    }
    for (Value value : constants) {
      Operation *constant = value.getDefiningOp()->clone();
      entry->push_back(constant);
      mapping.map(value, constant->getResult(0));
      ++nest.constantsCopied;
    }
    Operation *body = loop->clone(mapping);
    entry->push_back(body);
    OpBuilder builder(context);
    builder.setInsertionPointToEnd(entry);
    func::ReturnOp::create(builder, location);

    // Nothing in the copy may read the function it was copied from.
    bool escapes = false;
    copy.walk([&](Operation *op) {
      for (Value operand : op->getOperands()) {
        Block *block = operand.getParentBlock();
        Operation *parent = block ? block->getParentOp() : nullptr;
        if (!parent || !copy->isAncestor(parent))
          escapes = true;
      }
    });
    if (escapes) {
      nest.unsupported = "the copy still reads a value of the function";
      nest.outlinedName.clear();
      nest.scalars.clear();
      nest.constantsCopied = 0;
      return;
    }
    bool verifies = false;
    {
      // The refusal below is the report: the verifier's own diagnostic is not printed.
      ScopedDiagnosticHandler quiet(context, [](Diagnostic &) { return success(); });
      verifies = succeeded(verify(*module));
    }
    if (!verifies) {
      nest.unsupported = "the outlined copy does not verify";
      nest.outlinedName.clear();
      nest.scalars.clear();
      nest.constantsCopied = 0;
      return;
    }
    nest.module = std::move(module);
  }

  void run() {
    out.function = function.getSymName().str();
    if (function.isExternal()) {
      out.unsupported = "the function has no body";
      return;
    }
    if (!function.getBody().hasOneBlock()) {
      out.unsupported = "the function's control flow is by branches (its body has more than one block): "
                        "only loops written as operations are outlined";
      return;
    }
    numberRoots();
    if (!visit(function.getBody().front(), 0, 0, true)) {
      out.unsupported = "an operation of the function holds a region with more than one block: "
                        "only loops written as operations are outlined";
      out.order.clear();
      return;
    }
    // Stage 2: the binding of each nest, by the code the production pass uses
    // (Systolic/NestBinding.h). It numbers roots and nests by its own walk; when either
    // numbering differs from the one above, a record would name other roots or another
    // nest than the binding does, and no nest gets a binding.
    mlir::systolic::FunctionRoots shared = mlir::systolic::numberFunctionRoots(function);
    SmallVector<Operation *> sharedLoops = mlir::systolic::outermostLoops(function);
    std::string disagreement;
    if (shared.roots.size() != out.roots.size()) {
      disagreement = "the binding numbers " + std::to_string(shared.roots.size()) + " roots and the outliner " +
                     std::to_string(out.roots.size());
    } else {
      for (unsigned id = 0; id < out.roots.size() && disagreement.empty(); ++id) {
        const FunctionRoot &mine = out.roots[id];
        const mlir::systolic::FunctionRoot &theirs = shared.roots[id];
        bool argument = mine.origin == FunctionRoot::FunctionArgument;
        if (argument != theirs.argument || (argument && (mine.argument != theirs.argNumber ||
                                                         mine.restrictArgument != theirs.noalias)) ||
            mine.type != theirs.type)
          disagreement = "root " + std::to_string(id) + " is another root for the binding";
      }
    }
    if (disagreement.empty() && (sharedLoops.size() != loops.size() ||
                                 !std::equal(loops.begin(), loops.end(), sharedLoops.begin())))
      disagreement = "the binding numbers the function's outermost loops otherwise";

    // The position of each nest, from its entry in the order.
    for (const OrderEntry &entry : out.order) {
      if (entry.kind != OrderEntry::Nest)
        continue;
      Operation *loop = loops[entry.nest];
      OutlinedNest nest;
      nest.ordinal = entry.nest;
      nest.statement = entry.statement;
      nest.conditions = entry.conditions;
      nest.function = out.function;
      nest.loop = loop->getName().getStringRef().str();
      nest.location = locationText(loop->getLoc(), &nest.line, &nest.column);
      if (!disagreement.empty()) {
        nest.bindingRefusal = disagreement;
      } else {
        std::string why;
        FailureOr<mlir::systolic::NestBinding> binding =
            mlir::systolic::computeNestBinding(function, loop, entry.nest, shared, why);
        if (succeeded(binding))
          nest.bindingSha = binding->sha256;
        else
          nest.bindingRefusal = why;
      }
      outline(loop, nest);
      out.nests.push_back(std::move(nest));
    }
    // How the nests meet on memory, by root. A nest with effects that are no access
    // to a root may meet any other.
    for (unsigned p = 0; p < out.nests.size(); ++p)
      for (unsigned q = p + 1; q < out.nests.size(); ++q) {
        if (opaque[p] || opaque[q]) {
          NestDependence dependence;
          dependence.earlier = p;
          dependence.later = q;
          dependence.relation = NestDependence::UnknownEffects;
          out.dependences.push_back(dependence);
          continue;
        }
        for (const NestRoot &a : out.nests[p].roots)
          for (const NestRoot &b : out.nests[q].roots) {
            if (!((a.access | b.access) & NestWrites) || !a.access || !b.access)
              continue;
            bool same = a.root == b.root;
            if (!same && separation(a.root, b.root).apart)
              continue;
            NestDependence dependence;
            dependence.earlier = p;
            dependence.later = q;
            dependence.relation = same ? NestDependence::SameRoot : NestDependence::MayAlias;
            dependence.earlierRoot = a.root;
            dependence.laterRoot = b.root;
            dependence.earlierAccess = a.access;
            dependence.laterAccess = b.access;
            out.dependences.push_back(dependence);
          }
      }
  }
};

} // namespace

namespace mlir::generalized_gemm {

func::FuncOp OutlinedNest::outlined() const {
  if (!module)
    return func::FuncOp();
  return module.get().lookupSymbol<func::FuncOp>(outlinedName);
}

std::string separationName(unsigned first, unsigned second) {
  unsigned low = std::min(first, second), high = std::max(first, second);
  return "roots " + std::to_string(low) + " and " + std::to_string(high) + " are separate";
}

FunctionOutline outlineNests(func::FuncOp function, const Authorizations &authorized) {
  FunctionOutline out;
  Outliner outliner(function, authorized, out);
  outliner.run();
  return out;
}

} // namespace mlir::generalized_gemm
