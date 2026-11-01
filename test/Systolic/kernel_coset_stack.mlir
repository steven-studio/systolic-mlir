// REQUIRES: cfront-clang
// RUN: %python %S/Inputs/kernel_coset_stack.py %S/../../runtime/offload %t %systolic-cc

// Stack use of the lowered kernel-coset offload (finding F-KC-4). The emission
// allocates its staging slabs inside the loops that drive its tiles, and
// nothing below MLIR releases them per iteration, so a valid, in-bounds nest
// faulted once its tiles outgrew the stack. The synchronous tile lowering now
// moves such a slab out of those loops when reuse is provably safe. The runner
// executes real lowered kernels, on the software runtime, and pins both the
// fix and the cases it must leave alone.
