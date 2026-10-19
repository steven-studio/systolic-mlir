# ISSUE-003 — a constant logical coordinate crashed getDenseLogicalGemm

| field | value |
|---|---|
| Status | **FIXED** in the integration worktree (2026-10-15, uncommitted; not yet reviewed) |
| Found by | the read-only overflow audit of the Mac/Linux integration (probe `noloop_c1.mlir`) |
| Present in | HEAD 1a663af, the Mac fix, the Linux fix, the integrated version — pre-existing, not introduced by the integration |
| Component | `lib/Systolic/Transforms/AffineMatmulPartitioning.cpp`, `getDenseLogicalGemm`: `int64_t expected = T[d][loops.front()];` |
| Classification | **compiler crash** on a nest the recognizer accepts; no wrong code was produced (the process died) |

## What happened

A nest whose two operands index a shared coordinate with the same constant subscript —
`C[0][j] += A[0][kk*8+k] * B[kk*8+k][j]` (I = 0), likewise a constant J or K — is accepted by
`recognizeLogicalMatmul` (identical subscripts). Every loop drives J or K, so the list of loops
that drive I is empty, and `T[d][loops.front()]` indexed past it.

Measured (2026-10-15):
- builds with assertions (`build-dbg`, Debug): ``SmallVector::front(): Assertion `!empty()' failed``,
  stack `getDenseLogicalGemm` (`AffineMatmulPartitioning.cpp:110`) ← `TestAffineMatmulDiscovery.cpp:205`;
  the same for `--affine-matmul-to-systolic`;
- release builds of HEAD, the Mac fix, the Linux fix and the integrated version: SIGSEGV (exit 139);
  with LLVM's symbolizer enabled the crash handler could hang until a timeout instead;
- reachable from C: `systolic-cfront` translates the statement above to the nest of
  `@const_row_tiled_k` in `repro_constant_coordinate.mlir`.

## Fix

`getDenseLogicalGemm` refuses a coordinate that no loop drives, before `loops.front()`:
*"logical coordinate I is driven by no loop (a constant subscript)"*. The recognizer is unchanged.

## Specification

Whether such a nest is a GEMM to offload with M, N or K = 1 is **not decided** — recorded as
SQ-4 in `test/gemm_recognition/SPEC_QUESTIONS.md`. The fix only guarantees "refused with a
reason, never a crash"; it does not implement offloading.

## Tests

`repro_constant_coordinate.mlir` (this directory; lit, no XFAIL): constant I, J and K, both the
discovery pass and the lowering pass. It fails on the builds without the fix (measured: HEAD and
the Mac fix fail both RUN lines) and passes on the fixed release and Debug (assertions) builds.

## Update 2026-10-15 (SQ-4 decided, ISSUE-005)

SQ-4 is decided: a coordinate no loop drives is an extent-one coordinate (M, N or K = 1) and the
nest is offloaded (the user's P0.5). `repro_constant_coordinate.mlir` now expects 1 x 8 x 16,
8 x 1 x 16 and 16 x 8 x 1 GEMMs; it still guards the crash (the dense partition never indexes an
empty loop list).
