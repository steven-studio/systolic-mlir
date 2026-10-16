"""
showcase_cases.py -- the fixed showcase set: 80 positive cases (case_001 ..
case_080), 20 negative cases (case_081 .. case_100), and a few extended
cases (ext_001 ..) that document further forms, outside the hundred.

Every case is a complete, readable specification: the loop nest (bounds,
steps, comparison, roles), the arrays (dimensions, subscripts) and the
statement.  generator/make_showcase.py turns each into one self-contained C
program with its independent references, and into its manifest entry.

Conventions used below:
  L(var, lb, ub, role, step=1, cmp='<')   a loop; ub is exclusive for '<'
  arr(name, dims, subs)                   an array parameter and its subscripts
  gemm(...)                               a case whose a/b/c operands are A/B/C
"""

from casespec import Array, Case, Loop


def L(var, lb, ub, role, step=1, cmp="<", ctype="int", inc="++", cond=""):
    return Loop(var=var, lb=lb, ub=ub, step=step, cmp=cmp, role=role, ctype=ctype, inc=inc, cond=cond)


def arr(name, dims, subs, restrict=True, elem="float"):
    return Array(name=name, dims=tuple(dims), subs=tuple(subs), restrict=restrict, elem=elem)


def ijk(M, N, K, order="ijk", lb=(0, 0, 0)):
    roles = {"i": "I", "j": "J", "k": "K"}
    ext = {"i": M, "j": N, "k": K}
    lbs = {"i": lb[0], "j": lb[1], "k": lb[2]}
    return [L(v, lbs[v], lbs[v] + ext[v], roles[v]) for v in order]


def gemm(num, name, category, loops, A, B, C, set="positive", **kw):
    cid = "case_%03d" % num
    return Case(id=cid, name=name, set=set, category=category, loops=loops,
                arrays={"a": A, "b": B, "c": C}, **kw)


def ext_case(num, name, category, loops, arrays, **kw):
    return Case(id="ext_%03d" % num, name=name, set="extended", category=category,
                loops=loops, arrays=arrays, **kw)


def std_arrays(M, N, K, A="A", B="B", C="C"):
    """A[M][K], B[K][N], C[M][N] with the canonical subscripts."""
    return (arr(A, (M, K), ("i", "k")), arr(B, (K, N), ("k", "j")), arr(C, (M, N), ("i", "j")))


# ==========================================================================
# POSITIVE: expected to be recognized, partitioned, lowered and offloaded
# ==========================================================================

POSITIVE = []


def P(*a, **k):
    POSITIVE.append(gemm(*a, **k))


# --- G1: the basic GEMM, every loop order, every spelling of the statement
A8, B8, C8 = std_arrays(8, 8, 8)
P(1, "basic_ijk", "basic_gemm", ijk(8, 8, 8, "ijk"), *std_arrays(8, 8, 8),
  notes="The reference form: loops i, j, k; C[i][j] += A[i][k] * B[k][j].")
P(2, "order_ikj", "loop_order", ijk(8, 8, 8, "ikj"), *std_arrays(8, 8, 8))
P(3, "order_jik", "loop_order", ijk(8, 8, 8, "jik"), *std_arrays(8, 8, 8))
P(4, "order_jki", "loop_order", ijk(8, 8, 8, "jki"), *std_arrays(8, 8, 8))
P(5, "order_kij", "loop_order", ijk(8, 8, 8, "kij"), *std_arrays(8, 8, 8))
P(6, "order_kji", "loop_order", ijk(8, 8, 8, "kji"), *std_arrays(8, 8, 8),
  notes="The reduction loop outermost: every C element is read and written K times\n"
        "in an interleaved order; the accumulation order per element is unchanged.")
P(7, "stmt_c_plus_ab", "statement_form", ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="=C+AB",
  notes="C[i][j] = C[i][j] + A[i][k] * B[k][j]: the same arith.addf(C, mulf) as +=.")
P(8, "stmt_ab_plus_c", "statement_form", ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="=AB+C",
  notes="C[i][j] = A[i][k] * B[k][j] + C[i][j]: arith.addf(mulf, C); addf is commutative\n"
        "and the recognizer accepts the accumulator on either side.")
P(9, "stmt_b_times_a", "statement_form", ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="+=BA",
  notes="C[i][j] += B[k][j] * A[i][k]: the product's operands swapped; the recognizer\n"
        "tries both assignments of the two loads to logical A and B.")
P(10, "bounds_le", "loop_syntax",
  [L("i", 0, 7, "I", cmp="<="), L("j", 0, 7, "J", cmp="<="), L("k", 0, 7, "K", cmp="<=")],
  *std_arrays(8, 8, 8), notes="Inclusive bounds: i <= 7 is affine.for 0 to 8.")
P(11, "long_counters_i_eq_i_plus_1", "loop_syntax",
  [L("i", 0, 8, "I", ctype="long", inc="=+"), L("j", 0, 8, "J", ctype="long", inc="=+"),
   L("k", 0, 8, "K", ctype="long", inc="=+")], *std_arrays(8, 8, 8),
  notes="long counters, incremented as i = i + 1.")
P(12, "post_increment", "loop_syntax",
  [L("i", 0, 8, "I", inc="post++"), L("j", 0, 8, "J", inc="post++"), L("k", 0, 8, "K", inc="post++")],
  *std_arrays(8, 8, 8))

# --- G2: shapes
P(13, "shape_1x1x1", "shape", ijk(1, 1, 1), *std_arrays(1, 1, 1),
  notes="The smallest GEMM: one product.")
P(14, "shape_m1_1x8x8", "shape", ijk(1, 8, 8), *std_arrays(1, 8, 8),
  notes="M = 1: a row vector times a matrix, still a three-loop GEMM with rank-2 arrays.")
P(15, "shape_n1_8x1x8", "shape", ijk(8, 1, 8), *std_arrays(8, 1, 8),
  notes="N = 1: a matrix times a column vector held in a [8][1] array (rank 2).")
P(16, "shape_k1_8x8x1", "shape", ijk(8, 8, 1), *std_arrays(8, 8, 1),
  notes="K = 1: an outer product written as a GEMM.")
P(17, "shape_2x3x4", "shape", ijk(2, 3, 4), *std_arrays(2, 3, 4))
P(18, "shape_5x7x3", "shape", ijk(5, 7, 3), *std_arrays(5, 7, 3),
  notes="Odd extents: edge tiles on every axis.")
P(19, "shape_16x16x16", "shape", ijk(16, 16, 16), *std_arrays(16, 16, 16),
  notes="The fleet splits this into 8x8 and 4x4 tiles (10 invocations in test/CFront/gemm.c).")
P(20, "shape_20x12x40_kfold", "shape", ijk(20, 12, 40), *std_arrays(20, 12, 40),
  notes="K = 40 > k_max = 16: K is folded over invocations chained through c_in;\n"
        "M = 20 and N = 12 give edge tiles (27 invocations in test/CFront/gemm.c).")
P(21, "shape_9x17x33", "shape", ijk(9, 17, 33), *std_arrays(9, 17, 33))
P(22, "shape_8x8x17_kmax_plus_1", "shape", ijk(8, 8, 17), *std_arrays(8, 8, 17),
  notes="K = k_max + 1: two invocations per output tile, the second of depth 1.")
P(23, "shape_4x4x16_exact_tile", "shape", ijk(4, 4, 16), *std_arrays(4, 4, 16),
  notes="Exactly one 4x4 tile of depth k_max.")
P(24, "shape_12x20x8", "shape", ijk(12, 20, 8), *std_arrays(12, 20, 8))

# --- G3: non-zero loop origins
P(25, "origin_i_from_2", "loop_origin", ijk(8, 8, 8, lb=(2, 0, 0)),
  arr("A", (10, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (10, 8), ("i", "j")),
  notes="i runs 2..9: the logical row I = i has offset 2 (rows 2..9 of A and C).")
P(26, "origin_j_from_3", "loop_origin", ijk(8, 8, 8, lb=(0, 3, 0)),
  arr("A", (8, 8), ("i", "k")), arr("B", (8, 11), ("k", "j")), arr("C", (8, 11), ("i", "j")))
P(27, "origin_k_from_1", "loop_origin", ijk(8, 8, 8, lb=(0, 0, 1)),
  arr("A", (8, 9), ("i", "k")), arr("B", (9, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")))
P(28, "origin_all_nonzero", "loop_origin", ijk(8, 8, 8, lb=(2, 3, 1)),
  arr("A", (10, 9), ("i", "k")), arr("B", (9, 11), ("k", "j")), arr("C", (10, 11), ("i", "j")))
P(29, "origin_4_minus_4", "loop_origin", ijk(8, 8, 8, lb=(4, 0, 0)),
  arr("A", (8, 8), ("i - 4", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i - 4", "j")),
  notes="i runs 4..11 and the subscript is i - 4: the negative constant and the\n"
        "origin cancel, the logical row offset is 0 (T x0 + c = 4 - 4).")
P(30, "origin_1_plus_1", "loop_origin", ijk(8, 8, 8, lb=(1, 0, 0)),
  arr("A", (10, 8), ("i + 1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (10, 8), ("i + 1", "j")),
  notes="i runs 1..8 and the subscript is i + 1: rows 2..9.")

# --- G4: loop steps
P(31, "step_i_2", "loop_step", [L("i", 0, 16, "I", step=2), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (16, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("i", "j")),
  notes="i += 2 visits the even rows: the logical row I = i has stride 2 in memory.")
P(32, "step_k_3", "loop_step", [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 24, "K", step=3)],
  arr("A", (8, 24), ("i", "k")), arr("B", (24, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
  notes="k = 0, 3, ..., 21: eight reduction steps over every third column of A / row of B.")
P(33, "step_i_2_j_2", "loop_step", [L("i", 0, 16, "I", step=2), L("j", 0, 16, "J", step=2), L("k", 0, 8, "K")],
  arr("A", (16, 8), ("i", "k")), arr("B", (8, 16), ("k", "j")), arr("C", (16, 16), ("i", "j")))
P(34, "step_2_origin_1", "loop_step", [L("i", 1, 17, "I", step=2), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (17, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (17, 8), ("i", "j")),
  notes="i = 1, 3, ..., 15: offset 1, stride 2.")
P(35, "step_2_le_bound", "loop_step", [L("i", 0, 14, "I", step=2, cmp="<="), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (16, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("i", "j")),
  notes="i <= 14 with i += 2: affine.for 0 to 15 step 2, eight iterations.")

# --- G5: access offsets (the same expression in both operands that share a coordinate)
P(36, "offset_i_plus_2", "access_offset", ijk(8, 8, 8),
  arr("A", (10, 8), ("i + 2", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (10, 8), ("i + 2", "j")))
P(37, "offset_j_plus_3", "access_offset", ijk(8, 8, 8),
  arr("A", (8, 8), ("i", "k")), arr("B", (8, 11), ("k", "j + 3")), arr("C", (8, 11), ("i", "j + 3")))
P(38, "offset_k_plus_1", "access_offset", ijk(8, 8, 8),
  arr("A", (8, 9), ("i", "k + 1")), arr("B", (9, 8), ("k + 1", "j")), arr("C", (8, 8), ("i", "j")))
P(39, "offset_all", "access_offset", ijk(8, 8, 8),
  arr("A", (10, 9), ("i + 2", "k + 1")), arr("B", (9, 12), ("k + 1", "j + 4")), arr("C", (10, 12), ("i + 2", "j + 4")),
  notes="The user's seed A[i+2][k+1], B[k+3][j+2], C[i+1][j+4] made consistent: a\n"
        "coordinate shared by two operands must be the same expression in both for\n"
        "the current recognizer (see case_094 / ext_008 for the other kind).")
P(40, "offset_constant_first", "access_offset", ijk(8, 8, 8),
  arr("A", (10, 8), ("2 + i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (10, 8), ("2 + i", "j")),
  notes="The constant written first: 2 + i.")

# --- G6: access strides
P(41, "stride_i_2", "access_stride", ijk(8, 8, 8),
  arr("A", (16, 8), ("2 * i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("2 * i", "j")),
  notes="A[2*i][k], C[2*i][j]: the even rows (the user's seed).")
P(42, "stride_j_3", "access_stride", ijk(8, 8, 8),
  arr("A", (8, 8), ("i", "k")), arr("B", (8, 22), ("k", "3 * j")), arr("C", (8, 22), ("i", "3 * j")),
  notes="B[k][3*j], C[i][3*j]: every third column (the user's seed).")
P(43, "stride_k_2", "access_stride", ijk(8, 8, 8),
  arr("A", (8, 16), ("i", "2 * k")), arr("B", (16, 8), ("2 * k", "j")), arr("C", (8, 8), ("i", "j")),
  notes="A[i][2*k], B[2*k][j]: the reduction over the even columns / rows (the user's seed).")
P(44, "stride_all", "access_stride", ijk(8, 8, 8),
  arr("A", (16, 16), ("2 * i", "2 * k")), arr("B", (16, 22), ("2 * k", "3 * j")), arr("C", (16, 22), ("2 * i", "3 * j")))
P(45, "stride_and_offset_seed", "access_stride", ijk(8, 8, 8),
  arr("A", (18, 9), ("2 * i + 3", "k + 1")), arr("B", (9, 24), ("k + 1", "3 * j + 2")), arr("C", (18, 24), ("2 * i + 3", "3 * j + 2")),
  notes="The user's seed A[2*i+3][k+1], B[k+2][3*j+2], C[2*i+j][j] made a GEMM the\n"
        "current recognizer handles: I = 2i + 3 on A and C, J = 3j + 2 on B and C,\n"
        "K = k + 1 on A and B.  The seed as written is case_088.")
P(46, "stride_i_times_2_plus_1", "access_stride", ijk(8, 8, 8),
  arr("A", (16, 8), ("i * 2 + 1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("i * 2 + 1", "j")),
  notes="The coefficient written after the counter: i * 2 + 1 (odd rows).")
P(47, "stride_k_3_plus_2", "access_stride", [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 6, "K")],
  arr("A", (8, 18), ("i", "3 * k + 2")), arr("B", (18, 8), ("3 * k + 2", "j")), arr("C", (8, 8), ("i", "j")))

# --- G7: coordinates split over several loops (tiled nests)
P(48, "split_k_8", "split_coordinate",
  [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k0", 0, 2, "K"), L("k1", 0, 8, "K")],
  arr("A", (8, 16), ("i", "8 * k0 + k1")), arr("B", (16, 8), ("8 * k0 + k1", "j")), arr("C", (8, 8), ("i", "j")),
  notes="K = 8 k0 + k1 over two loops: a mixed-radix (dense) enumeration of 0..15.")
P(49, "split_i_j_tiled", "split_coordinate",
  [L("i0", 0, 2, "I"), L("j0", 0, 2, "J"), L("i1", 0, 4, "I"), L("j1", 0, 4, "J"), L("k", 0, 8, "K")],
  arr("A", (8, 8), ("4 * i0 + i1", "k")), arr("B", (8, 8), ("k", "4 * j0 + j1")), arr("C", (8, 8), ("4 * i0 + i1", "4 * j0 + j1")),
  notes="I and J tiled by 4: five loops, I on loops 0 and 2, J on loops 1 and 3.")
P(50, "split_all_tiled_16x16x24", "split_coordinate",
  [L("i0", 0, 2, "I"), L("j0", 0, 4, "J"), L("k0", 0, 3, "K"), L("i1", 0, 8, "I"), L("j1", 0, 4, "J"), L("k1", 0, 8, "K")],
  arr("A", (16, 24), ("8 * i0 + i1", "8 * k0 + k1")), arr("B", (24, 16), ("8 * k0 + k1", "4 * j0 + j1")),
  arr("C", (16, 16), ("8 * i0 + i1", "4 * j0 + j1")),
  notes="I, J and K all tiled (test/Systolic/affine_gemm_tiled.mlir in C): six loops.")
P(51, "split_interleaved_i0_k0_j0", "split_coordinate",
  [L("i0", 0, 2, "I"), L("k0", 0, 2, "K"), L("j0", 0, 2, "J"), L("i1", 0, 4, "I"), L("j1", 0, 4, "J"), L("k1", 0, 4, "K")],
  arr("A", (8, 8), ("4 * i0 + i1", "4 * k0 + k1")), arr("B", (8, 8), ("4 * k0 + k1", "4 * j0 + j1")),
  arr("C", (8, 8), ("4 * i0 + i1", "4 * j0 + j1")),
  notes="The tile loops in the order i0, k0, j0: a reduction tile loop outside the\n"
        "output tile loop j0.")
P(52, "split_with_offsets", "split_coordinate",
  [L("i0", 0, 2, "I"), L("j0", 0, 2, "J"), L("i1", 0, 4, "I"), L("j1", 0, 4, "J"), L("k", 0, 8, "K")],
  arr("A", (10, 8), ("4 * i0 + i1 + 2", "k")), arr("B", (8, 9), ("k", "4 * j0 + j1 + 1")),
  arr("C", (10, 9), ("4 * i0 + i1 + 2", "4 * j0 + j1 + 1")),
  notes="Tiled coordinates with constant offsets: rows 2..9, columns 1..8.")
P(53, "split_with_stride", "split_coordinate",
  [L("i0", 0, 2, "I"), L("i1", 0, 4, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (16, 8), ("8 * i0 + 2 * i1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("8 * i0 + 2 * i1", "j")),
  notes="I = 8 i0 + 2 i1: the even rows 0, 2, ..., 14, enumerated by two loops with\n"
        "scales 2 and 8 (dense: 8 = 2 * 4).")
P(54, "split_k_three_levels", "split_coordinate",
  [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k0", 0, 2, "K"), L("k1", 0, 4, "K"), L("k2", 0, 4, "K")],
  arr("A", (8, 32), ("i", "16 * k0 + 4 * k1 + k2")), arr("B", (32, 8), ("16 * k0 + 4 * k1 + k2", "j")), arr("C", (8, 8), ("i", "j")),
  notes="K = 16 k0 + 4 k1 + k2 over three loops (extents 2, 4, 4): K = 32, two folds.")
P(55, "split_radix_3", "split_coordinate",
  [L("i0", 0, 3, "I"), L("i1", 0, 3, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (9, 8), ("3 * i0 + i1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (9, 8), ("3 * i0 + i1", "j")),
  notes="A radix that is not a power of two: I = 3 i0 + i1, M = 9.")
P(56, "split_fine_loop_outside", "split_coordinate",
  [L("i1", 0, 4, "I"), L("j", 0, 8, "J"), L("i0", 0, 2, "I"), L("k", 0, 8, "K")],
  arr("A", (8, 8), ("4 * i0 + i1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("4 * i0 + i1", "j")),
  notes="The fine loop i1 is outside the coarse loop i0 (and j is between them):\n"
        "I = 4 i0 + i1 is still a dense enumeration; the order of the loops is free.")

# --- G8: repetition loops (legal: they stay outside, or are hoisted outward)
P(57, "repeat_outer_2", "repetition_loop",
  [L("r", 0, 2, "R"), L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
  notes="An outer loop no subscript uses runs the GEMM twice: C += 2 A B.  The nest\n"
        "anchored at r is not a GEMM (r is not a dimension); the one anchored at i is.")
P(58, "repeat_between_i_j", "repetition_loop",
  [L("i", 0, 8, "I"), L("r", 0, 3, "R"), L("j", 0, 8, "J"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
  notes="A repetition loop between i and j: --affine-matmul-to-systolic moves it\n"
        "outward (a legal interchange: it is outside the reduction) before recognizing.")
P(59, "repeat_between_j_k", "repetition_loop",
  [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("r", 0, 2, "R"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
  notes="A repetition loop between j and k (test/SystolicCC/depth.c d4_mid): hoisted.")
P(60, "repeat_two_loops", "repetition_loop",
  [L("r0", 0, 2, "R"), L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("r1", 0, 2, "R"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
  notes="Two repetition loops: one outside, one between j and k (hoisted): 4 executions.")
P(61, "repeat_with_tiled_nest", "repetition_loop",
  [L("r0", 0, 2, "R"), L("i0", 0, 2, "I"), L("j", 0, 8, "J"), L("r1", 0, 3, "R"), L("i1", 0, 4, "I"),
   L("k0", 0, 2, "K"), L("k1", 0, 8, "K")],
  arr("A", (8, 16), ("4 * i0 + i1", "8 * k0 + k1")), arr("B", (16, 8), ("8 * k0 + k1", "j")),
  arr("C", (8, 8), ("4 * i0 + i1", "j")),
  notes="Seven loops (test/SystolicCC/depth.c d7): r0 outside, r1 between j and i1,\n"
        "I = 4 i0 + i1, K = 8 k0 + k1; six executions of the 8x8x16 GEMM.")

# --- G9: the GEMM inside larger arrays
P(62, "subblock_top_left", "sub_matrix", ijk(8, 8, 8),
  arr("A", (16, 16), ("i", "k")), arr("B", (16, 16), ("k", "j")), arr("C", (16, 16), ("i", "j")),
  notes="The 8x8x8 GEMM on the top-left blocks of 16x16 arrays.")
P(63, "subblock_offsets", "sub_matrix", ijk(8, 8, 8),
  arr("A", (16, 16), ("i + 4", "k + 2")), arr("B", (16, 16), ("k + 2", "j + 2")), arr("C", (16, 16), ("i + 4", "j + 2")),
  notes="Interior blocks: A rows 4..11 x cols 2..9, B rows 2..9 x cols 2..9, C rows 4..11 x cols 2..9.")
P(64, "subblock_strided", "sub_matrix", ijk(8, 8, 8),
  arr("A", (16, 16), ("2 * i", "2 * k")), arr("B", (16, 16), ("2 * k", "2 * j")), arr("C", (16, 16), ("2 * i", "2 * j")),
  notes="Every other row and column of 16x16 arrays.")
P(65, "subblock_c_larger", "sub_matrix", ijk(8, 8, 8),
  arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (32, 32), ("i + 8", "j + 16")),
  notes="C is 32x32; the 8x8 product A B is accumulated into rows 8..15, columns\n"
        "16..23 of C.  A and B are indexed plainly: the constant offsets are on C only.\n"
        "This is a valid GEMM (C[8:16, 16:24] += A B); its row coordinate is i for A\n"
        "and i + 8 for C -- the same logical coordinate, two different index\n"
        "expressions.  The recognizer at 6e25528 requires the SAME affine expression\n"
        "in both operands and rejects it: a FALSE NEGATIVE, recorded as ISSUE-001\n"
        "(failures/issues/).  The expectation stays ACCEPT: it is set by the\n"
        "mathematics and the support specification, not by the current output.",
  issue="ISSUE-001")

# --- G10: one array used twice, small odd cases, syntax variants
P(66, "a_times_a", "shared_operand", ijk(8, 8, 8),
  arr("A", (8, 8), ("i", "k")), arr("A", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
  notes="C += A A: the same array is both operands, through one read-only view.")
P(67, "a_times_a_kij", "shared_operand", ijk(8, 8, 8, "kij"),
  arr("A", (8, 8), ("i", "k")), arr("A", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")))
P(68, "small_jki_offsets_3x3x3", "shape", ijk(3, 3, 3, "jki"),
  arr("A", (4, 3), ("i + 1", "k")), arr("B", (3, 4), ("k", "j + 1")), arr("C", (4, 4), ("i + 1", "j + 1")))
P(69, "braces_bodies", "loop_syntax", ijk(8, 8, 8), *std_arrays(8, 8, 8), kernel_style="braces",
  notes="Every loop body in braces: the same affine.for chain.")
P(70, "pointer_to_array_params", "parameter_syntax", ijk(8, 8, 8), *std_arrays(8, 8, 8), kernel_style="ptr_param",
  notes="float (*restrict A)[8]: the type C adjusts float A[restrict 8][8] to, written out.")
P(71, "counter_names_row_col_depth", "loop_syntax",
  [L("row", 0, 6, "I"), L("col", 0, 5, "J"), L("depth", 0, 7, "K")],
  arr("A", (6, 7), ("row", "depth")), arr("B", (7, 5), ("depth", "col")), arr("C", (6, 5), ("row", "col")))
P(72, "le_bound_step_2_origin_2", "loop_step",
  [L("i", 2, 16, "I", step=2, cmp="<="), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
  arr("A", (17, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (17, 8), ("i", "j")),
  notes="i = 2, 4, ..., 16 (i <= 16; i += 2): eight rows, offset 2, stride 2.")
P(73, "split_k_outer_and_inner", "split_coordinate",
  [L("k0", 0, 2, "K"), L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k1", 0, 8, "K")],
  arr("A", (8, 16), ("i", "8 * k0 + k1")), arr("B", (16, 8), ("8 * k0 + k1", "j")), arr("C", (8, 8), ("i", "j")),
  notes="The coarse reduction loop outermost, the fine one innermost, I and J between.")
P(74, "shape_32x32x32", "shape", ijk(32, 32, 32), *std_arrays(32, 32, 32),
  notes="32x32x32: the largest fixed case; two K folds per output tile.")
P(75, "shape_17x9x33", "shape", ijk(17, 9, 33), *std_arrays(17, 9, 33))
P(76, "shape_8x8x64_four_folds", "shape", ijk(8, 8, 64), *std_arrays(8, 8, 64),
  notes="K = 64 = 4 k_max: four chained invocations per output tile.")
P(77, "shape_8x8x5", "shape", ijk(8, 8, 5), *std_arrays(8, 8, 5))
P(78, "increment_plus_equals_1", "loop_syntax",
  [L("i", 0, 8, "I", inc="+="), L("j", 0, 8, "J", inc="+="), L("k", 0, 8, "K", inc="+=")], *std_arrays(8, 8, 8),
  notes="i += 1 as the increment.")
P(79, "kji_origins_offsets_strides", "combination",
  [L("k", 1, 9, "K"), L("j", 2, 10, "J"), L("i", 0, 8, "I")],
  arr("A", (16, 11), ("2 * i + 1", "k + 2")), arr("B", (11, 28), ("k + 2", "3 * j")), arr("C", (16, 28), ("2 * i + 1", "3 * j")),
  notes="Loops k, j, i with origins 1 and 2; I = 2i + 1 (odd rows), J = 3j (j from 2:\n"
        "columns 6, 9, ..., 27), K = k + 2 (k from 1: 3..10).")
P(80, "full_combination", "combination",
  [L("r", 0, 2, "R"), L("k0", 0, 2, "K"), L("j", 0, 8, "J"), L("i", 0, 8, "I"), L("k1", 0, 8, "K")],
  arr("A", (9, 16), ("i + 1", "8 * k0 + k1")), arr("B", (16, 10), ("8 * k0 + k1", "j + 2")), arr("C", (9, 10), ("i + 1", "j + 2")),
  notes="A repetition loop outside, the reduction split with its coarse loop\n"
        "outermost, j before i, offsets on I and J: everything at once.")

assert [c.id for c in POSITIVE] == ["case_%03d" % n for n in range(1, 81)]

# ==========================================================================
# NEGATIVE: expected NOT to be accepted by the current compiler -- each
# header says whether it is nevertheless a mathematically valid GEMM
# ==========================================================================

NEGATIVE = []


def Ng(*a, **k):
    k.setdefault("set", "negative")
    NEGATIVE.append(gemm(*a, **k))


Ng(81, "elementwise_2d_repeated", "elementwise",
   [L("i", 0, 8, "X"), L("j", 0, 8, "X"), L("k", 0, 4, "R")],
   arr("A", (8, 8), ("i", "j")), arr("B", (8, 8), ("i", "j")), arr("C", (8, 8), ("i", "j")),
   math_class="elementwise_product",
   math_note="C[i][j] += A[i][j] * B[i][j] repeated 4 times: the Hadamard (element-wise)\n"
             "product, C += 4 (A o B).  There is no reduction coordinate -- k is used by no\n"
             "subscript -- and no operand couples two coordinates.  Not a GEMM.\n"
             "Expected rejection: the only loops the accesses use are i and j; the three-\n"
             "deep candidate has k as a repetition loop, so the chain is not all GEMM\n"
             "dimensions (and after the lowering pass hoists k outward only two loops remain).")
Ng(82, "elementwise_3d", "elementwise",
   [L("i", 0, 4, "X"), L("j", 0, 4, "X"), L("k", 0, 4, "X")],
   arr("A", (4, 4, 4), ("i", "j", "k")), arr("B", (4, 4, 4), ("i", "j", "k")), arr("C", (4, 4, 4), ("i", "j", "k")),
   math_class="elementwise_product",
   math_note="C[i][j][k] += A[i][j][k] * B[i][j][k]: an element-wise product of rank-3\n"
             "arrays; every loop appears in every operand, nothing is reduced.  Not a GEMM.\n"
             "Expected rejection: with three subscripts the recognizer reads the leading\n"
             "one as a batch coordinate and needs A = (P, I, K), B = (P, K, J), C = (P, I, J)\n"
             "with I, J, K distinct; here I = J = K.")
Ng(83, "matrix_vector", "matrix_vector",
   [L("i", 0, 8, "I"), L("j", 0, 8, "K")],
   arr("M", (8, 8), ("i", "j")), arr("x", (8,), ("j",)), arr("y", (8,), ("i",)),
   math_class="matrix_vector",
   math_note="y[i] += M[i][j] * x[j]: a matrix-vector product (GEMV).  It is the N = 1\n"
             "special case of a GEMM (y and x as 8x1 matrices), written with two index\n"
             "dimensions and rank-1 operands.  Refused until 2026-10-15 (the recognizer\n"
             "looked for three nested loops and rank-2 operands); offloaded since ISSUE-005\n"
             "(P0: x and y get a unit column, memref.expand_shape).",
   issue="ISSUE-005")
Ng(84, "matrix_vector_repeated_4", "matrix_vector",
   [L("r", 0, 2, "R"), L("s", 0, 2, "R"), L("i", 0, 8, "I"), L("j", 0, 8, "K")],
   arr("M", (8, 8), ("i", "j")), arr("x", (8,), ("j",)), arr("y", (8,), ("i",)),
   math_class="matrix_vector",
   math_note="Four loops around a matrix-vector product (test/SystolicCC/depth.c\n"
             "ng_matvec4): y += 4 M x, r and s repeat it.  The GEMV is the N = 1 GEMM\n"
             "(ISSUE-005, P0) and r, s are repetition loops outside it: offloaded since\n"
             "2026-10-15, its tiles run four times per call (refused before: two loop\n"
             "counters).",
   issue="ISSUE-005")
Ng(85, "batched_gemm", "batched",
   [L("b", 0, 2, "P"), L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
   arr("A", (2, 4, 4), ("b", "i", "k")), arr("B", (2, 4, 4), ("b", "k", "j")), arr("C", (2, 4, 4), ("b", "i", "j")),
   math_note="A batched GEMM: for each b, C[b] += A[b] B[b] (two 4x4x4 GEMMs).  This IS a\n"
             "valid GEMM (a partition coordinate P = b in front of I, J, K).  A documented\n"
             "limitation until 2026-10-15; offloaded since ISSUE-006: the batch loop is\n"
             "re-emitted around the tiles, two invocations per call.",
   issue="ISSUE-006")
Ng(86, "batched_shared_b", "batched",
   [L("b", 0, 2, "P"), L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
   arr("A", (2, 4, 4), ("b", "i", "k")), arr("B", (4, 4), ("k", "j")), arr("C", (2, 4, 4), ("b", "i", "j")),
   math_note="A batched GEMM whose B is shared by every batch: C[b] += A[b] B.  Valid GEMMs\n"
             "(or, stacking the batches, one 8x4x4 GEMM with A reshaped).  B carries no\n"
             "batch subscript: it would be broadcast over the batch (a stride-0 view), or A\n"
             "and C merged over (b, i) -- neither is lowered (the recognizer: b is read by\n"
             "A and C only, a second I coordinate).  Legal, unsupported (ISSUE-006 lowers\n"
             "batches every operand carries).")
Ng(87, "coupled_i_plus_j", "coupled_mapping",
   [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (15, 8), ("i + j", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (15, 8), ("i + j", "j")),
   math_class="gemm_nonrectangular_region",
   math_note="C[i+j][j] += A[i+j][k] * B[k][j].  With r = i + j every computed element is\n"
             "C[r][j] += sum_k A[r][k] B[k][j], an element of the GEMM A B -- but the set of\n"
             "(r, j) computed is the parallelogram {j <= r < j + 8}, not a rectangle, so it\n"
             "is not one rectangular GEMM C' += A' B' of strided views.  The recognizer\n"
             "accepts the nest (I = i + j, J = j, K = k) and getDenseLogicalGemm refuses\n"
             "it: 'two logical coordinates are driven by the same loop (loop 1 drives I and\n"
             "J)' -- no box of (I, J, K) is a set of whole iterations.")
Ng(88, "skewed_output_seed", "coupled_mapping",
   [L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
   arr("A", (10, 5), ("2 * i + 3", "k + 1")), arr("B", (6, 12), ("k + 2", "3 * j + 2")), arr("C", (10, 4), ("2 * i + j", "j")),
   math_class="gemm_values_skewed_placement",
   math_note="The user's seed A[2*i+3][k+1], B[k+2][3*j+2], C[2*i+j][j], with 4x4x4 loops.\n"
             "P[i][j] = sum_k A[2i+3][k+1] B[k+2][3j+2] is a GEMM of two strided views\n"
             "(A rows 3,5,7,9 x cols 1..4; B rows 2..5 x cols 2,5,8,11) -- note the K views\n"
             "of A (k+1) and B (k+2) differ, which is fine mathematically.  Its value P[i][j]\n"
             "is accumulated into C[2i+j][j]: an injective but skewed placement, not a\n"
             "strided sub-matrix of C.  So the computed values are GEMM values; the output\n"
             "is not a GEMM operand view.  Expected rejection at recognition: the row\n"
             "expression of A (2i+3) is not the row expression of C (2i+j), and K differs\n"
             "between A and B.")
Ng(89, "singular_mapping", "singular_mapping",
   [L("x", 0, 4, "X"), L("y", 0, 4, "X"), L("z", 0, 4, "X")],
   arr("A", (7, 13), ("x + z", "x + y + 2 * z")), arr("B", (13, 7), ("x + y + 2 * z", "y + z")), arr("C", (7, 7), ("x + z", "y + z")),
   math_class="singular_mapping",
   math_note="test/Systolic/affine_gemm_singular_mapping.mlir in C.  The recognizer reads\n"
             "I = x + z, J = y + z, K = x + y + 2z = I + J: T = [[1 0 1],[0 1 1],[1 1 2]] is\n"
             "singular.  K is determined by (I, J), so for a fixed output element nothing\n"
             "is summed over an independent reduction index; instead several (x, y, z)\n"
             "map to the same (I, J) and the same product is added repeatedly.  Not a GEMM.\n"
             "Expected: recognized (the access pattern matches), refused by\n"
             "getDenseLogicalGemm: 'loop 0 drives I and K'.")
Ng(90, "singular_output_independent_of_i", "singular_mapping",
   [L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K"), L("p", 0, 4, "X")],
   arr("A", (10, 4), ("2 * j + k", "p")), arr("B", (4, 4), ("k", "j")), arr("C", (4, 4), ("i", "j")),
   math_class="singular_mapping",
   math_note="The user's seed A[2*j+k][p], B[k][j], C[i][j] with four 4-iteration loops.\n"
             "The summed value s[j] = sum_{k,p} A[2j+k][p] B[k][j] does not depend on i: every\n"
             "row of C receives the same vector (C += 1 s^T, a rank-1 update).  The output\n"
             "row coordinate i appears in no input, and A couples j and k.  Not a GEMM of A\n"
             "and B.  Expected rejection at recognition: A's row expression 2j+k is not C's\n"
             "row expression i.")
Ng(91, "transposed_a", "transposed_operand",
   ijk(8, 8, 8), arr("A", (8, 8), ("k", "i")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   math_note="C += A^T B (the user's seed A[k][i], B[k][j], C[i][j]): a valid GEMM whose\n"
             "left operand is read transposed.  A documented limitation until 2026-10-15;\n"
             "offloaded since ISSUE-005 (P0.2: A's view is permuted, memref.transpose).",
   issue="ISSUE-005")
Ng(92, "transposed_b", "transposed_operand",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("j", "k")), arr("C", (8, 8), ("i", "j")),
   math_note="C += A B^T (the user's seed A[i][k], B[j][k], C[i][j]): a valid GEMM whose\n"
             "right operand is read transposed (B[j][k] is (J, K)).  A documented\n"
             "limitation until 2026-10-15; offloaded since ISSUE-005 (P0.2).",
   issue="ISSUE-005")
Ng(93, "transposed_c", "transposed_operand",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("j", "i")),
   math_note="C^T += A B, i.e. C += B^T A^T: a valid GEMM written into the transposed\n"
             "output.  A documented limitation until 2026-10-15; offloaded since ISSUE-005\n"
             "(P0.2: the factor assignment that transposes least is chosen).",
   issue="ISSUE-005")
Ng(94, "operand_specific_stride", "operand_view_mismatch",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("2 * i", "j")),
   math_note="C[2i][j] += A[i][k] B[k][j] (the user's seed C[2*i][j]): the even rows of C\n"
             "receive A B.  A valid GEMM: C'(I,J) = C[2I][J] is a strided view, A'(I,K) =\n"
             "A[I][K] the plain one.  The recognizer requires the row expression of A and\n"
             "C to be the same affine expression (i vs 2i), so it is not recognized.\n"
             "Legal; a FALSE NEGATIVE of the same root cause as case_065 (ISSUE-001).",
   issue="ISSUE-001")
Ng(95, "reduction_index_in_output", "reduction_mismatch",
   [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "k")),
   math_class="not_gemm_scaled_rows",
   math_note="C[i][k] += A[i][k] * B[k][j]: the output is indexed by the would-be\n"
             "reduction index k and the loop j is summed away: C[i][k] += A[i][k] * (sum_j\n"
             "B[k][j]) -- A scaled column-wise by the row sums of B.  Not a GEMM.  Expected\n"
             "rejection at recognition: C's column expression k is not B's column j.")
Ng(96, "alpha_scaled", "scaled_product",
   ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="+=alpha", alpha=2.0,
   math_note="C += alpha A B with a scalar parameter alpha = 2: the BLAS GEMM with alpha\n"
             "!= 1, a valid GEMM.  The product is mulf(mulf(alpha, A), B): the recognizer\n"
             "needs mulf(load, load).  Legal, unsupported.")
Ng(97, "no_accumulation", "reduction_mismatch",
   ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="=AB",
   math_class="not_gemm_overwrite",
   math_note="C[i][j] = A[i][k] * B[k][j] overwrites C in every k iteration; the last one\n"
             "wins: C[i][j] = A[i][7] B[7][j], the outer product of A's last column and B's\n"
             "last row (a K = 1 GEMM of those two vectors, not the GEMM of A and B).\n"
             "Expected rejection at recognition: the stored value is a mulf, not an addf\n"
             "of the accumulator.")
Ng(98, "subtract_product", "scaled_product",
   ijk(8, 8, 8), *std_arrays(8, 8, 8), stmt="-=",
   math_note="C -= A B: the GEMM with alpha = -1, valid.  The front end emits arith.subf;\n"
             "the recognizer accepts arith.addf only.  Legal, unsupported.")
Ng(99, "init_inside_imperfect_nest", "imperfect_nest",
   ijk(8, 8, 8), *std_arrays(8, 8, 8), kernel_style="init_inside",
   math_note="for i, j { C[i][j] = 0; for k C[i][j] += A[i][k] B[k][j]; }: C = A B with\n"
             "beta = 0, the most common hand-written GEMM, valid.  The initialising store\n"
             "sits between the j and k loops, so the nest is not a perfect chain and the\n"
             "recognizer does not see a candidate (replacing the chain would drop the\n"
             "store).  Legal, unsupported.")
Ng(100, "double_precision", "element_type",
   ijk(4, 4, 4), arr("A", (4, 4), ("i", "k"), elem="double"), arr("B", (4, 4), ("k", "j"), elem="double"),
   arr("C", (4, 4), ("i", "j"), elem="double"), elem="double",
   math_note="A valid GEMM in double precision.  Recognized, dense, and the lowering pass\n"
             "itself would lower it; systolic-cc keeps it on the host because the FPGA\n"
             "datapath is fp32: 'the FPGA datapath is fp32 and it uses double arrays'.\n"
             "Legal, unsupported by the driver's policy.")

assert [c.id for c in NEGATIVE] == ["case_%03d" % n for n in range(81, 101)]

# ==========================================================================
# EXTENDED: further forms, outside the hundred (same tooling)
# ==========================================================================

EXTENDED = [
    ext_case(1, "scalar_accumulator", "frontend_subset", ijk(4, 4, 4),
             dict(zip("abc", std_arrays(4, 4, 4))), kernel_style="scalar_acc",
             math_note="C = A B through a scalar accumulator (float acc): a valid GEMM with\n"
                       "beta = 0; a local variable is outside the affine C subset, so the\n"
                       "front end refuses the function.  Legal, unsupported."),
    ext_case(2, "no_restrict_may_alias", "aliasing", ijk(8, 8, 8),
             {"a": arr("A", (8, 8), ("i", "k"), restrict=False), "b": arr("B", (8, 8), ("k", "j"), restrict=False),
              "c": arr("C", (8, 8), ("i", "j"), restrict=False)},
             math_note="The basic GEMM without restrict: recognized and dense, not lowered,\n"
                       "because A may overlap C (the arguments are not llvm.noalias)."),
    ext_case(3, "reversed_rows", "negative_coefficient", ijk(8, 8, 8),
             {"a": arr("A", (8, 8), ("7 - i", "k")), "b": arr("B", (8, 8), ("k", "j")),
              "c": arr("C", (8, 8), ("7 - i", "j"))},
             math_note="C[7-i][j] += A[7-i][k] B[k][j]: the GEMM over the rows in reverse\n"
                       "order -- the same mathematics as case_001 (a valid GEMM).  The\n"
                       "logical row I = 7 - i decreases along i; getDenseLogicalGemm reads it\n"
                       "backwards (rows 0..7 as a view) since ISSUE-004 (2026-10-15); before, it\n"
                       "refused negative scales."),
    ext_case(4, "repeat_inside_reduction", "repetition_loop",
             [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K"), L("r", 0, 2, "R")],
             dict(zip("abc", std_arrays(8, 8, 8))),
             math_note="A repetition loop inside the reduction (test/SystolicCC/depth.c\n"
                       "d4_illegal): C += 2 A B, a valid GEMM mathematically, but moving r\n"
                       "outward would interleave the repetitions with the reduction -- not\n"
                       "a legal interchange for floating point -- so the nest stays as it is\n"
                       "and is not recognized.  Legal, unsupported."),
    ext_case(5, "triple_product", "scaled_product", ijk(8, 8, 8),
             {"a": arr("A", (8, 8), ("i", "k")), "b": arr("B", (8, 8), ("k", "j")),
              "c": arr("C", (8, 8), ("i", "j")), "d": arr("D", (8, 8), ("k", "j"))},
             stmt="+=triple", math_class="gemm_of_hadamard_operand",
             math_note="C += sum_k A[i][k] (B[k][j] D[k][j]) = A (B o D): a GEMM of A and the\n"
                       "element-wise product B o D, not of the three arrays as given.  The\n"
                       "product has three factors; the recognizer needs mulf(load, load)."),
    ext_case(6, "sum_not_product", "not_gemm", ijk(8, 8, 8),
             dict(zip("abc", std_arrays(8, 8, 8))), stmt="+=sum", math_class="not_gemm_sum",
             math_note="C[i][j] += A[i][k] + B[k][j] over k: C[i][j] += (sum_k A[i][k]) +\n"
                       "(sum_k B[k][j]) -- the row sums of A plus the column sums of B; no\n"
                       "products at all.  Not a GEMM."),
    ext_case(7, "a_times_a_different_views", "shared_operand", ijk(8, 8, 8),
             {"a": arr("A", (8, 9), ("i", "k")), "b": arr("A", (8, 9), ("k", "j + 1")),
              "c": arr("C", (8, 8), ("i", "j"))},
             math_note="C += A[:, 0:8] A[:, 1:9]: both operands are views of the same array\n"
                       "but different ones.  A valid GEMM; the lowering refuses two different\n"
                       "restrict tensors of one buffer ('A and B are different views of the\n"
                       "same memref').  Legal, unsupported.  Before that check can even run,\n"
                       "the recognizer refuses the nest: B's column expression j + 1 is not\n"
                       "C's j (ISSUE-001's rule).", issue="ISSUE-001"),
    ext_case(8, "operand_specific_offset", "operand_view_mismatch", ijk(8, 8, 8),
             {"a": arr("A", (10, 8), ("i + 2", "k")), "b": arr("B", (8, 8), ("k", "j")),
              "c": arr("C", (8, 8), ("i", "j"))},
             math_note="C[i][j] += A[i+2][k] B[k][j]: C += A[2:10, :] B, a valid GEMM with an\n"
                       "offset view of A only.  The recognizer needs the same row expression\n"
                       "in A and C (i + 2 vs i).  Legal; a FALSE NEGATIVE of the same root\n"
                       "cause as case_065 (ISSUE-001).", issue="ISSUE-001"),
    ext_case(9, "equal_exprs_spelled_differently", "recognizer_probe", ijk(8, 8, 8),
             {"a": arr("A", (16, 8), ("i + i", "k")), "b": arr("B", (8, 8), ("k", "j")),
              "c": arr("C", (16, 8), ("2 * i", "j"))},
             math_note="A[i+i][k] and C[2*i][j]: the same affine function of i spelled two\n"
                       "ways.  Mathematically case_041.  Whether the recognizer sees them as\n"
                       "equal depends on MLIR's affine-expression canonicalisation; the\n"
                       "prediction here is 'accept' (canonical forms equal).  A probe."),
    ext_case(10, "mixed_double_a", "element_type", ijk(4, 4, 4),
             {"a": arr("A", (4, 4), ("i", "k"), elem="double"), "b": arr("B", (4, 4), ("k", "j")),
              "c": arr("C", (4, 4), ("i", "j"))},
             math_note="A is double, B and C float: C += (float)(A B) computed in double per\n"
                       "C's rules.  A valid GEMM mathematically (with conversions); the MAC\n"
                       "body contains extf/truncf, which the recognizer does not allow.\n"
                       "The exact small integers make the result the same as in float."),
    ext_case(11, "batched_two_batch_dims", "batched",
             [L("b0", 0, 2, "P"), L("b1", 0, 2, "P"), L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
             {"a": arr("A", (2, 2, 4, 4), ("b0", "b1", "i", "k")), "b": arr("B", (2, 2, 4, 4), ("b0", "b1", "k", "j")),
              "c": arr("C", (2, 2, 4, 4), ("b0", "b1", "i", "j"))},
             math_note="Two batch coordinates: four 4x4x4 GEMMs.  Valid; a documented\n"
                       "limitation until 2026-10-15, offloaded since ISSUE-006 (both batch\n"
                       "loops re-emitted around the tiles, four invocations per call).",
             issue="ISSUE-006"),
    ext_case(12, "eight_loops_tiled_repeated", "repetition_loop",
             [L("r0", 0, 2, "R"), L("i0", 0, 2, "I"), L("j0", 0, 2, "J"), L("r1", 0, 2, "R"), L("k0", 0, 2, "K"),
              L("i1", 0, 4, "I"), L("j1", 0, 4, "J"), L("k1", 0, 4, "K")],
             {"a": arr("A", (8, 8), ("4 * i0 + i1", "4 * k0 + k1")), "b": arr("B", (8, 8), ("4 * k0 + k1", "4 * j0 + j1")),
              "c": arr("C", (8, 8), ("4 * i0 + i1", "4 * j0 + j1"))},
             math_note="Eight loops (test/SystolicCC/depth.c d8): r0 outside, r1 between j0\n"
                       "and k0 (hoisted), I, J, K tiled; four executions.  Expected offloaded."),
    ext_case(13, "k_step_le_origin_offsets", "combination",
             [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 3, 17, "K", step=2, cmp="<=")],
             {"a": arr("A", (8, 20), ("i", "k + 1")), "b": arr("B", (20, 8), ("k + 1", "j")),
              "c": arr("C", (8, 8), ("i", "j"))},
             math_note="k = 3, 5, ..., 17 (k <= 17; k += 2) with K = k + 1: columns 4, 6, ..., 18\n"
                       "of A, eight reduction steps.  Expected offloaded."),
    ext_case(14, "stacked_batches_as_one_gemm", "split_coordinate",
             [L("b", 0, 2, "I"), L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
             {"a": arr("A", (8, 4), ("4 * b + i", "k")), "b": arr("B", (4, 4), ("k", "j")),
              "c": arr("C", (8, 4), ("4 * b + i", "j"))},
             math_note="case_086's batches stacked in rank-2 arrays: C[4b+i][j] += A[4b+i][k]\n"
                       "B[k][j] is one 8x4x4 GEMM with I = 4b + i split over two loops.\n"
                       "Expected offloaded -- the same mathematics as case_086, which is\n"
                       "refused only because of its rank-3 arrays."),
    ext_case(15, "batch_dependent_a_row", "batched",
             [L("i", 1, 5, "I"), L("j", 0, 4, "P"), L("k", 0, 4, "K")],
             {"a": arr("A", (17, 16), ("4 * i - j", "5 * k")), "b": arr("B", (16, 4), ("5 * k", "j")),
              "c": arr("C", (33, 4), ("8 * i", "j"))},
             math_note="C[8i][j] += A[4i - j][5k] B[5k][j], i in [1, 5): j is read by all three\n"
                       "operands -- a batch loop -- and A's row view moves with it (offset -j);\n"
                       "per j a 4x1x4 matrix-vector product, A's rows 4i - j inside [1, 16].\n"
                       "Offloaded since ISSUE-007 (four invocations per call).",
             issue="ISSUE-007"),
    ext_case(16, "batch_dependent_a_row_tiles", "batched",
             [L("i", 1, 17, "I"), L("j", 0, 4, "P"), L("k", 0, 16, "K")],
             {"a": arr("A", (65, 76), ("4 * i - j", "5 * k")), "b": arr("B", (76, 4), ("5 * k", "j")),
              "c": arr("C", (129, 4), ("8 * i", "j"))},
             math_note="ext_015 with i in [1, 17) and k in [0, 16): a 16x1x16 product per j,\n"
                       "several tiles per batch value, each a slice of the moving view.\n"
                       "Offloaded since ISSUE-007.",
             issue="ISSUE-007"),
    ext_case(17, "a_rows_skewed_by_column", "batched",
             [L("i", 0, 8, "I"), L("j", 0, 8, "P"), L("k", 0, 8, "K")],
             {"a": arr("A", (15, 8), ("i + j", "k")), "b": arr("B", (8, 16), ("k", "2 * j")),
              "c": arr("C", (8, 8), ("i", "j"))},
             math_note="C[i][j] += A[i + j][k] B[k][2j]: not one GEMM of A and B (A's row moves\n"
                       "with the output column), but j is read by all three operands: per j\n"
                       "the matrix-vector product C[:, j] += A[j .. j + 7][:] B[:, 2j].\n"
                       "Offloaded since ISSUE-007 (test/SystolicCC/depth.c ng_skew).",
             issue="ISSUE-007"),
    ext_case(18, "batch_index_per_operand", "batched",
             [L("b", 0, 2, "P"), L("i", 0, 4, "I"), L("j", 0, 4, "J"), L("k", 0, 4, "K")],
             {"a": arr("A", (3, 4, 4), ("b + 1", "i", "k")), "b": arr("B", (2, 4, 4), ("b", "k", "j")),
              "c": arr("C", (2, 4, 4), ("b", "i", "j"))},
             math_note="Each operand at its own batch index: A[b + 1] with B[b] and C[b] -- two\n"
                       "4x4x4 GEMMs.  Refused until ISSUE-007 (one view per operand of a batch\n"
                       "coordinate), offloaded since.",
             issue="ISSUE-007"),
]

# ==========================================================================
# BOUNDARY: cases 101 .. added after the hundred to cover the recognition
# boundaries the hundred left out (decreasing loops, non-divisible steps,
# per-operand offsets / strides, coupled and singular mappings, large and
# overflowing coefficients, empty loops).  Each goes to the set its
# SPECIFICATION puts it in -- positive (must be ACCEPTED), negative (must be
# REJECTED, or a documented limitation), extended (the specification does not
# decide: a recorded spec question) -- never to the set the current
# implementation's behaviour would suggest.  The hundred are untouched.
# ==========================================================================

BOUNDARY = []


def Bp(*a, **k):
    k.setdefault("set", "positive")
    BOUNDARY.append(gemm(*a, **k))


def Bn(*a, **k):
    k.setdefault("set", "negative")
    BOUNDARY.append(gemm(*a, **k))


def Bx(num, name, category, loops, A, B, C, **k):
    BOUNDARY.append(Case(id="case_%03d" % num, name=name, set="extended", category=category,
                         loops=loops, arrays={"a": A, "b": B, "c": C}, **k))


# --- B1: loop steps that do not divide the bounds, several steps at once
Bp(101, "step_3_of_10_nondivisible", "loop_step",
   [L("i", 0, 10, "I", step=3), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (10, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (10, 8), ("i", "j")),
   notes="i < 10 with i += 3 visits 0, 3, 6, 9: four iterations, the bound is not a\n"
         "multiple of the step (affine.for 0 to 10 step 3 has trip count ceil(10/3)).")
Bp(102, "steps_nondivisible_all_three", "loop_step",
   [L("i", 0, 10, "I", step=3), L("j", 0, 11, "J", step=4), L("k", 0, 7, "K", step=5)],
   arr("A", (10, 7), ("i", "k")), arr("B", (7, 11), ("k", "j")), arr("C", (10, 11), ("i", "j")),
   notes="Three steps, none dividing its bound: i = 0, 3, 6, 9; j = 0, 4, 8; k = 0, 5.\n"
         "M = 4, N = 3, K = 2 over strided views of 10x7, 7x11 and 10x11 arrays.")
Bp(103, "steps_2_3_4_divisible", "loop_step",
   [L("i", 0, 16, "I", step=2), L("j", 0, 24, "J", step=3), L("k", 0, 32, "K", step=4)],
   arr("A", (16, 32), ("i", "k")), arr("B", (32, 24), ("k", "j")), arr("C", (16, 24), ("i", "j")),
   notes="A different positive step on every loop (2, 3, 4), each dividing its bound.")
Bp(104, "le_bound_step_4_nondivisible", "loop_step",
   [L("i", 0, 10, "I", step=4, cmp="<="), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (11, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (11, 8), ("i", "j")),
   notes="i <= 10 with i += 4: 0, 4, 8 (affine.for 0 to 11 step 4, three iterations).")
Bp(105, "origin_1_step_5_nondivisible", "loop_step",
   [L("i", 1, 12, "I", step=5), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (12, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (12, 8), ("i", "j")),
   notes="i = 1, 6, 11: origin 1, step 5, bound 12 (rows 1, 6, 11 of A and C).")

# --- B2: a shared coordinate spelled differently by the two operands (per-operand
#         offsets and strides): dense GEMMs of the operands' own views.  All were
#         FALSE NEGATIVES of ISSUE-001 until its fix; they are TRUE POSITIVES now.
Bp(106, "operand_offset_b_on_k", "operand_view",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (10, 8), ("k + 2", "j")), arr("C", (8, 8), ("i", "j")),
   notes="C += A B[2:10, :]: the reduction coordinate is k on A and k + 2 on B --\n"
         "the same coordinate, B through its own offset view (ISSUE-001's family, v5).",
   issue="ISSUE-001")
Bp(107, "operand_offset_b_on_j", "operand_view",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 10), ("k", "j + 2")), arr("C", (8, 8), ("i", "j")),
   notes="C += A B[:, 2:10]: J is j on C and j + 2 on B.", issue="ISSUE-001")
Bp(108, "operand_offsets_differ_a_c", "operand_view",
   ijk(8, 8, 8), arr("A", (9, 8), ("i + 1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (11, 8), ("i + 3", "j")),
   notes="C[3:11, :] += A[1:9, :] B: both operands offset on I, by different amounts.",
   issue="ISSUE-001")
Bp(109, "operand_stride_a_on_i", "operand_view",
   ijk(8, 8, 8), arr("A", (16, 8), ("2 * i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   notes="C += A[0::2, :] B: the even rows of A against the plain rows of C (the mirror\n"
         "image of case_094, where the stride was on C).", issue="ISSUE-001")
Bp(110, "operand_stride_b_on_j", "operand_view",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 22), ("k", "3 * j")), arr("C", (8, 8), ("i", "j")),
   notes="C += A B[:, 0::3]: every third column of B into consecutive columns of C.",
   issue="ISSUE-001")
Bp(111, "operand_stride_a_on_k", "operand_view",
   ijk(8, 8, 8), arr("A", (8, 16), ("i", "2 * k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   notes="C += A[:, 0::2] B: K is 2k on A and k on B (the stride on the reduction, one\n"
         "operand only; case_043 has it on both).", issue="ISSUE-001")
Bp(112, "operand_stride_c_on_j", "operand_view",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 16), ("i", "2 * j")),
   notes="C[:, 0::2] += A B: the product lands in the even columns of C.", issue="ISSUE-001")
Bp(113, "operand_strides_2_and_3", "operand_view",
   ijk(8, 8, 8), arr("A", (16, 8), ("2 * i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (22, 8), ("3 * i", "j")),
   notes="C[0::3, :] += A[0::2, :] B: I has stride 2 on A and 3 on C -- neither expression\n"
         "is a multiple of the other; both are views of the one coordinate i.",
   issue="ISSUE-001")
Bp(114, "operand_views_all_coordinates", "operand_view",
   ijk(8, 8, 8), arr("A", (9, 16), ("i + 1", "2 * k")), arr("B", (11, 8), ("k + 3", "j")), arr("C", (16, 10), ("2 * i", "j + 2")),
   notes="Every shared coordinate spelled differently: I = i + 1 (A) / 2i (C), J = j (B) /\n"
         "j + 2 (C), K = 2k (A) / k + 3 (B).  C[0::2, 2:10] += A[1:9, 0::2] B[3:11, :].",
   issue="ISSUE-001")

# --- B3: large loop origins and coefficients that are valid C
Bp(115, "long_counter_origin_beyond_int", "loop_bounds",
   [L("i", 2147483645, 2147483649, "I", ctype="long"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (4, 8), ("i - 2147483645", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (4, 8), ("i - 2147483645", "j")),
   notes="A long counter running past INT_MAX (2147483645 .. 2147483648) with the\n"
         "subscript i - 2147483645: the origin and the constant cancel, rows 0..3.\n"
         "Loop bounds and the affine constant exceed 32 bits; the trip count is 4.")
Bp(116, "int_counter_last_increment_int_max", "loop_bounds",
   [L("i", 2147483645, 2147483646, "I", cmp="<="), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (2, 8), ("i - 2147483645", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (2, 8), ("i - 2147483645", "j")),
   notes="int i = 2147483645; i <= 2147483646: the last increment makes i exactly\n"
         "INT_MAX, which is defined behaviour (one more would not be, and the front end\n"
         "refuses such a loop: 'the loop counter could overflow').  Trip count 2.")
Bp(117, "huge_coefficient_single_iteration", "coefficient_range",
   [L("i", 0, 1, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (1, 8), ("i * 4611686018427387904", "k")), arr("B", (8, 8), ("k", "j")),
   arr("C", (1, 8), ("i * 4611686018427387904", "j")),
   notes="The row subscript i * 2^62 with a single iteration i = 0: a valid C program\n"
         "(the only index computed is 0) and a dense 1x8x8 GEMM whose row view has\n"
         "stride 2^62 and extent 1.  The coefficient is as large as int64 allows for a\n"
         "stride; every product the compiler forms from it (scale x extent, offset +\n"
         "scale x (extent - 1)) must be computed without overflow.")
Bp(118, "unsigned_counter", "loop_syntax",
   [L("i", 0, 8, "I", ctype="unsigned"), L("j", 0, 8, "J"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
   notes="An unsigned loop counter: inside the front end's subset (an initialized\n"
         "integer counter with static bounds).")

# --- B4: decreasing loops (negative steps): the coordinate decreases along its
#         loop. Written when the front end refused them (a documented
#         limitation then); supported since ISSUE-004 (2026-10-15) -- the cases
#         stay in negative/ (files are never moved), tagged with the issue
Bn(119, "decreasing_loop_i_step_minus_2", "decreasing_loop",
   [L("i", 7, 0, "I", step=-2, cmp=">=", inc="-="), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   math_note="for (int i = 7; i >= 0; i -= 2): rows 7, 5, 3, 1 of A and C, in that order.\n"
             "The same GEMM as case_031 on the odd rows, with I enumerated downward\n"
             "(I = 7 - 2 ordinal).  A valid GEMM; the specification lists a coordinate\n"
             "that decreased along its loop as a documented limitation until ISSUE-004\n"
             "(ext_003 is the same mathematics through a reversed subscript); now the\n"
             "front end emits the ordinal loop and 7 - 2 t, and it is offloaded.",
   issue="ISSUE-004")
Bn(120, "decreasing_loop_k_minus_minus", "decreasing_loop",
   [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 7, 0, "K", step=-1, cmp=">=", inc="--")],
   *std_arrays(8, 8, 8),
   math_note="The reduction loop counts down (for (int k = 7; k >= 0; --k)): the same\n"
             "products as case_001 summed in the opposite order.  The result is bitwise\n"
             "equal here only because the data are small integers; in general a reversed\n"
             "reduction order changes floating-point rounding (as for every offloaded\n"
             "GEMM: the arrays sum in their own order).  A valid GEMM; offloaded since\n"
             "ISSUE-004.",
   issue="ISSUE-004")
Bn(121, "decreasing_loop_j_gt_bound_step_3", "decreasing_loop",
   [L("i", 0, 8, "I"), L("j", 7, 0, "J", step=-3, cmp=">", inc="-="), L("k", 0, 8, "K")],
   *std_arrays(8, 8, 8),
   math_note="for (int j = 7; j > 0; j -= 3): columns 7, 4, 1 of B and C (the bound is\n"
             "exclusive and not a multiple of the step).  Offloaded since ISSUE-004.",
   issue="ISSUE-004")
Bn(122, "mixed_direction_steps", "decreasing_loop",
   [L("i", 0, 16, "I", step=2), L("j", 7, 0, "J", step=-3, cmp=">=", inc="-="), L("k", 0, 8, "K")],
   arr("A", (16, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (16, 8), ("i", "j")),
   math_note="i increases by 2, j decreases by 3 (7, 4, 1), k increases by 1: the even rows\n"
             "and the columns 7, 4, 1.  Offloaded since ISSUE-004.",
   issue="ISSUE-004")

# --- B5: mappings that are not a GEMM of the operands (coupled, singular,
#         the wrong coordinate in an operand)
Bn(123, "output_on_the_diagonal", "singular_mapping",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "i")),
   math_class="not_gemm_diagonal_output",
   math_note="C[i][i] += A[i][k] * B[k][j]: both subscripts of the output are the row\n"
             "coordinate; j is summed away: C[i][i] += sum_k A[i][k] (sum_j B[k][j]).\n"
             "The output is not indexed by (I, J).  Not a GEMM.  Expected rejection at\n"
             "recognition: C's column expression i is not B's column expression j.")
Bn(124, "b_column_is_the_row_coordinate", "coupled_mapping",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "i")), arr("C", (8, 8), ("i", "j")),
   math_class="not_gemm_rank1_broadcast",
   math_note="C[i][j] += A[i][k] * B[k][i]: B's column is the row coordinate i, j appears\n"
             "in C only.  Every column of C receives the diagonal of A B: C[i][j] +=\n"
             "(A B)[i][i].  Not a GEMM of A and B.  Expected rejection at recognition:\n"
             "B's column expression i is not C's column expression j.")
Bn(125, "a_on_its_diagonal", "singular_mapping",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "i")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   math_class="not_gemm_scaled_rows",
   math_note="C[i][j] += A[i][i] * B[k][j]: A is read on its diagonal (no K in A), so\n"
             "C[i][j] += A[i][i] (sum_k B[k][j]) -- the column sums of B scaled per row.\n"
             "Not a GEMM.  Expected rejection at recognition: A's column expression i is\n"
             "not B's row expression k.")
Bn(126, "a_column_couples_i_and_k", "coupled_mapping",
   ijk(8, 8, 8), arr("A", (8, 15), ("i", "i + k")), arr("B", (8, 8), ("k", "j")), arr("C", (8, 8), ("i", "j")),
   math_class="gemm_of_skewed_view",
   math_note="C[i][j] += A[i][i+k] * B[k][j]: A is read along a skew, A'(i,k) = A[i][i+k]\n"
             "(a Hankel-like gather).  Mathematically C += A' B for that A', but A' is not\n"
             "an affine strided view of A: the subscript i + k depends on two coordinates\n"
             "(coupled), which the specification's definition excludes.  Not a dense GEMM\n"
             "of A.  Expected rejection at recognition: A's column expression i + k is not\n"
             "B's row expression k.")
Bn(127, "c_row_couples_i_and_k", "coupled_mapping",
   ijk(8, 8, 8), arr("A", (8, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (15, 8), ("i + k", "j")),
   math_class="not_gemm_skewed_output",
   math_note="C[i+k][j] += A[i][k] * B[k][j]: the output row depends on the reduction\n"
             "index, so no element of C accumulates a full dot product; C[r][j] receives\n"
             "the sum over the anti-diagonal i + k = r.  Not a GEMM.  Expected rejection at\n"
             "recognition: C's row expression i + k is not A's row expression i.")

# --- B6: open questions of the specification (recorded, not judged)
Bx(128, "split_coordinate_radix_differs_per_operand", "split_coordinate",
   [L("i0", 0, 2, "I"), L("i1", 0, 4, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (8, 8), ("4 * i0 + i1", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (12, 8), ("8 * i0 + i1", "j")),
   math_note="I is split over i0 and i1 with radix 4 on A (rows 0..7) and radix 8 on C\n"
             "(rows 0..3 and 8..11): each operand's subscript is injective on (i0, i1),\n"
             "so by the definition C'(I,J) += A'(I,K) B'(K,J) holds with C' a gather of\n"
             "C's rows {0,1,2,3,8,9,10,11} -- but C' is not a strided sub-matrix.  The\n"
             "specification's 'inside' list names per-operand offsets and strides and\n"
             "split coordinates, not a per-operand radix: open question SQ-1.  The\n"
             "recognizer refuses it (the two row expressions are not multiples of one\n"
             "linear form); the behaviour is recorded, not judged.")
Bx(129, "zero_trip_i", "empty_domain",
   [L("i", 0, 0, "I"), L("j", 0, 8, "J"), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
   math_note="for (int i = 0; i < 0; ++i): the nest runs zero times, C is unchanged (an\n"
             "M = 0 GEMM).  Whether the compiler must offload it (zero invocations) or\n"
             "refuse it is open question SQ-2; the result must be correct either way.\n"
             "The front end emits affine.for 0 to 0; the recognizer accepts the nest and\n"
             "getDenseLogicalGemm refuses it (an empty loop has no normalized extent),\n"
             "with the message meant for symbolic bounds.")
Bx(130, "zero_trip_k", "empty_domain",
   [L("i", 0, 8, "I"), L("j", 0, 8, "J"), L("k", 0, 0, "K")], *std_arrays(8, 8, 8),
   math_note="K = 0: an empty reduction; C += A B with no products is C.  Open question\n"
             "SQ-2 (see case_129); the compiler refuses it today.")
Bx(131, "zero_trip_le_bound", "empty_domain",
   [L("i", 0, 8, "I"), L("j", 5, 4, "J", cmp="<="), L("k", 0, 8, "K")], *std_arrays(8, 8, 8),
   math_note="j = 5; j <= 4: an empty loop written with an inclusive bound (affine.for 5\n"
             "to 5).  Open question SQ-2.")

# --- B7: an affine loop condition (the counter inside an expression), counting
#         down -- translated since 2026-10-15 (front end: E OP C with E affine
#         in the counter is the equivalent comparison of the counter itself)
Bp(132, "affine_condition_counting_down", "loop_syntax",
   [L("i", 100, 1, "I", step=-1, cmp=">", inc="post--", cond="2 * i - 3 > 0"), L("j", 0, 8, "J"), L("k", 0, 8, "K")],
   arr("A", (101, 8), ("i", "k")), arr("B", (8, 8), ("k", "j")), arr("C", (101, 8), ("i", "j")),
   notes="for (int i = 100; 2 * i - 3 > 0; i--): the condition is an affine expression of\n"
         "the counter; over the integers it is i > 1 (2 * i - 3 > 0 is i > 1.5), so the\n"
         "rows are 100, 99, ..., 2 -- 99 iterations, the loop ends at i = 1 (2 - 3 = -1).\n"
         "The kernel keeps the condition as written; the generator runs it to get the\n"
         "values (and checks them against i > 1).  A 99 x 8 x 8 GEMM on rows 2 .. 100.")

assert [c.id for c in BOUNDARY] == ["case_%03d" % n for n in range(101, 133)]

ALL = POSITIVE + NEGATIVE + EXTENDED + BOUNDARY

# ==========================================================================
# MLIR-level cases (mlir_001 ..): nests the C front end cannot produce --
# static shapes on every dimension (so out-of-bounds views are decidable),
# coefficients beyond what a valid C program can index, loop bounds that
# depend on another counter, symbolic bounds.  They run through
# --test-affine-matmul-discovery and --affine-matmul-to-systolic only.
# ==========================================================================

from casespec import MlirCase  # noqa: E402


def gemm_mlir(name, A, B, C, loops, loadA, loadB, loadC, shapes=("8x8", "8x8", "8x8"), attrs=" {llvm.noalias}"):
    """A func.func with the canonical MAC body; `loops` is the affine.for header
    lines (outer to inner), `loadX` the index lists of the three accesses."""
    sa, sb, sc = shapes
    ind = "  " * (len(loops) + 1)
    body = []
    body.append("func.func @%s(%%%s: memref<%sxf32>%s, %%%s: memref<%sxf32>%s, %%%s: memref<%sxf32>%s) {"
                % (name, A, sa, attrs, B, sb, attrs, C, sc, attrs))
    for d, lp in enumerate(loops):
        body.append("  " * (d + 1) + lp + " {")
    body.append(ind + "%%a = affine.load %%%s[%s] : memref<%sxf32>" % (A, loadA, sa))
    body.append(ind + "%%b = affine.load %%%s[%s] : memref<%sxf32>" % (B, loadB, sb))
    body.append(ind + "%%c = affine.load %%%s[%s] : memref<%sxf32>" % (C, loadC, sc))
    body.append(ind + "%p = arith.mulf %a, %b : f32")
    body.append(ind + "%s = arith.addf %c, %p : f32")
    body.append(ind + "affine.store %%s, %%%s[%s] : memref<%sxf32>" % (C, loadC, sc))
    for d in range(len(loops) - 1, -1, -1):
        body.append("  " * (d + 1) + "}")
    body.append("  return")
    body.append("}")
    return "\n".join(body)


IJK8 = ["affine.for %i = 0 to 8", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"]

MLIR_CASES = [
    MlirCase("mlir_001", "oob_static_dim0_offset", "negative", "out_of_bounds", "oob_static_dim0_offset",
             gemm_mlir("oob_static_dim0_offset", "A", "B", "C", IJK8, "%i + 4, %k", "%k, %j", "%i + 4, %j"),
             math_note="C[i+4][j] += A[i+4][k] B[k][j] for i, j, k < 8 on 8x8 memrefs: rows 4..11 of A and\n"
                       "C do not exist.  As a program the nest is undefined (the accesses leave the\n"
                       "arrays), so there is no GEMM to lower: the lowering must refuse it, saying\n"
                       "why.  (The C front end gives array parameters a dynamic leading dimension,\n"
                       "so this check is reachable only with static shapes.)",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="supported", cur_lowering="unsupported",
             cur_reject_stage="lowering", cur_reject_reason="A is accessed out of bounds along dimension 0"),
    MlirCase("mlir_002", "oob_static_dim1_operand_stride", "negative", "out_of_bounds", "oob_static_dim1_operand_stride",
             gemm_mlir("oob_static_dim1_operand_stride", "A", "B", "C", IJK8, "%i, %k", "%k, %j * 3", "%i, %j"),
             math_note="C[i][j] += A[i][k] B[k][3j]: B's column view has stride 3 (a per-operand\n"
                       "stride, fine in itself) but 3 * 7 = 21 exceeds B's 8 columns.  Undefined;\n"
                       "the lowering must refuse B's view.",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="supported", cur_lowering="unsupported",
             cur_reject_stage="lowering", cur_reject_reason="B is accessed out of bounds along dimension 1",
             expected_views={"B": "[1*K + 0, 3*J + 0]"}),
    MlirCase("mlir_003", "oob_offset_plus_extent_by_one", "negative", "out_of_bounds", "oob_offset_plus_extent_by_one",
             gemm_mlir("oob_offset_plus_extent_by_one", "A", "B", "C",
                       ["affine.for %i = 0 to 7", "affine.for %j = 0 to 7", "affine.for %k = 0 to 8"],
                       "%i + 2, %k", "%k, %j + 2", "%i + 2, %j + 2"),
             math_note="Offsets 2 with extents 7 on 8x8 arrays: the last row/column touched is 2 + 6 =\n"
                       "8, one past the end on every operand but B's rows.  The boundary of the bounds\n"
                       "check: offset + scale * (extent - 1) = 8 is not < 8.",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="supported", cur_lowering="unsupported",
             cur_reject_stage="lowering", cur_reject_reason="A is accessed out of bounds along dimension 0"),
    MlirCase("mlir_004", "overflow_scale_2pow62_extent_3", "negative", "coefficient_overflow", "overflow_scale_2pow62_extent_3",
             gemm_mlir("overflow_scale_2pow62_extent_3", "A", "B", "C",
                       ["affine.for %i = 0 to 3", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i * 4611686018427387904, %k", "%k, %j", "%i * 4611686018427387904, %j"),
             math_note="The row subscript i * 2^62 for i = 0, 1, 2 on 8x8 arrays: rows 0, 2^62 and 2^63\n"
                       "-- the last does not even fit in int64.  Undefined (out of bounds); the\n"
                       "compiler must refuse it without computing a wrapped-around product: the\n"
                       "coordinate's scale x extent overflows 64 bits (ISSUE-002).",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="logical coordinate I's scale or extent overflows 64-bit arithmetic",
             issue="ISSUE-002"),
    MlirCase("mlir_005", "overflow_scale_2pow62_wraps_to_zero", "negative", "coefficient_overflow", "overflow_scale_2pow62_wraps_to_zero",
             gemm_mlir("overflow_scale_2pow62_wraps_to_zero", "A", "B", "C",
                       ["affine.for %i = 0 to 5", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i * 4611686018427387904, %k", "%k, %j", "%i * 4611686018427387904, %j"),
             math_note="i * 2^62 for i < 5: in wrapping int64 arithmetic the last row, 2^62 * 4 =\n"
                       "2^64, is 0 -- a bounds check computed without overflow detection would\n"
                       "accept rows [0, 0] and lower an out-of-bounds nest (at 6e25528 only the\n"
                       "partition's Presburger check happened to catch it, with the wrong reason).\n"
                       "Undefined; must be refused for the right reason (ISSUE-002).",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="logical coordinate I's scale or extent overflows 64-bit arithmetic",
             issue="ISSUE-002"),
    MlirCase("mlir_006", "offset_2pow62_out_of_bounds", "negative", "coefficient_overflow", "offset_2pow62_out_of_bounds",
             gemm_mlir("offset_2pow62_out_of_bounds", "A", "B", "C",
                       ["affine.for %i = 0 to 4", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i + 4611686018427387904, %k", "%k, %j", "%i + 4611686018427387904, %j"),
             math_note="A constant offset of 2^62 on the rows of A and C: representable, plainly out\n"
                       "of bounds.  Refused by the lowering's bounds check.",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="supported", cur_lowering="unsupported",
             cur_reject_stage="lowering", cur_reject_reason="A is accessed out of bounds along dimension 0"),
    MlirCase("mlir_007", "large_stride_representable_oob", "negative", "coefficient_overflow", "large_stride_representable_oob",
             gemm_mlir("large_stride_representable_oob", "A", "B", "C",
                       ["affine.for %i = 0 to 4", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i * 2305843009213693952, %k", "%k, %j", "%i * 2305843009213693952, %j"),
             math_note="Stride 2^61 with 4 iterations: the last row, 3 * 2^61, is representable and out\n"
                       "of bounds, but the coordinate's exclusive end 4 * 2^61 = 2^63 -- which the\n"
                       "partition's half-open regions are computed with -- is not; getDenseLogicalGemm\n"
                       "refuses the coordinate for that before the bounds check is reached (ISSUE-002).\n"
                       "Undefined either way; refused with the overflow reason.",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="logical coordinate I's scale or extent overflows 64-bit arithmetic",
             issue="ISSUE-002"),
    MlirCase("mlir_008", "per_operand_view_offset_overflow", "negative", "coefficient_overflow", "per_operand_view_offset_overflow",
             gemm_mlir("per_operand_view_offset_overflow", "A", "B", "C",
                       ["affine.for %i = 0 to 4", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i * 4611686018427387904 + 4611686018427387904, %k", "%k, %j", "%i * 4611686018427387904, %j"),
             math_note="A's rows are 2^62 i + 2^62, C's 2^62 i: the same coordinate i (scale 1,\n"
                       "extent 4: nothing overflows in the dense form), A and C through views of\n"
                       "stride 2^62 -- and A's offset plus its span, 2^62 + 2^62 * 3 = 2^64, exceeds\n"
                       "int64.  Undefined; the lowering must refuse A's view for the overflow rather\n"
                       "than compute a wrapped-around last index (ISSUE-002).",
             is_gemm=False, math_class="undefined_out_of_bounds",
             spec_recognition="REJECT", spec_basis_key="out_of_bounds",
             cur_recognition="accept", cur_dense="supported", cur_lowering="unsupported",
             cur_reject_stage="lowering",
             cur_reject_reason="A's subscripts along dimension 0 overflow 64-bit arithmetic",
             issue="ISSUE-002",
             expected_views={"A": "[4611686018427387904*I + 4611686018427387904, 1*K + 0]",
                             "C": "[4611686018427387904*I + 0, 1*J + 0]"}),
    MlirCase("mlir_009", "triangular_bounds", "negative", "coordinate_dependency", "triangular_bounds",
             gemm_mlir("triangular_bounds", "A", "B", "C",
                       ["affine.for %i = 0 to 8", "affine.for %j = affine_map<(d0) -> (d0)>(%i) to 8", "affine.for %k = 0 to 8"],
                       "%i, %k", "%k, %j", "%i, %j"),
             math_note="j runs from i: only the upper triangle of C (j >= i) accumulates A B.  The\n"
                       "iteration domain is not a box, so the computed elements are not one\n"
                       "rectangular GEMM of strided views (the specification's definition needs a\n"
                       "box).  Not a dense GEMM; must be refused.  The recognizer accepts the access\n"
                       "pattern; getDenseLogicalGemm refuses the non-static (counter-dependent)\n"
                       "bound.  The C front end cannot express this nest (bounds must be constants).",
             is_gemm=False, math_class="gemm_values_triangular_region",
             spec_recognition="REJECT", spec_basis_key="not_gemm",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="the loop bounds are not static, so the iteration domain has no static normalized extent"),
    MlirCase("mlir_010", "reversed_coordinate_on_a_only", "negative", "negative_coefficient", "reversed_coordinate_on_a_only",
             gemm_mlir("reversed_coordinate_on_a_only", "A", "B", "C", IJK8, "7 - %i, %k", "%k, %j", "%i, %j"),
             math_note="C[i][j] += A[7-i][k] B[k][j]: C += (A with its rows reversed) B, a valid GEMM\n"
                       "of a reversed view of A.  I is i on C and 7 - i on A: opposite directions, a\n"
                       "documented limitation until 2026-10-15; lowered since ISSUE-005 (P0.3): A's\n"
                       "view runs backwards from row 7 (a negative stride), C's forwards.",
             is_gemm=True, math_class="gemm",
             spec_recognition="ACCEPT", spec_basis_key="negative_scale",
             cur_recognition="accept", cur_dense="supported", cur_lowering="supported",
             issue="ISSUE-005", expected_views={"A": "[-1*I + 7, 1*K + 0]"}),
    MlirCase("mlir_011", "symbolic_bound", "extended", "symbolic_bound", "symbolic_bound",
             "func.func @symbolic_bound(%A: memref<?x8xf32> {llvm.noalias}, %B: memref<8x8xf32> {llvm.noalias}, "
             "%C: memref<?x8xf32> {llvm.noalias}, %n: index) {\n"
             "  affine.for %i = 0 to %n {\n"
             "    affine.for %j = 0 to 8 {\n"
             "      affine.for %k = 0 to 8 {\n"
             "        %a = affine.load %A[%i, %k] : memref<?x8xf32>\n"
             "        %b = affine.load %B[%k, %j] : memref<8x8xf32>\n"
             "        %c = affine.load %C[%i, %j] : memref<?x8xf32>\n"
             "        %p = arith.mulf %a, %b : f32\n"
             "        %s = arith.addf %c, %p : f32\n"
             "        affine.store %s, %C[%i, %j] : memref<?x8xf32>\n"
             "      }\n"
             "    }\n"
             "  }\n"
             "  return\n"
             "}",
             math_note="M = %n, a symbolic extent: a GEMM whose row count is a runtime value.  The C\n"
                       "front end cannot produce it (bounds must be constants), and the specification\n"
                       "does not say whether the MLIR path must support dynamic extents: open question\n"
                       "SQ-3.  Today: recognized (row extent symbolic), no dense form, not lowered.",
             is_gemm=True, math_class="gemm_dynamic_extent",
             spec_recognition="UNDETERMINED", spec_basis_key="SQ-3",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="the loop bounds are not static, so the iteration domain has no static normalized extent"),
    MlirCase("mlir_012", "split_radix_differs_per_operand", "extended", "split_coordinate", "split_radix_differs_per_operand",
             gemm_mlir("split_radix_differs_per_operand", "A", "B", "C",
                       ["affine.for %i0 = 0 to 2", "affine.for %i1 = 0 to 4", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i0 * 4 + %i1, %k", "%k, %j", "%i0 * 8 + %i1, %j", shapes=("8x8", "8x8", "12x8")),
             math_note="case_128 at the MLIR level: I = 4 i0 + i1 on A, 8 i0 + i1 on C.  Open question\n"
                       "SQ-1; the recognizer refuses the two expressions as one coordinate.",
             is_gemm=True, math_class="gemm_per_operand_radix",
             spec_recognition="UNDETERMINED", spec_basis_key="SQ-1",
             cur_recognition="reject", cur_dense="not_reached", cur_lowering="not_reached",
             cur_reject_stage="recognition",
             cur_reject_reason="not recognized: 4 i0 + i1 and 8 i0 + i1 are not multiples of one linear form"),
    MlirCase("mlir_013", "zero_trip_loop", "extended", "empty_domain", "zero_trip_loop",
             gemm_mlir("zero_trip_loop", "A", "B", "C",
                       ["affine.for %i = 0 to 0", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i, %k", "%k, %j", "%i, %j"),
             math_note="case_129 at the MLIR level (affine.for 0 to 0): open question SQ-2.  Today:\n"
                       "recognized, refused by getDenseLogicalGemm as an empty GEMM (its own\n"
                       "reason since 2026-10-15; before, the message meant for symbolic bounds).",
             is_gemm=True, math_class="gemm",
             spec_recognition="UNDETERMINED", spec_basis_key="SQ-2",
             cur_recognition="accept", cur_dense="unsupported", cur_lowering="not_reached",
             cur_reject_stage="dense_partition",
             cur_reject_reason="a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)"),
    MlirCase("mlir_014", "per_operand_views_all_coordinates", "positive", "operand_view", "per_operand_views_all_coordinates",
             gemm_mlir("per_operand_views_all_coordinates", "A", "B", "C",
                       ["affine.for %i = 0 to 4", "affine.for %j = 0 to 4", "affine.for %k = 0 to 4"],
                       "%i + 1, %k * 2", "%k + 3, %j", "%i * 2, %j + 2"),
             math_note="case_114 at the MLIR level with static 8x8 shapes: C[0::2, 2:6] += A[1:5, 0::2]\n"
                       "B[3:7, 0:4], every shared coordinate spelled differently by its two operands.\n"
                       "A dense GEMM of the operands' own views: must be recognized with the views\n"
                       "A = [I + 1, 2K], B = [K + 3, J], C = [2I, J + 2] and lowered.",
             is_gemm=True, math_class="gemm",
             spec_recognition="ACCEPT", spec_basis_key="dense_gemm",
             cur_recognition="accept", cur_dense="supported", cur_lowering="supported",
             issue="ISSUE-001",
             expected_views={"A": "[1*I + 1, 2*K + 0]", "B": "[1*K + 3, 1*J + 0]", "C": "[2*I + 0, 1*J + 2]"}),
    MlirCase("mlir_015", "huge_stride_single_row", "positive", "coefficient_range", "huge_stride_single_row",
             gemm_mlir("huge_stride_single_row", "A", "B", "C",
                       ["affine.for %i = 0 to 1", "affine.for %j = 0 to 8", "affine.for %k = 0 to 8"],
                       "%i * 4611686018427387904, %k", "%k, %j", "%i * 4611686018427387904, %j"),
             math_note="case_117 at the MLIR level: stride 2^62 on a coordinate of extent 1 -- row 0\n"
                       "only, a 1x8x8 GEMM, in bounds.  The compiler must not overflow while deciding\n"
                       "that (scale x extent = 2^62, offset + scale x (extent - 1) = 0).",
             is_gemm=True, math_class="gemm",
             spec_recognition="ACCEPT", spec_basis_key="dense_gemm",
             cur_recognition="accept", cur_dense="supported", cur_lowering="supported"),
]

assert [c.id for c in MLIR_CASES] == ["mlir_%03d" % n for n in range(1, 16)]
