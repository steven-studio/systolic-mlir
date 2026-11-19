//===- GeneralizedMatmulToSystolic.h - GEMMs lowered with records -*- C++ -*-===//
//
// The records mode of --affine-matmul-to-systolic (docs/GENERALIZED_GEMM_SCOPE.md, stages 2
// and 3): what the pass does when nest-records names a file of the research recognizer's
// records of loop nests. It is one walk over the module, in program order:
//
//   every outermost loop's binding     before anything is lowered (NestBinding.h)
//   a nest the affine recognizer       its record is a gate (NestRecordGate.h); admitted,
//   finds a GEMM in                    the GEMM is lowered by the pass's own dense lowering,
//                                      with every check that lowering makes without records
//   a nest it finds no GEMM in,        the GEMM is the record's: read and checked against
//   with record-driven                 the nest (NestRecordLowering.h), then emitted here,
//                                      by the record's formulas (each operand a view of its
//                                      memref) or by tables of the nest's addresses (A, B
//                                      and C gathered into matrices, C scattered back)
//
// Nests of affine.for loops that the recognizer finds no GEMM in, nests written with scf.for
// or scf.while, and nests whose loops carry their subscripts all come the second way: what
// tells them apart is in the record and in the nest's own checks, not here. Both ways number
// their GEMMs in one sequence and share the steps of Systolic/MatmulTileLowering.h.
//
// This is not a pass of its own. The mode needs the bindings of the module as the input has
// it and one numbering of the GEMMs across both kinds of nest, so it is one walk, entered
// from the pass, whose options it reads; a pass of its own would have to declare the dense
// lowering's options a second time.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_GENERALIZEDMATMULTOSYSTOLIC_H
#define SYSTOLIC_GENERALIZEDMATMULTOSYSTOLIC_H

#include "Systolic/AffineMatmulAnalysis.h"
#include "Systolic/SystolicTilePlan.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/Support/LLVM.h"

#include "llvm/ADT/STLFunctionalExtras.h"

#include <cstdint>
#include <string>

namespace mlir {
namespace systolic {

/// What the records mode reads of the options of --affine-matmul-to-systolic, under the
/// names the pass gives them.
struct GeneralizedMatmulOptions {
  /// nest-records: the file of records. record-driven and record-table-budget: stage 3.
  std::string nestRecords;
  bool recordDriven = false;
  int64_t recordTableBudget = 0;

  /// The dense lowering's, which the lowering from records reads as well.
  bool assumeNoAlias = false;
  bool allowFpReassociation = false;
  double sharedReadBytesPerCycle = 0.0;
  std::string costModel, operandPath;

  /// The recognition of the GEMMs that the records gate, and what is said of them.
  bool reductionBatches = false;
  bool classify = false;
  bool report = false;

  /// The pass's other paths. They read no records: with any of them the mode is refused.
  bool generalAffine = false;
  bool kernelCosetOffload = false;
  bool certifiedOffloadFirst = false;
  bool commonFiberOffload = false;
  bool fpDistribute = false;
};

/// The dense lowering of one GEMM that the affine recognizer found (the pass's lowerOne): the
/// GEMM lowered, or left as it is with the reason in `why` and its class, or a hard error.
using DenseMatmulLowering = llvm::function_ref<LogicalResult(
    const LogicalMatmul &matmul, const SystolicFleet &fleet, std::string &why,
    GemmMappingClass &mappingClass, bool &hardError)>;

/// Runs the records mode on `module`. `nextGemmIndex` is the pass's numbering of the GEMMs
/// lowered through a plan: set here from the module, advanced by `lowerOne` and by the
/// lowering from records alike. A failure is the pass's failure; what failed has been
/// reported on the module or on the nest.
LogicalResult runGeneralizedMatmulToSystolic(ModuleOp module,
                                             const GeneralizedMatmulOptions &options,
                                             int64_t &nextGemmIndex,
                                             DenseMatmulLowering lowerOne);

} // namespace systolic
} // namespace mlir

#endif // SYSTOLIC_GENERALIZEDMATMULTOSYSTOLIC_H
