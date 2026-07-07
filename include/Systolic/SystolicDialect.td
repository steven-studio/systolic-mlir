#ifndef SYSTOLIC_DIALECT_TD
#define SYSTOLIC_DIALECT_TD

include "mlir/IR/OpBase.td"

// -----------------------------------------------------------------------
// Dialect 定义
//
// 命名空间 "systolic",ops 会写成 systolic.pe_array / systolic.stream / systolic.mac
// -----------------------------------------------------------------------
def Systolic_Dialect : Dialect {
  let name = "systolic";
  let summary = "Spatial mapping dialect for systolic array accelerators";
  let description = [{
    Systolic dialect 表达「空间映射决策」:哪些维度映射到 PE 阵列的
    空间维度(row/col),哪些维度保留成时间维度(在同一个 PE 里 iterate),
    以及 operand 的 stationary 策略(weight-stationary / output-stationary /
    input-stationary)。

    这一层刻意不表达时脉、register、FIFO 深度等电路细节——那些留给
    下游的 HLS codegen 或 CIRCT 处理。
  }];
  let cppNamespace = "::mlir::systolic";
}

// 所有 Systolic op 的共同基础 class
class Systolic_Op<string mnemonic, list<Trait> traits = []> :
    Op<Systolic_Dialect, mnemonic, traits>;

#endif // SYSTOLIC_DIALECT_TD
