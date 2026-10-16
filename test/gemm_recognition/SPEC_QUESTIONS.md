# Open specification questions (UNDETERMINED cases)

Each question below is a point the support specification (README.md §2) does not decide. A case
whose expectation depends on one is marked `UNDETERMINED` in `showcase/manifest.json`
(`expected.recognition`, with `expected.spec_question`), is run like every other case, and is
judged with one of two verdicts, **neither of them a PASS** (every report lists and counts them on
their own: `run_showcase.py`, `regression.py diff`, `scripts/inventory.py`):

- `expected_refusal`: the compiler refused the case at the predicted stage with the predicted
  reason (where the compiler states one) and the program's check passed. A conservative answer —
  the program stays on the host and computes the right result whichever way the question is
  decided — so it does not fail the run; it is not a pass either;
- `spec_open`: the compiler offloaded the case, a choice the specification has not made.
  `run_showcase.py` and `regression.py` exit non-zero while one is present, unless
  `--allow-spec-open` is given (like `--allow-known-issues` for open issues);
- its program must still compute the right result: a wrong result is `fail_runtime`, and a stage
  that behaves differently from the prediction is `unexpected_stage`, as for any case;
- a question is decided only in writing, in README.md §2, from the mathematics and the documented
  rules — never by what the compiler happens to do. Then the affected cases get an ACCEPT, REJECT
  or OUT_OF_SCOPE expectation and the question is moved to "Decided" below with its date.

The wording of each question is also in `generator/casespec.py: SPEC_QUESTIONS` (the source the
manifest is generated from); keep the two identical.

## Open

| id | question | cases | what the compiler does today (measured 2026-10-15 on the integrated compiler, soft runtime; recorded, not judged) |
|---|---|---|---|
| SQ-1 | A coordinate split over several loops with a **different mixed radix per operand** (`A[4*i0 + i1][k]` with `C[8*i0 + i1][j]`). The definition ("an injective affine function of the coordinate's loops") admits it; the "inside" list does not name it; the operand views are not strided sub-matrices, so a rank-2 `memref.subview` cannot express them. Inside the specification (lowered with a copy or a rank-3 view), or a documented limitation? | case_128, mlir_012 | refused at recognition (the two subscripts are not affine images of one linear form); case_128's program check passes — verdict `expected_refusal` |
| SQ-2 | An **empty GEMM** (a loop of zero iterations: M, N or K = 0). The nest computes nothing. Must the compiler offload it (zero invocations) or refuse it? Either keeps the result correct. | case_129, case_130, case_131, mlir_013 | recognized, then refused by the dense partition with its own reason, "a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)" (since 2026-10-15; before, with the message meant for symbolic bounds); the program's check passes — verdict `expected_refusal` |
| SQ-5 | a batched nest whose per-batch GEMM has two of M, N, K equal to 1 -- the reduction index in the output with a single-iteration j (C[i][k] += A[i][k] * B[k][j], j in [1, 2)): k is read by every operand (a batch coordinate), N = K = 1, so each batch is a column scaled by a scalar.  The result is correct, but the specification lists 'the reduction index in the output' as not a GEMM; is a batched vector-times-scalar a GEMM to offload, or a documented limitation? | `test/Systolic/affine_gemm_batched_lowering.mlir` @batch_scalar_rows; generated cases of the `output_in_reduction` mutation whose j loop runs once (`gen_04bff9d675c2`); no showcase case yet | refused by the dense partition ("a batched GEMM whose tiles are a vector scaled by a scalar (two of M, N, K are 1) is not lowered pending spec question SQ-5"), since ISSUE-006 (2026-10-15); before ISSUE-006 refused as a batched GEMM -- verdict `expected_refusal` |
| SQ-3 | A **symbolic loop bound** (`affine.for 0 to %n`: a GEMM extent known only at run time), unreachable from the C front end, whose bounds are constants. Is the MLIR path's refusal ("no static normalized extent") a documented limitation, or must dynamic extents be supported? | mlir_011 | recognized, refused by the dense partition ("the loop bounds are not static") — verdict `expected_refusal` |

Inspected again on 2026-10-15 with the batched-GEMM change (ISSUE-006); none of the three is
decided by it, and none is silently reclassified:

- **SQ-1** (`A[4 i0 + i1]` with `C[8 i0 + i1]`): unchanged -- refused at recognition
  (UNKNOWN: operand-specific decompositions of one coordinate). A batch coordinate is unrelated:
  it is read identically by all three operands.
- **SQ-2** (an empty loop): unchanged -- an empty *batch* loop makes the whole nest empty and is
  refused with the same empty-GEMM reason (`affine_gemm_batched_lowering.mlir` @batch_empty), not
  lowered to a loop of zero iterations around tiles. Deciding SQ-2 as "offload" would make an
  empty batch loop lower naturally (zero iterations of the re-emitted loop); deciding it as
  "refuse" keeps today's behaviour.
- **SQ-3** (a symbolic bound): unchanged -- refused by the dense partition. A symbolic *batch*
  bound would be lowerable by re-emitting the loop as it is (the batch loops need no static trip
  count for correctness), but the dense partition requires static bounds on every loop and the
  remark's repeat count would be unknown; left as is pending SQ-3.

## Decided

| id | question | decision | basis |
|---|---|---|---|
| SQ-4 | A constant logical coordinate (`C[0][j] += A[0][k] * B[k][j]`: M, N or K = 1 spelled with a constant subscript) | **Inside the specification: offloaded** as a GEMM with M, N or K = 1 (decided 2026-10-15 by the user, P0.5: "handle extent-one coordinates correctly"). A role no loop drives has extent 1; its operands read it through constant subscripts (a size-1 dimension of the view) or have no dimension for it (a unit dimension, memref.expand_shape). | docs/AFFINE_GEMM_MAPPING.md section 2; ISSUE-005; `failures/issues/ISSUE-003-*/repro_constant_coordinate.mlir` (now offloaded 1x8x16, 8x1x16, 16x8x1) |

The question as it was recorded while open:

| SQ-4 | A **constant logical coordinate** (`C[0][j] += A[0][k] * B[k][j]` with K split over two loops, so that the nest has three loops: I is the constant 0, driven by no loop; likewise a constant J or K). Arithmetically a GEMM with M (or N, or K) = 1. Is it inside the specification (offloaded as a 1×N×K GEMM) or a documented limitation? Found with ISSUE-003 (it crashed the compiler). | `failures/issues/ISSUE-003-*/repro_constant_coordinate.mlir` (lit; no showcase case yet) | recognized, then refused by the dense partition: "logical coordinate I is driven by no loop (a constant subscript)" — since the fix of ISSUE-003; before it, the compiler crashed |
