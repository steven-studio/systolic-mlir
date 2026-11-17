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
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_NESTRECORDGATE_H
#define SYSTOLIC_NESTRECORDGATE_H

#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/NestBinding.h"

#include "mlir/IR/BuiltinOps.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Support/JSON.h"

#include <memory>
#include <optional>
#include <string>

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
