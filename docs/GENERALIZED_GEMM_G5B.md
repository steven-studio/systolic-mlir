# Generalized GEMM recognition: Phase G5b, beyond one lexicographic order

G5b extends G5a where its one guarded order over one coordinate family is not enough:
sibling loops, multiplicity that varies over the domain, and events ordered across loop
regions. This record holds the audit, the design, and the first milestone. It is
research-only, nothing here is linked into production lowering, and no result authorizes
one. Earlier records: `docs/GENERALIZED_GEMM_G0_G1.md` to `_G5A.md`.

The SemGuS review of 2026-11-05 decided that the existing grammar and search stay
(architecture A); no SemGuS code is part of G5b.

## Audit: what G5a cannot say, and why

Each limit below was reproduced with the committed G5a driver at `8aa5447`, and traced to
the function that imposes it.

| Program | G5a result | Where, and why |
| --- | --- | --- |
| `sib_same`: two loops `i`, `j` over the same roots and cells | search Unknown, "no candidate was verified within the limits" | `Enumerator::fillLive` (`FiniteDomain.cpp:271`) keeps as live state only variables defined at every event of the group. `j` is undefined during the first loop, so it is no coordinate; `i` keeps its final value through the second loop, so both second-loop events have the point `[1,0,1,2]` at n = 2. No term over state tells them apart |
| `sib_counter`: a counter carried through both loops | the same | The second loop's counter is a second block argument, a second state variable, undefined during the first loop |
| `sib_halves`, `sib_reversed`, `sib_conditional` | the same | The same |
| `sib_roots`: the second loop reads another operand root | two groups; each group's domain claim Verified; G4a REFUTED each with a witness; Theorem 13 REFUTED | `groupSites` (`FiniteDomain.cpp:36`) groups by the exact root triple. G4a's replay relation is per group, and each group's cells are written by the other, so each group's own relation is false. The refutation is correct for the claim made; the program as a sum of two groups has no claim at all |
| `growing`: `t in [0, k+1)` with `t` outside the family | Theorem 12 Verified; multiplicity `COUNTED` at points and `UNKNOWN` symbolically | `constantBox` (`DomainChecker.cpp:178`, G5a) admitted a box only when repeat bounds read parameters alone |

Two more facts from the audit:

- With the search's own candidates, `growing` has multiplicity `UNIT`: the search absorbs
  `t` as a K coordinate. Varying multiplicity arises when the family is given, as G6 will
  give it.
- The G5a proposer fitted `t`'s bound as the constant 3 and rescued the box with a
  piecewise guard of eleven disjuncts, because a constant fit was tried before any bound
  that moves with an outer level. Correct, verified, and needlessly weak.

So the sibling limit is one of **representation**: a coordinate family is one tuple of
terms (`CoordinateFamily.axes`), a guarded order is one sequence of levels
(`GuardedOrder.levels`), and both are evaluated at every event of the group. No grammar
extension changes that, since no term over the state at a second-loop event can recover
the first loop's position. The multiplicity limit was one of a **missing closed form**,
and is the first milestone.

## Design

### Objects

For a group with sites in several regions:

- A **segment** is a set of the group's sites with its own coordinate terms and its own
  guarded order. Every site is in exactly one segment.
- A **segmented family** gives, per segment, the tuple of terms for P, I, J, K. A term is
  evaluated only at the events of its segment, where it is defined.
- A **segmented order** is a sequence of segments. Its set is the disjoint union
  S = S₁ ⊔ … ⊔ Sₘ, ordered segment by segment and lexicographically within one.
- The **extended point** of an event is (segment index, λ_segment).

A program with one region has one segment, and every G5a object is that case.

### Theorem 15 (segmented orbit)

Let the segments be s = 1 … m in that order. Suppose D1 for segment 1 from the entry;
D2 and D3 within each segment as in Theorem 12; and

- **D4 (hand-over).** On every path from an event of segment s with no successor in S_s
  to the next event, that event is at a site of segment s+1 and its λ is min S_{s+1}; and
  on every path from an event of segment s with a successor, the next event is at a site
  of segment s.
- **D5 (empty segment).** On every path from the entry, or from the last event of
  segment s, that reaches the first event of a segment later than s+1 without an event of
  s+1, S_{s+1} is empty.

Then, for every parameter point in scope and every run that completes, the extended
points of the events in order are exactly the elements of S in order.

*Proof.* Theorem 12 within each segment; D4 and D5 glue consecutive segments, with an
empty segment skipped only when its set is empty. ∎

Each of D4 and D5 is a QF_BV query over G3b's walker, with the next event's site known
from the location it arrives at. A guard at the boundary (`sib_conditional`) is a path
condition like any other: the second segment's set is empty where the guard is false, and
D5 demands that.

### Theorem 13 over segments

G4a's replay relation is already per group, over every site of the group in execution
order, and knows nothing of segments. So Theorem 13 holds as stated with the segmented
order in place of the guarded order, and the per-cell key becomes (segment index, κ_s):
a cell written by both loops of `sib_same` has its first loop's contributions before its
second loop's, which D4 proves.

### Different operand roots

`sib_roots` is not a G5b domain question. The group is the unit of G4a's claim; a claim
about the sum of two groups needs a **multi-group replay relation** in G4a: replay all
listed groups' events in execution order, each with its own operands, and demand that
every other write be disjoint from the union of their cells. That is a change to
`ComputationChecker`'s public contract (`CheckedComputation` over a set of groups) and is
listed under the decisions below. Until then each group's claim is refuted, correctly.

### What does not change

- `CheckedFamily`, `CheckedComputation` and `CheckedDomain` keep their constructors
  private and their bindings; a segmented request is bound like a request.
- D1 to D3, K1, C1, T1, A1 and the reorder facts keep their statements within a segment.
- Refuted still needs a concrete point, and every solver unknown, limit or cut walk is
  Unknown.

### Milestones

| Milestone | Scope | Contract change |
| --- | --- | --- |
| **M1 (this record)** | Multiplicity as a box over the logical levels, Theorem 14 | None: a new `DomainMultiplicity` value and a wider μ term |
| M2 (below) | Segmented families and orders, Theorem 15, in G5a's checker; a segmented proposer | **Yes:** `DomainRequest` gains segments, each with its own `CoordinateFamily`; `CheckedDomain` gains per-segment facts and a shared-cells fact. `CoordinateFamily` itself is unchanged and G3b is **not** restated over segments: a segmented request's logical form stays `Proposed` |
| M3 | Multi-group replay in G4a for `sib_roots` | **Yes:** `CheckedComputation` over a set of groups |
| M4 | G3b over per-segment terms, so that a segmented Theorem 13 can carry a logical tier | **Yes:** G3b's SymbolicValid obligations restated over segments |

M3 and M4 are not started.

## Milestone 1: multiplicity as a box over the logical levels

**Theorem 14.** Let the levels be split into logical ones (the family's coordinates) and
repeat ones. Suppose: (C1) the guard reads no repeat level; (C2) no logical level's
bounds read a repeat level; (C3) no repeat level's bounds read a repeat level. Then for
every logical point x of S(v),

    μ(x) = Π over repeat levels r of max(hi_r(v, x) − lo_r(v, x), 0),

each span in its level's signedness, computed at 128 bits when the repeat widths sum
below 128.

*Proof.* Fix v and x. By C2 and C1, whether (x, r) ∈ S depends on r only through the repeat
bounds; by C3 each repeat bound is a function of v and x alone. So the repeat tuples of x
form a product of intervals [lo_r, hi_r), each with hi_r − lo_r members when lo_r < hi_r
in its signedness, else none. By Theorem 12 the events of x are exactly those tuples. The
extension to 128 bits is exact because every span is below 2^w and the widths sum below
128. ∎

When μ reads no level the form is `CONSTANT_BOX`, as in G5a; when it reads logical
levels it is `LOGICAL_BOX`, and `multiplicity_term` is read at any event of the point.
The finite route evaluates μ at the first event of each logical point and refuses a
mismatch as a defect of this derivation, which cannot happen under Theorem 14; the oracle
recomputes μ at every event of every point from the term and the counts.

Implemented in `boxMultiplicity` (`DomainChecker.cpp`), replacing `constantBox`, with the
condition C3 relaxed from "reads no level" to "reads no repeat level". The proposer now
fits a bound as an outer level plus a constant, nearest level first, takes such a bound
only where it brings the box nearer the events, chooses a level's signedness before its
direction, and adds repeat levels greedily by how many events each leaves out of order.
Each of those is a guess the checker judges. One of them is a matter of form: a
repeat level is tried innermost first, since placed outside a logical level it makes that
level's bound read it, which Theorem 14 refuses although the set is the same.

### How M1 is checked independently

The G3b search takes a repeat loop as a K coordinate, so no searched candidate has a
repeat level and the oracle would never see a `LOGICAL_BOX`. The driver therefore takes
`--family=p:ids;i:ids;j:ids;k:ids`, state ids per axis, which replaces the search by one
given candidate per group (`DomainAnalysisRequest::family`). It is a proposal like any
other: the checkers judge it, and nothing in a checker reads it. The oracle runs each spec
that names repeat loops twice: once to learn which state holds which loop variable from
the printed tables, then with the other loops as the family. It evaluates the emitted μ
term itself at the levels of every logical point of L(v), including points whose repeat
box is empty and so have no event, compares it with its own count of events, and checks that the term plus one
disagrees somewhere, so the comparison is no tautology. The run fails unless at least 4
`LOGICAL_BOX`, 1 `CONSTANT_BOX`, 2 refusals and 1 zero-event point were checked.

### What M1 does not do

- A repeat bound that reads another repeat level (`u in [0, t+1)`) stays `COUNTED` at
  points and `UNKNOWN` symbolically: that set is a simplex, not a box.
- A guard that reads a repeat level stays as it was.
- Nothing about sibling loops changes: they are pinned as Unknown by tests, with G4a's
  computation claim still Verified for them.

## Milestone 2: segmented families and orders

M2 lets one group's sites be split into **segments**, each with its own coordinate terms
and guarded order, and proves that the events of a run are those of the segments' sets one
after the other. It is what `sib_same`, `sib_halves`, `sib_conditional` and their kin
needed: two loops over the same roots, each with a loop variable that is undefined during
the other loop.

### Objects

```
struct Segment {                      // DomainModel.h
  std::vector<unsigned> sites;        // its sites: a block of the group's partition
  CoordinateFamily family;            // its logical terms, read only at its events
  GuardedOrder order;                 // levels, bounds, guard over its level variables
  std::vector<unsigned> cellKey;      // which levels order a cell, within the segment
  std::array<std::vector<Term>, 3> access;
};
struct DomainRequest { ...; std::vector<Segment> segments; };   // nonempty = segmented
struct SegmentFacts { sites; multiplicity; multiplicityTerm; cellKey; outputInjective; access; };
```

A request with `segments` nonempty is **segmented**; its own `order`, `cellKey` and
`access` are unused, and the family passed beside it must be empty, or the checker refuses
it (`a segmented request carries its own families`). The segments are in execution order,
and their sites must partition the group exactly: a site in no segment, in two, or outside
the group is refused. A request with one segment is the G5a request in another coat and
gets the same answers (tested).

The segments' **event sets** are disjoint by construction: an event belongs to the segment
of its site. Their **memory** need not be: two segments may write the same cells, and
whether they do is a fact (`CheckedDomain::sharedCells()`: Proven, no cell of two
segments over the scope; Refuted, a point and a cell written by two), never a condition.
The one-contribution fact T1 is judged over the whole group, so a shared cell refutes it.

### Theorem 15 (segmented orbit)

Let a group have segments s = 0, …, m−1 in that order, each with a guarded set S_s(v)
over its own levels, and let the run at parameter point v complete. The **extended point**
of an event is (s, λ_s), with s the segment of its site and λ_s its segment's levels read
at the block entry. The claim: the extended points of the events, in order, are exactly
S_0(v) ⊎ … ⊎ S_{m−1}(v) in order, segment by segment and lexicographically within one.

Obligations, each a QF_BV query over G3b's walker that must be **unsat**:

- **D1' (first event).** On every path from the entry to the first event, that event is
  at a site of some segment s, its λ_s is min S_s(v), and S_r(v) is empty for every r < s.
  On every path from the entry to the exit with no event, every S_s(v) is empty.
- **D2 (successor, within a segment).** From an event of segment s with a successor in
  S_s(v), on every path to the next event, that event is at a site of s and its λ_s is the
  successor. (Theorem 12's D2.)
- **D3 (last).** From an event of segment s with a successor in S_s(v), no path reaches
  the exit. (Theorem 12's D3, per segment.)
- **D4 (hand-over).** From an event of segment s with no successor in S_s(v), on every
  path to the next event, that event is at a site of a segment s' > s, its λ_{s'} is
  min S_{s'}(v), and S_r(v) is empty for s < r < s'.
- **D5 (no return, empty tails).** From an event of segment s, no path reaches an event of
  a segment r < s. From the last event of segment s (no successor), on every path to the
  exit, S_r(v) is empty for every r > s.

Every obligation is stated under the premise that the event it starts from is in its
segment's set (as in Theorem 12, by induction on the events) and under the scope.

*Proof.* By induction on the events of the run. D1' places the first event as the minimum
of the first nonempty set and shows every earlier set empty. Given that the events so far
are a prefix of S_0 ⊎ … ⊎ S_s ending at some element of S_s: if that element has a
successor in S_s, D2 makes the next event that successor and D3 forbids the exit; if it is
the last of S_s, D4 makes the next event the minimum of the next nonempty set, with the
sets between empty, and D5 forbids an event of an earlier segment ever again. At the exit
D5 shows every later set empty, so nothing of S is omitted; and nothing is duplicated
because each step moves to the strict successor in the segmented order. ∎

**Where each obligation is discharged.** Symbolically, `proveSegmentedOrbit`
(`GuardedOrbit.cpp`) asks every query of D1', D2, D3, D4 and D5 and returns true only
when all are unsat; any sat, unknown, limit or cut walk is Unknown with the first failing
obligation named (`D4 at between segment 0 to segment 2, segment 1 empty: sat`). At
points, `finiteSegmented` (`DomainChecker.cpp`) re-runs each point keeping the whole state
at every event (`eventStates`, so a segment's term can read a loop variable the live table
omitted), evaluates each event's segment terms there, and refutes with the point when the
events' segments ever decrease, or when any segment's events in order differ from its set
in order. Both routes then judge the key, C1 and μ within each segment, and T1 and shared
cells over the group. A proposal that cannot be evaluated at an event (a term reading
state undefined there) is refused, not guessed; so is an event at a site of no segment,
which the partition check already excludes.

**Guards at the boundary.** A sibling loop under `scf.if` (`sib_conditional`) is a
segment whose guard is the branch condition over the parameters: where it is false the
segment's set is empty, which D4 and D5 demand and the finite route checks. A guard
read from the program's conditions is still a proposal: the checker evaluates it, nothing
else.

### Theorem 13 over segments

`checkReduction` makes Theorem 13 for a segmented request exactly as before: G4a's
computation claim over the group, which knows nothing of segments, together with Theorem
15 over the scope, with the per-cell order (segment, κ_s). A cell written by both loops of
`sib_same` has its first loop's contributions before its second loop's, which D5 proves,
and the integer reorder fact is proven over both. What changes is the logical form: G3b
judges one family over every event of the group and a segmented request has one per
segment, so `logicalTier()` is `Proposed` for every segmented request. Theorem 13's
status and tier are unaffected (`VERIFIED SYMBOLIC` for the sibling cases), and G4a is
not bypassed: `sib_roots`, two loops over different operand roots, gets two groups, each
segmented and each with Theorem 15 verified, and G4a refutes each group's computation as
before.

### The segmented proposer (untrusted)

`proposeSegmentedDomain` reads the events of every sample point with their full state,
puts two sites in one segment when their events interleave at some point (transitively),
orders segments by their first site (program order), and fits each segment's order to its
own events with the G5a fitting (`fitOrder`, now shared): loop-carried block arguments
that vary within a point are its coordinates, other varying state its repeat candidates,
state with one value at every event a constant for the guards. Bounds may now be another
level plus a parameter plus a constant (`sib_counter`'s second loop runs its counter from
n). With `--segments --family=…` the family names the logical loop variables and the
rest are repeat levels, as with a flat `--family`; without it every loop variable is a
coordinate and a repeat loop is Unit over (j, t) rather than a box over j. A segment no
sample reaches gets an empty box with guard false, which the checker judges against the
scope. Nothing the proposer does is evidence.

### How M2 is checked independently

The oracle (`domain_oracle.py`) gained **segmented specs**: a list of nests run one after
the other, each with one MAC site, some under a condition on n, some empty at some points,
some with a repeat loop. The driver runs them with `--segments` (and again with the
logical variables named when a nest has repeats), and the oracle recomputes, at every
point of the scope and with its own interpreter:

- the events' segments never decrease, and each segment's events in order are its set in
  order (Theorem 15, from the request's own bounds and guards);
- the key, C1 and μ within each segment (μ at every logical point of the nest, as in M1);
- T1 over the group, a proven shared-cells fact against the cells two nests write, and a
  refuted one replayed at its point;
- Theorem 13 with random fills, folding each cell segment by segment and within a segment
  by the key;
- no family beside a segmented request, no G3b tier over segments, no later phase's claim.

The run fails below 8 verified segmented claims with two live segments, 2 empty segments
at a point, 1 shared-cells proof and 1 refutation, 1 box inside a segment and 1 fold that
crosses a segment boundary. The mutation harness refuses an anchor that is no longer
unique (14 of M1's anchors were duplicated by the segmented copies of the checks, and are
re-anchored with surrounding lines; each makes the same edit at the same place in the
one-order route, and one, `entry-block site admitted`, also deletes the comment line above
its check), and gains 35 mutants over `proveSegmentedOrbit`, `finiteSegmented`,
`symbolicSegmented`, `adoptSegments` and the dispatch, one of them listed equivalent with
its reason (the segment copy of the defensive box check). `testSegmentGates` gives each
obligation of Theorem 15 a request that only it refutes (a first segment too long for D4,
a shifted second segment for D4's first event, a later loop declared an empty earlier
segment for D5, a nonempty set before the first event and at a no-event exit for D1', a
nonempty set between a hand-over for D4), asks every obligation through a selective
solver, and runs the limit, certificate, sample, event-limit, wider-scope, entry-block,
shared-block and closed-gate refusals of the one-order route on the segmented one. The
first full mutation run left three survivors and two crashes, each a missing test rather
than a defect: a level that steps back without skipping (`sib_diag`, D2's increase
alone), a set of the same size with other elements (the second segment offset by one), a
scope wider than the enumeration, a set longer than the events (the size mutant now admits
only that, so it refutes instead of indexing past the end), and a site of no segment
(now a refusal, not a map error). The CTest oracle run keeps 40 programs and gets 3600 s.

### What M2 verifies, refuses, leaves

**Verified** (unit `segments`, oracle, lit `dom_segments.mlir`): two sibling loops over
identical roots and cells; disjoint halves; different bounds; the second loop reversed,
as j ascending or 3 − j descending; a sibling loop under a parameter condition; an empty
first or second segment inside the scope; three segments with the middle one empty at a
point; a repeat loop inside a segment as a logical box; a counter carried through SSA
from the first loop into the second, by the loop variables; one segment for one loop.

**Conservatively refused or refuted**, with the reason named: a family beside a segmented
request; sites in no, two, or foreign segments; segments swapped; a segment too long, too
short, or declared in the wrong direction; a level that is not monotone in its loop
(`sib_shuffle`'s shuffled index); the second segment's guard dropped; one segment for
two loops; a segment order wrongly ordered by the proposer (`seg_three` before the fix,
refuted by the finite route and Unknown at D4 symbolically).

**Unsupported, Unknown by design**: a segmented request's logical form (G3b over
segments, M4); two loops over different operand roots as one claim (M3); the
SSA-carried counter *as a level* over [n, 2n) is Verified at points and Unknown
symbolically at D2, because the symbolic route does not infer the invariant p₂ = n + j;
a segment with at most one event per sample point gets no proposal (`a segment whose
events no state tells apart`), though a hand-written request for it is verified; sites
that interleave at a point share a segment and are then judged as G5a would.

**Untested assumptions**: Theorem 15 is stated for runs that complete; its symbolic
discharge rests on G3b's walker and the solver as D1 to D3 did, and on obliviousness of
the trace, which `traceIsFixed` checks as before. The proposer's reading of full state at
each event assumes the reference run is representative, which the checker's own re-run
confirms or refutes.

**Coverage limits of the tests**: `finiteSegmented` re-runs each point under
`DomainLimits::events`; a re-run that does not return, or a point whose memory exceeds the
cell limit there, makes the point incomplete and the tier at most Point, with the reason
kept. No test sets those limits below the enumeration's own, so that branch is not
exercised, and the candidate mutant that would admit a cut re-run (it would compare a
prefix of the events with the set and refute falsely) was left out of the harness rather
than killed. Five of the ten equivalent mutants rest on an argument from the code's
structure, not on a named test; their reasons are in `domain_mutation.py`.

## Validation of M2 (2026-11-07, LLVM 24.0.0git, Z3 4.8.12, base 2a1afa0)

Every result below is from one frozen tree (hash `f1c676a8…` over `include/`, `lib/` and
`research/generalized-gemm`), unchanged from the start of the run to its end, with the
suites run one at a time and every subprocess under a limit. A first run of the same
script on the tree before the mutation follow-ups above had one CTest timeout (the oracle
at 1800 s) and five open mutants; the tests were added, and the whole run repeated.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 18 of 18 CTest suites, G1 to G5a and G5b-M1/M2; the oracle suite 1,840 s of its 3,600 |
| G5a/G5b unit, core | 709 checks, 0 failures; `segments` 228 among them (`testSegments` and `testSegmentGates`), `siblings` 60 unchanged |
| Oracle, seed 20261110 | 194 programs: 102 flat (375 points against S(v), 5,543 Theorem 13 runs, 3,502 permutations, 45 witnesses) and 92 segmented (134 verified symbolic claims, 475 points against Theorem 15, 7,582 segmented Theorem 13 runs, 212 empty segments at a point, 238 shared-cells proofs and 67 refutations replayed, 88 `LOGICAL_BOX` and 96 `CONSTANT_BOX` segment terms at every logical point, 4 without a proposal), 0 disagreements, 0 timeouts |
| Mutation | 126 mutants: 116 killed by a test's own assertion, 10 equivalent with reasons in `domain_mutation.py`, 0 survived, 0 timeouts, 0 resource limits, 0 harness errors |
| ASan, UBSan, leaks | Clean: unit core 709 checks and 8 oracle programs |
| Byte-identical | G1 42, G2a 14, G3a 14, G3b 14 outputs against 2542b68, G4a 16 against 4ecb778 |
| Lit | `dom_segments.mlir`: the sibling program verified through `--segments`, Unknown without |

## Validation of M1 (2026-11-06, LLVM 24.0.0git, Z3 4.8.12, base 8aa5447)

Every result below is from one frozen tree, unchanged from the start of the run to its
end, with the suites run one at a time and every subprocess under a limit.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 18 of 18 CTest suites, G1 to G5a and G5b-M1 |
| G5a/G5b unit, core | 481 checks, 0 failures; `multiplicity` 61 and `siblings` 60 among them |
| Oracle, seed 20261108 | 102 programs, 390 points against S(v), 5,929 Theorem 13 runs, 3,792 permutations, 40 witnesses replayed, 0 disagreements, 0 timeouts |
| Theorem 14, oracle | 42 repeat specs generated: 9 accepted `LOGICAL_BOX`, 9 `CONSTANT_BOX`, 14 refused (`COUNTED`/`UNKNOWN`), 9 whose family could not be mapped, 1 with no proposal. 54 μ terms evaluated at every logical point (27 `LOGICAL_BOX`, 27 `CONSTANT_BOX`), 6 zero-event points of L(v) checked, 0 disagreements. The run fails below 4 `LOGICAL_BOX`, 1 `CONSTANT_BOX`, 2 refusals or 1 zero-event point |
| Mutation | 91 mutants: 82 killed by a test's own assertion, 9 equivalent with reasons and evidence in `domain_mutation.py`, 0 survived, 0 timeouts, 0 resource limits, 0 harness errors |
| ASan, UBSan, leaks | Clean: unit core 481 checks and 8 oracle programs |
| Byte-identical | G1 42, G2a 14, G3a 14, G3b 14 outputs against 2542b68, G4a 16 against 4ecb778 |

The one earlier failure of this run was in the oracle, not the checker: its zero-event
check enumerated the logical nest without the spec's logical guard, so it asked for μ at a
point outside L(v), where Theorem 14 says nothing. The oracle now keeps the guard, and
skips the zero check where the guard reads a repeat variable, since no box exists there.

## Changes to earlier phases

None in G1 to G4a. In G5a, `DomainMultiplicity` gains `LogicalBox`, and the one unit test
that pinned the old limitation (`growing_repeat` in `testGapsG5` expected `Counted`) now
expects `LogicalBox` with its μ checked. No other assertion changed.

M2 changes no G5a or M1 assertion or expected output: `testSiblings` still pins the flat
route's answers for the sibling programs (Unknown, never a claim), and the segmented
route is reached only through a request with segments or `--segments`. New public
surface: `Segment`, `DomainRequest::segments`, `SegmentFacts`,
`CheckedDomain::segments()` and `sharedCells()`, `DomainLimits::events`,
`eventStates`, `proveSegmentedOrbit`, `proposeSegmentedDomain`,
`DomainAnalysisRequest::segmented`, the driver's `--segments`, and `shared_cells`,
`segments` and per-event `state` in the JSON.
