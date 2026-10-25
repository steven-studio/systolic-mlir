// RUN: systolic-cost fleet-sim --jobs %S/Inputs/cost_model_v2/case1_3x4x4_k16.jobs --k8 16 --k4 16 --dpti-phase-ps 3245 --dpti-period-ps 16666 | FileCheck %s
// RUN: %python %S/../../eval/cost_model_v2/check_r2_invariants.py --tool systolic-cost
//
// Observed on the unchanged Shared Writeback Revision 2 RTL, not generated
// from the model. Reproduce using build_bench.sh and validate_r2.py; every
// pre-edge resource state and final output timestamp is checked there.
// VH bytes=240 last_abs_ps=14524419, origin=2205000 ps, first fire=2.
//
// CHECK: job 0 dev=1 k=16 {{.*}} fire=2 {{.*}} fold=269 cv=386 wbreq=413 wbacc=413 done=422
// CHECK: job 1 dev=2 k=16 {{.*}} fire=4 {{.*}} fold=394 cv=511 wbreq=538 wbacc=538 done=547
// CHECK: job 2 dev=3 k=16 {{.*}} fire=6 {{.*}} fold=399 cv=516 wbreq=543 wbacc=548 done=557
// CHECK: enqueue 0 cycle=412
// CHECK: enqueue 1 cycle=537
// CHECK: enqueue 2 cycle=542
// CHECK: output expected=240 emitted=240 last_byte_ps=12299419
// CHECK: fleet device_makespan=555 readback_end=743 last_byte_to_host=1230
