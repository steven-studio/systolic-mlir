//===- NestBinding.h - Which loop nest a record is about --------*- C++ -*-===//
//
// Stage 2 of docs/GENERALIZED_GEMM_SCOPE.md. A record of the research recognizer
// (research/generalized-gemm, generalized-gemm-nest-report) is about one outermost loop of
// one function, as that loop stood in the input the record was computed from. The binding
// is how a reader of the record tells that the loop in front of it is that loop. The
// research tool and the production pass compute it with this one function, so the two
// agree on a loop only when the loop is the same.
//
// The canonical text holds, in order:
//   - the scheme, the function's name, and the loop's ordinal among the function's
//     outermost loops (loops written as operations, with no loop around them inside the
//     function, in program order);
//   - one line for every value the loop reads and does not define, other than an
//     `arith.constant`: a memory root of the function (its number among the function's
//     roots, and whether it is an argument, by number and with its `llvm.noalias`
//     attribute, or an allocation), or a scalar (an argument of the function by number, or
//     a value the function computes, by the name of the operation that computes it), with
//     its type;
//   - the loop, copied into a function of its own whose arguments are those values in that
//     order, with every `arith.constant` it reads copied in, printed in the generic form and
//     without locations.
//
// Locations are left out, so a record outlives a change of file name or of line. Any
// change of the loop's operations, attributes or types, of what it reads, of the
// function's name, or of the loop's position among the function's loops changes the
// binding. Nothing else about the function is in it: a record checks the function's roots
// on its own, and it never claims anything about the code around the loop.
//
// The function's memory roots are numbered as the research importer numbers them: the
// memref arguments in argument order, then every memref.alloc and memref.alloca in
// pre-order.
//
//===----------------------------------------------------------------------===//
#ifndef SYSTOLIC_NESTBINDING_H
#define SYSTOLIC_NESTBINDING_H

#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/IR/Operation.h"
#include "mlir/Support/LLVM.h"

#include "llvm/ADT/DenseMap.h"

#include <string>
#include <vector>

namespace mlir::systolic {

/// The scheme of the text below. A reader compares it before anything else.
constexpr const char *kNestBindingScheme = "generalized-gemm-nest-binding/1";

struct FunctionRoot {
  bool argument = false;   // An argument of the function, or an allocation.
  unsigned argNumber = 0;  // For an argument.
  bool noalias = false;    // For an argument: the llvm.noalias attribute.
  std::string type;        // The memref type, printed.
};

struct FunctionRoots {
  llvm::DenseMap<Value, unsigned> number;
  std::vector<FunctionRoot> roots;
};

/// The function's memory roots, numbered as above.
FunctionRoots numberFunctionRoots(func::FuncOp function);

/// The function's outermost loops, in program order: operations with LoopLikeOpInterface
/// that have no such operation around them inside the function.
SmallVector<Operation *> outermostLoops(func::FuncOp function);

struct NestBinding {
  std::string text;   // The canonical text above.
  std::string sha256; // Its SHA-256, in lowercase hexadecimal.
};

/// The binding of `loop`, the `ordinal`-th outermost loop of `function`, whose roots are
/// `roots`. Failure, with the reason in `why`, when the loop reads a value no binding
/// describes: a memref that is no root of the function, or a value that is neither a
/// memref nor an integer, index or floating-point scalar.
FailureOr<NestBinding> computeNestBinding(func::FuncOp function, Operation *loop, unsigned ordinal,
                                          const FunctionRoots &roots, std::string &why);

} // namespace mlir::systolic

#endif // SYSTOLIC_NESTBINDING_H
