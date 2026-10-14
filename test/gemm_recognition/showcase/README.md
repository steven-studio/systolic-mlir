# GEMM recognition showcase set

One hundred fixed, human-readable C programs that exercise the compiler's GEMM recognizer
(`recognizeLogicalMatmul`), its dense-partition view (`getDenseLogicalGemm`), the lowering
(`--affine-matmul-to-systolic`) and the driver (`systolic-cc`), each with its own independent
CPU references — plus a few extended cases outside the hundred.

```
showcase/
├── positive/case_001.c .. case_080.c   80 nests that ARE dense GEMMs the specification requires
│                                        to be recognized, partitioned, lowered and offloaded
├── negative/case_081.c .. case_100.c   20 nests the compiler at 6e25528 is expected NOT to accept;
│                                        each one says whether it is nevertheless a valid GEMM
├── extended/ext_001.c .. ext_014.c     further forms (front-end subset, aliasing, reversed rows,
│                                        a repetition inside the reduction, probes), same tooling
├── manifest.json                       every case: loops, arrays, subscripts, mathematical class,
│                                        the specification's expectation, the current
│                                        implementation's predicted behaviour, the classification
├── CASES.md                            the same as a table
└── README.md                           this file
```

The files are generated once from `generator/showcase_cases.py` by `generator/make_showcase.py`
and then kept fixed (`make_showcase.py --check` verifies they are unchanged). They are not lit
tests (the directory's `lit.local.cfg` hides them from `check-systolic`); they are run by
`scripts/run_showcase.py`, which compares every stage with `manifest.json` and writes the actual
results to `reports/`.

## What every file contains

1. A header comment: case id, name, set / category, mathematical class, the operation over its
   loop domain, the logical GEMM view (`C'(I,J) += A'(I,K) B'(K,J)`, which loops enumerate I, J,
   K, how each operand addresses them), array dimensions, loop order and roles, index formulas,
   the statement form, the element type; then **Mathematical GEMM**, **Expected recognition
   (specification)** with its basis, the **current implementation's** predicted result per stage,
   and the **classification**; the references it carries; how to run it; the exit codes.
2. The **kernel**: the loop nest under test, the only function in the affine C subset.
3. **Data**: small integers in [-3, 3] from the same formula as the Python oracle; `C` starts
   non-zero. Every partial sum is an integer far below 2^24, so the float results are exact and
   compared for exact equality — no tolerance anywhere.
4. **Reference 1 (`reference_direct`)**: the statement over the same loop domain on flat arrays
   with explicit row-major offsets, the product through a function call. It shares no index
   code with the kernel and is not a GEMM statement for the compiler's site finder.
5. **Reference 2 (`reference_logical`)**, for every nest that *is* a GEMM (positive or not):
   gather `A'`, `B'` through each operand's own index map, run a plain GEMM, scatter into `C'`.
   This reference *is* the claim "this nest computes a GEMM of these views".
6. **Reference 3**: `EXPECTED_CHECKSUM`, the result's checksum computed in Python at generation
   time (`generator/casespec.py`, exact integers).
7. `main`: checks the references against each other and against the oracle **before** calling
   the kernel (a disagreement is `RESULT ... FAIL (test bug)`, exit 3 — a bug in the test, never
   blamed on the compiler), calls the kernel once, reports the runtime's invocation count when
   built by `systolic-cc` (`-DGEMM_RECOGNITION_STATS`), and prints `RESULT case_NNN PASS|FAIL`.

## The two layers of expectation

`manifest.json` keeps two things apart, and so does every header:

- **`expected`** — the specification: set by the mathematics of the nest and by the support
  specification (`../README.md`, *Support specification*), **never by what the compiler
  prints**. `recognition` is `ACCEPT` (a dense GEMM inside the envelope), `REJECT` (not a dense
  GEMM of these operands) or `OUT_OF_SCOPE` (a valid GEMM the specification excludes
  explicitly — a documented limitation); `basis` names the source of the rule.
- **`current_implementation`** — what commit 6e25528 is *predicted* to do at each stage, from
  reading its sources (`generator/casespec.py: predict()`). `scripts/run_showcase.py` measures
  the real behaviour and compares with both layers.

The **classification** is the comparison of the two:

| classification | meaning |
|---|---|
| TRUE_POSITIVE | spec ACCEPT, implementation offloads |
| FALSE_NEGATIVE | spec ACCEPT, implementation refuses — a compiler issue (`failures/issues/`) |
| TRUE_NEGATIVE | spec REJECT (not a GEMM), implementation refuses |
| DOCUMENTED_LIMITATION | a valid GEMM the specification excludes explicitly; implementation refuses |
| FALSE_POSITIVE | spec REJECT, implementation offloads — must never happen |

A refusal is therefore never read as "not a GEMM": `mathematical_gemm` / `is_valid_gemm` says
what the nest is, `classification` says what the compiler does about it.

## The sets

**positive/ (80)**: dense GEMMs — every loop order; `<`, `<=`, `++`, `i++`, `+= S`, `i = i + S`,
`int` and `long` counters; shapes from 1×1×1 to 32×32×32 with edge tiles and K folding (K = 17,
33, 40, 64); non-zero loop origins (also with negative constants that cancel); loop steps 2 and 3;
access offsets; access strides 2 and 3; offsets and strides together; coordinates split over two
or three loops in any order (tiled nests, radix 3, the fine loop outside the coarse one);
repetition loops outside the nest and between its loops (hoisted by the lowering); the GEMM
inside larger arrays (top-left block, interior block, strided block, three differently sized
arrays); `A × A`; brace and pointer-to-array syntax; and combinations of all of these.
79 are TRUE_POSITIVE; **case_065** (`C[i+8][j+16] += A[i][k] B[k][j]`, the offsets on C only)
is a **FALSE_NEGATIVE**: a valid GEMM the recognizer refuses because it requires the same
affine expression for a shared coordinate in both operands — see
`../failures/issues/ISSUE-001-…/ISSUE-001.md`. It stays in the positive set with expectation
ACCEPT and fails the run until the recognizer is fixed.

**negative/ (20)**: expected not to be accepted by the compiler at 6e25528:

| cases | what | valid GEMM? | classification |
|---|---|---|---|
| 081, 082 | element-wise products (2-D with an unused loop, 3-D) | no | TRUE_NEGATIVE |
| 083, 084 | matrix-vector products (plain, with repetition loops) | a GEMV: the N = 1 degenerate GEMM, two index dimensions as written | DOCUMENTED_LIMITATION |
| 085, 086 | batched GEMM (rank-3 arrays; shared B) | yes | DOCUMENTED_LIMITATION |
| 087 | coupled `C[i+j][j]` | GEMM elements over a parallelogram, not a rectangular GEMM | TRUE_NEGATIVE |
| 088 | the seed `A[2i+3][k+1] B[k+2][3j+2] C[2i+j][j]` | GEMM values, skewed placement | TRUE_NEGATIVE |
| 089, 090 | singular mappings (the repo's test; the seed `A[2j+k][p]`) | no | TRUE_NEGATIVE |
| 091, 092, 093 | transposed A, B, C | yes | DOCUMENTED_LIMITATION |
| 094 | `C[2i][j] += A[i][k] B[k][j]` (stride on C only) | yes | **FALSE_NEGATIVE** (ISSUE-001) |
| 095 | reduction index in the output `C[i][k]` | no | TRUE_NEGATIVE |
| 096, 098 | `alpha A B`, `C -= A B` | yes (alpha ≠ 1) | DOCUMENTED_LIMITATION |
| 097 | no accumulation `C = A B` | no (an outer product of the last column/row) | TRUE_NEGATIVE |
| 099 | `C = 0` inside the nest (beta = 0) | yes | DOCUMENTED_LIMITATION |
| 100 | double precision | yes | DOCUMENTED_LIMITATION (driver: fp32 datapath) |

**extended/ (14)**: scalar accumulator (front end), no `restrict` (aliasing), reversed rows
(negative scale), a repetition loop inside the reduction, a three-factor product, a sum, `A × A`
with two different views, an offset on A only (ISSUE-001), equal expressions spelled differently
(a probe — accepted), mixed float/double operands, two batch coordinates, eight loops, a reduction
loop with `<=`, step and origin, and the batches of case_086 stacked into one GEMM.

## Running

```
# the kernel as written against its references, plain C compiler (no systolic-cc):
cc -std=c11 -O1 showcase/positive/case_001.c -o case_001 && ./case_001

# everything, stage by stage, through the compiler (software runtime model, NOT the FPGA):
python3 scripts/run_showcase.py --runtime soft
# on the board (authoritative):
SYSTOLIC_FPGA_CONFIG=<the programmed bitstream's fpga_config.txt> python3 scripts/run_showcase.py --runtime board
# one case, one set, more detail:
python3 scripts/run_showcase.py --case case_065 --keep-output
python3 scripts/run_showcase.py --set negative
```

Tool locations default to `build/bin/`, `$LLVM_BUILD` (or `~/work/llvm-project/build`) and
`$SYSTOLIC_CFRONT_CLANG`; `--build-dir`, `--llvm-build`, `--clang`, `--clang-major` override
them. Results go to `reports/showcase_<runtime>_<timestamp>.{json,md}` and
`reports/latest_<runtime>.json`; `manifest.json` always says `"tested": false` — it holds
expectations, never results.
