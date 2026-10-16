# Affine GEMM mapping — mathematical specification (v2: P0 scope)

Status: **implemented** (2026-10-15, uncommitted, on checkpoint `3f21bab`; record:
`test/gemm_recognition/failures/issues/ISSUE-005-p0-operand-views/`). Written before the source
changes and corrected where the implementation decided a detail (sections 5, 6, 12). The first
generalization (v1: the comparison on the iteration domain, the reasons, the brute-force oracle)
is the starting point; its snapshot is `~/work/integration-backups-20261015/target-pre-p0/`.

Every claim is marked **[proved]** (argument given here), **[tested]** (established only by tests)
or **[assumed]** (a premise that is relied on but not checked). Words in CAPITALS are the four
classifications of §9.

---

## 1. The original iteration domain

A perfect chain of n ≥ 1 `affine.for` loops around one statement. Loop s has:

- lower bound λ_s and exclusive upper bound υ_s: constants, or the max/min of constants, which
  are folded with checked arithmetic (P0.6: `min(8, 16)` = 8); every stage uses the folded
  values -- the source domain's stride (x_s − λ_s) mod σ_s = 0 and the re-emitted batch loops
  included (§18.1, §18.4);
- step σ_s ≥ 1 (the `affine.for` verifier rejects σ ≤ 0).

Its trip count is n_s = 0 if υ_s ≤ λ_s, else (υ_s − λ_s − 1) div σ_s + 1. This is computed
checked; the form avoids the overflow of ⌈(υ − λ)/σ⌉.

**Values versus ordinals.** x_s = λ_s + σ_s z_s with ordinal z_s ∈ [0, n_s).

D = {x : x_s = λ_s + σ_s z_s, 0 ≤ z_s < n_s} is in order-preserving bijection with the ordinal
box Z = Π [0, n_s) (lexicographic order = execution order). A loop is **degenerate** if n_s = 1,
**nondegenerate** if n_s ≥ 2. D is **empty** iff some n_s = 0.

**C loops** reach this form through `systolic-cfront`, which computes exact trip counts and
refuses what could overflow the C type:

- `for (T i = L; i < U | i <= U; i += S)` becomes `affine.for %i = L to U' step S`, with the IV
  equal to the counter's value;
- a loop that counts down (`i > B | i >= B`, `--`, `-= S`, `i = i − S`) becomes the ordinal loop
  `affine.for %t = 0 to N` with the counter **value** `L − S·t` substituted at every use;
- an affine condition `E OP c`, with E affine in the counter, is first turned into the equivalent
  counter comparison, exactly over Z.

| C | IV form | n | values |
|---|---|---|---|
| `for (int i = 5; i >= 0; i -= 2)` | t ∈ [0,3), i = 5 − 2t | 3 | 5, 3, 1 |
| `for (int i = 10; i >= 0; i -= 2)` | t ∈ [0,6), i = 10 − 2t | 6 | 10, 8, …, 0 |
| `for (int i = 100; 2*i - 3 > 0; i--)` | 2i − 3 > 0 ⇔ i ≥ 2; t ∈ [0,99), i = 100 − t | 99 | 100 … 2 |

A *decreasing induction sequence* is therefore an ordinary increasing ordinal with a negative
coefficient in the accesses. It is not a negative stride by itself (§5).

**Statement.** `X_C[f_C(x)] = X_C[f_C(x)] + X_A[f_A(x)] * X_B[f_B(x)]` (factors and addends in
either order), where each f_X(x) = (f_X,1(x), …, f_X,r_X(x)) is a tuple of **r_X ≥ 1** affine
subscripts with integer coefficients. The memrefs are never flattened. The C load and the C store
must address the same element at every x ∈ D.

**Domain forms (Lemma 0) [proved].** Write f(x) = Σ f_s x_s + f_0. Its domain form is the ordinal
form Σ (f_s σ_s) z_s + (f_0 + Σ f_s λ_s), with the terms of degenerate loops folded into the
constant. Two subscripts are equal on D iff their domain forms are identical. The *support* of
f is the set of nondegenerate loops with a nonzero coefficient. f is **constant on D** iff its
support is empty.

---

## 2. Logical coordinates

**Coordinate groups.** Join two nondegenerate loops whenever they occur together in the support
of one subscript (any operand, any dimension). The connected components are the groups G. Every
non-constant subscript has its support inside exactly one group, by construction.

**Lemma 1 (one coordinate per group) [proved].** In every valid mapping (§3), each coordinate of the
mapping is driven by its own set of loops (Theorem 3 (S1)). Each non-constant subscript is a nonzero
multiple of one coordinate plus a constant, and its support is all of that coordinate's loops.
Hence groups and coordinates coincide, and all subscripts in one group have proportional linear
parts. The sign of the ratio is free here: a view may run backwards, §5.

**Roles.** A group's role is fixed by the set of operands that read it:

| operands that read G | role | meaning |
|---|---|---|
| A and C | I | output row of A·B |
| B and C | J | output column |
| A and B | K | reduction |
| A, B and C | P | batch |
| C only | free output coordinate | every slice receives the same product: a broadcast of A and B (§10) |
| A only, or B only | reduction in one factor | Σ_u A(…u…)·B(…) needs B broadcast along u (§10) |

If an operand reads one group in two of its dimensions (`A[i][i]`, `C[i+j][j]` with j's loops
merged into i's group), two coordinates would share a loop, which Theorem 3 (S1) forbids.

**The mapping y = M z + b.** y = (P*, I, J, K) collects one ordinal per group. M has one row per
group and one column per loop, so it is 3 × n for a plain GEMM and (3 + |P|) × n when batched. Its
rank over Q equals the number of groups. It is generally rectangular and never needs to be
invertible: what matters is injectivity *on the box Z*, which Theorem 3 characterizes. The extra
coordinates beyond (I, J, K) are needed exactly for batch (P) and are never invented for I/J/K. A
split coordinate (`4 i0 + i1`) is one row whose mixed-radix digits are its loops. A degenerate
loop has an all-zero column (z_s ≡ 0). An **extent-one coordinate** (M, N or K = 1) has no group:
its operands read it through constant subscripts, and its row is the constant 0.

---

## 3. Validity: injectivity, coverage, multiplicity

The nest is a **valid GEMM mapping** iff there are extents E_G ≥ 1 (M, N, K, P*), a map
φ = (ordinal per role): D → R = Π [0, E) and, for every operand, a view (§5) such that:

- **(V)** every subscript of X equals, on D, the view's index of its role coordinate. For a
  dimension of X that reads no coordinate of X's roles, it equals a constant (an extent-one
  dimension, fixed index);
- **(B)** φ is a bijection D → R, so that every logical point is executed exactly once, with no
  collisions (injectivity) and no missing points (coverage);
- **(C0)** the C read and the C write coincide on D.

**Theorem 1 (equivalence, exact arithmetic) [proved].** If (V), (B) and (C0) hold and §7's aliasing
premise holds, then the nest and the batched or plain C′ += A′·B′ produce identical C, as exact
sums. *Proof:* as in v1. C's view is injective in (P*, I, J) because each of those coordinates has
a dimension of C with a nonzero stride, so the contributors of one C element are exactly the K
values of one (P*, I, J). Each is visited once by (B), with the term A′·B′ by (V). ∎

**Equal counts are not enough.** Each of these has |D| = M·N·K and is still not a GEMM of views:

- `A[i][k+u]` (K = 1, 2, 3 visited twice): violates (B) injectivity, a weighted sum;
- `C[i+j][j]` (a parallelogram): violates (B) coverage;
- `A[4i0+i1]` with `C[8i0+i1]`: no strided view (SQ-1).

**Theorem 3 (box bijections) [proved, v1 §4].** An integer affine F(z) = Φz + β on a box with all
n_s ≥ 2 is a bijection onto a box iff:

- **(S1)** each column of Φ has exactly one nonzero entry;
- **(S2)** per row, the magnitudes sorted ascending form a mixed radix starting at 1;
- **(S3)** β puts each coordinate's minimum at 0.

The proof uses unique factorization of the generating functions in the Laurent ring, then an
induction on the digits. Degenerate loops are unconstrained.

**Corollary 4 (decision procedure) [proved].** For D nonempty, the nest is a valid mapping with
strided views iff all of the following hold:

1. (C0) holds;
2. every group has a role among I, J, K, P, and each of I, J, K has at most one group;
3. no operand reads one group twice;
4. within each group, all subscripts have proportional linear parts. Let g_G be their primitive
   direction, signed so that the group's finest-magnitude ordinal coefficient is positive;
5. the ordinal coefficients of g_G, divided by the finest magnitude, form a signed mixed radix (S2).

A role with no group has extent 1 and constant subscripts. Then
ω_G = (|g_G|-reading of z), E_G = Π n_s, and each subscript f = s_f·g_G + o_f with s_f ≠ 0 of
either sign.

---

## 4. The systolic execution model

`systolic.matmul_tile` computes C′ ← C′ + A′·B′ on rank-2 tensors (M×K, K×N, M×N). The lowering
partitions R into tiles (§8). On the device, every operand tile is *copied* into a contiguous
row-major scratch buffer (`TileMatmulForFpga.cpp`, `stageOperand`: `memref.copy` from the operand's
view), so the device never sees the view's layout. Hence the operand views may have **any static
strides, including negative and permuted (transposed)**, provided MLIR's strided memref can
represent them. Measured: `memref.subview` with stride −2, `memref.reinterpret_cast` with stride
−8, and `memref.transpose` all verify. The device computes each invocation's product from a zero
accumulator, and the host adds it into C (`runtime/fpga_matmul_tiled.cpp:486-493`; the software
model is identical, `runtime/offload/soft_runtime.c:108-114`).

**Batched GEMMs (P ≠ ∅; ISSUE-006).** The tiles compute one batch; the batch loops — the nest's
loops with a nonzero coefficient in a P row and two or more iterations — are re-emitted as they
are (their bounds, steps and original order) around the tiles, and each operand's batch
dimension is read at the coordinate's value, an `affine.apply` of its expression over the new
loops' counters (a loop that runs once is a constant, folded in). This reproduces the original
iteration domain exactly: for each batch iteration, the I, J, K nest on the slice the batch index
addresses. **No condition is placed on the batch coordinate itself** (not density, not
injectivity): two batch iterations addressing the same slice accumulate into it twice, as the
loop does. What is required: a batch loop drives no I, J or K (otherwise the batches are
coupled, `C[b][b + i]`, and no box of (I, J, K) exists per batch value — PROVEN_INVALID), and
the three operands read the coordinate through one and the same expression on D (else one view
per operand of a batch coordinate would be needed — VALID_BUT_LOWERING_UNSUPPORTED). A batch
loop that sat between I/J/K loops moves outward: a legal interchange, since the iterations of
distinct batch values touch distinct C elements, and under the reassociation permission (§6) the
order of the accumulations into one element is free. The regions of §8 constrain only I, J, K, so
each runs for every batch iteration and the exact-cover proof holds per batch. An operand
shared by every batch (`C[b] += A[b] B`) has no batch dimension: refused (B broadcast over the
batch, or A and C merged over (b, i): P1.3 / P1.2). The remark's repeat count includes the
batch iterations.

---

## 5. Operand views, signed strides, transposes, physical addresses

Distinguish four things that the v1 code conflated:

1. a **decreasing induction sequence**: an ordinal with a negative coefficient in x(t). It is
   normalized away exactly (§1) and is never a stride;
2. a **negative access coefficient**: s_f < 0 in f = s_f·g + o, where the operand reads the
   coordinate backwards;
3. a **negative physical stride**: the view's stride ρ = s_f·scale_G < 0, the consequence of (2)
   for the chosen ordinal direction;
4. a **negative stride with an offset**: the view's origin (logical index 0) is the element of the
   *largest* physical index along that dimension, not the first one in the allocation.

**Ordinal direction.** Each coordinate's ordinal ω increases along g (g = off + scale·ω,
scale > 0). Any fixed direction is correct: the GEMM is invariant under reversing a coordinate in
all operands at once. The implemented choice orients g along the iteration order of its
outermost loop (its first nonzero coefficient is positive). An operand whose subscript decreases
in that order therefore gets the negative stride itself (`A[7 − i]`: A's view runs backwards,
C's forwards). Two exceptions keep the checkpoint's form: subscripts spelled identically in both
operands, for which the coordinate is that expression, and a role read through the positional
form with the same direction. There a decreasing loop is read backwards with positive strides
(ISSUE-004). The direction matters only for floating point (§6).

**The view of operand X**, rank r_X, shape (d_1, …, d_r), identity layout:

- each GEMM role ρ ∈ roles(X) (A: I, K; B: K, J; C: I, J; plus P) is read in exactly one
  dimension δ(X, ρ) with f = s·g_G + o. Its view index is u_ρ = o + s·off_G + (s·scale_G)·ω, that
  is, base index α = o + s·off_G and signed stride ρ_ρ = s·scale_G (in index units of dimension δ);
- every other dimension e of X has a constant subscript c_e on D (an extent-one dimension).

As a rank-preserving MLIR view:

    v0 = memref.subview X[o_1..o_r] [z_1..z_r] [t_1..t_r]   // rank-reducing: size 1 at constant
         dims; o_δ = α, t_δ = ρ_ρ (signed), z_δ = E_ρ; o_e = c_e, t_e = 1, z_e = 1
    v  = memref.transpose v0 (if the role order of the view's two dims is not the operand's
         GEMM order: A (I,K), B (K,J), C (I,J))

**Physical address [proved].** With row-major strides st_d = Π_{e > d} d_e and the base pointer of
X, element (ω_row, ω_col) of v is at

    base + Σ_e c_e st_e + Σ_{ρ ∈ {row, col}} (α_ρ + ρ_ρ ω_ρ) st_δ(ρ)

which is exactly the address of X[f_X(x)] for the x with φ(x) = (…, ω_row, ω_col, …), by (V). A
negative ρ_ρ makes the origin α_ρ the largest index of dimension δ; nothing is reversed or
copied. The subview's offset is the origin's linear offset, and MLIR's descriptor carries the
signed stride. `memref.copy` (generic strided path) and `memrefCopy` iterate with signed int64
strides **[assumed: MLIR runtime library; tested end to end]**.

**Bounds and memory safety [proved].** Along dimension δ the view touches exactly
{α + ρ ω : ω ∈ [0, E)}, which is the set of values f takes on D (by (V) and (B)). Its minimum
min(α, α + ρ(E−1)) must be ≥ 0, and its maximum max(α, α + ρ(E−1)) must be < d_δ when d_δ is
static, and likewise for every constant dimension. All products and sums are checked int64; one
that does not fit is refused, never wrapped. With a dynamic d_δ (`float A[][K]`), the upper bound
is the program's own guarantee **[assumed: no undefined behaviour in the source]**. The lower bound
is still checked.

**Transpose [proved].** A role read in X's dimensions in the opposite order (`A[k][i]`) is the
same view with its two dimensions permuted. `memref.transpose` permutes the descriptor's sizes
and strides, and the addresses are as above. No data is materialized beyond the per-tile staging
copy that every operand already gets.

**Output view.** C′ may have negative or permuted strides too, because the host's write-back is a
`memref.copy` into C's view (`bufferization.materialize_in_destination`).

---

## 6. Floating-point reduction semantics

**Measured fact.** For one C element and one invocation covering K values k_1 … k_m, the device
returns v = ((0 + a_1 b_1) + a_2 b_2) + … (its own fp32 datapath), and the host computes C ← C + v.
The loop computes C ← ((C + a_1 b_1) + a_2 b_2) + … . These are different IEEE operations:

- **reassociation:** C + (Σ) versus ((C + p_1) + p_2) …;
- **signed zero:** even for m = 1, if p_1 = −0 then v = 0 + (−0) = +0, so C = −0 gives +0
  where the loop gives −0;
- **NaN payloads and the device's rounding / denormal handling:** not verifiable here (no board,
  HLS floating-point cores). **[assumed / unverified]**

**Therefore no offload of an fp32 accumulation can be proven equal under strict IEEE semantics
[proved, given the measured runtime contract].** The original C program does not permit
reassociation (C11 §5.1.2.3: no `-ffast-math`; `FP_CONTRACT` allows contraction to an FMA, not
reassociation).

**Policy (P0.8).**

- **Default (strict):** an fp reduction is *not* offloaded. Diagnostic: "the systolic reduction
  order differs from the loop's; floating-point reassociation is not permitted".
- **Permitted:** offload when, and only when, reassociation is explicitly allowed:
  - the `arith.addf` of the nest carries the `reassoc` fast-math flag (the C front end sets it
    when compiled with `-ffast-math` / `-fassociative-math`), or
  - the user passes the option (`--affine-matmul-to-systolic=allow-fp-reassociation=true`;
    `systolic-cc --fp-reassociation`).

  Under permission, results equal the loop's exactly when every partial sum is exact, as in the
  tests' small-integer data. Otherwise they may differ in rounding, in the sign of a zero sum, and
  in NaN payload. Infinities and NaNs still propagate as IEEE requires: a NaN or ∞ operand yields
  NaN or ∞ in both, except ∞ − ∞ ordering effects, which reassociation may change.
- **Integer reductions:** not recognized (`arith.addf`/`mulf` only). An integer GEMM would need the
  source language's overflow semantics (C: signed overflow is undefined, unsigned wraps). It is
  out of scope and refused.

**Within permission** the order of K is immaterial to the result's exactness: the device
reassociates every tile anyway. No attempt is made to keep the loop's order of K across the K-fold
invocations; they run in increasing ordinal order.

---

## 7. Aliasing and other memory semantics (P0.7)

The nest writes only C, at C's view. Offload changes the order of the reads of A, B and C and of
the writes to C: all of A and B are read before C is written back (per tile). This is equivalent to
the loop iff no element C writes is read through A or B, and A and B are not written.

Policy, conservative **[proved under its premise]**: A and B must be provably disjoint from C.

- **Provably disjoint** means one of:
  - different root buffers, one of them a fresh `alloc`/`alloca`;
  - a `llvm.noalias` (C `restrict`) argument on either side (overlap would be UB in C);
  - the explicit user assertion `assume-no-alias`.
- **Otherwise** the nest is refused with the reason "A may overlap C". This holds for views of one
  root buffer even with disjoint regions; bufferization's `restrict` forbids two tensors of one
  buffer, and region disjointness is not analysed (P1).
- **A and B the same memref** (read-only) is allowed only through the same view (one tensor).
  Different views (`A·Aᵀ`) are VALID_BUT_LOWERING_UNSUPPORTED.

Negative-stride and transposed views change none of this: they are views of the same root
buffers and are traced through `ViewLikeOpInterface` as before.

---

## 8. Execution legality of the tiles

The planner's boxes are proven an exact partition of R: `checkExactCover` on the ordinal grid, and
for ≤ 64 regions a Presburger check in source coordinates. Each region is D restricted to an
ordinal box, which is independent of the views' signs and permutation, because the views are
functions of the ordinals. Hardware limits (array size, k_max, L1, devices) are the planner's;
they affect feasibility, not equivalence.

---

## 9. Classification and diagnostics (P0.9)

Every candidate nest (a perfect chain around one statement) gets one class and a reason:

| class | meaning | examples |
|---|---|---|
| PROVEN_VALID | a valid mapping (§3) exists and its views are representable. Offload still requires the FP permission (§6) and the memory premises (§7); a refusal for those reasons names them | all P0 positives |
| VALID_BUT_LOWERING_UNSUPPORTED | a valid mathematical mapping is established, but a lowering component is missing; the missing component is named | a batch coordinate read through different expressions per operand; broadcast / stride-0 (P1.3); two I (or J, K) groups needing a merged view; A and B as different views of one memref |
| PROVEN_INVALID | no valid mapping exists; the violated condition is named | statement not C += A·B; (C0) fails; (B) fails (coupled, multiplicity, missing points, non-box domain); a role read twice by one operand; a static out-of-bounds view (the original program is undefined) |
| UNKNOWN | correctness cannot be established by the implemented analysis | mod/div subscripts (P2); symbolic bounds (P1.4); SQ-1-type operand-specific decompositions (P1.2); a check that overflows 64-bit arithmetic |

UNKNOWN is never treated as valid. An empty domain is PROVEN_VALID with nothing to compute, and is
not offloaded (SQ-2 stays open; refusing preserves semantics exactly).

---

## 10. Scope

- **Supported mathematical class 𝒢₀ (completeness claimed, Corollary 4).**
  - Loops: a perfect chain, constant or constant-foldable bounds, a nonempty domain.
  - Statement: the fp32 MAC body.
  - Subscripts: linear in the IVs (no symbols, mod or div), operands of any rank ≥ 1, roles
    I, J, K with at most one group each (absent = extent 1), no batch.
  - Views: any signed strides, any dimension order, constant (extent-one) dimensions anywhere.
  - Every such nest that is a valid mapping is PROVEN_VALID and lowered, subject to §6 and §7.
  - Batch coordinates (ISSUE-006): any number, read identically by the three operands, in any
    dimension of each; the batch loops anywhere in the nest, with any bounds and steps.
- **P1, analysed and classified, not lowered.** A batch coordinate with per-operand views
  (`A[b + 1]` with `C[b]`) or an operand without it (shared by every batch); a batched nest
  whose per-batch GEMM has two unit extents (a vector scaled by a scalar: SQ-5); broadcast /
  stride-0; operand-specific decompositions (SQ-1); merged multi-group roles; symbolic bounds;
  disjoint regions of one buffer.
- **P2.** mod/div, quasi-affine, piecewise subscripts: UNKNOWN. Simplification into the affine
  class is attempted first: MLIR's affine expression simplification already folds e.g.
  `(4*i) floordiv 4`.

**Soundness [proved modulo the marked assumptions].** Every offloaded nest satisfies (V), (B),
(C0) and §7, so by Theorem 1 it computes the same exact sums. The FP difference is permitted only
under explicit reassociation permission.

Completeness is claimed only for 𝒢₀, never for arbitrary affine programs or contractions.

---

## 11. Proof obligations per stage

| stage | obligation | established by |
|---|---|---|
| domain (front end, normalization) | exact trip counts, value/ordinal, checked arithmetic, min/max folding | §1 [proved]; CFront tests; new P0.6 test [tested] |
| groups/roles | Lemma 1, roles table, one group per operand dimension | [proved]; oracle [tested] |
| bijection | Theorem 3 / Corollary 4 | [proved]; oracle, lit negatives [tested] |
| views | (V) with signed strides and permutations; addresses (§5) | [proved]; oracle compares offsets, sizes and signed strides; end-to-end numerical tests including the physical bounds of every access [tested] |
| memory | bounds, overflow, aliasing (§5, §7) | [proved]; lit negatives [tested] |
| FP policy | default refusal; permission paths | §6 [proved/measured]; lit + end-to-end default vs permitted [tested] |
| partition | exact cover | §8 [proved, library-checked] |

The **oracle** (`test/Systolic/Inputs/affine_gemm_mapping_oracle.py`) decides by enumeration of D,
independently of Theorem 3. For P0 it is extended with:

- transposed roles: each subscript's role is found by matching rank functions, not positions;
- signed strides: a subscript is an arithmetic progression in either direction;
- ranks other than 2, with constant dimensions;
- the physical address of every accessed element, checked to lie inside the allocation.

End-to-end tests run the original C loops and the offloaded code on the same data and compare.

---

## 12. Changes made (the plan, as implemented)

1. **Analysis (`AffineMatmulAnalysis.cpp`).**
   - Replace the positional matching ("results r−2 and r−1, A/B swap") by the group/role analysis
     of §2, over subscripts of any rank.
   - Proportionality of either sign.
   - Constant dimensions recorded per operand.
   - min/max constant bounds folded (P0.6).
   - Class + reason in `LogicalMatmul`.
2. **Dense partition.**
   - Unchanged math (Theorem 3), driven by groups.
   - The sign of each view is no longer an obstacle (v1 refused opposite directions at
     recognition and negative views in the lowering).
   - SQ-4 / extent-one: a role without a group gets extent 1.
3. **Lowering (`AffineMatmulToSystolic.cpp`).**
   - Rank-reducing `memref.subview` with signed strides, then `memref.transpose` when needed.
   - Bounds on min/max per dimension.
   - The FP policy: the `reassoc` flag or the pass option.
4. **Driver / front end.**
   - `systolic-cfront` emits `reassoc` on the MAC's `arith.addf` under `-ffast-math` /
     `-fassociative-math` (clang's own flags).
   - `systolic-cc --fp-reassociation` passes the pass option.
5. **Tests.**
   - P0 lit tests (each of the ten negative-stride items of the request, transposes, extent-one,
     empty, min/max, aliasing, overflow, FP default vs permitted).
   - The oracle extended.
   - End-to-end C tests.
   - Existing tests that offload today must state the FP permission explicitly (§13).

## 13. Decisions (taken by the user, 2026-10-15)

Strict FP default with explicit opt-in. Existing tests and scripts state the permission, and their
expected values are unchanged. SQ-4 is decided: offloaded. The protected test files that are
edited are `chmod`ed per file and restored to 0555. The board scripts get the flag. The questions
as they were put:

- **FP default (P0.8).** By §6 the strict default refuses *every* offload of the existing tests:
  - 23 lit test files run `--affine-matmul-to-systolic` and 8 `systolic-cc` tests offload;
  - the showcase, regression and generated corpus all predict offloading;
  - the board scripts `test/e2e/affine_c/run*.sh` and `test/e2e/systolic_cc/run_board.sh` also
    offload by default.

  Keeping their results requires adding the explicit permission to their invocations. That
  changes no expected value, but it does change files the previous rules protect: the generator,
  scripts and manifest are mode 0555, and the e2e board scripts are FPGA scripts.
- **SQ-4.** P0.5 ("handle extent-one coordinates correctly") is read as deciding SQ-4 in favour of
  offloading M, N or K = 1. This changes the expectation of `ISSUE-003/repro_constant_coordinate.mlir`
  and the SQ-4 entries.
- **P0.6** changes the expectation of `ISSUE-002/repro_overflow_diagnostics.mlir` (`@min_const_ub`
  is then an 8-row GEMM, not "not static").
- **P0.2 / P0.3** change the expectations of showcase case_091–093 (transposed) and mlir_010
  (opposite directions) from OUT_OF_SCOPE to ACCEPT. Batched case_085/086 stay out: P1.

## 14. Verification (2026-10-15, the final tree)

| test | result |
|---|---|
| check-systolic, release and debug (assertions) | 94/94 and 94/94 |
| oracle, debug build: 40 seeds × 400 nests with the permission | 16,000 nests, 10,291 accepted (2,675 with a transposed view, 9,367 with a negative stride, 4,063 with M/N/K = 1, 7,398 with an operand of rank ≠ 2, 4,000 with a loop that runs once): **0 mismatches** |
| oracle without the permission | 2,000 nests: nothing lowered, 0 mismatches |
| oracle sensitivity | catches mirrored rows (165 of 198) and an offset of +1 (198 of 198); against the checkpoint, 197 of 198 valid P0 nests are not lowered |
| showcase (161) | rc 0: 154 pass + 7 expected_refusal (SQ-1..SQ-3) |
| regression gate | rc 0: ISSUE-001 2/2, 002 9/9, 003 2/2, 005 2/2 (004: 0/0); 0 regressions |
| 800 generated cases | 800/800 pass |
| `scripts/test_permissions.py` | 48/48 |
| board scripts, FPGA, minimizer | NOT RUN (no board) |

## 15. ISSUE-006: batched GEMMs (2026-10-15)

Changes: `getDenseLogicalGemm` takes the first `numP` rows of T as batch coordinates (every
other row index is `numP + d`, so a plain GEMM is handled exactly as before), records the batch
loops, each coordinate's value range and the batch trip count; `restrictToBox` leaves the batch
coordinates unconstrained; the group analysis accepts a batch group the three operands read
identically and records each operand's batch dimension (`OperandLayout::batchDim`); the lowering
emits the batch loops, the `affine.apply` indices and subviews with a dynamic offset (a view of
batch dimensions only, rank 0, becomes the 1 × 1 matrix); the LLVM pipeline lowers the affine
loops before `expand-strided-metadata` (a dynamic subview offset computed from an `affine.for`
counter is an affine dimension, which the `affine.apply` the expansion builds may not use as a
symbol — found by the generated corpus on multi-tile batches). Snapshot before the change:
`~/work/integration-backups-20261015/target-pre-batched/`.

Verification (tree of 2026-10-15, after ISSUE-006):

| test | result |
|---|---|
| check-systolic, release and debug (assertions) | 97/97 and 97/97 (94 + `affine_gemm_batched_lowering.mlir`, `SystolicCC/affine_batched.c`, `ISSUE-006/repro_batched.mlir`) |
| oracle, debug build, 40 seeds × 400 | 16,000 nests, 10,561 accepted, 1,776 of them batched: **0 mismatches**; 1,200 nests without the permission: nothing lowered |
| showcase (161) | rc 0: 154 pass + 7 expected_refusal; case_085 and ext_011 now offloaded (2 and 4 invocations, verified); nothing else changed |
| SQ-1, SQ-2, SQ-3 | unchanged (`expected_refusal`); `SPEC_QUESTIONS.md` records what each would mean for a batch loop |
| SQ-5 (new) | a batched nest with two unit extents per batch (`C[i][k] += A[i][k] * B[k][j]`, `j` of one iteration: "the reduction index in the output" read as a batch): correct to lower, refused conservatively pending the question; found by the generated corpus as a false positive of the first batched build (`gen_04bff9d675c2`) |
| regression gate, 800 generated cases | `~/work/integration-backups-20261015/batched-validation/` |
| `test_permissions.py` | 48/48 |
| FPGA board, minimizer | NOT RUN (no board) |

## 16. ISSUE-007: batch-dependent operand views (2026-10-16)

### 16.1 The affine model and what the geometric conditions do and do not give

The nest runs over D = Π_s {o_s + σ_s t : 0 ≤ t < n_s} (σ_s ≥ 1; a loop with n_s = 1 is a constant
on D, Lemma 0, and is folded into the constants below). Operand X ∈ {A, B, C} reads
X[F_X(x)], F_X(x) = M_X x + m_X, one row per memref dimension. In the notation of the request,
for rank-2 operands M_C = [c^T; d^T] (Ic, Jc), M_A = [a^T; u^T] (Ia, Ka), M_B = [v^T; b^T]
(Kb, Jb); every result below is stated for any rank and any number n of loops.

**Readers.** For a loop s with n_s ≥ 2, R(s) = {X : column s of M_X is nonzero}. The class of s:

| R(s) | class | geometric meaning |
|---|---|---|
| {A, C} | I | M_B e_s = 0: B constant along e_s |
| {B, C} | J | M_A e_s = 0 |
| {A, B} | K | M_C e_s = 0: e_s ∈ null(M_C), a reduction direction |
| {A, B, C} | batch (P) | every operand moves along e_s |
| one operand | refused | the other two are broadcast along e_s (§10) |

The reduction directions the request asks for are exactly the K loops: e_s ∈ null(M_C), and,
by the role conditions below, a^T e_s = 0 and b^T e_s = 0 (A's row and B's column do not move)
while u^T e_s ≠ 0 and v^T e_s ≠ 0. Only axis directions need to be considered: on a box with all
n_s ≥ 2, Theorem 3 (S1) forces every logical coordinate onto its own set of loops, so a direction
mixing loops of two coordinates (r = e_i − e_j for C[i + j][j]) never yields a box of logical
points. The classification costs O(n · dims) and holds for any n.

**These conditions are necessary, not sufficient** — each of the following satisfies
rank(M_C) = 2, has a direction r with M_C r = 0, a^T r = b^T r = 0, u^T r ≠ 0, v^T r ≠ 0, and is
not a GEMM (all in `test/Systolic/affine_gemm_batch_views.mlir`):

1. C[i][j] += A[i][k0] · B[k1][j], r = e_k0 + e_k1: the nest is (Σ_u A[i][u]) · (Σ_v B[v][j]).
   Each of k0, k1 has one reader: refused by the readers table (a reduction in one factor).
2. A[i][k0 + k1] · B[k0 + k1][j]: Ka and Kb move together, but K = 1, 2, 3 is visited more than
   once — a weighted sum. Refused by Theorem 3 (S2): the K loops must number K densely.
3. K = 3k0 + 2k1 (k0 < 2, k1 < 3): injective (0, 2, 3, 4, 5, 7) but not a strided view: (S2).
4. C[i + j][j] += A[i + j][k] · B[k][j] (case_087) and C[i + k][j] += A[i][k] · B[k][j]
   (case_127): a direction of C's row moves with a loop all three operands read — the output
   position moves with the batch. Refused (C purity, below; case_087 already by Theorem 3 (S1)
   through the positional form).
5. The Phase 1 target with i starting at 0: every structural condition holds and A's row index
   reaches −3. Refused by the bounds check (the program itself is undefined).

### 16.2 The acceptance rule (batch-dependent views)

A nest is accepted as a batched GEMM of batch-dependent views iff, besides (C0), §6 and §7:

1. **Readers.** Every loop with n_s ≥ 2 is in exactly one class of the table; P = the batch loops.
2. **C purity.** Every dimension of C reads either the loops of one role (I or J) or batch loops
   only. A batch coordinate may fill several dimensions of C (C[b][b][i][j]); no dimension of C
   mixes a batch loop with another loop.
3. **Factor views.** Every dimension of A (B) is the sum of a role part — the loops of one of its
   roles, I or K (K or J) — a batch part Σ_{s ∈ P} β_{X,d,s} x_s with arbitrary integer β, and a
   constant. A dimension with no role part is a batch dimension of that factor.
4. **Per batch, a dense GEMM.** With the batch loops fixed, the role parts satisfy Corollary 4:
   each role's subscripts are views s·g + o of one coordinate whose loops number it densely
   (Theorem 3), so (I, J, K) is a bijection of the batch's points onto one box M × N × K, the
   same for every batch value (the role parts do not depend on the batch loops).
5. **Bounds.** For every dimension, [min, max] of its role part over the role box plus [min, max]
   of its batch part over the batch box (exact, the loops being independent:
   `rangeOnDomain`) lies inside the array, in checked 64-bit arithmetic.
6. **Unchanged refusals.** Coupled, skewed and element-wise mappings, broadcasts, an operand
   reading a role in two dimensions, non-dense coordinates, SQ-1, SQ-2, SQ-3, and SQ-5 (two of
   M, N, K equal to 1 per batch) are refused exactly as before.

**Theorem 5 (batch-dependent views) [proved].** If the rule holds, the lowering — the batch loops
re-emitted with their own iterations (λ_s + σ_s z, 0 ≤ z < n_s, from the folded bounds of §1:
§18.1), around the tiles of an exact partition of the role box (§8), every operand read through its view at the offset m + Σ β x_P evaluated per batch
iteration — performs exactly the updates of the nest: for every x ∈ D the one update
C[F_C(x)] += A[F_A(x)] · B[F_B(x)], and no other. *Proof.* The batch loops are re-emitted
with the same iterations, so every batch value p of D occurs exactly once. For a fixed p, condition 4 makes
(I, J, K) a bijection of D_p = {x ∈ D : x_P = p} onto the box, and the partition of §8 covers
the box exactly once, so every x ∈ D_p is executed by exactly one tile slot. At that slot, each
operand's element is its view's element: the role part by (V) of §3 (the views are the
coordinates' views, Corollary 4), plus the batch part β·p, which the offset adds — equal to
F_X(x) by condition 3 (C: condition 2, batch part zero in its role dimensions). Condition 5
keeps every address inside its array, §7 keeps A and B disjoint from C. ∎ With exact
arithmetic C is therefore identical; in floating point the order of the additions into one C
element changes (the tiles sum from zero; contributions of different batch iterations to one
element, possible when C's batch map is not injective, keep the batch loops' order up to the
K sums), which §6's reassociation permission allows.

### 16.3 The Phase 1 target, C[8i][j] += A[4i − j][5k] · B[5k][j]

x = (i, j, k). M_C = [[8, 0, 0], [0, 1, 0]] (rank 2), M_A = [[4, −1, 0], [0, 0, 5]],
M_B = [[0, 0, 5], [0, 1, 0]]. null(M_C) = span(e_k): a^T e_k = 0, u^T e_k = 5, v^T e_k = 5,
b^T e_k = 0. Readers: R(i) = {A, C} → I; R(j) = {A, B, C} → batch; R(k) = {A, B} → K; no loop
reads only B and C, so N = 1. C: dimension 0 is 8i (I), dimension 1 is j (batch): pure. A:
dimension 0 is the role part 4i plus the batch part −j; dimension 1 is 5k. B: dimension 0 is
5k (K), dimension 1 is j (a batch dimension). Per value of j: C'(I) = C[8I][j],
A'_j(I, K) = A[4I − j][5K], B'_j(K) = B[5K][j] — a 4 × 1 × 4 GEMV on strided views, A's offset
moving with j. Not one GEMM of A and B (A's row depends on the output column): before ISSUE-007
it was refused ("C reads one logical coordinate in two of its dimensions", the union of i and j
through A's row); the executable specification said REJECT ("A has a subscript that couples
loops of different coordinates").

Bounds: A's row index 4i − j ranges over [4·i_lo − (j_hi − 1), 4·(i_hi − 1) − j_lo]; with
i ∈ [1, 5), j ∈ [0, 4) that is [1, 16] inside 17 rows; C's row 8i over [8, 32] inside 33; A's
column and B's row 5k over [0, 15] inside 16. With i ∈ [0, 4) A's row reaches −3: refused
("A is accessed out of bounds along dimension 0", PROVEN_INVALID).

Lowering (`test/Systolic/affine_gemm_batch_views.mlir` @phase1_target):

    affine.for %j = 0 to 4 {
      %o = affine.apply affine_map<(d0) -> (-d0 + 4)>(%j)
      A' = memref.subview %A[%o, 0] [4, 4] [4, 5]          (rows 4I + 4 - j, columns 5K)
      B' = memref.subview %B[0, %j] [4, 1] [5, 1] -> expand_shape 4 x 1
      C' = memref.subview %C[8, %j] [4, 1] [8, 1] -> expand_shape 4 x 1
      systolic.matmul_tile A', B', C' m = 4 n = 1 k = 4
    }

### 16.4 Implementation and specification changes

- **Analysis** (`recognizeLogicalMatmul`, the analysis by groups): loops are classified by
  readers first; the batch loops take no part in grouping the role loops (only C's subscripts
  join batch loops into batch coordinates); a subscript of C mixing a batch loop with another loop
  is refused; a role group read by all three operands (loops of two roles coupled through one
  subscript, A[i + k]) is refused; C may read a batch coordinate in several dimensions; the role
  coordinates are matched on the subscripts without their batch terms; each operand's layout
  records, per dimension, the batch loops' coefficients (`OperandLayout::batchCoefficients`) and
  a batch dimension's static index; every dimension of C that reads batch loops only is a row of
  the partition prefix (so a batch coordinate filling several dimensions of C has every batch
  loop in some row). The positional form is still tried first (its prefix dimensions recorded
  the same way, its role dimensions without batch terms); when its reading couples a prefix loop
  with I, J or K -- never lowerable -- the analysis by groups decides instead (A[b][i][b] with
  B[b][b][j]: b a batch loop, K = 1), and the positional reading is kept only when that analysis
  refuses too, so that C[b][b + i][j] is refused by the dense partition with its diagnostic
  unchanged. Both were found by the oracle (7 of 16,000 nests, all refused valid mappings).
  The review generalized the fallback to any loop that drives two of the positional
  coordinates (§18.2).
- **Partitioning**: `rangeOnDomain` — the exact range of an affine form of the loop counters on
  the normalized box. `getDenseLogicalGemm` is unchanged.
- **Lowering**: one `affine.apply` per operand dimension with batch terms (identical ones
  shared), the dynamic subview offsets, every index range widened by its batch terms before the
  bounds and address checks, and a refusal for a batch term on a loop that is not re-emitted.
- **Specification**: the executable spec (`casespec.py`) decides such nests by enumerating each
  batch iteration (`batched_gemm_on_domain`, basis `batched_views`) and its predictor mirrors the
  new analysis; the oracle decides batched nests per batch iteration and now checks a lowered
  batched nest's views address by address for every batch value, evaluating the emitted
  `affine.apply` offsets (two new mutations: a batch offset in a factor's role dimension, valid;
  in C's, refused).
- **Expectations that changed by specification** (each now offloaded and verified against the
  CPU reference): per-operand batch indices (A[b + 1] with B[b], C[b]:
  `affine_gemm_batched_lowering.mlir` @batch_views_differ, `affine_to_systolic_refused.mlir`
  @batched replaced by a C-mixing case that stays refused); `test/SystolicCC/depth.c` ng_skew,
  C[i][j] += A[i + j][k] · B[k][2j] (the Phase 1 class: A's row moves with the output column).

### 16.5 Verification (Phase 1, 2026-10-16; logs in `~/work/integration-backups-20261016/phase1-validation/`)

| test | result |
|---|---|
| check-systolic, release and debug (assertions) | 100/100 and 100/100 (97 + `affine_gemm_batch_views.mlir`, `SystolicCC/affine_batch_views.c`, `ISSUE-007/repro_batch_views.mlir`) |
| oracle, debug build, 40 seeds × 400 (seeds 100–139) | 16,000 nests, 11,579 accepted, 2,200 batched -- 159 of them with a factor's view moving with the batch -- every batched lowering checked address by address per batch value: **0 mismatches**; 1,200 nests without the permission: nothing lowered |
| executed (`SystolicCC/affine_batch_views.c`) | 7 kernels, the Phase 1 target among them, equal to the CPU reference (soft runtime and the mock board), `--verify` bitwise 7/7 |
| showcase (165) | rc 0: 158 pass + 7 expected_refusal; ext_015..018 (new) offloaded and verified (4, 16, 16, 2 invocations); no existing case changed |
| regression gate | rc 0; ISSUE-001..007 reproducers pass; 168 unchanged; 161 pass + 7 expected_refusal |
| 800 generated cases | identical to the run before Phase 1 except `gen_de03cb2b804b`: REJECT → UNDETERMINED (SQ-5, decided on its domain: `i` runs once, a batch of dot products), refused either way (`pass` → `expected_refusal`) |
| `test_permissions.py` | 48/48 |
| found by the oracle during Phase 1 and fixed | a batch coordinate filling several dimensions of C (one partition row per such dimension), and a coupled positional reading pre-empting the analysis by groups (16.4): 7 refused valid mappings, now lowered and verified |
| FPGA board | NOT RUN (no board) |

## 17. Phase 2: A and B transposed, alone or together (2026-10-16)

A factor is **transposed** when its memref stores its two roles in the other order: A[k][i]
holds A'(I, K) with K in dimension 0, B[j][k] holds B'(K, J) with K in dimension 1. The two
factors are independent: either, both or neither may be transposed (C transposed is a separate
matter, below). Phase 2 established, stage by stage, that every combination keeps the operand
mapping, and added the executed coverage that A and B transposed together never had. No
compiler change was needed: the transposes of ISSUE-005 were already correct in every
combination; Phase 2 is verification and tests.

**How each stage keeps the mapping.**

- *Analysis* (`recognizeLogicalMatmul`): roles are found by the operands that read each loop,
  never by position; each operand's layout records the memref dimension of each of its roles
  (`OperandLayout::roleDim`, transposed when roleDim[0] > roleDim[1]), independently per operand.
  The factor assignment that transposes least is chosen (C first, then the factors as written):
  for A^T and B^T with C as written both assignments cost two transposes and A stays A; for
  C[j][i] += A[k][i] · B[j][k] the swapped assignment C^T = B · A needs none.
- *Partitioning* (`getDenseLogicalGemm`, `partitionLogicalMatmul`): works on the logical
  coordinates (I, J, K) and the role loops only; the boxes are boxes of logical ordinals. The
  layout never enters, so a transposed operand cannot change a region.
- *Lowering* (`getLogicalOperand`, `materializeOperandView`): the subview is taken in the
  memref's own dimension order -- offsets, sizes and signed strides of each role's dimension, the
  bounds checked per memref dimension -- and then `memref.transpose` swaps the two dimensions of
  a transposed operand, so that the tile receives A'(I, K) and B'(K, J). A batch offset
  (ISSUE-007) is a dynamic offset of the subview, before the transpose.
- *Tiling and staging* (`TileMatmulForFpga.cpp`): every tile operand is a `tensor.extract_slice`
  of the logical (transposed) view; staging copies that slice from its strided source buffer
  into a contiguous scratch tile (`memref.copy`, any layout), so the runtime only ever sees
  contiguous row-major tiles.
- *Runtime* (`fpga_matmul_tiled_auto_scheduled`, the soft model, the board): layout-free.

**The shared reduction coordinate.** With A[k][i] and B[j][k], K is A's dimension 0 and B's
dimension 1; each view carries its own K stride, and the tile pairs A'(I, K) with B'(K, J)
logically, which the address-exact oracle checks element by element. When both factors read K
backwards alike (A[7 − k], B[..][7 − k]) the coordinate K is the expression 7 − k itself and no
view runs backwards; a negative stride appears only when the operands disagree (A[7 − k][3 − i]
with B[j][k]: A's subview has strides (−1, −1)).

**Coverage** (each combination: lowered IR checked in lit, executed and compared bitwise with
the CPU reference, and in the oracle's address-exact check):

| # | combination | lit (`affine_gemm_transpose_ab.mlir`) | executed (`SystolicCC/affine_transpose_ab.c`) | corpus |
|---|---|---|---|---|
| 1 | neither | @nn | nn | the positive set |
| 2 | A only | @tn | tn | case_091, generated (9) |
| 3 | B only | @nt | nt | case_092, generated (14) |
| 4 | A and B | @tt (4 × 6 × 8) | tt | ext_019 |
| 5 | A and B, offsets | @tt_offset | tt_offset | -- |
| 6 | A and B, strides | @tt_stride | tt_stride | -- |
| 7 | A and B, negative stride | @tt_negative | tt_negative | -- |
| 8 | A and B, several tiles | @tt_tiles (to LLVM: 10 calls) | tt_tiles (10), tt_rect (22) | ext_020 |
| 9 | A and B, views moving with the batch | @tt_batch_views | tt_batch | ext_021 |
| -- | B transposed and backwards | @nt_negative | nt_negative | ext_022 |

The oracle counts the nests whose A and B views are both transposed (9 to 15 per 400 random
nests) and its lit test requires that count to be nonzero. C transposed is covered separately
(`affine_gemm_p0_views.mlir` @transposed_c, `affine_p0.c` neg_c_tr, showcase case_093).

**Verification** (Phase 2, 2026-10-16; logs in `~/work/integration-backups-20261016/phase2-validation/`):

| test | result |
|---|---|
| check-systolic, release and debug (assertions) | 102/102 and 102/102 (+ `affine_gemm_transpose_ab.mlir`, `SystolicCC/affine_transpose_ab.c`) |
| executed (`SystolicCC/affine_transpose_ab.c`) | 11 kernels (the nine combinations, B backwards, 8 × 40 × 24) equal to the CPU reference on the soft runtime and the mock board; `--verify` bitwise 11/11; without the permission nothing offloaded |
| oracle, debug build, 40 seeds × 400 | 16,000 nests, 0 mismatches; 477 with A and B both transposed, 2,200 batched, all checked address by address; 1,200 without the permission: nothing lowered |
| showcase (169) | rc 0: 162 pass + 7 expected_refusal; ext_019..022 (new) offloaded and verified (2, 10, 2, 1 invocations); no existing case changed |
| regression gate | rc 0; ISSUE-001..007 reproducers pass; 172 unchanged; 165 pass + 7 expected_refusal |
| 800 generated cases | identical to the Phase 1 run |
| `test_permissions.py` | 48/48 |
| FPGA board | NOT RUN (no board) |

## 18. Review-found defects and their fixes (2026-10-16)

An independent review of Phases 1 and 2 (two reviewers; every claim reproduced on the
compiler) found three defects; making the oracle strong enough to have caught them found a
fourth. Each is fixed and has a lit test that fails on the tree before the fix (checked with the
pre-fix debug build) and, where C can express it, an executed test.

### 18.1 Batch loops re-emitted with a bound map's first result (wrong code)

**Defect.** The lowering re-emitted each batch loop with `getConstantLowerBound()` /
`getConstantUpperBound()`, which for a bound map of several constant results — a max lower
bound, a min upper bound, inside the domain of §1 (P0.6) — return the map's first result,
while the analysis, the dense partition and every bounds check use the folded λ (the max of the
lower results) and υ (the min of the upper ones). `j = 0 to min(4, 3)`, three batch iterations
in the analysis and in the remark, was re-emitted as `0 to 4`: the iteration j = 3 reads and
writes column 3 of B and C, which have three columns. Likewise `max(0, 2) to 4` (re-emitted from
0) and the positional form of ISSUE-006 (`b = 0 to min(4, 2)` over two slices). Present since
ISSUE-006.

**What must hold** (the premise of Theorem 5): the re-emitted loops enumerate exactly the batch
loops' values {λ_s + σ_s z : 0 ≤ z < n_s}.

**Fix.** Each batch loop's origin, step and trip count come from the analysis's normalization
(x_s = S_ss z_s + sourceOrigin_s with z_s ∈ [0, n_s − 1] on the normalized domain), the values
every check used; the last value λ_s + σ_s (n_s − 1) is computed with checked arithmetic before
any IR is built (an overflow refuses the nest); the loop is emitted as `λ_s to υ' step σ_s`, υ'
the written upper bound when it is one constant (the same iterations, the IR as written) and
λ_s + σ_s (n_s − 1) + 1 otherwise.

**Tests.** `affine_gemm_batch_views.mlir` @batch_min_bound (`0 to 3`), @batch_max_bound
(`2 to 4`), @positional_batch_min_bound (`0 to 2`), @batch_stepped_split
(`max(-1, 1) to min(9, 8) step 3`, re-emitted as `1 to 8 step 3`); the oracle (18.5).

### 18.2 A positional reading in which one loop drives two coordinates (false refusal)

**Defect.** The positional form (A = (P*, I, K), B = (P*, K, J), C = (P*, I, J)) is tried first,
and Phase 1 let the analysis by groups decide only when a prefix loop also drove I, J or K. A
reading in which another loop drives two of I, J, K was kept and refused by the dense partition
as PROVEN_INVALID: `C[i][j] += A[i][k + j] · B[k + j][j]` ("loop 1 drives J and K") and
`A[i][k + i] · B[k + i][j]` — valid batched GEMMs (per j a 4 × 1 × 4 product reading K at the
offset j).

**Lemma.** If one loop drives two of the positional coordinates — the prefix P* taken as one,
I, J, K — all three operands read it. *Proof.* The prefix is read by all three operands, I by A
and C, J by B and C, K by A and B; any two of these four contain A, B and C. ∎ So the loop is a
batch loop by its readers (§16.2), and the positional reading, in which it drives two
coordinates, is never lowerable (Theorem 3: a loop drives at most one coordinate).

**Fix.** The fallback is taken when some loop has a nonzero domain coefficient in two or more of
{P* as one, I, J, K}; the analysis by groups then decides, and the positional reading — with its
diagnostic — is kept only when that analysis refuses too: case_087,
`C[i + j][j] += A[i + j][k] · B[k][j]`, is still refused by the dense partition with "loop 1
drives I and J" (the groups refuse it: C mixes the batch loop j with i). The executable
specification's predictor mirrors the condition (`casespec.py`, `coupled`).

**Tests.** @j_in_k (4 × 1 × 4 batched over 4), @i_in_k (1 × 4 × 4 batched over 4); executed
`bv_j_in_k`, `bv_i_in_k` (`SystolicCC/affine_batch_views.c`); showcase ext_023, ext_024; the
oracle's mutation `role_in_k`.

### 18.3 A positional prefix constant on the domain counted as a batch coordinate (false refusal)

**Defect.** A prefix subscript of the positional form whose value is constant on the domain — a
literal, or a loop that runs once — became a row of P*: the nest counted as batched, and a plain
GEMM with two unit extents was refused pending SQ-5 (`C[t][i][0] += A[t][i][1] · B[t][1][0]`,
t ∈ [1, 2): the 5 × 1 × 1 GEMM at t = 1). Present since ISSUE-006.

**Why it is no batch.** A loop that runs once is a constant on D (Lemma 0), and a subscript
constant on D is a fixed index, like a constant subscript (SQ-4). A batch coordinate is read
through a loop of two or more iterations by all three operands (§16.2); without one, P* is
empty and the nest is one GEMM.

**Fix.** `recordPrefix` keeps as rows of P* only the prefix subscripts that vary on the domain;
a constant one stays a fixed index of each operand (its dimension's `constantIndex`, no batch
terms). The analysis by groups already did so (its batch loops run two or more times), and so
did the executable specification (its domain forms drop the loops that run once).

**Tests.** @constant_prefix (5 × 1 × 1, not batched, every subview at index 1); executed
`bv_const_prefix`; showcase ext_025.

### 18.4 A stepped loop whose lower bound is a max (false refusal; found by the stronger oracle)

**Defect.** The source iteration domain came from MLIR's `addAffineForOpDomain`, which adds the
stride (x − λ) mod σ = 0 only for a lower bound of one constant; for `max(...)` with σ > 1 it
approximates D by every integer between the bounds. The regions hold exactly the loop's
iterations, so they need not cover that superset, and `checkSourceCover` refused the nest — e.g.
when such a loop and another drive one coordinate (K = k1 + k2 with
`k1 = max(-2, 0) to 8 step 4`). Sound — only refusals; the normalized domain the lowering uses
was exact — but a refusal of nests inside the domain of §1.

**Fix.** For such a loop the domain takes the stride from the folded λ (a local
q = (x − λ) floordiv σ and the equality x − λ − σ q = 0, as MLIR does for one constant), −λ
computed checked: the domain D of §1 exactly.

**Tests.** `affine_gemm_p0_views.mlir` @minmax_bounds_split_k and `affine_gemm_batch_views.mlir`
@batch_stepped_split, both refused before the fix; the oracle.

### 18.5 The oracle, strengthened

- Loops are emitted — on a random stream of their own, so a seed's nests are unchanged — with
  redundant constant bounds, max(λ, λ − d, ...) and min(υ, υ + d, ...), the effective bound in
  any position.
- A lowered batched nest's batch loops are compared with the nest's by their iterations (origin,
  step, trip count), no longer by the text of their bounds.
- Two mutations: `role_in_k`, a loop of I or J added in proportion to both factors' K
  subscripts (a batch loop the positional reading has drive two coordinates), and
  `batch_once`, the batch loop running once (a fixed index; in half of them the loops of two
  roles run once too).
- Its lit test passes the build's own `systolic-opt` (`--opt systolic-opt`, which lit
  substitutes). Before, the oracle ran the first `systolic-opt` on PATH, so the debug tree's lit
  run had checked the release binary (the 40-seed sweeps always named the debug binary).
- On the pre-fix tree (the debug build of commit `39b4d6d`) this oracle reports, for seeds 1–12,
  372 mismatches: 207 lowered batch loops that do not run the nest's iterations (18.1) and 165
  refusals of valid nests — among them 12 `role_in_k` nests refused with "two logical
  coordinates are driven by the same loop" (18.2) and the stepped max lower bounds (18.4). 18.3
  needs a positional prefix together with two unit extents, which the generator does not
  produce (no hit in 12 seeds); it is covered by the lit, executed and showcase tests above.

### 18.6 Verification (2026-10-16; logs in `~/work/integration-backups-20261016/review-fix-validation/`)

| test | result |
|---|---|
| check-systolic, release and debug (assertions) | 102/102 and 102/102, run with `count` and `not` built from the LLVM 24 sources and no `/usr/lib/llvm-18` on PATH (the runs of 16.5 and 17 had found those two helpers in `/usr/lib/llvm-18/bin`: version-independent programs, but the toolchain rule is LLVM 24 only) |
| executed (`SystolicCC/affine_batch_views.c`) | 10 kernels (+ `bv_j_in_k`, `bv_i_in_k`, `bv_const_prefix`: 4, 4, 2 invocations) equal to the CPU reference on the soft runtime and the mock board; `--verify` bitwise 10/10 |
| oracle, debug build, 40 seeds × 400 (seeds 100–139) | 16,000 nests, 11,774 accepted: 2,325 batched (391 with a factor's view moving with the batch), 489 with A and B both transposed, 10,685 with a max/min loop bound (1,206 batched nests whose batch loop has one), 315 `role_in_k` and 451 `batch_once` nests — **0 mismatches**; 1,200 nests without the permission (seeds 100–102): nothing lowered |
| showcase (172) | rc 0: 165 pass + 7 expected_refusal; ext_023..025 (new) offloaded and verified (4, 4, 2 invocations); no existing case changed |
| regression gate | rc 0; ISSUE-001..007 reproducers pass; 175 unchanged against baseline 20261015-160426 (not re-saved); 168 pass + 7 expected_refusal |
| 800 generated cases | every verdict as in the Phase 2 run (787 pass, 2 expected_refusal, 6 known_false_negative, 5 spec_open); one predicted reason changed with the compiler's: `gen_de03cb2b804b` (`C[i + j][3 + 3j]`, `i` of one iteration; UNDETERMINED, SQ-5) is refused with the SQ-5 reason instead of "loop 4 drives I and J" (18.2) |
| `test_permissions.py` | 48/48 |
| FPGA board | NOT RUN (no board) |
