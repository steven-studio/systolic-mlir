//===- NestRecordGate.h - Records of loop nests as a gate -------*- C++ -*-===//
//
// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md. With the option `nest-records=FILE`,
// --affine-matmul-to-systolic lowers a GEMM only where FILE, written by the research
// recognizer (research/generalized-gemm, generalized-gemm-nest-report), has a record of
// the GEMM's loop nest that
//
//   - is about this nest of this input: the binding the record carries
//     (Systolic/NestBinding.h) is the one computed here of the nest as it stands at the
//     pass's input, the function's roots are the ones the record names, and the record's
//     claims are made at this program's index width;
//   - is complete and verified: every field read below is there and well formed, the
//     status is VERIFIED, the ordered fold is verified over the whole scope at the
//     symbolic tier, nothing is refused, no range of any argument restricts the claims,
//     and no record claims to authorize a lowering;
//   - agrees with what the production recognizer found, field by field: M, N and K; the
//     loop of I, J and K with its bounds; each operand's root and, per dimension, the loop
//     it reads, the scale and the constant; the element type; the update (the operations,
//     the order of the addition's operands, the flags of both operations); and the
//     separations the record assumed, which must be the pairs the lowering's alias gate
//     checks itself.
//
// A record is a gate and nothing more. It never replaces a check of the lowering: the
// lowering then runs as it does without records, with its alias gate, its
// floating-point policy, its bounds and its exact partition. Without the option nothing
// here is called. Anything missing, stale, malformed or different leaves the nest as it
// is, with a remark that says which.
//
// Stage 3 (Systolic/NestRecordLowering.h) reads the same records for nests the production
// recognizer finds no GEMM in: checkRecordAtNest is the checks of a record by itself, shared
// by both; what stage 2 then compares with the recognizer, stage 3 compares with the nest.
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_NESTRECORDGATE_H
#define SYSTOLIC_NESTRECORDGATE_H

#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/NestBinding.h"

#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/JSON.h"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace mlir::systolic {

/// The format of a file of records, as the research tool names it.
constexpr const char *kNestRecordSchema = "generalized-gemm-nest/2";

/// The records of one file, read once.
class NestRecordFile {
public:
  /// Reads `path`: a JSON array of function objects of kNestRecordSchema, no function twice,
  /// no nest twice in a function. Anything else is an error: then it is no file of records.
  static FailureOr<NestRecordFile> load(StringRef path, std::string &error);

  /// The function's object, or null.
  const llvm::json::Object *function(StringRef name) const;
  /// The record of the `ordinal`-th outermost loop of the function, or null.
  const llvm::json::Object *nest(StringRef function, unsigned ordinal) const;

private:
  std::shared_ptr<llvm::json::Value> document;
  llvm::StringMap<const llvm::json::Object *> functions;
  llvm::StringMap<llvm::DenseMap<unsigned, const llvm::json::Object *>> nests;
};

/// The outermost loops of every function of a module, with their bindings, as the module
/// stands when this is called: before anything is lowered.
class BoundNests {
public:
  explicit BoundNests(ModuleOp module);

  struct Nest {
    func::FuncOp function;
    unsigned ordinal = 0;
    std::optional<NestBinding> binding; // Unset: no binding, the reason in `refusal`.
    std::string refusal;
  };
  /// The nest `loop` is the outermost loop of, or null when it is none of them.
  const Nest *find(Operation *loop) const;
  /// The roots of a function, numbered as the binding numbers them.
  const FunctionRoots *roots(func::FuncOp function) const;

private:
  llvm::DenseMap<Operation *, Nest> nestOf;
  llvm::DenseMap<Operation *, FunctionRoots> rootsOf;
};

/// Reads the fields of a record, and says which one is missing or ill-formed. The first
/// failure is kept.
struct NestRecordFields {
  std::string &why;

  void fail(const llvm::Twine &text) {
    if (why.empty())
      why = text.str();
  }
  const llvm::json::Object *object(const llvm::json::Object &in, StringRef key, StringRef where) {
    const llvm::json::Object *value = in.getObject(key);
    if (!value)
      fail("the record is incomplete: `" + where + "." + key + "` is no object");
    return value;
  }
  const llvm::json::Array *array(const llvm::json::Object &in, StringRef key, StringRef where) {
    const llvm::json::Array *value = in.getArray(key);
    if (!value)
      fail("the record is incomplete: `" + where + "." + key + "` is no array");
    return value;
  }
  std::optional<StringRef> string(const llvm::json::Object &in, StringRef key, StringRef where) {
    std::optional<StringRef> value = in.getString(key);
    if (!value)
      fail("the record is incomplete: `" + where + "." + key + "` is no string");
    return value;
  }
  std::optional<int64_t> integer(const llvm::json::Object &in, StringRef key, StringRef where) {
    std::optional<int64_t> value = in.getInteger(key);
    if (!value)
      fail("the record is incomplete: `" + where + "." + key + "` is no integer");
    return value;
  }
  std::optional<bool> boolean(const llvm::json::Object &in, StringRef key, StringRef where) {
    std::optional<bool> value = in.getBoolean(key);
    if (!value)
      fail("the record is incomplete: `" + where + "." + key + "` is no boolean");
    return value;
  }
};

/// A record located for a nest and checked by itself (checkRecordAtNest).
struct RecordAtNest {
  bool usable = false;
  std::string why;   // When it is not: the first check that failed, naming the nest once found.
  std::string where; // "nest N of F"
  const BoundNests::Nest *nest = nullptr;
  func::FuncOp function;
  const FunctionRoots *roots = nullptr;
  const llvm::json::Object *functionRecord = nullptr;
  const llvm::json::Object *record = nullptr;
  const llvm::json::Object *gemm = nullptr;
  std::string holdsFor;
  // With unmappedViews: the record is VERIFIED_NOT_MAPPED, and its every refusal is that the
  // research proposed no closed form for a view.
  bool viewsUnmapped = false;
};

/// The record of the nest whose outermost loop is `anchor`, and the checks of the record by
/// itself: `anchor` is an outermost loop of its function as the pass received it, with a
/// binding; the file has a record of that nest; the function's roots are the record's; no
/// record claims to authorize; the binding's scheme and hash are the nest's and the claims are
/// at `indexWidth`; VERIFIED, ordered fold verified, no refusals, no scope, Theorem 13 VERIFIED
/// at SYMBOLIC with logical tier SYMBOLIC, no other write, a box with no guard. With
/// `unmappedViews` (stage 3's tables), a record VERIFIED_NOT_MAPPED passes too when its every
/// refusal says only that a view has no affine closed form: none was proposed, or a dimension
/// of the one proposed is not affine.
RecordAtNest checkRecordAtNest(const NestRecordFile &file, const BoundNests &bound, Operation *anchor,
                               unsigned indexWidth, bool unmappedViews);

/// The research tool's names: of an element type (f32, f64, bvN) and of fast-math flags.
std::string nestRecordElementName(Type type);
std::vector<std::string> nestRecordFlagNames(arith::FastMathFlags flags);

struct NestRecordDecision {
  bool agrees = false;
  std::string why;     // When the record does not let the lowering go ahead.
  std::string summary; // When it does: what was compared.
};

/// Whether the record of `matmul`'s nest lets the dense lowering of `matmul` go ahead.
/// `indexWidth` is the program's index width.
NestRecordDecision checkNestRecord(const NestRecordFile &file, const BoundNests &bound,
                                   const LogicalMatmul &matmul, unsigned indexWidth);

} // namespace mlir::systolic

#endif // SYSTOLIC_NESTRECORDGATE_H
