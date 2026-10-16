// The integer lattice tools (include/Systolic/IntegerLattice.h,
// docs/AFFINE_GEMM_MAPPING.md section 19) on hand-picked cases with known
// answers: the matrices of the three specified tests, split coordinates, SQ-1,
// the injectivity criterion and the overflow guard. The randomized property test
// against an independent implementation is integer_lattice_random.mlir.
//
// RUN: systolic-opt --test-integer-lattice %s -o /dev/null 2>&1 | FileCheck %s

// Test 2: C's rows (1,1,-2), (1,2,1); the only direction keeping C fixed is d = (5,-3,1).
// CHECK: @test2_c: rank 2, kernel {{\[\[}}5, -3, 1]]
func.func @test2_c() attributes {lattice.matrix = [[1, 1, -2], [1, 2, 1]]} { return }
// Test 2: T = (Ic, Jc, Ka) has |det| 3 -- T(Z^3) has index 3: K = I (mod 3).
// CHECK: @test2_t: rank 3, kernel [], |det| 3
func.func @test2_t() attributes {lattice.matrix = [[1, 1, -2], [1, 2, 1], [1, 1, 1]]} { return }
// Test 2: A's kernel, u_J up to sign (-1,1,0).
// CHECK: @test2_a: rank 2, kernel {{\[\[}}1, -1, 0]]
func.func @test2_a() attributes {lattice.matrix = [[1, 1, -2], [1, 1, 1]]} { return }
// Test 2: B's kernel, u_I = (1,0,-1).
// CHECK: @test2_b: rank 2, kernel {{\[\[}}1, 0, -1]]
func.func @test2_b() attributes {lattice.matrix = [[1, 1, 1], [1, 2, 1]]} { return }
// Test 2: the columns u_I, u_J, d generate an index-3 lattice (the domain D* of section 19).
// CHECK: @test2_roles: rank 3, kernel [], |det| 3
func.func @test2_roles() attributes {lattice.matrix = [[1, -1, 5], [0, 1, -3], [-1, 0, 1]]} { return }
// Test 1: C's kernel is the k axis.
// CHECK: @test1_c: rank 2, kernel {{\[\[}}0, 0, 1]]
func.func @test1_c() attributes {lattice.matrix = [[1, 0, 0], [0, 1, 0]]} { return }
// Test 1: the six index vectors have rank 3.
// CHECK: @test1_six: rank 3, kernel []
func.func @test1_six() attributes {lattice.matrix = [[1, 0, 0], [0, 1, 0], [1, 0, 0], [1, 0, 1], [0, 1, 1], [0, 1, 0]]} { return }
// Test 3: rank 3 in the full loop space ...
// CHECK: @test3_six: rank 3, kernel []
func.func @test3_six() attributes {lattice.matrix = [[1, 0, 0], [0, 1, 0], [1, 0, 0], [0, 1, 1], [1, 0, 1], [0, 1, 0]]} { return }
// ... but rank 1 on the role space (i and j are batch loops): one dot product per batch.
// CHECK: @test3_role_space: rank 1, kernel []
func.func @test3_role_space() attributes {lattice.matrix = [[0], [0], [0], [1], [1], [0]]} { return }
// A split coordinate C[8 i0 + i1][j]: the kernel contains (1, -8, 0, 0), which joins no two points of a box with i1 in [0, 8).
// CHECK: @split_c: rank 2, kernel {{\[\[}}1, -8, 0, 0], [0, 0, 0, 1]]
func.func @split_c() attributes {lattice.matrix = [[8, 1, 0, 0], [0, 0, 1, 0]]} { return }
// The zero matrix: the identity basis.
// CHECK: @zero: rank 0, kernel {{\[\[}}1, 0, 0], [0, 1, 0], [0, 0, 1]]
func.func @zero() attributes {lattice.matrix = [[0, 0, 0], [0, 0, 0]]} { return }
// Rank 1, |det| 0.
// CHECK: @singular: rank 1, kernel {{\[\[}}3, -2]], |det| 0
func.func @singular() attributes {lattice.matrix = [[4, 6], [6, 9]]} { return }
// CHECK: @unimodular: rank 2, kernel [], |det| 1
func.func @unimodular() attributes {lattice.matrix = [[2, 1], [1, 1]]} { return }
// INT64_MIN has no magnitude: refused, not wrapped.
// CHECK: @overflow: overflow (or ragged rows)
func.func @overflow() attributes {lattice.matrix = [[-9223372036854775808, 1]]} { return }
// Large entries that still fit.
// CHECK: @large: rank 1, kernel {{\[\[}}9223372036854775806, -9223372036854775807]]
func.func @large() attributes {lattice.matrix = [[9223372036854775807, 9223372036854775806]]} { return }
// Common direction of the restrictions of several subscripts to one role's loops.
// CHECK: @forms_split: direction [8, 1], multipliers [1, 2, 0]
func.func @forms_split() attributes {lattice.forms = [[8, 1], [16, 2], [0, 0]]} { return }
// SQ-1: A[4 i0 + i1] with C[8 i0 + i1] are not proportional.
// CHECK: @forms_sq1: no common direction
func.func @forms_sq1() attributes {lattice.forms = [[8, 1], [4, 1]]} { return }
// CHECK: @forms_negative: direction [1, 2], multipliers [-2, 1]
func.func @forms_negative() attributes {lattice.forms = [[-2, -4], [1, 2]]} { return }
// CHECK: @forms_zero: no common direction
func.func @forms_zero() attributes {lattice.forms = [[0, 0]]} { return }
// Theorem 3 (S2).
// CHECK: @radix_split: mixed radix, extent 32
func.func @radix_split() attributes {lattice.radix = [8, 1], lattice.extents = [4, 8]} { return }
// K = 3 k0 + 2 k1: injective but not dense.
// CHECK: @radix_gap: not a mixed radix
func.func @radix_gap() attributes {lattice.radix = [3, 2], lattice.extents = [2, 3]} { return }
// CHECK: @radix_backwards: mixed radix, extent 4
func.func @radix_backwards() attributes {lattice.radix = [-1], lattice.extents = [4]} { return }
// CHECK: @radix_order: mixed radix, extent 12
func.func @radix_order() attributes {lattice.radix = [1, 4], lattice.extents = [4, 3]} { return }
// CHECK: @radix_repeat: not a mixed radix
func.func @radix_repeat() attributes {lattice.radix = [1, 1], lattice.extents = [2, 2]} { return }
// C[4 i + j] with N = 4: injective ...
// CHECK: @inject_flat: injective
func.func @inject_flat() attributes {lattice.inject = [[4], [1]], lattice.box = [4, 4]} { return }
// ... with N = 5: (0, 4) and (1, 0) both write C[4].
// CHECK: @inject_flat_wide: collision (1, -4)
func.func @inject_flat_wide() attributes {lattice.inject = [[4], [1]], lattice.box = [4, 5]} { return }
// C[i + j]: not injective.
// CHECK: @inject_sum: collision (1, -1)
func.func @inject_sum() attributes {lattice.inject = [[1], [1]], lattice.box = [4, 4]} { return }
// CHECK: @inject_plain: injective
func.func @inject_plain() attributes {lattice.inject = [[1, 0], [0, 1]], lattice.box = [4, 4]} { return }
// C[i + j][j]: injective.
// CHECK: @inject_skew: injective
func.func @inject_skew() attributes {lattice.inject = [[1, 0], [1, 1]], lattice.box = [4, 4]} { return }
// A rank-1 view (lamI = 2u, lamJ = u).
// CHECK: @inject_rank1: collision (1, -2)
func.func @inject_rank1() attributes {lattice.inject = [[2, 4], [1, 2]], lattice.box = [4, 4]} { return }
// CHECK: @inject_ignored_ok: injective
func.func @inject_ignored_ok() attributes {lattice.inject = [[0], [1]], lattice.box = [1, 5]} { return }
// A coordinate the map ignores collides along itself.
// CHECK: @inject_ignored: collision (1, 0)
func.func @inject_ignored() attributes {lattice.inject = [[0], [1]], lattice.box = [2, 5]} { return }
