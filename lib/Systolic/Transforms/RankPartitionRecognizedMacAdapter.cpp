//===- RankPartitionRecognizedMacAdapter.cpp -------------------------------===//
//
// Phase 5.4.3d-3b-4: from a recognized GEMM (the symbolic rank partition
// analysis) to the complete guarded MAC statement of Phases 1 to 3b-3,
// emitted into a private, never-called twin function. See the header.
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
#include "mlir/Interfaces/DataLayoutInterfaces.h"
#include "mlir/Interfaces/FunctionInterfaces.h"
#include "mlir/Interfaces/ViewLikeInterface.h"

#include <limits>

using namespace mlir;

namespace {

/// The disjointness rule of the production lowering
/// (AffineMatmulToSystolic.cpp, provablyDisjointFromOutput) without its
/// assume-no-alias escape: views are traced to the buffer they view; two views
/// of one buffer may overlap; different buffers are disjoint when one of them
/// is a fresh allocation or a restrict (llvm.noalias) argument.
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

Disjointness disjointFromOutput(Value input, Value output) {
  if (input == output)
    return Disjointness::Overlapping;
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

std::optional<GuardedMacTwin>
emitGuardedMacTwin(mlir::OpBuilder &builder, mlir::Operation *anchor,
                   llvm::StringRef twinName, std::string &why) {
  using namespace mlir::presburger;

  auto unknown = [&](const Twine &message)
      -> std::optional<GuardedMacTwin> {
    why = ("UNKNOWN: " + message).str();
    return std::nullopt;
  };
  auto refused = [&](const Twine &message)
      -> std::optional<GuardedMacTwin> {
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

  // The emitted addresses are i64 cast to index: exact only on a 64-bit
  // index type (a caller obligation of the Phase 3b-2 emitter).
  if (DataLayout(module).getTypeSizeInBits(builder.getIndexType()) != 64)
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
  // Memory disjointness of A and B from C: proven, or UNKNOWN.
  // ------------------------------------------------------------------

  for (unsigned input = 0; input < 2; ++input) {
    switch (disjointFromOutput(memrefs[input], memrefs[2])) {
    case Disjointness::Proven:
      break;
    case Disjointness::Overlapping:
      return refused(Twine(input == 0 ? "A" : "B") +
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

  std::array<std::vector<std::vector<BigInt>>, 3> rows;
  std::array<std::vector<BigInt>, 3> offsets;
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
      rows[a].push_back(std::move(converted));
    }
    for (int64_t offset : statement.accessOffsets[a])
      offsets[a].emplace_back(offset);
  }

  // ------------------------------------------------------------------
  // The twin: created last, erased on any refusal.
  // ------------------------------------------------------------------

  OpBuilder::InsertionGuard insertionGuard(builder);
  builder.setInsertionPointAfter(sourceFunction);

  Location loc = outermost.getLoc();
  SmallVector<Type> argumentTypes;
  for (Value memref : memrefs)
    argumentTypes.push_back(memref.getType());

  auto twin = func::FuncOp::create(
      builder, loc, twinName, builder.getFunctionType(argumentTypes, {}));
  twin.setPrivate();
  Block *entry = twin.addEntryBlock();
  builder.setInsertionPointToStart(entry);
  func::ReturnOp::create(builder, loc);
  builder.setInsertionPoint(entry->getTerminator());

  auto failTwin = [&](const Twine &message)
      -> std::optional<GuardedMacTwin> {
    twin.erase();
    why = ("REFUSED: " + message).str();
    return std::nullopt;
  };

  auto nest = emitGuardedAffineLoopNest(builder, loc, *plan);
  if (!nest)
    return failTwin("guarded loop nest emission");

  auto recovery = emitSourceIVRecovery(builder, loc, map, plan->bounds, *nest);
  if (!recovery)
    return failTwin("source IV recovery emission");

  std::array<GuardedMemoryOperand, 3> operands;
  for (unsigned a = 0; a < 3; ++a) {
    auto access = emitSourceAffineAccess(builder, loc, map, plan->bounds,
                                         *nest, *recovery, rows[a],
                                         offsets[a]);
    if (!access)
      return failTwin("source access emission");
    operands[a] = GuardedMemoryOperand{entry->getArgument(a), rows[a],
                                       offsets[a], *access};
  }

  auto loads = emitGuardedMemoryLoads(builder, loc, map, *plan, domain, *nest,
                                      *recovery, operands[0], operands[1],
                                      operands[2]);
  if (!loads)
    return failTwin("guarded memory loads emission");

  auto mac = emitGuardedMultiplyAccumulate(
      builder, loc, map, *plan, domain, *nest, *recovery, operands[0],
      operands[1], operands[2], *loads, semantics);
  if (!mac)
    return failTwin("guarded multiply-accumulate emission");

  GuardedMacTwin result(map, domain, *plan);
  result.function = twin;
  result.nest = *nest;
  result.recovery = *recovery;
  result.operands = operands;
  result.loads = *loads;
  result.mac = *mac;
  result.semantics = semantics;

  why = analysis->diagnostic;
  return result;
}

} // namespace systolic::logical
