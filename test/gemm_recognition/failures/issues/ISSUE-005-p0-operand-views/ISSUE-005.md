# ISSUE-005 — operand views: transposes, negative strides, extent-one roles, other ranks; the floating-point policy

| field | value |
|---|---|
| Status | **IMPLEMENTED** (2026-10-15, uncommitted, in `~/work/systolic-mlir-affine` on checkpoint `3f21bab`) — a specification change made on the user's explicit request (the P0 scope) |
| Before | documented limitations (README.md §2): transposed operand views; one operand reading a shared coordinate in the direction opposite to the other's; matrix-vector products (two loops, rank-1 operands); a constant coordinate (SQ-4, refused); `min`/`max` of constants in a loop bound (refused as "not static"); every offload implicitly reassociating the floating-point sum |
| Specification | `docs/AFFINE_GEMM_MAPPING.md` (v2): sections 2 (groups and roles), 5 (views, signed strides, transposes, addresses), 6 (floating point), 9 (classification) |
| Components | `lib/Systolic/Transforms/AffineMatmulAnalysis.cpp` (groups and roles, signed views, layouts, classification, shallow candidates, folded min/max bounds); `AffineMatmulPartitioning.cpp` (extent-one roles, classification); `AffineMatmulToSystolic.cpp` (rank-reducing subview with signed strides, transpose, expand_shape, address checks, the FP gate, `classify`); `tools/systolic-cfront` and `tools/systolic-cc` (`--fp-reassociation`) |
| Cases whose expectation changed | case_083 (GEMV), case_084 (GEMV under repetition loops), case_091–093 (transposed A, B, C), mlir_010 (opposite directions): OUT_OF_SCOPE → ACCEPT (they stay in negative/, issue="ISSUE-005"); SQ-4 decided (ISSUE-003 reproducer: offloaded with M, N or K = 1); ISSUE-002 `@min_const_ub` (an 8x8x8 GEMM) |
| Decisions (the user, 2026-10-15) | strict floating-point default with explicit opt-in; SQ-4 offloaded; per-file `chmod u+w` of the protected test files edited; the board scripts get the permission flag |

## The change

**Coordinates by groups and roles.** The checkpoint's positional form — A = (P*, I, K),
B = (P*, K, J), C = (P*, I, J), the factors either way — is tried first, so every nest it
recognized keeps its mapping, views and diagnostics. When it does not apply, loops that occur
together in a subscript are one coordinate (a group), and a group's role is fixed by the operands
that read it (A and C: I; B and C: J; A and B: K; all three: a batch; one only: a broadcast). Each
role's two subscripts must be multiples of one linear form, **of either sign**. Operands may have
any rank; a dimension whose subscript is constant on the iteration domain is an extent-one
(fixed) dimension; a role that no loop drives has extent 1 (M, N or K = 1).

**Views.** Each operand's view is a rank-reducing `memref.subview` over all of its dimensions
— signed strides: a dimension read backwards has a negative stride and its largest index as the
offset — then `memref.transpose` when the operand stores its roles in the other order, then
`memref.expand_shape` for a role the operand has no dimension for. The coordinate runs along its
outermost loop's order, so the negative stride is on the operand written backwards. Every index
range and every linear element address (origin and corners) is checked in 64-bit-checked
arithmetic. The FPGA lowering stages every operand tile through `memref.copy` into contiguous
scratch, which carries any strides: no change downstream.

**Floating point.** The device sums each tile from zero and the host adds the sum to C; the loop
adds the products into C one by one. These are different IEEE operation sequences (even a
one-term tile turns a −0 product into +0), so no offload preserves strict semantics. The default
keeps every fp GEMM on the host, classified PROVEN_VALID, with the reason. Offload requires
the `reassoc` fast-math flag on the accumulation (`systolic-cfront --fp-reassociation`,
`systolic-cc --fp-reassociation`) or `--affine-matmul-to-systolic=allow-fp-reassociation=true`.
Every existing test that offloads states the permission explicitly in its RUN line (unchanged
expected values); the showcase runner passes it (`scripts/stages.py`).

**Classification.** Every refusal is PROVEN_INVALID, VALID_BUT_LOWERING_UNSUPPORTED or UNKNOWN
(an empty GEMM and an FP refusal: PROVEN_VALID, not offloaded); `--test-affine-matmul-discovery`
prints `class:`, `--affine-matmul-to-systolic=classify=true` appends `[CLASS]` to remarks.

## Tests

- `repro_p0_views.mlir` (here): the showcase's changed cases at the MLIR level.
- `test/Systolic/affine_gemm_p0_views.mlir`: the ten negative-stride cases of the request (A, B, C
  output, a decreasing sequence, with a step, with an offset, with a transpose, an origin inside
  the allocation, near the 64-bit limit and past it, out of bounds), transposes, GEMV, outer
  product, rank 3, `min`/`max` bounds, the `reassoc` flag, aliasing, batch; the FP default.
- `test/Systolic/affine_gemm_mapping_oracle.mlir`: brute-force oracle, the compiler's emitted
  views checked semantically on 1200 random nests, and the default (nothing lowered).
- `test/SystolicCC/affine_p0.c`: 13 kernels end to end (with and without the permission).
