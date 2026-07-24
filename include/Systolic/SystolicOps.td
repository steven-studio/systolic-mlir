//===- SystolicOps.td - Systolic dialect ops --------------*- tablegen -*-===//
//
// NOTE: written without a local LLVM/MLIR toolchain to run mlir-tblgen
// against -- see README.md "Status" before relying on exact syntax here,
// in particular the inline `$dataflow` keyword usage in
// Systolic_DeviceOp's assemblyFormat.
//
//===----------------------------------------------------------------------===//

#ifndef SYSTOLIC_OPS
#define SYSTOLIC_OPS

include "Systolic/SystolicDialect.td"
include "mlir/IR/EnumAttr.td"
include "mlir/IR/SymbolInterfaces.td"
include "mlir/Interfaces/SideEffectInterfaces.td"   // <-- 加這行,提供 Pure trait

//===----------------------------------------------------------------------===//
// Dataflow enum
//===----------------------------------------------------------------------===//

def Systolic_WeightStationary : I32EnumAttrCase<"WeightStationary", 0, "weight_stationary">;
def Systolic_OutputStationary : I32EnumAttrCase<"OutputStationary", 1, "output_stationary">;
def Systolic_RowStationary    : I32EnumAttrCase<"RowStationary", 2, "row_stationary">;

def Systolic_DataflowKind : I32EnumAttr<
    "DataflowKind", "systolic array dataflow / reuse strategy",
    [Systolic_WeightStationary, Systolic_OutputStationary, Systolic_RowStationary]> {
  let cppNamespace = "::mlir::systolic";
}

def Systolic_DataflowAttr : EnumAttr<Systolic_Dialect, Systolic_DataflowKind, "dataflow">;

//===----------------------------------------------------------------------===//
// systolic.device -- hardware description of one accelerator instance
//===----------------------------------------------------------------------===//

def Systolic_DeviceOp : Systolic_Op<"device", [Symbol]> {
  let summary = "Declares one heterogeneous accelerator instance and its "
                "fixed hardware geometry.";
  let description = [{
    Example:
    ```mlir
    systolic.device @acc_8x8 rows = 8 cols = 8 depth = 1 dataflow = weight_stationary
    systolic.device @acc_4x4 rows = 4 cols = 4 depth = 1 dataflow = weight_stationary
    ```
    Every numeric field here is an ordinary attribute: nothing about this
    op is specific to one physical board. Re-targeting to a different
    array configuration means editing these attributes, not the dialect
    or the passes that consume them.
  }];

  let arguments = (ins
    SymbolNameAttr:$sym_name,
    I64Attr:$rows,
    I64Attr:$cols,
    I64Attr:$depth,
    Systolic_DataflowKind:$dataflow,
    OptionalAttr<F64Attr>:$clock_hz,
    OptionalAttr<F64Attr>:$dma_bytes_per_cycle
  );

  let assemblyFormat = "$sym_name `rows` `=` $rows `cols` `=` $cols "
      "`depth` `=` $depth `dataflow` `=` $dataflow attr-dict";
}

//===----------------------------------------------------------------------===//
// StationaryKind / StreamDirection enums
//===----------------------------------------------------------------------===//

def Systolic_StationaryKind : I32EnumAttr<"StationaryKind",
    "operand kept stationary in the PE array",
    [
      I32EnumAttrCase<"weight", 0>,
      I32EnumAttrCase<"activation", 1>
    ]> {
  let cppNamespace = "::mlir::systolic";
}
def Systolic_StationaryKindAttr :
    EnumAttr<Systolic_Dialect, Systolic_StationaryKind, "stationary">;

def Systolic_StreamDirection : I32EnumAttr<"StreamDirection",
    "systolic array 的資料流方向",
    [
      I32EnumAttrCase<"row", 0>,
      I32EnumAttrCase<"col", 1>
    ]> {
  let cppNamespace = "::mlir::systolic";
}
def Systolic_StreamDirectionAttr :
    EnumAttr<Systolic_Dialect, Systolic_StreamDirection, "stream_direction">;

//===----------------------------------------------------------------------===//
// StreamOp
//===----------------------------------------------------------------------===//

def Systolic_StreamOp : Systolic_Op<"stream", [Pure]> {
  let summary = "把資料以 skew 排程送進 systolic array 的一路 stream";
  let arguments = (ins
    AnyType:$data,
    Systolic_StreamDirectionAttr:$direction,
    I64Attr:$skew
  );
  let results = (outs AnyType:$result);
  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// PEArrayOp
//===----------------------------------------------------------------------===//

def Systolic_PEArrayOp : Systolic_Op<"pe_array", [Pure]> {
  let summary = "一個 rows x cols 的 PE array,對 stationary/moving operand 做 matmul";
  let arguments = (ins
    AnyType:$stationary_operand,
    AnyType:$moving_operand,
    Optional<AnyType>:$acc_operand,
    I64Attr:$rows,
    I64Attr:$cols,
    Systolic_StationaryKindAttr:$stationary
  );
  let results = (outs AnyType:$result);
  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// MacOp
//===----------------------------------------------------------------------===//

def Systolic_MacOp : Systolic_Op<"mac", [Pure]> {
  let summary = "單一 PE 的 multiply-accumulate,pass-through a/b,累加 acc";
  let arguments = (ins AnyType:$a, AnyType:$b, AnyType:$acc_in);
  let results = (outs AnyType:$a_out, AnyType:$b_out, AnyType:$acc_out);
}

//===----------------------------------------------------------------------===//
// systolic.matmul_tile -- one GEMM tile, optionally mapped to a device
//===----------------------------------------------------------------------===//

def Systolic_MatmulTileOp : Systolic_Op<"matmul_tile"> {
  let summary = "One (M,K)x(K,N) GEMM tile, optionally assigned to a "
                "systolic.device.";
  let description = [{
    `device` is left unset by the frontend and filled in by the
    accelerator-selection pass (stage 1, not yet written). `est_cycles` is
    left unset until the cost-analysis pass runs. Both are ordinary
    optional attributes, so this op is valid IR at every stage of the
    pipeline, not just the end.

    Before selection:
    ```mlir
    %0 = systolic.matmul_tile(64, 64, 64) : tensor<64x64xf32>
    ```
    After selection + cost analysis:
    ```mlir
    %0 = systolic.matmul_tile(64, 64, 64) on @acc_8x8 {est_cycles = 4110 : i64}
        : tensor<64x64xf32>
    ```

    NOTE: this first cut has no real tensor operands (it is a schedulable
    task node, not yet wired to actual data) -- adding those is a
    follow-up once the scheduling passes exist.
  }];

  let arguments = (ins
    I64Attr:$m,
    I64Attr:$n,
    I64Attr:$k,
    OptionalAttr<FlatSymbolRefAttr>:$device,
    OptionalAttr<I64Attr>:$est_cycles
  );
  let results = (outs AnyType:$result);

  let assemblyFormat = "`(` $m `,` $n `,` $k `)` (`on` $device^)? attr-dict `:` type($result)";
}

//===----------------------------------------------------------------------===//
// systolic.dma -- a scratchpad data transfer
//===----------------------------------------------------------------------===//

def Systolic_DmaOp : Systolic_Op<"dma"> {
  let summary = "A data transfer to/from a device's local scratchpad.";
  let description = [{
    Kept as its own op, separate from compute, on purpose: whether a given
    `systolic.dma` is serialized before/after a `systolic.matmul_tile` or
    overlapped with it is a *scheduling* decision (stage 3, not yet
    written), not something baked into the IR at this level.
  }];

  let arguments = (ins
    I64Attr:$bytes,
    OptionalAttr<FlatSymbolRefAttr>:$device,
    OptionalAttr<I64Attr>:$est_cycles
  );

  let assemblyFormat = "$bytes (`on` $device^)? attr-dict";
}

#endif // SYSTOLIC_OPS
