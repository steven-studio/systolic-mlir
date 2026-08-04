#ifndef SYSTOLIC_SYSTOLICOPS_H
#define SYSTOLIC_SYSTOLICOPS_H

#include "mlir/IR/BuiltinTypes.h"
#include "mlir/IR/OpDefinition.h"
#include "mlir/IR/SymbolTable.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Bytecode/BytecodeOpInterface.h"
#include "mlir/IR/OpImplementation.h"

#include "Systolic/SystolicDialect.h"

// 由 CMake 的 mlir_tablegen(... -gen-enum-decls) 产生
#include "Systolic/SystolicOpsEnums.h.inc"

#define GET_OP_CLASSES
#include "Systolic/SystolicOps.h.inc"

#endif // SYSTOLIC_SYSTOLICOPS_H
