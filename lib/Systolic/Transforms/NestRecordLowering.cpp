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
#include "llvm/ADT/SmallPtrSet.h"
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
/// operations: the counters, the indices its loops carry and step by a constant, integer
/// constants, addi, subi, muli by a constant, and affine.apply of such. Nothing else is read
/// as a form.
class Forms {
public:
  /// `carried`: each index a loop of the band carries and steps by a constant, as the form of
  /// the counters the band's reader found for it (IndexRecurrence).
  Forms(ArrayRef<Value> counters, const llvm::DenseMap<Value, Form> &carried)
      : counters(counters.begin(), counters.end()), carried(carried) {}

  std::optional<Form> of(Value value, unsigned depth = 0) {
    if (depth > 64)
      return std::nullopt;
    if (auto found = carried.find(value); found != carried.end())
      return found->second;
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
  const llvm::DenseMap<Value, Form> &carried;
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
/// counters, the indices its loops carry and step by a constant, integer constants of 64 bits, and arith and affine integer operations on index
/// and i64 values. Refused: a load (the address would depend on memory), a value from outside
/// the nest that is no constant (an argument: known only at run time), any other operation,
/// any other type.
class Evaluator {
public:
  /// `carried`: each index a loop of the band carries and steps by a constant, as its form of
  /// the counters; its value at an iteration is that form's, as a counter's is the point's.
  Evaluator(ArrayRef<Value> counters, Operation *band, const llvm::DenseMap<Value, Form> &carried)
      : band(band) {
    for (auto [p, counter] : llvm::enumerate(counters))
      slot[counter] = p;
    values.assign(counters.size(), 0);
    for (const auto &entry : carried) {
      slot[entry.first] = values.size();
      derived.push_back({(unsigned)values.size(), entry.second});
      values.push_back(0);
    }
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
    for (const auto &[at, form] : derived) {
      std::optional<int64_t> value = form.constant;
      for (unsigned p = 0; p < point.size() && value; ++p) {
        std::optional<int64_t> term = llvm::checkedMul(form.coefficient[p], point[p]);
        value = term ? llvm::checkedAdd(*value, *term) : std::nullopt;
      }
      if (!value) {
        why = "a carried index overflows 64-bit arithmetic at the iteration (" + pointText(point) + ")";
        return false;
      }
      values[at] = *value;
    }
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
  SmallVector<std::pair<unsigned, Form>> derived; // the carried indices: slot and form
};

// ---------------------------------------------------------------------------------------
// An index a loop carries and steps by a constant.
// ---------------------------------------------------------------------------------------

/// An index recurrence of a loop of the band: a value of index type the loop carries from one
/// iteration to the next, started at an arith.constant on every entry of the loop, whose next
/// value is itself plus or minus a constant (an arith.addi or arith.subi of the loop's body),
/// and whose last value, the loop's result, has no use. The loop's counter steps by 1 from its
/// lower bound, so at the iteration whose counter is k the value is
/// start + step * (k - lower): an affine form of the counter, read here from the IR and never
/// from the record. (The record's view must then be that form, as for any index.)
struct IndexRecurrence {
  Value inBody;                // the value the body reads
  int64_t start = 0, step = 0;
  Operation *update = nullptr; // the addi or subi whose result is handed to the next iteration
};

/// The constant by which `next` steps `carried`, or nothing.
std::optional<int64_t> constantStep(Value next, Value carried, Operation *&update) {
  if (auto add = next.getDefiningOp<arith::AddIOp>()) {
    update = add.getOperation();
    if (add.getLhs() == carried)
      return getConstantIntValue(add.getRhs());
    if (add.getRhs() == carried)
      return getConstantIntValue(add.getLhs());
    return std::nullopt;
  }
  if (auto sub = next.getDefiningOp<arith::SubIOp>()) {
    update = sub.getOperation();
    std::optional<int64_t> by = sub.getLhs() == carried ? getConstantIntValue(sub.getRhs()) : std::nullopt;
    if (!by || *by == std::numeric_limits<int64_t>::min())
      return std::nullopt;
    return -*by;
  }
  return std::nullopt;
}

/// Carried value `n` of a loop as an index recurrence, or nothing and why not (`what` names the
/// loop). `init` is what the loop starts it at, `inBody` what the body reads, `next` what the
/// body hands to the next iteration, `result` the loop's result for it.
std::optional<IndexRecurrence> readIndexRecurrence(unsigned n, Value init, Value inBody, Value next, Value result,
                                                   std::string &why) {
  std::string value = "value " + std::to_string(n);
  if (!inBody.getType().isIndex()) {
    std::string type;
    llvm::raw_string_ostream(type) << inBody.getType();
    why = value + " has type " + type + ", and the record's lowering takes a carried index only";
    return std::nullopt;
  }
  IndexRecurrence out;
  out.inBody = inBody;
  std::optional<int64_t> start = getConstantIntValue(init);
  if (!start) {
    why = value + " starts at a value that is no constant: known only at run time";
    return std::nullopt;
  }
  out.start = *start;
  std::optional<int64_t> step = constantStep(next, inBody, out.update);
  if (!step) {
    why = value + " is not stepped by a constant: the loop hands on another value than itself plus or minus "
                  "a constant";
    return std::nullopt;
  }
  out.step = *step;
  if (!result.use_empty()) {
    why = value + " has its last value read by the code after the loop, and the lowering would remove the loop "
                  "that makes it";
    return std::nullopt;
  }
  return out;
}

/// The index recurrences of an scf.for that carries values, or nothing and why not.
std::optional<SmallVector<IndexRecurrence, 2>> readCarriedIndices(scf::ForOp loop, std::string &why) {
  SmallVector<IndexRecurrence, 2> out;
  Operation *yield = loop.getBody()->getTerminator();
  for (unsigned n = 0, e = loop.getInitArgs().size(); n < e; ++n) {
    Value inBody = loop.getRegionIterArgs()[n];
    if (!inBody.getType().isIntOrIndex()) {
      why = "a loop of the nest (scf.for) carries values from one iteration to the next (iter_args): its "
            "reduction is held in a value, not in memory, and the record's lowering takes reductions in memory "
            "only";
      return std::nullopt;
    }
    std::string reason;
    std::optional<IndexRecurrence> one =
        readIndexRecurrence(n, loop.getInitArgs()[n], inBody, yield->getOperand(n), loop->getResult(n), reason);
    if (!one) {
      why = "a loop of the nest (scf.for) carries values from one iteration to the next (iter_args), and the "
            "record's lowering takes of them an index started at a constant and stepped by a constant only: " +
            reason;
      return std::nullopt;
    }
    out.push_back(*one);
  }
  return out;
}

// ---------------------------------------------------------------------------------------
// A counting scf.while.
// ---------------------------------------------------------------------------------------

/// What an scf.while is to the band when it is a counting loop. One of its carried values is
/// the counter, of index type, started at the constant `lower`: the one its comparison reads
/// (the only one, when it carries one value). Every other carried value is an index recurrence
/// (IndexRecurrence). Its before region holds constants, one comparison of the counter with a
/// constant, and scf.condition handing every carried value on as it is. The comparison is counter < upper (slt or ult with the counter first, sgt or ugt
/// with the bound first), or counter != upper (ne, either way round) with lower <= upper. Its
/// after region, whose argument is the counter the body reads, yields counter + 1, by an
/// arith.addi of that region. Its result, the counter's last value, has no use. Such a loop
/// runs its body once for each value from lower to upper - 1, in that order, and stops (no
/// value of the counter leaves [lower, upper], so nothing wraps): what an scf.for with these
/// bounds and step 1 does. (Reading a loop here does not verify it: a nest is lowered only with
/// a verified record. For a counter != upper loop the research's termination certificate holds
/// counter <= upper, which its proposer makes for such a comparison.)
struct CountingWhile {
  Value counter;                  // the after region's argument
  int64_t lower = 0, upper = 0;
  Operation *increment = nullptr; // the arith.addi whose result is yielded
  SmallVector<IndexRecurrence, 2> carried; // the other carried values
};

/// Each value an scf.while carries from one iteration to the next, said for what it is.
std::string describeCarried(scf::WhileOp loop) {
  Block *before = loop.getBeforeBody(), *after = loop.getAfterBody();
  scf::ConditionOp condition = loop.getConditionOp();
  Operation *yield = after->getTerminator();
  std::string text;
  llvm::raw_string_ostream os(text);
  for (unsigned n = 0, e = loop.getInits().size(); n < e; ++n) {
    Type type = before->getArgument(n).getType();
    // Handed on in place: the body's argument n is the before region's argument n.
    bool inPlace = n < condition.getArgs().size() && condition.getArgs()[n] == before->getArgument(n) &&
                   n < after->getNumArguments() && n < yield->getNumOperands();
    Value next = n < yield->getNumOperands() ? yield->getOperand(n) : Value();
    os << (n ? "; " : "") << "value " << n << " (" << type << ") ";
    if (inPlace && next == after->getArgument(n)) {
      os << "is carried unchanged";
    } else if (!type.isIntOrIndex()) {
      os << "is updated by the loop: a reduction or datum held in a value, not in memory";
    } else if (auto add = next ? next.getDefiningOp<arith::AddIOp>() : arith::AddIOp();
               inPlace && add &&
               ((add.getLhs() == after->getArgument(n) && getConstantIntValue(add.getRhs())) ||
                (add.getRhs() == after->getArgument(n) && getConstantIntValue(add.getLhs())))) {
      os << "is stepped by a constant";
    } else {
      os << "is updated by the loop otherwise";
    }
  }
  return text;
}

/// The counting loop `loop` is, or nothing and why not.
std::optional<CountingWhile> readCountingWhile(scf::WhileOp loop, std::string &why) {
  auto refuse = [&](const llvm::Twine &text) -> std::optional<CountingWhile> {
    why = ("a loop of the nest (scf.while) " + text).str();
    return std::nullopt;
  };
  if (loop.getInits().empty())
    return refuse("carries no value from one iteration to the next: it has no counter, and the record's lowering "
                  "takes a counting loop");
  Block *before = loop.getBeforeBody(), *after = loop.getAfterBody();
  const unsigned count = loop.getInits().size();
  auto notCounting = [&](const llvm::Twine &text) {
    return refuse("carries " + llvm::Twine(count) +
                  " values from one iteration to the next, and the record's lowering takes a counting loop, "
                  "which carries its counter and indices stepped by a constant only: " + text);
  };
  // The counter: the carried value the before region's comparison reads; with one carried value,
  // that one (and the checks below say what is wrong with it).
  unsigned c = 0;
  if (count != 1) {
    SmallVector<unsigned, 2> read;
    unsigned comparisons = 0;
    for (Operation &op : before->without_terminator())
      if (auto cmp = dyn_cast<arith::CmpIOp>(op)) {
        ++comparisons;
        for (unsigned n = 0; n < count; ++n)
          if (cmp.getLhs() == before->getArgument(n) || cmp.getRhs() == before->getArgument(n))
            read.push_back(n);
      }
    if (comparisons != 1 || read.size() != 1)
      return notCounting("its before region's comparison does not read exactly one of them, the counter, and "
                         "a constant; " + describeCarried(loop));
    c = read[0];
  }
  Value counter = before->getArgument(c);
  if (!counter.getType().isIndex()) {
    std::string type;
    llvm::raw_string_ostream(type) << counter.getType();
    return refuse("carries a value of type " + type + ", and a counting loop's counter is an index");
  }
  std::optional<int64_t> lower = getConstantIntValue(loop.getInits()[c]);
  if (!lower)
    return refuse("starts its counter at a value that is no constant: known only at run time");
  // The before region: constants, one comparison, scf.condition.
  scf::ConditionOp condition = loop.getConditionOp();
  arith::CmpIOp compare;
  for (Operation &op : before->without_terminator()) {
    if (isa<arith::ConstantOp>(op))
      continue;
    auto cmp = dyn_cast<arith::CmpIOp>(op);
    if (!cmp || compare)
      return refuse("computes more in its before region than one comparison of its counter (" +
                    op.getName().getStringRef() + "), and a counting loop's holds that comparison and constants");
    compare = cmp;
  }
  if (!compare || condition.getCondition() != compare.getResult())
    return refuse("does not go on by one comparison of its counter with a constant");
  if (condition.getArgs().size() != count || condition.getArgs()[c] != counter)
    return refuse("does not hand its counter to its body as it is: scf.condition forwards another value");
  std::optional<int64_t> upper;
  bool isUnsigned = false, isNotEqual = false;
  arith::CmpIPredicate predicate = compare.getPredicate();
  if (compare.getLhs() == counter &&
      (predicate == arith::CmpIPredicate::slt || predicate == arith::CmpIPredicate::ult)) {
    upper = getConstantIntValue(compare.getRhs());
    isUnsigned = predicate == arith::CmpIPredicate::ult;
  } else if (compare.getRhs() == counter &&
             (predicate == arith::CmpIPredicate::sgt || predicate == arith::CmpIPredicate::ugt)) {
    upper = getConstantIntValue(compare.getLhs());
    isUnsigned = predicate == arith::CmpIPredicate::ugt;
  } else if (predicate == arith::CmpIPredicate::ne && (compare.getLhs() == counter) != (compare.getRhs() == counter)) {
    upper = getConstantIntValue(compare.getLhs() == counter ? compare.getRhs() : compare.getLhs());
    isNotEqual = true;
  } else {
    return refuse("does not go on while its counter is below a bound: its condition is not counter < bound "
                  "(slt or ult), bound > counter (sgt or ugt) or counter != bound (ne)");
  }
  if (!upper)
    return refuse("compares its counter with a bound that is no constant: known only at run time");
  if (isUnsigned && (*lower < 0 || *upper < 0))
    return refuse("compares its counter with its bound unsigned, and one of them is negative");
  if (isNotEqual && *lower > *upper)
    return refuse("goes on while its counter differs from a bound below its start: it does not stop at the bound");
  // The after region: its argument is the counter, and counter + 1 is yielded.
  Value inBody = after->getArgument(c);
  Operation *yield = after->getTerminator();
  auto add = yield->getOperand(c).getDefiningOp<arith::AddIOp>();
  // (An addition of the after region's argument is an operation of that region: no other can
  // read the argument and be read by the region's scf.yield.)
  bool plusOne = add && ((add.getLhs() == inBody && getConstantIntValue(add.getRhs()) == std::optional<int64_t>(1)) ||
                         (add.getRhs() == inBody && getConstantIntValue(add.getLhs()) == std::optional<int64_t>(1)));
  if (!plusOne)
    return refuse("does not step its counter by 1: its after region yields another value than counter + 1");
  if (!loop->getResult(c).use_empty())
    return refuse("gives its counter's last value to the code after it, and the lowering would remove the loop "
                  "that makes it");
  CountingWhile out;
  // The other carried values: index recurrences, handed to the body as they are.
  for (unsigned n = 0; n < count; ++n) {
    if (n == c)
      continue;
    std::string reason;
    std::optional<IndexRecurrence> one;
    if (condition.getArgs()[n] != before->getArgument(n))
      reason = "value " + std::to_string(n) + " is not handed to the body as it is: scf.condition forwards "
               "another value";
    else
      one = readIndexRecurrence(n, loop.getInits()[n], after->getArgument(n), yield->getOperand(n),
                                loop->getResult(n), reason);
    if (!one)
      return notCounting(reason + "; " + describeCarried(loop));
    out.carried.push_back(*one);
  }
  out.counter = inBody;
  out.lower = *lower;
  out.upper = *upper;
  out.increment = add.getOperation();
  return out;
}

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
  // The band: a perfect nest of affine.for, scf.for or counting scf.while loops, each with
  // constant bounds and step 1; an scf.for carries index recurrences only, an scf.while its
  // counter and index recurrences (readCountingWhile), an affine.for nothing. Between two
  // loops, integer and float constants only (an scf.for's or scf.while's bounds are often
  // defined there) and the additions that step what a loop carries; they compute nothing the
  // GEMM depends on but the bounds, the counters and the carried indices, which are read here.
  // ---------------------------------------------------------------------------------
  // What the loops of the nest carry from one iteration to the next, before the band is read,
  // since it is why such a nest is no band of the kind below. An scf.for's carried values
  // (iter_args) are index recurrences (IndexRecurrence), which the band reads as forms of its
  // counter; any other (a reduction held in a value, not in memory) is refused, as is every
  // carried value of an affine.for. An scf.while carries its counter and index recurrences;
  // otherwise it is no counting loop, and each value is said for what it is.
  std::string carried;
  anchor->walk<WalkOrder::PreOrder>([&](Operation *op) {
    if (!carried.empty() || !isa<LoopLikeOpInterface>(op))
      return;
    if (auto w = dyn_cast<scf::WhileOp>(op)) {
      if (w.getInits().size() != 1)
        readCountingWhile(w, carried);
    } else if (auto f = dyn_cast<scf::ForOp>(op)) {
      if (op->getNumResults() != 0)
        readCarriedIndices(f, carried);
    } else if (op->getNumResults() != 0) {
      carried = std::string("a loop of the nest (") + op->getName().getStringRef().str() +
                ") carries values from one iteration to the next (iter_args)";
      carried += llvm::all_of(op->getResultTypes(), [](Type type) { return type.isIntOrIndex(); })
                     ? ", and the record's lowering reads the carried indices of an scf.for or an scf.while only"
                     : ": its reduction is held in a value, not in memory, and the record's lowering takes "
                       "reductions in memory only";
    }
  });
  if (!carried.empty())
    return refuse(carried);
  struct BandLoop {
    Operation *op = nullptr;
    Value counter;
    int64_t lower = 0, upper = 0;
    Block *body = nullptr;
    Operation *increment = nullptr; // scf.while: the addition whose result its after region yields
    SmallVector<IndexRecurrence, 2> carried; // the indices it carries and steps by a constant
    bool steps(Operation *child) const {
      return child == increment ||
             llvm::any_of(carried, [&](const IndexRecurrence &r) { return r.update == child; });
    }
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
      if (op->getNumResults() != 0) {
        std::string reason;
        std::optional<SmallVector<IndexRecurrence, 2>> indices = readCarriedIndices(f, reason);
        if (!indices)
          return refuse(reason);
        loop.carried = std::move(*indices);
      }
    } else if (auto w = dyn_cast<scf::WhileOp>(op)) {
      std::string reason;
      std::optional<CountingWhile> counting = readCountingWhile(w, reason);
      if (!counting)
        return refuse(reason);
      loop.counter = counting->counter;
      loop.lower = counting->lower;
      loop.upper = counting->upper;
      loop.body = w.getAfterBody();
      loop.increment = counting->increment;
      loop.carried = std::move(counting->carried);
    } else {
      return refuse(std::string("the nest's loop ") + op->getName().getStringRef().str() +
                    " is none of affine.for, scf.for and scf.while, the loops the record's lowering takes");
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
      } else if (!isa<arith::ConstantOp>(child) && !loop.steps(&child)) {
        ++others;
      }
    }
    if (nested == 0)
      break; // the innermost loop: its body is read below
    if (nested > 1 || others != 0)
      return refuse(std::string("the nest is no perfect band: a loop's body holds more than the next loop and "
                                "constants") +
                    (!loop.carried.empty() ? " and the steps of what it carries"
                                            : loop.increment ? " and the step of its counter" : ""));
    op = inner;
  }
  if (band.size() != 3)
    return refuse("the nest has " + std::to_string(band.size()) + " loops, and the record's GEMM is a box of three");
  SmallVector<Value, 3> counters;
  for (const BandLoop &loop : band)
    counters.push_back(loop.counter);
  // Each carried index as a form of its loop's counter: start + step * (counter - lower). The
  // loop is entered with lower < upper (the record's levels, below), and neither the form's
  // constant nor the value handed on by the last iteration leaves 64 bits, so no value of the
  // recurrence wraps.
  llvm::DenseMap<Value, Form> carriedForms;
  for (unsigned p = 0; p < 3; ++p)
    for (const IndexRecurrence &r : band[p].carried) {
      std::optional<int64_t> trips = llvm::checkedSub(band[p].upper, band[p].lower);
      std::optional<int64_t> span = trips ? llvm::checkedMul(r.step, *trips) : std::nullopt;
      std::optional<int64_t> last = span ? llvm::checkedAdd(r.start, *span) : std::nullopt;
      std::optional<int64_t> back = llvm::checkedMul(r.step, band[p].lower);
      std::optional<int64_t> constant = back ? llvm::checkedSub(r.start, *back) : std::nullopt;
      if (!last || !constant)
        return refuse("an index loop " + std::to_string(p) + " of the nest carries overflows 64-bit arithmetic");
      Form form;
      form.coefficient[p] = r.step;
      form.constant = *constant;
      carriedForms[r.inBody] = form;
    }

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
  // A level of the record is a loop of the band, or that loop's counter plus a constant: the
  // research may take for a level the subscript every access reads (k + 2, from 2 to 14, of a
  // loop from 0 to 12). Such a level names the same iterations in the same order, each by a
  // number `shift` greater. The shift is the difference of the lower bounds, the extents must
  // be equal, and nothing more is taken on the record's word: the record's closed forms are
  // rewritten to the loop's counter with it and must then be the forms read from the IR.
  std::array<int64_t, 3> lower{}, extent{}; // per axis, of the band's loop
  std::array<int64_t, 3> shift{0, 0, 0};    // per band position: the record's level minus the counter
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
    std::optional<int64_t> by = llvm::checkedSub(*lo, band[l].lower);
    std::optional<int64_t> end = by ? llvm::checkedAdd(band[l].upper, *by) : std::nullopt;
    if (a == 3 || seen[a] || *descending || !end || *hi != *end || *hi <= *lo)
      return refuse("level " + std::to_string(l) + " of the record is not loop " + std::to_string(l) +
                    " of the nest: axis, direction or bounds differ");
    if (*by != 0 && at.viewsUnmapped)
      return refuse("level " + std::to_string(l) + " of the record is loop " + std::to_string(l) +
                    " of the nest plus " + std::to_string(*by) +
                    ", and the record has no closed forms to check that against the nest's indices");
    seen[a] = true;
    levelOf[a] = l;
    shift[l] = *by;
    lower[a] = band[l].lower;
    extent[a] = band[l].upper - band[l].lower;
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
  // Experimental (exp/semgus-indirect-dp): a gather load, dp[...], an integer or index load
  // (memref.load, or affine.load as systolic-cfront writes it) whose value, through at most
  // one index_cast or index_castui, is used only as an index of memref.loads of the body:
  // A[dp[i], k]. It is no access of the update; the formulas match it with the record's
  // gather (below).
  struct GatherValue {
    Operation *load = nullptr;
    Access access;
    bool zeroExtend = false;
  };
  llvm::DenseMap<Value, GatherValue> gatherValues;
  auto onlyIndexOfLoads = [&](Value v) {
    if (v.use_empty())
      return false;
    for (OpOperand &use : v.getUses()) {
      auto user = dyn_cast<memref::LoadOp>(use.getOwner());
      if (!user || user->getBlock() != body || use.getOperandNumber() == 0)
        return false;
    }
    return true;
  };
  // Only a record that gathers lets a load be read so: without one, every nest is read as it
  // was, and a load in an address is refused as before.
  bool recordGathers = false;
  if (const json::Object *views = gemm.getObject("views"))
    for (const char *name : {"A", "B", "C"})
      if (const json::Object *one = views->getObject(name))
        if (const json::Array *dimensions = one->getArray("dimensions"))
          for (const json::Value &dimension : *dimensions)
            if (const json::Object *object = dimension.getAsObject())
              recordGathers = recordGathers || object->getObject("gather");
  for (Operation &op : body->without_terminator()) {
    Operation *load = isa<memref::LoadOp, affine::AffineLoadOp>(op) ? &op : nullptr;
    if (!recordGathers || !load || !load->getResult(0).getType().isSignlessIntOrIndex())
      continue;
    std::optional<Access> tableAccess = accessOf(load);
    if (!tableAccess || tableAccess->operands.empty() ||
        (tableAccess->map && tableAccess->map.getNumResults() != 1))
      continue;
    Value result = load->getResult(0);
    if (onlyIndexOfLoads(result) && result.getType().isIndex()) {
      gatherValues[result] = {load, *tableAccess, false};
      continue;
    }
    if (!result.hasOneUse())
      continue;
    Operation *cast = *result.getUsers().begin();
    if (cast->getBlock() != body || !isa<arith::IndexCastOp, arith::IndexCastUIOp>(cast) ||
        !onlyIndexOfLoads(cast->getResult(0)))
      continue;
    gatherValues[cast->getResult(0)] = {load, *tableAccess, isa<arith::IndexCastUIOp>(cast)};
  }
  llvm::SmallPtrSet<Operation *, 2> gatherLoads;
  SmallVector<Operation *, 2> addressTables; // integer loads of a gathering record's addresses
  for (auto &[value, gather] : gatherValues)
    gatherLoads.insert(gather.load);
  for (Operation &op : body->without_terminator()) {
    if (gatherLoads.contains(&op))
      continue;
    // With a record that gathers, an integer load is a table read for an address (the
    // update's three loads are of f32): C's (exp/general-scatter), checked by the scatter-add.
    if (recordGathers && isa<memref::LoadOp, affine::AffineLoadOp>(op) &&
        op.getResult(0).getType().isSignlessIntOrIndex()) {
      addressTables.push_back(&op);
      continue;
    }
    if (isa<memref::LoadOp, affine::AffineLoadOp>(op))
      loads.push_back(&op);
    else if (isa<memref::StoreOp, affine::AffineStoreOp>(op))
      stores.push_back(&op);
    else if (auto mul = dyn_cast<arith::MulFOp>(op))
      products.push_back(mul);
    else if (auto add = dyn_cast<arith::AddFOp>(op))
      sums.push_back(add);
    else if (isa<arith::CmpIOp>(op) ||
             (isa<arith::SelectOp>(op) && op.getResult(0).getType().isSignlessIntOrIndex()))
      continue; // index arithmetic of an address (exp/general-scatter: C[i > j ? ... : ...])
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

  // Experimental: the roots the nest gathers from (dp). Neither A, B nor C.
  llvm::SmallVector<int64_t, 1> gatherRootNumbers;
  SmallVector<Value, 2> tableMemrefs;
  for (auto &[value, gather] : gatherValues)
    tableMemrefs.push_back(gather.access.memref);
  for (Operation *op : addressTables)
    if (std::optional<Access> t = accessOf(op))
      tableMemrefs.push_back(t->memref);
  for (Value tableMemref : tableMemrefs) {
    const int64_t number = rootOf(tableMemref);
    if (number < 0)
      return refuse("the nest gathers from a memref that is no root of the function");
    if (number == rootNumber[0] || number == rootNumber[1] || number == rootNumber[2])
      return refuse("the nest gathers from an operand of the update");
    if (!llvm::is_contained(gatherRootNumbers, number))
      gatherRootNumbers.push_back(number);
  }

  // The separations the record rests on: proofs by an allocation, or (A, C) and (B, C), and,
  // with a gather, (dp, C).
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
    bool gatherPair = llvm::any_of(gatherRootNumbers, [&](int64_t g) { return is(g, rootNumber[2]); });
    if (!is(rootNumber[0], rootNumber[2]) && !is(rootNumber[1], rootNumber[2]) && !gatherPair)
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
  for (auto &[value, gather] : gatherValues)
    if (!llvm::is_contained(out.gatherTables, gather.access.memref))
      out.gatherTables.push_back(gather.access.memref);

  // The record's affine closed form of dimension d of operand x, over the band's counters
  // (the record's levels are the band's loops, checked above). Failure when the record has
  // none: no such dimension, not affine, a parameter, anything ill-formed. Reads only.
  std::function<LogicalResult(const json::Object *, Form &)> objectForm;
  auto recordForm = [&](unsigned x, unsigned d, Form &form) -> LogicalResult {
    const json::Array *dimensions = view[x]->getArray("dimensions");
    if (!dimensions || d >= dimensions->size())
      return failure();
    return objectForm((*dimensions)[d].getAsObject(), form);
  };
  objectForm = [&](const json::Object *dimension, Form &form) -> LogicalResult {
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
    // The record's levels to the band's counters: level = counter + shift.
    for (unsigned l = 0; l < 3; ++l) {
      std::optional<int64_t> term = llvm::checkedMul(form.coefficient[l], shift[l]);
      std::optional<int64_t> sum = term ? llvm::checkedAdd(form.constant, *term) : std::nullopt;
      if (!sum)
        return failure();
      form.constant = *sum;
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
    Forms forms(counters, carriedForms);
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
      // Experimental: an operand read at a gathered index, A[dp[i], k]: each dimension is the
      // record's affine form or the record's gather, the nest's own; the operand is then
      // packed, matrix[r][c] = A[...], and the tiles read the matrix.
      if (x != 2 && !access[x].map &&
          llvm::any_of(access[x].operands, [&](Value v) { return gatherValues.count(v); })) {
        const int64_t rank = type[x].getRank();
        const json::Array *dimensions = view[x]->getArray("dimensions");
        if (!dimensions || (int64_t)dimensions->size() != rank)
          return refuse(std::string("the record's view of ") + name + " does not have the rank of " + name);
        // A form over the band's counters as (base, coefficient over the operand's roles).
        auto overRoles = [&](const Form &f, std::array<int64_t, 2> &coefficient, int64_t &base,
                             std::string &problem) -> bool {
          coefficient = {0, 0};
          std::optional<int64_t> b = f.constant;
          for (unsigned p = 0; p < 3; ++p) {
            if (f.coefficient[p] == 0)
              continue;
            int role = -1;
            for (int r = 0; r < 2; ++r)
              if (levelOf[kRolesOf[x][r]] == p)
                role = r;
            if (role < 0) {
              problem = "reads the loop of a coordinate that is none of its roles";
              return false;
            }
            coefficient[role] = f.coefficient[p];
            std::optional<int64_t> term = llvm::checkedMul(f.coefficient[p], lower[kRolesOf[x][role]]);
            b = b && term ? llvm::checkedAdd(*b, *term) : std::nullopt;
          }
          if (!b) {
            problem = "overflows 64-bit arithmetic";
            return false;
          }
          base = *b;
          return true;
        };
        // The index's range over the box, inside [0, size).
        auto inside = [&](const std::array<int64_t, 2> &coefficient, int64_t base, int64_t size) {
          std::optional<int64_t> lo = base, hi = base;
          for (int r = 0; r < 2; ++r) {
            std::optional<int64_t> span = llvm::checkedMul(coefficient[r], extent[kRolesOf[x][r]] - 1);
            if (!span || !lo || !hi)
              return false;
            lo = llvm::checkedAdd(*lo, std::min<int64_t>(*span, 0));
            hi = llvm::checkedAdd(*hi, std::max<int64_t>(*span, 0));
          }
          return lo && hi && *lo >= 0 && *hi < size;
        };
        SmallVector<DrivenOperand::DrivenGather, 2> pack;
        for (int64_t d = 0; d < rank; ++d) {
          const std::string where = "dimension " + std::to_string(d) + " of " + name;
          const json::Object *dimension = (*dimensions)[d].getAsObject();
          const json::Object *gather = dimension ? dimension->getObject("gather") : nullptr;
          Value index = access[x].operands[d];
          DrivenOperand::DrivenGather one;
          std::string problem;
          auto found = gatherValues.find(index);
          if (found == gatherValues.end()) {
            if (gather)
              return refuse("the record gathers " + where + ", and the nest does not");
            Form mine;
            std::optional<Form> theirs = forms.of(index);
            if (failed(recordForm(x, d, mine)) || !theirs || !(mine == *theirs))
              return refuse("the record's form of " + where + " is not the nest's index");
            if (!overRoles(*theirs, one.coefficient, one.base, problem))
              return refuse(where + " " + problem);
            if (!inside(one.coefficient, one.base, type[x].getDimSize(d)))
              return refuse(std::string(name) + " is read out of bounds along dimension " + std::to_string(d));
            pack.push_back(one);
            continue;
          }
          // The gather: the record's root, extension, element and index are the nest's.
          if (!gather)
            return refuse("the nest gathers " + where + ", and the record does not");
          const Access &tableAccess = found->second.access;
          const Type loadedType = found->second.load->getResult(0).getType();
          auto tableType = dyn_cast<MemRefType>(tableAccess.memref.getType());
          const json::Object *gatherRoot = gather->getObject("root");
          std::optional<int64_t> gatherRootNumber = gatherRoot ? gatherRoot->getInteger("root") : std::nullopt;
          std::optional<StringRef> extend = gather->getString("extend");
          std::optional<StringRef> elementText = gather->getString("element");
          const json::Array *gatherIndex = gather->getArray("index");
          if (!gatherRootNumber || !extend || !elementText || !gatherIndex)
            return refuse("the record is incomplete: the gather of " + where);
          if (*gatherRootNumber != rootOf(tableAccess.memref))
            return refuse("the record gathers " + where + " from root " + std::to_string(*gatherRootNumber) +
                          ", and the nest from root " + std::to_string(rootOf(tableAccess.memref)));
          if (!tableType || !tableType.hasStaticShape() || !tableType.getLayout().isIdentity() ||
              tableType.getRank() != 1 || gatherIndex->size() != 1)
            return refuse("the nest gathers " + where + " from no one-dimensional memref of static shape");
          const bool isIndex = loadedType.isIndex();
          const unsigned width = isIndex ? indexWidth : loadedType.getIntOrFloatBitWidth();
          const char *nestExtend = isIndex ? "none" : found->second.zeroExtend ? "zext" : "sext";
          if (*elementText != "bv" + std::to_string(width) || *extend != nestExtend)
            return refuse("the record's gather of " + where + " is " + *extend + " of " + *elementText +
                          ", and the nest's is " + nestExtend + " of bv" + std::to_string(width));
          Form mine;
          std::optional<Form> theirs =
              tableAccess.map ? forms.ofExpr(tableAccess.map.getResult(0), tableAccess.map.getNumDims(),
                                             tableAccess.operands)
                              : forms.of(tableAccess.operands[0]);
          if (failed(objectForm((*gatherIndex)[0].getAsObject(), mine)) || !theirs || !(mine == *theirs))
            return refuse("the record's index of the gather of " + where + " is not the nest's");
          if (!overRoles(*theirs, one.coefficient, one.base, problem))
            return refuse("the gather of " + where + " " + problem);
          if (!inside(one.coefficient, one.base, tableType.getDimSize(0)))
            return refuse("the gather of " + where + " reads its table out of bounds");
          one.gathered = true;
          one.table = tableAccess.memref;
          one.zeroExtend = found->second.zeroExtend;
          pack.push_back(one);
        }
        out.operands[x].pack = std::move(pack);
        // The tiles read the packed rows x columns matrix as it is.
        out.operands[x].dimensions.push_back({0, 1, 0});
        out.operands[x].dimensions.push_back({1, 1, 0});
        continue;
      }
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
    if (!out.gatherTables.empty())
      out.summary += "; A or B packed by the record's gather before the tiles";
    return out;
  }

  // ---------------------------------------------------------------------------------
  // exp/general-scatter: a C that is no view of its memref. Track A, C's row a bijection
  // (a i + b) mod M inverted in closed form; Track B, any address of (i, j) the nest
  // computes, through read-only tables, with collisions, by a scatter-add through a
  // run-time inverse map. Both read A and B by their affine forms, packed. Otherwise the
  // tables below.
  // ---------------------------------------------------------------------------------
  {
    Forms forms(counters, carriedForms);
    auto formsOf = [&](const Access &a, SmallVector<std::optional<Form>, 3> &result) {
      result.clear();
      if (a.map) {
        for (AffineExpr expr : a.map.getResults())
          result.push_back(forms.ofExpr(expr, a.map.getNumDims(), a.operands));
      } else {
        for (Value v : a.operands)
          result.push_back(forms.of(v));
      }
    };
    // A form over the band's counters as base + coefficient . (y_role0, y_role1) of operand x.
    auto overRoles = [&](unsigned x, const Form &f, std::array<int64_t, 2> &coefficient,
                         int64_t &base) -> bool {
      coefficient = {0, 0};
      std::optional<int64_t> b = f.constant;
      for (unsigned p = 0; p < 3; ++p) {
        if (f.coefficient[p] == 0)
          continue;
        int role = -1;
        for (int r = 0; r < 2; ++r)
          if (levelOf[kRolesOf[x][r]] == p)
            role = r;
        if (role < 0)
          return false;
        coefficient[role] = f.coefficient[p];
        std::optional<int64_t> term = llvm::checkedMul(f.coefficient[p], lower[kRolesOf[x][role]]);
        b = b && term ? llvm::checkedAdd(*b, *term) : std::nullopt;
      }
      if (!b)
        return false;
      base = *b;
      return true;
    };
    auto inside = [&](unsigned x, const std::array<int64_t, 2> &coefficient, int64_t base, int64_t size) {
      std::optional<int64_t> lo = base, hi = base;
      for (int r = 0; r < 2; ++r) {
        std::optional<int64_t> span = llvm::checkedMul(coefficient[r], extent[kRolesOf[x][r]] - 1);
        if (!span || !lo || !hi)
          return false;
        lo = llvm::checkedAdd(*lo, std::min<int64_t>(*span, 0));
        hi = llvm::checkedAdd(*hi, std::max<int64_t>(*span, 0));
      }
      return lo && hi && *lo >= 0 && *hi < size;
    };
    // A or B packed by its affine forms: matrix (row, column) = (role 0, role 1).
    auto packOf = [&](unsigned x, SmallVector<DrivenOperand::DrivenGather, 2> &pack,
                      std::string &problem) -> bool {
      SmallVector<std::optional<Form>, 3> f;
      formsOf(access[x], f);
      pack.clear();
      for (unsigned d = 0; d < f.size(); ++d) {
        DrivenOperand::DrivenGather one;
        if (!f[d] || !overRoles(x, *f[d], one.coefficient, one.base)) {
          problem = std::string("dimension ") + std::to_string(d) + " of " + kOperandName[x] +
                    " is no affine form of its roles";
          return false;
        }
        if (!inside(x, one.coefficient, one.base, type[x].getDimSize(d))) {
          problem = std::string(kOperandName[x]) + " is read out of bounds along dimension " + std::to_string(d);
          return false;
        }
        pack.push_back(one);
      }
      return true;
    };
    auto sameIndex = [](const Access &a, const Access &b) { return a.map == b.map && a.operands == b.operands; };
    const Access &st = access[2];

    // Track A.
    std::string whyA;
    auto trackA = [&]() -> bool {
      if (type[2].getRank() != 2) {
        whyA = "C is not of rank 2";
        return false;
      }
      if (!sameIndex(*c, st)) {
        whyA = "the update reads C at other index values than it writes";
        return false;
      }
      std::optional<Form> inner;
      int64_t modulus = 0;
      int modDim = -1;
      std::array<std::optional<Form>, 2> plain;
      for (unsigned d = 0; d < 2; ++d) {
        std::optional<Form> in;
        int64_t m = 0;
        if (st.map) {
          AffineExpr e = st.map.getResult(d);
          if (e.getKind() == AffineExprKind::Mod)
            if (auto k = dyn_cast<AffineConstantExpr>(cast<AffineBinaryOpExpr>(e).getRHS())) {
              m = k.getValue();
              in = forms.ofExpr(cast<AffineBinaryOpExpr>(e).getLHS(), st.map.getNumDims(), st.operands);
            }
          if (!m)
            plain[d] = forms.ofExpr(e, st.map.getNumDims(), st.operands);
        } else {
          Operation *def = st.operands[d].getDefiningOp();
          if (def && isa<arith::RemUIOp, arith::RemSIOp>(def))
            if (auto k = def->getOperand(1).getDefiningOp<arith::ConstantIndexOp>()) {
              m = k.value();
              in = forms.of(def->getOperand(0));
            }
          if (!m)
            plain[d] = forms.of(st.operands[d]);
        }
        if (m) {
          if (modDim >= 0 || !in || m <= 0) {
            whyA = "C's address is not one dimension (affine) mod M beside an affine one";
            return false;
          }
          modDim = (int)d;
          modulus = m;
          inner = in;
        }
      }
      if (modDim < 0 || !plain[1 - modDim]) {
        whyA = "C's address is not one dimension (affine) mod M beside an affine one";
        return false;
      }
      const unsigned pI = levelOf[0];
      for (unsigned p = 0; p < 3; ++p)
        if (p != pI && inner->coefficient[p] != 0) {
          whyA = "the dimension mod M reads another loop than I's";
          return false;
        }
      const __int128 cI = inner->coefficient[pI];
      const __int128 b0 = (__int128)inner->constant + cI * lower[0];
      const __int128 low = b0 + std::min<__int128>(0, cI * (extent[0] - 1));
      if (low < 0) {
        whyA = "the dividend of the remainder may be negative";
        return false;
      }
      if (extent[0] != modulus || modulus > type[2].getDimSize(modDim)) {
        whyA = "I runs " + std::to_string(extent[0]) + " times and the modulus is " + std::to_string(modulus) +
               ": no bijection onto the rows of C";
        return false;
      }
      const __int128 M = modulus;
      const __int128 a = ((cI % M) + M) % M, b = ((b0 % M) + M) % M;
      // The inverse of a modulo M (extended Euclid), when gcd(a, M) = 1.
      __int128 r0 = M, r1 = a, t0 = 0, t1 = 1;
      while (r1 != 0) {
        __int128 q = r0 / r1, r2 = r0 - q * r1, t2 = t0 - q * t1;
        r0 = r1, r1 = r2, t0 = t1, t1 = t2;
      }
      if (r0 != 1) {
        whyA = "the multiplier and the modulus are not coprime: (a i + b) mod M is no bijection";
        return false;
      }
      const __int128 inv = ((t0 % M) + M) % M, cc = (M - (inv * b) % M) % M;
      for (__int128 y = 0; y < M; ++y) {
        const __int128 phi = (a * y + b) % M;
        if ((inv * phi + cc) % M != y) {
          whyA = "the closed-form inverse does not invert the address";
          return false;
        }
      }
      std::array<int64_t, 2> cJ;
      int64_t baseJ;
      if (!overRoles(2, *plain[1 - modDim], cJ, baseJ) || cJ[0] != 0 || cJ[1] == 0 ||
          !inside(2, cJ, baseJ, type[2].getDimSize(1 - modDim))) {
        whyA = "C's other dimension is no affine form of J inside C";
        return false;
      }
      SmallVector<DrivenOperand::DrivenGather, 2> packA, packB;
      if (!packOf(0, packA, whyA) || !packOf(1, packB, whyA))
        return false;
      for (DrivenOperand::DrivenGather &one : packA) {
        if (one.coefficient[0] == 0)
          continue;
        // y_I = inv(m): the row of A_packed is C's row m.
        one.modScale = one.coefficient[0];
        one.coefficient[0] = 0;
        one.modulus = modulus;
        one.modCoefficient = {(int64_t)inv, 0};
        one.modBase = (int64_t)cc;
      }
      out.inverse = true;
      out.operands[0].pack = std::move(packA);
      out.operands[1].pack = std::move(packB);
      for (unsigned x = 0; x < 2; ++x) {
        out.operands[x].dimensions.push_back({0, 1, 0});
        out.operands[x].dimensions.push_back({1, 1, 0});
      }
      for (unsigned d = 0; d < 2; ++d)
        out.operands[2].dimensions.push_back((int)d == modDim ? DrivenDimension{0, 1, 0}
                                                              : DrivenDimension{1, cJ[1], baseJ});
      out.summary = "inverse: C's rows (" + std::to_string((long long)a) + " i + " + std::to_string((long long)b) +
                    ") mod " + std::to_string(modulus) + ", a bijection, inverted in closed form (" +
                    std::to_string((long long)inv) + " m + " + std::to_string((long long)cc) + ") mod " +
                    std::to_string(modulus) + "; A packed by the inverse, C written as a view";
      return true;
    };

    // Track B: the slice of the nest that computes C's indices.
    std::string whyB;
    bool readsTable = false;
    Operation *anchorOp = band.front().op;
    llvm::SmallPtrSet<Operation *, 16> inSlice;
    auto sliceOk = [&]() -> bool {
      if (!sameIndex(*c, st)) {
        whyB = "the update reads C at other index values than it writes";
        return false;
      }
      SmallVector<Value, 8> work(st.operands.begin(), st.operands.end());
      while (!work.empty()) {
        Value v = work.pop_back_val();
        if (auto arg = dyn_cast<BlockArgument>(v)) {
          if (anchorOp->isAncestor(arg.getOwner()->getParentOp()) && v != counters[levelOf[0]] &&
              v != counters[levelOf[1]]) {
            whyB = "C's address reads the loop of K or a value a loop carries";
            return false;
          }
          continue;
        }
        Operation *def = v.getDefiningOp();
        if (!anchorOp->isAncestor(def) || !inSlice.insert(def).second)
          continue;
        if (def->getNumRegions() != 0) {
          whyB = "C's address is computed by an operation with regions";
          return false;
        }
        if (isa<memref::LoadOp, affine::AffineLoadOp>(def)) {
          std::optional<Access> t = accessOf(def);
          auto tableType = t ? dyn_cast<MemRefType>(t->memref.getType()) : MemRefType();
          bool readOnly = t && isa<BlockArgument>(t->memref) && !anchorOp->isAncestor(
                              cast<BlockArgument>(t->memref).getOwner()->getParentOp());
          for (Operation *user : t ? t->memref.getUsers() : Value().getUsers())
            readOnly = readOnly && isa<memref::LoadOp, affine::AffineLoadOp>(user);
          if (!readOnly || !tableType || !tableType.hasStaticShape() || !tableType.getLayout().isIdentity() ||
              !tableType.getElementType().isSignlessIntOrIndex() || t->memref == access[0].memref ||
              t->memref == access[1].memref || t->memref == access[2].memref) {
            whyB = "C's address loads from a memref that is no read-only integer table argument of static shape";
            return false;
          }
          readsTable = true;
          work.append(t->operands.begin(), t->operands.end());
          continue;
        }
        if (!isa<arith::ArithDialect>(def->getDialect()) && !isa<affine::AffineApplyOp>(def)) {
          whyB = std::string("C's address uses ") + def->getName().getStringRef().str() +
                 ", which the scatter-add does not re-emit";
          return false;
        }
        work.append(def->getOperands().begin(), def->getOperands().end());
      }
      return true;
    };
    const bool sliceRead = sliceOk();
    const bool needScatter = at.outputMayCollide || at.outputGathered || (sliceRead && readsTable);
    if (!needScatter && trackA())
      return out;
    if (needScatter) {
      if (!sliceRead)
        return refuse("the scatter-add: " + whyB);
      if (indexWidth != 64)
        return refuse("the scatter-add re-emits 64-bit index arithmetic, and the program's index has " +
                      std::to_string(indexWidth) + " bits");
      SmallVector<DrivenOperand::DrivenGather, 2> packA, packB;
      if (!packOf(0, packA, whyB) || !packOf(1, packB, whyB))
        return refuse("the scatter-add: " + whyB);
      out.scatter = true;
      out.operands[0].pack = std::move(packA);
      out.operands[1].pack = std::move(packB);
      for (unsigned x = 0; x < 2; ++x) {
        out.operands[x].dimensions.push_back({0, 1, 0});
        out.operands[x].dimensions.push_back({1, 1, 0});
      }
      for (Operation *op : inSlice)
        if (std::optional<Access> t = isa<memref::LoadOp, affine::AffineLoadOp>(op) ? accessOf(op) : std::nullopt)
          if (!llvm::is_contained(out.gatherTables, t->memref))
            out.gatherTables.push_back(t->memref);
      out.scatterSource.store = st.op;
      anchorOp->walk<WalkOrder::PreOrder>([&](Operation *op) {
        if (inSlice.contains(op))
          out.scatterSource.slice.push_back(op);
      });
      out.scatterSource.counterI = counters[levelOf[0]];
      out.scatterSource.counterJ = counters[levelOf[1]];
      out.scatterSource.lowerI = lower[0];
      out.scatterSource.lowerJ = lower[1];
      out.summary = std::string("scatter-add: C's address re-emitted at run time") +
                    (readsTable ? " through read-only tables" : "") +
                    (at.outputMayCollide ? ", with collisions" : "") +
                    ", every index checked before a write, a two-layer inverse map (bucket offsets, "
                    "sources sorted by cell of C), each cell of C written once";
      return out;
    }
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
  Evaluator evaluator(counters, band.front().op, carriedForms);
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
