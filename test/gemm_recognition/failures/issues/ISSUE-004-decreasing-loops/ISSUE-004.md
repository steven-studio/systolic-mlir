# ISSUE-004 — loops that count down, and coordinates that decrease along their loop

| field | value |
|---|---|
| Status | **IMPLEMENTED** (2026-10-15, uncommitted, in `~/work/systolic-mlir-affine`) — a specification change made on the user's explicit request |
| Before | documented limitation (README.md §2: "a coordinate that decreases along its loop (negative stride)"); the C front end refused `>` / `>=` loop conditions ("the loop condition must be 'counter < bound' or 'counter <= bound'"), `getDenseLogicalGemm` refused a negative coefficient ("logical coordinate I decreases along its loop") |
| Components | `tools/systolic-cfront/systolic-cfront.c` (`for_stmt`, `for_trip_count`, `affine_index`); `lib/Systolic/Transforms/AffineMatmulPartitioning.cpp` (`getDenseLogicalGemm`); `include/Systolic/AffineMatmulPartitioning.h` (documentation) |
| Cases whose expectation changed | case_119, case_120, case_121, case_122 (negative/, stay there — files are never moved), ext_003: OUT_OF_SCOPE → ACCEPT. mlir_010 (one operand reversed against the other) stays OUT_OF_SCOPE |

## The change

**Front end.** `for (T i = LB; i > B | i >= B; --i | i-- | i -= S | i = i - S)` is translated.
`affine.for` only counts up (its verifier rejects a step of zero or below), so the loop becomes the
ordinal loop `affine.for %i = 0 to N` and every use of `i` the affine expression `LB - S * %i` —
the same values in the same order. The trip count is exact: `i >= B` runs
`N = LB >= B ? (LB - B) / S + 1 : 0` times, `i > B` is `i >= B + 1` (so `i = 5; i >= 0; i -= 2`
and `i = 5; i > 0; i -= 2` both run 5, 3, 1; `i = 6; i >= 0` runs 6, 4, 2, 0 and `i = 6; i > 0`
runs 6, 4, 2). Refused with their own diagnostic: a zero step ("must not be zero"), a step
against the condition's direction (`i < 8; i--`, `i > 0; i += 2`), and a counter whose last
decrement would leave its type (an `unsigned` counter with `i >= 0`, or one stepping past 0).
`for_trip_count` (the GEMM-site finder of `--explain`) counts the same way.

**Dense partition.** A loop along which a coordinate decreases (a negative coefficient — the
front end's `LB - S * t`, or a reversed subscript such as `7 - i`) enumerates the same values in
the opposite order: with z in [0, n), `T z = T (n - 1) + |T| (n - 1 - z)`. `getDenseLogicalGemm`
reads such a loop backwards: the coordinate is `offset' + scale * ordinal` with `offset'` the
smallest value and `scale = |T|` (checked arithmetic; `|INT64_MIN|` is refused as an overflow).
Each operand's view is built from the coordinate's values, so the lowering is unchanged.

## Why it is correct

The set of (I, J, K) points and every product are the same; only the order in which the loop
nest visits them differs. For I and J that order is immaterial (each point accumulates into its
own C element). For K it is the order of the sums — which the systolic lowering already changes
for every GEMM (AffineMatmulToSystolic.cpp, header: "floating-point results equal the loop's only
when the arithmetic is exact"); a reversed K loop is the same reassociation.

Still out of scope: two operands that run through a shared coordinate in opposite directions
(`A[7 - i]` with `C[i]`) — no strided view of one matches the other; `matchSharedCoordinate`
refuses it at recognition (mlir_010, `negative_scale`).

## Tests (lit, check-systolic)

- `test/CFront/downward.c` — the emitted loops and index expressions for `>=` / `>`, steps that do
  and do not divide the distance, `--i`, `i--`, `i = i - S`, a downward counter in a subscript next
  to an upward one, empty loops, an unsigned counter reaching 0 exactly, the unchanged upward form.
- `test/CFront/downward_unsupported.c` — zero step (both directions), step against the condition
  (both directions), unsigned counter wrapping (`>= 0`, and `> 0` skipping 0).
- `test/CFront/unsupported.c` — `DOWN` now checks a downward condition with an upward step;
  `NE` checks the new wording of the condition diagnostic.
- `test/Systolic/affine_gemm_loop_steps.mlir` — the front end's forms at the MLIR level: views and
  GEMM extents for rows 5,3,1 / 6,4,2 / 6,4,2,0, a reversed reduction, a split coordinate with a
  reversed fine loop, empty loops (refused as empty GEMMs, not as decreasing), opposite directions
  (refused).
- `test/Systolic/affine_gemm_loop_steps_invalid.mlir` — `affine.for` with step 0 and step -2 are
  rejected by the IR verifier.
- `test/SystolicCC/downward.c` — end to end: nine GEMMs with loops that count down (and an upward
  control) offloaded, every result equal to an independent reference that spells out the visited
  indices, on the software model and in mock-hardware mode; `--verify` bitwise; the empty loop
  stays on the host.
- `failures/issues/ISSUE-002-*/repro_overflow_guards.mlir` — `@min_coef_identical` (coefficient
  INT64_MIN on both operands) is now refused because reading it backwards needs `|INT64_MIN|`.
- showcase case_119–122, ext_003 — offloaded, program check against the Python oracle.
