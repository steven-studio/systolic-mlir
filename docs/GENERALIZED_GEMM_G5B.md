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
| M2 | Segmented families and orders, Theorem 15, in G5a's checker; G3b's V1 and V2 over per-segment terms (`checkFamilySymbolic`) | **Yes:** `CoordinateFamily` gains per-site terms, `DomainRequest` gains segments; G3b's SymbolicValid obligations are restated over them |
| M3 | Multi-group replay in G4a for `sib_roots` | **Yes:** `CheckedComputation` over a set of groups |
| M4 | Proposer: segment detection by site location, per-segment fitting | None |

M2 and M3 change public proof contracts and are not started. The decision they need is
below.

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
