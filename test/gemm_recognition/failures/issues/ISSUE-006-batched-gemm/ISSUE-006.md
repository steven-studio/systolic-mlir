# ISSUE-006 — batched GEMMs: the batch loops re-emitted around the systolic tiles

| field | value |
|---|---|
| Status | **IMPLEMENTED** (2026-10-15, uncommitted, in `~/work/systolic-mlir-affine` on checkpoint `3f21bab`, after ISSUE-005) — a specification change: batched GEMMs move from DOCUMENTED_LIMITATION to inside the specification |
| Before | `getDenseLogicalGemm` refused any partition prefix ("batched GEMM (a partition prefix P* of N coordinates) is not supported"); the group analysis of ISSUE-005 classified a batch coordinate VALID_BUT_LOWERING_UNSUPPORTED |
| Specification | `docs/AFFINE_GEMM_MAPPING.md` section 2 (roles: a coordinate read by all three operands is a batch coordinate) and section 4 (the batch loop) |
| Components | `lib/Systolic/Transforms/SystolicPipelines.cpp` (`buildSystolicRuntimeToLLVMPipeline`: `lower-affine` before `expand-strided-metadata`, see below); `lib/Systolic/Transforms/AffineMatmulPartitioning.cpp` (`getDenseLogicalGemm`: the P rows of T, the batch loops, their value range; `restrictToBox`: rows numP + d); `AffineMatmulAnalysis.cpp` (the group path accepts a batch coordinate the three operands read identically; `OperandLayout::batchDim`); `AffineMatmulToSystolic.cpp` (the batch loops, `affine.apply` batch indices, subviews with a dynamic offset, a rank-0 view expanded to 1 x 1; the remark's repeat count) |
| Cases whose expectation changed | case_085 (batched), ext_011 (two batch coordinates): OUT_OF_SCOPE → ACCEPT (`issue="ISSUE-006"`; case_085 stays in negative/, files are never moved). case_086 (B shared by every batch) stays OUT_OF_SCOPE with the new basis `batched_shared`. `depth.c` ng_batched: offloaded, 2 invocations per call. `affine_to_systolic_refused.mlir` @batched now shows a batch coordinate read through different expressions (refused) |
| Snapshot before | `~/work/integration-backups-20261015/target-pre-batched/` (tree, modes, diff, the P0 validation results as the baseline) |

## The change

A batched GEMM has one or more coordinates every operand reads (`C[b][i][j] += A[b][i][k] *
B[b][k][j]`): for each value of b, a GEMM on the slices `A[b]`, `B[b]`, `C[b]`. The recognizer
already carried them as the partition prefix P* of `y = (P*, I, J, K)`; the dense partition
refused them.

**What the lowering does now.** The nest's loops that drive a batch coordinate (a nonzero
coefficient in a P row, two or more iterations) are **re-emitted as they are** -- their own
bounds and steps, in their original order -- around the tiles. Inside them, each batch
coordinate's value is an `affine.apply` of its expression over the new loops' counters (a loop
that runs once is a constant, folded in), and each operand's batch dimension is read at that
value: a `memref.subview` with a dynamic offset there and size 1 (dropped from the rank-2
view). The I, J, K views, the fleet plan and the tiles are those of one batch: a plain GEMM of
M x N x K. The FPGA lowering stages every tile through a copy, so nothing downstream changes.

**Why it is correct.** For each batch iteration the original nest runs the full I, J, K nest on
the slice the batch index addresses; the lowering runs the tiles on exactly that slice (the
subview's offset is the same affine expression of the same loop counters, by construction). The
batch loops are not tiled, so nothing is required of the batch coordinate: not density, not
injectivity -- two batch iterations addressing the same slice accumulate into it twice, as the
loop does. The one condition is that a batch loop drives no I, J or K (the batches would be
coupled: `C[b][b + i]` has no box per batch value); it is checked in `getDenseLogicalGemm`
("two logical coordinates are driven by the same loop (loop s drives a batch coordinate and
I)"). Moving a batch loop that sat between I/J/K loops outward is a legal interchange: the
iterations of different batch values touch different C elements, and under the reassociation
permission the order of the accumulations into one C element is free anyway. The region
domains (`restrictToBox`) constrain only I, J, K, so every region runs for every batch value
and the exact-cover proof holds per batch iteration.

**Not lowered (classified, with a reason).** A batch coordinate the operands read through
different expressions (`A[b + 1]` with `C[b]`: one view per operand of a batch coordinate),
VALID_BUT_LOWERING_UNSUPPORTED; an operand shared by every batch (`C[b] += A[b] B`: B would be
broadcast, or A and C merged over (b, i)), refused by the group analysis as two coordinates in
one GEMM role; an element-wise product (every coordinate a batch one), PROVEN_INVALID.

**The LLVM pipeline.** A tile of a batch is a `tensor.extract_slice` of the batch's view, which
bufferizes to a `memref.subview` of a subview with a dynamic offset (the batch index).
`expand-strided-metadata` rewrites that into an `affine.apply` with the dynamic offset as a
*symbol*; computed from an `affine.for` counter, the offset is an affine *dimension*, and the
apply's verifier refuses it ("dimensional operand cannot be used as a symbol"). Found by the
generated corpus (13 of 800 cases, every batched case with more than one tile; the lit and
end-to-end cases written first had one tile each). Fixed in `buildSystolicRuntimeToLLVMPipeline`:
`lower-affine` runs before `expand-strided-metadata` (the loops are `scf.for` by then and the
operand is neither dimension nor symbol, which `affine.apply` allows), and again after it for
the applies the expansion creates. `affine_gemm_batched_lowering.mlir` @batched_tiles and
`affine_batched.c` bat_tiles (16 x 16 x 16, 10 tiles per batch) cover it down to LLVM and at
run time.

**SQ-5 (found by the generated corpus, `gen_04bff9d675c2`).** `C[i][k] += A[i][k] * B[k][j]`
with `j` of one iteration: every operand reads `k`, so it is a batch coordinate, `N = K = 1`,
and each batch is a column scaled by a scalar. The lowering computes the right result (the
case's program check passed), but README section 2 lists "the reduction index in the output"
as not a GEMM, and whether a batched vector-times-scalar is a GEMM to offload is not decided.
Refused conservatively (`getDenseLogicalGemm`: a batched nest whose per-batch GEMM has two
of M, N, K equal to one; VALID_BUT_LOWERING_UNSUPPORTED), recorded as **SQ-5** in
`SPEC_QUESTIONS.md`; the generator classifies the shape UNDETERMINED (`expected_refusal`).

**Semantics preserved for plain GEMMs.** Every row index of T in the dense partition is
`numP + d`, which is `d` when there is no prefix; the batch logic is skipped for `numP = 0`.

## Tests

- `test/Systolic/affine_gemm_batched_lowering.mlir`: the canonical batched GEMM, the batch loop
  in the middle of the nest with a step, two batch coordinates, an offset and counting-down
  batch with the batch dimension inside A; refused: coupled, different expressions,
  element-wise, an empty batch loop.
- `test/SystolicCC/affine_batched.c`: eight kernels end to end (the batch loop outermost, in the
  middle, strided, two coordinates, inside A, with a transposed and backwards operand, counting
  down, with an offset): equal to the literal loops, `--verify` bitwise, the invocation counts.
- `test/Systolic/affine_gemm_mapping_oracle.mlir`: the oracle generates batched nests (one in
  six) and the mutations `batch` (a role's subscript duplicated as a batch: coupled) and
  `batch_shift` (one operand's batch subscript shifted: not lowered); for a lowered batched nest
  it checks the extents and one `affine.for` per batch loop.
- `repro_batched.mlir` (here): case_085 and ext_011 at the MLIR level.
- Updated: `affine_gemm_batched.mlir`, `affine_gemm_partition_coupled.mlir`,
  `affine_to_systolic_refused.mlir`, `affine_gemm_p0_views.mlir` @batch_inner, `depth.c`.
