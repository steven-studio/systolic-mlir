// GEMMs written with scf.while that the lowering from records refuses (stage 3 of
// docs/GENERALIZED_GEMM_SCOPE.md, on scf.while loops), each with its reason, and one it takes.
// A refused nest stays as it is. Each case is the research tool's own record of a nest
// (Inputs/nest_records_while_refusals.json, written by
// research/generalized-gemm/test/fuzz/nest_gate_records.py), or a record with @whl_gemm's
// verified content (Inputs/nest_records_while.json) transplanted onto the nest's own binding
// by Inputs/edit_nest_records.py, so that every check on the record passes and a check on the
// nest refuses.
//
// The research tool's own records: the status of each record the research does not verify (a
// loop that never ends or has no termination certificate, a wrong update, another condition, a
// side effect, a read outside A, a bound or a start known at run time, a result used after the
// loop, a reduction carried in a value), the alias gate, the floating-point contract, three nests
// the research verifies and the lowering does not read as counting loops (arithmetic in the before
// region, a carried invariant, a counter of type i64), and a second carried induction, taken.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while_refusals.json record-driven=true report=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=HONEST
// HONEST: remark: GEMM not lowered from its record: nest 0 of step_first: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is UNKNOWN (no termination certificate)
// HONEST: remark: GEMM not lowered from its record: nest 0 of no_step: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is UNKNOWN (no termination certificate)
// HONEST: remark: GEMM not lowered from its record: nest 0 of always: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of compares_other: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of condition_other: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of swapped_wrong: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of eq_bound: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of step2: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: GEMM not lowered from its record: nest 0 of step2_first: the record's status is VERIFIED_NOT_MAPPED
// HONEST: remark: GEMM not lowered from its record: nest 0 of down: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of sle: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of out_of_bounds: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of before_store: the record's status is NOT_VERIFIED: the nest writes memory outside the multiply-accumulate: 1 write(s) in the frame of the computation claim
// HONEST: remark: GEMM not lowered from its record: nest 0 of second_write: the record's status is NOT_VERIFIED: the nest writes memory outside the multiply-accumulate: 1 write(s) in the frame of the computation claim
// HONEST: remark: GEMM not lowered from its record: nest 0 of alias: A may overlap C; declare the arguments llvm.noalias (C restrict) or pass assume-no-alias
// HONEST: remark: GEMM not lowered from its record: nest 0 of strict: floating-point reassociation is not permitted
// HONEST: remark: GEMM not lowered from its record: nest 0 of result_used: the record's status is {{UNSUPPORTED}}: the nest has no faithful copy: the loop yields a value that is used after it (result 0 of scf.while): the nest's effect is not on memory alone
// HONEST: remark: GEMM not lowered from its record: nest 0 of inner_result_used: the record's status is NOT_VERIFIED: the nest writes memory outside the multiply-accumulate: 1 write(s) in the frame of the computation claim
// HONEST: remark: GEMM not lowered from its record: nest 0 of dynamic: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of dynamic_init: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of forwards_other: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is UNKNOWN (no termination certificate)
// HONEST: remark: GEMM not lowered from its record: nest 0 of before_compute: a loop of the nest (scf.while) computes more in its before region than one comparison of its counter (arith.addi), and a counting loop's holds that comparison and constants
// HONEST: remark: GEMM not lowered from its record: nest 0 of unsigned_negative: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of unsigned_negative_upper: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of ne_below: the record's status is NOT_VERIFIED
// HONEST: remark: GEMM not lowered from its record: nest 0 of accumulator: the record's status is NOT_VERIFIED: no claim was made about the nest: the gate is {{UNSUPPORTED}} (a scalar accumulator has no exact output view)
// HONEST: remark: GEMM 16x8x12 of nest 0 of second_induction lowered from its record as 5 exact region(s) (formulas:
// HONEST: remark: GEMM not lowered from its record: nest 0 of invariant: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: its before region's comparison does not read exactly one of them, the counter, and a constant; value 0 (index) is stepped by a constant; value 1 (index) is carried unchanged
// HONEST: remark: GEMM not lowered from its record: nest 0 of i64: a loop of the nest (scf.while) carries a value of type i64, and a counting loop's counter is an index
// HONEST: remark: GEMM not lowered from its record: nest 0 of no_carried: the record's status is NOT_VERIFIED
// HONEST-NOT: remark:
//
// Every nest stays as written: no tile is made, and each scf.while is still there.
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%S/Inputs/nest_records_while_refusals.json record-driven=true" | FileCheck %s --check-prefix=KEPT --implicit-check-not=systolic.matmul_tile
// KEPT-LABEL: func.func @step_first(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @no_step(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @always(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @compares_other(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @condition_other(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @swapped_wrong(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @eq_bound(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @step2(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @step2_first(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @down(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @sle(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @out_of_bounds(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @before_store(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @second_write(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @alias(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @strict(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @result_used(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @inner_result_used(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @dynamic(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @dynamic_init(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @forwards_other(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @before_compute(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @unsigned_negative(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @unsigned_negative_upper(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @ne_below(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @accumulator(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @second_induction(
// KEPT-COUNT-5: systolic.matmul_tile
// (@second_induction is taken since the lowering reads carried indices: its tiles, no loop;
// nest_records_carried.mlir has the nests whose subscripts are carried.)
// KEPT-LABEL: func.func @invariant(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @i64(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
// KEPT-LABEL: func.func @no_carried(
// KEPT: scf.while
// KEPT: scf.while
// KEPT: scf.while
//
// Transplants: every check on the record passes, and the nest's own shape refuses.
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-no_step %S/Inputs/nest_records_while.json whl_gemm > %t.nostep.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.nostep.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOSTEP
// NOSTEP: remark: GEMM not lowered from its record: nest 0 of no_step: a loop of the nest (scf.while) does not step its counter by 1: its after region yields another value than counter + 1
// NOSTEP-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-always %S/Inputs/nest_records_while.json whl_gemm > %t.always.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.always.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ALWAYS
// ALWAYS: remark: GEMM not lowered from its record: nest 0 of always: a loop of the nest (scf.while) does not go on while its counter is below a bound
// ALWAYS-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-compares_other %S/Inputs/nest_records_while.json whl_gemm > %t.comparesother.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.comparesother.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=COMPARESOTHER
// COMPARESOTHER: remark: GEMM not lowered from its record: nest 0 of compares_other: a loop of the nest (scf.while) does not go on while its counter is below a bound
// COMPARESOTHER-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-condition_other %S/Inputs/nest_records_while.json whl_gemm > %t.conditionother.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.conditionother.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=CONDITIONOTHER
// CONDITIONOTHER: remark: GEMM not lowered from its record: nest 0 of condition_other: a loop of the nest (scf.while) does not go on by one comparison of its counter with a constant
// CONDITIONOTHER-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-swapped_wrong %S/Inputs/nest_records_while.json whl_gemm > %t.swappedwrong.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.swappedwrong.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SWAPPEDWRONG
// SWAPPEDWRONG: remark: GEMM not lowered from its record: nest 0 of swapped_wrong: a loop of the nest (scf.while) does not go on while its counter is below a bound
// SWAPPEDWRONG-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-eq_bound %S/Inputs/nest_records_while.json whl_gemm > %t.eqbound.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.eqbound.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=EQBOUND
// EQBOUND: remark: GEMM not lowered from its record: nest 0 of eq_bound: a loop of the nest (scf.while) does not go on while its counter is below a bound
// EQBOUND-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-step2 %S/Inputs/nest_records_while.json whl_gemm > %t.steptwo.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.steptwo.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STEPTWO
// STEPTWO: remark: GEMM not lowered from its record: nest 0 of step2: a loop of the nest (scf.while) does not step its counter by 1: its after region yields another value than counter + 1
// STEPTWO-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-step2_first %S/Inputs/nest_records_while.json whl_gemm > %t.steptwofirst.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.steptwofirst.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=STEPTWOFIRST
// STEPTWOFIRST: remark: GEMM not lowered from its record: nest 0 of step2_first: a loop of the nest (scf.while) does not step its counter by 1: its after region yields another value than counter + 1
// STEPTWOFIRST-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-down %S/Inputs/nest_records_while.json whl_gemm > %t.down.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.down.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DOWN
// DOWN: remark: GEMM not lowered from its record: nest 0 of down: a loop of the nest (scf.while) does not step its counter by 1: its after region yields another value than counter + 1
// DOWN-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-sle %S/Inputs/nest_records_while.json whl_gemm > %t.sle.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.sle.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SLE
// SLE: remark: GEMM not lowered from its record: nest 0 of sle: a loop of the nest (scf.while) does not go on while its counter is below a bound
// SLE-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-out_of_bounds %S/Inputs/nest_records_while.json whl_gemm > %t.outofbounds.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.outofbounds.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=OUTOFBOUNDS
// OUTOFBOUNDS: remark: GEMM not lowered from its record: nest 0 of out_of_bounds: A is read out of bounds along dimension 1
// OUTOFBOUNDS-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-before_store %S/Inputs/nest_records_while.json whl_gemm > %t.beforestore.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.beforestore.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=BEFORESTORE
// BEFORESTORE: remark: GEMM not lowered from its record: nest 0 of before_store: a loop of the nest (scf.while) computes more in its before region than one comparison of its counter (memref.store), and a counting loop's holds that comparison and constants
// BEFORESTORE-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-second_write %S/Inputs/nest_records_while.json whl_gemm > %t.secondwrite.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.secondwrite.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=SECONDWRITE
// SECONDWRITE: remark: GEMM not lowered from its record: nest 0 of second_write: the loop body is not one multiply-accumulate: 3 loads, 2 stores, 1 multiplications, 1 additions
// SECONDWRITE-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-result_used %S/Inputs/nest_records_while.json whl_gemm > %t.resultused.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.resultused.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=RESULTUSED
// RESULTUSED: remark: GEMM not lowered from its record: nest 0 of result_used: a loop of the nest (scf.while) gives its counter's last value to the code after it, and the lowering would remove the loop that makes it
// RESULTUSED-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-inner_result_used %S/Inputs/nest_records_while.json whl_gemm > %t.innerresult.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.innerresult.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=INNERRESULT
// INNERRESULT: remark: GEMM not lowered from its record: nest 0 of inner_result_used: the nest is no perfect band: a loop's body holds more than the next loop and constants and the step of its counter
// INNERRESULT-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-dynamic %S/Inputs/nest_records_while.json whl_gemm > %t.dynamic.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.dynamic.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMIC
// DYNAMIC: remark: GEMM not lowered from its record: nest 0 of dynamic: a loop of the nest (scf.while) compares its counter with a bound that is no constant: known only at run time
// DYNAMIC-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-dynamic_init %S/Inputs/nest_records_while.json whl_gemm > %t.dynamicinit.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.dynamicinit.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=DYNAMICINIT
// DYNAMICINIT: remark: GEMM not lowered from its record: nest 0 of dynamic_init: a loop of the nest (scf.while) starts its counter at a value that is no constant: known only at run time
// DYNAMICINIT-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-forwards_other %S/Inputs/nest_records_while.json whl_gemm > %t.forwards.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.forwards.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=FORWARDS
// FORWARDS: remark: GEMM not lowered from its record: nest 0 of forwards_other: a loop of the nest (scf.while) does not hand its counter to its body as it is: scf.condition forwards another value
// FORWARDS-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-unsigned_negative %S/Inputs/nest_records_while.json whl_gemm > %t.unsigned.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unsigned.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNSIGNED
// UNSIGNED: remark: GEMM not lowered from its record: nest 0 of unsigned_negative: a loop of the nest (scf.while) compares its counter with its bound unsigned, and one of them is negative
// UNSIGNED-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-unsigned_negative_upper %S/Inputs/nest_records_while.json whl_gemm > %t.unsignedupper.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.unsignedupper.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=UNSIGNEDUPPER
// UNSIGNEDUPPER: remark: GEMM not lowered from its record: nest 0 of unsigned_negative_upper: a loop of the nest (scf.while) compares its counter with its bound unsigned, and one of them is negative
// UNSIGNEDUPPER-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-ne_below %S/Inputs/nest_records_while.json whl_gemm > %t.nebelow.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.nebelow.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NEBELOW
// NEBELOW: remark: GEMM not lowered from its record: nest 0 of ne_below: a loop of the nest (scf.while) goes on while its counter differs from a bound below its start: it does not stop at the bound
// NEBELOW-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-accumulator %S/Inputs/nest_records_while.json whl_gemm > %t.accumulator.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.accumulator.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=ACCUMULATOR
// ACCUMULATOR: remark: GEMM not lowered from its record: nest 0 of accumulator: a loop of the nest (scf.while) carries 2 values from one iteration to the next, and the record's lowering takes a counting loop, which carries its counter and indices stepped by a constant only: value 1 has type f32, and the record's lowering takes a carried index only; value 0 (index) is stepped by a constant; value 1 (f32) is updated by the loop: a reduction or datum held in a value, not in memory
// ACCUMULATOR-NOT: remark:
// RUN: %python %S/Inputs/edit_nest_records.py %S/Inputs/nest_records_while_refusals.json transplant-no_carried %S/Inputs/nest_records_while.json whl_gemm > %t.nocarried.json
// RUN: systolic-opt %s --systolic-declare-fpga-fleet --affine-matmul-to-systolic="nest-records=%t.nocarried.json record-driven=true" -o /dev/null 2>&1 | FileCheck %s --check-prefix=NOCARRIED
// NOCARRIED: remark: GEMM not lowered from its record: nest 0 of no_carried: a loop of the nest (scf.while) carries no value from one iteration to the next: it has no counter, and the record's lowering takes a counting loop
// NOCARRIED-NOT: remark:
// The middle loop steps its counter before the inner loop: counter + 1 is held across that loop, and
// the research has no termination certificate for it. The lowering would read the loop.
func.func @step_first(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %jn = arith.addi %jb, %c1 : index
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The inner loop yields its counter unchanged: it never ends.
func.func @no_step(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kb : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The inner loop goes on while its counter is at least 0: past every bound of the arrays.
func.func @always(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi sge, %k, %c0 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The inner loop's comparison does not read its counter.
func.func @compares_other(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %c0, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The inner loop compares its counter and goes on by a constant instead: it stops at no bound.
func.func @condition_other(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        %go = arith.constant true
        scf.condition(%go) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// bound < counter: false at the start, so K never runs.
func.func @swapped_wrong(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %c12, %k : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// counter == bound: false at the start, so K never runs.
func.func @eq_bound(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi eq, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A step of 2 for I.
func.func @step2(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %c2 = arith.constant 2 : index
    %in = arith.addi %ib, %c2 : index
    scf.yield %in : index
  }
  return
}
// A step of 2 for I, the constant first.
func.func @step2_first(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %c2 = arith.constant 2 : index
    %in = arith.addi %c2, %ib : index
    scf.yield %in : index
  }
  return
}
// The inner loop counts down from 0: a wrong update.
func.func @down(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.subi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// counter <= 12: thirteen iterations of K, and a read outside A.
func.func @sle(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi sle, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A has 11 columns and K runs to 12: a read outside A.
func.func @out_of_bounds(%a: memref<16x11xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x11xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A store in the inner loop's before region: the condition has a side effect.
func.func @before_store(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %zero = arith.constant 0.0 : f32
        memref.store %zero, %d[%c0, %c0] : memref<16x8xf32>
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A second write beside the update.
func.func @second_write(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %d: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        memref.store %m, %d[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// No llvm.noalias on C.
func.func @alias(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// No reassoc on the accumulation.
func.func @strict(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The outer loop's result, its counter's last value, is stored after the nest.
func.func @result_used(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %e: memref<4xindex>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  memref.store %ri, %e[%c0] : memref<4xindex>
  return
}
// The inner loop's result is stored in the middle loop's body.
func.func @inner_result_used(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %e: memref<4xindex>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      memref.store %rk, %e[%c0] : memref<4xindex>
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The bound of I is an argument, known only at run time.
func.func @dynamic(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %n : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// I starts at an argument, known only at run time.
func.func @dynamic_init(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %n: index) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %n) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// scf.condition hands the body a constant, not the counter: the counter is stepped from 0 each time.
func.func @forwards_other(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %c0 : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// Arithmetic in the before region: the research verifies the nest, the lowering reads a comparison of
// the counter itself only.
func.func @before_compute(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %kk = arith.addi %k, %c0 : index
        %ck = arith.cmpi slt, %kk, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// An unsigned comparison from -4: as unsigned, 2^64 - 4 is past 12, and the loop never runs.
func.func @unsigned_negative(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %cm4 = arith.constant -4 : index
  %ri = scf.while (%i = %cm4) : (index) -> index {
    %ci = arith.cmpi ult, %i, %c12 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// An unsigned comparison up to -4: as unsigned, 2^64 - 4, which the loop would run to.
func.func @unsigned_negative_upper(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %cm4 = arith.constant -4 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi ult, %i, %cm4 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c0) : (index) -> index {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// K goes on while it differs from 8, from 12: it does not stop at its bound.
func.func @ne_below(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %c12) : (index) -> index {
        %ck = arith.cmpi ne, %k, %c8 : index
        scf.condition(%ck) %k : index
      } do {
      ^bb0(%kb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn : index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// The reduction over K carried in a value beside the counter, stored after the loop.
func.func @accumulator(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %init = memref.load %c[%ib, %jb] : memref<16x8xf32>
      %rk, %sum = scf.while (%k = %c0, %acc = %init) : (index, f32) -> (index, f32) {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k, %acc : index, f32
      } do {
      ^bb0(%kb: index, %accb: f32):
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %accb, %m fastmath<reassoc> : f32
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn, %s : index, f32
      }
      memref.store %sum, %c[%ib, %jb] : memref<16x8xf32>
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A second carried index stepped with K and never read: another induction. The research verifies
// the nest, and the lowering takes it: the loop carries its counter and an index recurrence.
func.func @second_induction(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk, %rp = scf.while (%k = %c0, %p = %c0) : (index, index) -> (index, index) {
        %ck = arith.cmpi slt, %k, %c12 : index
        scf.condition(%ck) %k, %p : index, index
      } do {
      ^bb0(%kb: index, %pb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        %pn = arith.addi %pb, %c1 : index
        scf.yield %kn, %pn : index, index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A second carried index that never changes, the bound handed round the loop. The research verifies
// the nest; the lowering takes a loop whose comparison reads its counter and a constant.
func.func @invariant(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk, %rh = scf.while (%k = %c0, %h = %c12) : (index, index) -> (index, index) {
        %ck = arith.cmpi slt, %k, %h : index
        scf.condition(%ck) %k, %h : index, index
      } do {
      ^bb0(%kb: index, %hb: index):
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %kb, %c1 : index
        scf.yield %kn, %hb : index, index
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// A counter of type i64, cast to index for the accesses. The research verifies the nest; the
// lowering reads counters of index type.
func.func @i64(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}) {
  %z0 = arith.constant 0 : i64
  %z1 = arith.constant 1 : i64
  %z12 = arith.constant 12 : i64
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      %rk = scf.while (%k = %z0) : (i64) -> i64 {
        %ck = arith.cmpi slt, %k, %z12 : i64
        scf.condition(%ck) %k : i64
      } do {
      ^bb0(%k64: i64):
        %kb = arith.index_cast %k64 : i64 to index
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %kb] : memref<16x12xf32>
        %y = memref.load %b[%kb, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        %kn = arith.addi %k64, %z1 : i64
        scf.yield %kn : i64
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
// No carried value: the inner loop goes on by a flag read from memory, and has no counter.
func.func @no_carried(%a: memref<16x12xf32>, %b: memref<12x8xf32>, %c: memref<16x8xf32> {llvm.noalias}, %f: memref<1xi1>) {
  %c0 = arith.constant 0 : index
  %c1 = arith.constant 1 : index
  %c8 = arith.constant 8 : index
  %c12 = arith.constant 12 : index
  %c16 = arith.constant 16 : index
  %ri = scf.while (%i = %c0) : (index) -> index {
    %ci = arith.cmpi slt, %i, %c16 : index
    scf.condition(%ci) %i : index
  } do {
  ^bb0(%ib: index):
    %rj = scf.while (%j = %c0) : (index) -> index {
      %cj = arith.cmpi slt, %j, %c8 : index
      scf.condition(%cj) %j : index
    } do {
    ^bb0(%jb: index):
      scf.while : () -> () {
        %go = memref.load %f[%c0] : memref<1xi1>
        scf.condition(%go)
      } do {
        %z = memref.load %c[%ib, %jb] : memref<16x8xf32>
        %x = memref.load %a[%ib, %c0] : memref<16x12xf32>
        %y = memref.load %b[%c0, %jb] : memref<12x8xf32>
        %m = arith.mulf %x, %y : f32
        %s = arith.addf %z, %m fastmath<reassoc> : f32
        memref.store %s, %c[%ib, %jb] : memref<16x8xf32>
        scf.yield
      }
      %jn = arith.addi %jb, %c1 : index
      scf.yield %jn : index
    }
    %in = arith.addi %ib, %c1 : index
    scf.yield %in : index
  }
  return
}
