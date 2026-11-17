//===- NestRecordLowering.cpp - A GEMM lowered from its record -----------===//
//
// See NestRecordLowering.h. Every check below can only refuse; the first that fails is the
// reason, which names the nest once it is found.
//
//===----------------------------------------------------------------------===//
#include "Systolic/NestRecordLowering.h"

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Utils/StaticValueUtils.h"
#include "mlir/IR/AffineExpr.h"
#include "mlir/IR/AffineMap.h"
#include "mlir/IR/BuiltinTypes.h"
#include "mlir/Interfaces/LoopLikeInterface.h"

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/Support/CheckedArithmetic.h"
#include "llvm/Support/raw_ostream.h"

#include <functional>
#include <limits>

using namespace mlir;
using namespace mlir::systolic;
namespace json = llvm::json;

namespace {

const char *const kAxisName[] = {"I", "J", "K"};
const char *const kOperandName[] = {"A", "B", "C"};
/// Each operand's two roles, as axes (0 I, 1 J, 2 K), in GEMM order: A (I, K), B (K, J), C (I, J).
const unsigned kRolesOf[3][2] = {{0, 2}, {2, 1}, {0, 1}};

// ---------------------------------------------------------------------------------------
// Affine forms of the band's counters, for the formulas.
// ---------------------------------------------------------------------------------------

/// sum over the band's loops p of coefficient[p] * counter[p], plus constant.
struct Form {
  std::array<int64_t, 3> coefficient{0, 0, 0};
  int64_t constant = 0;

  bool isConstant() const { return coefficient[0] == 0 && coefficient[1] == 0 && coefficient[2] == 0; }
  bool operator==(const Form &other) const {
    return coefficient == other.coefficient && constant == other.constant;
  }
};

std::optional<Form> combine(const Form &a, const Form &b, int64_t sign) {
  Form out;
  for (unsigned p = 0; p < 3; ++p) {
    std::optional<int64_t> term = llvm::checkedMul(sign, b.coefficient[p]);
    std::optional<int64_t> sum = term ? llvm::checkedAdd(a.coefficient[p], *term) : std::nullopt;
    if (!sum)
      return std::nullopt;
    out.coefficient[p] = *sum;
  }
  std::optional<int64_t> term = llvm::checkedMul(sign, b.constant);
  std::optional<int64_t> sum = term ? llvm::checkedAdd(a.constant, *term) : std::nullopt;
  if (!sum)
    return std::nullopt;
  out.constant = *sum;
  return out;
}

std::optional<Form> scaled(const Form &a, int64_t factor) {
  Form out;
  for (unsigned p = 0; p < 3; ++p) {
    std::optional<int64_t> product = llvm::checkedMul(a.coefficient[p], factor);
    if (!product)
      return std::nullopt;
    out.coefficient[p] = *product;
  }
  std::optional<int64_t> product = llvm::checkedMul(a.constant, factor);
  if (!product)
    return std::nullopt;
  out.constant = *product;
  return out;
}

std::string formText(const Form &form) {
  std::string out;
  for (unsigned p = 0; p < 3; ++p)
    if (form.coefficient[p] != 0)
      out += (out.empty() ? "" : " + ") + std::to_string(form.coefficient[p]) + " * loop " + std::to_string(p);
  return out.empty() ? std::to_string(form.constant) : out + " + " + std::to_string(form.constant);
}

/// The value of an index as an affine form of the band's counters, read from the nest's own
/// operations: the counters, integer constants, addi, subi, muli by a constant, and
/// affine.apply of such. Nothing else is read as a form.
class Forms {
public:
  explicit Forms(ArrayRef<Value> counters) : counters(counters.begin(), counters.end()) {}

  std::optional<Form> of(Value value, unsigned depth = 0) {
    if (depth > 64)
      return std::nullopt;
    for (unsigned p = 0; p < counters.size(); ++p)
      if (value == counters[p]) {
        Form form;
        form.coefficient[p] = 1;
        return form;
      }
    Operation *op = value.getDefiningOp();
    if (!op)
      return std::nullopt;
    if (auto constant = dyn_cast<arith::ConstantOp>(op)) {
      auto attr = dyn_cast<IntegerAttr>(constant.getValue());
      if (!attr || !(attr.getType().isIndex() || attr.getType().isInteger(64)))
        return std::nullopt;
      Form form;
      form.constant = attr.getValue().getSExtValue();
      return form;
    }
    if (auto addi = dyn_cast<arith::AddIOp>(op)) {
      std::optional<Form> a = of(addi.getLhs(), depth + 1), b = of(addi.getRhs(), depth + 1);
      return a && b ? combine(*a, *b, 1) : std::nullopt;
    }
    if (auto subi = dyn_cast<arith::SubIOp>(op)) {
      std::optional<Form> a = of(subi.getLhs(), depth + 1), b = of(subi.getRhs(), depth + 1);
      return a && b ? combine(*a, *b, -1) : std::nullopt;
    }
    if (auto muli = dyn_cast<arith::MulIOp>(op)) {
      std::optional<Form> a = of(muli.getLhs(), depth + 1), b = of(muli.getRhs(), depth + 1);
      if (!a || !b)
        return std::nullopt;
      if (b->isConstant())
        return scaled(*a, b->constant);
      if (a->isConstant())
        return scaled(*b, a->constant);
      return std::nullopt;
    }
    if (auto apply = dyn_cast<affine::AffineApplyOp>(op)) {
      AffineMap map = apply.getAffineMap();
      if (map.getNumResults() != 1)
        return std::nullopt;
      return ofExpr(map.getResult(0), map.getNumDims(), apply.getMapOperands(), depth + 1);
    }
    return std::nullopt;
  }

  /// An affine expression over `operands` (the first `numDims` its dims, the rest its symbols).
  std::optional<Form> ofExpr(AffineExpr expr, unsigned numDims, ValueRange operands, unsigned depth = 0) {
    if (auto dim = dyn_cast<AffineDimExpr>(expr))
      return dim.getPosition() < numDims ? of(operands[dim.getPosition()], depth + 1) : std::nullopt;
    if (auto symbol = dyn_cast<AffineSymbolExpr>(expr)) {
      unsigned at = numDims + symbol.getPosition();
      return at < operands.size() ? of(operands[at], depth + 1) : std::nullopt;
    }
    if (auto constant = dyn_cast<AffineConstantExpr>(expr)) {
      Form form;
      form.constant = constant.getValue();
      return form;
    }
    auto binary = dyn_cast<AffineBinaryOpExpr>(expr);
    if (!binary)
      return std::nullopt;
    std::optional<Form> a = ofExpr(binary.getLHS(), numDims, operands, depth + 1);
    std::optional<Form> b = ofExpr(binary.getRHS(), numDims, operands, depth + 1);
    if (!a || !b)
      return std::nullopt;
    switch (expr.getKind()) {
    case AffineExprKind::Add:
      return combine(*a, *b, 1);
    case AffineExprKind::Mul:
      if (b->isConstant())
        return scaled(*a, b->constant);
      if (a->isConstant())
        return scaled(*b, a->constant);
      return std::nullopt;
    default:
      return std::nullopt; // floordiv, ceildiv, mod: no affine form
    }
  }

private:
  SmallVector<Value, 3> counters;
};

// ---------------------------------------------------------------------------------------
// The nest's index arithmetic at one iteration, for the tables.
// ---------------------------------------------------------------------------------------

bool isSixtyFour(Type type) { return type.isIndex() || type.isInteger(64); }

/// Integer arithmetic of 64 bits, checked: overflow, a division by zero, a shift by more than
/// the width are errors, never a value.
std::optional<int64_t> floorDivision(int64_t a, int64_t b) {
  if (b == 0 || (a == std::numeric_limits<int64_t>::min() && b == -1))
    return std::nullopt;
  int64_t q = a / b;
  if ((a % b != 0) && ((a < 0) != (b < 0)))
    --q;
  return q;
}
std::optional<int64_t> ceilDivision(int64_t a, int64_t b) {
  if (b == 0 || (a == std::numeric_limits<int64_t>::min() && b == -1))
    return std::nullopt;
  int64_t q = a / b;
  if ((a % b != 0) && ((a < 0) == (b < 0)))
    ++q;
  return q;
}

/// The nest's index arithmetic, evaluated at iterations of the band: the backward slice of
/// the addresses is checked once (admit), then run per iteration (run). Admitted: the band's
/// counters, integer constants of 64 bits, and arith and affine integer operations on index
/// and i64 values. Refused: a load (the address would depend on memory), a value from outside
/// the nest that is no constant (an argument: known only at run time), any other operation,
/// any other type.
class Evaluator {
public:
  Evaluator(ArrayRef<Value> counters, Operation *band) : band(band) {
    for (auto [p, counter] : llvm::enumerate(counters))
      slot[counter] = p;
    values.assign(counters.size(), 0);
  }

  /// Takes `value`'s slice into the program, or says why it cannot be evaluated.
  LogicalResult admit(Value value, StringRef what, std::string &why) {
    if (slot.count(value))
      return success();
    Operation *op = value.getDefiningOp();
    if (!op) {
      why = (what + " depends on a value that is no counter of the band's loops (an argument of the function: "
                    "known only at run time)")
                .str();
      return failure();
    }
    if (!isSixtyFour(value.getType())) {
      why = (what + " is computed in " + printed(value.getType()) +
             ", and the tables evaluate index and i64 arithmetic only")
                .str();
      return failure();
    }
    const bool inside = band->isAncestor(op);
    if (auto constant = dyn_cast<arith::ConstantOp>(op)) {
      if (!isa<IntegerAttr>(constant.getValue())) {
        why = (what + " uses a constant that is no integer").str();
        return failure();
      }
    } else if (!inside) {
      why = (what + " depends on a value computed before the nest (" + op->getName().getStringRef() +
             "): known only at run time")
                .str();
      return failure();
    } else if (isa<memref::LoadOp, affine::AffineLoadOp>(op)) {
      why = (what + " depends on memory (a load of " + printed(op->getOperand(0).getType()) +
             "): the address is data, not arithmetic of the counters")
                .str();
      return failure();
    } else if (!isa<arith::AddIOp, arith::SubIOp, arith::MulIOp, arith::DivSIOp, arith::DivUIOp,
                    arith::CeilDivSIOp, arith::FloorDivSIOp, arith::RemSIOp, arith::RemUIOp, arith::MinSIOp,
                    arith::MaxSIOp, arith::MinUIOp, arith::MaxUIOp, arith::AndIOp, arith::OrIOp, arith::XOrIOp,
                    arith::ShLIOp, arith::ShRSIOp, arith::ShRUIOp, arith::IndexCastOp, arith::IndexCastUIOp,
                    affine::AffineApplyOp, affine::AffineMinOp, affine::AffineMaxOp>(op)) {
      why = (what + " uses " + op->getName().getStringRef() + ", which the tables do not evaluate").str();
      return failure();
    }
    for (Value operand : op->getOperands()) {
      if (!isSixtyFour(operand.getType())) {
        why = (what + " is computed from " + printed(operand.getType()) +
               ", and the tables evaluate index and i64 arithmetic only")
                  .str();
        return failure();
      }
      if (failed(admit(operand, what, why)))
        return failure();
    }
    if (op->getNumResults() != 1)
      return failure();
    if (!slot.count(op->getResult(0))) {
      slot[op->getResult(0)] = values.size();
      values.push_back(0);
      program.push_back(op);
    }
    return success();
  }

  /// Runs the program at `point` (the counters' values). False with `why` on an error.
  bool run(ArrayRef<int64_t> point, std::string &why) {
    for (unsigned p = 0; p < point.size(); ++p)
      values[p] = point[p];
    for (Operation *op : program) {
      std::optional<int64_t> result = evaluate(op);
      if (!result) {
        why = "the address arithmetic fails at the iteration (" + pointText(point) + "): " +
              op->getName().getStringRef().str() + " overflows, divides by zero or shifts too far";
        return false;
      }
      values[slot[op->getResult(0)]] = *result;
    }
    return true;
  }

  int64_t valueOf(Value value) const { return values[slot.lookup(value)]; }

  static std::string pointText(ArrayRef<int64_t> point) {
    std::string out;
    for (int64_t v : point)
      out += (out.empty() ? "" : ", ") + std::to_string(v);
    return out;
  }

private:
  static std::string printed(Type type) {
    std::string text;
    llvm::raw_string_ostream os(text);
    type.print(os);
    return text;
  }

  int64_t operandValue(Operation *op, unsigned i) const { return values[slot.lookup(op->getOperand(i))]; }

  std::optional<int64_t> evaluate(Operation *op) {
    if (auto constant = dyn_cast<arith::ConstantOp>(op))
      return cast<IntegerAttr>(constant.getValue()).getValue().getSExtValue();
    auto binary = [&](auto f) -> std::optional<int64_t> { return f(operandValue(op, 0), operandValue(op, 1)); };
    if (isa<arith::AddIOp>(op))
      return binary([](int64_t a, int64_t b) { return llvm::checkedAdd(a, b); });
    if (isa<arith::SubIOp>(op))
      return binary([](int64_t a, int64_t b) { return llvm::checkedSub(a, b); });
    if (isa<arith::MulIOp>(op))
      return binary([](int64_t a, int64_t b) { return llvm::checkedMul(a, b); });
    if (isa<arith::DivSIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b == 0 || (a == std::numeric_limits<int64_t>::min() && b == -1))
          return std::nullopt;
        return a / b;
      });
    if (isa<arith::RemSIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b == 0)
          return std::nullopt;
        return b == -1 ? 0 : a % b;
      });
    if (isa<arith::DivUIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b == 0)
          return std::nullopt;
        return int64_t(uint64_t(a) / uint64_t(b));
      });
    if (isa<arith::RemUIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b == 0)
          return std::nullopt;
        return int64_t(uint64_t(a) % uint64_t(b));
      });
    if (isa<arith::FloorDivSIOp>(op))
      return binary([](int64_t a, int64_t b) { return floorDivision(a, b); });
    if (isa<arith::CeilDivSIOp>(op))
      return binary([](int64_t a, int64_t b) { return ceilDivision(a, b); });
    if (isa<arith::MinSIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> { return std::min(a, b); });
    if (isa<arith::MaxSIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> { return std::max(a, b); });
    if (isa<arith::MinUIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        return uint64_t(a) < uint64_t(b) ? a : b;
      });
    if (isa<arith::MaxUIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        return uint64_t(a) < uint64_t(b) ? b : a;
      });
    if (isa<arith::AndIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> { return a & b; });
    if (isa<arith::OrIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> { return a | b; });
    if (isa<arith::XOrIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> { return a ^ b; });
    if (isa<arith::ShLIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b < 0 || b > 62)
          return std::nullopt;
        return llvm::checkedMul(a, int64_t(1) << b);
      });
    if (isa<arith::ShRSIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b < 0 || b > 63)
          return std::nullopt;
        return a >> b;
      });
    if (isa<arith::ShRUIOp>(op))
      return binary([](int64_t a, int64_t b) -> std::optional<int64_t> {
        if (b < 0 || b > 63)
          return std::nullopt;
        return int64_t(uint64_t(a) >> b);
      });
    if (isa<arith::IndexCastOp, arith::IndexCastUIOp>(op))
      return operandValue(op, 0); // index and i64 alike: 64 bits each (the program's index width)
    if (auto apply = dyn_cast<affine::AffineApplyOp>(op)) {
      AffineMap map = apply.getAffineMap();
      if (map.getNumResults() != 1)
        return std::nullopt;
      return affineValue(map.getResult(0), map.getNumDims(), op);
    }
    if (auto min = dyn_cast<affine::AffineMinOp>(op))
      return extreme(min.getMap(), op, /*greatest=*/false);
    if (auto max = dyn_cast<affine::AffineMaxOp>(op))
      return extreme(max.getMap(), op, /*greatest=*/true);
    return std::nullopt;
  }

  std::optional<int64_t> extreme(AffineMap map, Operation *op, bool greatest) {
    std::optional<int64_t> out;
    for (AffineExpr result : map.getResults()) {
      std::optional<int64_t> value = affineValue(result, map.getNumDims(), op);
      if (!value)
        return std::nullopt;
      out = !out ? *value : (greatest ? std::max(*out, *value) : std::min(*out, *value));
    }
    return out;
  }

  /// An affine expression over the operands of `op` (dims, then symbols), with the affine
  /// dialect's own division: floordiv, ceildiv and mod by a positive constant.
  std::optional<int64_t> affineValue(AffineExpr expr, unsigned numDims, Operation *op) {
    if (auto dim = dyn_cast<AffineDimExpr>(expr))
      return operandValue(op, dim.getPosition());
    if (auto symbol = dyn_cast<AffineSymbolExpr>(expr))
      return operandValue(op, numDims + symbol.getPosition());
    if (auto constant = dyn_cast<AffineConstantExpr>(expr))
      return constant.getValue();
    auto binary = dyn_cast<AffineBinaryOpExpr>(expr);
    if (!binary)
      return std::nullopt;
    std::optional<int64_t> a = affineValue(binary.getLHS(), numDims, op);
    std::optional<int64_t> b = affineValue(binary.getRHS(), numDims, op);
    if (!a || !b)
      return std::nullopt;
    switch (expr.getKind()) {
    case AffineExprKind::Add:
      return llvm::checkedAdd(*a, *b);
    case AffineExprKind::Mul:
      return llvm::checkedMul(*a, *b);
    case AffineExprKind::FloorDiv:
      return *b > 0 ? floorDivision(*a, *b) : std::nullopt;
    case AffineExprKind::CeilDiv:
      return *b > 0 ? ceilDivision(*a, *b) : std::nullopt;
    case AffineExprKind::Mod: {
      if (*b <= 0)
        return std::nullopt;
      int64_t r = *a % *b;
      return r < 0 ? r + *b : r;
    }
    default:
      return std::nullopt;
    }
  }

  Operation *band;
  llvm::DenseMap<Value, unsigned> slot;
  SmallVector<int64_t> values;
  SmallVector<Operation *> program;
};

// ---------------------------------------------------------------------------------------
// The accesses of the update.
// ---------------------------------------------------------------------------------------

/// A load or a store of the update: its memref, and its index values (the indices of a
/// memref.load or memref.store; the map and its operands of an affine one).
struct Access {
  Operation *op = nullptr;
  Value memref;
  AffineMap map; // null for memref.load / memref.store
  SmallVector<Value> operands;
};

std::optional<Access> accessOf(Operation *op) {
  Access access;
  access.op = op;
  if (auto load = dyn_cast<memref::LoadOp>(op)) {
    access.memref = load.getMemRef();
    access.operands.assign(load.getIndices().begin(), load.getIndices().end());
    return access;
  }
  if (auto store = dyn_cast<memref::StoreOp>(op)) {
    access.memref = store.getMemRef();
    access.operands.assign(store.getIndices().begin(), store.getIndices().end());
    return access;
  }
  if (auto load = dyn_cast<affine::AffineLoadOp>(op)) {
    access.memref = load.getMemRef();
    access.map = load.getAffineMap();
    access.operands.assign(load.getMapOperands().begin(), load.getMapOperands().end());
    return access;
  }
  if (auto store = dyn_cast<affine::AffineStoreOp>(op)) {
    access.memref = store.getMemRef();
    access.map = store.getAffineMap();
    access.operands.assign(store.getMapOperands().begin(), store.getMapOperands().end());
    return access;
  }
  return std::nullopt;
}

} // namespace

FailureOr<RecordDrivenGemm> mlir::systolic::analyzeRecordDrivenNest(const NestRecordFile &file,
                                                                    const BoundNests &bound, Operation *anchor,
                                                                    unsigned indexWidth, int64_t tableBudget,
                                                                    std::string &why) {
  // ---------------------------------------------------------------------------------
  // The record by itself, as stage 2 checks it (NestRecordGate.h); for the tables, a record
  // VERIFIED but for the closed forms of its views.
  // ---------------------------------------------------------------------------------
  RecordAtNest at = checkRecordAtNest(file, bound, anchor, indexWidth, /*unmappedViews=*/true);
  if (!at.usable) {
    why = at.why;
    return failure();
  }
  std::string prefix = at.where + ": ";
  auto refuse = [&](const llvm::Twine &text) -> FailureOr<RecordDrivenGemm> {
    if (why.empty()) {
      why = text.str();
      if (why.empty())
        why = "the record is incomplete";
    }
    why = prefix + why;
    return failure();
  };
  NestRecordFields fields{why};
  const json::Object &gemm = *at.gemm;
  const json::Object &record = *at.record;

  // ---------------------------------------------------------------------------------
  // The band: a perfect nest of affine.for or scf.for loops, each with constant bounds, step 1
  // and no value carried from one iteration to the next. Between two loops, integer and
  // float constants only (an scf.for's bounds are often defined there); they compute nothing
  // the GEMM depends on but the bounds, which are read here.
  // ---------------------------------------------------------------------------------
  // A loop that carries values from one iteration to the next (iter_args of scf.for or
  // affine.for) holds its reduction in a value, not in memory: refused before the band is
  // read, since it is why such a nest is no band of the kind below.
  Operation *carrying = nullptr;
  anchor->walk([&](LoopLikeOpInterface loop) {
    if (!carrying && loop->getNumResults() != 0)
      carrying = loop;
  });
  if (carrying)
    return refuse(std::string("a loop of the nest (") + carrying->getName().getStringRef().str() +
                  ") carries values from one iteration to the next (iter_args): its reduction is held in a value, "
                  "not in memory, and the record's lowering takes reductions in memory only");
  struct BandLoop {
    Operation *op = nullptr;
    Value counter;
    int64_t lower = 0, upper = 0;
    Block *body = nullptr;
  };
  SmallVector<BandLoop, 3> band;
  for (Operation *op = anchor;;) {
    BandLoop loop;
    loop.op = op;
    if (auto a = dyn_cast<affine::AffineForOp>(op)) {
      if (!a.hasConstantLowerBound() || !a.hasConstantUpperBound() || a.getStepAsInt() != 1)
        return refuse("a loop of the nest has no constant bounds, a step other than 1, or results");
      loop.counter = a.getInductionVar();
      loop.lower = a.getConstantLowerBound();
      loop.upper = a.getConstantUpperBound();
      loop.body = a.getBody();
    } else if (auto f = dyn_cast<scf::ForOp>(op)) {
      std::optional<int64_t> lo = getConstantIntValue(f.getLowerBound());
      std::optional<int64_t> hi = getConstantIntValue(f.getUpperBound());
      if (!lo || !hi)
        return refuse("a loop of the nest has a bound that is no constant: the scf.for's bounds are known only "
                      "at run time");
      std::optional<int64_t> step = getConstantIntValue(f.getStep());
      if (!step || *step != 1)
        return refuse("a loop of the nest has a step other than 1");
      if (f.getUnsignedCmp() && (*lo < 0 || *hi < 0))
        return refuse("a loop of the nest compares its bounds unsigned, and a bound is negative");
      loop.counter = f.getInductionVar();
      loop.lower = *lo;
      loop.upper = *hi;
      loop.body = f.getBody();
    } else {
      return refuse(std::string("the nest's loop ") + op->getName().getStringRef().str() +
                    " is neither affine.for nor scf.for, the loops the record's lowering takes");
    }
    band.push_back(loop);
    if (band.size() > 3)
      return refuse("the nest has more than three loops, and the record's GEMM is a box of three");
    Operation *inner = nullptr;
    unsigned nested = 0, others = 0;
    for (Operation &child : loop.body->without_terminator()) {
      if (isa<LoopLikeOpInterface>(child)) {
        ++nested;
        inner = &child;
      } else if (!isa<arith::ConstantOp>(child)) {
        ++others;
      }
    }
    if (nested == 0)
      break; // the innermost loop: its body is read below
    if (nested > 1 || others != 0)
      return refuse("the nest is no perfect band: a loop's body holds more than the next loop and constants");
    op = inner;
  }
  if (band.size() != 3)
    return refuse("the nest has " + std::to_string(band.size()) + " loops, and the record's GEMM is a box of three");
  SmallVector<Value, 3> counters;
  for (const BandLoop &loop : band)
    counters.push_back(loop.counter);

  // ---------------------------------------------------------------------------------
  // The record's levels, axes and extents are the band's loops.
  // ---------------------------------------------------------------------------------
  const json::Array *levels = fields.array(gemm, "levels", "gemm");
  const json::Object *axes = fields.object(gemm, "axes", "gemm");
  const json::Object *extents = fields.object(gemm, "extents", "gemm");
  if (!levels || !axes || !extents)
    return refuse("");
  if (levels->size() != 3)
    return refuse("the record has " + std::to_string(levels->size()) + " levels, and the band 3 loops");
  std::array<int64_t, 3> lower{}, extent{}; // per axis
  std::array<unsigned, 3> levelOf{};        // per axis: the band position
  std::array<bool, 3> seen{false, false, false};
  for (unsigned l = 0; l < 3; ++l) {
    const json::Object *level = (*levels)[l].getAsObject();
    if (!level)
      return refuse("the record is incomplete: a level is no object");
    std::optional<StringRef> axis = fields.string(*level, "axis", "gemm.levels[]");
    std::optional<bool> descending = fields.boolean(*level, "descending", "gemm.levels[]");
    std::optional<int64_t> lo = fields.integer(*level, "lower", "gemm.levels[]");
    std::optional<int64_t> hi = fields.integer(*level, "upper", "gemm.levels[]");
    if (!axis || !descending || !lo || !hi)
      return refuse("");
    unsigned a = 0;
    while (a < 3 && *axis != kAxisName[a])
      ++a;
    if (a == 3 || seen[a] || *descending || *lo != band[l].lower || *hi != band[l].upper || *hi <= *lo)
      return refuse("level " + std::to_string(l) + " of the record is not loop " + std::to_string(l) +
                    " of the nest: axis, direction or bounds differ");
    seen[a] = true;
    levelOf[a] = l;
    lower[a] = *lo;
    extent[a] = *hi - *lo;
    const json::Array *list = fields.array(*axes, kAxisName[a], "gemm.axes");
    if (!list)
      return refuse("");
    std::optional<int64_t> listed = list->size() == 1 ? (*list)[0].getAsInteger() : std::nullopt;
    if (!listed || *listed != (int64_t)l)
      return refuse(std::string("the record's axis ") + kAxisName[a] + " is not on level " + std::to_string(l));
  }
  if (const json::Array *batch = axes->getArray("P"); batch && !batch->empty())
    return refuse("the record has a batch axis, which the record's lowering does not map");
  static const char *const extentName[] = {"M", "N", "K"};
  for (unsigned a = 0; a < 3; ++a) {
    std::optional<int64_t> e = fields.integer(*extents, extentName[a], "gemm.extents");
    if (!e)
      return refuse("");
    if (*e != extent[a])
      return refuse(std::string("the record's ") + extentName[a] + " is " + std::to_string(*e) + " and the loop of " +
                    kAxisName[a] + " runs " + std::to_string(extent[a]) + " times");
  }

  // ---------------------------------------------------------------------------------
  // The body: one multiply-accumulate, and index arithmetic.
  // ---------------------------------------------------------------------------------
  Block *body = band.back().body;
  SmallVector<Operation *, 3> loads;
  SmallVector<Operation *, 1> stores;
  SmallVector<arith::MulFOp, 1> products;
  SmallVector<arith::AddFOp, 1> sums;
  for (Operation &op : body->without_terminator()) {
    if (isa<memref::LoadOp, affine::AffineLoadOp>(op))
      loads.push_back(&op);
    else if (isa<memref::StoreOp, affine::AffineStoreOp>(op))
      stores.push_back(&op);
    else if (auto mul = dyn_cast<arith::MulFOp>(op))
      products.push_back(mul);
    else if (auto add = dyn_cast<arith::AddFOp>(op))
      sums.push_back(add);
    else if (!isa<arith::ConstantOp, arith::AddIOp, arith::SubIOp, arith::MulIOp, arith::DivSIOp, arith::DivUIOp,
                   arith::CeilDivSIOp, arith::FloorDivSIOp, arith::RemSIOp, arith::RemUIOp, arith::MinSIOp,
                   arith::MaxSIOp, arith::MinUIOp, arith::MaxUIOp, arith::AndIOp, arith::OrIOp, arith::XOrIOp,
                   arith::ShLIOp, arith::ShRSIOp, arith::ShRUIOp, arith::IndexCastOp, arith::IndexCastUIOp,
                   affine::AffineApplyOp, affine::AffineMinOp, affine::AffineMaxOp>(op))
      return refuse(std::string("the loop body holds ") + op.getName().getStringRef().str() +
                    ", and the record's lowering takes one multiply-accumulate and index arithmetic");
  }
  if (loads.size() != 3 || stores.size() != 1 || products.size() != 1 || sums.size() != 1)
    return refuse("the loop body is not one multiply-accumulate: " + std::to_string(loads.size()) + " loads, " +
                  std::to_string(stores.size()) + " stores, " + std::to_string(products.size()) +
                  " multiplications, " + std::to_string(sums.size()) + " additions");
  std::optional<Access> store = accessOf(stores[0]);
  arith::AddFOp add = sums[0];
  arith::MulFOp mul = products[0];
  Value stored = isa<memref::StoreOp>(stores[0]) ? cast<memref::StoreOp>(stores[0]).getValueToStore()
                                                 : cast<affine::AffineStoreOp>(stores[0]).getValueToStore();
  if (!store || stored != add.getResult())
    return refuse("the store does not write the sum");
  const bool accumulatorFirst = add.getRhs() == mul.getResult();
  if (!accumulatorFirst && add.getLhs() != mul.getResult())
    return refuse("the addition does not add the product");
  Value accumulator = accumulatorFirst ? add.getLhs() : add.getRhs();
  Operation *accumulatorLoad = accumulator.getDefiningOp();
  if (!accumulatorLoad || !llvm::is_contained(loads, accumulatorLoad))
    return refuse("the addition's other operand is no load of the body");
  Operation *factor[2] = {mul.getLhs().getDefiningOp(), mul.getRhs().getDefiningOp()};
  for (Operation *op : factor)
    if (!op || !llvm::is_contained(loads, op) || op == accumulatorLoad)
      return refuse("the product is not of two loads of the body other than the accumulator's");
  if (factor[0] == factor[1])
    return refuse("the product squares one load");
  for (Operation *op : {add.getOperation(), mul.getOperation(), accumulatorLoad, factor[0], factor[1]})
    if (!op->getResult(0).hasOneUse())
      return refuse("a value of the update has another use in the body");
  std::optional<Access> c = accessOf(accumulatorLoad);
  std::optional<Access> f0 = accessOf(factor[0]), f1 = accessOf(factor[1]);
  if (!c || !f0 || !f1)
    return refuse("an access of the update is no load");
  if (c->memref != store->memref)
    return refuse("the accumulator is not read from the memref the sum is stored to");
  Type element = add.getType();
  if (!element.isF32() || mul.getType() != element)
    return refuse("the update is not in f32, the device's binary32");

  // ---------------------------------------------------------------------------------
  // The record's update, flags and accumulator are the nest's.
  // ---------------------------------------------------------------------------------
  const json::Object *accumulation = fields.object(gemm, "accumulation", "gemm");
  if (!accumulation)
    return refuse("");
  std::optional<StringRef> elementName = fields.string(*accumulation, "element", "gemm.accumulation");
  std::optional<StringRef> reduction = fields.string(*accumulation, "reduction", "gemm.accumulation");
  std::optional<StringRef> multiply = fields.string(*accumulation, "multiply", "gemm.accumulation");
  std::optional<bool> first = fields.boolean(*accumulation, "accumulator_first", "gemm.accumulation");
  std::optional<int64_t> sites = fields.integer(*accumulation, "sites", "gemm.accumulation");
  std::optional<bool> sitesAgree = fields.boolean(*accumulation, "sites_agree", "gemm.accumulation");
  const json::Array *reductionFlags = fields.array(gemm, "reduction_flags", "gemm");
  const json::Array *multiplyFlags = fields.array(gemm, "multiply_flags", "gemm");
  if (!elementName || !reduction || !multiply || !first || !sites || !sitesAgree || !reductionFlags ||
      !multiplyFlags)
    return refuse("");
  if (*elementName != nestRecordElementName(element))
    return refuse("the record's element type is " + *elementName + " and the nest's is " +
                  nestRecordElementName(element));
  if (*reduction != "fadd" || *multiply != "fmul" || *sites != 1 || !*sitesAgree)
    return refuse("the record's update is not one site of fadd over fmul");
  if (*first != accumulatorFirst)
    return refuse("the record's addition takes the accumulator on the other side");
  auto namesOf = [](const json::Array &array, std::vector<std::string> &out) {
    for (const json::Value &value : array) {
      std::optional<StringRef> name = value.getAsString();
      if (!name)
        return false;
      out.push_back(name->str());
    }
    return true;
  };
  std::vector<std::string> recordReduction, recordMultiply;
  if (!namesOf(*reductionFlags, recordReduction) || !namesOf(*multiplyFlags, recordMultiply))
    return refuse("the record is incomplete: a flag is no string");
  if (recordReduction != nestRecordFlagNames(add.getFastmath()) ||
      recordMultiply != nestRecordFlagNames(mul.getFastmath()))
    return refuse("the record's flags are not the nest's");
  const bool reassoc = arith::bitEnumContainsAll(add.getFastmath(), arith::FastMathFlags::reassoc);
  const json::Object *reassociation = fields.object(record, "reassociation", "nest");
  if (!reassociation)
    return refuse("");
  std::optional<bool> sourceFlag = reassociation->getBoolean("source_flag_reassoc");
  if (!sourceFlag || *sourceFlag != reassoc)
    return refuse("the record's `reassociation.source_flag_reassoc` is not the nest's flag");

  // ---------------------------------------------------------------------------------
  // The operands: roots of the function, the record's, with static shapes.
  // ---------------------------------------------------------------------------------
  const json::Object *views = fields.object(gemm, "views", "gemm");
  if (!views)
    return refuse("");
  // A, B, C: which factor is A is the record's: A's root.
  std::array<const json::Object *, 3> view{};
  std::array<int64_t, 3> rootNumber{};
  for (unsigned x = 0; x < 3; ++x) {
    view[x] = fields.object(*views, kOperandName[x], "gemm.views");
    const json::Object *root =
        view[x] ? fields.object(*view[x], "root", std::string("gemm.views.") + kOperandName[x]) : nullptr;
    std::optional<int64_t> number =
        root ? fields.integer(*root, "root", std::string("gemm.views.") + kOperandName[x] + ".root") : std::nullopt;
    if (!number)
      return refuse("");
    rootNumber[x] = *number;
  }
  auto rootOf = [&](Value memref) -> int64_t {
    auto found = at.roots->number.find(memref);
    return found == at.roots->number.end() ? -1 : (int64_t)found->second;
  };
  std::array<Access, 3> access; // A, B, C (C: the store; its load is checked against it)
  {
    const int64_t r0 = rootOf(f0->memref), r1 = rootOf(f1->memref), rc = rootOf(store->memref);
    if (r0 < 0 || r1 < 0 || rc < 0)
      return refuse("an operand of the update is no root of the function");
    if (rc != rootNumber[2])
      return refuse("the record's C is root " + std::to_string(rootNumber[2]) + " and the nest stores to root " +
                    std::to_string(rc));
    if (r0 == rootNumber[0] && r1 == rootNumber[1]) {
      access[0] = *f0;
      access[1] = *f1;
    } else if (r1 == rootNumber[0] && r0 == rootNumber[1] && r0 != r1) {
      access[0] = *f1;
      access[1] = *f0;
    } else {
      return refuse("the record's A and B are not the roots the product reads");
    }
    access[2] = *store;
  }
  std::array<MemRefType, 3> type{};
  for (unsigned x = 0; x < 3; ++x) {
    type[x] = dyn_cast<MemRefType>(access[x].memref.getType());
    if (!type[x] || !type[x].hasStaticShape() || !type[x].getLayout().isIdentity() || type[x].getRank() < 1 ||
        type[x].getElementType() != element)
      return refuse(std::string(kOperandName[x]) +
                    " is no memref of static shape and the identity layout over the update's element type");
  }

  // The separations the record rests on: proofs by an allocation, or (A, C) and (B, C).
  const json::Array *separations = fields.array(record, "separation", "nest");
  if (!separations)
    return refuse("");
  for (const json::Value &value : *separations) {
    const json::Object *separation = value.getAsObject();
    if (!separation)
      return refuse("the record is incomplete: a separation is no object");
    std::optional<bool> used = fields.boolean(*separation, "used", "separation[]");
    std::optional<StringRef> kind = fields.string(*separation, "kind", "separation[]");
    const json::Array *pair = fields.array(*separation, "roots", "separation[]");
    if (!used || !kind || !pair)
      return refuse("");
    std::optional<int64_t> a = pair->size() == 2 ? (*pair)[0].getAsInteger() : std::nullopt;
    std::optional<int64_t> b = pair->size() == 2 ? (*pair)[1].getAsInteger() : std::nullopt;
    if (!a || !b || *a < 0 || *b < 0 || *a >= (int64_t)at.roots->roots.size() ||
        *b >= (int64_t)at.roots->roots.size())
      return refuse("the record is incomplete: a separation names no two roots of the function");
    if (!*used)
      continue;
    auto is = [&](int64_t x, int64_t y) { return (*a == x && *b == y) || (*a == y && *b == x); };
    if (*kind == "proof") {
      if (at.roots->roots[*a].argument && at.roots->roots[*b].argument)
        return refuse("the record proves two roots apart by an allocation, and both are arguments here");
      continue;
    }
    if (*kind != "assumption")
      return refuse("the record rests on a separation it does not know (" + *kind + ")");
    if (!is(rootNumber[0], rootNumber[2]) && !is(rootNumber[1], rootNumber[2]))
      return refuse("the record assumes roots " + std::to_string(*a) + " and " + std::to_string(*b) +
                    " apart, and the lowering checks only A and B against C");
  }

  RecordDrivenGemm out;
  out.anchor = anchor;
  out.M = extent[0];
  out.N = extent[1];
  out.K = extent[2];
  out.reassoc = reassoc;
  out.where = at.where;
  for (unsigned x = 0; x < 3; ++x)
    out.operands[x].memref = access[x].memref;

  // The record's affine closed form of dimension d of operand x, over the band's counters
  // (the record's levels are the band's loops, checked above). Failure when the record has
  // none: no such dimension, not affine, a parameter, anything ill-formed. Reads only.
  auto recordForm = [&](unsigned x, unsigned d, Form &form) -> LogicalResult {
    const json::Array *dimensions = view[x]->getArray("dimensions");
    if (!dimensions || d >= dimensions->size())
      return failure();
    const json::Object *dimension = (*dimensions)[d].getAsObject();
    if (!dimension)
      return failure();
    std::optional<bool> affine = dimension->getBoolean("affine");
    std::optional<int64_t> constant = dimension->getInteger("constant");
    const json::Array *terms = dimension->getArray("levels");
    const json::Array *parameters = dimension->getArray("parameters");
    if (!affine || !*affine || !constant || !terms || !parameters || !parameters->empty())
      return failure();
    form = Form();
    form.constant = *constant;
    for (const json::Value &value : *terms) {
      const json::Object *term = value.getAsObject();
      std::optional<int64_t> level = term ? term->getInteger("level") : std::nullopt;
      std::optional<int64_t> coefficient = term ? term->getInteger("coefficient") : std::nullopt;
      if (!level || !coefficient || *level < 0 || *level >= 3)
        return failure();
      std::optional<int64_t> sum = llvm::checkedAdd(form.coefficient[*level], *coefficient);
      if (!sum)
        return failure();
      form.coefficient[*level] = *sum;
    }
    return success();
  };
  auto proved = [&](unsigned x) {
    std::optional<bool> value = view[x]->getBoolean("proved");
    return value && *value;
  };

  // ---------------------------------------------------------------------------------
  // Formulas: the record's closed forms, checked against the nest's index arithmetic.
  // ---------------------------------------------------------------------------------
  if (!at.viewsUnmapped) {
    Forms forms(counters);
    auto indexForms = [&](const Access &a, SmallVector<Form, 2> &result) -> bool {
      result.clear();
      if (a.map) {
        for (AffineExpr expr : a.map.getResults()) {
          std::optional<Form> form = forms.ofExpr(expr, a.map.getNumDims(), a.operands);
          if (!form)
            return false;
          result.push_back(*form);
        }
        return true;
      }
      for (Value operand : a.operands) {
        std::optional<Form> form = forms.of(operand);
        if (!form)
          return false;
        result.push_back(*form);
      }
      return true;
    };
    SmallVector<Form, 2> cLoad;
    if (!indexForms(*c, cLoad))
      return refuse("the nest reads C at an index the lowering cannot read as an affine form of its loops");
    for (unsigned x = 0; x < 3; ++x) {
      const char *name = kOperandName[x];
      if (!proved(x))
        return refuse(std::string("the record does not prove the view of ") + name);
      SmallVector<Form, 2> nest;
      if (!indexForms(access[x], nest))
        return refuse(std::string("the nest reads ") + name +
                      " at an index the lowering cannot read as an affine form of its loops");
      if (x == 2 && nest != cLoad)
        return refuse("the update does not read the cell of C it writes");
      const int64_t rank = type[x].getRank();
      const json::Array *dimensions = view[x]->getArray("dimensions");
      if (!dimensions || (int64_t)dimensions->size() != rank)
        return refuse(std::string("the record's view of ") + name + " has " +
                      std::to_string(dimensions ? dimensions->size() : 0) + " dimensions, and " + name +
                      " has rank " + std::to_string(rank));
      // Per dimension: the record's form, which must be the nest's; then its index as the
      // operand's two coordinates y (each its counter from its lower bound) would give it:
      // base + coefficient[0] * y_row + coefficient[1] * y_column. A coordinate that is none of
      // the operand's roles is no part of its index.
      SmallVector<std::array<int64_t, 2>, 2> coefficient(rank, {0, 0});
      SmallVector<int64_t, 2> base(rank, 0);
      for (int64_t d = 0; d < rank; ++d) {
        Form mine;
        if (failed(recordForm(x, d, mine)))
          return refuse(std::string("the record has no affine closed form for dimension ") + std::to_string(d) +
                        " of the view of " + name);
        if (!(mine == nest[d]))
          return refuse(std::string("dimension ") + std::to_string(d) + " of the record's view of " + name +
                        " is not the nest's index: " + formText(nest[d]));
        std::optional<int64_t> b = nest[d].constant;
        for (unsigned p = 0; p < 3; ++p) {
          if (nest[d].coefficient[p] == 0)
            continue;
          int role = -1;
          for (int r = 0; r < 2; ++r)
            if (levelOf[kRolesOf[x][r]] == p)
              role = r;
          if (role < 0)
            return refuse(std::string("dimension ") + std::to_string(d) + " of " + name +
                          " reads the loop of a coordinate that is none of its roles");
          coefficient[d][role] = nest[d].coefficient[p];
          std::optional<int64_t> term = llvm::checkedMul(nest[d].coefficient[p], lower[kRolesOf[x][role]]);
          b = b && term ? llvm::checkedAdd(*b, *term) : std::nullopt;
        }
        if (!b)
          return refuse(std::string("the indices of ") + name + " overflow 64-bit arithmetic");
        base[d] = *b;
        // The dimension's indices over the box, which must lie inside it.
        std::optional<int64_t> lo = base[d], hi = base[d];
        for (int r = 0; r < 2; ++r) {
          std::optional<int64_t> span = llvm::checkedMul(coefficient[d][r], extent[kRolesOf[x][r]] - 1);
          if (!span || !lo || !hi)
            return refuse(std::string("the indices of ") + name + " overflow 64-bit arithmetic");
          lo = llvm::checkedAdd(*lo, std::min<int64_t>(*span, 0));
          hi = llvm::checkedAdd(*hi, std::max<int64_t>(*span, 0));
        }
        if (!lo || !hi)
          return refuse(std::string("the indices of ") + name + " overflow 64-bit arithmetic");
        if (*lo < 0 || *hi >= type[x].getDimSize(d))
          return refuse(std::string(name) + " is read out of bounds along dimension " + std::to_string(d));
      }
      // One role per dimension of a rank-2 memref, each with a nonzero scale: a subview.
      bool rolePure = rank == 2;
      for (int64_t d = 0; d < rank && rolePure; ++d)
        rolePure = (coefficient[d][0] != 0) != (coefficient[d][1] != 0);
      if (rolePure && (coefficient[0][0] != 0) == (coefficient[1][0] != 0))
        rolePure = false; // both dimensions read one role
      if (rolePure) {
        for (int64_t d = 0; d < 2; ++d) {
          const int role = coefficient[d][0] != 0 ? 0 : 1;
          out.operands[x].dimensions.push_back({role, coefficient[d][role], base[d]});
        }
        continue;
      }
      // Otherwise an explicit view: the element address is affine in (y_row, y_column),
      // elementOffset + elementStride[0] * y_row + elementStride[1] * y_column, over the
      // memref's row-major strides.
      SmallVector<int64_t, 4> rowMajor(rank, 1);
      for (int64_t d = rank - 2; d >= 0; --d) {
        std::optional<int64_t> s = llvm::checkedMul(rowMajor[d + 1], type[x].getDimSize(d + 1));
        if (!s)
          return refuse(std::string("the strides of ") + name + " overflow 64-bit arithmetic");
        rowMajor[d] = *s;
      }
      std::optional<int64_t> offset = int64_t(0);
      std::array<std::optional<int64_t>, 2> stride{int64_t(0), int64_t(0)};
      for (int64_t d = 0; d < rank; ++d) {
        std::optional<int64_t> t = llvm::checkedMul(rowMajor[d], base[d]);
        offset = offset && t ? llvm::checkedAdd(*offset, *t) : std::nullopt;
        for (int r = 0; r < 2; ++r) {
          std::optional<int64_t> u = llvm::checkedMul(rowMajor[d], coefficient[d][r]);
          stride[r] = stride[r] && u ? llvm::checkedAdd(*stride[r], *u) : std::nullopt;
        }
      }
      if (!offset || !stride[0] || !stride[1])
        return refuse(std::string("the element addresses of ") + name + " overflow 64-bit arithmetic");
      std::array<int64_t, 2> roleExtent{extent[kRolesOf[x][0]], extent[kRolesOf[x][1]]};
      for (int r = 0; r < 2; ++r) {
        if (roleExtent[r] == 1) {
          stride[r] = 1; // a unit dimension: no stride
          continue;
        }
        if (*stride[r] <= 0)
          return refuse(std::string("the view of ") + name + " has the element stride " +
                        std::to_string(*stride[r]) + " along " + kAxisName[kRolesOf[x][r]] +
                        ", and an explicit view takes positive strides");
      }
      // C's view must be one to one: no (d_row, d_column) other than (0, 0) inside the box
      // with stride[0] * d_row + stride[1] * d_column = 0.
      if (x == 2) {
        for (int64_t dr = -(roleExtent[0] - 1); dr <= roleExtent[0] - 1; ++dr) {
          std::optional<int64_t> lhs = llvm::checkedMul(*stride[0], dr);
          if (!lhs)
            return refuse("the element addresses of C overflow 64-bit arithmetic");
          if (*lhs % *stride[1] != 0)
            continue;
          const int64_t dc = -*lhs / *stride[1];
          if ((dr != 0 || dc != 0) && dc > -roleExtent[1] && dc < roleExtent[1])
            return refuse("two cells of C share an address: the view of C is not one to one");
        }
      }
      out.operands[x].explicitView = true;
      out.operands[x].elementOffset = *offset;
      out.operands[x].elementStrides = {*stride[0], *stride[1]};
    }
    out.summary = "formulas: the record's views of A, B and C are the nest's own index arithmetic";
    return out;
  }

  // ---------------------------------------------------------------------------------
  // Tables: the nest's addresses at every iteration of its static domain.
  // ---------------------------------------------------------------------------------
  if (indexWidth != 64)
    return refuse("the tables evaluate 64-bit index arithmetic, and the program's index has " +
                  std::to_string(indexWidth) + " bits");
  std::optional<int64_t> mn = llvm::checkedMul(extent[0], extent[1]);
  std::optional<int64_t> iterations = mn ? llvm::checkedMul(*mn, extent[2]) : std::nullopt;
  if (!iterations || *iterations > tableBudget)
    return refuse("the domain has more than " + std::to_string(tableBudget) +
                  " iterations, the most the tables are built for at compile time");
  Evaluator evaluator(counters, band.front().op);
  std::array<const Access *, 4> evaluated = {&access[0], &access[1], &access[2], &*c};
  static const char *const accessName[] = {"the address of A", "the address of B", "the address of C (stored)",
                                           "the address of C (read)"};
  for (unsigned e = 0; e < 4; ++e)
    for (Value operand : evaluated[e]->operands)
      if (failed(evaluator.admit(operand, accessName[e], why)))
        return refuse("");
  // Row-major element strides of each operand.
  std::array<SmallVector<int64_t, 4>, 3> stride;
  for (unsigned x = 0; x < 3; ++x) {
    const int64_t rank = type[x].getRank();
    stride[x].assign(rank, 1);
    for (int64_t d = rank - 2; d >= 0; --d) {
      std::optional<int64_t> s = llvm::checkedMul(stride[x][d + 1], type[x].getDimSize(d + 1));
      if (!s)
        return refuse(std::string("the strides of ") + kOperandName[x] + " overflow 64-bit arithmetic");
      stride[x][d] = *s;
    }
  }
  // The element offset an access reads at the current iteration, inside its memref, or nullopt.
  SmallVector<int64_t, 4> indexValues;
  auto elementOffset = [&](unsigned e, unsigned x, ArrayRef<int64_t> point) -> std::optional<int64_t> {
    const Access &a = *evaluated[e];
    indexValues.clear();
    if (a.map) {
      for (AffineExpr expr : a.map.getResults()) {
        // The access's map, over the evaluated operands.
        std::function<std::optional<int64_t>(AffineExpr)> eval = [&](AffineExpr ex) -> std::optional<int64_t> {
          if (auto dim = dyn_cast<AffineDimExpr>(ex))
            return evaluator.valueOf(a.operands[dim.getPosition()]);
          if (auto symbol = dyn_cast<AffineSymbolExpr>(ex))
            return evaluator.valueOf(a.operands[a.map.getNumDims() + symbol.getPosition()]);
          if (auto constant = dyn_cast<AffineConstantExpr>(ex))
            return constant.getValue();
          auto binary = cast<AffineBinaryOpExpr>(ex);
          std::optional<int64_t> l = eval(binary.getLHS()), r = eval(binary.getRHS());
          if (!l || !r)
            return std::nullopt;
          switch (ex.getKind()) {
          case AffineExprKind::Add:
            return llvm::checkedAdd(*l, *r);
          case AffineExprKind::Mul:
            return llvm::checkedMul(*l, *r);
          case AffineExprKind::FloorDiv:
            return *r > 0 ? floorDivision(*l, *r) : std::nullopt;
          case AffineExprKind::CeilDiv:
            return *r > 0 ? ceilDivision(*l, *r) : std::nullopt;
          case AffineExprKind::Mod: {
            if (*r <= 0)
              return std::nullopt;
            int64_t m = *l % *r;
            return m < 0 ? m + *r : m;
          }
          default:
            return std::nullopt;
          }
        };
        std::optional<int64_t> v = eval(expr);
        if (!v)
          return std::nullopt;
        indexValues.push_back(*v);
      }
    } else {
      for (Value operand : a.operands)
        indexValues.push_back(evaluator.valueOf(operand));
    }
    if ((int64_t)indexValues.size() != type[x].getRank())
      return std::nullopt;
    int64_t offset = 0;
    for (unsigned d = 0; d < indexValues.size(); ++d) {
      if (indexValues[d] < 0 || indexValues[d] >= type[x].getDimSize(d)) {
        why = std::string(kOperandName[x]) + " is read or written out of bounds along dimension " +
              std::to_string(d) + " at the iteration (" + Evaluator::pointText(point) + ")";
        return std::nullopt;
      }
      offset += indexValues[d] * stride[x][d]; // within the static shape: no overflow
    }
    return offset;
  };
  // The record's closed form of operand x, where it proves one that is affine in every
  // dimension: the tables must agree with it. (A form that is not affine, as (5 i) mod 16, is
  // the research's; the tables are the nest's own arithmetic, evaluated, and stand alone.)
  std::array<std::optional<SmallVector<Form, 2>>, 3> closed;
  for (unsigned x = 0; x < 3; ++x) {
    if (!proved(x))
      continue;
    SmallVector<Form, 2> list;
    bool all = true;
    for (unsigned d = 0; d < (unsigned)type[x].getRank() && all; ++d) {
      Form form;
      all = succeeded(recordForm(x, d, form));
      list.push_back(form);
    }
    if (all)
      closed[x] = list;
  }
  std::array<SmallVector<int64_t>, 3> table;
  table[0].assign(extent[0] * extent[2], -1);
  table[1].assign(extent[2] * extent[1], -1);
  table[2].assign(extent[0] * extent[1], -1);
  std::array<int64_t, 3> point{};
  for (point[0] = band[0].lower; point[0] < band[0].upper; ++point[0])
    for (point[1] = band[1].lower; point[1] < band[1].upper; ++point[1])
      for (point[2] = band[2].lower; point[2] < band[2].upper; ++point[2]) {
        std::string failure;
        if (!evaluator.run(point, failure))
          return refuse(failure);
        // The forward map: the iteration's coordinates, each a counter from its lower bound.
        std::array<int64_t, 3> y;
        for (unsigned a = 0; a < 3; ++a)
          y[a] = point[levelOf[a]] - lower[a];
        std::array<std::optional<int64_t>, 4> offsets;
        for (unsigned e = 0; e < 4; ++e) {
          offsets[e] = elementOffset(e, e < 3 ? e : 2, point);
          if (!offsets[e])
            return refuse(why.empty() ? std::string(accessName[e]) + " has no value at the iteration (" +
                                            Evaluator::pointText(point) + ")"
                                      : "");
        }
        if (*offsets[2] != *offsets[3])
          return refuse("the update does not read the cell of C it writes, at the iteration (" +
                        Evaluator::pointText(point) + ")");
        for (unsigned x = 0; x < 3; ++x) {
          const unsigned row = kRolesOf[x][0], column = kRolesOf[x][1];
          int64_t &slot = table[x][y[row] * extent[column] + y[column]];
          if (slot < 0)
            slot = *offsets[x];
          else if (slot != *offsets[x])
            return refuse(std::string("the address of ") + kOperandName[x] + " depends on " +
                          kAxisName[3 - row - column] + ", the coordinate it does not read (iteration (" +
                          Evaluator::pointText(point) + "))");
          if (closed[x]) {
            int64_t expected = 0;
            for (unsigned d = 0; d < (unsigned)type[x].getRank(); ++d) {
              const Form &form = (*closed[x])[d];
              int64_t index = form.constant;
              for (unsigned p = 0; p < 3; ++p)
                index += form.coefficient[p] * point[p];
              expected += index * stride[x][d];
            }
            if (expected != *offsets[x])
              return refuse(std::string("the record's closed form of ") + kOperandName[x] +
                            " is not the nest's address at the iteration (" + Evaluator::pointText(point) + ")");
          }
        }
      }
  // No two cells of C share an address: each output is one cell, written by its own sum.
  {
    llvm::DenseMap<int64_t, int64_t> owner;
    for (int64_t cell = 0; cell < (int64_t)table[2].size(); ++cell) {
      auto [it, inserted] = owner.try_emplace(table[2][cell], cell);
      if (!inserted)
        return refuse("two cells (I, J) of C, (" + std::to_string(it->second / extent[1]) + ", " +
                      std::to_string(it->second % extent[1]) + ") and (" + std::to_string(cell / extent[1]) + ", " +
                      std::to_string(cell % extent[1]) + "), share the address " + std::to_string(table[2][cell]));
    }
  }
  out.tables = true;
  out.iterations = *iterations;
  for (unsigned x = 0; x < 3; ++x)
    out.operands[x].table = std::move(table[x]);
  out.summary = "tables: the nest's own addresses at its " + std::to_string(*iterations) +
                " iterations, A one per (I, K), B one per (K, J), C one per (I, J) and one to one; each (I, J, K) "
                "is one iteration";
  return out;
}
