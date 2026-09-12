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
    systolic.device @acc_8x8 rows = 8 cols = 8 dataflow = output_stationary
      {k_max = 256 : i64, tile_overhead = 95 : i64,
       dma_bytes_per_cycle = 32.0 : f64, l1_bytes = 16384 : i64}
    systolic.device @acc_4x4 rows = 4 cols = 4 dataflow = output_stationary
      {k_max = 256 : i64, tile_overhead = 95 : i64}
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
    Systolic_DataflowKind:$dataflow,

    // Reduction capacity: how many reduction steps the operand buffers
    // hold in one invocation. This is a synthesis-time parameter of the
    // implementation, not a property of the architecture, and it decides
    // how many times the per-invocation costs recur for a deep reduction.
    // Distinct from l1_bytes, which is a scratchpad capacity read only by
    // the optional capacity check in device selection.
    OptionalAttr<I64Attr>:$k_max,
    OptionalAttr<I64Attr>:$l1_bytes,
    OptionalAttr<F64Attr>:$clock_hz,
    OptionalAttr<F64Attr>:$dma_bytes_per_cycle,

    // Calibration constants, per microarchitecture rather than per array
    // shape. The geometric term of the cost model is determined by rows,
    // cols and k_max; these two describe what the datapath adds on top of
    // it -- pipeline latency of the arithmetic, the final reduction, and
    // whatever the output handshake costs.
    //
    // They belong on the device rather than in the model because two
    // accelerators of identical geometry can differ here by more than an
    // order of magnitude. The HLS pipeline this model was first fitted to
    // has tile_overhead 6; the hand-written fold RTL measures 104. A
    // single global constant cannot describe both, and using the wrong one
    // makes device selection pick the wrong accelerator for the right
    // reason.
    //
    // tile_overhead has no default: a device that does not declare it is
    // costed with H = 0, which is exactly the geometric model. Comparing
    // the calibrated map against the geometric one is therefore a matter
    // of one attribute.
    OptionalAttr<I64Attr>:$initiation_interval,
    OptionalAttr<I64Attr>:$tile_overhead
  );

  let assemblyFormat = "$sym_name `rows` `=` $rows `cols` `=` $cols "
      "`dataflow` `=` $dataflow attr-dict";
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
    Systolic_StreamDirection:$direction,
    I64Attr:$skew
  );
  let results = (outs AnyType:$result);

  let assemblyFormat = [{
      $data `direction` `(` $direction `)` `skew` `(` $skew `)` attr-dict
      `:` functional-type(operands, results)
    }];

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
    Systolic_StationaryKind:$stationary
  );
  let results = (outs AnyType:$result);

  let assemblyFormat = [{
      `<` $rows `x` $cols `>` `stationary` `(` $stationary `)`
      $stationary_operand `,` $moving_operand (`,` $acc_operand^)? attr-dict
      `:` functional-type(operands, results)
    }];

  let hasVerifier = 1;
}

//===----------------------------------------------------------------------===//
// MacOp
//===----------------------------------------------------------------------===//

def Systolic_MacOp : Systolic_Op<"mac", [Pure,
    AllTypesMatch<["a", "a_out"]>,
    AllTypesMatch<["b", "b_out"]>,
    AllTypesMatch<["acc_in", "acc_out"]>]> {
  let summary = "單一 PE 的 multiply-accumulate,pass-through a/b,累加 acc";
  let description = [{
    Takes (a, b, acc) and returns (a, b, acc + a*b). Both input operands are
    forwarded unchanged to the neighbouring PEs -- that pass-through is the
    systolic property, and the type constraints above state it, so an
    expansion that dropped an operand would not verify. The accumulator type
    is deliberately left free of the input types: an int8 or bf16 array
    accumulating in a wider type is the same operation.
  }];
  let arguments = (ins AnyType:$a, AnyType:$b, AnyType:$acc_in);
  let results = (outs AnyType:$a_out, AnyType:$b_out, AnyType:$acc_out);

  let assemblyFormat = [{
      $a `,` $b `,` $acc_in attr-dict
      `:` type($a) `,` type($b) `,` type($acc_in)
    }];
}

//===----------------------------------------------------------------------===//
// systolic.matmul_tile -- one GEMM tile, optionally mapped to a device
//===----------------------------------------------------------------------===//

def Systolic_MatmulTileOp : Systolic_Op<"matmul_tile",
    [Pure, AllTypesMatch<["c_in", "result"]>]> {
  let summary = "One (M,K)x(K,N) GEMM tile, optionally assigned to a "
                "systolic.device.";

  let arguments = (ins
    AnyRankedTensor:$a,
    AnyRankedTensor:$b,
    AnyRankedTensor:$c_in,
    I64Attr:$m,
    I64Attr:$n,
    I64Attr:$k,
    OptionalAttr<FlatSymbolRefAttr>:$device,
    OptionalAttr<I64Attr>:$est_cycles,
    OptionalAttr<I64Attr>:$start_cycle
  );
  let results = (outs AnyRankedTensor:$result);
  let hasVerifier = 1;   // 檢查 a=m×k, b=k×n, c_in=m×n

  let assemblyFormat = [{
    $a `,` $b `,` $c_in (`on` $device^)? attr-dict
    `:` `(` type($a) `,` type($b) `,` type($c_in) `)` `->` type($result)
  }];
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
