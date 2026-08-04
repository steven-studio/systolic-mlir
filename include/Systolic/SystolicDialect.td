//===- SystolicDialect.td - Systolic dialect definition --*- tablegen -*-===//
//
// Defines the `systolic` MLIR dialect: an IR for expressing GEMM tiles,
// DMA transfers, and heterogeneous accelerator hardware descriptions for
// SoCs built from multiple fixed-geometry systolic arrays.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_DIALECT
#define SYSTOLIC_DIALECT

include "mlir/IR/OpBase.td"

def Systolic_Dialect : Dialect {
  let name = "systolic";
  let summary = "IR for scheduling GEMM tiles across heterogeneous, "
                "fixed-geometry systolic array accelerators.";
  let description = [{
    The `systolic` dialect models three things at compile time:

    1. Hardware description -- one `systolic.device` op per accelerator
       instance in the target SoC, carrying its fixed array geometry
       (rows, cols, depth) and dataflow as IR attributes. These are data,
       not compiled-in constants, so the same IR and passes retarget to a
       different board just by changing these attributes.
    2. Compute tiles -- `systolic.matmul_tile` ops representing one
       (M, K) x (K, N) GEMM tile, optionally already assigned to a device.
    3. Data movement -- `systolic.dma` ops representing scratchpad
       transfers, kept separate from compute so a later scheduling pass
       can decide whether to serialize or overlap them.

    This dialect intentionally stays low-level and analysis-friendly: it
    is the substrate that the cost model and scheduling passes (selection,
    partitioning, overlap-aware parallel scheduling) operate over.
  }];
  let cppNamespace = "::mlir::systolic";
}

// Base class for all ops in this dialect.
class Systolic_Op<string mnemonic, list<Trait> traits = []> :
    Op<Systolic_Dialect, mnemonic, traits>;

#endif // SYSTOLIC_DIALECT
