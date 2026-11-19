# Generalized GEMM recognition: Phase G4a, computation verification

G4a establishes what a program computes, for

    C[h(x)] += A[f(x)] B[g(x)]

G3b proved properties of coordinates, and no checker before this phase read a value.
G4a compares each value with a stated relation. It is research-only: nothing here is
linked into production lowering, and no result can authorize one. The earlier records
are `docs/GENERALIZED_GEMM_G0_G1.md`, `_G2A.md`, `_G3A.md` and `_G3B.md`.

Decisions recorded on 2026-11-02:

1. The claim is the replay relation over the run's own ordered events. It needs no
   coordinates and no coverage, so it is proved once per group and keeps every
   alternative candidate.
2. Output initialization is handled at the finite tiers only. Proving "before every
   event on that cell" over a whole scope is order evidence, which G5 owns. So
   `C[i,j] = 0` before the k loop stays Unknown at the symbolic tier.
3. Disjointness in OP2 is order-free. An overlap that reads before it writes gives
   Unknown at the symbolic tier, even where the program is correct.
4. Other writes proven disjoint go into a frame, and do not block the claim. Two
   groups that overlap on output cells give Unknown, even for integers.
5. Herbrand replay is the finite proof. Fills are used only to look for a witness.
6. Different root ids imply nothing. Separation comes from a fresh allocation or from
   a name somebody authorized, and `llvm.noalias` needs the name too, since it is a
   promise and no proof. Every use stays in the result.

## The four levels

| Level | Meaning | Here |
| --- | --- | --- |
| A | Valid coordinate decomposition | Re-established by calling the G3 checkers |
| B | Valid computation decomposition | Proved in the fragment below |
| C | Exact domain and reduction order | Evidence passed on, `domain_and_order` always UNKNOWN |
| D | Semantics-preserving transformation | `transformation` always UNKNOWN, `authorizes_lowering` false |

## The relation

Fix a site group G with roots (C, A, B), parameters v in the scope, and any initial
memory M₀ whose run completes. Let E be that run's ordered events, repeats included.

    Replay:  Mem := M₀, with each prefix constant in place
             for e in E, in order:
               Mem[C][h(e)] := Mem[C][h(e)] ⊕ₑ ( M₀[A][f(e)] ⊗ₑ M₀[B][g(e)] )

⊕ₑ and ⊗ₑ are the exact operations of e's site, with their flags and operand order.

**Claim B, for the group.**
1. Each operand read of each event is the initial value of its cell.
2. Each event writes Replay's next value, so every intermediate value agrees.
3. The final value of each cell the group writes is Replay's value.
4. Every other memory effect is listed in the frame.

Per cell, for a cell c:

    C_final[c] = foldl ⊕ C_init[c] [ a(e) ⊗ b(e) : e ∈ E, h(e) = c, in execution order ]

**Claim B, for a candidate** adds A: h(e) = H(P,I,J), f(e) = F(P,I,K), g(e) = G(P,K,J) on
the events. H, F, G exist as functions on the image. No closed form is claimed.

**Permitted initial memories.** Every assignment of values to the cells of argument
roots whose run completes. A cell of a fresh allocation has no value, and a read of one
stops the run.

**Not claimed.** Runs that stop are outside the relation, and whether another program
stops on the same memories is G7's. Intermediate memory is not observed. Floats are
strict as written; a source flag such as `contract` allows more behaviours, and this is
one of them, so the flags are reported and never used.

**Logical and physical.** A physical cell here is the model's (root, index tuple).
Byte addresses are outside the model, which is why G1 refuses views and non-identity
layouts. Y[p,i,j] is the list of contributions with that output coordinate, and a cell's
list is the interleaving, in execution order, of Y[l] over H⁻¹(c). A non-injective H is
valid: it means interleaving, not concatenation.

**Integers (Theorem 10).** For integer sites, in a completing run,

    C_final[c] = C_init[c] + Σ a(e)·b(e) mod 2ʷ

in any order. It concerns values only. `nsw` makes a reordered sum stop on other
memories, so it permits no reordering. For floats there is no such statement, whatever
the flags say: `fold` is `ORDERED_FLOAT`, and reassociation is never assumed.

## Assumptions

Each is stated in the result or here, and none is silent.

1. The G1 importer is faithful, and Z3's unsat answers are correct.
2. Roots are separate: fresh allocations by construction, others by an authorized name.
   The requirement covers every written root against every touched root, not only the
   group's: a value can pass through a third root and come back into the output.
3. Final memory, returns and stop status are the observables. There is no concurrency.
4. `holds_for` says "every run" only with `PROVEN_TERMINATING` and no memory-dependent
   stop. Otherwise it says "runs that complete", which is every float program.
5. Theorem 9a: an oblivious program has one transition sequence at a point, for every
   memory on which it completes. Obliviousness is computed by G1 and gated here.

## Evidence tiers

A separate scale from the coordinate tiers.

| Tier | Claim | Proof | Issued by |
| --- | --- | --- | --- |
| Proposed | None | None | |
| Point | All M₀, at each listed point | Herbrand replay | `checkComputationFinite` |
| ScopeEnumerated | All M₀, at every point of a finite scope | The same, no sample, no open point | `checkComputationFinite` |
| Symbolic | All M₀, the whole scope | E, OP, R, W, F below | `checkComputationSymbolic` |

Neither route follows from the other, and a random fill is evidence of nothing. A
candidate's tier is the least of its coordinate and computation tiers, at the points
they share. A solver's unknown, a limit, an unsupported form and a mismatch of nodes
never raise a tier. **Refuted needs a concrete run**, replayed, at which the program and
the relation differ. It refutes this relation in this fragment and says nothing about
any other decomposition or about whether the code is correct.

## Obligations

| ID | Category | Obligation | Method |
| --- | --- | --- | --- |
| P | Parameter scope | Program supported, trace fixed, certificate re-checked here, one scope throughout | Gates |
| AL | Aliasing | Every written root against every root the program touches: fresh, or an authorized name | Gate, recorded |
| E0 | Event correspondence | All transitions of a block list the same effects | Structural |
| E1 | Event correspondence | The site's store is a write of its root at its index | Structural |
| E2 | Multiplicity | Each site owns its store, and no other site shares it | Structural |
| OP1 | Operand access | The operand values are loads of the recorded accesses, of the memory the listed writes leave | Structural |
| OP2 | Operand access | No write of the program reaches an operand cell | Two-copy QF_BV |
| R1 | Reduction | The stored value is exactly acc ⊕ (a ⊗ b), with flags and operand order read back from the terms | Structural |
| W1 | Output consistency | The update reads, before its store, the cell it writes | Structural, QF_BV |
| W2 | Output consistency | No write between that load and the store reaches the cell, or the read is stale | Structural, QF_BV |
| F1 | Completeness | No other write reaches a cell of the group | Two-copy QF_BV |
| F2 | Completeness | Returns are independent of memory | Structural |
| F3 | Definedness | The group's roots have initial cells. Otherwise the claim could be vacuous | Structural |
| NV | Definedness | A point of the scope has a completing run with an event, found by exact replay | Concrete |
| Z | Definedness | Stops are reported, with those that depend on memory named | Report |

Facts, and no condition of validity: symbolic injectivity of H, F, G, the frame, and the
number of output, prefix and residual cells.

Floats never reach the solver: R1 and OP1 are identity of terms. No claim rests on the
SMT float theory.

## Theorems

**Theorem 9a (one trace).** If no guard and no address depends on memory, then at fixed
parameters all completing runs take one transition sequence.

*Proof.* Induction on steps: the next location and the next memory-independent state
are functions of those at this step, and of the parameters. Memory-dependent state reaches
no guard and no address, by the taint fixpoint, which also covers poison shadows. ∎

**Theorem 9 (replay, symbolic).** If P, AL, E, OP, R, W and F hold, then for every v in
the scope and every M₀ whose run completes, Claim B holds.

*Proof.* Induct over the ordered writes of the run. By F1, only the group's stores
write the group's cells. By OP2, no write reaches an operand cell, so with OP1 each operand
load returns its M₀ value, through any store chain. By W1 and W2 the accumulator read is
the cell's current value. R1 makes each store exactly one Replay step, and E gives one
event per store. Each query takes two independent points of D̂ at one parameter, and D̂
contains every state at which an event of a completing run happens, so the disjointness
covers every pair of events of one run and each event with itself. D̂ drops conditions
that read memory, which only weakens a premise. ∎

**Theorem 11 (Herbrand replay).** At one point of an oblivious program, if the replay
outcome is Equal, then Claim B holds for every M₀ whose run completes.

*Proof.* By 9a, that run takes the transition sequence the machine followed. A side
condition that depends on memory is false in a run that completes, and is taken as false.
Let the leaves stand for the cells of M₀. Each node is one operation applied to its
children, keyed by operation, flags, width and operand order, and nothing is simplified.
So by induction every node has one value under any M₀, whatever the operations mean, and
equal nodes have equal values. The machine compares nodes at each event and final cell. ∎

The converse is false: `x * 1` and `x` are different nodes. So a mismatch means only
that this argument failed, and **the result is Unknown unless a concrete run differs**.
The run carries on past a mismatch, because a witness needs every prefix constant.

**Prefix.** A constant written to a cell before its first event starts that cell's
fold. Any other value does not, so the fold starts from the initial value and the next
comparison shows the difference. After the first event the replay does not change.

## Architecture

| Part | Trusted | Role |
| --- | --- | --- |
| `ComputationModel.h` | Yes | Claims and their parts |
| `HerbrandReplay` | Yes | Uninterpreted execution. The program side reads transitions alone, and only the replay side reads site records |
| `ComputationChecker` | Yes | Sole authority. Links neither the grammar nor the search |
| `ComputationAnalysis` | No | Pipeline: search, check, JSON |
| `reachLocations` | Yes | Arrivals at any block. Shares its walk with `reachSites`, which is unchanged |
| `provenPremises` | Yes | The G3b premises, exported with no change to them |

`CheckedComputation` has no public constructor for a verified result. A
`CheckedFamily` names no program, family or group, so G4a runs the G3 checkers itself.

## Fragment

Direct sites of an oblivious program. Scalar accumulators stay Unsupported, so the
common `s = 0; for k: s += a*b; C = s` form gives Unsupported. Loads and the multiply
must be in one block, so a hoisted load is a foreign store and gives Unknown.

## Known limits

- Symbolic Unknown where finite tiers may succeed: output initialization, reads before
  writes, an intermediate write in the block (`stale` in the tests) even when it is
  overwritten, and every group whose D̂ query is spurious.
- Two groups writing one cell give Unknown, even when the integers would commute.
- A return that reads memory gives Unknown, even when it reads something unrelated.
- A semantically equal program with different nodes gives Unknown: `x * 1`.
- NV looks at 16 points of the scope. A scope whose events are all elsewhere gives
  Unknown, which is a cost and no error.
- A program whose reference run stops, such as a division by a loaded value at fill 0,
  gives no finite claim, though its symbolic claim may stand: the enumerator runs one
  fill to decide that a point has a run.
- Sites of a group that no run reaches are not reported. NV asks for one event of the
  group, not one per site.
- `llvm.noalias` is treated more strictly here than in G1, deliberately. G1's
  `rootPairs` are unchanged.
- The in-process boundary of G3a still applies: a caller could drop points from the
  enumeration it passes. A claim is bound to its program, group, scope and
  authorizations, and a candidate joins no other claim.

## Changes to earlier phases

Additive, with default output unchanged: `reachLocations` shares a walker with
`reachSites`, and `provenPremises` exposes the premises of `Symbolic`. No G1 or G2a
source changed, and no G3a source changed.

## Validation (2026-11-03, LLVM 24.0.0git, Z3 4.8.12, base 2542b68)

Every result below comes from one frozen tree, and that tree was unchanged from the start
of the run to its end. The suites ran one at a time.

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 15 of 15 CTest suites, G1 to G4a and 16 lit tests |
| G4a unit | 2,883 checks, 0 failures |
| Exhaustive, CTest | 812 i8 and 77 f32 programs, 240,128 fills: 0 false Verified, 0 false Refuted, 0 Unknown where a fill differs |
| Exhaustive, full | 1,405 i8 programs (lengths 1 and 2, and one in 37 of length 3) and 812 f32 programs (lengths 1 and 2), 651,520 fills: 0 false Verified, 0 false Refuted. 10 Unknown where no fill differs: incompleteness, and no error |
| Oracle, seed 20261103 | 220 programs, 47,982 completing and 8,778 stopped runs, 325 Verified and 76 Refuted claims, 76 witnesses replayed, 20 hand expectations, 0 disagreements, 0 timeouts |
| Mutation | 108 mutants: 103 killed by a test's own assertion, 5 equivalent with evidence, 0 survived, 0 timeouts, 0 resource limits, 0 harness errors. One worker, a 6 GB cap per process |
| ASan, UBSan, leaks | Clean: G4a unit 2,883, G3b unit 517, G3a unit 355, and 16 oracle programs |
| Byte-identical to 2542b68 | G1 42 outputs, G2a 14, G3a 14, G3b 14, over the lit inputs |
| Solver limits | Each takes effect alone: a timeout ends a hard query in 0.21 s, and a resource limit in 0.02 s |
| G1 to G3a sources | Unchanged. G3b gained `reachLocations` and `provenPremises`, additively |
| Polynomial suites | 13 of 13 |
| `check-systolic` | 188 of 190: the two known legacy failures, `general_affine_domains.mlir` and `general_affine.c` |

The equivalent mutants are listed in `computation_mutation.py` with their reasons:
`output not separated`, `operands not compared`, `width not in a node`, `finite candidate
unchecked` and `short key passes`. Two kills pin conservative choices that remain sound
without them. These are `flags not in a node` and `operands sorted`: with them, equal values
reached by a different operation would be Verified, while the relation promises the exact
operation.

**Defects found before this record, all fixed and covered by tests.** An independent
review found:
- D1, a false Verified: a value went through a third root, and the pair needed no
  separation.
- D2, the finite route did not check a site's effect kind.
- D3, a false Refuted: a prefix and a site in one block.
- D4, a false Refuted: equal values in different cells.
- D5, vacuous symbolic claims, now NV.
- D6, a crash and a key collision on wide constants.

The exhaustive test found D3 independently. Mutation testing then showed that NV masked the
query obligations, so programs that go wrong only at i == 20 now test each of them.

Two interruptions came from memory pressure on the machine. A process of another session,
running with no memory limit, reached 24.45 GB. The harness now caps each process and its
descendants, and kills a timed-out test's whole process group.

## Trusted base

Everything in the G1 to G3b records, including Z3's unsat answers. The Python oracle tries
to break claims with fills and does not prove them.

## Build and test

```sh
ninja -C <build> check-generalized-gemm
COMPUTATION_FULL=1 <build>/generalized-gemm-computation-test exhaustive
python3 research/generalized-gemm/test/fuzz/computation_mutation.py --source . \
  --scratch <new dir> --llvm-dir $L/lib/cmake/llvm --mlir-dir $L/lib/cmake/mlir
```
