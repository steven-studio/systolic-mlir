//===- SemanticImport.h - MLIR to SemanticProgram ---------------*- C++ -*-===//
//
// Read-only import of one func.func into a SemanticProgram (invariant I7): all
// rewriting happens on a private clone of the enclosing module.
//
// Input: verified flat cf/arith/memref/func, or, with lowerStructured,
// anything the upstream lower-affine and convert-scf-to-cf passes reduce to it.
// Those two passes are trusted, as is the printing of the fingerprint. Calls
// are inlined on the clone when the callee has a body, is acyclic and is not
// recursive.
//
// A reachable construct outside the whitelist refuses the whole function
// (invariant I3). Unreachable blocks are verified by MLIR and otherwise
// ignored: they have no semantics.
//
//===----------------------------------------------------------------------===//
#ifndef GENERALIZEDGEMM_SEMANTICIMPORT_H
#define GENERALIZEDGEMM_SEMANTICIMPORT_H

#include "GeneralizedGemm/SemanticProgram.h"

#include "mlir/Dialect/Func/IR/FuncOps.h"

namespace mlir::generalized_gemm {

struct ImportLimits {
  unsigned maxBlocks = 512, maxOperations = 8192, maxStates = 4096;
  unsigned maxInlinedCalls = 64, maxTerms = 1u << 20;
};

struct ImportOptions {
  // Required when `index` appears: MLIR leaves its width to the target. Zero
  // refuses such functions rather than choosing one (invariant I1).
  unsigned indexBitWidth = 0;
  bool lowerStructured = false;
  ImportLimits limits;
};

SemanticProgram importFunction(func::FuncOp function,
                               const ImportOptions &options);

/// Whether `program` was imported from this exact function text and options.
bool matchesSource(const SemanticProgram &program, func::FuncOp function,
                   const ImportOptions &options);

} // namespace mlir::generalized_gemm
#endif
