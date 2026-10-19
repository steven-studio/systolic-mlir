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
loop does. What was required here until 2026-10-16 -- a batch loop drives no I, J or K
(`C[b][b + i]` was PROVEN_INVALID), and the three operands read the coordinate through one and
the same expression -- is **superseded**: by §16 (ISSUE-007) each operand reads a batch
coordinate through its own expression, and by §19.3 (ISSUE-008) a batch loop may offset any
operand's role dimensions, C's included (`C[b][b + i][j]` and `C[i + k][j]` are lowered); the
condition is G2 per batch value (§19.1). A batch loop that sat between I/J/K loops moves
outward. When distinct batch values touch distinct C elements that is a legal interchange; when
their C windows overlap (`C[i + k][j]`: (i, k) = (1, 0) and (0, 1) both update C[1][j]) the move
reorders the accumulations into one element and rests on the reassociation permission (§6)
alone, as §19.1's floating-point paragraph says. The regions of §8 constrain only I, J, K, so
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
- **A and B the same memref** (read-only) is allowed through one view or through two: when the
  first factor's (I, K) element map and the second's (K, J) element map coincide, one tensor
  serves both operands; when they differ (`A·Aᵀ`, `A[i][i + k] * A[k][j]`, `A[i][2k] * A[2k][j]`,
  `A[3 - i][k] * A[k][j]`) each factor is its own read-only view of the shared base with its own
  `bufferization.to_tensor ... restrict` (since 2026-10-18, §19.12). The two `restrict` tensors
  alias; what keeps that sound is an invariant of the lowering, not of bufferization: neither
  tensor is writable, every tile of A and B is staged through `bufferization.to_buffer read_only`
  and a copy from its own view, and C is provably disjoint from the shared base (one root buffer
  for an input and the output is refused, "A may overlap C"), so no write can reach either view
  through any alias. Between 2026-10-16 and the morning of 2026-10-18 two different views were
  refused VALID_BUT_LOWERING_UNSUPPORTED, and for a few hours an orientation search (each GEMM
  coordinate enumerated upward or downward) made some pairs one tensor at the price of writing C
  through a reversed view; the two views supersede it (C is written at its own view).

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

## 19. The generalized affine GEMM mapping (2026-10-16)

The decisions of 2026-10-16 (after the Phase A analysis): operand views are general affine
functions of the operand's roles and of the batch loops -- skewed and merged dimensions,
transposes, batch-dependent offsets in A, B **and C** (no C purity); C is legal when its view is
injective per batch; the batched dot products of SQ-5 are not refused a priori; general affine
loop domains are accepted only with a proof that their integer points correspond to the GEMM's
(batch, I, J, K) points. Geometric (rank) conditions are a screening, never a proof.

### 19.1 Definition: a batched GEMM of affine views

D is the finite set of iteration vectors; F_X(x) = M_X x + m_X (X = A, B, C) are the accesses.
With A and B read-only and disjoint from C (§7) and the reassociation permission (§6), the nest
leaves C = C0 + Σ_{x∈D} e_{F_C(x)} · A[F_A(x)] · B[F_B(x)].

The nest is a **batched GEMM of affine views** iff there are a box Z = Π_t [0, n_t) (n_t ≥ 2), an
integer affine **bijection** ξ: Z → D, a partition of Z's coordinates into P (batch), I, J, K,
role ordinals R̂(z) (a bijection of the role's coordinates onto [0, E_R); an absent role has
E_R = 1) and integer affine views V_C(p, I, J), V_A(p, I, K), V_B(p, K, J) such that

- **(G1)** F_X(ξ(z)) = V_X(p(z), R̂…(z)) for X = A, B, C — the per-iteration update is the GEMM
  slot's update: output invariance, A and B paired at the same logical K (their physical
  indices may differ), the (batch, I, J, K) decomposition;
- **(G2)** for every batch value p, (I, J) ↦ V_C(p, I, J) is injective on [0,E_I) × [0,E_J): a
  tile reads and writes each C element once (across batches C may overlap: the batch
  iterations run in order);
- **(G3)** every index lies inside its array (checked int64);
- **(G4)** C0 (the C load and store coincide), §7, §6;
- **non-degeneracy**: a batched nest has at least one of I, J, K. (Every MAC nest is a batch
  of 1 × 1 × 1 products with ξ the identity and all coordinates batch: without this the
  definition would be vacuous. Such a nest is an element-wise product, not a GEMM. A nest of
  one iteration, no batch, is the 1 × 1 × 1 GEMM of extent-one coordinates, SQ-4.)

**Theorem 6 (equivalence) [proved].** Under G1–G4 the nest and the batches of
C′_p += A′_p · B′_p, run in order, produce the same C as exact sums. *Proof.* ξ is a bijection,
so the nest's updates are those of the slots (p, I, J, K) of the box, each once (G1); within a
batch the tile's C elements are distinct (G2), so each receives exactly its K-sum; the batches
are sequential, so contributions of different batches to one element add up. ∎ Theorems 1 and
5 are the special cases of role-pure views (one role per dimension) and pure C.

*Floating point.* Theorem 6 is about exact sums. In floating point the lowered code adds the
same rounded products in another order -- the reassociation the permission of §6 allows, now
also across batch iterations whose C windows overlap -- and the arrays may differ beyond
reassociation: a K-sum started at +0.0 (the sign of a zero), a fused multiply-add, a wider
accumulator, flush-to-zero. The executed tests use integer-valued data, for which every
order gives the same bits; with general data the comparison needs a tolerance.

Real-space invertibility is not required (a role split over several loops makes ξ⁻¹ ∘ τ a
non-square map); what is required is the bijection on the finite Z, and the reduction of an
output (p, I, J) is its fibre — exactly the K slots.

### 19.2 Structure: roles are loop directions, found by readers

**Theorem 3 applied to the slot map.** For a box D with every loop of two or more iterations,
τ = (slot map) ∘ ξ⁻¹ is an affine bijection between boxes, so (S1) every loop drives exactly one
of the batch coordinates, I, J, K and (S2) the loops of each are a signed mixed radix.
*Readers.* By G1, a loop driving I is not read by B (V_B does not depend on I); one driving J is
not read by A; one driving K is not read by C. Hence a loop read by A, B and C drives a batch
coordinate in **every** decomposition; the classification AC → I, BC → J, AB → K, ABC → P is the
decomposition with the fewest batch coordinates (every other valid one demotes role loops to
batch loops); a loop with one reader is a broadcast (§10). On a box domain the reduction
lattice is therefore spanned by the K loops' axes: a direction that keeps C fixed but is not
an axis combination of K loops (Test 2's d = (5, −3, 1)) joins different batch iterations.

*Demotion.* Any loop may be made a batch loop (its terms become batch offsets; per batch its
coordinate is fixed), and a loop read by one operand may be a role along which the other
operand's view is constant -- a stride-0 view, which 19.1 admits (V_B need not depend on K).
Hence on a box domain a nest is a batched GEMM of affine views iff some loop of two or more
iterations is **not** read by all three operands (with C0 and the bounds): a loop read by
exactly two operands is that role with every other loop demoted (per batch that one loop is a
dense role, and C's view of it is injective because C reads it, or it is K and C' is one
element); a loop read by A only is K with B constant along it (C[i][j] += A[i][k] B[i][j] is,
per (i, j), the dot product of A's row with a constant), one read by B only is K with A
constant along it, one read by C only is I (or J) with A and B constant along it (C'(I) +=
a scalar, injective because C reads it). The executable specification searches these
decompositions (casespec.generalized_gemm_on_domain: every assignment of the loops to P, I, J,
K that G1 allows, each role one dense coordinate of its loops, C injective per batch). The
recognizer constructs only the canonical decomposition (the fewest batch coordinates, no
stride-0 view); a nest whose canonical decomposition fails -- a role that is not one dense
coordinate (A[i][k0 + k1], a split radix with gaps), two coordinates in one role, a broadcast,
C not injective (G2) -- is still a batched GEMM under another decomposition and is refused as
VALID_BUT_LOWERING_UNSUPPORTED (not PROVEN_INVALID); an operand-specific decomposition (SQ-1,
A[4 i0 + i1] with C[8 i0 + i1]) is refused UNKNOWN while the question is open. Only a nest
whose loops are all read by A, B and C (Test 2 on a box, a batched product with
M = N = K = 1) has no decomposition at all: PROVEN_INVALID, whichever analysis reads it (the
positional form's dense partition refuses it too, since 2026-10-18).

*Non-box domains.* The readers rule is about the box's own loops; on another domain the
readers are taken after the parametrization ξ, on Z's coordinates. In the original counters a
loop read by all three operands may then drive a role: C[i][j − i] += A[i][k] · B[k][j − i]
with i ≤ j < i + N is one GEMM over (i, j − i, k).

### 19.3 The legality of C

G2 replaces C purity. With the C view V_C(p, I, J) = λ_I I + λ_J J + (batch terms) + c (λ_I, λ_J
the per-dimension coefficients), injectivity on the box is decided exactly (IntegerLattice.h,
injectiveOnBox): rank [λ_I λ_J] = 2 is injective; λ_I = 0 needs M = 1 (λ_J: N = 1); for
λ_I = a u, λ_J = b u (u primitive, g = gcd(a, b)) the smallest collision is (b/g, −a/g), so the
view is injective iff |b|/g ≥ M or |a|/g ≥ N. Re-decided with it:

- **case_087**, C[i + j][j] += A[i + j][k] · B[k][j]: j is read by all three (batch); per j the
  GEMV C′(I) = C[I + j][j], A′(I, K) = A[I + j][K], B′(K) = B[K][j]; C′ injective, and different
  batches write disjoint columns. **A batched GEMM** (was refused by C purity).
- **case_127**, C[i + k][j] += A[i][k] · B[k][j]: k is read by all three (batch); per k the
  rank-1 update C′(I, J) = C[I + k][J] += A[I][k] · B[k][J] (K = 1), injective per batch; the
  windows of different batches overlap and add up in order. **A batched GEMM** (was refused).
- C[i + j] += A[i][k] · B[k][j] (1-D C, M, N ≥ 2): (0, 1) and (1, 0) write C[1] — G2 fails for
  the canonical roles (a tile would lose an update). Demoting J to a batch loop gives the valid
  decomposition C′_j(I) = C[I + j] (a GEMV per j): mathematically a batched GEMM, which the
  recognizer does not construct (it does not demote) — VALID_BUT_LOWERING_UNSUPPORTED.

### 19.4 The integer lattice

Every direction is taken on the loops' ordinals z (x_s = lower_s + step_s z_s, coefficients
times steps): the iteration points are then the lattice Z^n itself, and a kernel vector is a
step between iteration points. The tools (include/Systolic/IntegerLattice.h, checked int64 —
an overflow is refused, never wrapped): rank and |det| and a basis of the integer kernel by
unimodular column operations (the kernel in Hermite normal form, unique for the lattice);
primitive vectors; the common direction of several linear forms (a role's canonical form,
SQ-1 excluded); Theorem 3 (S2) for one coordinate; the injectivity criterion of 19.3.

### 19.5 Layer 1: the geometric screening (necessary conditions only)

screenAffineMatmulGeometry (printed by --test-affine-matmul-discovery=geometry=true) computes,
on the ordinals: the readers and class of every loop of two or more iterations; the ranks of
C's and of all subscripts in the full loop space and in the **role space** (the batch loops
removed: since D = D_P × D_R is a product, fixing the batch values and taking the quotient by
the batch coordinates give the same vectors — legitimate exactly when the batch loops are the
readers-ABC loops of a product domain); C's integer kernel; the reduction loops; the kernel
directions that join two points of the box and are not reductions; and the task statement's
conditions A–D on operands of two dimensions read as stored. It decides nothing.

**Conditions A–D are neither sufficient nor necessary** (each case computed by the screening,
affine_gemm_geometry.mlir, and checked by enumeration):

| case | A–D (full space) | A GEMM of affine views? |
|---|---|---|
| Test 2 | all hold, d = (5, −3, 1) | no: every loop is a batch loop (19.6) |
| Test 3 | all hold, d = e_k | not one GEMM: a batch of dot products (19.6) |
| C[i][j] += A[i][k0 + k1] · B[k0 + k1][j] | all hold | not one GEMM (K = 1 is summed twice, S2); a batched GEMM only with k0 or k1 as a batch loop (19.2), not constructed |
| GEMV C[i][0] += A[i][k] · B[k][0] | A and D3 fail | yes (N = 1, SQ-4) |
| C[4i + j] += A[i][k] · B[k][j], j < 4 | not two dimensions | yes: a flat, injective C view |
| C[i][j] += A[i + k] · B[k][j] | not two dimensions | yes: a Hankel (overlapping) A view |

In the role space the batch-separated ranks expose the structure: Test 2's role space is empty;
Test 3 keeps only k (rank 1) — one dot product per (i, j).

### 19.6 The three specified tests

**Test 1, C[i][j] += A[i][i + k] · B[j + k][j]** (i, j, k ∈ [0, 4) in the tests).
*Mapping.* Ic = i, Jc = j; Ia = i, Ka = i + k; Kb = j + k, Jb = j. *Screening.* Readers AC, BC, AB:
I = i, J = j, K = k; rank(cI, cJ) = 2, the six vectors rank 3, C's kernel e_k with aK·e_k =
bK·e_k = 1, D holds — in the full and the role space alike. *Decomposition.* τ = (i, j, k), a
bijection onto the box; V_C(I, J) = C[I][J] (injective), V_A(I, K) = A[I][I + K],
V_B(K, J) = B[J + K][J]: the reduction views are offset by the output coordinate (skewed: A's
dimension 1 combines I and K, B's dimension 0 combines K and J); Ka ≠ Kb physically, the same
logical K at every iteration. *Bounds.* A's column I + K ≤ M + K − 2, B's row J + K ≤ N + K − 2.
*Addresses.* For row-major A[M][M+K−1] the view is offset 0, strides (M + K, 1); for
B[N+K−1][N] strides (N, N + 1) — a strided view (memref.reinterpret_cast), not a subview.
**A GEMM** (refused before 19: "reads one logical coordinate in two of its dimensions").

**Test 2, C[i+j−2k][i+2j+k] += A[i+j−2k][i+j+k] · B[i+j+k][i+2j+k].** *Mapping.* Ic = Ia =
(1,1,−2)·x, Jc = Jb = (1,2,1)·x, Ka = Kb = (1,1,1)·x. *Screening.* On a box every loop is read by
A, B and C: all three are batch loops, the role space is empty. In the full space A–D hold:
C's integer kernel is d = (5, −3, 1) (aK·d = bK·d = 3), but d is no loop direction.
T = (Ic, Jc, Ka) has |det| 3, T(Z³) = {K ≡ I (mod 3)}: output rows of different residues mod 3
use disjoint reduction residues. On i ∈ [2,10), j ∈ [0,6), k ∈ [0,2) (every index ≥ 0; from 0
the row index i + j − 2k is negative: out of bounds) the 96 iterations write 87 outputs, 78 of
them once and 9 twice, scattered in a 15 × 19 range. **Not a GEMM on any box domain**: by 19.2
the only decomposition is all-batch (element-wise), a counterexample to the sufficiency of
A–D. *The domain on which it is one.* u_I = (1,0,−1) spans B's kernel, u_J = (−1,1,0) A's, d C's;
on D* = {x0 + a u_I + J′ u_J + b d : (a, J′, b) ∈ box} — an index-3 lattice parallelepiped
(det [u_I u_J d] = 3; i + j + k is constant mod 3) — the nest is the GEMM
C′(a, J′) = C[c0 + 3a][c1 + J′] += Σ_b A[c0 + 3a][c2 + 3b] · B[c2 + 3b][c1 + J′]
(strided views, stride 3; c0, c1, c2 the indices at x0), with reduction direction d: the
reparametrization (a, J′, b) ↦ x is a bijection onto D*, so the nest over D* and the program
with loops (a, J′, b) perform the same updates. D* is not the set of integer points of its convex
hull but that set intersected with the lattice i + j + k ≡ c (mod 3): 8 of the hull's 12 integer
points at M = N = K = 2, 216 of 516 at 6, 6, 6, every extra point writing a C row of another
residue, which the GEMM never touches. Neither its bounds nor a real-polyhedral argument suffice:
an affine.for nest enumerating D* needs a congruence (a stride or a mod in a bound), and
recognizing it needs a lattice parametrization of the domain (19.7).

**Test 3, C[i][j] += A[i][j + k] · B[i + k][j].** *Mapping.* Ic = Ia = i, Jc = Jb = j; Ka = j + k,
Kb = i + k. *Screening.* i and j are read by all three operands (batch loops), k by A and B
(the reduction); A–D hold in the full space, the role space keeps k alone (rank 1).
*Decomposition.* τ = ((i, j), k): per batch (i, j) the dot product C[i][j] += Σ_K A[i][j + K] ·
B[i + K][j] with the views A′(0, K) = A[i][j + K] (1 × K) and B′(K, 0) = B[i + K][j] (K × 1), offsets
affine in the batch counters; C′ is one element (injective). **A batched GEMM with M = N = 1**
(19.8). *Not one GEMM:* the only reparametrization that separates the factors is K′ = I + J + K
(A′(I, K′) = A[I][K′ − I], B′(K′, J) = B[K′ − J][J]), whose domain {0 ≤ K′ − I − J < K} is not a
box (S1 fails for the column of i) -- on the box. (On the skewed domain −i − j ≤ k < K − i − j the
nest is that single GEMM of the views A[I][κ − I], B[κ − J][J].) Precisely (checked
exhaustively): with integer-affine operand
views A[α(I, κ)], B[β(κ, J)] -- even with arbitrary zero masks -- there is no single GEMM,
already for M = N = 2, K = 1; under 19.1 the only decomposition is batch = (i, j), K = k. (With
gathered, zero-padded operand matrices, one column per iteration (R = MNK), every MAC nest is a
single GEMM: data replication, not a view of the operands -- not a mapping this compiler
considers.)

### 19.7 Non-box loop domains

Accepted only through a parametrization ξ: Z → D proved to be a bijection from a box.
*Constant-extent parallelotopes* (each loop's lower and upper bound are the same affine form of
the outer counters plus constants, any step, max/min of such forms with one linear part):
x_s = ℓ_s(x_<s) + a_s + σ_s z_s is lower triangular with diagonal σ, a bijection onto D that
preserves the iteration order; the analysis then runs on z (the shifted reduction t ∈ [i, i + K)
is the GEMM over k = t − i) -- implemented on 2026-10-18 as the domain normalization of §19.12,
which rewrites such a nest onto its box before any analysis. *Lattice parallelepipeds* such as Test 2's D* need ξ from the
accesses' kernels and an exact integer-set equality D = ξ(Z) (Presburger) -- on the real
parallelepiped {0 ≤ cI·x < 3a, 0 ≤ cJ·x < b, 0 ≤ aK·x < 3c} without the congruence, Test 2 is
a batched GEMM whose batch is I mod 3; *other* domains
(triangular: extents that vary) have no constant-extent parametrization, and the implementation
labelled them UNKNOWN, never PROVEN_INVALID, until 2026-10-18 (§19.13 decides a band whose exact
integer set is a parallelotope, and proves a non-symmetric one invalid). Some are provably not GEMMs
-- every affine image of a box is centrally symmetric (z ↦ z_min + z_max − z maps the box onto
itself, so ξ(Z) is symmetric about ξ of the box's centre), and the triangle {0 ≤ i ≤ j < 8} is
not ((i, j) = (0, 7) is in it, its mirror (7, 0) is not), so no ξ exists and the specification
says REJECT for it (showcase mlir_009) -- the specification's proof, which the compiler makes
too since 2026-10-18 when the accesses tell the iterations apart (§19.13.4). Before 19 every non-box domain was labelled PROVEN_INVALID with the argument
"no bijection onto the GEMM's box can come from a non-box domain (Theorem 3)", which is false
for parallelotopes (Theorem 3 is about maps between boxes).

### 19.8 SQ-5 and Test 3

A batched nest whose per-batch GEMM has two unit extents (Test 3: M = N = 1; the
"reduction index in the output" nests: N = K = 1) satisfies 19.1: it is mathematically a batched
GEMM. Recognition (the readers analysis) and the operand views (batch-dependent offsets,
ISSUE-007) already handle it; it was stopped only by the SQ-5 refusal in the dense partition.
Lowering it means removing that refusal: the batch loops run one 1 × 1 × K (or M × 1 × 1) GEMM
per batch value, partitioned by the planner like any other (§8: several tiles when M exceeds an
array or K exceeds k_max) — correct, and inefficient on the arrays (the decision is recorded in
SPEC_QUESTIONS.md). Three unit extents per batch are not SQ-5 but an element-wise product,
refused (19.1).


### 19.9 Implementation: Phases B and C (2026-10-16), and the five outcome categories

**Phase B** (commit `8cb8b59`) built the tools of 19.4 and the screening of 19.5 with their own
tests (`integer_lattice.mlir` by hand; `integer_lattice_random.mlir` against an independent
Python implementation -- fractions for ranks and determinants, enumeration for kernels, mixed
radixes and injectivity; `affine_gemm_geometry*.mlir`). Nothing of Phase B decides a nest.

**Phase C** (recognition; ISSUE-008) keeps every earlier decision path and adds one:

1. The positional form, then the analysis by groups, unchanged: every nest they recognized keeps
   its logical mapping, views and diagnostics.
2. When the analysis by groups refuses a nest **only** because a subscript is not role-pure --
   C mixing a batch loop with a role loop, an operand reading one group in two dimensions, a
   group read by all three operands without a batch loop (`relaxableRefusal`) -- the **readers
   analysis** (`byReaders`) decides instead, from a reset result: (a) roles by readers (19.2);
   a loop with one reader is a broadcast, no role at all an element-wise product; (b) each
   role's direction, the common direction of the subscripts restricted to its loops
   (`lattice::commonDirection`; not proportional: separate coordinates, or SQ-1); (c) each
   operand dimension's coefficients on its two roles, its constant and its batch terms
   (`OperandLayout::roleCoefficients`, `dimConstants`, `batchCoefficients`); a layout with a
   dimension on both roles, or a role on several dimensions, is `general`; (d) each role a
   mixed radix of its loops' ordinals (`lattice::mixedRadixExtent`, 19.4), with its scale; (e)
   G2 by `lattice::injectiveOnBox` with λ = multiplier × scale; (f) the batch rows of P*: C's
   batch-only dimensions, and one row per batch loop no such dimension covers.
3. Every dimension of every operand is checked on the domain to be its view of the coordinates
   plus its batch terms (`dimensionsAreViews`, G1, checked int64).
4. A positional reading in which a loop drives two coordinates is never lowerable; it is no
   longer restored when the analyses refuse the nest -- their refusal is the reason.
5. The dense partition's SQ-5 refusal is removed (19.8).
6. A role-pure layout is lowered as before (`memref.subview`, batch offsets per dimension, now
   also on C's role dimensions); a general one is refused by the lowering, VALID_BUT_LOWERING_
   UNSUPPORTED, until its `memref.reinterpret_cast` view exists (Phase D).

The executable specification decides the same nests independently of the compiler, by
enumeration (`casespec.generalized_gemm_on_domain`: roles by readers on the box, every role an
arithmetic progression taken once per point of its loops, (I, J, K) a bijection onto the box per
batch, C injective per batch, one shape for every batch), and its predictor mirrors the
compiler's rules (`predict_by_readers`, `groups_or_readers`) so that the regression gate notices
any behaviour change. The oracle (`affine_gemm_mapping_oracle.py`) decides random nests the same
way and checks every accepted nest's views address by address.

**The five outcome categories**, kept apart as the request asks:

| category | how it is recorded | examples |
|---|---|---|
| mathematically not a GEMM | specification REJECT; compiler PROVEN_INVALID (or UNKNOWN where it does not decide) | every loop read by all three operands (element-wise: Test 2 on a box, case_089, ext_027, a batched product with M = N = K = 1 -- PROVEN_INVALID); out-of-bounds subscripts (PROVEN_INVALID); a sum, a third factor, no accumulation (PROVEN_INVALID); a non-box domain whose updates are not centrally symmetric (the triangle, mlir_009, a clipped parallelogram: REJECT by 19.7's symmetry argument applied to the updates, `casespec._no_gemm_proof` -- an asymmetric set whose accesses do not tell the iterations apart can be a GEMM, the remainder tile: OUT_OF_SCOPE `non_box_unnormalized`; the compiler said UNKNOWN until 2026-10-18 and makes the argument since -- PROVEN_INVALID when the accesses tell the iterations apart (rank n), the factors and C are three buffers, the bounds are linear forms within 64 bits and the set is not centrally symmetric, UNKNOWN otherwise, §19.13.4) |
| explicitly outside the specification | OUT_OF_SCOPE, DOCUMENTED_LIMITATION | double or mixed precision, operands that may alias C, alpha ≠ 1, an initialisation inside the nest, a repetition loop inside a reduction loop, an operand shared by every batch, a nest whose every loop is a repetition loop around one scalar multiply-accumulate (no GEMM loop to anchor on; the all-once nest is the 1 x 1 x 1 GEMM and is offloaded) |
| recognition not implemented | OUT_OF_SCOPE (`demotion`, `broadcast`); UNDETERMINED (SQ-1); compiler VALID_BUT_LOWERING_UNSUPPORTED or UNKNOWN | a batched GEMM only under another decomposition (19.2): a loop read by one operand (case_090, case_095, case_123–125: a stride-0 view or a batch loop), two coordinates in one role, `A[i][k0 + k1]`, C not injective per batch (`C[i + j]`); SQ-1 (UNKNOWN); Test 2's D* written as its own loop nest (19.7, 19.12: a lattice parallelepiped, not normalized onto a box; every readers verdict is UNKNOWN on a domain left non-box), a strip-mined loop with a remainder tile (`k = 2 k1 + k0` on an asymmetric set: a GEMM the accesses do not tell apart, UNKNOWN, §19.13.4); a repetition loop the lowering cannot move outward (UNKNOWN: the mapping is not analysed) |
| lowering not implemented | ACCEPT, FALSE_NEGATIVE; compiler VALID_BUT_LOWERING_UNSUPPORTED | none known since Phase D (19.11): every GEMM of affine views the recognizer constructs with three distinct operands, or with one array as both factors (§7), is lowered -- Test 1 (ext_026), case_088 (`C[2i + j][j]`), case_126 (`A[i][i + k]`), a flat injective C (`C[4i + j]`), a Hankel view (`A[i + k]`) were in this row between 2026-10-16 and 2026-10-18. One array as both factors through two different views (`A·Aᵀ`, `A[i][i + k] * A[k][j]`) was in this position until 2026-10-18 (refused VALID_BUT_LOWERING_UNSUPPORTED, OUT_OF_SCOPE `same_memref_views` in the specification); it is lowered through two read-only tensors since (§7, §19.12) |
| hardware or runtime not supporting it | the driver's refusal | double arrays (the datapath is fp32); a floating-point nest without the reassociation permission (§6); the FPGA board itself was not run (NOT RUN) |

**Verification of Phase C** (final run 2026-10-18 after the review fixes; LLVM 24 toolchain
only; logs in `~/work/integration-backups-20261016/phaseC-validation/`: `showcase2`, `oracle2`,
`gate3`, `test_permissions2.log`, the lit logs in the session scratchpad):

- lit, release and debug (assertions) builds: 108 tests, 107 passed, 1 expectedly failed
  (ISSUE-008's XFAIL reproducer of the open lowering, Phase D);
- the oracle on the debug build, 40 seeds × 400 nests and 3 seeds without the reassociation
  permission: 17,200 nests, 13,823 accepted, 3,326 rejected, 51 skipped, **0 mismatches**; the new
  paths: C reading a batch loop in a role dimension 179, two unit extents 386, skewed or merged
  views (recognized, refused by the lowering) 785, C not injective (refused) 96;
- the showcase on the software runtime model: 175 cases, 165 pass, 3 known false negatives
  (case_088, case_126, ext_026: the open lowering), 7 expected refusals (SQ-1, SQ-2, SQ-3);
  case_087 and case_127 offloaded (16 and 8 invocations) and equal to their CPU references,
  Test 3 (ext_028) offloaded (16 invocations), Test 2 on a box (ext_027) refused at recognition
  and computed on the host;
- the regression gate: against the baseline of 2026-10-15 (not re-saved) case_088 and case_126
  went from pass to known_false_negative (re-decided ACCEPT; their lowering is open) and ext_026
  is new; the issue reproducers of ISSUE-001..007 unchanged, ISSUE-008's fails as its XFAIL
  says; check-systolic 107 + 1 XFAIL;
- the six standard generated batches (800 cases): 792 pass, 8 known false negatives (skewed or
  merged views, ISSUE-008), no unexpected stage, no runtime failure, no false positive; against
  the previous run 21 verdicts changed, every one explained: 8 pass → known_false_negative
  (skewed views, ACCEPT since this section), 7 SQ-5 cases (spec_open or expected_refusal → pass,
  offloaded), 6 batched products with every role of extent one (known_false_negative → pass:
  REJECT as element-wise now, refused by the compiler, where the former specification had
  accepted them and the SQ-5 refusal had refused them);
- the review's own instruments on the fixed tree: its 19 constructed cases, 244 formerly flagged
  random nests and 1,200 fresh random nests give predictor = compiler at every stage (0 flagged);
  the geometry cross-check 900 nests, 0 failures;
- the permissions test: 48 checks, 0 failed;
- the FPGA board: NOT RUN.

### 19.10 The task algorithm of 2026-10-18, line by line

The algorithm as given, on the six coefficient vectors of C[Ic][Jc] += A[Ia][Ka] · B[Kb][Jb]
(each a row of coefficients over the loop counters), with N = null(Ic, Jc):

```
if (rank(Ic, Jc) != 2) return error;                       // L1
if (rank(Ia, Ka) != 2) return error;                       // L2
if (rank(Kb, Jb) != 2) return error;                       // L3
if (rank(Ic, Jc, Ia, Ka, Kb, Jb) != 3) return error;       // L4
if (rank(Ia, Ka, Kb, Jb) != 3) return error;               // L5
if (dot(Ia, N) == 0 && dot(Ka, N) == 0) return error;      // L6
if (dot(Kb, N) == 0 && dot(Jb, N) == 0) return error;      // L7
if (dot(Ia, N) != 0 && dot(Ka, N) == 0) swap(Ia, Ka);      // L8
if (dot(Kb, N) == 0 && dot(Jb, N) != 0) swap(Kb, Jb);      // L9
...                                                        // (not given)
```

**Representation and the conversion to it.** A subscript is `LinearForm{coefficients, constant}`
over the nest's loop counters x (AffineMatmulAnalysis.cpp, extractLinearForm): exactly the
"仿射係數向量" of the statement; the constant plays no part in any line. The screening
(screenAffineMatmulGeometry) works on the loops' ordinals z, x_s = lower_s + step_s z_s, so a
coefficient vector v on x becomes v S on z (S the diagonal matrix of the steps, all nonzero).
Every line is invariant under this change of variables: rank(M S) = rank(M); null(M S) =
S⁻¹ null(M); (v S)·(S⁻¹ w) = v·w, so dot(v S, null(M S)) = 0 iff dot(v, null(M)) = 0. A loop of one
iteration is a constant on the domain (Lemma 0) and is left out ("full space": the loops of two
or more iterations); the "role space" leaves the batch loops out as well (19.5). rank is over Q
(equal to the rank over Z, lattice::columnEchelon); N is a basis of the integer kernel in Hermite
normal form, which spans the rational null space, so "dot(v, N) == 0" is decided as *v orthogonal
to every basis vector* ⇔ v in the row space of [Ic; Jc] -- independent of the basis and correct
for a null space of any dimension (`variesAlongKernel`; the lit case @alg_nullity_two has a
2-dimensional N in which A varies along one basis vector only). All arithmetic is checked
64-bit; an overflow is reported as "not screened", never a wrapped value. Transposed or
differently stored operands need no conversion: the vectors are taken as the subscripts are
written, and the two swap lines are the statement's own handling of the stored order.

**Where each line is.** Computed in AffineMatmulAnalysis.cpp, `conditions` of
screenAffineMatmulGeometry (SixVectorConditions: a, d1, d2, b, e, aVaries, bVaries, swapA, swapB,
passes), printed by `--test-affine-matmul-discovery=geometry=true` as the "task algorithm" line,
once in the full space and once in the role space, for operands of two dimensions each; checked
against an independent implementation (fractions, rational kernels) on random nests by
test/Systolic/Inputs/affine_geometry_check.py (affine_gemm_geometry_random.mlir).

| line | screening | the decision (19.2, 19.3) that carries its content | passes alone / fails alone (affine_gemm_geometry.mlir) |
|---|---|---|---|
| L1 rank(Ic, Jc) = 2 | `a` | G2: rank [λ_I λ_J] = 2 is the sufficient case of C's injectivity (19.3); the rank-1 and zero cases are decided by the gcd criterion | fails alone: @alg_fail_rank_c, C[i][i] (valid by demotion) |
| L2 rank(Ia, Ka) = 2 | `d1` | A reads I and K (readers: an AC loop and an AB loop); each role one dense coordinate (mixedRadixExtent) | fails alone: @alg_fail_rank_a, A[i + k][2i + 2k] (a GEMM of a general view: not necessary) |
| L3 rank(Kb, Jb) = 2 | `d2` | B reads K and J, likewise | fails alone: @alg_fail_rank_b |
| L4 rank(six) = 3 | `b` | exactly three roles plus batch loops (the readers classification; a fourth direction is a broadcast or a batch loop) | fails alone: @alg_fail_rank_six, B[k][l] (a broadcast: valid, stride-0 view, not lowered) |
| L5 rank(Ia, Ka, Kb, Jb) = 3 | `e` | the factors span I, K, J: K read by both factors, I by A, J by B | fails alone: @alg_fail_rank_four, B[k][i] (case_124: valid by demotion) |
| L6 A varies along N | `aVaries` | the reduction loops are read by A (a loop read by B only is a broadcast, refused) | fails alone: @alg_fail_a_varies, A[i][j] |
| L7 B varies along N | `bVaries` | likewise for B | fails alone: @alg_fail_b_varies, B[i][j] |
| L8 swap(Ia, Ka) | `swapA` | A's role dimensions in the other order: `roleDim` = {1, 0}, memref.transpose (P0.2) | @alg_both_swaps (A[k][i]); @test1 no swap |
| L9 swap(Kb, Jb) | `swapB` | likewise for B | @alg_both_swaps (B[j][k]) |

Positive cases (every line passes, no swap): @test1, @v_skew_a_only, @v_skew_c, @alg_nullity_two
(N of dimension 2, a repetition loop). Cases the lines do not apply to (operands not of two
dimensions each): @v_flat, @v_c_not_injective, every batched nest with three-dimensional
operands (the screening prints no task line; the role space of 19.5 is the analogue).

**What the lines decide, and what they do not.** The lines are necessary for a *plain* GEMM of
two-dimensional operands with three loops (I, J, K each a loop); they are implemented and
reported verbatim, and nothing of them was dropped, reordered or replaced. They are *not* the
decision procedure, because for the specification's class (19.1) they are neither sufficient
nor necessary -- each statement below is a test:

- not sufficient: Test 2 on a box passes every line (d = (5, −3, 1)) and is an element-wise
  product (19.6; @test2); Test 3 passes every line and is a batch of dot products, not one
  GEMM (@test3); the lines see no domain, so they cannot tell the box from D*;
- not necessary: the K = 1 GEMM (outer product, SQ-4) has N = {0} and fails L2–L7
  (@alg_outer_product_k1); the N = 1 GEMM (GEMV, SQ-4) fails L1; A[i + k][2i + 2k] fails L2 and
  is a GEMM of a general view (@alg_fail_rank_a); a one-reader loop fails L4, L6 or L7 and is a
  batched GEMM with a stride-0 view or a demoted loop (19.2; @alg_fail_rank_six,
  @alg_fail_a_varies, @alg_fail_b_varies); C[i][i] fails L1 and is valid by demotion
  (@alg_fail_rank_c).

The decision procedure therefore implements the lines' content in the generalized form of
19.2–19.3 (the table's third column): readers instead of the three ranks, G2 instead of L1, the
role dimensions' order instead of the swaps, the broadcast refusal instead of L6–L7, and the
domain (Theorem 3 on the box, UNKNOWN elsewhere) that the lines lack. Where the two disagree
on a case, the case is listed above with its proof, as the request asks, rather than one of
them silently adjusted.


### 19.11 Phase D: the lowering of general views (2026-10-18)

**What a general layout is, in addresses.** For an operand X of rank ρ with row-major element
strides w_d (w_{ρ−1} = 1, w_d = w_{d+1} · size_{d+1}; a dynamic leading dimension has a static
stride) the readers analysis records every dimension as

  index_d(x) = c_d + λ_{d,0} · y_0 + λ_{d,1} · y_1 + Σ_s β_{d,s} x_s

(c_d = dimConstants, λ = roleCoefficients on X's two roles in GEMM order, β = batchCoefficients on
the batch loops; G1 checks this on the domain). With the dense partition's y_r = o_r + s_r · ord_r
(offset, scale, ordinal in [0, E_r)) the element address of the view's element (r, c) is

  addr(r, c) = Σ_d w_d · index_d
             = [Σ_d w_d (c_d + λ_{d,0} o_0 + λ_{d,1} o_1)] + [Σ_d w_d λ_{d,0} s_0] · r
               + [Σ_d w_d λ_{d,1} s_1] · c + Σ_s [Σ_d w_d β_{d,s}] · x_s
             = O + S_0 · r + S_1 · c + Σ_s B_s · x_s,

affine in (r, c) with integer strides S_0, S_1 and an offset O plus batch terms: exactly a
strided two-dimensional view of the memref's buffer, whatever the memref's own dimensions. The
lowering builds it as one `memref.reinterpret_cast` of the operand to `rows x cols` with
`offset: [O]` (or the `affine.apply` of O + Σ B_s x_s over the re-emitted batch counters),
`sizes: [E_row, E_col]`, `strides: [S_0, S_1]` -- the offset relative to the buffer's base, as
the operation defines it, which is why it is applied to the function argument (identity layout,
offset 0) and never to a view. A unit extent gets stride 1 (immaterial). The staging copies and
the tile slices downstream are layout-agnostic (`bufferization.to_tensor` of a strided memref,
`tensor.extract_slice`, `memref.copy` → `memrefCopy` on the strides), so Test 1 reaches the
runtime as the same `systolic.matmul_tile` calls as a plain GEMM.

**Correctness, condition by condition.**

- G1 (every iteration's update is its slot's) is the identity above, checked on the domain by
  the recognizer in checked arithmetic (`dimensionsAreViews`).
- G3 (bounds): per dimension d the index range over the domain -- c_d plus each role's extreme
  contribution (λ s · 0 and λ s (E − 1), either order) plus the batch range -- must lie in
  [0, size_d) (lo ≥ 0 always; the dynamic leading dimension is the program's guarantee), in
  checked arithmetic; then the extreme element addresses are checked to fit in 64 bits. Since a
  valid in-bounds index tuple has a unique row-major address, every view element is an element
  the nest itself accesses, and the view never leaves the allocation.
- G2 for C: the recognizer's injectivity on the (I, J) box is on the index tuple; distinct
  in-bounds tuples have distinct addresses, so the view of C is injective: each tile reads and
  writes each C element once, and the tiles of one batch value write disjoint elements. Across
  batch values the windows may overlap (C[i + k][j]); the batch loops run in program order and
  each tile's read-modify-write of its window completes before the next (§19.3, the runtime is
  synchronous), so the overlapping updates add up in order.
- A and B may repeat elements (a Hankel A[i + k]: S_0 = S_1): a read view, harmless; the
  address-exact oracle locates a factor's element by the row (A) or column (B) the output
  fixes, so repeated elements are checked too.
- A flat C[4i + j] with j < 4: S_0 = 4, S_1 = 1, injective on the box because G2 holds
  (4 ≥ N). The same flat C with j < 5 is refused by the recognizer (G2), never lowered.
- A skew read backwards (A[i][6 − i − k]: S_1 = −1) is a negative stride, as the role-pure
  views already have (§5).
- The same explicit view is also used for a role-pure operand whose rank-reducing subview
  would be ambiguous -- a kept dimension of size one next to a dropped one (`memref.subview`'s
  rank-reduced result type does not say which size-one dimension was dropped; the verifier's
  guess by position or by strides differed from the lowering's for a batched 1 x K view with a
  stride-2 subscript, invalid IR for a valid nest: review finding 7). The reinterpret_cast
  carries the offset and strides explicitly and is unambiguous.
- Nothing changes for the role-pure views otherwise: they keep their `memref.subview` /
  `memref.transpose` / `memref.expand_shape` construction (every earlier lit expectation holds),
  so transposed, negative-stride and batched GEMMs are lowered as before.

**What is still not lowered** (as of the Phase D commit, 2d4eaee): a broadcast (stride-0) view
(§10, refused by the recognizer: a one-reader loop), a demoted decomposition (§19.2), SQ-1,
non-box domains (§19.7) -- recognition limits, listed in §19.9's table --, and one array as both
factors through two different views (§7). The last two moved on 2026-10-18, after that commit:
constant-extent parallelotope domains are normalized onto a box, and two different views of one
array are lowered as two read-only tensors (§19.12). One array as both factors through ONE view
(`A[i][i + k] * A[k][k + j]`: one `reinterpret_cast` feeds both operands) is lowered as before;
the orientation search of the review fixes below is superseded by the two views.

**Review fixes (2026-10-18, before the Phase D commit).** The adversarial review of Phase D (four
lenses: the C++ against this section, the lowering's safety on the newly lowered shapes, the
predictor's fidelity, the specification's soundness; each finding re-examined by a skeptic)
reported 14 findings, 13 confirmed, 1 refuted ("finding 7 does not reproduce": it does, with a
stride-2 batched 1 x K view). Fixed: the general view's bounds test formed one per-role sum in
plain int64 (a wrapped sum let `A[i][i + 2^61 k]` pass and emitted an offset of 2^62, or a
negative one): every term is checked now, and an offset the view type cannot state is refused;
one array as both factors -- the predictor compared subscript texts (refusing
`A[i][i + k] * A[k][k + j]`, which the compiler lowers through one view) and ignored the
direction of enumeration (accepting `A[i][k] * A[3 - k][j]` read backwards), the specification
had no rule for general views and a direction-blind one for dense views: all three now decide
"one tensor serves both" the same way (§7; the lowering tries the eight orientations, the
predictor the same eight analytically, the specification by enumeration -- superseded the same afternoon by the two views of §19.12); a nest of loops that
run once with constant subscripts is the 1 x 1 x 1 GEMM (SQ-4) and is lowered (it was UNKNOWN:
"loop 0 of the nest is used by no access"; such a loop is an iteration dimension of one value in
the recognizer and the hoist pass alike), while a nest whose every loop is a repetition loop of
two or more iterations is a documented limitation (basis `no_gemm_loop`); the readers analysis'
predictor returned a count-down loop's coordinate on the front end's ordinal and the dense check
folded the step twice (a dense split role refused); the showcase notes, the generator's mutation
notes and the README rows that still called the skewed views "not implemented yet" are reworded.
Tests: `test/Systolic/affine_gemm_shared_factor.mlir` (eight A x A nests: the emitted shared
view in each orientation, the refusals), `test/Systolic/affine_gemm_review_d.mlir` (the once-loop
nests, the two overflow nests refused UNKNOWN), four more kernels in
`test/SystolicCC/affine_skewed_views.c`.

**Tests.** `test/Systolic/affine_gemm_generalized.mlir` (Test 1, the skewed C, the flat and the
Hankel view: the emitted `reinterpret_cast` offsets, sizes and strides, and the tiles);
`test/SystolicCC/affine_skewed_views.c` (eighteen kernels executed on the software runtime model
and compared bitwise with CPU references on integer data, each offloaded call also verified
against the function as written: Test 1, the flat idiom, case_088's seed, a Hankel view,
case_126, both factors skewed and roles-swapped, a skew read backwards, Test 1 per batch value,
Test 1 at 16 x 16 x 16 over several tiles, Test 3, case_087, case_127, Test 2 over D* against
the original statement enumerated over D*'s points, Test 2 on a box staying on the host, one
array as both factors through one skewed view, the same with K read downward, `A[3 - i][k] *
A[k][j]` through one reversed tensor, and the all-once 1 x 1 x 1 nest);
the oracle (`GENERAL_VIEWS_LOWERED = True`: every lowered skewed or merged view is checked
address by address, the factors' repeated elements included); the executable specification's
predictor (a general operand's bounds per dimension); showcase case_088, case_126 and ext_026
(TRUE_POSITIVE since); `failures/issues/ISSUE-008-*/repro_skewed_views.mlir` (its XFAIL removed:
a permanent regression test). FPGA board: NOT RUN.

**Verification of Phase D** (2026-10-18; LLVM 24 toolchain only; logs in
`~/work/integration-backups-20261016/phaseD-validation/`):

- lit, release and debug (assertions) builds: 111 tests, 111 passed (the new end-to-end test,
  the former XFAIL reproducer and the two review-fix tests among them);
- the oracle on the debug build, 43 seeds × 400 nests: 17,200 nests, 13,823 accepted, 3,326
  rejected, 51 skipped, **0 mismatches** -- 785 skewed or merged views lowered and checked
  address by address, 179 with C reading a batch loop in a role dimension, 386 with two unit
  extents, 96 refused as not injective;
- the showcase on the software runtime model: 175 cases, 168 pass, **0 known false negatives**,
  7 expected refusals (SQ-1, SQ-2, SQ-3); case_088, case_126 and ext_026 offloaded (one
  invocation each) and equal to their CPU references;
- the regression gate: no regression against the baseline of 2026-10-15 (not re-saved); every
  issue reproducer passes (ISSUE-008's included); check-systolic 111/111;
- the six standard generated batches: **800 of 800 pass** (the 8 known false negatives of Phase C
  -- skewed or merged views -- pass; no other verdict changed);
- the review's random fuzz on the fixed tree: 1,999 nests, predictor = compiler at every stage;
  30 hand-written review nests (A x A in every orientation, once-loops, count-down split roles):
  predictor = compiler, and the specification's verdict consistent with both;
- `test/SystolicCC/affine_skewed_views.c`: 18 kernels equal to their CPU references on the
  software runtime model and under the mock-hardware flag, 17 offloaded calls verified bitwise
  against the function as written, nothing offloaded without the reassociation permission;
- the permissions test: 48 checks, 0 failed;
- the adversarial review of Phase D: 14 findings, 13 confirmed and fixed (above), 1 refuted;
- the FPGA board: NOT RUN.

### 19.12 Domain normalization, distinct A x A views, the exact mapping, K = 1 (2026-10-18)

The implementation task of 2026-10-18, after the Phase D commit 2d4eaee: the generalized domain
normalization for the constant-extent parallelotopes of §19.7, the lowering of one array as both
factors through two different views, an exact (integer and rational) report of the task's own
coordinate construction, and K = 1 kept eligible. Demotion decomposition is deferred (19.12.6).
FPGA board: NOT RUN; every execution below ran on the software runtime model.

**19.12.1 The family F and its lattice.** A perfect band of n loops x_0, …, x_{n−1} around one
multiply-accumulate (three loads, one product of two of them, one sum of the third with the
product, one store of the sum to the third load's memref with as many indices -- whether the
two address the same element is the recognizer's question, so a load differing by a term of a
loop that runs once is admitted) is in F when every loop s has lower
bound ℓ_s·x_{<s} + a_s and upper bound ℓ_s·x_{<s} + b_s with one and the same integer row ℓ_s
over the outer counters (several results of a `max` / `min` bound must share that row; their
constants fold to the max / min) and step σ_s ≥ 1. Its iteration set is
D = {x : a_s ≤ x_s − ℓ_s·x_{<s} < b_s, x_s − ℓ_s·x_{<s} ≡ a_s (mod σ_s)}. Let U be the
lower unitriangular (hence unimodular) matrix with U[s][u] = δ_su + Σ_{u ≤ t < s} ℓ_{s,t} U[t][u],
so that x = U x' is exactly x_s = x'_s + ℓ_s·x_{<s} for every s, and let
D' = {x' : a_s ≤ x'_s < b_s, x'_s ≡ a_s (mod σ_s)} be the box. Then:

- U⁻¹ -- the shifts undone innermost first, x'_s = x_s − ℓ_s·x_{<s} -- maps D onto D' and U maps
  D' onto D: for fixed outer values, x_s = ℓ_s·x_{<s} + a_s + tσ_s iff x'_s = a_s + tσ_s, so the
  s-th coordinate runs over the same number of points in the same order; the bijection preserves
  the lexicographic (iteration) order. This is integer-point coverage, not real invertibility:
  both sets are enumerated coordinate by coordinate with the same t.
- The lattice: D is the image under U of the box lattice {a + Σ_s t_s σ_s e_s, 0 ≤ t_s <
  ⌈(b_s − a_s)/σ_s⌉}; origin a (the constant lower bounds), basis the columns of U·diag(σ),
  membership x ∈ D iff U⁻¹x ∈ D' (coordinate by coordinate: the box test), bounds a_s ≤ x'_s < b_s.
- Coverage of the accesses: F(x) = M x + f becomes F(U x') = (M U) x' + f -- the same element at
  the same iteration. The rewritten nest has constant bounds [a_s, b_s) step σ_s and access maps
  M U, so the box pipeline (§19.1–19.11: readers, G1–G4, partition, views, tiles) runs on it
  unchanged and its verdicts and tiles are those of the original nest. Every coefficient of M U,
  of U and of the bounds is formed in checked 64-bit arithmetic; an overflow leaves the nest as
  written (UNKNOWN downstream).

Implementation: `normalizeParallelotopeDomains` (AffineMatmulAnalysis.cpp), run by the lowering
pass before `hoistRepetitionLoops` and by the discovery pass first. Its exact scope: the body
must be the recognizer's multiply-accumulate (any other nest is left as written, so what the
pass does not lower it does not touch); every use of a counter must be a bound of an inner loop
of the band or an index of one of the three loads or the store; every access operand a counter
of the band (no symbols); every map result a linear form. The rewritten accesses carry as
operands only the counters with a nonzero coefficient in M U, so operand membership keeps
meaning "the access depends on this counter": a repetition loop between the shift's source and
the shifted loop stays a repetition loop and is hoisted as before (`@shift_over_repetition`),
or, when the interchange is illegal, leaves the recognizer's verdict UNKNOWN as before
(`@shift_repetition_stuck`). The remark `iteration domain normalized to a box (x = U x', ...)`
states each loop's own shift (`loop 2: x2 = x2' + 1*x1` is ℓ_2 over the original counters; U's
row composes the shifts, x2 = x2' + x0' + x1'). Not in F, left as written, UNKNOWN downstream (as of
this section; §19.13 decides more of them): a triangle (`k in [i, 8)`: different linear parts), a
clipped parallelogram (`max(i − 2, 0)`), a floordiv or a symbol in a bound, a symbol in an access
(`@shift_symbol_access`), Test 2's lattice parallelepiped D* (a congruence, not a shift).

The C front end (systolic-cfront) translates the family: a loop whose initial value and bound
are affine in the enclosing counters with the same counter terms (`for (int k = i; k < i + 4;
++k)`, also `<=`, any positive step, under counting-down outer loops) becomes `affine.for` with
`affine_map` bounds over the enclosing counters; every bound is checked against the counter's
type at every enclosing value; subscript ranges are computed on the box (the shifts substituted
innermost first), so they are exact, not a hull; a varying trip count (a triangle) is outside
the subset ("the loop's trip count depends on an enclosing loop counter") -- superseded the same
evening: §19.13.1 translates such loops.

**19.12.2 Cases.** `test/Systolic/affine_gemm_domain_normalization.mlir`: the shifted J, K and
both; a shift read by C (a skewed C view); two chained shifts (batched, K = 1 per batch); a
shifted K of one iteration; a repetition loop between the shift's source and the shifted loop,
hoisted and run twice; one that cannot move (UNKNOWN); untouched and UNKNOWN: a triangle, a
floordiv bound, a symbolic bound, a clipped parallelogram, a symbol in an access (the triangle
and the clipped parallelogram are PROVEN_INVALID since §19.13.4); in F but not
a GEMM: `C[i + j]` under a shift (not injective after it: VALID_BUT_LOWERING_UNSUPPORTED, the
demotion candidate named), out of bounds after the shift (PROVEN_INVALID).
`affine_gemm_generalized.mlir @parallelotope_rank1` (UNKNOWN before: lowered, batched 4 x 4 x 1)
and `affine_gemm_geometry.mlir @shifted_reduction` ("not screened" before: screened and lowered
as 4 x 4 x 4) re-decided. Executed, `test/SystolicCC/affine_domain_normalization.c`: six kernels
through the front end's affine bounds -- the shifted K, the shifted J, the skewed C (4 x 7), two
chained shifts batched (2 invocations), a shift by a counting-down counter with `<=`, a shifted
loop of step 2 (stride-2 views) -- offloaded on the software runtime model and under the
mock-hardware flag, bitwise equal to CPU references, `--verify` 6 of 6; a triangle stays plain
C (translated since §19.13.1, and kept on the host as PROVEN_INVALID); an out-of-bounds shifted nest stays on the host (PROVEN_INVALID; compiled, not run: undefined
in C).

**19.12.3 One array as both factors, two views.** Representation: the recognizer's two operand
views (A's (I, K) element map and B's (K, J) element map, §19.1) stay distinct whatever their
base; equality of the views (`sameView`), never identity of the base, decides whether one tensor
serves both. Lowering: each view becomes its own `reinterpret_cast` / `subview` / `transpose`
and its own read-only `to_tensor restrict`; the tile lowering stages every A and B tile from its
own view's buffer (`to_buffer read_only` and a copy), the staging every operand already goes
through -- no duplicate of the matrix is materialized, and no copy is inserted to make the shared
base acceptable. Soundness: §7's invariant. Kept refused: C the shared base (`A[i][j] += A[i][k]
* B[k][j]`: "A may overlap C", UNKNOWN). Tests: `affine_gemm_shared_factor.mlir` (eight nests:
one view; two views skewed, reversed, transposed, strided; C the base refused),
`test/SystolicCC/affine_shared_factor.c` (seven kernels executed: A·A, A·Aᵀ, Aᵀ·A, a skew read by
one factor, a stride-2 view against its transpose, ext_007's offset view, the rows reversed
against the whole memref -- bitwise equal, `--verify` 7 of 7). The executable specification and
the showcase no longer refuse (`same_memref_views` removed; ext_007 re-decided ACCEPT).

**19.12.4 The exact mapping (reported, not decided).** For a nest whose C has two indices, one
loop per role and no batch coordinate, the discovery pass prints the task's construction over
the loops of two or more iterations (a loop of one iteration is a constant on the domain, Lemma
0; the counters are those of the nest as analysed, i.e. after normalization): T = [c_I; c_J; q]
(C's two index rows and the reduction row), d, det T (signed fraction-free Bareiss elimination
over arbitrary-precision integers, `lattice::determinant`; `adjugate`, `unimodularInverse` and
`rowTimesInverse` give the exact rational λ = r·T⁻¹ for A's and B's rows), whether T·diag(σ) is
a signed permutation (then T(D) + d is a complete integer box in these coordinates; otherwise a
sublattice), the primitive null vector v of [c_I; c_J] with ΔI_a = a_I·v, ΔK_a, ΔK_b, ΔJ_b (A and
B must vary along v), rank(C), rank(A), rank(B) by integer column echelon, and the swaps. det T =
0 is reported as singular (rank(I_c, J_c, q) < 3). These lines are a diagnostic beside the role
mapping of §19.2, which decides; they are not a second decision procedure.

**19.12.5 K = 1.** A GEMM whose reduction has one iteration -- a reduction loop of trip 1, a
constant reduction index with no reduction loop, batched -- is recognized and lowered as before
(SQ-4, Lemma 0): the review of 2026-10-18 found no check that rejects a candidate for a singleton
reduction extent, so none was changed; the exact mapping prints its degenerate 2-D path (T =
[c_I; c_J], no v). Tests: `test/SystolicCC/affine_k1.c` (four kernels offloaded, bitwise equal to
the references, `--verify` 4 of 4, C's accumulation kept; an aliasing K = 1 nest stays on the
host; an out-of-bounds one is PROVEN_INVALID, compiled and not run),
`affine_gemm_domain_normalization.mlir @shift_k1` (a shifted reduction of one iteration).

**19.12.6 Demotion decomposition: deferred, not implemented.** A nest whose readers
decomposition fails G2 but which is a batched GEMM under another decomposition -- one of C's
index loops demoted to a batch loop, the iteration space read as the quotient by that loop:
case_090, case_095, case_123–125, `C[i + j]` under a shift -- is still refused
VALID_BUT_LOWERING_UNSUPPORTED (or UNKNOWN), the refusal naming the candidate decomposition
("a decomposition with the J (or I) loops as batch loops would be valid, and is not
constructed"). The architecture keeps its place: the recognizer's reason carries the candidate,
the executable specification's `_check_decomposition` enumerates decompositions, and §19.2–19.3
define the per-batch GEMM a demotion must produce. Nothing in 19.12 decides a demotion.

**19.12.7 Generator and oracle coverage of these families (2026-10-18, afternoon).** Until this
point no generated case had a non-constant loop bound or one array as both factors (a
regeneration of the six standard batches in memory: 0 of 800 for either; K = 1 appeared in 303,
262 of them the all-once 1 x 1 x 1 nests of the enumeration modes), and the oracle's 17,200 nests
covered none of 19.12 either. Both generators now produce them and derive the expected verdicts
independently of the compiler:

- *The MLIR oracle* (`test/Systolic/Inputs/affine_gemm_mapping_oracle.py`): a nest is still decided
  as a BOX nest by enumeration; 30% of the nests are then *emitted* on a sheared domain -- one or two
  loops with bounds shifted by an integer form of outer counters (coefficients in {−2, −1, 1, 2},
  one or two outer loops, chained shifts preferred, steps and max/min decorations kept), the
  subscripts composed with L (x' = L x, lower unitriangular) so that the emitted nest touches the
  same elements at corresponding points, which `check_shear` asserts on every sheared nest -- and
  18% of those are *triangles* (the upper bound shifted by another form or not at all: outside the
  family, expected not lowered). 12% of the nests make B read A's memref through its own view, 4%
  are built as one view (`generate_same_view`), 4% make C a view of A's memref (expected not
  lowered, "A may overlap C"). The tile's operands are named by their memref argument and, on a
  shared one, by position (retried swapped). New counters prove the coverage per run
  (`a sheared domain N (chained, with a step > 1, with a max/min bound)`, `one array as both factors
  N (two different views, one view, served by one tensor)`, `K = 1 N (without a reduction loop)`,
  and the refusals `domain outside the normalized family`, `C the base of a factor`, `out of bounds
  after a shear`, `C not injective after a shear`); the headline line counts every nest (accepted,
  rejected, skipped, refused by rule), and the lit test requires every new counter nonzero except
  "C not injective after a shear" (rare: it may be 0 in a seed). Sweep on the debug build, 40
  runs x 400 nests with the reassociation permission: 16,000 nests, 12,097 accepted, 2,694 rejected,
  30 skipped, 839 triangles and 340 aliasing outputs refused as predicted; of the lowered and
  verified nests 3,144 sheared (833 chained, 1,514 with a step > 1, 1,859 with a max/min bound),
  1,297 with one array as both factors (724 through two different views, 573 through one view, 558 of
  those served by one tensor), 1,930 with K = 1 (1,602 with no loop at all read as the reduction),
  364 out of bounds after a shear and 21 with C not injective after a shear refused; plus 3 strict
  runs (1,200 nests, nothing lowered, 64 triangles and 34 aliasing outputs); **0 mismatches** in
  all 43. The first run of the extended oracle found a compiler gap: the body test of
  `normalizeParallelotopeDomains` demanded the accumulator's load and store maps to be equal, so a
  sheared nest whose load differs by a term of a loop that runs once (constant on the domain) was
  left sheared and refused in silence -- 123 oracle mismatches over 8 seeds, all of that kind; the
  test now asks for the memref and the index count only (`@shift_acc_once_term`).
  `affine_geometry_check.py` reuses the oracle's generator: a parallelotope is screened after the
  compiler's normalization (the Python model is the box nest), and a triangle must come back "not
  screened" (checked).
- *The C generator* (`test/gemm_recognition/generator/gen_cases.py`, family `domains`): twelve
  variants of a positive case -- `shift` (one loop shifted by an enclosing counter, coefficient ±1),
  `shear` (a multiple, or two enclosing counters), `chain` (a loop shifted by a shifted loop),
  `step_shift` (a shifted loop of step > 1; rejected when the case has none), `shift_same_base`,
  `shift_batched` (the `batched` mutation, then a shift by the batch loop or another),
  `same_base` (B reads A's array through its own view), `same_view` (B's subscripts A's with the
  roles' counters moved and the I, J, K loops made one range: one element map, as the compiler's
  `sameView` requires), `k1_const` (the reduction index a constant, no K loop), `triangle` (the lower
  bound rises by the loop's step per unit of an enclosing counter, the upper does not, and the trip
  count genuinely varies: outside the front end's subset), `alias_c` and `alias_c_b` (the output is
  the first / second factor's array; no repetition loop, since the feedback grows). In the
  `enumerate` and `boundary` modes the shapes are walked with the index itself (the generic walk
  stays at 1 x 1 x 1 for its first 921,600 indices, where nothing can be shifted). The subscripts
  of a sheared variant are composed with L so that the
  nest is the same GEMM as the positive case; `casespec.Loop` carries the bounds' counter terms
  (`lb_terms`, `ub_terms`), the C text prints them (`for (int k = 2 * i + 1; k < 2 * i + 5; ++k)`), and
  the specification decides a family-F case on its *box form* (`casespec.box_form`: x = U x', the
  compiler's normalization replayed independently) -- the analysis, the prediction and the
  specification of the box, whose equivalence the generator asserts by evaluating both nests (`C`
  identical). A triangle's points are enumerated as written (bounds evaluated at the enclosing
  counters' values, `domain_points`). It is a GEMM when the readers' roles map them bijectively
  onto a box with every point repeated the same number of times: OUT_OF_SCOPE
  `non_box_unnormalized` since §19.13 (until then ACCEPT, `non_box_gemm`: a GEMM the front end
  refused for its syntax). It is REJECT `non_box_domain` only with a proof that no decomposition
  exists (its updates are not centrally symmetric, §19.13.6), and UNDETERMINED otherwise
  (`non_box_undecided`) and above 200,000 points (`non_box_unenumerated`). The generated
  triangles are built to vary their trip count genuinely. The predicted front-end refusal is the front end's own message. An
  output that is also a factor is decided by the addresses (`_alias_verdict`): REJECT
  `aliasing_not_gemm` when some product reads an element written at an earlier point of the nest
  (not the GEMM of the operands, whatever the data), OUT_OF_SCOPE `aliasing` otherwise, the
  evaluation against the factors' initial values cross-checking it. A reduction with no loop and
  constant subscripts in A and B is the K = 1 GEMM in the dense analysis (SQ-4; different constants
  in A and B, `A[i][6] * B[1][j]`, are one coordinate, not SQ-1), so `C[i][j] += A[i][0] * B[0][j]`
  carries the logical reference like any GEMM. Features record each family (`domain:parallelotope`, `bounds:shifted`,
  `bounds:sheared`, `bounds:chained`, `bounds:shifted_step_gt1`, `bounds:not_together`,
  `factors:same_base`, `factors:two_views` / `factors:same_view`, `output:aliases_factor`,
  `K1:no_loop`, `K1:loop`, the two K = 1 tags only on nests the analysis finds to be GEMMs) so a
  batch's coverage is read off `coverage.json`; the showcase set is byte-identical to a fresh
  generation (no existing expectation changed). Two batches were added to the validation gate
  script (`random 2027 domains 4 x 50`, `enumerate enum domains 2 x 50`); that script is kept with
  the validation logs under `~/work/integration-backups-20261016/generator-ext-validation/`
  (`run_gate_and_loops_ext.sh`), not in the repository, like the six-batch script before it.

### 19.13 Exact non-box domains end to end (2026-10-18, evening)

The gap this section closes: a nest whose bounds do not shift together (§19.12.1's family F
excludes them) but whose written integer points form a box -- the executable specification
accepted such a nest (basis `non_box_gemm`), the C front end refused it ("the loop's trip count
depends on an enclosing loop counter ... only bounds with the same counter terms are
supported") and the compiler never saw it. The root cause was the front end's bookkeeping, not
its translation: its value ranges (each counter's first and last value, the shift substituted
into subscripts, the overflow checks, and the interface's `leading_extent`, which the driver
uses as the memref descriptor's size and the `--verify` copy size, so it must be exact) were
computed in closed form, exact only on parallelotopes.

**19.13.1 The front end.** A loop counting up whose initial value and bound are affine in the
enclosing counters with any counter terms is translated to `affine.for` with `affine_map` bounds
(C's semantics exactly: lb, lb + S, ... while < ub). Conditions are canonicalized: `L OP R` with
L − R = a·k + g(enclosing counters) + c becomes `k OP' bound` when a = ±1 (`k - i < 4`, `i + 4 >
k`, `k + i < 8`); with |a| > 1 only for constant g (the earlier rule). Enclosing-counter terms
that cancel leave a constant bound: `k + i < 8 + i` is `k < 8` (the previous front end refused
both spellings of a condition that is not `counter OP bound`). When some loop of the nest is no
parallelotope, the subscript ranges and that loop's checks are computed by running the loops in
scope (each loop's bounds evaluated at the outer counters' values), exactly. One run may give
the counters at most 4,194,304 values, every level counted -- the outermost loop's points, plus
those of the outer two loops, and so on through the nest -- so the limit bounds the work, not
only the innermost points. Beyond it the function is refused, "the iteration domain is not a
parallelotope and running its loops takes more than 4194304 counter values: its exact subscript
ranges are not computed"; test/CFront/nonbox_bounds_unsupported.c refuses a nest of 2,048
points for the 4,196,352 values of its outer loops. The predictor of the generated cases counts
the same way (`casespec.frontend_enumeration_values`). On a parallelotope nest every check is a
closed form, a canonicalized condition's included: `for (int k = j; k - j < 4; ++k)` around a
2048 × 2048 GEMM is translated like `k < j + 4`, with the same MLIR, in 0.01 s (the first
version of this change ran the enclosing loops for such a condition and refused it at the limit
after 10 to 13 s, with a message that called the domain no parallelotope; review of 2026-10-18,
#18). The limit thus applies only to a nest with a loop whose bounds do not shift together,
however a condition is spelled.

*C's integer types* (review of 2026-10-18, #15-#17). C evaluates every integer node of a loop
header -- the initial value, the bound, both sides of the condition -- and of a subscript in that
node's own type (an operator's promoted or common type; every implicit conversion is a node of
its own). The front end computes with exact integer values, so it translates a function only
when every such node's exact value lies in its type's range at every point where C evaluates
it: no signed overflow (undefined behaviour), no unsigned wrap-around, no conversion that
changes a value. C's arithmetic, unsigned included, is then exact arithmetic, and the nest runs
C's iterations on C's elements. The ranges are exact: in closed form on a parallelotope -- the
initial value and the bound at every point of the enclosing loops, a condition's nodes at the
counter's first value and at the value that ends the loop, between which each node, affine in
the counter, is monotone -- and by running the enclosing loops for a loop whose bounds do not
shift together (within the limit above). A node that reads a counter -- the loop's own one
included, and also when its counter terms cancel (`k - i` at k = i − 1) -- is checked only where
C evaluates it: nowhere in code that never runs (under a loop of no iteration, or in a nest
that has no point). A node that reads none (a constant: `0 - 1`, a macro `N - 1` with N = 0) is
checked as written wherever it is, in a nest with a loop whose bounds do not shift together
too, as the other checks of a constant are; its refusal names its value, not counter values
(test/CFront/header_reach_bitint_depth.c). The first version of these checks decided "reads a
counter" from the node's value at the loop's first value, so `for (int k = -5; k < 8u; ++k)`
and `k - i < 4u` under a loop of no iteration were refused, with counter values C never
evaluates -- a GEMM followed by such a dead tail stayed on the host --, and a constant node in
a nest without a point was not checked (review of 2026-10-19). Checked as written, a constant
still costs code that never runs its translation: a dead tail `C[j][16 - N - 1u]` with N = 16
keeps its function on the host, in a box nest as in a non-box one. An access that never runs
touches no element, so it gives the interface no range: an array's extent counts only the rows
C can reach, as systolic-cc needs -- it sizes the memref descriptor and its snapshot of the
caller's array by it (since the third review of 2026-10-19: a 16-row C behind a GEMM and a dead
`for (t = 0; t < 0; ++t) for (k = -5; k < 8; ++k) C[k + 15][0] = 0` got extent 23, and the
driver's copy read past the array; an array accessed only there now gets extent 0). The previous front end (a5bcda9) stripped the conversions and compared only some final
values with the counter's type, so it translated the following with other iterations or
elements than C's, or despite C's undefined behaviour (13 of the 19 refusals of
test/CFront/header_types_unsupported.c; the other 6 are non-box spellings that only the first
version of this change translated); they are refused now, with the node's exact value:
- "the loop counter's initial value converts -1 to 'unsigned int', which cannot represent it"
  -- `unsigned r = -1; r < 10` (C starts at 4294967295 and runs nothing; the translation ran
  `affine.for %r = -1 to 10`, 11 iterations); likewise `int r = 3000000000`, and
  `int c = 2147483649L` counting down;
- "the loop counter's initial value overflows 'int' (an operation's exact value is 2147483648)"
  -- `long k = i + 2147483647` (the sum is int arithmetic); "the loop counter's initial value
  wraps around in 'unsigned int' (an operation's exact value is -2)" -- `long k = u - 2` for an
  unsigned u that takes the values 0 and 1 (the first version of the non-box translation ran a
  repetition loop `for (long s = r - 1; s < 1; ++s)`, r unsigned, around a 4 × 4 × 4 GEMM 3
  times; C runs it once);
- "the loop bound converts -1 to 'unsigned int', which cannot represent it" -- `unsigned k = 0;
  k < -1` (C runs 4294967295 iterations, the translation none), and `unsigned k = i + 4; k < i`
  at i = -3, checked at every point, also where the loop runs zero times; "the loop bound wraps
  around in 'unsigned int' (...)" -- `k < u - 2`, `k < i - 2u`, and "the loop bound overflows
  'int' (...)" -- `k < i + 2147483647` (non-box bounds: a5bcda9 refused them for their shape,
  the first version of this change translated them); inside a constant, `(2147483647 + 1) -
  2147483645` (a5bcda9 took it as 3; C's int arithmetic overflows);
- "the loop condition converts -5 to 'unsigned int', which cannot represent it (it is evaluated
  for the counter values -5 .. 8)" -- `int k = -5; k < 8u` (C compares 4294967291 < 8 and runs
  nothing; the translation ran 13 iterations; `k < 8ul` alike); as before, "the loop condition
  could overflow (it is evaluated for the counter values A .. B)" for an operation of either
  side that reads the counter, and "the loop condition overflows 'int' (an operation's exact
  value is 2147483648)" for one of a constant side (`-c > -(-2147483647 - 1)`);
- "the subscript wraps around in 'unsigned int' (an operation's exact value is -1)" -- `A[u - 1]`
  at u = 0 (C addresses A[4294967295], the translation A[-1]); "the subscript overflows 'int'
  (...)"; "the subscript converts -3 to 'unsigned int', which cannot represent it".
The older checks keep their messages ("the loop counter could overflow 'T'" for a counter value
outside its type, the last increment included). Unsigned arithmetic that does not wrap around
is exact and is translated now: an unsigned char or unsigned short counter (the comparison
promotes it to int: `c < 50`), `2 * i < 10` and `k + u < 4` on unsigned counters -- the previous
front end refused them as a possible overflow.

A `wb` / `uwb` literal has a bit-precise type, `_BitInt(N)` or `unsigned _BitInt(N)` (C23; clang
accepts it in the front end's default -std=c11 as an extension). A literal fits its type, and an
enclosing counter its own, so neither is checked or looked up; arithmetic in such a type of at
most 64 bits is checked in its range like any other -- `i < 7uwb + 2uwb` is refused, "the loop
bound wraps around in 'unsigned _BitInt(3)' (an operation's exact value is 9)" (C runs one
iteration; a5bcda9 translated nine). A wider bit-precise type comes only from a literal beyond
64 bits, which is refused as such ("integer constant expression overflows", or the shape
message of its position) before its type is looked up. `i < 4wb`, `A[i][1wb]`
and `int i = 1uwb` translate exactly; the first version of the checks looked every node's type
up, literals included, and refused them with messages about their shape (review of
2026-10-19). The checks, the subscript parser and the parser of a condition's sides recurse
once per AST level with small frames (their linear forms on the heap): an initial value, a
bound, either side of a condition -- a canonicalized one too (`4 > i + 0 + ...`, `k - i + 0 +
... < 4`) --, a shifted bound or a subscript of 4,980 terms -- near the most clang's JSON lets
the front end read ("nesting too deep" beyond about 5,000) -- is translated with an 8 MB stack,
a 4 MB one too. The first version of the checks overflowed an 8 MB stack at about 3,600 terms,
the subscript parser at about 3,800 and the condition's parser at about 3,700 (it kept two
forms per level until the third review of 2026-10-19); a5bcda9's subscript parser overflowed it
at about 2,200. The interface gives an array's leading extent as the last row reached plus one,
so an access reaching row 2^63 − 1 is refused ("the access reaches row 9223372036854775807 of
'A', so the array's extent does not fit in 64 bits"; the interface said -2^63, from a signed
overflow inside the front end).

The GEMM sites' execution counts in the plan are counted by running the loops too whenever the
product of trip counts is unknown: a constant-extent shifted repetition loop, which used to
report an unknown count (-1), now reports the exact one. A site's count is computed once (the
first version recomputed it for every multiply-accumulate statement of the site: eight
statements under a 2040 × 2040 pair of repetition loops took 27.6 s, now about 4 s, the time of
one count; #19). The trip-count helper no longer reads `s + r < 3` as `s < 3` (it ignored a left
side that was not the counter alone; such loops were refused before the canonicalization, so no
earlier count was wrong). A counting-down loop whose bounds read a counter is still refused.
Parallelotope nests keep the closed form. Of the 202 C files under test/ (273 front-end
invocations, each file with its own RUN lines' flags, the SystolicCC ones as systolic-cc calls
the front end), 190 translate byte for byte as with the previous front end (a5bcda9): MLIR,
interface, GEMM sites, diagnostics and exit code -- test/e2e, the regression and failures
corpora and 157 of the 160 showcase files among them. The twelve others are the four new
front-end tests (nonbox_bounds.c, nonbox_bounds_unsupported.c, header_types_unsupported.c,
header_reach_bitint_depth.c), test/CFront/affine_condition.c (two unsigned conditions added),
test/SystolicCC/affine_exact_domains.c (new), test/SystolicCC/affine_domain_normalization.c,
whose triangle is now translated, and five whose only change is the interface's extent of an
array accessed only where C runs nothing (test/CFront/downward.c, test/SystolicCC/downward.c,
the empty GEMMs of showcase cases 129-131: extent 0). The C-type checks alone change no
translation but the new tests' functions: against the first version of this change, 238 of the
260 invocations and 650 freshly generated cases are byte-identical, and the other 22
invocations are the new functions; the fixes of the review of 2026-10-19 leave 261 of the 272
invocations byte-identical, the other 11 being the new test's and header_types_unsupported.c's
constant-side message; those of its third round leave 264 of the 273 byte-identical: 8 differ
only in the extents of arrays accessed where C runs nothing, and on the DEEP RUN line of
header_reach_bitint_depth.c the build before them crashed (its 4,000-term conditions) and this
one translates. (The comparison passes that line's generated file as a flag, so it counted the
line identical; run as lit runs it, the line differs, and in the second round's count too: the
build before that round crashed on its deep subscript -- 260 of 272 identical then.)

Gaps that remain:
- The step S is taken by its value. `i += 2u` on a negative int i is C's unsigned addition
  converted back, an implementation-defined conversion that clang defines as wrapping, giving
  i + 2.
- A conversion or unsigned operation whose wrap-around a later operation undoes is refused,
  although C's result is exact (`c0 + 2 * c2 + 1` with a negative int c0 and an unsigned long
  c2, back in range), and so is the well-defined `for (unsigned i = 7; i + 1 > 0; --i)` (the
  exit-value rule).
- An exact value beyond 2^63 − 1 (unsigned long long) is refused ("integer constant expression
  overflows"): the front end's forms are 64-bit signed.
- A counter of a typedef'd type (`size_t`, `int64_t`) or of a bit-precise type (`_BitInt(8)`) is
  refused, as before ("the loop counter must be an initialized integer variable").
- An expression nested more deeply than clang's JSON output lets the front end read (about 5,000
  terms) is refused ("cannot read clang's AST: nesting too deep"); clang's JSON dump of that many
  terms needs about 2.5 MB of stack when the hard limit is that low (with only the soft limit
  lowered, clang raises its own).
- The execution count of a site in a function the front end refuses ignores C's types; these
  counts only label offloaded functions.

**19.13.2 Family X: exact parallelotopes.** For a perfect multiply-accumulate band (the body
test of 19.12.1) whose bounds are max / min of linear forms of the outer counters (no symbols,
no floordiv), the band's iteration set D is built exactly -- one inequality per bound result,
one local quotient per stepped loop -- and compared with candidate parallelotopes
  P = { x : a_s <= x_s − l_s·x_<s <= e_s,  x_s − l_s·x_<s ≡ a_s (mod step_s) }
where each loop's shift row l_s is zero or the linear part of one of its own written bound
results, tried in that order (zero, the lower results' rows, the upper results' rows; the first
that passes is taken, so a loop whose shift source runs once keeps its written coordinates).
Loop by loop, outermost first: a_s and e_s are the exact integer minimum and maximum of x_s −
l_s·x_<s on D (integer lexicographic minimum, `findIntegerLexMin`), and the projection of D
onto x_0..x_s must equal P's (the Presburger library's integer-exact set equality); a choice
that passes fixes nothing beyond the projection, which is D's own, so the greedy search is
complete over the candidates. If D = P, every loop's bounds are replaced by P's -- the same
counters, the same points, hence the same iteration order -- and the band is normalized onto
P's box by 19.12's x = U x'. Conditions: a stepped loop starts at the max of its lower bounds,
so several lower results with different linear parts must have one of them the max at every
outer point that enters the loop (an exact emptiness test on the outer loops' own set); and the
64-bit rule below holds. Typical members: a triangle that degenerates (`for i in [2, 25) step 2, for j in [i, 4)`: only
i = 2 has points -- the set is {2} × {2, 3}), a max / min of forms that never wins on the
domain, a shifted loop written with a redundant second bound.

*The 64-bit rule, families F and X* (`exactBandSet`, review of 2026-10-18). The run time
evaluates every result of a loop's bounds -- affine.for takes their max / min, so a result a
fold drops is evaluated too -- at every outer point that enters the loop, in 64-bit arithmetic:
lower-affine emits a coefficient times a counter as `arith.muli ... overflow<nsw>` (poison when
the product does not fit) and a sum as a plain `arith.addi` (it wraps, and so gives the true
value whenever that fits). The mathematical set is the program's only when every product and
every result of every written bound has its exact range within 64 bits over the outer loops'
own iteration set -- not over D's counter ranges: a bound is evaluated also at outer points
where the loops below it run nothing. Both families check this before anything is rewritten;
before the review family F did not (`@shifted_bound_overflow`: an upper bound of 2^63 + 2 that
the run time wraps negative was normalized and lowered), and family X skipped the results a
stepped loop's fold drops (`@fold_drops_overflowing_result`). The rewritten accesses obey the
same rule: a composed product (a coefficient of x'_u times x'_u over the box) that does not fit
leaves the band as written (`@composed_product_overflow`), so the rewrite adds no overflow the
written nest does not have.

**19.13.3 Family R: repetition loops of any shape.** Loops no access reads, whose bounds read
only each other (a triangle of repetitions, `r1 in [0, 3), r0 in [r1, 2)`), with the GEMM loops'
bounds reading only GEMM loops: the iteration set is the product of the two, every GEMM point
runs once per repetition point. `hoistRepetitionLoops` now moves such loops outward too (its
legality test -- an integer-exact dependence check at each enclosing GEMM loop -- treats a loop
whose bounds do not fix its trip count as one that may run twice), the GEMM loops must have
constant bounds there; a prefix of such loops around a band the normalization can rewrite is
handled by normalizing the band below the prefix (a retry below every prefix whose counters
nothing below it reads, shortest first); the hoist then moves out any repetition loop left
inside the normalized band. It runs on the whole band, the prefix included, and moves nothing
when a repetition loop's bounds read a dimension's counter (`boundsOk`, silently: the recognizer
then refuses the nest, UNKNOWN) -- also through an operand that a constant bound map lists
without reading it (`affine_map<(d0) -> (0)>(%i)`): the permutation would move the loop past
that operand's definition (third review of 2026-10-19: "operand #0 does not dominate this use",
and with the dimension's band lowered a crash; `@dead_operand_once`, `@dead_operand_inside`). (A second
normalization after the hoist, in the first version, could change nothing -- a hoisted band's
GEMM loops have constant bounds -- and was removed by the review.) The hoist's dependence test
(`checkMemrefAccessDependence`) sees one memref at a time: accesses through different memref
values never depend for it. So the interchange is decided only when every other memref of the
body is provably disjoint from each one it stores to -- the lowering's alias rule: distinct
allocations, `llvm.noalias` (C `restrict`) arguments, or `assume-no-alias`; otherwise the nest
stays as written, with a remark ("moving it outward is a legal loop interchange only if the
memrefs the nest reads do not overlap the one it writes", `@alias_hoist_triangle`). Before the
review such a nest was hoisted, then refused for "A may overlap C", and its permuted loop order
ran on the CPU: with aliasing arguments, a different result than C's. The dependence analysis
flattens the bound and access maps in 64-bit arithmetic, which wraps for constants near 2^63:
with a lower bound max(-2^63, 0) it found no dependence, and a repetition loop of two iterations
moved out of the reduction loop, reordering the accumulation (under strict floating point the
nest was then refused, and its CPU code ran permuted). So a bound or subscript of the loops
around the accesses that flattens to a coefficient or constant of 2^48 or more leaves the nest
as written, UNKNOWN ("whether moving it outward reorders dependent iterations is not decided",
`@min_folded`; round 4 of the review of 2026-10-19). Both tests matter only
when the interchange reorders iterations: two iterations change their order only if they differ
in a dimension of two or more iterations and in a repetition loop inside it that may run twice
(the first repetition loop inside that dimension in which they differ reads only counters on
which they agree, so one of a single iteration takes one value for both). Moving repetition
loops outward past loops that run once -- a leading loop of one iteration -- keeps the program's
order and permutes the band without either test (`@leading_once_alias`, `@leading_once_read`;
review of 2026-10-19: the alias rule refused such a nest with its remark, and the inner GEMM
candidate was then recognized and refused again for "A may overlap C"). A loop of one iteration
is a dimension when its bounds are constant (a constant on the domain, Lemma 0, as the
recognizer's `singleIterationValue` reads it: a max / min of constants folded, `max(0, -1) to
min(1, 4)` -- the hoist took that for a repetition loop that may run twice until the third
review of 2026-10-19, and refused a nest the recognizer then lowered, `@maxmin_once_inner`,
`@maxmin_once_alias`; a difference to the lower bound beyond 64 bits, `-1 to min(0, 2^63 -
1)`, is never the min, `@ovf_once_inner`), unless a loop's bounds read its counter: then it is
a repetition loop of one iteration, which cannot run twice and moves out with its readers
(round 4: as a dimension it made `boundsOk` refuse its readers, so `t, r1 in [0, 3), r0 in
[r1 + t, 2), i, q, j, k` stayed on the host, and folding a max / min spelling of t lost
lowerings the previous build made; `@once_read_by_repetition`, run 6 times, `@once_read_alias`,
`@maxmin_once_read`). One whose bounds read an outer repetition counter,
`r0 in [r1, r1 + 1)`, is a repetition loop of one iteration -- it was taken for a dimension with
bounds that are not constant, and the nest stayed as written, unrecognized
(`@shifted_once_repetition`, lowered now and run 6 times; inside the reduction loop it moves out
too, `@once_inside_reduction`, `@once_inside_reduction_restrict`). A repetition loop whose trip
count its bounds do not fix (`0 to %n`, a triangle) may run twice (`@symbol_range_inside`).
Under `classify=true` the hoist's two
refusals carry the class UNKNOWN, as every refusal remark does (§19.9; they carried none). The number of tile
invocations per call is the number of repetition points, counted exactly (`enclosingRepetitions`:
a loop whose bounds differ by a constant has that many trips, clamped at 0 -- a negative
difference was cast to uint64 before, `@rep_never_runs`; other bounds are evaluated by running
the enclosing loops), so the driver checks the invocation count as usual.

**19.13.4 Proofs, both ways.**
- *Bijection certificate, every lowered GEMM* (`verifyLogicalBijection`, AffineMatmulPartitioning):
  independently of the mixed-radix argument that built the logical mapping, (1) the source
  domain equals x = S z + origin over the box of its loops' ordinals (Presburger equality, the
  strides as floordiv locals), (2) the logical ordinals of the batch loops and of I, J, K are an
  integer map of z (each term divisible by the coordinate's scale), (3) their ranges lie in the
  box batch × [0, M) × [0, N) × [0, K), (4) the map is injective on the z box (no nonzero integer
  kernel vector with |δ_s| < n_s: one integer-emptiness test per leading position), (5) the two
  boxes have as many points -- hence onto. No logical iteration is missing (surjective), out of
  range, or duplicated (injective). The discovery pass prints it ("logical bijection: verified");
  every recognized GEMM of the lit inputs verifies, and so does every lowered nest of the oracle
  sweep. The certificate is shown not to be vacuous: the discovery pass's test option
  `perturb-certificate` re-checks each verified mapping with five wrong ones -- M + 1, M − 1, I's
  scale doubled, loop 0's origin + 1, I's row of T zero -- each refused by a different one of the
  tests (5), (3), (2), (1), (4) (`affine_gemm_exact_domains_partition.mlir`, which also partitions
  a 20 × 20 × 24 GEMM in disguise into 26 regions and A·Aᵀ in disguise into 10, and takes A·Aᵀ
  through the tile lowering: per-tile slab copies of A and Aᵀ only, no copy of the matrix).
- *Disproof, non-box domains left as written* (`disproveAffineImageOfBox`): a GEMM's updates, in
  any decomposition, are an affine image of a box (the C, A and B elements each update touches,
  as affine functions of the box coordinates), and every affine image of a box's integer points
  is centrally symmetric. When the accesses tell the iterations apart -- the stacked map
  x ↦ (F_A(x), F_B(x), F_C(x)) has rank n, so the nest's updates are an injective affine copy of
  D, and an injective affine map keeps central symmetry both ways (n the candidate band's loops;
  C's map is the accumulator load's, which the recognizer accepts when it equals the store's on
  D: a band whose store alone has full rank, which proves the same, is left UNKNOWN,
  conservatively; `@load_rank_full`, `@store_rank_full`) -- and the two factors and C
  are three buffers, a D that is not centrally symmetric is no GEMM's. A symmetric set is
  symmetric about the midpoint of its lexicographic extremes p, q (the reflection reverses the
  order), so D is compared with p + q − D exactly; when they differ, a point whose mirror is
  missing is reported and the class is PROVEN_INVALID ("the iteration domain is not centrally
  symmetric -- the point (6, 7, 0) is in it, its mirror (1, 0, 7) ... is not --, so it is no
  affine image of a box; the accesses tell the iterations apart (their stacked map is
  injective), so ..."). The set reasoned on is the one the program runs: built from checked
  linear forms of the bounds (`exactIterationSet`; MLIR's flattening wraps -- `(3i + 1)·2^62`
  became the coefficient −2^62, a set of phantom points, `@flattening_wraps`), under the 64-bit
  rule of §19.13.2 at the outer points (`@outer_point_overflow`, `@overflow_bound_asymmetric`).
  Not attempted (UNKNOWN): a rank below n -- a loop no access reads, and the strip-mined
  remainder: k = 2·k1 + k0 on {(0, 0), (0, 1), (1, 0)} takes 0, 1, 2 once each, the 4 × 4 × 3
  GEMM on an asymmetric set (`@remainder_tile_k`, executed as `ex_remainder`); a buffer shared by
  two operands (A × A: the product commutes, two views of one buffer may exchange roles from
  point to point, `@axa_triangle`); a bound with floordiv / ceildiv / mod (`@mod_bound`) or a
  symbol; a stepped loop whose lower bound reads a counter (a max of constants is fine,
  `@stepped_max_of_constants`); a bound outside 64 bits; a symmetric set (a hexagon: symmetry
  does not exclude every parametrization). Before the review of 2026-10-18 the premise was
  "every loop is read by an access" (syntactic, `isFunctionOfDim`), the set was the flattened
  source domain, and the 64-bit guard bounded intervals over D's counter ranges with a wrong
  interval for `mod`: the remainder tiles, the phantom set, the `mod` bound and the outer-point
  overflow were all PROVEN_INVALID -- wrongly. Re-decided by this: the triangle of `affine_gemm_mapping.mlir`,
  `affine_gemm_domain_normalization.mlir` @triangle and @max_bound (a clipped parallelogram, rows
  of 2, 3, 4, 4, ... points), the ISSUE-002 reproducer's triangular nest and showcase mlir_009 --
  UNKNOWN before, PROVEN_INVALID since; the specification's REJECT for them is unchanged.
- *Candidates the recognizer does not decide.* A candidate whose access reads the counter of a
  loop outside it -- the inner band (i, j, k) of a batched GEMM C[b][i][j] -- has that counter as
  a parameter: it may be a GEMM at each of its values, which the recognizer does not analyse, so
  it is UNKNOWN ("an access uses the counter of a loop outside the nest"); the discovery pass
  called it PROVEN_INVALID before the third review of 2026-10-19 (`affine_gemm_batched_lowering.mlir`
  @batched; the lowering pass never reported it: the enclosing candidate is the one lowered or
  refused).

**19.13.5 The classes, as tested** (`test/Systolic/affine_gemm_exact_domains.mlir`, 48 nests).
Lowered (PROVEN_VALID): a degenerate triangle (1 × 2 × 4), an inactive max / min (4 × 4 × 4), a
sheared box in disguise, a stepped loop with a dominant max, repetition triangles between GEMM
loops and as a prefix around a shifted band (run 3 times), det T = 2 with a stride-2 C view (the
determinant decides nothing: the certificate does), A·Aᵀ through two views and A·A through one
view on domains in disguise, K = 1 in disguise. Refused: PROVEN_INVALID for a genuine triangle
(missing logical iterations), K = 1 out of bounds, out of bounds after the normalization;
VALID_BUT_LOWERING_UNSUPPORTED for duplicated logical iterations on a box (K read as k + l: a
batched GEMM only by demotion, which stays deferred); UNKNOWN for a repetition loop whose bound
reads a GEMM loop (each row accumulated a different number of times), a symmetric hexagon, a
bound that overflows 64 bits, and C aliasing a factor ("A may overlap C", conservative). Added by
the review of 2026-10-18: the 64-bit rule (a dropped fold result, family F, a composed product,
the flattening, `mod`, the outer points, the asymmetric overflow variant: UNKNOWN), the rank
condition (the remainder tile: UNKNOWN), A × A on a triangle (UNKNOWN), a stepped max of
constants (PROVEN_INVALID), a stepped max with no dominant result (UNKNOWN), the hoist under
possible aliasing (not hoisted), a repetition loop that never runs (counted 0). Added by the
review of 2026-10-19: a leading loop of one iteration around a repetition loop, with aliasing
arguments (one refusal, the GEMM's "A may overlap C", UNKNOWN) and with restrict ones and an
access reading it (lowered, run twice), and a repetition loop of one iteration with shifted
bounds (lowered, run 6 times). Added by its third round: a loop of one iteration whose bounds
are a max / min of constants, innermost (lowered, no hoist remark) and between i and j with
aliasing arguments (one refusal, "A may overlap C"); a repetition loop with a symbol range
inside i, aliasing arguments (not hoisted, the hoist's remark: an unknown trip count may be
two); a repetition loop of one iteration inside the reduction loop (moved out without a test:
one refusal with aliasing arguments, lowered and run 6 times with restrict ones); a repetition
loop whose constant bounds list a dimension's counter as an unread operand, leading one-
iteration loop (not hoisted, valid IR, one refusal) and inside i (not hoisted, unrecognized, no
remark); a triangle whose accumulator load and store differ off the domain, the load's stacked
map of full rank (PROVEN_INVALID) and the store's (UNKNOWN). Added by its fourth round: a
repetition loop reading a leading loop of one iteration, with a repetition loop to move out of
i (lowered, run 6 times; with aliasing arguments one refusal, the hoist's); a max / min loop of
one iteration read by a repetition triangle inside i (lowered, run 6 times); a loop of one
iteration whose upper bound's second result is 2^63 - 1 (lowered, no hoist remark); a repetition
loop inside the reduction loop with a lower bound max(-2^63, 0) (not hoisted, the order kept,
UNKNOWN).

**19.13.6 Specification, predictor, generator, oracle** (revised by the review of 2026-10-18,
#21-#30 and #40). *Specification* (`casespec._spec_domain`). A non-box nest is decided on a box
when one exists: on its box form when its bounds shift together (family F, `box_form`); on the
box of its points when they are a parallelotope with the bounds' shift rows (family X,
`exact_form`, the enumeration twin of the compiler's search: the same candidates, decided on the
enumerated points; a loop counting down keeps only the values it takes on the set, from e down
to a, and `gen_cases.finalize` asserts that the box computes the same C, #24); on its surrogate
in family R (the repetition loops as plain two-iteration loops, the repetition count the exact
number of their points). Family R: independent repetition loops (no access reads them, their
bounds read only each other) a prefix of which leads the nest around a band the normalization
rewrites as written, inner repetition loops included (the compiler's sub-band retry normalizes
the band, whose inner repetition loops then have constant bounds, and the hoist moves them out,
#21), or that sit anywhere around GEMM loops with constant bounds. A nest with no iteration point is
UNDETERMINED (SQ-2), as a box with a loop of zero iterations, also when only its repetition set
is empty, which the surrogate hides (#28). Any other nest is enumerated as written. When the
readers' roles map its points bijectively onto a box, every point repeated alike (per batch
value, one shape), it is a GEMM on a domain the compiler does not normalize: OUT_OF_SCOPE
`non_box_unnormalized` (strip-mined loops with a remainder, k = 2 k1 + k0 on {(0, 0), (0, 1),
(1, 0)}). Otherwise no other decomposition is searched, so a refusal needs a proof
(`_no_gemm_proof`): the nest's updates, the multiset {(F_A(x), F_B(x), F_C(x)) : x ∈ D} over
every iteration point with multiplicity, are not centrally symmetric. A GEMM's updates are those
of its slots (19.1: ξ is an affine bijection of a box Z onto D, so they are the image of Z under
the affine map F ∘ ξ), once per slot and as often again per point of independent repetition
loops; they are therefore symmetric about the image of Z's centre, and so is every affine
projection of them. With one array as both factors the product commutes, and the
exchange-invariant projection (F_C(x), F_A(x) + F_B(x)) is used. With the proof the verdict is
REJECT `non_box_domain`; without it UNDETERMINED `non_box_undecided` (the updates are symmetric
while the canonical decomposition fails, and a decomposition the specification does not search
may exist) -- a limit of the executable specification, not a spec question, like
`non_box_unenumerated` above 200,000 points. The compiler's PROVEN_INVALID implies the proof
(rank n makes the updates an injective affine copy of D, and three arrays exclude the exchange),
so no nest the specification calls a GEMM can be predicted PROVEN_INVALID; the proof is applied
to a GEMM found too, and a batched one needs its batch values to be the image of a box under the
parametrization (Z is a box of all the coordinates, 19.1; review of 2026-10-19: a triangle of
batch values, one 2 × 2 × 2 GEMM per value, was called a GEMM, and the compiler's correct
PROVEN_INVALID a wrong proof). The test, `_is_box_image`, accepts a box sheared along the loop
order -- at every value of the earlier batch counters the next one runs over an arithmetic
progression of one step and length whose start is affine in them (b2 in [b1, b1 + 2)) -- as well
as an axis-aligned one, which is all it accepted until the third review of 2026-10-19 (a
parallelogram of batch values was UNDETERMINED); an image sheared otherwise (a diamond) stays
undecided. The surrogate keeps a loop of one iteration with constant bounds as written: it
repeats nothing (given two iterations inside the reduction it made a repeated GEMM
`repeat_in_reduction`); a loop no access reads whose shifted bounds fix its trip count at 0 or 1
gets one iteration: it cannot run twice, so the hoist moves it without a test (third review of
2026-10-19: given two, r0 in [r1, r1 + 1) inside the reduction loop made the specification
`repeat_in_reduction` and a correct offload false_positive). Family R's retry is the compiler's:
below the shortest prefix of the leading run whose counters nothing below it reads and whose
band normalizes (the whole run only until the third review of 2026-10-19: r1, r0 in [r1, 2), q
and below them s in [q, q + 2) -- normalized below r1, r0 and lowered 12 times -- was
OUT_OF_SCOPE and the run false_positive; a repetition loop below the run reading the run's
counter had made the band ill-defined before, a KeyError). A loop of one iteration that a
repetition loop's bounds read is a repetition loop for the hoist too (§19.13.3), so it stops
nothing; the third round's model of the hoist taking it for a dimension made correct offloads
false_positive (`t, r1, r0 in [r1 + t, 2), i, w in [0, 1), j, k`: nothing to move) and was
removed in the fourth. Independence is decided on the nest as written and, failing that, on the
nest as the normalization leaves it (`_normalized_view`: the band below the shortest such prefix
on its box when it is a parallelotope): an inner repetition loop `s in [i, i + 2)` reads the GEMM
counter i as written and nothing on the box, where the compiler hoists it (round 4: the
specification called such nests OUT_OF_SCOPE, false_positive).

*Predictor* (`casespec._predict_domain`). The compiler's dimensions are the loops an access reads
and the one-iteration loops with constant bounds unless they lead the nest (`_compiler_dims`;
Lemma 0, recognizeLogicalMatmul; #22), and for the hoist not those whose counter a loop's bounds
read (`_hoist_dims`): the repetition structure is decided with the hoist's, on the nest as
written or as the normalization leaves it (`_predictor_view`). A prefix of repetition loops needs
no hoist: `hoisting_required` is set only for interleaved ones (round 4: predict() read a
surrogate's one-iteration loop in the prefix as a dimension and set it). A band left as written is
recognized on a surrogate that replaces
only the loops whose bounds read counters (#23), and a front-end refusal predicted on it is the
nest's (#25). The dense partition is predicted PROVEN_INVALID, with the remark's wording,
exactly when `disproof_holds`: (2) no stepped loop's lower bound reads a counter (a loop
counting down is an ordinal loop of step 1); (3) every product (coefficient × counter) and every
result of every loop's bounds as emitted (the lower bound, the exclusive upper bound) is within
64 bits at every point of the outer loops entering that loop (`written_bounds_fit_int64`; the
front end checks the bounds as written in their C types, §19.13.1, but emits canonicalized
ones, so this is checked, not assumed); (4) the stacked access map has rank n -- every
subscript of A, B and C as a row over all n loops, one-iteration loops included, computed
exactly in fractions (`stacked_access_rank`; a loop no access reads is a zero column); (5) the
two factors and C are three different arrays; (6) the iteration set is nonempty and not
centrally symmetric. Condition (1), bounds that are linear forms of the nest's outer counters,
holds for every translated C nest. A leading run of unread loops whose counters a loop below
reads leaves the band below as the candidate, UNKNOWN. Otherwise the reason is UNKNOWN's: "the
loop bounds are not static ...". The predictor mirrors the hoist's alias rule (§19.13.3; review
#3): a repetition loop inside a dimension is moved only when each factor other than C is
provably disjoint from C (one of the two a restrict argument), tested before the interchange's
dependence test; otherwise the nest stays on the host with the hoist's remark ("loop N of the
nest repeats the accumulation inside it, and moving it outward is a legal loop interchange only
if the memrefs the nest reads do not overlap the one it writes; ..."), when the interchange
reorders iterations -- a repetition loop that may run twice (two or more iterations, or a trip
count its bounds do not fix) inside a dimension of two or more (2026-10-19; at any position
before). An unsigned counter with a written condition is not a case (`Loop` raises
ValueError): the front end translates it only when no operation of the condition wraps around
(§19.13.1), which the predictor does not model (#29).

*Verdicts* (`scripts/verdict.py`). A refusal that states the disproof ("is no affine image of a
box") on a case the specification calls a GEMM (`mathematical_gemm` / `legal_gemm`) is a
soundness mismatch -- `unexpected_stage`, whatever the prediction. (Under the former "every loop
read" rule the predictor mirrored the compiler's PROVEN_INVALID on the remainder tile, and such
a run passed.)

*Generator.* The `domains` family has `disguise` (a degenerate triangle), `rep_triangle` and
`shift_cond` (a shifted loop written `k - i < c`); `DOMAIN_NOTES['triangle']` states the
specification's rule and the compiler's conditions.

*MLIR oracle.* It emits boxes in disguise (an inactive extra bound result with another linear
part, or an unsheared outer loop extended by values that empty an inner loop: asserted point for
point) and decides every emitted triangle by enumeration. A parallelotope with the bounds' shift
rows is the GEMM of its box (`find_parallelotope`, independent of the compiler): it must be
lowered and is checked address by address ("a degenerate triangle"). Any other triangle must
not be lowered. The pass runs with classify=true (the IR is unchanged); each refusal remark is
mapped to its nest by its line (with `-split-input-file` the lines are the whole input's), and
every refused triangle's class is checked against its points (#30): PROVEN_INVALID only on a set
that is not centrally symmetric, with a stacked access map of full column rank (C's subscripts
the store's or the accumulator load's, which agree on the points) and three different memrefs,
and with a correct witness -- the remark's point in the set, its stated mirror the reflection
about the midpoint of the lexicographic extremes and outside the set; conversely, a recognized
triangle meeting those conditions with the load's map (the compiler's), with no stepped loop
whose lower bound reads a counter, must be PROVEN_INVALID. The summary counts the classes
("refused triangles by class: ..."), pinned by the lit test, which requires PROVEN_INVALID
triangles on seeds 2 and 3. Review of 2026-10-19: ranking the store's subscripts gave false
mismatches on 3 of 415 seeds (153, 255, 333), both ways; and since the generator makes no
nonempty symmetric refused triangle (a triangle's rows grow with the outer counters), the
witness check is what exercises the symmetry test on generated nests -- a symmetric set is
covered by @symmetric_hexagon only. The checks are made on the compiler's candidate band, the
loops from the outermost one an access reads inward, whose points, ranks and witness they are:
a leading loop no access reads only runs it again (third review of 2026-10-19: seed 877's
case_347, a correct PROVEN_INVALID under such a loop, was a false mismatch -- rank 3 below the
nest's 4, a witness of 3 coordinates -- outside the lit seeds and the 1-415 sweep; a band whose
bounds read such a loop's counter has parameters, and the disproof never claims it). No lit seed
has a recognized triangle whose load and store maps differ in rank, so the load rule is pinned
by @load_rank_full and @store_rank_full (§19.13.5). The band's points are its own, enumerated
alone (`band_points`): a leading run with no point runs the band zero times, and the projection
of the nest's points was then empty -- a correct PROVEN_INVALID a mismatch, a wrong UNKNOWN
unnoticed (round 4). A lowered triangle is a mismatch unless it is a GEMM over a parallelotope
repeated by independent repetition loops (family R, `repetition_family_nest`: the repetition
loops read only each other and nothing reads them, and the GEMM loops' points form a box sheared
along the loop order), which is SKIPped as `decide()` SKIPs a repetition loop on a box (round 4:
a box GEMM under a repetition triangle, which the compiler lowers, was a mismatch).

*Validation of the specification changes* (2026-10-18, the build with the rank condition).
3,467 generated `domains` cases (random seeds 2027, 1, 3, 7, indices 0-599; the enumerate mode,
indices 0-1,499) and the 175 showcase cases: the review's changes moved 33 predictions from
PROVEN_INVALID to UNKNOWN (triangles over split coordinates read only together, stacked rank 3
of 4 to 8 loops), reworded 38 PROVEN_INVALID predictions and changed no other prediction,
specification, analysis or classification; the 158 triangle cases have new header and basis
texts, and mlir_009 a new note. All 71 changed predictions agree with the compiler, stage and
reason; 12 of them were also run end to end through systolic-cc and the program. Every REJECT
`non_box_domain` (158) carries an independently computed proof, and no predicted PROVEN_INVALID
(38) is a legal GEMM. Oracle seeds 1-16 × 400: 0 mismatches; refused triangles 22
PROVEN_INVALID, 72 UNKNOWN, 199 not recognized. Fabricated classes are all flagged
(PROVEN_INVALID on 61 rank-deficient and 18 shared-memref triangles, UNKNOWN on 26 provable
ones, seeds 1-8). Seed 5's case_68 is a rank-deficient triangle that is a GEMM: J = 9 x0 + 3 x3
on {(0, 0), (0, 1), (1, -1), (1, 0), (1, 1)} takes 0, 3, 6, 9, 12 once each. It is UNKNOWN; a
disproof without the rank condition would claim PROVEN_INVALID, which is false.

**19.13.7 Limits that remain.** Non-box domains outside the families F, X and R are not lowered:
the compiler proves them no GEMM (PROVEN_INVALID) under the conditions of §19.13.4 or leaves them
UNKNOWN -- among them genuine GEMMs such as the strip-mined remainder tile, which only a lattice
parametrization of the domain would recognize; demotion stays deferred (§19.12.6). A repetition
loop whose bounds read the counter of a one-iteration loop is hoisted with it since the fourth
round of the review of 2026-10-19 (it was a limitation before). A repetition loop whose constant
bound map lists an unread operand naming a dimension is not hoisted (conservative: dropping the
operand first would let it move). A hoist that needs the dependence test is not made when a bound
or subscript around the accesses flattens to a constant or coefficient of 2^48 or more (UNKNOWN;
the analysis' 64-bit arithmetic). A band whose C store alone has full stacked rank is UNKNOWN, not
PROVEN_INVALID (§19.13.4). The specification's batch test is sufficient only: batch values that
are an affine image of a box sheared otherwise than along the loop order are undecided
(UNDETERMINED), never called a GEMM or refused. Transposed operand views are correct but costly downstream:
`bufferization.to_buffer` of a strided [1, N] view into an identity layout makes
`--systolic-runtime-to-llvm` materialize the transposed matrix once per tile (ten 16 × 16 copies
for a 10-tile A·Aᵀ, and the same for a plain Bᵀ) -- a cost of the existing tile lowering, not of
this change. The front end's gaps are listed in §19.13.1, the executable specification's limits
(`non_box_undecided`, `non_box_unenumerated`) in §19.13.6. The FPGA board was not run for any of
this section's results.

---

## 20. Assignment nests: C = A · B as the K = 1 GEMM into a zeroed C (2026-10-19)

The task of 2026-10-19 (the rank screening): accept the mathematically legal, safely lowerable
two-dimensional, degenerate and rank-1 nests the recognizer refused, first of all

```
C[i][j] = A[i] * B[j]      C[i][j] = A[j] * B[i]      C[i][j] += A[i] * B[j]      C[i][j] += A[j] * B[i]
```

The audit (`~/work/integration-backups-20261016/outer-product-validation/`) found every
**accumulation** form already recognized and lowered (rank-1 outer products, transposed and
permuted ones, mixed ranks, K = 1 by constant subscripts or a one-iteration loop, M = 1 / N = 1
with the reduction kept, offsets, strides, transposed, flattened and skewed views, batches: §2,
§19), and every **assignment** form `C = A * B` refused -- not a candidate (the shallow-nest filter
wanted an `arith.addf` store) or "not exactly one multiply-accumulate". The recognizer modelled only
`C = C + A * B`. This section defines the assignment and how it is lowered; nothing else of the
screening changed (broadcasts stay unsupported, §10; element-wise products stay refused, §19.1).

### 20.1 Definition

An **assignment nest** is a perfect `affine.for` chain whose innermost body holds two
`affine.load`, one `arith.mulf` of the two loads, one `affine.store` of the product and nothing
else. It is a GEMM of its operands iff

- **(A1)** the subscripts satisfy the conditions of §19 as for an accumulation (roles by readers or
  groups, every subscript an affine view of its roles plus batch terms, C injective per batch,
  the exact bijection of §19.13), and
- **(A2)** C's full index map is injective on the whole iteration domain D: every element of C is
  written exactly once.

**Theorem 5 (assignment = K = 1 GEMM, zero start) [proved].** Under (A1) and (A2) the reduction
coordinate K has extent 1 (a loop of two or more iterations that only the factors read would write
the same C element at each of its values, against (A2)); each element C(P*, I, J) receives exactly
one product, so the nest computes `C = 0 + A'·B'` with K = 1 -- the batched or plain GEMM of §19.1
into a C whose prior value is replaced by zero. In exact arithmetic the values are identical;
under IEEE the lowered form computes 0 + (0 + a b) where the loop computes a b: the +0 start of the
tile turns a −0 product into +0, exactly the deviation §6 already lists for a one-term tile, so
the strict-FP policy applies unchanged. *Proof:* by (A2) the contributions to one element form a
set of size one; the view and bijection arguments of Theorem 1 give the product's operands. ∎

(A2) fails in two ways, both refused as PROVEN_INVALID ("not a GEMM of A and B"):

- a loop of two or more iterations that no subscript of C reads -- a reduction loop
  (`C[i][j] = A[i][k] * B[k][j]`, k of 4: the last product wins), or a loop read only by the
  factors (`C[i][0] = A[i] * B[j]`, `C[i] = A[i] * B[j]`); the recognizer refuses it on the domain
  forms of the subscripts (a loop that runs once is a constant, Lemma 0; a trip count that is not
  a constant leaves it UNKNOWN);
- batches that overlap in C, `C[i + b][j] = A[b][i] * B[b][j]` or `C[i][j + k] = A[i][k] * B[k][j]`
  (per batch C is injective and the accumulation form is a batched GEMM of views, §19.3); the
  lowering refuses it with `verifyOutputWrittenOnce`: C's indices as an integer map of the box of
  ordinals z (x = S z + origin, the box `verifyLogicalBijection` proved the domain equal to), no
  nonzero difference δ with |δ_s| ≤ n_s − 1 in its kernel -- the argument of §19.13 step 4,
  Presburger integer emptiness, one polytope per first nonzero position.

A flattened C with collisions (`C[4i + j] = A[i] * B[j]`, j < 5) is refused by G2 at recognition,
PROVEN_INVALID for an assignment (an overwrite) where the accumulation form is "valid, not
constructed" (a tile would lose an update). A repetition loop (read by no access) around an
assignment rewrites the same values: the hoist of §19.13.3 moves it outward as for an accumulation
(the interchange keeps the per-element order of the writes), and the GEMM then runs once per
iteration of it -- idempotent, the loop's result. A factor that may alias C is decided by the
lowering's rule of §7 as for an accumulation (lowered when the windows are provably disjoint,
UNKNOWN otherwise). The front end's site finder (`systolic-cfront --gemm-sites`) is unchanged:
assignment nests, like the K = 1 accumulations of two loops, are not sites and get the driver's
synthetic-site path (one report per call); the runtime ABI is untouched.

### 20.2 Implementation

- `collectAffineMatmulCandidates`: a shallow nest (one or two loops) is a candidate when its
  innermost body stores an `arith.addf` or an `arith.mulf`.
- `recognizeLogicalMatmul`: two loads and a stored `arith.mulf` of them are the assignment form
  (`LogicalMatmul::accumulates = false`); C's access is the store's; the overwrite check above;
  `reductionMayReassociate` is read from the product's `fastmath<reassoc>` (the nest's only
  floating-point operation; `systolic-cfront --fp-reassociation` marks mulf as it marks addf).
  The discovery pass prints `statement: C = A * B (assignment: ...)` or `C = C + A * B`.
- `normalizeParallelotopeDomains` accepts the two-load body (non-box assignment domains are
  normalized like accumulations); `hoistRepetitionLoops` needed no change.
- `AffineMatmulToSystolic::lowerOne`: after the partition, the bijection and the FP permission,
  `verifyOutputWrittenOnce` and K = 1 are required; then, inside the batch loops, C's logical
  window -- the materialized view, exactly the elements the nest writes -- is zeroed with
  `affine.for` / `affine.store 0.0` before `bufferization.to_tensor` takes it, and the unchanged
  tile plan accumulates into it. The remark reads `... lowered as R exact region(s) (an assignment
  C = A B: C zeroed before the tiles)`; the strict-FP refusal names the product.

### 20.3 Verification (2026-10-19; evidence in `~/work/integration-backups-20261016/outer-product-validation/`)

`test/Systolic/affine_gemm_assignment.mlir` (the IR of the zeroing, the remarks, the discovery
lines and classes of 12 nests, strict FP) and `test/SystolicCC/affine_assignment.c` (22 offloaded
kernels -- the four forms above, permuted outputs, mixed ranks, K = 1 constant and one-iteration
reductions, 1 × 7 / 7 × 1 / 9 × 13 over six tiles, offsets, strides, a strided, a flattened, a
skewed and a reversed C, transposed rank-2 factors, a batched and a repeated nest -- each bitwise
equal to the loops as written on integer data with a nonzero initial C, on the software runtime
and in mock-FPGA mode, `--verify` 22 of 22; seven nests that must stay on the host; the two forms
checked against each other on the same inputs) and `affine_assignment_signzero.c` (the −0
products of a zero factor: numerically equal, bitwise different, accepted by `--verify-rtol`).
The probe matrices `phase1/{before,after,after2}` record every form's classification before and
after. The FPGA board was not run.
