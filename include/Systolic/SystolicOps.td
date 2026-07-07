#ifndef SYSTOLIC_OPS_TD
#define SYSTOLIC_OPS_TD

include "Systolic/SystolicDialect.td"
include "mlir/IR/BuiltinTypes.td"
include "mlir/IR/EnumAttr.td"
include "mlir/Interfaces/SideEffectInterfaces.td"

// -----------------------------------------------------------------------
// Enum:stationary operand 策略
// -----------------------------------------------------------------------
def Stationary_Weight : I32EnumAttrCase<"weight", 0>;
def Stationary_Input  : I32EnumAttrCase<"input", 1>;
def Stationary_Output : I32EnumAttrCase<"output", 2>;

def StationaryKindAttr : I32EnumAttr<"StationaryKind", "PE stationary operand",
    [Stationary_Weight, Stationary_Input, Stationary_Output]> {
  let cppNamespace = "::mlir::systolic";
}

// -----------------------------------------------------------------------
// Enum:资料 stream 方向
// -----------------------------------------------------------------------
def Direction_Row : I32EnumAttrCase<"row", 0>;
def Direction_Col : I32EnumAttrCase<"col", 1>;

def StreamDirectionAttr : I32EnumAttr<"StreamDirection", "systolic stream direction",
    [Direction_Row, Direction_Col]> {
  let cppNamespace = "::mlir::systolic";
}

// -----------------------------------------------------------------------
// systolic.pe_array
//
// 把一段计算(通常来自 linalg.matmul lowering 而来)标记成一个 PE 阵列区块。
// rows/cols 是空间维度大小,stationary 决定哪个 operand 固定在 PE 里。
// -----------------------------------------------------------------------
def Systolic_PEArrayOp : Systolic_Op<"pe_array"> {
  let summary = "Marks a computation region as mapped onto a PE array";

  let arguments = (ins
    AnyRankedTensor:$stationary_operand,
    AnyRankedTensor:$moving_operand,
    Optional<AnyRankedTensor>:$acc_operand,
    I64Attr:$rows,
    I64Attr:$cols,
    StationaryKindAttr:$stationary
  );
  let results = (outs AnyRankedTensor:$result);

  let assemblyFormat = [{
    `<` $rows `x` $cols `>` `stationary` `(` $stationary `)`
    $stationary_operand `,` $moving_operand (`,` $acc_operand^)?
    attr-dict `:` functional-type(operands, results)
  }];

  let hasVerifier = 1;
}

// -----------------------------------------------------------------------
// systolic.stream
//
// 表达资料沿 row 或 col 方向进入阵列,skew 是相邻 PE 之间的时间偏移
// (systolic array 的核心特征:资料像波一样斜向传播)。
// -----------------------------------------------------------------------
def Systolic_StreamOp : Systolic_Op<"stream", [Pure]> {
  let summary = "Streams an operand into the PE array along a direction";

  let arguments = (ins
    AnyRankedTensor:$data,
    StreamDirectionAttr:$direction,
    I64Attr:$skew
  );
  let results = (outs AnyRankedTensor:$result);

  let assemblyFormat = [{
    $data `direction` `(` $direction `)` `skew` `(` $skew `)`
    attr-dict `:` functional-type($data, results)
  }];
  let hasVerifier = 1;
}

// -----------------------------------------------------------------------
// systolic.mac
//
// 单一 PE 内部的计算原语:acc = a * b + acc,并把 a, b 传给下一个 PE。
// 这个 op 之后会被 lowering pass 展开成阵列大小份、接上 stream skew。
// -----------------------------------------------------------------------
def Systolic_MacOp : Systolic_Op<"mac", [Pure]> {
  let summary = "Single PE multiply-accumulate-and-forward primitive";

  let arguments = (ins AnyType:$a, AnyType:$b, AnyType:$acc);
  let results = (outs AnyType:$acc_out, AnyType:$a_out, AnyType:$b_out);

  let assemblyFormat = [{
    $a `,` $b `,` $acc attr-dict `:` functional-type(operands, results)
  }];
}

#endif // SYSTOLIC_OPS_TD
