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
| M3 (below) | Multi-group replay in G4a for `sib_roots` | **Yes:** `CheckedComputation` over a set of groups, two new entry points and two facts |
| M4 | G3b over per-segment terms, so that a segmented Theorem 13 can carry a logical tier | **Yes:** G3b's SymbolicValid obligations restated over segments |

M4 is not started.

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

## Milestone 3: one computation claim over several groups

M3 lets G4a make one claim about the events of several groups at once. It is what
`sib_roots` needed: two loops into the same cells with different operand roots are two
groups, and each group's own claim is refuted, correctly, because the other group's
stores are foreign writes to its cells. M3 does not weaken that claim. It adds a second,
separate claim about the union, which must be proved on its own.

### Objects and contract

```
checkGroupSetFinite(program, enumeration, ArrayRef<unsigned> groups, options)   // Point, ScopeEnumerated
checkGroupSetSymbolic(program, certificate, ArrayRef<SiteGroup> groups, options, backend)  // Symbolic
CheckedComputation::groupSites()   // the sites of each listed group, in the listed order
CheckedComputation::groupOrder()   // SetFact: events in blocks, in the listed order
CheckedComputation::sharedCells()  // SetFact: two listed groups write one cell
CheckedComputation::groupPoints()  // per replayed point: events per group, shared cells, order
SetFact { status Proven|Refuted|Unknown, tier, detail, point }   // ComputationModel.h
replayGroupSet(program, united, groupOfSite, groupCount, point, limits)   // HerbrandReplay.h
ComputationLimits::maxUnfolding    // the walk behind the symbolic order fact
ComputationRequest::groupSets, ComputationResult::sets, driver --groups=all;all-reversed;0,1
```

A set is refused, and is Unknown, when it is empty, lists more than 64 groups, lists a
group or a site twice, or lists a group that is not whole (the sites of its roots, no
more, no fewer, and no site the program does not have). The listed order is used by
`groupOrder()`, the binding, `groupSites()` and the per-group counts of `groupPoints()`,
and by nothing else. The union's sites are taken in increasing order, as one group's are,
and F3 visits the groups by their first site, so the claim, its reason, its records and
its contracts are the same for every listing. The binding names every listed group in order, so a set's claim never stands in
for the claim of one group, or of another set; `checkCandidateComputation` compares
bindings and refuses it. With no set requested, every output of G4a is unchanged.

### Theorem 16 (replay over several groups)

Let G₁ … G_k be distinct whole groups with roots (C_i, A_i, B_i), U the union of their
sites, v in the scope, and M₀ an initial memory whose run completes. Let E be the run's
events at sites of U, in execution order, each event e with its own site's roots
(C_e, A_e, B_e) and indices h(e), f(e), g(e).

    Replay_U:  Mem := M₀, with each prefix constant in place
               for e in E, in order:
                 Mem[C_e][h(e)] := Mem[C_e][h(e)] ⊕ₑ ( M₀[A_e][f(e)] ⊗ₑ M₀[B_e][g(e)] )

**Claim B_U.** (1) each operand read of each event is the initial value of its cell;
(2) each event writes Replay_U's next value; (3) the final value of each cell any event
of U writes is Replay_U's, so for a cell c

    C_final[c] = foldl ⊕ C_init[c] [ a(e) ⊗ b(e) : e ∈ E, cell(e) = c, in execution order ]

with the contributions of every group that writes c interleaved as they ran; (4) every
other memory effect is listed in the frame.

**Obligations.** The obligations of Theorem 9 with U in place of the group's sites,
nothing removed:

| ID | Over the union |
| --- | --- |
| P, AL, E0, F2 | As in G4a: the program, the trace, the certificate, every written root against every touched root, returns free of memory |
| E1, E2, R1, OP1, W1, W2 | For every site of U. E2: each site of U owns its store, and no site outside U shares one |
| OP2 | No write of the program, of any listed group or none, reaches an operand cell of any site of U. Two-copy and order-free |
| F1 | No write outside U reaches a cell a store of U writes |
| F3 | The output and operand roots of **every** listed group are arguments |
| NV | A point of the scope has a completing run with an event of U |

*Proof.* As for Theorem 9, by induction over the ordered writes of the run. By F1, only
stores of U write a cell of U. By OP2, no write reaches an operand cell of U before or
after its read, so with OP1 every operand load returns its M₀ value; this covers a store
of one listed group to an operand cell of another. By W1 and W2 each event's accumulator
read is its cell's current value, and that value is Replay_U's by the induction, since
every earlier write of the cell was a store of U and each such store was one Replay_U
step (R1, E2). Two groups that write one cell are two kinds of step of one fold, taken in
execution order; nothing about their cells being apart is needed or assumed. ∎

The finite route is Theorem 11 with the machine running over U: each event's operands
are compared with initial leaves, each written node with Replay_U's next node, and each
final cell U wrote with Replay_U's; a foreign constant before a cell's first event of U
starts its fold, any other foreign write makes the next comparison differ. **Refuted**
needs a concrete run on which the program and Replay_U differ, found by fills and judged
again by `replayDifference` over U.

**Not the union of the groups' claims.** Each group's claim is checked as before and is
not used. The union is its own claim, with its own replay, its own queries and its own
witness search. (Within G4a's model the per-group obligations of every listed group do
imply the union's, since each already covers every write of the program; the converse is
false, which is `sib_roots`. Nothing relies on either direction.)

**Shared cells, read-after-write and write-after-write.** A cell two groups write is one
fold (write after write, in execution order). An operand cell that a listed group writes
and another reads is refused: the finite route sees the operand node differ and searches
a witness, the symbolic route fails OP2 (read after write, and write after read, since
OP2 is order-free). An operand cell written only after its last read is therefore
Verified at finite tiers and Unknown symbolically (`reads_first`), as for one group.

### The two facts

Facts, and no condition: the claim holds or fails whatever they say, and a fact's query
that fails changes the fact alone.

**Order (16b).** `groupOrder()` says whether every event of a listed group comes before
every event of each later-listed group. Finite: the machine counts each event's listed
group; at a point whose trace reached the exit, an event of a group after one of a
later-listed group refutes the fact with that point. The point's reference run completed
and the trace is one at a point (Theorem 9a), so every completing run there shows it.
Otherwise the fact is Proven at the claim's tier when the claim is Verified. Symbolic:
from every arrival at a site of the j-th listed group, every feasible way to the next
event of U (G3b's walker, pruned by the solver) arrives at a group listed no earlier
than j; by induction on the events the listed indices never fall. A path the walker
reports is feasible, so reaching an earlier group fails with no query of its own. A block
with sites of two listed groups holds two events that the walk takes for one, so it is
not judged (Unknown); the finite route refutes such an interleaving at the first point
where one block runs twice.

**Shared cells (16c).** `sharedCells()` says whether two listed groups ever write one
cell. Finite: Refuted at the first point with such a cell, otherwise Proven at the
claim's tier. Symbolic: Proven when, for each pair of stores of different groups on one
root, the two-copy query that they meet is unsat at one parameter point; any other
answer leaves it Unknown. The symbolic route never refutes a fact.

### What M3 verifies, refuses, leaves

The ten cases of the M3 brief, as `testGroupSets` and the oracle pin them:

| Case | Finite | Symbolic | Order fact | Shared fact |
| --- | --- | --- | --- | --- |
| 1. Disjoint cells, one root (`different_cells`) | Verified, scope | Verified | Proven | Proven, both routes |
| 2. Same cells (`same_cells`) | Verified, scope | Verified | Proven | Refuted with point; symbolic Unknown |
| 3. Second group reads the first's output (`reads_written`) | **Refuted, witness** | Unknown at OP2 | Unknown | Unknown |
| 4. The reads first (`reads_first`) | Verified, scope | Unknown at OP2 (order-free) | Proven | Proven |
| 4. Floats in both orders (`float_ab`, `float_ba`) | Verified, each | Verified, each | Per listing | Refuted; Unknown |
| 5. Wrong listing (`float_ba` listed ab, `same_cells` backwards) | Verified | Verified | **Refuted** (finite), Unknown (symbolic) | as above |
| 5. One block, two groups (`interleaved`) | Verified | Verified | Refuted at n = 2; symbolic Unknown | Refuted; Unknown |
| 6. Empty group (`empty_second`, `never`) | Verified, zero events listed | Verified | Proven | Proven |
| 6. A set of only the empty group | Unknown, no events | Unknown at NV | Unknown | Unknown |
| 7. Multiplicity 3 per cell (`repeat_shared`) | Verified | Verified | Proven | Refuted; Unknown |
| 8. Different operand roots (`same_cells`, `numeric`, `three_groups`) | Verified | Verified | Proven | Refuted; Unknown |
| 9. A writer left out (`three_groups` minus one) | **Refuted, witness** | Unknown at F1 | Unknown | Refuted; Unknown |
| 9. A group or site twice, part of a group, no group, 65 groups | Refused (Unknown) | Refused (Unknown) | Unknown, "refused" | Unknown |
| 9. Listing reversed | Same claim | Same claim | Changes | Same |
| 10. Solver: OP2 or F1 unknown or sat; limits; no certificate | Unknown | Unknown | Unknown | Unknown |
| 10. A fact's query unknown, or its pair limit | claim unchanged | claim unchanged | that fact Unknown | that fact Unknown |

The numeric example of the brief runs as written: with C = [0, 0], A = [1, 2], B = [10, 20]
and a unit factor, the trace is C[0] += 1, C[1] += 2, C[0] += 10, C[1] += 20 and C ends as
[11, 22]. The union of the two groups is Verified on both routes, each group alone is
Refuted with a witness, and the concrete run agrees with the union's replay.

**Remaining Unknown, by design.** An operand cell written after its last read (OP2 is
order-free). A shared-cells fact on the symbolic route when cells are shared, since the
symbolic route never refutes a fact. The order fact where one block holds sites of two
groups. Everything G4a leaves Unknown for one group stays Unknown for a set. A union with
one float site is `ORDERED_FLOAT`, whichever group comes first.

**Not exercised.** A symbolic refutation, which the symbolic route never gives, and a set
of more than 64 groups past its refusal.

**Not changed.** Theorem 13 and `checkReduction` still take one group, so a segmented
Theorem 13 over several groups is not claimed: `sib_roots` stays Refuted per group in G5a
and G5b, and no M2 result uses a set's claim. Connecting Theorem 15 over a union of
groups to Theorem 16 is the next step, and it changes G5a's public contract.

### How M3 is checked independently

`multigroup_oracle.py` is new and shares no code with the library or with the other
oracles. It generates sequences of nests over six roots, some reading another nest's
output, some guarded, empty or repeated, some with foreign stores. It asks the driver for
every set: all, all backwards, each alone, each leaving one out, a group twice and an
index that does not exist. It then recomputes from each run's source trace the merged
replay, the facts per point, and every witness. It checks that the listing changes no
claim and that a set of one group is that group's claim. It perturbs each verified fold
with a dropped, duplicated and swapped contribution, to show that the check can see
them. A run fails below its minimum counts (shared cells, cross-group reads, refuted
order facts, empty groups, three contributions, missing writers and others). It runs in
CTest as `generalized-gemm-multigroup-programs`.

### Changes to earlier phases (M3)

- `ComputationChecker.cpp` is refactored so that one implementation serves one group and
  a union. The group's own claims, bindings and JSON are unchanged, which is checked byte
  for byte against the M2 binaries over every computation and domain lit RUN line.
- `groupIsWhole` now refuses a group that names a site id the program does not have. Before,
  such a hand-made `SiteGroup` passed the check and was read out of bounds later. No
  enumeration produces one, and no existing output changes.
- `computation_mutation.py`: an anchor must now occur exactly once. The anchor of `sample
  called a scope` had been stale since G5a replaced the line it mutated, and is
  re-anchored to the current line; `frame hides observers` follows the new frame code. 34
  mutants are added for M3, 142 in all, and the `sets` section and the multi-group oracle join the
  tests every mutant runs.
- The G4a unit test gains the `sets` section; no earlier test or expectation changed.

**Found by the first validation run.** The oracle compared the claims of `all` and
`all-backwards` and found two programs (one in CTest's seed, one in the fresh seed) whose
symbolic claims were Unknown under both listings but named different failing OP2 queries:
the union's sites were in the listed order, and so was every obligation. No status or tier
differed. The union's sites are now in increasing order and F3 visits groups by their
first site, `two_overlaps` and `fresh_both` pin the reasons, records and contracts across
listings, the oracle compares obligations, contracts and points as well, and two mutants
(`union in the listed order`, `F3 in the listed order`) remove each ordering. The whole
validation was then repeated on the new tree.

## Milestone 4-0: one rule for groups and cells

M4-0 is the groundwork the M4 design review asked for before any set-level claim. It adds
no claim. G3b, G4a and G5a each kept their own whole-group check, and the copies had
drifted. The domain checker compared cells by their index alone. G3b promoted a finite
check with a point removed to the whole scope, and a refuted flat family read as if the
program had no decomposition. M4a to M4d, G6 and G7 are not started.

### Findings fixed

| | Before | Now |
| --- | --- | --- |
| F1 | The G3b and G5a gates did not bound site ids: a hand-made group naming a site past the program passed them and was read out of bounds. Only G4a's gate had the bound (M3). | One gate, `wholeGroup`, for all three. |
| F2 | G5a keyed output cells by index alone, finite and symbolic. Harmless within one group, which has one output root, and wrong for a set of groups. | A cell is a root and an index. Two roots that may alias are never called apart. |
| F3 | G3b's finite tier was the whole scope whenever the list was not marked as a sample: a list with a point removed was promoted. | `enumerationRanItsScope`: the scope the enumerator recorded, every listed point inside it, and as many distinct points, with those outside the domain, as it counted. |
| F4 | A refuted family carried no scope: a flat family failing over sibling loops read like a program with no decomposition. | `CheckedFamily::refutes()`: this family, or every family, or nothing. |
| F5 | G3b turned G3a's unknown into a refutation. | Unknown stays unknown. No enumerated domain reaches this case. |
| F6 | G3b's finite route indexed a point's domains by the group with no bound, and applied no gate. G4a's and G5a's finite routes do both. | The same bound, "no such group", and the same gate. |
| F7 | No single-group gate refused a site listed twice, and every checker walks the list. The set route already refused it. | The gate refuses it, in the set route's words. |

F1 to F5 are the review's findings. F6 and F7 were found while unifying the gates. No
group that the enumerator makes is affected by any of them: every earlier output is
byte-identical.

### The gate

`SiteSets.h` is the one definition. A group fails at the first of these, in this order,
with these words:

| Order | Condition | Words |
| --- | --- | --- |
| 1 | marked unsupported | the group's own words |
| 2 | no output root | `the group has no output in memory` |
| 3 | no site listed | `the group has no sites` |
| 4 | an id at or past the program's site count | `the group names a site the program does not have` |
| 5 | an id listed twice | `a site is listed twice` |
| 6 | not exactly the direct sites of its root triple | `the group is not the sites of its roots` |

A refused group gets these words from the symbolic routes of G3b, G4a and G5a, after
their checks of the program itself. Each finite route first says the gate of the
enumeration is closed, then `no such group` for an index past the groups, then the gate's
words. Only refusal text changes, never a status: G4a and G5a said "no output in memory"
for an empty list, and G3b gave an empty reason for a group without an output.

### Cells

| First | Second | Relation |
| --- | --- | --- |
| root r, index i | root r, index i | `SAME` |
| root r, index i | root r, index j ≠ i | `DISTINCT` |
| root r, one rank or width | root r, another | `MAY_ALIAS`, never `DISTINCT` |
| root r | root s ≠ r, either a fresh allocation | `DISTINCT`, a proof |
| root r | root s ≠ r, `roots min and max are separate` authorized | `DISTINCT`, an assumption that G4a records |
| root r | root s ≠ r, otherwise | `MAY_ALIAS` |
| a root the program does not have | anything | `MAY_ALIAS` |

Different root ids say nothing by themselves, and the name of an authorization is read
lower root first, as G4a writes it. `rootSeparation` is the function G4a's separations
now call. `sameCellFormula` is what the domain checker's symbolic routes ask. For one root
it is both events happening with equal indices. For roots apart it is false. For roots
that may alias it is both events happening, so a fact that rests on the cells being
apart stays unproven. The finite route keys a cell by `cellKey`, which is the root and
then the index.

### What a refutation refutes

| G3b outcome | `refutes()` |
| --- | --- |
| Verified, or Unknown | `NOTHING` |
| The family is malformed or reads memory, on either route | `THIS_FAMILY` |
| A coordinate has no value, or is undefined, at an event | `THIS_FAMILY` |
| G3a refutes the labels at a point, with no `output_read_differs` there | `THIS_FAMILY` |
| G3a reports `output_read_differs` at the failing point: an update reads a cell other than the one it writes | `EVERY_FAMILY` |

`EVERY_FAMILY` is the one refutation that holds for every coordinate family, since no
labelling changes which cell an update reads. A program whose flat family fails over
sibling loops stays open to a segmented claim: on `sib_same`, the flat family is refuted
for itself only, and G4a's claim, G5a's domain and the segmented Theorem 13 all stand.

### Proof levels

Point, ScopeEnumerated and the symbolic tiers mean what they did. G3b's ScopeEnumerated
now needs `enumerationRanItsScope` and no stopped point. G4a and G5a keep
`enumerationCoversScope`, which compares the list with the scope of the claim. A sample,
a list with a point removed, and a list with a point removed and another repeated are
Point on all three, in the unit test and in the oracle. Nothing is promoted to a tier
it did not reach.

### New public surface

`SiteSets.h` (`wholeGroup`, `RootSeparation`, `rootSeparation`, `CellRelation`,
`rootRelation`, `CellRef`, `relateCells`, `cellKey`, `sameCellFormula`), `Refutes` and
`CheckedFamily::refutes()`, `DomainEnumeration::scope` and `::scopePoints`,
`enumerationRanItsScope`, and the driver `generalized-gemm-identity-report`. The public
interface of `CoordinateFamily` does not change. No earlier driver's output changes.

### How M4-0 is checked independently

- **Unit.** The `identity` section of the domain test: the gate at site 0, at the last
  site and at the first id past them, for all three checkers and both routes; `C[0]`
  against `D[0]` with and without the authorization; a fresh root; the formula; the
  tiers under trimmed, repeated, sampled, moved and closed lists and a point outside the
  domain; the refutation scope on sibling loops, on C0, on a refused family and on an
  undefined coordinate.
- **Oracle.** `identity_oracle.py` shares no code with the library. It writes random
  programs of sibling nests over four argument roots and sometimes a fresh one, some
  updates reading another root or a shifted cell, and interprets them itself. It
  predicts the sites, the groups, every gate refusal with its words, every cell relation,
  and G3b's verdict on the empty family point by point, with its refutation scope, its
  failing point and its reason. It checks that a removed point demotes G3b and G4a. With
  every point run, it checks that G4a's tier agrees with G4a's own report of a point left
  open: that half is consistency, not prediction. It fails below minimum counts of each
  outcome, the closed gate among them. It runs in CTest as
  `generalized-gemm-identity-programs`, and with `--memory-limit-gb 0` against a
  sanitizer build, whose shadow memory a 4 GiB address-space limit refuses.
- **Mutation.** The domain harness runs the identity section first and the identity
  oracle last. It gains 41 mutants of the gate, the cells, the refutation scope and the
  recorded scope. Five are equivalent, with reasons in `domain_mutation.py`: four
  call sites of G5a's cells, where one group has one output root, and G3a's unknown, which
  no enumerated domain reaches. The six G3b and six G4a mutants whose code moved into
  `SiteSets.cpp` are re-anchored there with the same edit, and one is added for G3b's
  call of the gate.

### What stays Unknown

- Two argument roots with no authorization may alias. Every fact that needs their cells
  apart stays Unknown.
- G3b's symbolic route refutes only a refused family. A failed obligation there stays
  Unknown, as before.
- A group refused by the gate is Unknown on every route, whatever its sites compute.

### Not done

Segments that belong to one group (Theorem 18), coordinates validated per segment,
coverage across segments from Theorem 15, and a set-level verdict kept apart from the
one-group verdict are M4a to M4d. None is started.

## Milestone 4a: one domain claim over a union of groups

M4a is the first of M4's milestones, and the only one started. It adds one claim: the
events of several groups, as one segmented domain. M3 already claims their values
together (G4a over a set); M4a claims their events, their order and their cells. It claims
no fold over the set (Theorem 17, M4b), no coordinates per segment (Theorem 18, M4c) and
nothing about whether the segments compose into one GEMM (G6).

### Contract

`DomainChecker.h` gains `checkSetDomainFinite` (groups by their index in an enumeration)
and `checkSetDomainSymbolic` (the groups themselves), which return a `CheckedSetDomain`.
It is a class of its own, so no set claim can stand where a one-group claim is expected,
and its binding (`…|set|…`) is no one-group binding. Its parts:

| Part | Meaning |
| --- | --- |
| `status()`, `tier()`, `reason()` | Theorem 15 over the union, at Point or ScopeEnumerated (finite) or Symbolic |
| `groups()`, `segmentGroups()` | The listed groups by their first site, and the group of each segment |
| `segments()` | Each segment's key, collision, access and multiplicity facts |
| `sharedCells()`, `singleContribution()` | Root-aware: Proven, Refuted at a point on one root, or Unknown |
| `rootPairs()` | Every two output roots of the segments: `DISTINCT` by fresh allocation (a proof) or by name (an assumption), or `MAY_ALIAS` |
| `points()`, `records()`, `refutation()`, `failingPoint()` | As for one group. `points()` counts cells by root and index: for roots that may alias it counts keys, not memory cells, and no fact rests on it. A refutation is of this request's Theorem 15 at a point, never a statement that no segmentation exists |

### Theorem 15 over a union

The union's sites are the event source. Nothing about events changes: the segments must
partition the sites, and D1' to D5 are asked of the arrivals at all of them, exactly as M2
asks them of one group's. Theorem 15 never reads a root, so it holds of a union on the same
proof. The finite route reads each listed point's run and checks the same statement: the
union's events never return to an earlier segment, and each segment's events in order are
its set S_s(v) in order. The one-group routes are now these routes with one group's sites
and no set, and every one-group output is unchanged byte for byte.

Three checks come before any obligation:

| | Check | Refusal |
| --- | --- | --- |
| XS | `uniteGroups`, which G4a's set route now shares: 1 to 64 whole groups, no triple twice, no site twice | the gate's words |
| XP | The segments partition the union's sites, and each segment's sites belong to one group | `segment s mixes groups`, and M2's partition words with "set" for "group" |
| CI | Cells of one root compare by index. Cells of two roots compare only through their relation | none: the facts that need them apart stay Unknown |

On the symbolic route a pair of events on two roots never reaches a query: apart, they
share no cell; may alias, the facts stay open. The finite route keys cells by root and
index, which separates two roots only when they are apart, so the same rule leaves its
facts open for roots that may alias. A shared cell on one root is refuted at a point on
either kind of union, since it needs no separation.

### The seven obligations of the brief

| Obligation | Where it is checked |
| --- | --- |
| 1. Site-set validity | XS, and XP for the segments |
| 2. Exact event coverage | Theorem 15: each segment's events are exactly its set, the segments cover the union's sites, and each point's event count is the union's |
| 3. Segmented partition | XP: every site in exactly one segment, every segment in one group |
| 4. Order preservation | Theorem 15: D2 and D3 within a segment, D4 and D5 between segments, never back to an earlier one |
| 5. Root-aware cell identity | CI |
| 6. Scope soundness | ScopeEnumerated only with every point of the claim's scope (`enumerationCoversScope`) and no stopped point; Symbolic only with every obligation unsat |
| 7. Proof composition | Verified only on the full path of one route; a fact never raises the claim, and the finite and symbolic claims are never joined |

### The brief's two cases

Case A is `sib_roots`: two groups, C[i] += A[i]·B[i] and C[i] += A[i]·D[i]. Over n = 2 the
union has exactly four events, two per group in program order. Both routes verify the
union. The finite route refutes "no cell of two segments" at a point, and the symbolic
route leaves it Unknown, as it leaves every fact it cannot prove: the events are disjoint
and the cells are not.

Case B writes C in the first group and D in the second. With `roots 2 and 3 are separate`
authorized, no cell is common on either route, and no query is asked for it. With no name,
or with the reversed one, both facts are Unknown, saying the two roots may alias, while the
claim about events still stands. A fresh allocation in place of D is apart with no name, as
a proof.

### What stays Unknown or is refused

- Two sites of the union in one block, of one group or two: the symbolic route has no order
  for them. The finite route refutes an interleaving where a run shows one.
- Cells of two roots that may alias, for every fact that needs them apart.
- A set that is not whole groups, a segment that mixes groups, a flat request over a set,
  and everything a one-group segmented claim leaves Unknown.
- The fold over the union (M4b), per-segment coordinates (M4c), the candidate verdict and a
  proposer over unions (M4d), and G6 and G7.

### How M4a is checked independently

- **Unit.** The `setdomain` section of the domain test runs both cases, possibly aliasing
  and fresh roots, interleaving, the segments backwards, a set that misses events and one
  with points that have none, every refusal, empty segments and groups, a sample and a
  trimmed list, every obligation answered sat and unknown, both limits, listing in either
  order, and a set of one group against that group's own claim.
- **Oracle.** `setdomain_oracle.py` shares no code with the library. It writes programs of
  nests over several root triples and interprets them itself. For each union and each
  group alone, with every name authorized and with none, it checks the partition, Theorem
  15 at each listed point (finite) or every point (symbolic), each point's event count,
  every refutation's point, the root pairs, and the cell facts against cells of one root.
  It checks that the listing changes nothing, that a set of one group is the group's own
  claim, and that interleaved groups are never verified. It runs in CTest as
  `generalized-gemm-setdomain-programs`.
- **Driver.** `generalized-gemm-domain-report --groups=…` asks for sets, with `all` and
  `all-reversed`, and prints `group_sets` only then. The request for a set joins each
  group's own segmented proposal in program order: an untrusted proposal for the checker,
  not the proposer over unions, which is M4d's.

### Changes to earlier phases (M4a)

- `SiteSets.h` gains `uniteGroups`, and G4a's set route calls it: the same checks, in the
  same order, in the same words. M3's results are unchanged, and the six G4a mutants of
  that code are re-anchored on it with the same edits.
- The domain checker's segmented routes run over a list of sites. One-group claims, their
  bindings and their JSON are unchanged.
- The domain test gains the `setdomain` section. The domain harness runs it second, and the
  set oracle last; it gains 16 mutants. One M4-0 equivalence, `domain segment cells by index
  only`, is no longer equivalent, since a union holds two roots, and is run as a mutant.

**Found on the way, not changed.** The untrusted flat proposer aborts a debug build on some
programs: `conditionAtoms` builds a one-bit constant from the value 2. It is reached on the
default path of the domain driver, with no `--segments`, for `sib_roots` at width 16 and
scope 1 to 3, and on M4-0 alike. No claim depends on it, since a proposal is judged anyway,
and the set route uses the segmented proposer.

## Validation of M4a (2026-11-11, LLVM 24.0.0git, Z3 4.8.12, base 5e08faa)

Every result below is from one frozen tree (hash `9d1bed38…` over `include/`, `lib/` and
`research/generalized-gemm`), unchanged from the start of the run to its end, with the
suites run one at a time and every subprocess under a limit. The M4-0 binaries it is
compared with were built from a snapshot of the commit, not from this worktree.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 21 of 21 CTest suites, the new `setdomain-programs` among them |
| G5a/G5b unit, core | 1,037 checks, 0 failures: `setdomain` 120, and every M4-0 section's count unchanged |
| G4a, G3b and G3a units | 3,658, 517 and 355 checks, 0 failures, as at M4-0, section by section for G4a |
| Set oracle, seed 20261113 | 60 programs, 0 failures: 480 finite and 460 symbolic claims verified and 2,296 points checked against Theorem 15; 36 finite refutations replayed, all of them interleaved unions; 120 listings compared; 308 one-group sets equal to the group's own claim; shared cells refuted on one root 194 times and proven across roots apart 36 times, and Unknown for roots that may alias 20 times; 90 segments empty at a point |
| Identity, domain, G4a and multi-group oracles, seed 20261113 | 0 failures, 0 disagreements |
| Byte-identical against M4-0 | G1 42, G2a 14, G3a 14, G3b 14 outputs; G4a 18 functions; 19 computation and domain RUN lines, G4a's set lines among them; 6 identity-driver outputs: 0 differ |
| Lit | `dom_sets.mlir`: both cases through `--groups` |
| ASan, UBSan, leaks | Clean: domain core 1,037 and G4a 3,658 checks, 8 set-oracle and 12 identity-oracle programs against the sanitized drivers |
| Mutation, domain harness, all | 183 mutants: 169 killed by a test's own assertion (every M4a mutant by `setdomain`), 14 equivalent with reasons, 0 survived, 0 timeouts, 0 crashes |
| Mutation, G4a harness | The mutants of `ComputationChecker.cpp` and `SiteSets.cpp`: 103 killed, 6 equivalent, 0 survived, 0 timeouts, 0 crashes |
| Mutation, G3b, strict | The mutants of `SiteSets.cpp`: 4 killed |
| Not rerun | The mutants of files M4a did not change: 33 in the G4a harness, 110 in the G3b harness, the 88 of the G3a harness. The four G3b and G3a mutants left open at M4-0 stay open |

## Validation of M4-0 (2026-11-10, LLVM 24.0.0git, Z3 4.8.12, base 7bf6534)

Every result below is from one frozen tree (hash `1846937d…` over `include/`, `lib/` and
`research/generalized-gemm`), unchanged from the start of the run to its end, with the
suites run one at a time and every subprocess under a limit. The last mutation row is the
final tree (`3f093064…`), which adds one equivalence to `computation_mutation.py` and
nothing else. A first run, on the tree before the oracle fixes of this milestone, stopped
at the domain mutation baseline: the identity oracle expected G4a's whole-scope tier
whenever G4a verified with every point, and G4a demotes to Point when a replay stays open.
The oracle was fixed and checked on eight seeds, and the whole run repeated.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 20 of 20 CTest suites, the new `identity-programs` among them |
| G5a/G5b unit, core | 917 checks, 0 failures: `identity` 208, and every earlier section's count as at M3 (709) |
| G4a, G3b and G3a units | 3,658, 517 and 355 checks, 0 failures: the counts the M3 binaries give, section by section for G4a |
| Identity oracle, seed 20261112 | 120 programs, 0 failures: 1,857 whole groups and 8,031 refusals word for word, 5,163 cell relations (`C[0]` against `D[0]`: 263 apart when authorized, 60 may alias when not), the empty family 176 times whole scope, 107 times Point with a point removed, 128 times refuting every family and 266 times this family only, 57 flat families refuted over sibling loops, 45 closed gates |
| Domain, G4a and multi-group oracles, seed 20261112 | 0 disagreements each: 114 domain specs, 60 + 20 G4a programs, 24 + 14 multi-group programs |
| Byte-identical against the M3 binaries | G1 42, G2a 14, G3a 14, G3b 14 outputs, G4a 18 functions, 18 computation and domain lit RUN lines |
| ASan, UBSan, leaks | Clean: domain core 917, G4a 3,658 and G3b 517 checks, and 12 identity oracle programs against the sanitized driver. G3b's finite route without its group bound stops with an AddressSanitizer report |
| Mutation, domain harness | 167 mutants: 152 killed by a test's own assertion (41 by `identity`), 15 equivalent with reasons, 0 open |
| Mutation, G3b and G3a, strict | The mutants of `SymbolicCandidateChecker.cpp`, `SiteSets.cpp` and `FiniteDomain.cpp`: 68 killed of 72, **4 open**. `no order accepted`, `bound may read inner levels` and `memory term adopted` end in a segfault inside the mutated code with no assertion first. `assumptions not passed to the check` fails two assertions and the test then crashes on an unguarded index. All four are guards older than M4-0, on lines it did not change; three sit inside functions it did change (`checkFamilySymbolic` twice, `enumerateDomain` once). The harnesses' own runners count a crash as a kill; the strict runner never does, and they stay open |
| Mutation, G4a harness, final tree | The mutants of `ComputationChecker.cpp` and `SiteSets.cpp`: 103 killed, 6 equivalent, 0 open. `set lists a site twice` is equivalent since M4-0: the gate refuses a site twice first, in the same words |

## Validation of M3 (2026-11-08, LLVM 24.0.0git, Z3 4.8.12, base 8d6c8d0)

Every result below is from one frozen tree (hash `dea9972f…` over `include/`, `lib/` and
`research/generalized-gemm`), unchanged from the start of the run to its end, with the
suites run one at a time and every subprocess under a limit. The first run of the same
script, on the tree before the listing fix above, failed only the multi-group oracle; the
whole run was repeated after the fix.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 19 of 19 CTest suites, the new `multigroup-programs` among them |
| G4a unit, every section | 3,658 checks, 0 failures; `sets` 775 of them; no earlier section's count changed |
| G5a/G5b unit, core | 709 checks, 0 failures, as at M2 |
| Multi-group oracle, seed 20261111 | 94 programs and 1,200 set claims: 468 Verified (254 finite at scope, 214 symbolic), 343 Refuted with the witness replayed (119 with a cross-group read), 389 Unknown; 704 points' facts matched exactly; 188 listings and 253 one-group sets compared; 106 sets that leave out a writer of their cells, none verified; perturbations seen: 4,492 drops, 4,385 duplicates, 95 float swaps; 0 disagreements, 0 timeouts |
| G4a oracle, seed 20261111 | 140 programs, 207 Verified and 46 Refuted claims, 46 witnesses replayed, 0 disagreements |
| Mutation, G4a harness | 142 mutants: 137 killed by a test's own assertion (all 34 new ones by `sets`), 5 equivalent (G4a's, unchanged), 0 survived, 0 timeouts, 0 resource limits, 0 harness errors |
| ASan, UBSan, leaks | Clean: G4a unit 3,658 checks and 6 multi-group oracle programs |
| Byte-identical | G1 42, G2a 14, G3a 14, G3b 14 outputs against 2542b68; G4a 18 against 4ecb778 and 18 against 8d6c8d0; 18 computation and domain lit RUN lines against 8d6c8d0 |
| Not re-run | The domain mutation suite: no domain source, domain test or domain output changed, and the domain unit core and every domain lit RUN line give the M2 results |

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

**Proposer fix (2026-11-11).** The flat domain proposer's congruence atoms, y mod 2, 3
and 4 for each order level, were built for every level whatever its width. The coordinate
search can propose a one-bit comparison term as a coordinate, and the proposer then built
`APInt(1, 2)`, which aborted a debug build before any report was printed (a release build
would have built a bit pattern wider than its sort instead). A modulus the level's width
cannot hold is now skipped, never truncated: a one-bit level gets no congruence, a two-bit
level gets 2 and 3, three bits and up get all three. `proposalConditionAtoms` exposes the
atoms to tests, untrusted like the rest of the proposer; no checker changed. The `proposer`
section of the domain test and `dom_congruence.mlir` pin widths 1, 2, 8 and 32, the
program that aborted, and a stepped loop whose proposal needs the congruence. The record
is in `integration-backups-20261111/conditionatoms-crash/`.

M4-0 changes no earlier assertion or expected output, and no output of an earlier driver.
It changes refusal text only, for groups the enumerator never makes, as the gate table
above lists. G3b's ScopeEnumerated needs every point of the recorded scope where it needed
only an unsampled list. The domain test gains the `identity` section, first in its order.
The synthesis and computation mutation harnesses re-anchor the mutants whose code moved
into `SiteSets.cpp`, with the same edits, and the synthesis harness gains one.
