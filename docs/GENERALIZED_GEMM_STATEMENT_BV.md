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
synthesis and proof of one table template.

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
