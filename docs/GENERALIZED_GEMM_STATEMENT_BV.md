# Statement-level index programs: the bit-vector adaptation of the thesis method

This record covers phase 1 of implementing Section 4.4 and Appendix G of the thesis
(semantics-guided synthesis of index programs) inside the generalized GEMM research
build. The whole effort is named, here and in the code, **the bit-vector adaptation of
the thesis method**（論文方法的 Bit-vector 改編版）: the thesis states its grammar,
semantics and specification over the mathematical integers; this implementation states
them at the exact bit widths of the program under analysis, and the differences are
listed below, not hidden. Research-only: nothing here is linked into production lowering,
and no result authorizes a transformation.

Phase 1 is not the method. It is the loop-free fragment, wired into the existing search
and checkers. Phase 2 (Section 7) adds the slice, the inductive specification and proved
loop-free closed forms; phase 3 (Section 8) while candidates at the finite tier; phase 4
(Section 11) their correctness and termination proofs for one while template, and the
counters, identities and views that connect the closed forms to the recognition pipeline;
phase 5 (Section 12) the arrays of Appendix G.5 in the grammar and the semantics, and the
synthesis and proof of one table template. Section 13 is an inductive invariant path for
the result of a scalar loop; Section 14 carries that path into the GEMM itself, for the
scalar state its subscripts read, and hands what it proves to the coordinate and domain
checkers.

## 1. What the thesis has and has not done, as it says itself

| Part | Thesis status | Where |
| --- | --- | --- |
| Statement grammar G_S: P, S, E, B with skip, assignment, sequence, if, while, return | defined | G.1 |
| Semantics: one CHC relation per nonterminal, least fixpoint, partial function | defined | G.2 |
| Specification (G.2): a positive atom at 0 and an implication for t+1, totality by induction | defined | G.3 |
| Example P_K = (t+1)!, hand proof, z3 check of the invariant's three conditions | done, "checks the hand derivation, not a synthesis" | G.4 |
| Arrays: partial maps, no rule for an out-of-bounds or never-written read, P_L lattice table | defined and hand-checked | G.5 |
| Slice from the six subscripts on the CFG of Definition 3.1 | **not implemented** (only on the prototype's model read from C) | 4.4, F.7 |
| A SemGuS-Lib problem file | **not written** | G.4, G.5 |
| Any SemGuS solver run | **none** | G.4, G.5, 2.5 |
| Synthesis of programs with array writes | **not done** (phase 5 here synthesizes one template of them, Section 12) | G.5 |
| Machine integers inside Appendix G | **ignored** (bridged elsewhere by Lemmas F.1, F.6, F.11 for expressions) | G.2, G.4 |
| Bit-vector grammars | listed as future work | Chapter 6 |

So there is no thesis experiment to reproduce for statement-level synthesis. What can be
checked against the thesis are the hand-derived values of Table G.1 and the stated rules,
and those are unit tests here.

## 2. Phase 1: what is implemented

**Data and meaning** (`include/GeneralizedGemm/IndexProgram.h`, `lib/.../IndexProgram.cpp`,
in the tier-checker library):

- `IndexProgram`: the grammar G_S as an arena of nodes. Inputs are terms over state and
  parameters (read, never assigned, as the thesis's t); locals start at zero (the σ_t
  convention of G.2); every node has an exact width; division and remainder take a
  positive constant divisor.
- `runIndexProgram`: the rules of G.2, one `case` per rule, at bit-vector widths. Sem_E
  and Sem_B are total. Sem_S is partial through `while` alone, and a run that exhausts its
  fuel returns `OUT_OF_FUEL` with no value. That outcome is recorded as what it is: no
  value, no statement about termination either way.
- `indexProgramTerm`: a loop-free program lowered to a `Term` of the same meaning
  (assignment substitutes, a branch becomes `ite` per changed local, a sequence
  composes). `nullopt` for a program with `while`. The lowering is **tested against the
  interpreter** on random programs and inputs and is not otherwise trusted.
- `formatIndexProgram`: deterministic text in the grammar.

**SemGuS-Lib style text** (`IndexProgramSemgus.h/.cpp`, in the synthesis library):
`exportSemgusProblem` writes the three parts in the structure the thesis describes for the
format: `declare-term-types` with one constructor per production, `define-funs-rec` with
one Boolean function per nonterminal matching on constructors, `synth-fun`, example
`constraint`s and `check-synth`. `semgusTerm`/`parseSemgusTerm` print and read a candidate
in constructor form. **Status, written into the file's own header: no external SemGuS
parser has read this text and no SemGuS solver has been given it.** The only checks are
`semgusExportWellFormed` (balanced parentheses, known top-level commands) and the round
trip (print, parse, run the same), which check this code against itself and nothing else.
There is no external SemGuS integration in this repository, and this record does not claim
one.

**Grammar** (`CoordinateGrammar.h/.cpp`): `GrammarSpec::statements` adds the loop-free
statement productions as a last enumeration step at the level in force:
`if B then v := E else v := E ; return v`, nested up to `maxConditionals`. The conditions
B are, first, the comparisons the program itself branches on (the guards γ_e of the slice
in Section 4.4), then comparisons of operands with constants and with each other
(`=`, `<`, `<=`, unsigned); a condition constant on the samples, or the complement of one
already kept, is dropped, since swapping arms gives the negation. The arms E are the
operands of the level. Each program is lowered and offered like a grown term; two programs
equal on the sampled events keep one. Program atoms have their own cap (`maxProgramAtoms`)
so they never displace the term atoms. `atLevel` is unchanged: `statements` is false at
every level, and `SynthesisOptions::statementGrammar` (CLI `--statement-grammar` on the
synthesis, computation and domain reports) turns it on.

**Record**: `Atom::program` and `CoordinateFamily::programs` carry the program text next to
the lowered term; `origin` becomes "selector search over statement programs"; the JSON
gains `index_programs`. Text only: every checker reads the term.

## 3. Semantics: thesis versus adaptation

| | Thesis (Appendix G) | This adaptation |
| --- | --- | --- |
| Values | σ: V → Z, overflow ignored | σ: V → BV_w at the program's widths, wrapping |
| div, mod | Euclidean, constant divisor C | unsigned `udiv`, `urem`, nonzero constant divisor |
| Comparisons | =, <, ≤ over Z | =, unsigned <, unsigned ≤ (signed comparisons are outside the grammar) |
| Partiality | nontermination, division by zero, array misuse | nontermination (fuel) and, from phase 5, array misuse (index outside the extents, read of an element without a value: `NoRule`); expressions over scalars are total |
| Specification | (G.2) inductive relation over t | phase 1: the sampled events as examples for the search; the inductive form is phase 2 |
| Meaning of acceptance | a program satisfying (G.2) | a term whose labels the existing checkers accept at the stated tier |

Why bit vectors, and why this is a deviation and not a shortcut: the recognizer compares
coordinates for equality at exact widths (`CoordinateFamily.h`), its term language has no
integer sort (`SemanticTerm.h`, invariant I1), and the thesis bridges Z to the machine
only through per-candidate range lemmas (F.1, F.6) that would have to be proved for
every candidate here. The cost of the deviation is stated in Section 4.

## 4. "No wrong acceptance": premises and the evidence for each

The claim is narrow: **enabling the statement grammar cannot make the checkers accept a
family they would otherwise refuse.** It rests on these premises, each with its evidence.

- **P1. A program is a proposal; the verdict is the checker's.** `buildAtoms` and
  `synthesizeCoordinates` produce `CoordinateFamily` values; acceptance is
  `checkFamilyFinite` (G3a facts over the exact `AccessTable` of each enumerated point)
  and `checkFamilySymbolic`, which link neither the grammar nor the search
  (`research/generalized-gemm/CMakeLists.txt`, the tier-checker and synthesis targets).
  The statement grammar adds terms to the same input; it touches no checker.
- **P2. The term is what is judged, at the event's exact state and width.**
  `familyLabels` evaluates each axis term with `EventEvaluator` at the recorded state of
  every event and demands `definedness`; a term with no value or undefined at an event
  refutes the family (`Refutes::ThisFamily`). Program terms use `ite`, `icmp`, `udiv`,
  `urem` with nonzero constants, all of which `ConcreteEvaluator` and `RelationalEncoding`
  already implement. Evidence: the unit test checks every program atom's value at every
  event against the interpreter's run of its program.
- **P3. The program text and the term agree.** `indexProgramTerm` is tested against
  `runIndexProgram` on 300 random loop-free programs × 8 inputs at width 8, and the
  constructor-form round trip reproduces each program. A disagreement would be a wrong
  *record* of what was judged, never a wrong verdict, because the term is judged.
- **P4. Wrapping is the program's own semantics, not an assumption about it.** Events come
  from `runConcrete` at the program's widths; a run that stops on a side condition
  (signed overflow with `nsw`, division by zero, ...) yields no domain, so no event with
  undefined behaviour is ever labelled. Equality of labels at width w between a wrapped
  candidate value and a wrapped program value is exact equality of machine values. What
  the adaptation gives up is the other direction: a program correct over Z whose
  intermediate values leave the width is rejected, never accepted. Evidence: the P_K
  test shows 12! equal to the integer and 13! differing from it at 32 bits.
- **P5. The whole pipeline still runs on the family.** The computation (G4a) and domain
  (G5a/G5b) checkers consume the verified family unchanged. The lit test runs the domain
  report with `--statement-grammar`: on `two_phase` every later claim is `VERIFIED` over
  the scope on the statement-based family; on `rotated` the later stages say `UNKNOWN`
  with the proposer's reason. A family accepted at the coordinate tier is **not** a GEMM
  claim; the later stages say what they say, and a candidate index program counts as
  recognized only when they do.
- **P6. Claims are per enumerated point or scope.** Phase 1 produces `POINT` or
  `SCOPE_ENUMERATED` at most. The symbolic tiers need their own obligations, which exist
  for terms (`checkFamilySymbolic`) and are not changed here; a timeout, a resource limit
  or a solver's unknown stays `UNKNOWN`.

What the premises do not cover: whether a *found* family is the "natural" one, and
whether the statement grammar finds a family at all; a grammar that lacks a function says
nothing about it (Section 2.5 of the thesis, and `SynthesisAnalysis.h`).

## 5. The examples of phase 1

Both are in the unit tests and in `test/lit/synth_statements.mlir`; both have one
counter t over [0, 2n) with the guard t < n choosing the phase, index width 8, scope
n ∈ {2, 3}.

**`two_phase`**: column 0 reduces in the order t, column 1 in the order t − n. The
expression grammar reaches a full family at level 2 (t div n, t mod n). With
`--statement-grammar` the search returns at level 0

```
J = if t < n then (v0 := 0) else (v0 := 1) ; return v0
K = if t < n then (v0 := t) else (v0 := (t - n)) ; return v0
```

and that family, judged by the unchanged checkers, reaches `SYMBOLIC_EXACT` at the
coordinate tier, and the domain report's computation (finite and symbolic), domain (finite
and symbolic, exact order) and Theorem 13 claims are all `VERIFIED` over the scope. This
is the instance of P5: a statement-based family going through the complete pipeline.
`authorizes_lowering` stays false, as for every research result.

**`rotated`**: column 1 reduces in the order (t + 1 − n) mod n. Within levels 0 to 3 the
expression grammar has no full family (the test asserts it); the statement grammar's

```
K = if t < n then (v0 := t) else (v0 := (((t + 1) - n) mod n)) ; return v0
```

is accepted by `checkFamilyFinite` at `SCOPE_ENUMERATED`, and swapping its arms at one
phase is refused. The later stages report `UNKNOWN` with the proposer's reason, "no guard
from the program's conditions is exact at the samples": the G5a domain proposer fits one
guarded lexicographic order over the family, and a phase that walks K in rotated order has
none. That is a limit of the later stage, recorded as such; nothing is claimed past the
coordinate tier for this program.

**`reversed`** (second half walked backwards) is a control: the expression grammar already
has a coordinate there, t · (2n − 1 − t), shared by both phases, and the option changes no
verdict.

## 6. Tests

- `SynthesisTest.cpp`: `testIndexPrograms` (every G.2 rule; Table G.1 values 1, 2, 6, 24,
  120 at 32 bits; the 13! wrap; fuel; ill-sorted builders; lowering versus interpreter;
  SemGuS text well-formedness, the unchecked-status marker, reader refusals) and
  `testStatementGrammar` (default off; the bank; every program atom's values; the search
  with and without the option; the forged family; `reversed` as control).
- `test/lit/synth_statements.mlir`: the reports with and without `--statement-grammar`,
  and the domain report on the statement-based family.
- Default output unchanged: with the option off, the report binaries produce the same
  text as the pre-change binaries on the lit inputs (compared in the phase record).

Results and the remaining limits of each phase are in the phase reports
(`~/work/integration-backups-20261113/semgus-statement-synthesis-design/PHASE1.md`,
`PHASE2.md`).

## 7. Phase 2: slice, inductive specification, closed forms

Phase 2 implements the thesis's "division of labour" (Section 4.4) and "synthesis from
the model" (Appendix F.7) on the `SemanticProgram`, with the project's own enumerator
in place of cvc5. Files: `IndexSlice.h/.cpp` (tier-checker library: slice,
specification, replay, obligations) and `ClosedFormSynthesis.h/.cpp` (synthesis library:
samples, goal-directed enumeration, CEGIS, JSON). CLI: `--slices` on the synthesis
report adds a `closed_forms` object; without it the output is unchanged.

**The slice** (`sliceSubscripts`). For every distinct subscript term of every MAC site:
its state support, closed under the assignments to those variables and the guards of the
transitions that carry them. `Transition.assignments` and `Transition.guard` are the τ_b
and γ_e of the thesis's Definition 3.1, so this is the slice the thesis describes and did
not implement on a compiler's graph.

**The loop specification** (`LoopSpecification`), per cut location h of the certificate's
cut set, for innermost loops only (no path from h reaches a cut that h dominates), as the
thesis restricts F.7 to loops without an inner loop:

- the paths h → h of `enumerateCutPaths` are the iterations; c is the disjunction of their
  conditions and τ_v the ite over them of each path's end state of v, exactly the terms the
  termination checker uses for its own obligations;
- carried = the slice's variables the loop assigns, closed under what c and the τ read
  among the loop's other assigned variables (F.7: "closed under the dependences inside the
  loop"); fixed = what else is read; a fixed variable that every entry path defines by the
  same term over parameters alone, and no cut path assigns, is read as that definition
  (a materialized constant, a bound computed before the loop);
- init_v is the entry path's term when it is over parameters alone and unique (a loop at
  depth 0, the thesis's "closed term"), otherwise the symbolic entry value entry_v;
- m and entry_v are synthetic State leaves past the program's state; m has the index
  width.

**That the specification is the program's** is checked, not assumed, by `replayLoop`:
during a concrete run from each enumerated point, every arrival at h after the first of
an activation must satisfy c and τ from the previous arrival, c must be false where the
loop is left, and the subscript evaluated at the entry of the block that emits an event
must equal the index the event recorded. The JSON reports iterations, step and condition
mismatches, subscript matches and mismatches, and undefined evaluations. This is test
evidence for the replayed points; the construction from the cut paths is the structural
argument. The unit test also forges a specification (a step off by one, a negated
condition) and checks that the replay reports the mismatches.

**The obligations** (`checkClosedForms`), the thesis's (F.2) at bit-vector width w with
the parameter scope as premise:

```
CF-init   scope ∧ f_v(0) ≠ init_v                              (init_v, or entry_v)
CF-step   scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ f_v(m+1) ≠ τ_v(F(m))   (F: every carried variable by its form)
```

Both `unsat` → `PROVED`, which means: at width w, for every m in [0, 2^w − 2] and every
parameter in the scope, the form satisfies init and step. `sat` → `REFUTED` with the
model (m, entry values, fixed variables, parameters). Anything else → `UNKNOWN`, with the
solver's answer in the record. A form whose c or τ reads a carried variable without a
form is `UNKNOWN` with that reason. Nothing in these statuses is about the integers.

**The synthesizer** (`synthesizeClosedForms`). Samples are the replayed traces: inputs
(m, entry values, fixed variables, parameters) and the carried values at m. Per variable,
expressions over the leaves and the constants (0 to 8, the program's literals, extras)
with +, −, ×, div and mod by a constant are enumerated by size (default up to 5), one per
distinct value vector on the samples; a direct match is the candidate, otherwise one
branch `if B then a else b` is sought over conditions (=, <, ≤ of a small term against a
leaf or constant) whose two sides are covered by terms agreeing with the target there.
The candidate is checked; a refuting model is replayed through the specification itself
(`runSpecification`) into a new trace, and the search runs again, up to 6 rounds. The
record prints the form as a statement program (`v0 := ... ; return v0`, or one `if`).

**Results** (unit test `testSliceSpecification`, lit `synth_slices.mlir`):

| Program | Loop | Form | Status | Rounds |
| --- | --- | --- | --- | --- |
| `parity` (Listing E.5 as a reduction, i += i even ? 7 : 3) | the one loop, depth 0 | i: `if (m mod 2) = 0 then 5*m else 2 + 5*m`; k: `m` | PROVED at width 8, n ∈ [2, 6] | 1 |
| `gemm` | k loop | k: `m + entry_k` (phase 2); `m` since phase 5 read the entry k := c0 as the constant 0 (Section 12.4) | PROVED at width 8, n ∈ [1, 3] | 2 in phase 2; 1 since phase 5 |
| `two_phase`, `rotated` | t loop (two paths: the two phases) | t: `entry_t + m` | PROVED at width 8 | 2 |

The parity form is the thesis's Proposition 4.3, 5n + 2(n mod 2), here as a branch and at
width 8: the test checks the values 0, 7, 10, 17, 20, 27, 30 and that the form wraps at
m = 52 as the program does, which the integer statement does not. Replays: 0 step, 0
condition, 0 subscript mismatches on every point of every program above; parity 40/40
subscript matches over 5 points, gemm 72/72 over 3.

**Negative and non-proof controls** (unit test): the forms `m` (wrong init) and
`entry + 2m` (wrong step) for gemm's k were `REFUTED` with models naming entry_k and m (since
phase 5 the init is known, and the controls are `entry + m`, refuted at init with the entry in
the model, and `2m`, refuted at the step), and
the model run through the specification disagrees with the wrong form and agrees with the
right one; a stub backend that always answers unknown gives `UNKNOWN` on every form and
a record with no discharged obligation; a stub that answers sat with no model gives
`REFUTED` with an empty model, never `PROVED`.

**Limits of phase 2.** Innermost loops only; one branch per form; m at the index width,
so a carried variable of another width gets no form; constants 0 to 8 are a choice of this
implementation (the thesis's grammar G for Listing E.5 also lists 5 by hand); the
enumeration and the condition search have size and count limits, and a missing candidate
is `UNKNOWN`, not evidence of absence; the random and sampled checks are test evidence for
the points and programs run, not a proof about other programs; the closed forms are
reported beside the coordinate families and do not yet feed the coordinate search (the
iteration-count input of the design note is not wired in).

## 8. Phase 3: while programs at the finite tier

Phase 3 adds candidate index programs with `while`, run by the interpreter of Section 2
with a fuel, and a finite-tier CEGIS. It changes no checker of the coordinate pipeline.

**Where while programs enter.** As index programs for slice variables (Section 7), when a
variable has no loop-free form in the grammar or its loop-free candidates were refuted.
They do not enter the coordinate grammar of Section 2, for a reason the examples made
plain: for a program whose subscripts are terms over the live state, the subscript of A
(selected per site by the site's guard, an `if`) is itself a loop-free coordinate, so a
`while` coordinate is never needed at the finite tier; it would be needed only where a
subscript is not such a term, which is the table case of Appendix G.5 (phase 5).

**The shape** is Appendix G.1's P_K:

```
r := 0 ; v := init ; while r < m do (v := E ; r := r + 1) ; return v
```

The body E is found from the sampled steps: every pair (v(m), v(m+1)) of the seed traces,
with r = m, the other inputs and the proved loop-free forms of the other carried variables
at r, is a loop-free synthesis problem, which the goal-directed enumerator of Section 7
solves with one branch allowed. The body then becomes a program, and the program, not the
body, is judged.

**The judge** is `checkProgramOnTraces` (tier-checker library): the program runs at every
sampled iteration of every trace with the fuel `programFuel` (default 4096 statements) and
is compared with the recorded value. Equal everywhere → `SAMPLE_CONSISTENT`, whose reason
text says "at the finite tier; not a proof". A difference → `REFUTED` with that sample as
the model. A run cut by the fuel → `UNKNOWN`, reason "no value there, and no claim about
termination". `PROVED` is not reachable for a while program in this phase; the symbolic
obligations and the termination argument are phase 4 (Section 11), which keeps
`SAMPLE_CONSISTENT` as the status of a program the solver did not decide.

**CEGIS at the finite tier.** Seeds are the traces of the first point. A body fitting the
seed steps gives a program; the program is judged on every trace; a failing trace joins
the seeds and the search runs again, up to `maxWhileRounds` (6). The thesis's remark that
finitely many examples are satisfied by infinitely many programs is visible in the
record: `factorial` needed three rounds and two counterexample traces, and `stepped` on a
narrow scope accepted a body that fits every sampled iteration and is wrong beyond them,
which is exactly what `SAMPLE_CONSISTENT` says and `PROVED` would not.

**Results** (unit test `testWhilePrograms`, lit `synth_slices.mlir`):

| Program | Variable | Result | Status | Rounds / counterexamples |
| --- | --- | --- | --- | --- |
| `factorial` (a = (t+1)!, Listing G.1 as a reduction) | a | `r := 0 ; a := 1 ; while r < m do (a := a * (2 + r) ; r := r + 1) ; return a` | phase 3: SAMPLE_CONSISTENT, 20 iterations of 5 activations at 16 bits; phase 4 (Section 11): PROVED | 3 / 2 |
| `factorial` | t | loop-free `m` | phase 3: UNKNOWN, the condition reads a, which had no closed form; phase 4: PROVED with a as a free leaf | 1 / 0 |
| `stepped` (i steps 7 or 3 by parity, no counter), n ∈ [8, 40] | i | loop-free `if (m mod 2) = 0 then 5m else 2 + 5m` | PROVED at width 8 (init known: every entry into the loop sets i := 0) | 1 / 0 |

The unit test also runs the found factorial program on m = 0..8, where it returns 1, 2,
6, 24, 120, 720, 5040, 40320 and then 9! modulo 2^16, as the program would; refutes a
doubling program at m = 2 with that sample as the model; shows a fuel of 3 giving
`UNKNOWN` with the no-termination-claim text and a starved search proving nothing; and
shows the option off leaving every such variable `UNKNOWN`.

**Limits of phase 3.** One shape of while program; the body is a loop-free expression
or one branch over the current value, the counter, the inputs and proved forms; the
finite tier only, so a program consistent with all samples may be wrong beyond them, as
`stepped` on a narrow scope shows; termination is neither claimed nor refuted, the fuel
only cuts; nothing of this feeds the coordinate search (Section 9).

## 9. Integration backlog: closed forms into the coordinate search

The closed forms of Sections 7 and 8 are reported beside the coordinate families and do
not change what the search finds. This section states the gap, the acceptance cases
with their measured baselines, and the intended after-state, so that the integration is
judged by recognition results and not by the report.

**I1. Iteration counts as coordinates.** The symbolic tier needs an enumeration order of
unit steps (`proposeOrder`, Theorem 8); a loop whose only counter steps irregularly has
none, so its families stop at `SCOPE_ENUMERATED`. The iteration count m of the loop is a
unit-step coordinate, and a proved closed form i = f(m) relates it to the subscript. The
intended design is an instrumentation of the `SemanticProgram` with a counter per cut
(m := 0 on entry into the loop, m := m + 1 on the back edge), a semantics-preserving
extension whose events are checked equal to the original's by replay; m is then ordinary
state, the search can choose it, and the existing symbolic checker proves coverage with
no change to its obligations. Acceptance case: `stepped`, index width 8, scope n ∈
[8, 40].

| | Baseline (measured 2026-11-14) | Intended after I1 |
| --- | --- | --- |
| coordinate search | CANDIDATE_FOUND at level 3; every candidate uses a batch axis and products such as i·64 | a family with K = m_inner, J = j, no batch |
| symbolic tier | UNKNOWN, "no enumeration order to bound the coordinates", on all 5 candidates | SYMBOLIC_EXACT |
| domain report | not run (no order) | computation, domain and Theorem 13 VERIFIED |
| closed form | i = `if (m mod 2) = 0 then 5m else 2 + 5m` PROVED at width 8 | the same, reported as the view A[0][f(m)]; (V) of Definition 4.5 asks for an affine view, so this is a verified decomposition and not a GEMM mapping, and the record must say so |

**I2. Rotated phases.** `rotated` (Section 5) passes the coordinate tier and stops at the
domain proposer: one guarded lexicographic order cannot express a phase that walks K in
rotated order. Measured 2026-11-14: `--segments` (G5b) gives the same reason, "segment
of site 0: no guard from the program's conditions is exact at the samples". The
intended design is a proposer that admits, per segment, an order whose level is a proved
closed form of the segment's counter rather than the counter itself; it belongs with
phase 4, since the closed form must be proved for the order to be used symbolically.
Acceptance: `rotated`'s domain report from UNKNOWN to a VERIFIED or REFUTED verdict with a
stated order.

**I3. Views from closed forms.** When every subscript of a group has a proved closed form
over the iteration counts, the view of each operand is known as a function of (I, J, K);
the record should state whether it is affine, which is what a lowering needs and what
Definition 4.5 (V) demands. Acceptance: `parity` and `stepped` report the view
A[0][5m + 2(m mod 2)] as non-affine; `gemm` and `two_phase` report affine views.

Status after phase 4 (Section 11): I1, I2 and I3 are implemented and measured on the
acceptance cases; the measured after-state is in Section 11.5, next to what each case
still does not reach. The checker libraries link neither the grammar nor the synthesizer,
as before.

## 10. Later phases

4. Done: Section 11.
5. Done for one template, with the rest listed as not done: Section 12.

Separately from the numbered phases, Section 13 adds an inductive invariant path for
the result of a scalar function's one loop, and Section 14 inductive invariants for the
subscripts of a GEMM, consumed by the existing index checkers.

## 11. Phase 4: proofs for while candidates, counters as coordinates, views

Phase 4 adds the symbolic obligations for the while candidates of phase 3, including
termination, and connects the closed forms to the recognition pipeline (the backlog of
Section 9). Files: `IndexProgram.h/.cpp` (statement lowering, template matcher),
`IndexSlice.h/.cpp` (W obligations, free leaves, the induction fixpoint, entry
definitions), `ClosedFormSynthesis.cpp` (the symbolic round, model replay),
`LoopCounters.h/.cpp` (checker library: counters and the side-by-side replay),
`CoordinateGrammar.cpp` (a counter takes over an equal label kernel), `DomainProposer.cpp`
(identities in bounds, guards and views; views checked against the sampled indices),
`DomainAnalysis.cpp` (identities, certificate strengthening, views and the summary). CLI:
`--iteration-counters` on the synthesis, computation and domain reports,
`--closed-form-views` on the domain report. Without them nothing in the output changes
(Section 11.6).

### 11.1 The while shape this implementation supports, and what it does not

**The while search and the while proofs cover one template only**, the fixed-counter
template of `matchCounterTemplate`:

```
r := 0 ; v := init ; while r < m do (S ; r := r + 1) ; return v
```

with r and v distinct locals, init over inputs alone, m an input (the iteration count),
and S a loop-free statement that assigns v and no other local and does not read m. The
body S may branch. **This is not the general `while B do S` of G.1**: a loop with another
test, another counter step, a body that writes several locals or reads the bound is
neither searched for nor proved; `W-shape` refuses it and the record says
"not of the fixed-counter template". Appendix G.1's P_K is an instance of the template,
and the thesis's statement-level examples (G.4) have this shape; nothing wider is claimed.

### 11.2 Obligations for a while candidate, and why they prove it

For a carried variable v of a loop with specification (F.2) and a candidate W of the
template with body S, `checkClosedForms` asks the solver, at the width w of m and under the
scope:

```
W-shape   structural: the template matched, m is the iteration leaf, every input a leaf
W-init    scope ∧ init_W ≠ init_v
W-step    scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ S[v := s_v, r := m] ≠ τ_v(F(m))
W-term    r <u m ∧ ¬((m − (r+1)) <u (m − r))
```

where F(m) reads every carried variable that has a form as that form and every other one,
v included, as a free leaf. All four `unsat`, and every substituted form itself `PROVED`,
is `PROVED`; the reason text names the template.

*Lemma W1 (coupling).* For a program of the template, for every m < 2^w − 1 and every
input: W(m+1) = S[v := W(m), r := m]. Proof: the run on m+1 tests r < m+1 for
r = 0, …, m; the run on m tests r < m for r = 0, …, m−1. S does not read the bound, so the
first m iterations of both runs are the same statements on the same values, and after them
both hold (v, r) = (W(m), m) (r never wraps, since r ≤ m ≤ 2^w − 2). The run on m+1 then
runs S once more from that state, sets r := m+1, fails the test and returns S[W(m), m].
The run on m fails the test at r = m and returns W(m). ∎ The unit test checks W1 on the
factorial program for m = 0..6, which is evidence for those values, not the proof.

*Soundness of PROVED.* Claim: W(m) = v(m) at every arrival m of an activation of the loop
in the scope. Base: the loop does not run on m = 0, so W(0) = init_W, and W-init gives
init_W = init_v = v(0). Step: suppose W(m) = v(m) and the loop continues from arrival m,
so c(state(m)) holds. The specification (replayed against the program, Section 7) gives
v(m+1) = τ_v(state(m)). W1 gives W(m+1) = S[v(m), m]. W-step, instantiated at the actual
state(m) (its free leaves may take any value, so this one) and with every substituted form
equal to its variable by the simultaneous induction, gives S[v(m), m] = τ_v(state(m)). ∎
Reading v and the variables without forms as free leaves is what makes the obligation hold
for every value, hence for the actual one; the price is that an obligation that depends
on the actual values may be `sat` at an unreachable value, which is why a model over a
free leaf is **Unknown with the model kept, never REFUTED**, until the specification's own
run from that model (`runSpecification`) shows the disagreement.

*Termination.* W-term says the unsigned variant m − r falls by one at every iteration
while r <u m; with the template's `r := r + 1` as the only write to r (W-shape), the loop
runs exactly m times and the program terminates on every input. This is the termination
proof of the candidate program; the fuel of phase 3 is no longer the only word on it. A
run cut by the fuel is still `UNKNOWN` and still no statement about termination: the
proof is about the template, the cut is about a limit.

*Simultaneous induction.* A form's obligations may substitute the forms of the other
carried variables (a's step reads t). A form is `PROVED` only if every form it
substituted is `PROVED`: `checkClosedForms` computes the fixpoint over the list and
demotes the rest to `UNKNOWN` ("obligations discharged, but the substituted form of s<w>
is not proved"); the synthesizer then re-checks such a form with the proved forms alone and
the rest free. Phase 2 lacked this fixpoint, so a loop-free form could have stood on an
unproved neighbour; that gap is closed here.

*What a model means now.* Phase 2 called every `sat` `REFUTED`. Phase 4 replays every
model through the specification: a trace that disagrees with the candidate makes it
`REFUTED` (and a new sample); a trace that agrees makes it `UNKNOWN` with the reason
"the solver's model does not replay into a disagreement, so it refutes nothing". The
`stepped` program on n ∈ [8, 12] shows the difference: its loop-free candidate's step has
a model at m = 185, an iteration no run in the scope reaches, and the candidate is
`UNKNOWN`, where phase 3 had reported a `SAMPLE_CONSISTENT` while program fitted to the
same few samples.

**Results** (unit test `testWhilePrograms`, lit `synth_slices.mlir`):

| Program | Variable | Candidate | Phase 3 | Phase 4 |
| --- | --- | --- | --- | --- |
| `factorial`, 16 bits, n ∈ [1, 5] | a | `r := 0 ; a := 1 ; while r < m do (a := a * (2 + r) ; r := r + 1) ; return a` | SAMPLE_CONSISTENT | **PROVED**: W-shape matched, W-init, W-step, W-term unsat; depends on t |
| `factorial` | t | `m` | UNKNOWN (a had no form) | **PROVED** with a free |
| `stepped`, 8 bits, n ∈ [8, 40] | i | `if (m mod 2) = 0 then 5m else 2 + 5m` | PROVED | PROVED |
| `stepped`, n ∈ [8, 12] | i | `if 1 < m then 7m − 4 else 7m` | REFUTED, then a while program SAMPLE_CONSISTENT | UNKNOWN: the model at m = 185 does not replay |

Controls in the unit test: the doubling program's W-step has a model and is `UNKNOWN`,
not `REFUTED`, with a free in the reason; the factorial program alone (t's form not given)
is not proved; with a wrong form of t (2m) neither form is proved; a body that reads the
bound fails W-shape with "reads the bound"; a stub backend that answers unknown leaves the
while program `SAMPLE_CONSISTENT` with "symbolic: the solver did not decide"; a stub that
answers sat without a model proves nothing; `proveWhile = false` keeps phase 3's finite
tier.

### 11.3 I1: iteration counters as state (`LoopCounters.h`)

`addIterationCounters` adds, after the certificate is found, one state variable c_h of
kind `IterationCounter` per cut h, set to 0 on every transition into h from outside the
natural loop of h and to c_h + 1 on every transition into h from inside it. Nothing reads a
counter, so every run performs the same transitions, events and returns; `compareRuns`
runs the original and the instrumented program side by side from an entry and also checks
that each counter equals the number of header arrivals since the loop was entered (the m
of Section 7), recomputed from the transitions taken. The reports print this replay for
every enumerated point (`iteration_counters.replay_equal`); it is evidence for those
points. The certificate stays the one found for the original program, and every checker
re-checks it against the instrumented program as before. A counter wraps at 2^w
iterations like any state.

In the coordinate grammar two atoms with one label kernel are one label; the counter has
the kernel of the loop variable it counts (both are injective per activation), so without
a rule it would be dropped as redundant. The rule added: a counter takes the place of a
term with its kernel. The finite verdict on the label is the same either way (P1); what
changes is which term represents it, and the counter's unit steps are what the symbolic
tier's order needs.

### 11.4 I2 and I3: identities, views, and the certificate

With `--closed-form-views` the domain analysis runs the closed-form synthesis over the
enumerated points and hands the proposer **state identities**, proposals of the form
v = term at every event:

- every `PROVED` loop-free form f_v of a loop whose header has a counter c_h, with m read
  as c_h: v = f_v(c_h), at cut h (a `PROVED` while program gives no term and no identity,
  and the record says so);
- every state variable that every path from the entry defines by one term over
  parameters and constants and that no path between cuts assigns (`entryDefinedState`):
  rotated's `two_n = 2n` and its materialized constants.

The proposer reads an identified variable as its term when it writes guards, bounds
(`fit` also tries an identity's term as a bound base) and views. Views are now checked
against the recorded indices of every sampled event of every site before they are
proposed; where the index term cannot be written over levels, a level whose values equal
that dimension at every event is proposed instead; a view with a dimension nothing fits
is not proposed. This last change is not gated by a flag, since it only replaces a
proposal that the finite checker would have refuted by one it may prove or by none.

Per candidate, the identities and the proposed bounds of every counter level are added
to the certificate's invariants (v = f(c_h) and lower ≤ c_h ≤ upper at cut h, entry
definitions at every cut), and `checkCertificate` is asked about the strengthened
certificate; only if it accepts is the strengthened one handed on, and every checker
re-checks the certificate it receives. This is where a proved closed form becomes usable
by the symbolic domain checker: its D2 needs i = f(c) at the header to know the next
event, and the invariant gives it. The record (`certificate.invariants_added`,
`certificate.note`) names what was added and what the checker said.

The record of each candidate ends with three separate answers (`recognition`):

- `decomposition`: Theorem 13's status and tier;
- `views`: whether all three views (C, A, B) were proposed and proved, over the scope or
  at the points, and `views_affine`: whether every proposed view is affine in the level
  variables (`affineOverLevels`, a syntactic test: sums of constant multiples of leaves);
- `lowering_eligible`: decomposition verified over the scope, every view proved over the
  scope and affine, the order exact, and a reorder fact (trivial, integer or float) proved
  over the scope, which is what Definition 4.5 (V) and Theorem 13 together ask for. It is
  a statement of eligibility; `authorizes_lowering` stays `false` in every record of this
  research build.

### 11.5 Acceptance cases, measured (2026-11-14)

| | `stepped`, 8 bits, n ∈ [8, 40] (I1, I3) | `rotated`, 8 bits, n ∈ [2, 3] (I2) |
| --- | --- | --- |
| baseline (Section 9) | CANDIDATE_FOUND at level 3 with batch axes; symbolic UNKNOWN; domain not run | coordinate tier SCOPE_ENUMERATED; proposer: "no guard from the program's conditions is exact" |
| coordinate search | K = c_inner (the counter), J = j, level 0; the plain `i` no longer appears | unchanged: I = i, J = if t<n then 0 else 1, K = if t<n then t else (t+1−n) mod n |
| coordinate tier | SCOPE_ENUMERATED; symbolic still "no enumeration order": the iteration count of the inner loop is not a parameter plus a shift (it is (n+9) div 10 + (n+2) div 10), which `proposeOrder` cannot write | SCOPE_ENUMERATED (as in phase 1) |
| identities | i = if (c mod 2) = 0 then 5c else 2 + 5c, from the PROVED form | two_n = 2n, and the constants 0, 1, 2 |
| certificate | strengthened with the identity and 0 ≤ c ≤ 8 at cut 3; accepted (O0–O7) | strengthened with the entry definitions; accepted |
| domain | finite and symbolic VERIFIED, order exact | finite and symbolic VERIFIED, order exact |
| Theorem 13 | VERIFIED over the scope | VERIFIED over the scope |
| views | C = (0, J) affine; A = (0, f(K)), B = (f(K), J) proved over the scope, **not affine** | (I, J), (I, K), (K, J): proved over the scope, affine |
| reorder | integer reorder proved | integer reorder proved |
| `lowering_eligible` | **false**: "a view is not affine, which Definition 4.5 (V) requires" | **true** (eligibility, not authorization) |
| without the new flags | proposer: "no guard from the program's conditions is exact" | proposer: the same |

Both columns are the domain report on the whole pipeline (`test/lit/dom_phase4.mlir`,
unit test `testPhase4Views`), not a separate output. The two_phase program of phase 1,
run the same way, is eligible as well, with the coordinate tier at SYMBOLIC_EXACT.

### 11.6 Limits of phase 4

- The while template is the only while shape searched or proved (Section 11.1).
- `PROVED` for a while program is at the width and scope stated, under the specification
  of Section 7, which is checked against the program by replay at the enumerated points
  and constructed from the cut paths; W1 is a hand proof about the template.
- The coordinate tier's symbolic order has no language for a bound such as stepped's
  iteration count; the family stays SCOPE_ENUMERATED there while the domain checker proves
  the domain over the scope through the strengthened certificate. Theorem 13's tier is the
  checker's own joining of the parts.
- A counter's upper bound in the certificate comes from the proposer's samples (8 for
  n ≤ 40); a scope where it is too small makes the certificate checker refuse the
  strengthening, and the given certificate is used, with the refusal recorded.
- An identity from a while program (no term) is not available to the proposer.
- Views are proposed from one site's index terms or from level equality on the samples;
  a view that is neither is "not proposed", never refuted.
- Everything about the external SemGuS format is as in Section 2: no external parser or
  solver has seen anything from this repository.
- The default output of the three reports is unchanged with the new options off, except
  for proposals that were refuted by the finite checker before (Section 11.4): in the lit
  inputs run against the pre-phase-1 binaries, no such line changed.

## 12. Phase 5: arrays (Appendix G.5)

Phase 5 adds arrays to the statement grammar and its semantics as Appendix G.5 defines
them, at bit-vector widths; searches for and proves table programs of one template; puts
the result into the recognition record; and checks the SemGuS-Lib style export against
the candidate. Files: `IndexProgram.h/.cpp` (arrays, `NoRule`, `TableTemplate`),
`IndexProgramSemgus.h/.cpp` (array forms, sorts and rules, constraints, the export
self-check), `IndexSlice.cpp` (TT obligations, entry definitions for inits),
`ClosedFormSynthesis.cpp` (`TableSynthesizer`), `DomainAnalysis.cpp` (index programs on
the views), `test/fuzz/table_oracle.py`. No new CLI flag: `--slices` and
`--closed-form-views` carry the table programs.

### 12.1 Grammar and semantics (G.5), as implemented

- **Declaration.** `IndexProgram::array(width, indexWidth, extents, input)`: a rank
  (the number of extents) and one extent expression per dimension over the inputs and
  constants (a local in an extent is refused). `read(a, indices)` and
  `write(a, indices, value)` take exactly rank indices of the index width, or throw: a
  partially indexed array is no term, a deeper index is not derivable, as G.5 types the
  grammar by the declaration.
- **State.** Every array is a partial map from index tuples to values (`IndexTable`),
  with its extents fixed when the run starts (the extent expressions evaluated at the
  inputs). A non-input array starts with no element; an input array takes the table
  given to the run, which must have the declared extents.
- **Rules.** Read: every index below its extent (unsigned at the index width) and the
  element written, else **no rule** (`IndexRunStatus::NoRule`, with the array, the
  index and the dimension in the reason). Write: value and indices in the state before
  the write, every index below its extent, the one element replaced, nothing else
  changed (frame). Operands are evaluated left to right, so the first failing rule is
  the same in every build (C++ leaves `a + b`'s order open; the thesis's own check stops
  P_L without its boundary loops at dp[0][1], and so does this one).
- **Partiality.** `NoRule` is a definite outcome: the same inputs give the same stop,
  and a candidate that reaches it at a sampled input fails the specification's totality
  (`checkProgramOnTraces` reports `REFUTED` with that sample). `OutOfFuel` stays what it
  was: no value and no claim.
- **Alias.** Arrays of a program are distinct named objects: a write to T0 changes no
  element of T1 (tested). Nothing of the program under analysis is aliased with a
  candidate's table: a table is the candidate's own state. Input arrays fed from the
  program's memory are implemented in the semantics (the a1 case of F.6, tested) but not
  used by the pipeline (Section 12.6).
- **Text.** `array T0[e1]...[er] ; S ; return E`, writes `T0[e] := E`, reads `T0[e]`.

### 12.2 The hand-written reference, kept apart from synthesis

The lattice program P_L of (G.4), rank 2 with extents (M, N), is written by hand in the
unit test `testArrays`, as the thesis wrote it, and run under the rules: for every
1 ≤ M, N ≤ 8 it returns C(M+N−2, M−1) and every element (i, j) holds C(i+j, i); MN
elements have a value; M + N + (M−1)(N−1) writes, (0, 0) written twice; at 32 bits the
element at M = 18, N = 19 is C(35, 17) modulo 2^32, which the integer statement is not.
Without its boundary loops it stops at the read of dp[0][1]; `dp[M][0] := 1` stops at
index M outside [0, M); `dp[0]` on a rank-2 array is refused by the builder. **This is a
check of the hand derivation against the rules, as the thesis's own script was; it is not
a synthesis result, and the record never lists it as one.**

### 12.3 The one table template searched and proved

```
T[0] := k_0 ; ... ; T[d−1] := k_{d−1} ; r := d ;
while r < m + 1 do (T[r] := E(r, T[r−1], ..., T[r−d], inputs) ; r := r + 1) ; return T[m]
```

one array T of rank 1 and extent m + d, d ≥ 1 init writes of expressions over inputs
alone, E over r, the reads T[r−j] for 1 ≤ j ≤ d, the inputs other than m, and constants
(`matchTableTemplate`). **This is not the array grammar of G.5**: a second array, a rank
above one, a write at another index, a nest of loops, a read at another index, a
branching body, a program that returns another element, or the lattice P_L itself is
neither searched for nor proved; `TT-shape` refuses it and the record says so.

**Search** (`TableSynthesizer`, from the sampled traces, never from a written answer): for
d = 1, 2, 3 in turn, the inits are the values at m < d when they are one constant across
the seeds (or the symbolic entry at m = 0); the *history chain* is the carried variables
that trail the variable by 1, 2, ... iterations on the seeds; the body E is a loop-free
synthesis problem over r, the reads and the inputs with the goal-directed enumerator of
Section 7 on the steps m ≥ d; the program is built, matched against the template, run on
the seeds, and then judged on every trace and put to the obligations as in Section 11
(CEGIS at the finite tier, replay of every solver model). A table is tried when nothing is
`PROVED` yet; a `PROVED` table replaces anything, a merely sample-consistent one does
not replace a sample-consistent while program.

**Obligations** (`checkClosedForms`, width w, scope, and m < 2^w − d so that the extent
does not wrap):

```
TT-shape    structural: template matched, m the iteration leaf, inputs leaves of the
            specification, the chain v_0 = v, v_1, ..., v_{d−1} proposed or found
TT-init_j   scope ∧ c(F(0)) ∧ ... ∧ c(F(j−1)) ∧ v(j) ≠ k_j      (F unrolled from the entry)
TT-step     scope ∧ m ≠ 2^w−1 ∧ m ≥ d−1 ∧ c(F(m)) ∧ E[r := m+1, T[r−j] := v_{j−1}] ≠ τ_v(F(m))
TT-shift_j  scope ∧ m ≠ 2^w−1 ∧ c(F(m)) ∧ τ_{v_j}(F(m)) ≠ v_{j−1}
TT-bounds   d ≤ r < m+1 ∧ ¬(r < m+d ∧ r−j < r for j = 1..d)
TT-term     r < m+1 ∧ ¬((m+1 − (r+1)) <u (m+1 − r))
```

*Lemma T1 (coupling).* After the run on m the table holds T[j] for 0 ≤ j ≤ m, T[j] = k_j
for j < d and T[j] = E(j, T[j−1], ..., T[j−d]) for d ≤ j ≤ m: the init writes set the
first d, the loop runs r = d, ..., m once each (TT-term: the variant m+1−r), writes T[r]
within the extent m + d and reads only elements written before (TT-bounds), and S has no
other effect; the run on m+1 differs only by one more iteration, so its table agrees with
the run on m up to index m. ∎

*Soundness of PROVED.* Claim: T[m] = v(m) at every arrival m < 2^w − d in the scope, with
T[m−j] = v_j(m) for j < d (the chain). Base, m < d: TT-init_m gives v(m) = k_m = T[m]
when the loop reached m (the premise), and the chain at m is the earlier inits.
Step, m ≥ d−1 to m+1: the specification gives v(m+1) = τ_v(state(m)); by T1, T[m+1] =
E(m+1, T[m], ..., T[m+1−d]) = E(m+1, v_0(m), ..., v_{d−1}(m)) by the chain at m; TT-step
at state(m) gives that this is τ_v(state(m)). The chain at m+1: v_j(m+1) = τ_{v_j}(state(m))
= v_{j−1}(m) = T[m+1−j] by TT-shift_j. ∎ The chain variables are free leaves in TT-step
and TT-shift (the actual values are one instance), and a model over them is `Unknown` with
the model, replayed before anything is called refuted (Section 11.2).

### 12.4 Two changes the proofs needed

- **Inits read through the entry definitions.** The entry value of a nested loop's
  variable is often `v := c0` with c0 a materialized constant; phase 2 saw a state
  variable and left the init symbolic. Phase 5 substitutes the entry definitions of
  every state variable (Section 11.4's `entryDefinedState` rule) into the init terms,
  so f := c0 is the constant 0. Without it, TT-init_1 for the history program has a
  model at an entry value no run has (entry_f = 4), and its replay refuted the correct
  candidate. The gemm k form of Section 7 is `m` now, not `m + entry_k`.
- **Model replay takes an entry value only where the entry is symbolic.** With a known
  init the entry leaf is an unconstrained auxiliary of the script; a replay from the
  model's value for it started the specification in a state no run reaches and left
  a wrong trace among the samples (found by the oracle: the correct fib table was
  refuted at m = 1 by such a trace).

### 12.5 Results

**`history`** (8 bits, n ∈ [2, 9]): C[0][j] += A[0][f]·B[f][j] over k < n with f, g
carried, f' = f + g + 1, g' = f; f = 0, 1, 2, 4, 7, 12, 20, 33, 54. The slice of the
subscript carries {k, f, g}.

| Variable | Result | Status |
| --- | --- | --- |
| k | `m` (loop-free) | PROVED |
| f | `array T0[(m + 2)] ; T0[0] := 0 ; T0[1] := 1 ; v0 := 2 ; while v0 < (m + 1) do (T0[v0] := (1 + (T0[(v0 - 1)] + T0[(v0 - 2)])) ; v0 := (v0 + 1)) ; return T0[m]`, chain g | **PROVED**: TT-shape, TT-init0, TT-init1, TT-step, TT-shift1, TT-bounds, TT-term unsat; for m < 2^8 − 2 |
| g | the same table with inits 0, 0 | SAMPLE_CONSISTENT (104 sampled iterations): "no carried variable trails the chain at depth 1" |

The program for f was found by the search (3 rounds, 2 counterexample traces), not
written; it coincides with the hand-written template of `testArrays`, which is a check
on the search and not the proof. Phases 2 to 4 leave f and g `UNKNOWN`: no loop-free
form, and a while body over f alone cannot read g (it has no proved form).

**Whole pipeline** (`test/lit/dom_tables.mlir`, the domain report with
`--closed-form-views`): coordinates J = j, K = k at SCOPE_ENUMERATED (the symbolic
coordinate tier stops at V1f, the dependence check); domain finite and symbolic VERIFIED,
Theorem 13 VERIFIED over the scope; view C = (0, J) proposed, proved, affine; views A and
B have the dimension f, which no term describes: the record carries the PROVED table
program on that dimension (`views.A.index_programs`), `views_affine` is null,
`lowering_eligible` is false with the reason that a view is a statement program with a
table, and `authorizes_lowering` is false.

**Oracle** (`test/fuzz/table_oracle.py`): a family of reductions with
f(m+1) = a·f(m) + b·f(m−1) + c and given f(0), f(1), written as MLIR by the script,
run through the driver with `--slices`; the script recomputes the subscript sequence at
every point of the scope by its own arithmetic and runs every reported while or table
program with its own parser of the statement text and its own interpreter of the G.5
rules. `PROVED` and `SAMPLE_CONSISTENT` programs must agree at every m of every point,
`REFUTED` ones must differ at their model, `UNKNOWN` requires nothing, and a table must be
of the claimed template. Two seeds, 36 cases, 0 problems; 7 programs PROVED (while and
table), the overfitted while bodies (e.g. `if 1 < v1 then (5·v1) div 3 else ...`) all
`SAMPLE_CONSISTENT`, never `PROVED`.

### 12.6 Not done, by name

- **The general G.5 grammar is not searched.** Only the history table template
  (Section 12.3). Rank 2 or 3, several arrays, nests, writes at computed indices, reads at
  other indices, branching bodies: representable, runnable, exportable, not synthesized
  and not proved. P_L is a reference only.
- **Input tables do not reach the pipeline.** A program whose subscript is read from a
  table (`a[0][tab[k]]`) has an address that depends on memory, and the model's
  obliviousness gate (invariant I8, "the trace depends on memory contents") closes the
  enumeration for it; the a1–a3 cases of F.6 are therefore in the semantics and the unit
  tests only.
- **Element-wise table specifications** (G.3 over the exposed final state) are not posed:
  the specification here is the slice's (F.2) for one variable, and the table is the
  candidate's device for its history; the exposed final state is checked at the finite
  tier (every element of P_L) and by Lemma T1 symbolically through T[m].
- **The coordinate tier's symbolic order** for `history` stops at V1f; the domain checker
  verifies the domain over the scope, as for `stepped`.
- **SemGuS-Lib.** The export now carries nested array sorts, `$read_T`/`$write_T`
  constructors, the G.5 read and write rules with the written-flag arrays and the bound
  premises, `declare-var` and the inductive constraints of G.3 rendered from the slice
  (`semgusTermText`, shadows read as false); `semgusExportMissing` checks that every
  constructor a candidate uses has a production and a rule in the text, and
  `semgusExportWellFormed` the parentheses and commands. **No external SemGuS parser or
  solver has read any of it** (none is installed: no ks2, cvc5, z3 CLI or python z3);
  the header of every export says so, and nothing here is an external integration.
- **Widths.** Everything is at the program's widths; `PROVED` for a table is for
  m < 2^w − d; the thesis's integer statements are not what is proved.

## 13. The inductive invariant path for a scalar loop

Before this section the architecture had no verification path for the value a scalar
loop computes. The synthesis report refused such a function ("the program has no MAC
site"); the slice, the loop specification and every obligation above start from a MAC
subscript. Where the existing parts work, the record is:

| Concern | Where | What it does |
| --- | --- | --- |
| Enumerating candidate expressions | `enumerateGrammar` (`ClosedFormSynthesis`), the `Synthesizer`, `WhileSynthesizer`, `TableSynthesizer`; `CoordinateGrammar` for coordinates | bottom-up by size, one term per value vector on the samples |
| Enumerating inputs and states | the report's scope enumeration (`FiniteDomain`), `replayLoop`, `runConcrete`; here `loopPoints` and `sampleLoop` | concrete runs at parameter points |
| Unrolling a loop iteration by iteration | `runConcrete` and every concrete replay; `runIndexProgram` with its fuel (`checkProgramOnTraces`, the finite tier of while candidates); `TT-init_j` unrolls d ≤ 3 steps symbolically | executed, or symbolically unrolled, per iteration |
| Symbolic verification | `checkClosedForms` (CF, W, TT), `checkCertificate` (G2a O0–O7), the symbolic coordinate and domain tiers; here `checkLoopInvariant` | SMT-LIB text to `SmtBackend` (Z3 as a library), unsat is the only proof |

The new path reuses all four and adds one checker and one search; it builds no
separate SyGuS pipeline.

### 13.1 What is proved, from what

`invariantLoop` (`LoopInvariant.h`) reads the function's cut paths (`CutPaths.h`).
Stage 1: integer parameters and results, no memory root, no float, exactly one cut.
Paths split into entry → header (Pre), header → header (G and T, one per path),
header → return (X and the returned values, composed with `extendSymbolicPath`), and
entry → return without the header (a bypass). The terms are the program's own: the
operations of each block in their order (so `x++` reads x before the update), the
counter's update on the back edge, the guards, the no-stop conditions of every block
passed, and a variable no path assigns is unchanged (the frame; the specification
JSON lists it as `s' = s`).

For a function candidate F (one term over the parameters per returned value) and an
invariant Inv (a conjunction over the header variables, their poison bits and the
parameters), `checkLoopInvariant` asks, at the program's widths and under the scope:

```
INV-init    scope ∧ Pre(s) ∧ ¬Inv(s)                     per entry path
INV-step    scope ∧ Inv(s) ∧ G(s) ∧ T(s,s') ∧ ¬Inv(s')     per iterate path (T by substitution)
INV-exit    scope ∧ Inv(s) ∧ X(s) ∧ ¬Post(s)               per leaving path
RET-bypass  scope ∧ X(s) ∧ ¬Post(s)                        per bypass path
```

All unsat is `PROVED` partial correctness: a run in the scope that returns, returns F.
X is the leaving path's condition, not ¬G: ¬G also holds where a run is stuck on a
side condition, which returns nothing.

Two devices keep the queries decidable in practice, both sound and recorded:

- **Elimination.** An atom v = t (or v ± u = t, u − v = t; v a header variable not in
  t) is a premise wherever Inv is, so the query is stated with v replaced by t. The
  substituted query is equisatisfiable with the literal one. Applied only where Inv
  is a premise (INV-step, INV-exit, INV-poison from the header).
- **INV-exit-eq.** For each comparison in X, `scope ∧ Inv ∧ X ∧ a ≠ b` is asked first;
  an unsat makes a = b a lemma, solved the same way. For the request's loop this is
  i = N.

Measured on Z3 4.8.12, before this machine's system upgrade of 2026-11-17, the literal
exit query of the request's loop at 32 bits timed out at 60 s and the eliminated one
took 9 ms (the scratch logs of that measurement were lost with a reboot; it is not
re-run here). With Z3 4.13.3 the search's proof of that loop is 5 queries (13.6).

### 13.2 What a model means

- INV-init sat: an initial state violates Inv. The invariant candidate is wrong.
- INV-step sat: Inv is not inductive; the model's state need not be reachable. The
  invariant candidate fails; the function candidate is untouched.
- INV-exit sat: the model may sit where no run goes (the invariant may be too weak).
  It refutes F only through a run: `runConcrete` from the model's parameters must
  return another value. When that run cannot decide (fuel, a stop, the same value),
  the query is asked again with every parameter in [−8, 8], then [−64, 64], then one
  parameter at a time in those ranges, and each model is run. No run that disagrees
  → F stays `UNKNOWN`, with the model kept as `unconfirmed_exit_model`.
- Unknown, a timeout, a resource limit: nothing. An undecided conjunction is split atom
  by atom (each atom alone unsat proves the conjunction).

### 13.3 The search and the CEGIS loop

`proveLoopFunction` (`InvariantSynthesis.h`) proposes; every verdict above is the
checker's or a run's.

1. Samples: complete runs at small parameter points of the scope (`loopPoints`), the
   header state at each arrival. They choose candidates only; no obligation reads one.
2. A sampled run that returns another value refutes F at once (a run of the program).
3. The template, by levels (degree, size) = (1, 5), (2, 3), (2, 5): equalities between
   sums of monomials of degree ≤ d over the header variables and parameters (the
   closed-form enumerator over monomial leaves, + and − and constant ×, constants 0 to 8
   and the program's literals, `size` counting leaves and operators), offered when two
   terms agree at every sampled state and differ at a random witness state, and only
   when linearly independent of those already offered; comparisons ≤s <s ≤u <u between
   two variables or a variable and 0, and the loop's own comparisons and their
   non-strict forms; not-poison per variable. A conjunction only: no disjunction, no
   quantifier, no division, at most 400 equalities and 200 000 terms.
4. Houdini: check; drop the atoms a model of INV-init or INV-step falsifies (and those
   left undecided); repeat. The result is the strongest inductive conjunction of the
   offered atoms.
5. INV-exit unsat: proved; the certificate is minimized (each atom dropped if the rest
   still proves, and still excludes poison where it did) and checked once more. A
   confirmed run: F refuted. Otherwise the next level; after the last, `UNKNOWN`, which
   says nothing about invariants outside the template.
   Since M1 (§13.9) the null-space levels (degree 1, then 2) run first and these
   term-collision levels after them; `--loop-invariant-candidates` selects one source.
6. Without a given F, the closed-form grammar over the parameters and their monomials
   (size ≤ 5) proposes one from the sampled results; a refuting run's parameters join
   the samples and the next F is sought (up to 4 rounds).

### 13.4 Semantics, poison and termination

Bit vectors at the program's widths; values wrap; nsw, nuw and exact set poison bits
and a branch on poison is a stop (`SemanticTerm.h`, I2). Post compares bits, so
`PROVED` says the returned bits equal F's modulo 2^w; no statement over the integers is
made. For the C loop with `int` (nsw on every operation) the bits are proved; whether a
returned value can be poison is the informational INV-poison query, and the claim says
"not shown free of poison" when it is not excluded (for nsw it is not: g can overflow).

An invariant proves partial correctness only. The report joins it with the G2a verdict:
`PROVEN_TERMINATING` gives total correctness over the scope;
`PROVEN_TERMINATING_IF_DEFINED` gives "returns F or stops on a side condition"; anything
else leaves partial correctness only.

### 13.5 Fallback and labels

When nothing is proved and nothing refuted, the finite tier runs the program at the
scope's points, every iteration executed (`checkLoopFunctionOnRuns`): `SCOPE_ENUMERATED`
when every point of a finite scope ran and agreed (a complete bounded check of that
scope), `SAMPLE_CONSISTENT` otherwise (tests only), `REFUTED` with the run. Only
`PROVED_BY_INVARIANT` reports `"unrolling_omitted": true`. The proof's own queries do not
depend on N: the same certificate checked for N ≤ 0, N ≤ 10 and N ≤ 2^31 − 1 gives the
same obligations, and `proof.program_runs` is 0. The SMT solver's internal search is not
claimed to be absent; only the program's loop is not unrolled.

CLI: `--loop-invariants` on the synthesis report, with `--loop-function=j:term`,
`--loop-invariant=conjunction` (check instead of search), `--loop-invariant-scripts`,
`--loop-invariant-levels`, and since M1 `--loop-invariant-degrees=d[,d...]` and
`--loop-invariant-candidates=both|nullspace|collisions`. Without `--loop-invariants` the
output is unchanged.

### 13.6 Results (2026-11-17, g++ 15.2, CMake 4.2.3, Z3 4.13.3)

The request's loop, `for (int i = 0; i < N; i++) g += 2 * x++ + 1;`, candidate
`p2 + (p1 + p0)·(p1 + p0) − p1·p1`, scope N ∈ [0, 2^31 − 1], i32:

| | Result |
| --- | --- |
| request's certificate, given | INV-init, INV-step, INV-exit-eq (i = N), INV-exit unsat; 5 queries, 0 program runs |
| search | `PROVED_BY_INVARIANT` in about 5 s: `s0 ≤u p0`, `p1 = s1 − s0`, `s2 + p1·p1 = p2 + s1·s1`, `¬sp1`, `¬sp2` (minimized from 15 atoms); proof 5 queries, 0 runs; samples 48 runs, at most 13 header arrivals each |
| termination (G2a) | `PROVEN_TERMINATING`, so the claim is total correctness over the scope at width 32 |
| nsw (C int) | bits proved; poison not excluded, and the claim says so |
| wrong candidate (no −x0²) | `REFUTED` by a sampled run; with the right certificate, by INV-exit and a run from its model |
| candidate wrong only at x0 = 1000 | no sampled run sees it; INV-exit's model, re-asked with N small, gives a run that refutes it |
| weak certificate (no bounds on i) | inductive, INV-exit models no run reaches; not refuted, `SAMPLE_CONSISTENT` |

Validation of this version: unit section `invariants` 109 checks, 0 failures (101 s,
608 MB peak); the sections that use the shared enumerator (`index statements slices
while counters arrays tables`) 6460 checks, 0 failures; every existing lit RUN line of
the synthesis, computation and domain reports byte-identical to d974400 built with the
same tools (51 of 51); lit `synth_invariants.mlir` passes; `invariant_oracle.py` (ctest
`generalized-gemm-invariant-programs`) 28 cases, 0 problems: 17 of 17 wrong candidates
refuted by runs the oracle replays, 4 proofs confirmed over every input and, at width 3,
2 certificates by brute force, 2 refused certificates' violations found by brute force.
Sanitizer (Debug, `-fsanitize=address,undefined -fno-sanitize-recover=undefined`): the
`invariants` section 109 checks, 0 failures, no ASan or UBSan report (40 min under a
machine load near 11, 1.9 GB peak).
Mutation (`synthesis_mutation.py`, Debug, the `invariants` section as judge): 10 of 10
new mutants killed by failing checks. In the first pass three of them ended in the 900 s
command timeout under a machine load near 13; a timeout is not counted as a kill here,
and those three were run again with `--timeout 3600` at low load, each killed by failing
checks.

### 13.7 Limits and what stays UNKNOWN

- One loop, scalar integers only: memory, floats, nested or consecutive loops are
  `UNSUPPORTED` with the reason; nothing is claimed.
- The template is a conjunction of polynomial equalities of degree ≤ 2 and simple
  comparisons. An invariant that needs a disjunction, a quantifier, division, a
  bit operation, or a relation outside the levels leaves `UNKNOWN` (the INV-exit model
  is kept). At widths 3 and 4 the oracle's catalogue has 4 right candidates of 8 left
  unproved this way: relations that hold only modulo 8 or 16 crowd the offered
  equalities; the same loop at 32 bits is proved (`g − g0 = i + 2·i·x0`).
- Without a given candidate the closed-form grammar (size ≤ 5 over the parameters and
  their monomials) proposes one; the request's own candidate is larger and is not found
  that way, so it is given.
- A solver unknown, a timeout or a resource limit on any obligation leaves `UNKNOWN`; an
  INV-exit model no run confirms leaves `UNKNOWN`, never `REFUTED`.
- Partial correctness only, unless G2a's termination verdict is joined; poison returns
  are excluded only when INV-poison says so.
- Not run on this version: the earlier synthesis, computation and domain mutation
  catalogues and the other unit sections, which this change does not touch.

### 13.8 M0: a fixed baseline with public benchmarks (2026-11-17)

Scripts, programs and results live outside the repository, in
`~/work/semgus-invariant-validation/coverage` (`catalogue.py`, `nla.py`,
`m0-baseline.jsonl`, `m1.jsonl`, `m0-vs-m1.txt`, `degree3.txt`); the baseline binary is
a8d8910 built with the same tools (`baseline-m0/`).

The catalogue holds the 17 earlier loops plus 8 programs translated from SV-COMP
`c/nla-digbench` (sosy-lab/sv-benchmarks, `master`): ps2, ps3, cohencu, mannadiv,
prodbin, sqrt1, geo1, divbin. The translation is by hand and follows fixed rules: C `int`
becomes i32 with wrapping operations (no nsw), each nondeterministic input becomes a
parameter, each `assume` becomes the `--scope`, and the function returns the variable
the benchmark's postcondition names. The SV-COMP property (a reachability assertion
over mathematical integers) is therefore not what is checked here; what is checked is
that the returned bits equal a formula modulo 2^32.

Each loop runs three ways: A, with a formula written by hand (the formula is an input,
not a result); B, with nothing given (the closed-form grammar proposes the formula, the
search proposes the invariant); C, with wrong formulas. They are counted apart, and a
proof under A is never counted as automatic synthesis:

| Bucket | M0 (a8d8910) | M1 |
| --- | --- | --- |
| formula and invariant both found (B `PROVED_BY_INVARIANT`) | 8: step_const, step_param, step_precomputed, countdown, unsigned_guard, sum_of_i, branch_same, nla_prodbin | the same 8 |
| formula given by hand, invariant found (A proved, B not) | 2: odd_sum, nsw_odd_sum | 3: odd_sum, nsw_odd_sum, **two_vars** |
| neither: UNKNOWN, SAMPLE_CONSISTENT (tests only) or UNSUPPORTED | 15 | 14 |
| wrong formulas (C) refuted by a confirmed run | 24 of 24 | 24 of 24 |

### 13.9 M1: null-space equality candidates

`nullSpaceEqualities` (`InvariantSynthesis.h`) reads each sampled header state as a row:
the values of the monomials of degree ≤ d over the header variables and the parameters,
each value read as a signed integer and multiplied exactly (`llvm::DynamicAPInt`). A
variable that holds one constant at every sample is folded: it gets the atom `v == c`
and leaves the matrix. Fraction-free Gauss–Jordan elimination over Q gives a basis of
the null space, one vector per free column, scaled to coprime integers; each becomes the
bit-vector atom `(positive part) == (negative part)` at the variables' width, with the
free column last so that the checker's elimination can solve for it. A vector with a
coefficient of magnitude ≥ 2^(w−1) is skipped and counted. At most 128 columns.

The same samples answer differently over Z/2^w: a relation over Q read modulo 2^w still
holds at the samples, but is only a candidate. Nothing about the null space is used as
evidence. The atoms enter Houdini like any other, and the checker's INV-init,
INV-step, INV-exit, RET-bypass and poison obligations are unchanged. An INV-init model
is an initial state the samples missed: its parameters (re-asked with every parameter
in [−64, 64] when the model's are large, so that the run does not wrap) become a new
sampled run, the falsified atoms are excluded for good, and the level is offered again
(at most 8 times; Sharma et al., Theorem 3). Nothing here covers every invariant modulo
2^w: one with an even coefficient, or one that holds only because values wrap, is
outside it.

Measured (Release, Z3 4.13.3):

- two_vars (`x += 2; y += x`), which neither level of the old template proved: degree 2,
  28 columns, 254 rows, rank 20, basis 8, 2 folded; proved with 27 atoms in about 5 s.
- Degree 3, with the formula given, against degrees 1 and 2 (`degree3.txt`): two_vars
  4.7 s either way; cubic 0.3 s → 15 s; nla_ps3 0.2 s → 18 s; nla_cohencu 57 s, 375 MB
  → 495 s, 2.3 GB, 56 columns, basis 34. None of the three is proved at degree 3, so
  degree 3 is not in the default; `--loop-invariant-degrees=1,2,3` asks for it.
- A regression the catalogue found and the unit section now checks: capping each
  refinement run at 64 states first skipped a run altogether once the 512-state budget
  was full, and the function CEGIS then refit without the refuting run's return
  (unsigned_guard, mode B, ended `REFUTED` on a wrong synthesized formula). Every added
  point is now run; only its states are capped.

What stops the rest, case by case (M1):

- Formula search (B: "no term of the grammar returns result 0 at every sampled run"):
  init_param, stride3, odd_sum, nsw_odd_sum, two_vars, scaled_sum, cubic, branch_param,
  branch_min, doubling, nla_ps3, nla_cohencu, nla_mannadiv, nla_sqrt1, nla_geo1. The
  closed-form grammar (size ≤ 5) has no such term; M1 does not change it.
- Invariant search, formula given (A: an INV-exit model no run confirms, so the
  invariant found is too weak): init_param, stride3, branch_min, nla_ps3 (needs degree 3
  and a division by 6), nla_cohencu (degree 3 offered 34 equalities and still did not
  prove; not analyzed further), nla_mannadiv (needs a quotient and remainder relation).
- Solver: nla_ps2, whose invariant `2·s2 = s0² + s0` is offered at degree 2, but
  INV-step is not decided (rlimit 20M: canceled; without the limit, 120 s per query: a
  timeout). The even coefficient means it cannot be eliminated. branch_param: INV-exit
  canceled.
- Formula: scaled_sum's hand-written formula is wrong modulo 2^32 and stays `REFUTED`
  by a run.
- Shape: nla_divbin has two loops, `UNSUPPORTED`.

Validation: unit sections `invariants` and `nullspace` 144 checks, 0 failures (129 s,
846 MB); lit `synth_invariants.mlir` passes; `invariant_oracle.py`, extended with
two-variable loops and formula-free cases, 40 cases, 0 problems, on both a8d8910 and M1.
ctest `generalized-gemm-invariant-programs` passes. Mutation (Debug, `--unit-only`): 4 of 4
new mutants (constant column dropped, folded atom dropped, sign of a basis entry lost,
INV-init models never run) killed by failing checks in the `nullspace` section, and the
two search mutants whose code M1 touched (Houdini keeps falsified atoms, sampled runs
refute nothing) killed again by the `invariants` section. Sanitizer (Debug,
ASan+UBSan): `nullspace invariants` 142 checks, 0 failures, no report (80 min, 2.1 GB)
before the regression fix; `nullspace` again after it, 35 checks, 0 failures, no
report (33 min, 1.9 GB). Not rerun: the other unit sections and the earlier mutation
catalogues, which this change does not touch.

## 14. Index invariants for a GEMM's subscripts

Section 13 proves a function of a scalar loop. This section uses the same kind of proof
inside a GEMM: for the integer state that the subscripts of a MAC site read, at every
loop header of the nest. It verifies how loop counters and index variables evolve, how
candidate coordinates I, J, K relate to the subscripts as written, and (through the
checker that already asked for it) that the address of A depends on (I, K) alone, of B on
(K, J), of C on (I, J). It does not prove anything about the cells' values, adds no float
reorder authorization, and leaves computation, domain, aliasing, bounds, poison and
lowering to the checkers that own them.

The checker of Section 13 (`checkLoopInvariant`) could not be used as it was: it takes a
function with no memory root, no float and exactly one cut, and its conclusion is a
returned value. A GEMM has three roots, a cut per loop header, and no result; what must
be concluded is a fact at the block where a subscript is used. So the milestone adds the
minimal counterpart, `checkIndexProof`: any number of cuts (a path that enters an inner
loop from an outer header is one more obligation), programs with memory and floats (the
atoms read integer state only), `scf.for` `iter_args` as the header arguments the
structured lowering makes of them, and IDX-use in place of INV-exit. It shares the
obligation formula (`invariantViolation`), the term text and the null-space generator
with Section 13; `checkLoopInvariant` itself is unchanged.

Files: `IndexInvariant.h/.cpp` (tier-checker library: the checker, the certificate
strengthening, the sampler), `IndexInvariantSynthesis.h/.cpp` (synthesis library: the
search, the relation proposer, the identities, the record), a truncation option on
`nullSpaceEqualities` (`InvariantSynthesis`), `referenceEntryState` exported from
`FiniteDomain`, `indexIdentities` on `DomainAnalysisRequest`. CLI, on the synthesis and
the domain report: `--index-invariants` (search), `--index-invariant=cut:conjunction;...`
(a given certificate: a debugging comparison); on the synthesis report also
`--index-relation=site:A|B|C:dimension:term;...`, `--index-invariant-degrees`,
`--index-invariant-samples`, `--index-invariant-scripts`. Without them no output changes.

### 14.1 The gap, measured before the change (23cac4c)

The acceptance case is a GEMM written with `scf.for` whose K subscript is a scalar
recurrence carried by `iter_args`:

```
for i in [0, n): for j in [0, 3):
  x = x0
  for k in [0, kk):   C[i][j] += A[i][x] * B[x][j];   x = x + 1
```

index width 8, scope n in [1, 3], kk in [1, 4], x0 in [0, 5]. After the structured
lowering x and k are the two arguments of the k header (state s6 and s5; x0 is p2).

| Report | Before |
| --- | --- |
| synthesis: finite tier | `SCOPE_ENUMERATED` with I = i, J = j, K = k |
| synthesis: symbolic tier | `PROPOSED`, `UNKNOWN`: **"V1f dependence: sat"** |
| domain (roots authorized): access views | C proved; A and B "nothing proposed" |

V1f is `checkFamilySymbolic`'s obligation that two events with equal (P, I, K) have the
same A address. It has a model because the termination certificate's invariant at the k
header says nothing that relates x to k, so in the over-approximated domain one k meets
any x. The model is no counterexample: the finite tier holds at every point of the scope.

Phase 4's closed forms (Section 11.4) reach this program only in the domain report and
only through a ghost counter: with `--iteration-counters --closed-form-views` the forms
k = c and x = c + p2 are proved and conjoined to the certificate, and the coordinate tier
becomes `SYMBOLIC_EXACT`; no view of A or B is proposed (the identity reads the counter,
which is no level of the family), and the route needs an innermost loop, an initial
value over parameters alone and the instrumentation. An invariant states a relation
between the program's own variables, at every header, with the loop's entry as one more
path to prove.

### 14.2 Where it connects

**The scalar state, transitions, guards and entry conditions** come from the cut paths of
the termination certificate's cut set (`enumerateCutPaths`), the same paths G2a's O3 and
O4 use: a path from the entry into the outermost header, a path from each header into
the header nested in it (what the inner loop may assume on entry), the back edges, and
the exits back to an enclosing header. Each has the program's own condition and
simultaneous update, with the frame. The candidates' variables are every integer state
variable of at most 64 bits that no value read from memory reaches and that has a value
at every sampled arrival at the cut. They are not the slice of the subscript (Section 7):
the relation wanted is x = x0 + k, and k is not in x's slice (no assignment to x reads k,
and the guard that reads k is on a transition that does not assign x).

**The steps that need the relations** are, in `checkFamilySymbolic`: V1h, V1f and V1g (the
two-event functional dependences), V2, V3 and the coverage simulation S1 to S3, all of
which take their premises from the certificate's invariants at the cuts
(`SymbolicDomain.h`); in the domain checker, the access views; and inside
`checkCertificate` itself, O7, where a stop (a subscript outside its extent, a poison
subscript) may become dischargeable.

**How a proof is handed on.** Never as a flag. The proved atoms are conjoined to a copy
of the termination certificate (`strengthenCertificate`), `checkCertificate` is asked
about the copy, and only an accepted copy is handed to `synthesizeProgram` or
`analyzeDomains`. Every checker downstream runs `checkCertificate` again on the
certificate it is handed (`enumerateDomain`, `checkFamilySymbolic`, `checkReduction`), so
initiation and consecution are asked again by code that links nothing of the search. An
atom that only held on samples, or was given by hand and is not inductive, fails there:
the unit test conjoins three such atoms into a certificate by hand, and
`checkFamilySymbolic` answers "the certificate was not accepted".

### 14.3 The obligations

`checkIndexProof` (`IndexInvariant.h`). The certificate is untrusted: per cut c a
conjunction Inv_c of atoms over the integer state, its poison bits and the parameters.

```
IDX-init  scope ∧ cond(π) ∧ ¬Inv_b(post π)                    per path entry → cut b
IDX-step  scope ∧ Inv_a ∧ cond(π) ∧ ¬Inv_b(post π)            per path cut a → cut b
IDX-use   scope ∧ Inv_a ∧ premise(ρ) ∧ index(end ρ) ≠ R(end ρ)  per arrival ρ at a site
```

- IDX-init and IDX-step are initiation and preservation. The cut set is checked
  (`cutSetBreaksCycles`), every atom is adopted (`adoptProofTerm`: existing integer
  leaves only, no memory, no float) and must be a condition. The conjunction is asked
  first; when the solver does not decide it, one query per atom.
- IDX-use is the conclusion where a subscript is used. ρ ranges over the arrivals at the
  site's block from the entry or a cut (`reachSites`); its premise holds the guards and
  the no-stop conditions up to and including the site's block, since a block that stops
  contributes no event. R is an index relation: a term over the state at the entry of
  the site's block and the parameters. It is asked only when IDX-init and IDX-step are
  all unsat. With a family the record also says whether R reads state only through the
  coordinates of the operand's axes (P, I, K for A; P, K, J for B; P, I, J for C): a
  structural label, which is what makes the proved equality say "the address is this
  function of those coordinates".
- The two-event obligation is not duplicated. It is V1h, V1f, V1g of
  `checkFamilySymbolic`: for two arrivals with their premises, each under its own copy of
  the state and one copy of the parameters, equal keys and different addresses must be
  unsat. That checker is the consumer, and the record prints its three results with the
  strengthened certificate and with the certificate as it was.

Every term is a bit vector at the program's widths and values wrap; `nsw`, `nuw` and
`exact` set poison bits (I2), a poison subscript is a stop, and a stopped block has no
event. So an equality that survives is true of the bits through any wrap, and whether a
variable can be poison is its own atom.

**What a model means.** A model of IDX-init or IDX-step refutes the invariant candidate
and nothing else: the state of an IDX-step model need not be reachable. A model of
IDX-use refutes the relation only when the run of the program from the model's
parameters returns and has an event of the site whose recorded subscript differs from
the relation's value at the entry of the event's block; otherwise the relation is
`UNKNOWN` with the model kept as text. Neither is ever a statement that the program is
wrong. Unknown, a timeout, a resource limit and the checker's query limit prove and
refute nothing.

**What is not claimed.** A functional dependence is not domain coverage, not unique
coverage and not a GEMM: those remain the coverage simulation's, the domain checker's
and Theorem 13's.

### 14.4 The search, and the relations it proposes

`findIndexInvariants` (`IndexInvariantSynthesis.h`) proposes; the verdicts are the
checker's.

1. Samples: complete runs at a few parameter points of the scope (`loopPoints`, 24 by
   default) with the reference memory; at each arrival at a cut, the state
   (`sampleCutStates`). They choose candidates only.
2. Candidates per cut, by degree (1 by default): `v == c` for a variable constant at the
   samples; a basis of the null space over Q of the monomials over the remaining state
   and the parameters (M1, Section 13.9); `¬poison(v)` for a variable some transition can
   poison. With `truncations` (on), the leaves of a narrower width also read the wider
   state and parameters truncated to it, so that a recurrence carried at 8 bits beside a
   16-bit counter has the candidate x8 == x0 + trunc8(k), which is true through a wrap.
   After a level, a variable that a kept equality determines leaves the next level's
   monomials.
3. Houdini over all cuts at once. The termination certificate's own invariant is the
   first atom of each cut and is never dropped (a model that falsifies it ends the search
   as `UNKNOWN`). The atoms a model falsifies at the end of its path are dropped. An
   IDX-init model is an initial state: its parameters become one more sampled run and the
   candidates are derived again, without the atoms falsified at initial states (at most 8
   times). An atom the solver does not decide is set aside.
4. The surviving atoms are conjoined to a copy of the certificate, which
   `checkCertificate` must accept (Section 14.2).

`proposeIndexRelations`: per subscript, the term with every variable that no coordinate
of the operand's axes reads replaced by its solution from a proved equality of the cut
the site is reached from (an equality is solved for a variable whose coefficient is odd,
a unit modulo 2^w), under casts as well. A proposal: IDX-use decides. `indexIdentities`
gives the domain proposer the same solutions as `v = term`, from which it writes views
over the levels; the domain checker proves each view under the certificate it is handed.

A given certificate (`--index-invariant`) is checked with no search and no sample and
is reported with `"automatic": false`. It is a debugging comparison and is never counted
as found.

### 14.5 Results (2026-11-18, g++ 15.2, CMake 4.2.3, Z3 4.13.3, Release)

The acceptance case, searched:

| | Result |
| --- | --- |
| invariant | `s6 == p2 + s5` (x == x0 + k) at the k header, the only atom beyond the certificate's |
| proof | IDX-init 1, IDX-step 5, all unsat: 6 queries, 0 program runs |
| candidate search | 24 sampled runs (1410 steps), 1 candidate offered, 1 kept, 0 refinements |
| certificate | strengthened; accepted by `checkCertificate` (O0–O7), verdict `PROVEN_TERMINATING` where it was `PROVEN_TERMINATING_IF_DEFINED`: the subscripts' bounds follow |
| index relations | A = (I, x0 + K), B = (x0 + K, J), C = (I, J): 6 IDX-use unsat, each over the coordinates |
| coordinate checker | V1h, V1f, V1g unsat, `SYMBOLIC_EXACT` ("V0 to V4 and S1 to S3 discharged"); with the certificate as it was: V1f sat, `PROPOSED` |
| domain report (roots authorized) | views A = (L0, p2 + L2), B = (p2 + L2, L1), C = (L0, L1) proved at both tiers and affine; decomposition `VERIFIED at SYMBOLIC` |
| time | about 5 s (synthesis report), 7 s (domain report) |

The same invariant given by hand gives the same proof, reported as given. With one
sample point the search offers 12 atoms, three IDX-init models become runs, and the same
single atom is proved.

Variants, all searched (`~/work/semgus-invariant-validation/gemm-index/programs`, outside
the repository; the unit and lit tests hold the acceptance case, `carried`, `narrow`,
`narrow_nsw` and `bump`):

| Program | Invariant found | Coordinate tier with / without |
| --- | --- | --- |
| x updated before the MAC | `s6 == p2 + s5`; relation A[1] = (p2 + s5) + 1 | `SYMBOLIC_EXACT` / `PROPOSED` (V1f) |
| step 2 | `s7 == p2 + 2·s6` | `SYMBOLIC_EXACT` / `PROPOSED` |
| x runs down | `s5 + s6 == p2`; relation p2 − s5 | `SYMBOLIC_EXACT` / `PROPOSED` |
| two recurrences (x for A, y for B) | `s6 == p2 + s5`, `s7 == p3 + s5` | `SYMBOLIC_EXACT` / `PROPOSED` |
| plain GEMM (no recurrence) | none (`NOTHING_FOUND`) | `SYMBOLIC_EXACT` / the same |
| 8-bit x, 16-bit k, K up to 300 (x wraps) | `s7 == p2 + trunc8(s6)`; relation zext16(p2 + trunc8(s6)) | `SYMBOLIC_EXACT` / `PROPOSED` |
| the same with nsw, K up to 20 | the equality and `¬poison(x)` | `SYMBOLIC_EXACT`, every run |
| the same with nsw, K up to 300 | the equality; `¬poison(x)` offered and dropped | `SYMBOLIC_EXACT` for runs that complete |
| strength-reduced, linearized arrays (`bump`): ra += kk per i, x = ra + k, y = j + k·m, C[rc + j] | degree 1: x == ra + k, cj == rc + j; degree 2 adds ra == kk·i, rc == m·i (outer headers), y == m·k + j | degree 1: `PROPOSED` (V1h); degrees 1, 2: `SYMBOLIC_EXACT`, relations i·kk + k, j + m·k, i·m + j proved; 5 to 7 s at index widths 8, 16 and 32 |

Reverse cases:

| Case | Answer |
| --- | --- |
| wrong invariant `x == x0 + k + 1` | `NOT_INDUCTIVE`: IDX-step has a model on the path that enters the k loop; no certificate strengthened; the coordinate tier stays at V1f |
| `x == x0` (true on entry, not preserved) | `NOT_INDUCTIVE` on the back edge |
| false at an initial state (`i == 1` at the outer header) | `NOT_INDUCTIVE` by IDX-init; the model's parameters are a point of the scope |
| wrong index relation A[1] = x0 + k + 1 | `REFUTED`: a run from the model's parameters returns and its event has subscript 4 where the relation gives 5; "refutes the relation, not the program" |
| true relation judged under no invariant | `UNKNOWN`: IDX-use has a model, the run from it returns with the relation true at every event; the model is kept as text and refutes nothing |
| true relation that reads x itself | `PROVED`, `over_coordinates: false` |
| missing entry condition: x carried through the j loop, `x == x0 + k` given | `NOT_INDUCTIVE` on the path from the j header into the k header; the search finds what does hold there, x == xj + k |
| missing entry condition on a parameter: `x == k` given | `NOT_INDUCTIVE` with x0 in [0, 5]; `INDUCTIVE` with x0 = 0 |
| overflow: `zext16(x8) == k + zext16(x0)` given | `NOT_INDUCTIVE` with K up to 300 (model at x8 = 255); `INDUCTIVE` with K and x0 up to 100 |
| not a cut, not a condition, not a cut set, no certificate | no verdict (`UNKNOWN`), nothing handed on |
| a solver that answers unknown, or sat with no model | never `INDUCTIVE`, never a strengthened certificate |
| `checkCertificate` undecided on its own O4 | the search's atoms are `INDUCTIVE` and are not handed on |

**No unrolling.** The proof's queries are one per path between cuts. For the 8-bit
recurrence with K up to 4, up to 300 and up to 30000 (index width 16): 6 IDX-init and
IDX-step queries and 6 IDX-use queries each time, 0 program runs, 5.3 to 5.9 s; the
candidate search ran 24 sampled runs of at most 12 iterations of the k loop in all three.
The finite tier at those scopes is a sample (`POINT`), and says so; the symbolic tier is
about every point.

**Where recognition stops** (domain report, `--index-invariants`):

| Program | Reaches | Stops at |
| --- | --- | --- |
| acceptance case, i32 cells, roots authorized | coordinate `SYMBOLIC_EXACT`, computation, domain and Theorem 13 `SYMBOLIC`, views proved and affine, `lowering_eligible: true` | `authorizes_lowering: false`, as for every research result; the separation of the roots is an authorization, an assumption on record |
| the same, roots not authorized | coordinate `SYMBOLIC_EXACT`, domain `SYMBOLIC`, views proved | computation: "not authorized: roots 0 and 2 are separate"; decomposition `UNKNOWN` |
| the same with f32 cells | coordinate `SYMBOLIC_EXACT`, decomposition `VERIFIED at SYMBOLIC`, views proved | "no reorder fact (trivial, integer or float) is proved over the scope": the float reorder is `UNKNOWN`, and nothing here authorizes one |
| x carried through the j loop | the invariant x == xj + k | coordinate tier `PROPOSED`: "no enumeration order to bound the coordinates" (the families the search finds there have x as a batch coordinate); finite tier only |
| nsw recurrence with no scope at all | the equality; V0 to V4 discharged (`SYMBOLIC_VALID`) | coverage: "the walk to the next event is longer than the limit" |

A proved index relation is a statement about subscripts. Where the table says the
pipeline stops, the claim stops there too.

### 14.6 Validation

All on the final sources, Release unless said; logs in
`~/work/semgus-invariant-validation/gemm-index/logs`, with `STATUS` naming each stage.

- **Unit**: section `indexinv`, 216 checks, 0 failures (the acceptance case end to end,
  the forged certificates, relations, given certificates, stub solvers, thin samples,
  the sampler, `carried`, `bump`, the narrow and nsw cases). With `invariants` and
  `nullspace`: 360 checks, 0 failures (101 s, 847 MB). The whole synthesis unit test:
  7337 checks, 0 failures, 502 s; its CTest timeout is raised from 600 to 900 s.
  Domain unit test `core`: 1545 checks, 0 failures (255 s).
- **Lit**: `synth_index_invariants.mlir`, 11 RUN lines on both reports; the suite 31 of 31.
- **Default output unchanged**: every report RUN line of the existing lit files, run
  with the 23cac4c binaries and the new ones (same tools): 60 of 60 byte-identical; the
  9 lines that differ use an option the baseline does not have.
- **Independent oracle** (`index_invariant_oracle.py`, CTest
  `generalized-gemm-index-invariant-programs`): its own interpreter of each spec at every
  point of the scope and its own reader of the report's terms; no solver, no project
  code. Seed 1: 23 programs (11 hand-stated, 12 random), 0 problems; seed 2: 27
  programs, 0 problems. Over both: about 210 000 atom-at-arrival checks, 774 000
  relation-at-event checks, 58 refutation witnesses replayed, 6560 scope points checked
  for a symbolic tier, and 120 given invariants that are false at some arrival, none
  called inductive. The oracle itself needed two fixes while it was written (a scope its
  generator stated outside a narrow width, and operators printed with `nsw`); neither
  was a defect of the reports. `invariant_oracle.py` (Section 13) passes again.
- **Mutation** (`synthesis_mutation.py`, Debug, `--unit-only`, baseline passing first):
  22 of 22 new mutants killed by the `indexinv` section (16 in the checker, 5 in the
  search, 1 in the truncation leaves); the 4 null-space mutants of M1, whose function
  this change touched, killed again by `nullspace`. The mutants were judged on the
  sources as they stood before the `bump` checks were added; later edits only add checks
  and a record field. The domain wiring
  got its own test and mutants afterwards (below).
- **Sanitizers** (Debug, ASan and UBSan, `-fno-sanitize-recover=undefined`): `indexinv`
  216 checks, 0 failures, no report (71 s, 1.1 GB); `nullspace` 35 checks, 0 failures, no
  report (24 min, 1.8 GB); the synthesis and domain reports on the acceptance case with
  `--index-invariants`, no report.
- **Domain wiring** (added after 4f607b4). `handIndexInvariants` (`DomainAnalysis.h`) is
  the one place where the strengthened certificate and the identities are handed to the
  domain analysis; the domain report calls it. Section `indexwiring` of the domain unit
  test runs the acceptance case four ways and asserts each answer (35 checks, 0
  failures, 14 s): strengthened certificate and identity, A and B views proved at both
  tiers; strengthened certificate alone, coordinate tier `SYMBOLIC_EXACT` and no A or B
  view proposed; identity alone with the certificate as it was, the views proposed and
  not proved over the scope; a wrong identity (x = x0 + k + 3), proved at no tier.
  `domain_mutation.py` (Debug, baseline passing first, a kill only by a test's own
  assertion): 3 of 3 killed by that section, the identities not handed to the proposer,
  the identities not gathered from the search, and the given certificate handed on in
  place of the strengthened one. The harness gained `--tests`, to run named sections
  only; its default is unchanged. `phase4` and the lit files `synth_index_invariants`
  and `dom_phase4` pass again.
- **Not run**: the other mutation catalogues and oracles (coordinate, computation,
  domain, termination), the `holes` section of the domain unit test, and
  `check-systolic`; this change does not touch what they exercise beyond the lines the
  byte comparison and the domain unit test cover.

### 14.7 Supported, and not

Supported: programs the model supports with MAC sites; any nest for which G2a has a
certificate (its cut set is used as it is); loop-carried integer state of any width up
to 64 bits, including `scf.for` `iter_args` after the structured lowering; relations
that are polynomial equalities of the stated degree over the state and parameters at a
header, with truncations of wider leaves; not-poison atoms; given conjunctions of any
atoms the term language holds.

Not supported, by name:

- **No termination certificate, no invariants.** The cuts and the consumers come from it.
- **Candidates are equalities and not-poison atoms.** No bounds, no congruences, no
  disjunctions are proposed; the certificate's own invariants supply the bounds that
  were needed here. A relation that needs one the certificate lacks is not found.
- **Samples are read over the integers.** A narrow recurrence that already wraps inside
  the sampled runs (3 bits in the oracle's catalogue) satisfies no integer relation
  there, and the search proposes nothing for it; the relation given by hand is proved.
  A relation with an even coefficient on every variable has no solution for a
  subscript, so no relation is proposed from it, though the atom itself is checked.
- **Mixed widths only by truncation.** Zero and sign extensions are not candidate
  leaves; a relation that reads a narrow variable as an integer is true only without a
  wrap and needs bounds the search does not propose.
- **Degree 2 is on request** (`--index-invariant-degrees=1,2`), and its matrices are
  skipped above 128 monomials. It was measured on one program (`bump`).
- **Relations are proposed from one cut's equalities** and only where the rewritten
  subscript reads the state the coordinates read; coordinates that are not plain state
  variables get a structural label only when the relation contains their terms whole.
- **Sites reached without passing a cut** have the entry's premise only.
- **The domain report prints no per-candidate relations**; it hands the identities to
  the proposer and reports the views the domain checker proved.
- Everything Sections 13.7 and the earlier phases list about solver answers applies: a
  timeout or an unknown leaves `UNKNOWN`.

### 14.8 In a nest's record (integration branch, 2026-11-18)

On `integrate/index-invariants-into-nest-records`: this line merged with the lowering
line's committed 771b700 (nest records, `docs/GENERALIZED_GEMM_SCOPE.md`), and nothing of
its uncommitted scf.while work.

`analyzeNest` (`NestRecord.cpp`) runs `findIndexInvariants` between the termination and
the domain analysis when `NestAnalysisOptions::indexInvariants` is set
(`--index-invariants` on `generalized-gemm-nest-report`; off by default), and hands the
result on with `handIndexInvariants`. The record gains `index_invariants` (status, the
invariants, the identities, whether the certificate checker accepted the strengthened
certificate), printed only when asked. The record's status is still read off the checkers.

The arguments of an `scf.while` body are copies of its header's. `indexIdentities` adds,
for a state variable whose every assignment is one other variable, "copy = the source's
solution" (or the source itself): a proposal, from which the proposer writes a view of a
subscript that reads the body's argument. `DomainProposer.cpp` is not changed.

Records, 2x3x3 f32 nests with `reassoc`, index width 16 (unit section `indexinvariants`):

| Nest | Without | With `indexInvariants` |
| --- | --- | --- |
| x by `iter_args` from the constant 1 | `VERIFIED_NOT_MAPPED`: coordinates not verified over the scope, no closed form of A or B | `VERIFIED`, A = [I, K + 1], B = [K + 1, J], no refusal |
| x a second carried value of an `scf.while` | `VERIFIED_NOT_MAPPED` | `VERIFIED`, the same views |
| plain GEMM | `VERIFIED` | `VERIFIED`; nothing found, nothing handed on, same refusals and assumptions |
| x from a function argument (scope [0, 1]) | | invariant proved; `VERIFIED_NOT_MAPPED`: "reads a parameter, and a symbolic offset is not mapped" |
| x carried through the j loop | not mapped | not mapped |

The binding (stage 2) already covers the recurrence: changing the start or the step gives
another binding (checked in the same section), so `NestBinding` and `NestRecordGate` are
unchanged.

Validation: nest unit test 917 checks, 0 failures (231 s), of which `indexinvariants` 78
checks; lit `nest_*` and `synth_index_invariants` 8 of 8; `indexwiring` 35 and `indexinv`
216 checks again, 0 failures. `nest_mutation.py` (Debug, a kill only by an assertion): 4 of
4 killed by `indexinvariants`: the search never run, the certificate handed on as it was,
the certificate handed on without the identities, copies given no identity. The harness
gained `--build-jobs` and `--tests`; defaults unchanged. Not run: the nest oracle, the
nest e2e and gate harnesses (they need the production build, and the lowering is not
changed), and the other catalogues.

**A verified record is not a lowering.** The lowering (`NestRecordLowering.cpp`) refuses
both shapes: a loop that carries values, and an `scf.while` that carries more than its
counter. Reading a carried index as start + step * counter from the IR, and comparing
that with the record's view, is the remaining wiring; it sits in the band reader the
other line is changing, and waits for that. Plan and acceptance programs:
`~/work/semgus-invariant-validation/gemm-index/integration/WIRING_PLAN.md`.
