# Generalized GEMM recognition: Phase G2a, path-sensitive termination

G2a analyzes the transition system of a G1 `SemanticProgram` and tries to prove
that it terminates. It is research-only: nothing here is linked into production
lowering, and it issues no GEMM verdict. The G1 record is
`docs/GENERALIZED_GEMM_G0_G1.md`.

Decisions recorded on 2026-11-01:

1. Four verdicts with exact distinctions. `IF_DEFINED` is never promoted.
2. Minimal additive exports from G1, with no change to its behaviour.
3. `REFUTED_NONTERMINATING` only from an exact, replayed, complete-state cycle.
4. P2b is an optional rank proposer. Its verdicts are not read.
5. The FSM branch is not integrated. SCCs are computed over G1 transitions.

## Verdicts

Each claim quantifies over the domain: the given parameter scope, defined
parameters, non-negative memref extents, and fully initialized argument memory.

With `--lower-structured` the claims are about the lowered CFG, whose entries
include some that the source forbids, such as `scf.for` with step zero. A proof
covers those too, and stands. A cycle does not refute the source, so it gives
`UNKNOWN`, and a replayed stop may be such an entry.

| Verdict | Meaning |
| --- | --- |
| `PROVEN_TERMINATING` | Every run returns |
| `PROVEN_TERMINATING_IF_DEFINED` | No run is infinite, but a run may stop on a side condition. Undefined behaviour allows anything, so this says nothing about a compiled program on such a run |
| `REFUTED_NONTERMINATING` | One entry in the domain, replayed exactly, repeats a complete configuration |
| `UNKNOWN` | Anything else, including every failure. It never implies nontermination |

`CheckedVerdict` has no public constructor for the first three. They come only
from `checkCertificate` and `verifyNonterminationWitness`. Proposers are in a
separate library that the checker does not link.

A proven verdict comes with a report per side-condition kind (UB, underspecified,
model limit): `UNREACHABLE` (all of O7 discharged), `REACHABLE` (a replayed entry
stops that way), or `UNKNOWN`.

## Exact integer semantics

Everything is G1's bitvector semantics at exact widths. In particular:

- Unflagged arithmetic wraps. `nsw` and `nuw` overflow is deferred poison, and
  only a branch on it is undefined. They are different programs: at i8,
  `i<n; i+=2` diverges under wrap for n=127 and hits UB under `nsw`.
- C is not an input. The C loops in the tests are hand translations, in `nsw`
  form and in unflagged form (like `-fwrapv`).
- C overflow is immediate UB; MLIR `nsw` overflow into a dead value is not, and
  a dead value leaves no trace in a `SemanticProgram`. So `IF_DEFINED` carries
  over to C (UB-free C runs are poison-free MLIR runs), and `PROVEN_TERMINATING`
  does not on its own.
- `index` results hold only at the stated `--index-bit-width`.
- There is no forward-progress assumption from C, C++ or LLVM.

## Proof rule

A cut set `C` breaks every cycle, so every run splits into paths from the entry
or a cut to the next cut or the exit. A path is a sequence of G1 transitions, so
parallel edges stay separate. `cond(π)` is its guards and the no-stop conditions
of its source blocks. `post(π)` is its simultaneous update.

`cond` excludes the block where the path ends. Otherwise O4 would not establish
the invariant at a state that is stuck on arrival, and O7 could not exclude that
state. (The design text said otherwise. This is the correction.)

Each formula below is the negation of the obligation, as a G1 term over the
configuration at the start of the path, so `unsat` discharges it.

| ID | Obligation |
| --- | --- |
| O0 | Structural, no solver: the model is supported; the fingerprint matches; the scope is well formed; the cut set breaks every cycle (recomputed); path enumeration is complete; invariants and ranks hold only existing integer leaves; the proof tree matches SCCs recomputed here, lists exactly each SCC's paths, and removes at least one |
| O1 | The domain is nonempty (`sat` expected) |
| O2 | At each location exactly one guard holds, so a run cannot vanish |
| O3 | Initiation: `Scope ∧ cond(π) ⇒ Inv_b(post)` for paths from the entry |
| O4 | Consecution: `Scope ∧ Inv_a ∧ cond(π) ⇒ Inv_b(post)` |
| O5 | Strict descent: `Scope ∧ Inv_a ∧ cond(π) ∧ Cont_S,b(post) ⇒ R_b(post) <u R_a` |
| O6 | Weak descent: the same with `≤u` |
| O7 | Each side condition: `Scope ∧ Inv_a ∧ reached ∧ violated` is unsat |

`Cont_S,b(c)` says some path of the current SCC `S` leaving `b` is enabled at
`c`. An infinite run inside `S` satisfies it at every step. So a step that leaves
the loop, or lands on a stuck state, owes no descent. One rule covers exit
overshoot and poison successors.

Ranks are `BV(W)` under unsigned `<`: a finite total order, so a rank is in a
well-founded domain by its sort. Wrapping inside a rank is not a soundness
problem: it only defines a different function, which must still pass O5.

**Why no infinite run exists (O0 to O6).** O3 and O4 give `Inv` at every cut
visit. An infinite run visits cuts forever, and eventually stays in one SCC of
the path graph. In a proof node, all paths weakly decrease a well-founded rank
and the strict ones decrease it, so strict paths occur finitely often. The tail
then lies in an SCC of the remaining paths: a child node, which `O0` proves
exists. Each node removes a path, so this ends in a contradiction.

**Why every run returns (with O7).** A run that is not infinite ends at the exit,
at a stop, or with no enabled guard. O2 excludes the last. O7 excludes each stop:
all earlier blocks did not stop, and `Inv` holds even at a state stuck on arrival.

### Things the obligations do not see

- An uninitialized read is no side condition, and the relation leaves it free.
  That over-approximates, so "no infinite run" stands. It would be wrong for
  "every run returns", so any read of a fresh allocation keeps the underspecified
  report at `UNKNOWN`. Initialization is not tracked.
- Side conditions containing a float term are dropped from `cond`. A premise only
  weakens, so proofs cover more runs. They stay O7 obligations. This is required:
  leaving FP terms in a query turned 60 ms into `unknown` in the prototype.
- Under `reassoc`, `contract`, `arcp` or `afn`, an O7 over a float term covers the
  expression as written. That report stays `UNKNOWN`, so such a program does not
  reach `PROVEN_TERMINATING` through it. Floats never reach control: `cmpf` and
  FP casts are refused.
- If O1 to O6 read memory, the verdict rests on G1's root-separation assumptions,
  and without authorization it is `UNKNOWN`. If only an O7 needs it, that stop
  stays `UNKNOWN` and the termination theorem is unaffected.

## Candidates (untrusted)

| Template | Form |
| --- | --- |
| T1 guard distance | `ext(b) − ext(a) + c`, `c` in {0, 1}, at width `w+2`, keeping the comparison's signedness; both signs for `eq`/`ne` |
| T2 modular distance | `c⁻¹·(t − x) mod 2^w`, for odd `c` with `x' = x + c` on every path of the SCC |
| T3 bounded affine | `Σ aₖ·ext(xₖ) + a₀`, `\|aₖ\| ≤ 4`, up to 6 atoms, by CEGIS on bit-level counterexamples with QF_LIA |
| T4 lexicographic | SCC slicing over T1 to T3, one rank per node |
| P2b hint | `generateCandidates` only, with the bound taken from the model's own comparisons |

T2 proves wrapping `i<n; i+=3`: `i<n` implies `i≠n`, so the number of steps left
is nonzero and falls by exactly one, with no overflow argument.

**Invariants** are Houdini: start from a candidate set per cut and drop what a
solver model falsifies. Candidates: equalities to values that depend on no state
(this is what makes an entry-block constant visible, since `T` has it as state),
`¬shadow`, each comparison in a path condition and its negation, signs, bounds
from those values, low-bit and odd-modulus congruences, and orderings among loop
variables and parameters. Each may be excused where a variable is poison. The
result is proven by O3 and O4 in the checker.

**Witness search** replays model parameters from failed obligations, then boundary
values, with Brent's algorithm in constant space. Each cycle is re-verified from
its entry by `verifyNonterminationWitness`: the configuration after `prefix`
steps must equal, in location, state, shadow and every memory cell, the one
`period` steps later. A cycle beyond the fuel is `UNKNOWN`.

If a replay contradicts a proof, the verdict becomes `UNKNOWN` and `conflict` is
set. That would be a defect, and neither side is believed.

## Supported and unsupported

Supported: any CFG G1 imports, including multiple exits and latches, `cf.switch`,
parallel edges, nested and irreducible loops, symbolic bounds, signed and
unsigned guards, and non-affine updates as far as Z3 can finish.

These give `UNKNOWN`, with a reason:

- Every G1 refusal: external, indirect or recursive calls, callees with cycles,
  and the rest of the list in the G1 record.
- A proof needing a polynomial, multiphase, disjunctive or memory-reading rank,
  coefficients beyond the cap, or an invariant outside the candidates.
- Any solver timeout, resource limit, error or refused script.
- A path, term, query or candidate limit. A limit can only lose a verdict.

Known misses, with exact truth at i8 in the test table: piecewise `+3/+7` under
wrap with the full domain, `i+=k; k+=1` under wrap. Both terminate.

## Trusted base

- Everything in the G1 record's trusted base.
- Z3 4.8.12's `unsat` answers. There is no proof object and no second solver.
  Replay uses the same Z3, so it finds wrong records and hidden state, not a Z3
  bug. The exhaustive oracle covers only small widths.
- A crash inside the Z3 library cannot be contained in process. API errors can.

## Changes to G1

Additive only: `smtSortText`, `smtTermName`, `encodeTerms` in
`RelationalEncoding.h`, and `startSymbolicPath`, `extendSymbolicPath` in
`SymbolicEvaluator.h`. `encodeTransitionRelation` and `summarizeAcyclic` now call
the same code.

## Known limitations

- `PROVEN_TERMINATING_IF_DEFINED` says no run is infinite. It does not say a run
  returns normally, and it is never promoted.
- Under `--lower-structured` a proof stands, and a cycle gives `UNKNOWN`: lowering
  drops source preconditions such as a positive `scf.for` step.
- With `reassoc`, `contract`, `arcp` or `afn` anywhere in the program, a discharged
  float O7 stays Unknown: it covers the evaluation as written, not the others.
- A read of a fresh allocation keeps Underspecified Unknown, because initialization
  is not tracked.
- Memory in O1 to O6 gates the verdict on the root separation assumption. Memory
  in an O7 only leaves that stop Unknown until the assumption is authorized.
- The model has `nsw` poison, not C's immediate overflow, and a dead overflow is
  invisible to it. A C-level claim needs the front end to preserve that.
- Replay fuel limits refutation, never a proof. A long cycle gives `UNKNOWN`.

## Validation (2026-11-01, LLVM 24.0.0git, Z3 4.8.12, base b0f0814)

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 8 of 8 CTest suites: G1 unit, relational, oracle fuzz, native differential; G2a unit, oracle, replay; lit |
| G2a unit | 119,466 checks, 0 failures, with 31 proofs checked without a solver over 2,385 entries and 10,674 path segments |
| Oracle, fresh seed 20261102 | 2,000 programs plus the catalogue, 35,898 entries, 422,151 certificate checks, 0 disagreements |
| Verdicts in that run | 854 proven, 549 proven if defined, 665 refuted, 70 unknown; 9 functions lie outside the oracle |
| ASan, UBSan, leaks | Clean: unit, and 200 programs plus the catalogue with 0 disagreements |
| G1 output | Byte-identical to b0f0814: 162 outputs over 1,857 functions |
| Mutation | 72 of 72 killed, baseline passing first; see below |

The full mutation run killed 65 of 72. The other 7 had tests added and are killed
in a rerun on the final tree. The 65 ran on the tree before those test additions
and were not repeated, since only tests and one comment changed.

Two of the 7, `empty strict accepted` and `listed paths not compared`, cannot change
a verdict for the worse: the certificate is refused later, or the computed paths are
the ones checked. Their tests pin the diagnostic only. The other 5 are soundness or
precision tests, and `children not discharged` and `witness run may end` were real
gaps in the suite. The code was correct in both.

Polynomial suites (13 of 13) and `check-systolic` (188 of 190, the 2 known legacy
failures) ran before those last test additions. Neither builds any file that changed
after that run. No FPGA or hardware path is involved.
