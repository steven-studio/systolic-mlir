# ISSUE-001 — the recognizer rejects a GEMM whose operands spell a shared coordinate differently

| field | value |
|---|---|
| Status | **FIXED** (2026-10-15, in the working tree on top of 6e25528; see section 11) — the former XFAIL reproducers in this directory are now permanent regression tests |
| Found by | showcase case_065 (`showcase/positive/case_065.c`), 2026-10-15 |
| Also reproduced by | case_094 (`C[2*i][j]`), ext_008 (`A[i+2][k]`), ext_007 (`A[k][j+1]`), variants v1, v3–v7 in `variants.mlir` |
| Compiler commit | 6e25528 `Extend affine GEMM recognition and systolic offloading` |
| Component | `lib/Systolic/Transforms/AffineMatmulAnalysis.cpp`, `recognizeLogicalMatmul`, the `matchesGemmAccesses` lambda (lines 1286–1306) |
| Classification | **FALSE NEGATIVE** of the recognizer (a valid GEMM inside the documented support envelope is refused) |

## 1. The case

`showcase/positive/case_065.c` (kept exactly as first written; the complete program with its
references is that file). The kernel:

```c
void case_065_kernel(float A[restrict 8][8], float B[restrict 8][8], float C[restrict 32][32]) {
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      for (int k = 0; k < 8; ++k)
        C[i + 8][j + 16] += A[i][k] * B[k][j];
}
```

- loop bounds: `i, j, k ∈ [0, 8)`, step 1, perfect nest, one multiply-accumulate;
- arrays: `A: float[8][8]`, `B: float[8][8]`, `C: float[32][32]`;
- subscripts reached: `A[0..7][0..7]`, `B[0..7][0..7]`, `C[8..15][16..23]` — every one in bounds,
  the program is valid C with no undefined behaviour;
- data: integers in `[-3, 3]`, `C` non-zero before the call; every partial sum is an integer far
  below 2^24, so the float result is exact.

| | |
|---|---|
| Mathematical GEMM | **YES** |
| Expected recognition (specification) | **ACCEPT** |
| Current recognition (commit 6e25528, measured) | **REJECT** (`recognition result: failure`, no remark) |
| Classification | **FALSE NEGATIVE** |
| Lowering support | **to be verified** once recognized (see §5: the lowering also assumes one shared view per coordinate) |
| FPGA execution support | **to be verified** once lowered |

## 2. Independent verification of the semantics

With `r = i + 8` and `c = j + 16` the statement is, for every `(r, c) ∈ [8,16) × [16,24)`,

    C[r][c] += Σ_{k=0}^{7} A[r − 8][k] · B[k][c − 16]

i.e. `C[8:16, 16:24] += A · B`: the 8×8×8 product of `A` and `B`, accumulated into the
sub-matrix of `C` at row offset 8 and column offset 16. Logical coordinates `I = i`, `J = j`,
`K = k`, each enumerated once by its own loop; operand views `A'(I,K) = A[I][K]`,
`B'(K,J) = B[K][J]`, `C'(I,J) = C[I+8][J+16]`, each an injective affine function of its
coordinates. The constant offsets on `C` change where the result lands, not the operation.

Three independent references agree, and the kernel as written (compiled by gcc 13 and clang 18
with `-Wall -Wextra -Werror`, no warnings) agrees with them — from
`reports/showcase_cpu_20261015-001035.json` (plain C compiler, no systolic-cc):

```
[gemm_recognition] case_065 subblock_c_larger: positive / sub_matrix, class gemm
[gemm_recognition] references: direct and logical-view agree (1024 elements)
[gemm_recognition] references: checksum 1837 matches the Python oracle
[gemm_recognition] kernel: equal to the reference (1024 elements, exact), checksum 1837
RESULT case_065 PASS
```

- reference 1 (`reference_direct`): the statement over the same loop domain on flat arrays
  with explicit row-major offsets, the product through a function call;
- reference 2 (`reference_logical`): gather `A'`, `B'`, run a plain 8×8×8 GEMM, scatter into
  `C[8+I][16+J]` — this reference *is* the claim "C[8:16,16:24] += A·B";
- reference 3: the checksum of the result computed in Python (`generator/casespec.py`,
  exact integers) at generation time.

The case therefore satisfies "mathematically legal GEMM, index legality, reduction dimension,
output coordinates" in full; nothing about it is outside the specification (README.md,
*Support specification*: offsets and strides on operand views are inside it; the headers of the
compiler itself say so — `AffineMatmulPartitioning.h`: "nonzero origins, access offsets and
strides ... are all of this form"; `AffineMatmulAnalysis.h`: one factor map `phi` per operand).

## 3. What the compiler actually does (measured)

The front end translates the kernel (clang 18 JSON AST, `--clang-major 18`, container):

```mlir
affine.for %i = 0 to 8 {
  affine.for %j = 0 to 8 {
    affine.for %k = 0 to 8 {
      %t0 = affine.load %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
      %t1 = affine.load %A[%i, %k] : memref<?x8xf32>
      %t2 = affine.load %B[%k, %j] : memref<?x8xf32>
      %t3 = arith.mulf %t1, %t2 : f32
      %t4 = arith.addf %t0, %t3 : f32
      affine.store %t4, %C[(%i + 8), (%j + 16)] : memref<?x32xf32>
```

`--systolic-declare-fpga-fleet --test-affine-matmul-discovery` on it (recognizer, partitioning
and discovery pass of 6e25528 compiled unchanged; see *Environment* below):

```
candidate count: 1
recognition result: failure
unique logical matmul count: 0
```

The discovery pass prints no reason: `recognizeLogicalMatmul` returns `failure()` silently, and
`--affine-matmul-to-systolic` emits no remark for an unrecognized nest (remarks exist only for
recognized-but-not-lowered nests), so `systolic-cc --explain` reports only
`'case_065_kernel' stays on the host: no GEMM the systolic arrays can run was recognized in it`.

## 4. Root cause (traced in the source)

`recognizeLogicalMatmul` (`lib/Systolic/Transforms/AffineMatmulAnalysis.cpp`):

1. The structural checks pass: perfect three-loop chain, three loads and one store, the store
   value is `addf(load C, mulf(load A, load B))`, the accumulator is loaded and stored through the
   same map, every loop's IV is used (lines 1045–1228).
2. `normalizeAccessMap` rewrites each access map over the common iteration dims
   (`d0 = i, d1 = j, d2 = k`): `A = (d0, d2)`, `B = (d2, d1)`, `C = (d0 + 8, d1 + 16)`.
3. Lines 1286–1306 decide whether the three maps form a GEMM:

```cpp
  auto matchesGemmAccesses =
      [&](AffineMap a, AffineMap b) {
        ...
        AffineExpr i = a.getResult(numPartitionDims);
        AffineExpr k = a.getResult(numPartitionDims + 1);
        AffineExpr j = b.getResult(numPartitionDims + 1);

        return i == normalizedAcc->getResult(numPartitionDims) &&
               j == normalizedAcc->getResult(numPartitionDims + 1) &&
               k == b.getResult(numPartitionDims) &&
               i != j && i != k && j != k;
      };

  if (!matchesGemmAccesses(accessA, accessB)) {
    if (!matchesGemmAccesses(accessB, accessA))
      return failure();                                  // <- case_065 leaves here
```

`AffineExpr::operator==` is identity of the (uniqued, canonicalised) expression. The lambda
therefore requires the **same affine expression** — not the same logical coordinate — in the two
operands that share it: `A`'s row expression must be `C`'s row expression, `B`'s column
expression must be `C`'s column expression, `A`'s column expression must be `B`'s row expression.
For case_065, `d0 ≠ d0 + 8` for `I` (and `d1 ≠ d1 + 16` for `J`), both operand orders fail, and
the nest is declared "not recognized".

The condition conflates two different things:

- *which loops* drive a coordinate — what makes the nest a GEMM (A's row and C's row must be
  the same function **up to each operand's own injective affine view**);
- *how each operand addresses* that coordinate — the per-operand factor map `phi_A`, `phi_B`,
  `phi_C` that the header `AffineMatmulAnalysis.h` itself defines as separate maps.

The implementation collapses the second into the first: after the match it sets
`phiA/phiB/phiC` to the access results themselves (lines 1420–1464, "derived from the recognized
access structure rather than rediscovered independently"), so `phi = identity` relative to one
shared expression per coordinate is the only form it can represent.

`ext_009` (`A[i + i][k]` vs `C[2 * i][j]`) is **accepted** — MLIR canonicalises both to `d0 * 2`
— which confirms that the rule is expression identity after canonicalisation, nothing about
spelling.

## 5. Empirical isolation (`variants.mlir`, same tool, same commit)

| variant | C | A | B | recognizer at 6e25528 | by the specification |
|---|---|---|---|---|---|
| v1 = case_065 | `[i+8][j+16]` | `[i][k]` | `[k][j]` | failure | ACCEPT |
| v2 control | `[i+8][j+16]` | `[i+8][k]` | `[k][j+16]` | success, dense, 1 exact region | ACCEPT |
| v3 | `[i+8][j]` | `[i][k]` | `[k][j]` | failure | ACCEPT |
| v4 | `[i][j+16]` | `[i][k]` | `[k][j]` | failure | ACCEPT |
| v5 | `[i][j]` | `[i][k+1]` | `[k+2][j]` | failure | ACCEPT |
| v6 = case_094 | `[2i][j]` | `[i][k]` | `[k][j]` | failure | ACCEPT |
| v7 = ext_008 | `[i][j]` | `[i+2][k]` | `[k][j]` | failure | ACCEPT |

v2 shows the offsets themselves are fine (the same nest with the offsets copied onto A and B is
recognized, partitioned and planned); v3–v7 show that *any* difference in the shared
coordinate's expression — offset or stride, on C, on A, or on the reduction between A and B —
triggers the rejection. The rule, not the arithmetic, is what refuses case_065.

## 6. Consequences downstream (why "lowering support: to be verified")

Even with the recognizer fixed, two later stages assume one shared view per coordinate:

- `getDenseLogicalGemm` (`AffineMatmulPartitioning.cpp`) derives one `(offset, scale)` per
  coordinate from `T` and `c`;
- `getLogicalOperand` (`AffineMatmulToSystolic.cpp`, lines 134–167) builds the `memref.subview`
  of **every** operand from `dense.offset[d]` / `dense.scale[d]` of the coordinate — with
  per-operand views, `C`'s subview must be `[8, 16] [8, 8] [1, 1]` while `A`'s is `[0, 0] [8, 8]
  [1, 1]`; today the same `(offset, scale)` would be applied to both.

A fix must carry each operand's own `(offset, scale)` per coordinate from the recognizer
(`phi_A`, `phi_B`, `phi_C` as real factor maps) into the lowering's view construction. The
partitioning itself is unaffected: the logical coordinates `(I, J, K)` and the iteration domain
stay what they are; only the operand views change. The runtime path is unaffected: a tile reads a
contiguous sub-block of each view exactly as for case_063 (interior blocks with the offsets on
all three operands).

## 7. Fix plan (proposal — nothing implemented; needs the user's approval)

In `matchesGemmAccesses`, replace *expression identity* by *same coordinate*:

1. two expressions address the same coordinate iff they are linear forms over the **same set of
   loop IVs** (identical support), each with positive coefficients, and that set is disjoint from
   the sets of the other two coordinates (the `i != j != k` distinctness becomes disjoint
   supports);
2. the logical coordinate `I` (row of `T`) is defined by the loops themselves — e.g. the ordinal
   enumeration of its loop set — and each operand records its own factor map
   `phi_op(I) = offset_op + scale_op · ordinal` (a dense, mixed-radix check per operand, exactly
   the check `getDenseLogicalGemm` does once today);
3. `LogicalMatmul` gains per-operand `(offset, scale)` per coordinate; `getLogicalOperand` uses
   the operand's own pair; `getDenseLogicalGemm`'s dense check runs per operand.

What must **not** change (the negatives and limitations the new rule has to keep refusing; all
in the showcase and re-run by `scripts/run_showcase.py`):

| must stay refused | why the proposed rule still refuses it |
|---|---|
| case_081 / 082 element-wise | A's second subscript's loop set equals J's (081) or I=J=K (082): supports not disjoint |
| case_087 coupled `C[i+j][j]` | C's row support `{i, j}` overlaps C's column support `{j}` |
| case_088 skewed output `C[2i+j][j]` | C's row support `{i, j}` ≠ A's row support `{i}` |
| case_089 / 090 singular | supports overlap / A's row support `{j, k}` ≠ C's `{i}` |
| case_095 `C[i][k]` | C's column support `{k}` equals K's support |
| case_097 no accumulation, 096 alpha, 098 subtract, 099 init inside | unchanged earlier checks (statement form, perfect chain) |
| case_091–093 transposed | positional (I,K)/(K,J)/(I,J) convention unchanged: A's row support `{k}` ≠ C's row support `{i}` |
| case_085 / 086 batched | unchanged: partition prefix handled as today, rank mismatch as today |
| ext_003 reversed rows | negative coefficient: refused by rule 1 (or later by the dense check) |
| ext_007 `A[i][k]·A[k][j+1]` | recognized by the new rule, then refused by the existing "different views of the same memref" check — the correct stage |

## 8. Verification plan (after a fix, before anything is called fixed)

1. `scripts/run_issue.sh ISSUE-001`: `variants.mlir` v1, v3–v7 must become `success` with
   `dense logical GEMM: yes` and an exact fleet partition; v2 unchanged.
2. `repro_case_065.mlir` (the lit XFAIL test in this directory): it must XPASS; then remove its
   `XFAIL` line — it becomes a permanent regression test.
3. `scripts/run_showcase.py --runtime soft` then `--runtime board`: case_065, case_094, ext_008
   must reach `driver = offloaded`, the program's own check must print `RESULT ... PASS`
   (kernel == references, exact), `systolic-cc --verify` must print `verified`, and the
   invocation count must equal the plan. After the fix case_094 and ext_008 are TRUE
   POSITIVES and move to the positive set.
4. `cmake --build build --target check-systolic`: the existing 38 tests (plus this XFAIL) unchanged.
5. The negatives above: every one still refused at the same stage with the same reason
   (`run_showcase.py` compares each stage with `manifest.json`).
6. The generated corpus (`scripts/loop.py`): no new failures on the regression corpus.

## 9. Environment of the measurements in this record

- recognizer, partitioning and discovery pass of commit 6e25528, compiled unchanged in the
  cloud container against LLVM/MLIR 18 (`/usr/lib/llvm-18`) into a stop-gap driver
  (`discovery-opt`: the dialect + `--test-affine-matmul-discovery` + `--systolic-declare-fpga-fleet`);
  the repository's own 17 discovery lit tests pass through it unchanged;
- C front end: `build/bin/systolic-cfront` driven by clang 18.1.3 with `--clang-major 18`
  (the project standardises on clang 24; the AST JSON of clang 18 is accepted by the front end
  for these programs);
- to be re-run on the LLVM 24 build (in progress in the container) and on the Linux board
  machine with clang 24: `scripts/run_issue.sh ISSUE-001` writes `actual_<host>_<date>.txt` here.

## 10. Log

- 2026-10-15 00:0x — case_065 first classified away from the positive set by the generator's
  predictor; reverted: the expectation is set by the mathematics, not by the implementation.
- 2026-10-15 00:2x — rejection reproduced with the real recognizer code; root cause traced to
  `matchesGemmAccesses`; variants v1–v7 isolate the rule; this record written.
  Compiler untouched.
- 2026-10-15 08:xx — fixed (section 11); case_065, case_094, ext_008 offloaded and verified
  on the software runtime model; `variants.mlir` and `repro_case_065.mlir` un-XFAILed;
  check-systolic 74/74.

## 11. The fix (implemented after the user allowed compiler changes)

**Root cause, restated.** `matchesGemmAccesses` compared the two operands' index *expressions*
for a shared coordinate with `AffineExpr ==`. A coordinate is a linear form of the loop
counters; an operand may read it through its own affine view (`C[i + 8][j]` with `A[i][k]`,
`C[2i][j]` with `A[i][k]`), and the pass that builds the operand views (`getLogicalOperand`)
assumed one view per coordinate, so the recognizer had to refuse everything else.

**Change** (three files, no new pass, no change to partitioning policy or code generation):

1. `lib/Systolic/Transforms/AffineMatmulAnalysis.cpp` — `matchSharedCoordinate(a, b)`: two
   expressions address the same coordinate when they are identical (as before: the coordinate is
   that expression, both views are the identity, so `T`, `c` and every existing test are
   unchanged) or when both are `s * g + o` for one linear form `g` over the loop IVs with integer
   `s > 0` — then the coordinate is the primitive form `g` (coefficients divided by their gcd, no
   constant) and each operand records its view `(s, o)`. Expressions over different loop
   combinations (`4 i0 + i1` vs `8 i0 + i1`, spec question SQ-1), of opposite direction (`i` vs
   `7 - i`, the documented negative-stride limitation) or without a loop are not one coordinate.
   `recognizeLogicalMatmul` uses it for I (A/C), J (B/C) and K (A/B); I, J, K must still be three
   different expressions. `LogicalMatmul` gains `viewA/viewB/viewC` (`OperandCoordinateView`:
   scale, offset, per coordinate of the operand) and the factor maps `phiA/B/C` are now what the
   header always documented: maps over the logical coordinates `(P*, I, J, K)` whose results are
   the operand's views (`F_A = phiA o (P, I, K)`).
2. `lib/Systolic/Transforms/AffineMatmulToSystolic.cpp` — `getLogicalOperand` builds each
   operand's `memref.subview` from its own view: offset `view.offset + view.scale * dense.offset`,
   stride `view.scale * dense.scale`, size `dense.extent`; the bounds check uses the operand's own
   last index. A view that would decrease (scale <= 0) is refused with its own message (a
   `memref.subview` has no negative stride) — unreachable today, since the recognizer gives the
   primitive form the sign.
3. `lib/Systolic/Transforms/TestAffineMatmulDiscovery.cpp` — prints `view A/B/C: [s*I + o, ...]`
   so tests can check the views.

Done in the same change, because the new views made the arithmetic reachable: the overflow
hardening of ISSUE-002 (checked 64-bit arithmetic in `normalizeLogicalCoordinates`,
`getDenseLogicalGemm` and `getLogicalOperand`).

**What it does not change.** Shared expressions keep exactly the previous `T`/`c` representation
(`affine_gemm_access_stride.mlir`: `T[0] = [8 0 0]`, `c = [5 0 0]`, unchanged); the dense
partition, the tile plan, the exact-cover proof, the runtime path and `systolic-cc` are untouched.
Every refusal in section 7's table still happens, at the same stage, with the same reason
(measured: `reports/showcase_soft_*`, all of case_081–100 and ext_003–006 pass as TRUE
NEGATIVE / DOCUMENTED LIMITATION); ext_007 (`A[i][k] · A[k][j+1]`) is now refused at the
correct stage, the lowering's "A and B are different views of the same memref".

**Verification (section 8, executed).**

1. `variants.mlir`: v1–v7 all `recognition result: success`, dense, 1 exact fleet region each,
   the views as expected (`view C: [1*I + 8, 1*J + 16]` for v1, `[2*I + 0, 1*J + 0]` for v6,
   `view A: [1*I + 2, 1*K + 0]` for v7); `XFAIL` removed, the views added as `VIEW:` checks.
2. `repro_case_065.mlir`: XPASS, then `XFAIL` removed; its `SYS` check had named the argument
   `%C` where the printer says `%arg2` (a bug in the test written before the fix), corrected to
   `%{{.*}}[8, 16] [8, 8] [1, 1]`.
3. `scripts/run_showcase.py --runtime soft`: case_065, case_094, ext_008 → `driver = offloaded`,
   1 invocation (= the plan), `verified: N elements equal the CPU reference (bitwise)`,
   `RESULT ... PASS`. The board (`--runtime board`) has not been run from here: the software
   runtime model is not the FPGA.
4. `cmake --build build --target check-systolic`: 74/74 (the 72 existing tests unchanged, the
   two former XFAILs pass).
5. The negatives: every one still refused at the same stage with the same reason (the
   showcase run compares each stage with `manifest.json`; 160/160 cases pass or are open
   spec questions, `reports/latest_soft.json`).
6. The generator: `scripts/loop.py --family views` (the family built to track this issue, 100%
   known false negatives before) — 12/12 and then 120/120 pass; mixed / enumerate / boundary
   batches: no new failures (`generated/batch_random_fix*`, `generated/coverage.json`).

The predictions in `generator/casespec.py: predict()` follow the new rule (`shared_coordinate`,
mirroring `matchSharedCoordinate`), and `generator/make_showcase.py --force` regenerated the
headers of the 11 affected case files (case_065, case_094, ext_007, ext_008 and the seven
negatives whose predicted reject-reason wording changed); no kernel, reference or expectation
changed. case_094 stays in `showcase/negative/` — files are never moved — with its header saying
it is a TRUE POSITIVE since this fix.
