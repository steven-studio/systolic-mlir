# ISSUE-002 — the bounds arithmetic of the affine GEMM path overflowed int64

| field | value |
|---|---|
| Status | **FIXED** (2026-10-15, in the working tree on top of 6e25528, together with ISSUE-001) |
| Found by | the boundary review of this task (category "affine coefficients that may overflow"), probing with `showcase/negative/mlir_004.mlir`, `mlir_005.mlir`, `mlir_007.mlir`, `mlir_008.mlir` |
| Compiler commit | 6e25528 `Extend affine GEMM recognition and systolic offloading` |
| Component | `lib/Systolic/Transforms/AffineMatmulPartitioning.cpp` `getDenseLogicalGemm` (scale × extent, offset + scale × extent), `restrictToBox`; `lib/Systolic/Transforms/AffineMatmulToSystolic.cpp` `getLogicalOperand` (the operand's last index); `AffineMatmulAnalysis.cpp` `normalizeLogicalCoordinates` (T × step, T × origin) |
| Classification | **unsound check** — an out-of-bounds nest could pass the bounds test through wrapped-around arithmetic; at 6e25528 no case was found that was actually lowered wrongly (the Presburger exact-cover check caught the probe for another reason), so no FALSE POSITIVE was *observed*; the check itself was wrong |

## 1. The finding

`getLogicalOperand` decided "in bounds" from `last = offset + scale * (extent - 1)` in plain
`int64_t`. For `A[%i * 4611686018427387904, %k]` (scale 2^62) with 5 iterations, `last` is
2^62 × 4 = 2^64, which wraps to 0: `0 < 8`, "in bounds", for a nest whose second iteration
reads row 2^62 of an 8-row array. Signed overflow is undefined behaviour in C++ to begin with.
The same plain arithmetic computed the coordinate's values in `getDenseLogicalGemm`
(`expected *= extentOf[s]`, used for the dense mixed-radix test) and the half-open region
bounds in `restrictToBox` (`offset + scale * (lower + extent)`), and the composition of `T`
with the loop steps and origins in `normalizeLogicalCoordinates`.

Measured at 6e25528 (`scripts/run_showcase.py --case mlir_005` before the fix, and the probe in
the task log): `recognition result: success`, `dense logical GEMM: yes` (!), then
`fleet partition: not exact (the regions do not cover the source iteration domain)` — the
region arithmetic had overflowed too, and the exact-cover proof (which runs on the Presburger
domain) happened to fail. Refused, but for the wrong reason and by luck of a later check.

With ISSUE-001's per-operand views a second path became reachable: a small coordinate with an
operand view of offset 2^62 and stride 2^62 (`mlir_008`): the dense form is fine (scale 1,
extent 4), and only the operand's own last index overflows — which the old code would have
wrapped.

## 2. The fix

Checked arithmetic (`llvm::checkedMul` / `checkedAdd`, `llvm/Support/CheckedArithmetic.h`)
everywhere a product or sum of coefficients, steps, origins and extents is formed:

- `normalizeLogicalCoordinates`: `T × step`, `T × origin + c` — overflow → no normalized form
  (the nest is then refused downstream as "no static normalized extent");
- `getDenseLogicalGemm`: the mixed-radix products and the coordinate's exclusive end
  `offset + scale × extent` (what `restrictToBox` computes) — overflow → refused:
  *"logical coordinate I's scale or extent overflows 64-bit arithmetic"* / *"... values overflow
  64-bit arithmetic"*;
- `getLogicalOperand`: the operand's view offset, stride and last index — overflow → refused:
  *"A's subscripts along dimension 0 overflow 64-bit arithmetic"*.

A refusal is the only correct outcome: a nest whose subscripts do not fit in 64 bits cannot be
in bounds of any memref. Nothing representable changes: the control `@single_row_huge_stride`
(stride 2^62, extent 1: row 0 only; showcase case_117 / mlir_015) is still lowered, and ran
correctly on the software runtime model.

## 3. Reproducers and tests

- `repro_overflow.mlir` (this directory): `@wraps_to_zero`, `@end_overflows`, `@view_overflow`
  refused with the reasons above, `@single_row_huge_stride` lowered — a lit test of
  `check-systolic` (no XFAIL: written with the fix).
- showcase: `mlir_004`, `mlir_005`, `mlir_007`, `mlir_008` (negative, TRUE NEGATIVE, the reason
  checked), `mlir_006` (offset 2^62, representable, plain out of bounds), `mlir_015` and
  `case_117` (positive controls).

## 4. Specification

README.md section 2 does not need a new rule: an out-of-bounds nest is undefined as a program
and has no GEMM to lower; the lowering must refuse it and say so (`SPEC_BASIS["out_of_bounds"]`
in `generator/casespec.py`). The message for a loop of zero iterations ("the loop bounds are not
static ...") is a separate, cosmetic imprecision noted under spec question SQ-2.

## 5. Addendum 2026-10-15 (Mac/Linux integration): the recognizer's own arithmetic

An audit of every product and sum on the path, made while integrating the Mac and Linux fixes
of ISSUE-001, found arithmetic the fix above did not reach. Each could wrap around and let the
wrapped value take part in recognition, partitioning or lowering; each is now checked
(`llvm::checkedAdd` / `checkedSub` / `checkedMul`) or exact:

| where | what was plain int64 | now |
|---|---|---|
| `extractLinearForm` | the sum and the product of coefficients and constants | overflow → no linear form → not recognized |
| `normalizeLogicalCoordinates`, `recoverLogicalExtent` | the trip count `(upper - lower + step - 1) / step` | `checkedTripCount`: `upper - lower` checked, `(d - 1) / s + 1`; overflow → no normalized form / extent `unknown` |
| `getDenseLogicalGemm` | (reason only) | a static, nonempty nest without a normalized form is refused with *"the iteration domain's normalized form overflows 64-bit arithmetic"* instead of the message for symbolic bounds; an empty loop keeps the old wording (SQ-2) |
| `getDenseLogicalGemm` | the trip count `ub + 1` from the normalized domain | checked |
| `isInvertibleSquareMapping` | fraction-free elimination products (answered "not invertible" for `diag(2^62, 2^62, 2^62)`) | exact determinant in arbitrary precision (`presburger::IntMatrix`, `DynamicAPInt`) |
| `Rational` (`invertSquareMapping`, `transformDomainToLogical`) | every operation, the negation of `INT64_MIN` and its gcd | checked; an overflowing value is invalid and the (debug-only) inverse is then not printed — nothing is decided from it |
| `buildExactLogicalDomain` | the negation of `T` and `c` | checked; on overflow the optional exact logical domain is left unset |
| `restrictToBox` | the region bounds `offset + scale × (lower + extent)`, `c - first`, `next - 1 - c`, `-T` | checked; overflow → the partition is refused with the reason |
| `recognizeLogicalMatmul` | (new check) | every operand subscript must equal `scale × coordinate + offset` exactly, recomputed in checked arithmetic, before the views are used |
| `enclosingRepetitions` (report remark) | the product of trip counts | checked; past 64 bits the count is reported as unknown |

Regression test: `repro_overflow_analysis.mlir` (this directory; no XFAIL). It fails on the
compiler with only the fix of sections 2–3 (measured: both RUN lines fail) and passes with this
addendum: `@mul_overflow` (the wrapped constant 2^63 had become A's view offset and was refused
only because it happened to be negative), `@trip_overflow` (the trip count had wrapped to
`-9223372036854775807`), `@exact_rank` (the rank test answered "no" for a diagonal `T`), and the
control `@add_overflow` (MLIR merges its two terms and wraps them to an INT64_MIN coefficient, so
it reaches matchSharedCoordinate's INT64_MIN guard, not extractLinearForm's checked addition).

## 6. Follow-up 2026-10-15: diagnostics and parser compatibility

- **Overflow diagnostics.** The first version of section 5 reported "the iteration domain's
  normalized form overflows 64-bit arithmetic" whenever the Presburger source domain had constant
  bounds but no normalization was computed — also for `min(8, 16)` / `min(s0, 8)` bounds, a
  triangular nest and an empty loop `0 to INT64_MIN`, none of which overflows. Now
  `normalizeLogicalCoordinates` records whether a checked operation actually failed
  (`LogicalMatmul::normalizationOverflow`) and `getDenseLogicalGemm` reports an overflow on that
  flag alone; every other nest keeps the wording "the loop bounds are not static ...".
  Test: `repro_overflow_diagnostics.mlir` (fails on the heuristic build at its first
  `DISC-NOT: normalized form overflows`, measured).
- **INT64_MIN literal.** `-9223372036854775808` parses only with MLIR at or after llvm-project
  e803069070cf (2026-08-14); with an older MLIR the whole file fails to parse and every RUN
  line fails. The cases that need the literal (`@int64_min`, and the empty loop `0 to INT64_MIN`)
  moved to `repro_overflow_int64min_literal.mlir`, which states that minimum; the other tests of
  this directory use only literals in (-2^63, 2^63). Compatibility with an older MLIR was NOT
  tested (no such build exists here); only the parser source of e803069070cf's parent was read.
- **The guards themselves, literal-free.** `repro_overflow_guards.mlir` reaches an INT64_MIN
  coefficient as -2^62 i - 2^62 i (merged by MLIR into a constant `d0 * -9223372036854775808`):
  matchSharedCoordinate's INT64_MIN guard, Rational's INT64_MIN guard and
  buildExactLogicalDomain's checked negation each executed (gdb on a Debug build), and the exact
  determinant answers "invertible: yes" where the int64 elimination said "no". It also reaches
  extractLinearForm's checked addition with `(i + 2^62 - 1) * 2 + 2^62`, which MLIR leaves
  unsimplified.

Not fixed here, outside this compiler: MLIR's own affine-expression folding wraps around before
the pass sees the nest — `(i * 2^62) * 4` is parsed as `0`, `i * INT64_MAX + i` as
`i * INT64_MIN`. Such a subscript reaches the recognizer already wrapped and cannot be told
apart from one written that way.
