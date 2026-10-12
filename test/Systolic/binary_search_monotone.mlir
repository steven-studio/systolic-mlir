// RUN: systolic-cost check-monotone --m 64 --n 64 --k 64 | FileCheck %s
// RUN: systolic-cost check-monotone --m 64 --n 64 --k 64 --shared-read-words-per-cycle 3.51 | FileCheck %s
// RUN: systolic-cost check-monotone --m 100 --n 36 --k 147 --k8 24 --k4 12 | FileCheck %s
// RUN: systolic-cost check-monotone --m 100 --n 36 --k 147 --k8 24 --k4 12 --shared-read-words-per-cycle 1 | FileCheck %s
// RUN: systolic-cost check-monotone --m 49 --n 512 --k 4608 --k8 17 --k4 7 | FileCheck %s
// RUN: systolic-cost check-monotone --m 1 --n 1000 --k 512 --k8 64 --k4 8 --shared-read-words-per-cycle 3.627 | FileCheck %s
// RUN: systolic-cost check-monotone --m 13 --n 7 --k 1 --k8 1 --k4 1 --compute-only | FileCheck %s

// The schedule the binary search returns must be valid -- every output
// element covered exactly once, no padding-only tile, each tile on an
// accelerator of its size for exactly the cost model's tile cost, no two
// tiles of one accelerator overlapping, all done by the makespan -- and its
// deadline T* (SystolicOptimizer.cpp) the smallest feasible one --
// feasible(T* - 1) false -- and feasibility must
// hold at every deadline probed past it (T* .. T* + 16, 2 T*, 4 T*, 8 T*),
// with the cost model's read / compute / write tile costs (every invocation
// at its own depth), with and without a shared read bandwidth, on shapes
// that are not multiples of either geometry.
// CHECK: schedule makespan={{[0-9]+}}
// CHECK: check valid=yes minimal=yes monotone=yes
