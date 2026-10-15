# NOTE-001 — three "false positives" of the loop were a specification refinement, not compiler errors

| field | value |
|---|---|
| Status | **RESOLVED 2026-10-15 — the specification and the predictor were refined; the compiler was right** |
| Found by | `scripts/loop.py`, batches `enumerate:enum` (indices 106, 107) and `random:777 --family mutation` (index 47), runtime soft |
| Records | `failures/resolved/20261015-014240_gen_06c950c874b9/`, `…_gen_55eeb58cd4f7/`, `failures/resolved/20261015-014340_gen_d55b7b137c7c/` (full programs, every stage's output, minimized variant) |
| Compiler commit | 6e25528, untouched |

## What the loop reported

Three generated nests of the `repeat_inside_K` mutation — a repetition loop `rr` (2 iterations)
innermost, *inside* the reduction loop — were **offloaded**, with correct results (the program's
own check: kernel == references, exact; `systolic-cc --verify`: verified; invocations = plan).
The specification in force said OUT_OF_SCOPE ("a repetition loop inside the reduction cannot be
moved outward"), so `verdict.py` flagged `false_positive`. Example (`gen_06c950c874b9`):

```c
for (int k = 0; k < 1; ++k)            // the reduction loop has ONE iteration
  for (int i = 2; i < 3; ++i)
    for (int j = 0; j < 1; ++j)
      for (int rr = 0; rr < 2; ++rr)   // the repetition loop inside it
        C[i + 2][j] += A[i + 2][k] * B[k][j];
```

The third case (`gen_d55b7b137c7c`, minimized to `i < 2 step 2, j in [3,4), k in [1,2), rr < 2`)
has the same shape: `k` runs once.

## Analysis

`hoistRepetitionLoops` does not refuse "a repetition loop inside the reduction" as such; it asks
`affine::isValidLoopInterchangePermutation` whether moving the repetition loop outward reorders
dependent iterations. With a reduction loop of a single iteration there is nothing to reorder —
for every C element the accumulation order is unchanged — so the interchange is legal, the loop
is hoisted, the nest becomes `C += 2 · A B` with K = 1, a valid GEMM, and it is offloaded. The
results are exact (verified). The compiler behaved correctly; the specification's wording (and
`spec_expect()` / `predict()`, which encoded it) was coarser than the documented rule.

## What changed

- README.md §2 and `casespec.SPEC_BASIS["repeat_in_reduction"]`: the exclusion is "a repetition
  loop of more than one iteration inside a reduction loop of more than one iteration" (the case
  where the interchange is illegal), as the sources say.
- `casespec.predict()` and `casespec.spec_expect()`: the same trip-count condition.
- The three cases are TRUE_POSITIVE; their corpus entries (`regression/corpus/`) were
  regenerated with the refined classification and they stay in the corpus (the compiler must
  keep offloading them correctly). ext_004 (`k` of 8 iterations, `rr` of 2) is unchanged:
  DOCUMENTED_LIMITATION, refused as before.

This is the one situation in which an expectation may change: the specification was shown to be
wrong by the mathematics and the documented rule, with the compiler's output verified exact
against independent references. It is recorded here so that it is never mistaken for "the test
was made to pass".
