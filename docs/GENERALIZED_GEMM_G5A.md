# Generalized GEMM recognition: Phase G5a, exact domain and reduction semantics

G5a establishes which points a group's events are, in what order, how many times each,
and what each output cell's final value is as a fold over its contributions. It builds
on G3b's coordinates and G4a's replay relation, re-checks both, and is research-only:
nothing here is linked into production lowering, and no result authorizes one.

Decisions D1 to D6 of the G5 design were approved on 2026-11-03:

1. The guarded orbit (Theorem 12) is the symbolic coverage proof. No Presburger set and no
   Simplex is in the trusted path.
2. Multiplicity is Unit or ConstantBox symbolically, and counted exactly at enumerated
   points. A μ that varies over the domain stays Unknown symbolically.
3. Reorder results are facts, never authorizations.
4. G5a's proposer fits guards to sample tables. An affine proposer in a child process is
   G5b's.
5. G3b gains one additive export, its walker. G3b's output does not change.
6. G5a is this phase; G5b, G6 and G7 are not started.

## Objects

For group G at parameters v and a run that completes, with events E in execution order:

| Object | Definition |
| --- | --- |
| Event domain | E, ordered, with repeats |
| Logical point | φ(e) = (P, I, J, K)(e), the family's terms |
| Repeat levels | Levels that are not family coordinates. They tell repeated events apart |
| Extended point | λ(e), the values of all levels at the event's block entry |
| Guarded set | S(v), defined below |
| Multiplicity | μ(x) = number of events with φ = x |
| Physical cell | h(e), the output index tuple. Not assumed injective in φ |
| Per-cell order | The order of E restricted to one cell, described by a key κ, a subset of levels |

**Guarded order.** Levels l₁ … lₘ, outermost first. Each has a value term over
memory-independent state, a direction and a signedness, and bounds lo ≤ y < hi that are
BV terms over parameters and outer level variables. A guard g is a BV1 term over
parameters and all level variables. It can be affine or not, with disjunctions.

    S(v) = { y : lo_l(v, y_<l) ≤ y_l < hi_l(v, y_<l) for every l, and g(v, y) }

ordered lexicographically, each level in its own direction and signedness. Every value is
a bit vector at its width. **S(v) is not a set of mathematical integers**: a bound that
wraps is what it is at the width, and G6 must not read it otherwise.

Rectangular, batched, triangular, banded, holed, piecewise, stepped and non-affine
domains are instances. No domain is assumed to be a Cartesian product.

## Theorems

**Theorem 12 (guarded orbit).** Let y be free level variables, λ_x the levels' values at a
path's end. Suppose:

- **D1.** On every path from the entry to the first event, λ ∈ S and ¬∃y ∈ S with y < λ.
  A path to the exit with no event gives ¬∃y ∈ S.
- **D2.** On every path from an event b to the next event x: x ∈ S, b < x, and
  ¬∃y ∈ S with b < y < x.
- **D3.** On every path from an event b to the exit with no event between: ¬∃y ∈ S with
  y > b.

Then for every parameter point in scope and every run that completes, λ maps E in order
onto S(v) in order: nothing missing, nothing repeated, nothing made up.

*Proof.* Induct over events.
- D1 makes λ(e₁) the minimum of S, or S empty when there is no event.
- Each event's λ is in S, by D1 and then D2. That is the assumption D2 and D3 start
  from at an arrival, and the over-approximation D̂ contains every real event state.
- D2 makes λ(e_{t+1}) the immediate successor of λ(e_t) in S, so no member between them
  is skipped.
- D3 makes the last event the maximum of S.
- The lexicographic order is strict, so λ is injective.
- B(v) is finite, and a completing run is finite.

Hence λ(E) = S(v), in order. ∎

Each query is QF_BV with y free in an unsat query, so y is universal in the obligation and
existential in the query: no quantifier alternates. The walk between events is G3b's,
which ignores cuts, prunes where the solver shows a prefix infeasible, and fails at its
depth. Theorem 8 of G3b is the case g = true with unit steps.

**The walk sees one event per block arrival.** λ is read at the block's entry, so two
sites of the group in one block are two events with one λ. The symbolic route refuses such
groups, and the finite route refutes any order for them. The independent review found this
(defect R1 below).

**Theorem 13 (coordinate-level reduction semantics).** Take G4a's Claim B, Theorem 12
and K1 at a common tier. Then for every cell c and every initial memory M₀ whose run
completes:

    C_final[c] = foldl ⊕ C_init[c] [ A₀[f(λ)] ⊗ B₀[g(λ)] : λ ∈ S(v), h(λ) = c, in κ order ]

Each step keeps its site's exact operation, flags and operand order. A logical point with
μ = 3 appears three times, at its own positions. h, f and g are functions on S because λ
is injective. With G3's coordinates as well, the same holds over (P, I, J, K), which is
the reported `logical_tier`.

*Proof.* G4a gives the per-cell fold over the events in execution order. Theorem 12
replaces the events of the cell, in order, by the points of S with h = c, in λ order. K1
makes that the κ order. ∎

When K1 is not proven, the statement holds with κ = every level.

## Obligations

| ID | Obligation | Method | Status on failure |
| --- | --- | --- | --- |
| X0 | Program supported and oblivious, group whole, certificate re-checked, solver limits non-zero | Gates | Unknown |
| X1 | Level values adopt as proof terms of memory-independent state with no shadow. Bounds read parameters and outer levels, and the guard parameters and levels. Every family coordinate is a level | Structural | Unknown, "refused" |
| X2 | No two sites of the group share a block (symbolic) | Structural | Unknown |
| D1–D3 | Theorem 12 | QF_BV over walks | Unknown |
| M | Unit: no repeat level. ConstantBox: repeat bounds read parameters only, nothing else reads a repeat level, and their widths sum below 128. Then μ = Π max(hi − lo, 0) at 128 bits | Structural | Counted (finite), Unknown (symbolic) |
| K1 | Two events of one cell are in λ order exactly when they are in κ order | Two-copy QF_BV | Unknown |
| C1 | No two logical outputs (P, I, J) share a cell | Two-copy QF_BV | Unknown, or Refuted at a point |
| T1 | Every cell has at most one contribution | Two-copy QF_BV | Unknown, or Refuted at a point |
| A1 | Proposed closed forms equal the index terms at every event | QF_BV | Unknown, or Refuted at a point |
| F | At each enumerated point: λ strictly increasing, λ list = S(v) exactly, μ counted, key, collisions, closed forms | Exact enumeration | Refuted with the point |

The finite route claims ScopeEnumerated only when the enumeration ran exactly the given
scope (`enumerationCoversScope`). A sampled or shorter enumeration gives Point.

## Reorder facts

Each fact carries its own tier, which may be lower than Theorem 13's.

| Fact | Proven when | Refuted by |
| --- | --- | --- |
| trivial | T1 over the scope, or at the listed points (tier Point or ScopeEnumerated) | A point with two contributions to a cell |
| integer | All of these hold: G4a verified with an integer fold; no `nsw` or `nuw` on the reduction add; no frame read of the output; no stop or guard reading the output, or any state derived from memory. It rests on G4a's event fold alone, not on Theorem 12 | A replayed memory where the original completes and reversing a cell's contributions overflows a flagged add |
| float | Only by trivial | A replayed memory where reversing a cell's contributions changes its final bits |

Equal final values are never taken as enough, and associativity is never assumed.

## Evidence

| Tier | Route |
| --- | --- |
| Point / ScopeEnumerated | Exact enumeration of S(v) against the run's events at each point, for all fills (the trace is fixed by obliviousness) |
| Symbolic | X, D, M, K1, C1, T1, A1 over the scope |

Statuses:
- **Proven** carries a tier.
- **Refuted** carries a concrete parameter point, and a memory for a value fact. Any run
  reproduces it, because coverage depends on parameters alone.
- **Unknown** covers every solver unknown, limit, refusal and cut walk.

A `CheckedDomain` and a `ReductionVerdict` have no public constructor of a result. Both
are bound to:
- the program fingerprint, group and scope;
- the authorizations;
- the request (levels, guard, key, closed forms) and the family.

`checkReduction` runs the G3 and G4a checkers itself. `transformation` stays Unknown, and
the JSON carries `"authorizes_lowering": false`.

## Defects found and fixed before commit

The independent review found these, and each now has a regression test (`reviewed`):

- **R1, false Verified.** Two sites in one block were taken as one event per arrival, so a
  symbolic claim held of a program with two events per point. Fixed by gate X2.
- **R2, point evidence reported beside a symbolic claim.** The trivial fact fell back to
  the finite route's "proven at the listed points" while Theorem 13 was symbolic, and the
  float fact copied it. Fixed: facts carry tiers.
- **R3, false Proven.** The integer fact missed a stop reading a partial sum through a loop
  carried value. Fixed: the condition uses dependence on memory through state, not loads
  alone.
- **R4, binding.**
  - The family and the closed forms were not bound.
  - A finite tier trusted the scope it was handed, not the one enumerated. **G4a's committed
    finite route had the same hole**, now closed there by the same check.
  - The verdict was a public struct.
- **R5, μ could wrap at 128 bits** with two 64-bit repeat levels. Fixed by the width test.

Found here first:
- The proposer's derived levels: an exact bound [x, x+1) is empty where x + 1 wraps. Each
  such level is now compared signed or unsigned as its samples require. This was a
  proposer error, and the checker refuted it.
- The name `MultiplicityForm` clashed with G3a's and was renamed.

## Limits

- **Unknown symbolically:**
  - two sites of a group in one block;
  - zigzag or other non-lexicographic orders;
  - a guard whose QF_BV queries time out;
  - μ that varies over the domain;
  - repeat levels wider than 127 bits in total.
- **Finite only:** domains whose box is larger than the limit at a point give Unknown
  there.
- **Proposer:** it only reads conditions over levels held in plain state, constants and
  small congruences. A domain needing other atoms gets no proposal, which is Unknown and
  no error.
- **Inherited from G3b and G4a:**
  - only oblivious programs and completing runs;
  - walks bounded by depth;
  - sibling loops have no single λ (G5b);
  - output initialization stays Unknown symbolically in G4a.
- **Width:** S(v) is fixed-width. Whether a bound wraps is visible in the terms and is
  G6's to read correctly.

## Changes to earlier phases

- **G3b:** `walkToNextEvents` exports the existing walker. `proveCoverage` is unchanged.
- **G4a:** `enumerationCoversScope` is new. The finite route now claims ScopeEnumerated
  only for an enumeration of exactly its scope (R4 above), which changes no existing
  output, because every caller passes matching scopes.
- **G1, G2a and G3a:** unchanged.

## Validation (2026-11-05, LLVM 24.0.0git, Z3 4.8.12, base 4ecb778)

Every result below comes from one frozen tree, unchanged from the start of the run to its
end, with the suites run one at a time.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 18 of 18 CTest suites, G1 to G5a and 18 lit tests |
| G5a unit, core | 356 checks, 0 failures |
| Orders, exhaustive | Every order and direction of three loops against every other: 4,704 requests, 48 verified, each on both routes, 0 false |
| Holes, exhaustive | All 512 holes of a 3 x 3 box, each against its own guard and the 9 one bit away: 15,361 requests on both routes, 511 verified (mask 0 has no event), 4,608 refuted, 0 false |
| Oracle, seed 20261105 | 131 programs, 116 with claims, 348 points against S(v), 7,697 Theorem 13 runs with random fills, 4,308 permutations, 45 witnesses replayed, 0 disagreements, 0 timeouts. 12 had no proposal and 3 no candidate: Unknown, no error |
| Mutation | 87 mutants: 79 killed by a test's own assertion and 8 equivalent, with reasons and evidence in `domain_mutation.py`. 0 survived, 0 timeouts, 0 resource limits, 0 harness errors. One worker, a 6 GB cap per process |
| ASan, UBSan, leaks | Clean: G5a core 356, G4a 2,883, G3b 517, G3a 355, and 8 oracle programs |
| Byte-identical | G1 42, G2a 14, G3a 14 and G3b 14 outputs against 2542b68, and G4a 16 against 4ecb778 |
| Polynomial suites | 13 of 13 |
| `check-systolic` | 188 of 190: the two known legacy failures |

The first mutation run, before the second round of tests, had 33 survivors and 4 harness
errors. None was a defect in the checker; each was a test that a stronger check or an
earlier gate masked, and several stood where a false claim would have hidden. Each now has
a test, or is listed as equivalent with its proof.

## Trusted base

Everything in the G1 to G4a records, including Z3's unsat answers. The Python oracle tries
to break claims and proves nothing.

## Build and test

```sh
ninja -C <build> check-generalized-gemm
DOMAIN_FULL=1 <build>/generalized-gemm-domain-test holes
python3 research/generalized-gemm/test/fuzz/domain_mutation.py --source . --scratch <new dir> \
  --results <file> --llvm-dir $L/lib/cmake/llvm --mlir-dir $L/lib/cmake/mlir --workers 1
```
