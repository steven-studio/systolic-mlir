//===- RankPartitionRecognizedMacAdapter.cpp -------------------------------===//
//
// Phase 5.4.3d-3b-4 / 3b-5: from a recognized GEMM (the symbolic rank
// partition analysis) to the complete guarded MAC statement of Phases 1 to
// 3b-3: into a private twin function (3b-4), or in place of the source nest
// (3b-5, transactional). See the header.
//
//===----------------------------------------------------------------------===//

#include "Systolic/RankPartitionRecognizedMacAdapter.h"

#include "Systolic/GeneralAffineMatmul.h"
#include "Systolic/RankPartitionLogicalDomainNormalization.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/IR/Verifier.h"
#include "mlir/Interfaces/DataLayoutInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"

#include <limits>

using namespace mlir;

namespace {

/// The disjointness rule of the production lowering
/// (AffineMatmulToSystolic.cpp, provablyDisjointFromOutput): views are traced
/// to the buffer they view; two views of one buffer may overlap; different
/// buffers are disjoint when one of them is a fresh allocation or a restrict
/// (llvm.noalias) argument, or when the caller assumes no aliasing.
enum class Disjointness { Proven, Overlapping, Unknown };

bool isNoAliasArgument(Value memref) {
  auto arg = dyn_cast<BlockArgument>(memref);
  if (!arg || !arg.getOwner()->isEntryBlock())
    return false;
  auto fn = dyn_cast<FunctionOpInterface>(arg.getOwner()->getParentOp());
  return fn && fn.getArgAttr(arg.getArgNumber(),
                             LLVM::LLVMDialect::getNoAliasAttrName());
}

bool isFreshAllocation(Value memref) {
  return memref.getDefiningOp<memref::AllocOp>() ||
         memref.getDefiningOp<memref::AllocaOp>();
}

Value viewRoot(Value memref) {
  while (auto view = memref.getDefiningOp<ViewLikeOpInterface>())
    memref = view.getViewSource();
  return memref;
}

Disjointness disjointFromOutput(Value input, Value output,
                                bool assumeNoAlias) {
  if (input == output)
    return Disjointness::Overlapping;
  if (assumeNoAlias)
    return Disjointness::Proven;
  Value inputRoot = viewRoot(input);
  Value outputRoot = viewRoot(output);
  if (inputRoot == outputRoot)
    return Disjointness::Overlapping;
  if (isFreshAllocation(inputRoot) || isFreshAllocation(outputRoot))
    return Disjointness::Proven;
  if (isNoAliasArgument(outputRoot) || isNoAliasArgument(inputRoot))
    return Disjointness::Proven;
  return Disjointness::Unknown;
}

} // namespace

// Defined inside the namespace: with `using namespace mlir` in effect the
// unqualified name `systolic` would be ambiguous with mlir::systolic.
namespace systolic::logical {

std::optional<GuardedMacPreflight>
prepareGuardedMac(mlir::Operation *anchor, bool assumeNoAlias,
                  GuardedMacOutcome &outcome, std::string &why) {
  using namespace mlir::presburger;

  auto unknown = [&](const Twine &message)
      -> std::optional<GuardedMacPreflight> {
    outcome = GuardedMacOutcome::Unknown;
    why = ("UNKNOWN: " + message).str();
    return std::nullopt;
  };
  auto invalid = [&](const Twine &message)
      -> std::optional<GuardedMacPreflight> {
    outcome = GuardedMacOutcome::Invalid;
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };

  if (!anchor)
    return unknown("no anchor operation");

  // ------------------------------------------------------------------
  // The recognizer's verified information.
  // ------------------------------------------------------------------

  std::string recognizerWhy;
  auto analysis =
      mlir::systolic::analyzeSymbolicRankPartition(anchor, recognizerWhy);
  if (!analysis)
    return unknown("not a recognized GEMM: " + recognizerWhy);
  if (!analysis->statement)
    return unknown("the recognizer exposed no statement");

  const mlir::systolic::RecognizedMacStatement &statement =
      *analysis->statement;
  const std::size_t n = statement.loops.size();

  if (n == 0 || !statement.store || !statement.multiply || !statement.add)
    return unknown("incomplete recognized statement");
  for (affine::AffineLoadOp load : statement.loads)
    if (!load)
      return unknown("incomplete recognized statement");

  affine::AffineForOp outermost = statement.loops.front();
  if (!outermost->getBlock())
    return unknown("the source nest is detached");
  if (outermost->getNumResults() != 0)
    return unknown("the source nest yields results");

  auto module = outermost->getParentOfType<ModuleOp>();
  if (!module)
    return unknown("the source nest is not inside a module");

  // The emitted addresses are i64 cast to index: exact only on a 64-bit
  // index type (a caller obligation of the Phase 3b-2 emitter).
  if (DataLayout(module).getTypeSizeInBits(
          IndexType::get(anchor->getContext())) != 64)
    return unknown("the index type is not 64 bits wide");

  // ------------------------------------------------------------------
  // Operands, operand order and attributes: decided from the statement.
  // ------------------------------------------------------------------

  affine::AffineLoadOp loadA = statement.loads[0];
  affine::AffineLoadOp loadB = statement.loads[1];
  affine::AffineLoadOp loadC = statement.loads[2];
  arith::MulFOp multiply = statement.multiply;
  arith::AddFOp add = statement.add;
  affine::AffineStoreOp store = statement.store;

  std::array<Value, 3> memrefs = {loadA.getMemRef(), loadB.getMemRef(),
                                  loadC.getMemRef()};
  if (store.getMemRef() != memrefs[2])
    return unknown("the store and the C load use different memrefs");

  Type elementType;
  for (Value memref : memrefs) {
    auto type = dyn_cast<MemRefType>(memref.getType());
    if (!type)
      return unknown("an operand is not a memref");
    if (!elementType)
      elementType = type.getElementType();
    if (type.getElementType() != elementType)
      return unknown("the operands have different element types");
  }

  GuardedMacSemantics semantics;

  Value product = multiply.getResult();
  if (multiply.getLhs() == loadA.getResult() &&
      multiply.getRhs() == loadB.getResult())
    semantics.aIsLeftFactor = true;
  else if (multiply.getLhs() == loadB.getResult() &&
           multiply.getRhs() == loadA.getResult())
    semantics.aIsLeftFactor = false;
  else
    return unknown("the multiplication does not multiply the A and B loads");

  if (add.getLhs() == loadC.getResult() && add.getRhs() == product)
    semantics.accumulatorIsLeftAddend = true;
  else if (add.getLhs() == product && add.getRhs() == loadC.getResult())
    semantics.accumulatorIsLeftAddend = false;
  else
    return unknown("the addition does not add the product to the C load");

  if (store.getValueToStore() != add.getResult())
    return unknown("the store does not store the sum");

  if (multiply.getFastmath() != arith::FastMathFlags::none ||
      add.getFastmath() != arith::FastMathFlags::none)
    return unknown("fast-math flags on the source statement are not "
                   "mirrored in this phase");
  if (multiply.getRoundingmode().has_value() ||
      add.getRoundingmode().has_value())
    return unknown("a rounding-mode attribute on the source statement is "
                   "not mirrored in this phase");

  semantics.fastMath = arith::FastMathFlags::none;
  semantics.roundingMode = std::nullopt;
  semantics.integerOverflow = arith::IntegerOverflowFlags::none;

  // ------------------------------------------------------------------
  // Memory disjointness of A and B from C: proven, invalid, or UNKNOWN.
  // ------------------------------------------------------------------

  for (unsigned input = 0; input < 2; ++input) {
    switch (disjointFromOutput(memrefs[input], memrefs[2], assumeNoAlias)) {
    case Disjointness::Proven:
      break;
    case Disjointness::Overlapping:
      return invalid(Twine(input == 0 ? "A" : "B") +
                     " shares its buffer with C");
    case Disjointness::Unknown:
      return unknown(Twine(input == 0 ? "A" : "B") +
                     " may alias C: neither buffer is a fresh allocation "
                     "or an llvm.noalias argument");
    }
  }

  // ------------------------------------------------------------------
  // Exact source domain and its logical image.
  // ------------------------------------------------------------------

  const IntegerPolyhedron &domain = statement.sourceDomain;
  const PartitionCoordinateMap &map = statement.coordinateMap;

  if (domain.getNumSymbolVars() != 0 || domain.getNumDimVars() != n)
    return unknown("the source domain is not over the source loops");
  if (domain.getNumLocalVars() != 0)
    return unknown("existential locals in the source domain are not "
                   "supported by the exact normalization");
  if (map.sourceToLogical.size() != n || map.logicalToSource.size() != n)
    return unknown("the coordinate map does not match the loop count");

  AffineIntegerDomain inequalities;
  inequalities.sourceDimension = n;

  auto toBig = [](const llvm::DynamicAPInt &value)
      -> std::optional<BigInt> {
    auto narrow = guard_detail::toI64(value);
    if (!narrow)
      return std::nullopt;
    return BigInt(*narrow);
  };

  // c + a . x >= 0  <=>  (-a) . x <= c
  auto pushRow = [&](ArrayRef<llvm::DynamicAPInt> row, bool negate)
      -> bool {
    if (row.size() != n + 1)
      return false;
    AffineInequality constraint;
    constraint.coefficients.resize(n);
    for (std::size_t j = 0; j < n; ++j) {
      auto value = toBig(row[j]);
      if (!value)
        return false;
      constraint.coefficients[j] = negate ? *value : -*value;
    }
    auto constant = toBig(row.back());
    if (!constant)
      return false;
    constraint.upperBound = negate ? -*constant : *constant;
    inequalities.inequalities.push_back(std::move(constraint));
    return true;
  };

  for (unsigned r = 0; r < domain.getNumInequalities(); ++r)
    if (!pushRow(domain.getInequality(r), false))
      return unknown("unrepresentable source-domain inequality");
  for (unsigned r = 0; r < domain.getNumEqualities(); ++r) {
    // c + a . x == 0: both  c + a . x >= 0  and  -c - a . x >= 0.
    if (!pushRow(domain.getEquality(r), false) ||
        !pushRow(domain.getEquality(r), true))
      return unknown("unrepresentable source-domain equality");
  }

  auto normalized = buildRecoverableLogicalDomain(map, inequalities);
  if (!normalized)
    return unknown("the exact logical image needs more than one congruence "
                   "local or has unrepresentable coefficients");

  // Candidate box: the image of the constant bounds of every source loop
  // under T, by interval arithmetic. Coverage is proven exactly below.

  std::vector<ClosedIntegerInterval> box(n);
  {
    std::vector<BigInt> lower(n), upper(n);
    for (std::size_t j = 0; j < n; ++j) {
      auto lowerBound = domain.getConstantBound(BoundType::LB, j);
      auto upperBound = domain.getConstantBound(BoundType::UB, j);
      std::optional<std::int64_t> lowerValue, upperValue;
      if (lowerBound)
        lowerValue = guard_detail::toI64(*lowerBound);
      if (upperBound)
        upperValue = guard_detail::toI64(*upperBound);
      if (!lowerValue || !upperValue)
        return unknown("a source loop bound is absent or does not fit i64");
      lower[j] = BigInt(*lowerValue);
      upper[j] = BigInt(*upperValue);
    }
    const BigInt minI64(std::numeric_limits<std::int64_t>::min());
    const BigInt maxI64(std::numeric_limits<std::int64_t>::max());
    for (std::size_t i = 0; i < n; ++i) {
      if (map.sourceToLogical[i].size() != n)
        return unknown("malformed coordinate map");
      BigInt low = 0, high = 0;
      for (std::size_t j = 0; j < n; ++j) {
        const BigInt &t = map.sourceToLogical[i][j];
        const BigInt a = t * lower[j], b = t * upper[j];
        low += a < b ? a : b;
        high += a > b ? a : b;
      }
      // The loop emitters need lower > INT64_MIN and upper < INT64_MAX.
      if (low <= minI64 || high >= maxI64)
        return unknown("the logical box does not fit i64");
      box[i].lower = low.convert_to<std::int64_t>();
      box[i].upper = high.convert_to<std::int64_t>();
    }
  }

  auto plan = buildSymbolicIterationPlan(*normalized, box);
  if (!plan)
    return unknown("the candidate box does not cover the logical domain");
  if (!buildExactGuardPlan(*plan))
    return unknown("the exact guard is not representable in i64 "
                   "arithmetic over the candidate box");

  // Access functions, exactly the recognizer's linear forms.

  GuardedMacPreflight preflight(map, domain, *plan);

  for (unsigned a = 0; a < 3; ++a) {
    if (statement.accessRows[a].size() != statement.accessOffsets[a].size())
      return unknown("malformed access function");
    for (const auto &row : statement.accessRows[a]) {
      if (row.size() != n)
        return unknown("malformed access row");
      std::vector<BigInt> converted;
      converted.reserve(n);
      for (int64_t coefficient : row)
        converted.emplace_back(coefficient);
      preflight.accessRows[a].push_back(std::move(converted));
    }
    for (int64_t offset : statement.accessOffsets[a])
      preflight.accessOffsets[a].emplace_back(offset);
  }

  preflight.outermost = outermost;
  preflight.memrefs = memrefs;
  preflight.diagnostic = analysis->diagnostic;
  preflight.semantics = semantics;

  outcome = GuardedMacOutcome::Proven;
  why = preflight.diagnostic;
  return preflight;
}

std::optional<EmittedGuardedMacStatement>
emitGuardedMacStatement(mlir::OpBuilder &builder, mlir::Location loc,
                        const GuardedMacPreflight &preflight,
                        llvm::ArrayRef<mlir::Value> memrefs, std::string &why) {

  if (memrefs.size() != 3) {
    why = "REFUSED: three memrefs are required";
    return std::nullopt;
  }

  OpBuilder::InsertionGuard insertionGuard(builder);

  auto nest = emitGuardedAffineLoopNest(builder, loc, preflight.plan);
  if (!nest) {
    why = "REFUSED: guarded loop nest emission";
    return std::nullopt;
  }

  // From here on, every failure erases the whole nest.
  auto failNest = [&](const Twine &message)
      -> std::optional<EmittedGuardedMacStatement> {
    affine::AffineForOp outermost = nest->outermost;
    outermost->erase();
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };

  auto recovery = emitSourceIVRecovery(builder, loc, preflight.coordinateMap,
                                       preflight.plan.bounds, *nest);
  if (!recovery)
    return failNest("source IV recovery emission");

  std::array<GuardedMemoryOperand, 3> operands;
  for (unsigned a = 0; a < 3; ++a) {
    auto access = emitSourceAffineAccess(
        builder, loc, preflight.coordinateMap, preflight.plan.bounds, *nest,
        *recovery, preflight.accessRows[a], preflight.accessOffsets[a]);
    if (!access)
      return failNest("source access emission");
    operands[a] = GuardedMemoryOperand{memrefs[a], preflight.accessRows[a],
                                       preflight.accessOffsets[a], *access};
  }

  auto loads = emitGuardedMemoryLoads(
      builder, loc, preflight.coordinateMap, preflight.plan,
      preflight.sourceDomain, *nest, *recovery, operands[0], operands[1],
      operands[2]);
  if (!loads)
    return failNest("guarded memory loads emission");

  auto mac = emitGuardedMultiplyAccumulate(
      builder, loc, preflight.coordinateMap, preflight.plan,
      preflight.sourceDomain, *nest, *recovery, operands[0], operands[1],
      operands[2], *loads, preflight.semantics);
  if (!mac)
    return failNest("guarded multiply-accumulate emission");

  EmittedGuardedMacStatement statement;
  statement.nest = *nest;
  statement.recovery = *recovery;
  statement.operands = operands;
  statement.loads = *loads;
  statement.mac = *mac;
  return statement;
}

std::optional<GuardedMacTwin>
emitGuardedMacTwin(mlir::OpBuilder &builder, mlir::Operation *anchor,
                   llvm::StringRef twinName, std::string &why) {

  GuardedMacOutcome outcome = GuardedMacOutcome::Unknown;
  auto preflight = prepareGuardedMac(anchor, /*assumeNoAlias=*/false,
                                     outcome, why);
  if (!preflight)
    return std::nullopt;

  auto refused = [&](const Twine &message) -> std::optional<GuardedMacTwin> {
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };
  auto unknown = [&](const Twine &message) -> std::optional<GuardedMacTwin> {
    why = ("UNKNOWN: " + message).str();
    return std::nullopt;
  };

  affine::AffineForOp outermost = preflight->outermost;
  auto sourceFunction = outermost->getParentOfType<func::FuncOp>();
  if (!sourceFunction)
    return unknown("the source nest is not inside a func.func");
  auto module = sourceFunction->getParentOfType<ModuleOp>();
  if (!module)
    return unknown("the source function is not inside a module");
  if (sourceFunction->getParentOp() != module.getOperation())
    return unknown("the source function is not a direct child of its "
                   "module (nested symbol tables are not supported)");
  if (SymbolTable::lookupSymbolIn(module, twinName))
    return refused("the twin symbol already exists: " + twinName);

  // ------------------------------------------------------------------
  // The twin: created last, erased on any refusal.
  // ------------------------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);
  builder.setInsertionPointAfter(sourceFunction);

  Location loc = outermost.getLoc();
  SmallVector<Type> argumentTypes;
  for (Value memref : preflight->memrefs)
    argumentTypes.push_back(memref.getType());

  auto twin = func::FuncOp::create(
      builder, loc, twinName, builder.getFunctionType(argumentTypes, {}));
  twin.setPrivate();
  Block *entry = twin.addEntryBlock();
  builder.setInsertionPointToStart(entry);
  func::ReturnOp::create(builder, loc);
  builder.setInsertionPoint(entry->getTerminator());

  SmallVector<Value, 3> arguments(entry->getArguments());
  auto statement =
      emitGuardedMacStatement(builder, loc, *preflight, arguments, why);
  if (!statement) {
    twin.erase();
    return std::nullopt;
  }

  GuardedMacTwin result(preflight->coordinateMap, preflight->sourceDomain,
                        preflight->plan);
  result.function = twin;
  result.nest = statement->nest;
  result.recovery = statement->recovery;
  result.operands = statement->operands;
  result.loads = statement->loads;
  result.mac = statement->mac;
  result.semantics = preflight->semantics;

  why = preflight->diagnostic;
  return result;
}

std::optional<GuardedMacReplacement>
replaceGemmWithGuardedMac(mlir::OpBuilder &builder, mlir::Operation *anchor,
                          bool assumeNoAlias, GuardedMacOutcome &outcome,
                          std::string &why) {

  // 1. Every decision, before any operation is created.
  auto preflight = prepareGuardedMac(anchor, assumeNoAlias, outcome, why);
  if (!preflight)
    return std::nullopt;

  affine::AffineForOp original = preflight->outermost;

  auto unsupported = [&](const Twine &message)
      -> std::optional<GuardedMacReplacement> {
    outcome = GuardedMacOutcome::Unsupported;
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };

  // 2. The statement immediately before the source nest, on the source
  //    memrefs. Both nests exist only until step 4, inside this call.
  OpBuilder::InsertionGuard insertionGuard(builder);
  builder.setInsertionPoint(original);

  auto statement = emitGuardedMacStatement(builder, original.getLoc(),
                                           *preflight, preflight->memrefs,
                                           why);
  if (!statement) {
    outcome = GuardedMacOutcome::Unsupported;
    return std::nullopt;
  }

  affine::AffineForOp replacement = statement->nest.outermost;

  // 3. The new nest must verify before the source is touched.
  if (failed(mlir::verify(replacement.getOperation()))) {
    replacement->erase();
    return unsupported("the replacement nest does not verify");
  }

  // 4. The source nest goes; it has no results and no external uses.
  original->erase();

  GuardedMacReplacement result;
  result.outermost = replacement;
  result.statement = *statement;
  result.diagnostic = preflight->diagnostic;

  outcome = GuardedMacOutcome::Proven;
  why = preflight->diagnostic;
  return result;
}

} // namespace systolic::logical
