# Generalized GEMM recognition: Phase G3b, coordinate synthesis

G3b synthesizes coordinates (P, I, J, K) as terms, for programs that canonical
recovery cannot split and across symbolic parameters. A bounded enumerator proposes
atoms, a SAT selector assigns them to axes, and independent checkers decide what the
evidence supports. It is research-only: no G3b result proves GEMM equivalence or
authorizes a transformation. The earlier records are `docs/GENERALIZED_GEMM_G0_G1.md`,
`docs/GENERALIZED_GEMM_G2A.md` and `docs/GENERALIZED_GEMM_G3A.md`.

Decisions recorded on 2026-11-01:

1. Live-state coordinates through a default-off option on the G3a enumerator. A
   coordinate need not appear in any address.
2. The existing `SmtBackend`, with our own bounded enumerator and SAT selectors. No
   external SyGuS solver is needed.
3. Theorem 8, within its fragment. `SymbolicValid` never rises to `SymbolicExact`
   without the coverage obligations.
4. The best candidate and up to four alternatives with distinct structures.
   Preference orders the search only, and never affects soundness.
5. Scalar accumulators stay Unsupported until their flush is represented.
6. `structurally_full`, beside `genuine`, whose meaning is unchanged. A decomposition
   whose view ignores an axis can be full and valid with `genuine` false.

## The problem

Given a program, a site group, a scope S at fixed widths, a grammar and a G2a
certificate, find tuples of terms P, I, J, K over live integer state and parameters
such that, for every v in S:

- the three dependences hold on D(v);
- φ = (P,I,J,K) is a bijection from the events of D(v) onto a declared set B(v);
- B(v) is a batched box, or is reported as partial.

H, F and G need not be injective. This is the difference from G3a. Dependence alone
is trivial, and so is the whole problem: P = x works for any program. So the output is
a verified candidate at a stated tier, with a preference that has no effect on
soundness. Theorem 5 of G3a makes this unavoidable: inequivalent answers can all be
valid.

## Evidence tiers

| Tier | Claim | Obligations | Issued by |
| --- | --- | --- | --- |
| Proposed | None | None | The search |
| Point | Exact at each listed point | G3a checker on exact domains, labels defined at every event | `checkFamilyFinite` |
| ScopeEnumerated | The same, at every point of a finite scope | The same, with every point of the scope the enumeration recorded (`enumerationRanItsScope`, G5b-M4-0) and no stopped point | `checkFamilyFinite` |
| SymbolicValid | For every point of S: V0 to V4. No coverage | SMT on an over-approximation | `checkFamilySymbolic` |
| SymbolicExact | SymbolicValid or its bijection by S, and events are the orbit of the order | S1 to S3 as well | `checkFamilySymbolic` |

A finite tier and a symbolic tier are separate evidence, and neither follows from the
other. A sample is a sample. An unknown answer, a limit and an unsupported form never
raise a tier. `equivalence` is always Unknown, and the JSON carries
`"authorizes_lowering": false`.

**What a symbolic tier quantifies over.** `holds_for` says it. With a certificate of
`PROVEN_TERMINATING` it is every run of every point of the scope. With
`PROVEN_TERMINATING_IF_DEFINED` it is the runs that complete, as G1 defines D: a point
whose every run stops on undefined behaviour satisfies the claim vacuously, so the stops
must be read with the tier. Requiring the stronger verdict would exclude every float
GEMM, since NaN stops are Underspecified.

`SymbolicValid` needs V2. If V2 fails and coverage holds, the result is `SymbolicExact`,
because Theorem 8 gives the bijection, and the record shows V2 as undischarged. If both
fail, the result is Proposed and Unknown.

## Obligations

Each is a query that must be unsat, in plain QF_BV. A failure on the over-approximation
is a counterexample only once replayed, so its parameters become a sample and the
search or the order is tried again.

| ID | Obligation |
| --- | --- |
| V0 | Each coordinate is defined wherever an event can happen |
| V1h, V1f, V1g | Equal (P,I,J) force equal h, and likewise f and g: two copies of the state, one of the parameters |
| V2 | Equal coordinates force the same event, by Theorem 7 |
| V3 | The coordinates stay in the order's set |
| V4 | The update reads the cell it writes, at every event (C0) |
| I | A definition fact holds on every path into its cut (O3 and O4) |
| S1, S2, S3 | Theorem 8 |

`checkFamilySymbolic` also re-checks the certificate for this fingerprint and scope,
requires an oblivious program, requires that the group be every direct site of its
roots, and requires that the order's levels be exactly the coordinates. Bounds read
parameters only, and `upperOuter` reads an earlier level only. A coordinate may read
only parameters and state independent of memory: the symbolic queries share one memory
between their two copies, so a value that was loaded could look like a function of the
iteration without being one.

### The domain of the universal claims

D̂ is every state at which a site can fire: paths from the entry or a certified cut to
the site, within the cut-free region. A premise holds the scope, the invariant at its
start, and every guard and no-stop condition up to and including the site's block, as
a block that stops contributes no event. Conditions reading memory or floats are
dropped, which only weakens the premise, and a guard reading memory ends in Unknown.
Every event of a completing run is in D̂, and D̂ may hold more, so it supports
universal claims and no others (invariant I8).

The certificate's invariants do not relate a temporary to its inputs: a loop bound
computed once is unrelated to the variable it came from. So the checker proposes
`t == T` for each temporary with one definition, and keeps a fact only when it holds
on every path into its cut under the kept facts. These are O3 and O4, so they add
strength and no trust, and the last pass is in the record as obligation I.

## Theorems

**Theorem 6 (clause soundness).** Each clause the search learns holds for every valid
assignment of atoms to axes.

- *Dependence:* if h(e₁) ≠ h(e₂), some atom separating e₁ and e₂ is in P, I or J.
  Likewise for f and g.
- *Unit multiplicity:* two distinct events are separated by some selected atom.
- *Box:* let Q be the atoms now in P. If a is in axis A and b in axis B, and no event of
  the Q-batch of e₁ and e₂ has a = a(e₁) with b = b(e₂), then some atom of Q has left P, or
  ¬s_A(a), or ¬s_B(b), or some atom separating e₁ and e₂ is in P.

*Proof of the box clause.* Take an assignment that keeps every atom of Q in P, with a in
A, b in B, and P(e₁) = P(e₂). Its batch of e₁ lies inside the Q-batch. If that batch is a
box, a point with A(e₁) and B(e₂) exists, and its event has a = a(e₁) and b = b(e₂), a
contradiction. Adding atoms to an axis only refines it, so the clause stays valid. ∎

When every pair occurs and only the triple is missing, the same argument gives the
fallback: with e₁, e₂, e₃ witnesses of the I, J and K labels, some atom of Q has left P,
or some selected atom of I, J or K has left its axis, or an atom separating e₁ from e₂
or e₁ from e₃ is in P.

The first literal is needed. A smaller P joins batches, and the missing pair may exist in
the batch it joins. Without that literal the clause excluded valid assignments; the
brute-force test in `SynthesisTest.cpp` found this, and it now judges every assignment of
small banks against every learned clause.

So exhausting the clauses means only that no assignment of these atoms works. It says
nothing about other coordinates.

**Theorem 7 (distinct events differ).** In a completing run of an oblivious program,
two events of one site differ in the state that is live for control at its location.

*Proof.* `controlLiveState` is closed: a variable a guard reads is live, and a live
variable's definition makes what it reads live. So the next location and the next
live state are functions of the location, the live state and the parameters. A repeat
would make the run periodic forever, against completion. ∎

**Theorem 8 (exact coverage by simulation).** Let the order have first(v) and succ.
Suppose S1: the first event is first(v), or there is none and first(v) is outside the
nested set. S2: from an element b, every feasible path to the next event gives succ(b),
inside the set. S3: an element with a successor cannot reach the exit, and one without
has no next event. Then for each completing run, φ is an order isomorphism from the
events onto the orbit of first(v) under succ.

*Proof.* Induction on the event's position. Guards are exclusive (O2), so the paths
partition the behaviours. S1 fixes the first event, S2 each next one, S3 the end, and
termination rules out running forever between events. succ is strictly increasing in
the lexicographic order, so the orbit has no repeats. ∎

The enumerated set is the orbit. It is the whole nested set when that holds first(v),
because S2 demands that each step stay inside, so a gap in the nested set fails the
proof. The walk from one event to the next ignores cuts, prunes where the solver shows
a prefix infeasible, and fails at the depth limit unless that path is infeasible. The
same result fixes the execution order, which later phases need.

## The search

**Grammar.** `GrammarSpec` is data. Level 0 is state and subterms of the accesses; 1
adds addition, subtraction and constant multiples; 2 adds products, division and
remainder; 3 adds comparison and if-then-else; 4 adds bit operations. Levels are tried
smallest first, and stop when something full is verified. Terms are built in the
program's `TermContext`, at exact widths, with G1's total operators. Constants come
from the program, and parameters are operands but never atoms, since a parameter
separates nothing within a point.

**Definedness.** `definedness(t)` holds when every divisor is nonzero, no signed
division overflows, and every shift is below the width. It is separate from the value.
A term undefined at a sample is not offered; a family is refuted at an undefined event;
V0 demands it on D̂.

**Atoms.** Deduplicated by kernel over the samples, keeping the smallest term, because
a label is only compared for equality. Atoms equal on the samples may differ elsewhere,
which can only cost the search a candidate.

**Selectors.** One per atom and axis, at most one per atom, so an axis is a tuple of any
length. The solver proposes, the G3a facts judge on the samples, and each failure is a
Theorem 6 clause. Layers order the search: a box before partial, unit multiplicity,
all axes present, few batch atoms. The limit on candidates is per mode, so a run of box
candidates cannot crowd out a partial decomposition. A candidate whose structure
matches an earlier one is dropped, by the kernels on every sampled point.

**Order.** `proposeOrder` guesses levels from how often each value changes, and bounds
from the samples. It is a guess: a wrong one costs a tier, and the model of a failed
query becomes a sample.

## Non-injective views

| Program | Found | Tier |
| --- | --- | --- |
| `C[i,j] += A[k] B[k,j]` | I=i, J=j, K=k, F ignores I | SymbolicExact, full, not genuine |
| `C[i,j] += A[i,k] B[k]` | I=i, J=j, K=k, G ignores J | SymbolicExact, full, not genuine |
| `C[p,i,j] += A[i,k] B[k,j]` | I=(p,i), J=j, K=k, and P=p as an alternative | SymbolicExact, full, not genuine |
| `C[i,j] += A[i] B[j]` over k | I=i, J=j, K=k, where k reaches no address | SymbolicExact, full, not genuine |

Each is valid by itself. Selection among them is a preference, and G6 can use the
alternatives: `I=(p,i)` and `P=p` are both true, and neither is more correct.

## Fragments

| Fragment | Status |
| --- | --- |
| A finite scope, enumerated | Decidable and exact, within the limits |
| Fixed-width universal queries over D̂ | Decidable (QF_BV). Fast when linear or modular, often unknown with symbolic multiplication |
| Bounded grammar search | Finite. Exhaustion says only "none in this grammar" |
| Exact symbolic coverage | Lexicographic orders of unit steps with parameter or outer-level bounds, and bounded walks between events |
| Claims across widths, or over mathematical integers | Unsupported: the model has neither |
| Reduction order, floating-point equivalence, aliasing | Not examined here: G4 to G7 |

Measured in prototypes: linearized `i*n+j` with symbolic `n` is unknown from 16 bits, and
so is its division inverse, so that stays Unknown at the symbolic tiers. A constant
stride or a finite scope remains available.

## Known limits

- The enumerated tiers hold for a `DomainEnumeration` from the enumerator. A caller can
  not make a domain, but it could drop points from the list it passes in. This is an
  in-process boundary, as in G3a: there is no API that launders a result.
- Groups whose sites share no state: sibling loops with their own variables have no one
  term that is a coordinate of both, so G3b gives Unknown where G3a may have labels.
- Coverage needs unit steps, and the two bound forms above. Other forms stop at a lower
  tier. A hole in a domain has no order, and stays at the finite tiers.
- Two sites of a group in one block have no coverage proof.
- A site with no coordinates has no order, so it has no symbolic tier.
- Every G3a limit on the evidence applies: oblivious programs, completing runs, loads and
  multiply in one block, and each point's own run. Coverage inherits G2a's.
- Grammar levels and rounds are capped, so a result of Unknown is not exhaustion unless
  `exhausted` says so.
- Selector solving slows with many atoms. The cap is 40.

## How G4 to G7 use this

G3b hands over terms, tier, obligation records with scripts, the alternatives and the
open obligations. As G3a does with G2a certificates, later phases re-check.

| Phase | Uses | Owns |
| --- | --- | --- |
| G4 | H, F, G dependences, C0 | Proof that the program computes the GEMM |
| G5 | Tier, multiplicity, the order of Theorem 8 | Domain and ordered-batch semantics |
| G6 | All alternatives | Choosing a lowering that preserves semantics |
| G7 | Roots, bounds, stops | Aliasing, bounds, floating point |

## Changes to G3a

One defect fixed, which changes one result. A site that reads one root and writes
another, `C := D + A * B`, was accepted by G3a: its C0 check compared index views and
never the roots, so it could report CANDIDATE_FOUND for an update that is no reduction
at any index. Such a site is now a group of its own, and Unsupported.

Everything else is additive, with default output byte-identical: `liveStatePoints`, `onlyPoints`, `sampled`
and `memoryIndependentState` in `FiniteDomain`, `structurallyFull` in `CandidateReport`,
which the G3a report does not print, and `reportToJSON`. G1 and G2a are unchanged.

## Validation (2026-11-02, LLVM 24.0.0git, Z3 4.8.12, base 05727c9)

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 13 of 13 CTest suites, including G1, G2a and G3a |
| G3b unit | 517 checks, 0 failures |
| G3a unit | 355 checks, 0 failures |
| Theorem 6, brute force | Every assignment of up to 5 atoms to 4 axes, on 7 programs: no valid assignment excluded by a learned clause |
| Programs, seed 20261104 | 268 programs, 16,428 events, 1,253 candidates, 4,891 facts recomputed, 0 disagreements, 0 closed gates, 0 timeouts |
| Tiers in that run | 708 Proposed, 121 Point, 1,132 ScopeEnumerated, 3 SymbolicValid, 542 SymbolicExact |
| Symbolic replay, same run | 2,180 points the driver never ran, 0 disagreements |
| G3a tables, same seed | 18,587 exhaustive tables, 100,000 candidates, 5,000 hidden boxes, 20,000 subsets, 0 disagreements |
| G3a programs, same seed | 331 programs, 993 points, 18,444 events, 0 disagreements |
| ASan, UBSan, leaks | Clean: both unit suites and 30 oracle programs with 568 replayed points |
| Mutation | 113 of 113 killed in one run on the validated tree, baseline passing first |
| G1 output | Byte-identical to b0f0814: 255 outputs |
| G2a and G3a output | Byte-identical to 05727c9: 85 outputs each, default modes, on this corpus |
| G1 and G2a sources | Unchanged from 05727c9 |
| Polynomial suites | 13 of 13 |
| `check-systolic` | 188 of 190; the 2 failures are the known legacy ones |

The byte comparison does not cover the one intended change to G3a, described under
"Changes to G3a": no output in this corpus differs, so it holds no site the fix affects.
The unit tests cover that case.

The first mutation run killed 70 of 104. The survivors were test gaps, some of them
masked by a stronger obligation, so those tests now call the coverage proof directly.
Closing the gaps found a real defect: the box clause of Theorem 6 ignored the batch, which
could exclude a valid assignment. It never produced a false candidate, because the checker judges each one, but
it could make "no assignment of these atoms works" false. The fix is the first literal of
the box clause. One mutant, `definitions from any term`, is equivalent and is listed in
`EQUIVALENT` with its reason: the filter only saves queries, since every proposed fact is
proved or dropped. The harness now refuses a snapshot that differs from the source, after
a reused one reported survivors against tests it did not contain.

## Trusted base

Everything in the G1, G2a and G3a records, including Z3's unsat answers. The Python oracle
replays symbolic claims at points the driver never ran, and does not prove them.

## Build and test

```sh
ninja -C <build> check-generalized-gemm
python3 research/generalized-gemm/test/fuzz/synthesis_mutation.py --source . \
  --scratch <empty dir> --llvm-dir $L/lib/cmake/llvm --mlir-dir $L/lib/cmake/mlir
```

## Experimental: one-level gathers (`--admit-gather`, branch exp/semgus-indirect-dp)

Off by default. Without the option every output is byte-identical: the 16 record files of
`nest_gate_records.py --check` are unchanged, and so is `check-systolic`. With
`generalized-gemm-nest-report --admit-gather` (`NestAnalysisOptions::admitGather`,
`ImportOptions::admitGather`), a nest such as

```mlir
%t = memref.load %dp[%i] : memref<16xi32>
%r = arith.index_cast %t : i32 to index
%x = memref.load %a[%r, %k] : memref<32x12xf32>   // C[i, j] += A[dp[i], k] * B[k, j]
```

is analysed as an oblivious program, and its record can be VERIFIED. `dp` stands for a
read-only integer index array at run time: MoE's `sorted_token_ids`, a BSR `col_idx`.

**What is admitted.** The importer computes `SemanticProgram::addressGather`: every
address that depends on memory does so only through a Load from a root that no block
writes, read at its entry memory, at an index that reads no memory and no memory-dependent
state. `dp[dp[i]]` is refused (the inner index reads memory), and so is a gather in a
guard (control is not oblivious). `traceOblivious(program)` replaces the five
`controlOblivious && addressOblivious` gates (finite enumeration, G3b symbolic, G4a, G5,
concrete evaluation).

**Where `dp[E]` lives.** In the view grammar, not in the coordinate grammar. The
coordinates of `A[dp[i], k]` are still I = i, J = j, K = k. `dp` appears only in the view
F = (dp[i], k), which G3b already allows to be non-injective. The coordinate grammar skips
any subterm with a Load: a coordinate that reads memory would break Theorem 7 and the
two-copy argument of `adopted()`. The view proposer (DomainProposer) accepts
`[sext|zext] Load(dp, level form)`, and A1 proves it at the symbolic tier. No Injective(dp)
premise is needed: V1f holds by congruence, because both copies share dp's one memory.

**Why the claims stay sound.** A gather root is never written, so a load from it is a
function of its index for the whole run, provided dp does not alias a root that is
written. That separation is an assumption the record names: with C as the output,
"roots dp and C are separate ... used by the computation claim". A dp entry that
addresses outside A stops the run on out-of-bounds UB. The claims cover the runs that
complete (`PROVEN_TERMINATING_IF_DEFINED`), so no separate bound premise is added.

**Finite evidence.** The samples carry no memory. Wherever a finite check or replay needs
dp's contents (the enumeration's entry memory, the Herbrand replay, `evaluateLevelTerm`
with `gather`), it gives cell c of a gather root the value c mod `gatherFillModulus`: the
smallest static extent that a gathered index addresses, so every sampled address is in
bounds. That is a sample, as the finite tiers always are. The symbolic tiers quantify
over every dp.

**The record.** A gathered dimension of a view has `"affine": false` and a `"gather"`
object: the gather root (as `root` everywhere else), `"element": "bvN"`,
`"extend": "sext" | "zext" | "none"`, and `"index"`, one affine dimension per dimension of
the root, in the format of every other dimension (levels, roles, offset). The object is
written only for a gather, so no other record changes. A gather is mapped when every
index is an affine form of the roles of its operand. Then the status is VERIFIED.

**The lowering** (`--affine-matmul-to-systolic="nest-records=FILE record-driven=true"`,
`Systolic/NestRecordLowering.h`). A body load is read as a gather only when the record
gathers. Such a load is an integer or index load whose value, through at most one
`index_cast` or `index_castui`, is used only as an index of loads of the body. The
lowering checks the record's root, extension, element width and index form against the
nest, and checks that dp is read in bounds. It accepts the separation (dp, C) and checks
dp against C in its alias gate, as it does A and B. Before the tiles it emits
`A_packed[r][c] = A[dp[f(r, c)], g(r, c)]` (`materializeGatherPack`, two affine.for), and
the tiles run on that dense matrix. The systolic schedule is unchanged.

**Validation (2026-11-20, base 3fbe627; first on d21e842).** `check-systolic` 223/223: the
222 tests of the base and `nest_records_gather.mlir` (on d21e842, 206/206). `nest_gate_records.py --check` covers 18 files, all
identical: the 16 of the base and the two gather records. The G1 to G5 unit suites and
the research lit pass. End to end on the software runtime model (sync and async), the
`gather` (sext) and `gather_unsigned` (zext) functions, with a dp that repeats rows, give
every one of their 128 cells bit for bit, in 5 invocations, as the remarks announce. Not
run: the fuzz program suites, the mutation harness, the FPGA board, and any Triton
codegen.

**From C, by default (2026-11-21).** systolic-cc's Generalized GEMM fallback now takes
gathers by default. `--no-admit-gather` turns them off, and the fallback is then what it
was before.

- The front end (`systolic-cfront --gather-subscripts`, given with `--static-extents`)
  translates `C[i][j] += A[dp[i]][k] * B[k][j]`. The table is a one-dimensional integer
  array parameter: `int`, `unsigned`, `long`, `int32_t`, and so on. It becomes
  `affine.load dp[i]`, then `arith.index_cast` (`index_castui` for an unsigned element),
  then `memref.load A[row, k]`. A's leading extent is the bound written as an integer
  literal (`float A[restrict 32][12]`).
- The front end refuses each of these, and the function stays C on the host:
  - no literal bound on A;
  - a gather in an array that is written (`C[dp[i]][j]`: the stores stay affine);
  - a second level (`A[dp[dp[i]]]`);
  - a parameter or a `%` beside the gather;
  - a nest whose bounds read another counter;
  - a table read as anything but such a subscript.
- The interface names the table (`gather_table`, `gather_extent`; `index_table`, `signed`).
- systolic-cc passes `--admit-gather` to `generalized-gemm-nest-report`, and does not count
  an integer table as a non-fp32 operand.
- At every call, before it offloads, the shim checks that each entry of dp the nest reads
  is a row in `[0, extent)`. Otherwise the call runs the function as written, on the CPU.
  C does not enforce the bound written, so a dp that reaches past it into a larger
  caller's array stays correct.
- `test/SystolicCC/generalized_gather.c` checks, bit for bit:
  - a dp that repeats rows: 5 tile invocations, and FPGA Executed with the mock hardware;
  - a dp past the bound: the shim's check sends the call to the CPU;
  - `--no-admit-gather`: the function stays on the host.
- Defaults unchanged elsewhere: `check-systolic` passes 224/224. The records systolic-cc
  writes for `generalized_fallback.c` are identical with `--admit-gather`.

**Limits.** One level, a one-dimensional dp of static shape, read-only for the whole
nest, and no gather into C (a scatter would need Injective(dp) for C1). The bounds of
`A[dp[i]]` are the program's own: the lowering reads the same addresses as the nest.

## Experimental: outputs at non-affine addresses (2026-11-21)

On by default in systolic-cc's Generalized GEMM fallback (`--no-general-scatter` turns it
off). C may now be written at any address of (i, j) that the nest computes: `C[(3 i + 1) % 16][j]`,
`C[dp1[dp2[i]]][j]`, `C[(i * i + j) % 16][i > j ? (i ^ j) & 7 : j / 2]`. Several targets may
coincide.

**Front end** (`systolic-cfront --scatter-subscripts`, given with `--static-extents`).

- A subscript of a written array may be any integer expression. It may read loop counters,
  signed integer parameters, literals, and one-dimensional integer tables to any depth.
- The operators are `+ - * / % & | ^ << >>`, unary `-` and `~`, and `?:` on an integer
  comparison.
- It becomes index arithmetic (`divsi`, `remsi`, `select`, and so on: C's `int` result
  wherever C defines one), read and written with `memref.load` and `memref.store`.
- The array's bound, and every table's bound, must be written as an integer literal.
- Refused, so the function stays C: an unsigned computation (it wraps), a counter whose
  bounds read another counter, a second general write of the same array.
- The interface gives the expressions as C text, together with the counters and their ranges.

**Research.** `addressGather` now admits gathers at any depth, each from a root that no block
writes. With that, `C[dp1[dp2[i]]][j]` is VERIFIED_NOT_MAPPED: Theorem 13 VERIFIED, the
coordinates SYMBOLIC_EXACT, C's view PROVEN at the symbolic tier. The records keep their
refusals ("not affine", "gathers at an index that is not mapped", "two logical outputs may
share a cell of C"). The production gate lets only the scatter-add take a record whose
remaining refusals are of those kinds.

**Track A, a closed-form inverse.** Applies when one dimension of C's address is
`(affine form of i) mod M`, the other is affine in j, the dividend is never negative, I runs
exactly M times, and gcd(a, M) = 1.

- The lowering computes a⁻¹ mod M by the extended Euclidean algorithm. The inverse is
  `(a⁻¹ m + c) mod M`; for `(3 i + 1) mod 16` it is `(11 m + 5) mod 16`.
- It checks `inv(φ(y)) = y` for every y in I's range, exactly.
- A is packed as `A_packed[m][k] = A[inv(m)][k]` (an affine map with `mod`), and C is
  written as a view. Nothing is built at run time.
- The research record proves the view (A1) and the fold. The inverse is the nest's own
  arithmetic, checked at compile time, as the tables are.
- Other static bijections (bit reversal, polynomials) take the existing compile-time tables.

**Track B, a scatter-add through a run-time inverse map.** Applies to any other address of
(i, j): one that reads a table, or one where targets collide.

- The lowering re-emits the nest's own computation of C's indices; every operation must be
  arith, `affine.apply`, or a load from a read-only integer table argument of static shape.
- In the kernel, for every (i, j), each table index and each index of C is checked inside
  its memref; a failing index is replaced by 0, so nothing is read or written outside.
- When every index is inside:
  - A and B are packed;
  - the tiles write a zeroed M x N matrix T;
  - `bucket_offsets[0 .. |C|]` is built by counting the targets and taking a prefix sum;
  - `sorted_src_idx[0 .. M N)` holds the sources by cell of C, stably;
  - each cell of C that has a source gets its value plus its sources' cells of T, in source
    order, and is written once.
- Otherwise the nest runs as written: it is moved into the else branch of the `scf.if`.
- Collisions change the order of the floating-point additions, so the floating-point policy
  applies as for every tile: `reassoc`, or `allow-fp-reassociation`.
- The CSR is built by the kernel's host code, after the tiles and before C is written; the
  shim does not build it. Handing it over from the shim would change the kernel's C ABI.

**Guards.**

- The shim evaluates the C text of the subscripts at every value of the counters they read,
  and runs `NAME__systolic_reference` unless every target is inside `[0, bound)`. C does not
  enforce the bound written, and the snapshot and the undo cover `[0, bound)` only, so a
  target outside never reaches the kernel.
- The kernel's own check is the second line of defence. It also covers a table index past a
  table's bound. When it fails, the nest runs as written. Under systolic-cc that path is then
  not "completed" (0 invocations), so the shim restores the snapshot and runs the reference.
  This is correct because the shim has already shown every target inside the snapshot.

**Validation.**

- `test/SystolicCC/generalized_non_affine_scatter.c`, bit for bit:
  - (a) the permutation by Track A;
  - (b) two levels of tables with collisions by Track B;
  - a coupled 2-D address with collisions by Track B;
  - (c) a target past C's bound, sent to the CPU by the shim (CPU Executed with the mock
    hardware);
  - `--no-general-scatter`: the functions stay C.
- `test/Systolic/nest_records_scatter.mlir`: the IR of both tracks.
- The kernel's fallback was run outside systolic-cc: 0 invocations, and all 256 cells right,
  including those past the 16 rows of the descriptor.
- Not run: the FPGA board, the research fuzz and mutation suites.

**Limits.**

- Track A covers only `(a i + b) mod M` on C's row with I running exactly M times.
- Track B needs C's address to read only I and J (V1h), and A and B affine.
- The shim's check costs one evaluation of the subscripts per point of (i, j) per call.
- Not every integer expression can be verified: the research's grammar and solver limits
  apply. A nest it does not verify stays on the host.


## Experimental: outputs whose address reads k (2026-11-21)

Covered: `C[φ(i, j, k)][…] += A[i][k] * B[k][j]` for any deterministic integer expression φ the
front end takes. That includes polynomials (`i * i + k`), higher powers, division and remainder
(`(i * i * i + 2 * k) % 16`), bit operations, `?:`, and tables read at any depth (`dp[i + k]`).
It is on by default, with the scatter-add (`--no-general-scatter` turns both off).

**What changes when φ reads k.** The products of one (i, j) no longer meet in one cell, so k is
no longer only a reduction axis. A tile that sums over k into one cell cannot run such a nest.
Two rewrites that would make it a dense GEMM are refused because they are not sound:

- Pre-summing A's rows by bucket uses distributivity, `Σᵢ aᵢ b = (Σᵢ aᵢ) b`. The program's
  `reassoc` does not permit that; LLVM factors only with `reassoc` and `nsz`.
- Padding with zeros adds `0 * Inf = NaN` where the program has no such product.

The lowering instead computes exactly the program's products, each once, and reassociates
only the sums.

**Research.** G3b needs C's view to be a function of (P, I, J). For these nests the family is
P = (i, j, k), with I, J and K empty: a batch of rank-1 products. The search, which prefers
I, J and K, does not propose it. With `--admit-gather`, `DomainAnalysisRequest::pointwiseFallback`
adds the pointwise family (P = the group's loop counters). It is added only when no
candidate has its fold verified at the symbolic tier with a symbolic logical tier, so every
other record is unchanged. Its checkers prove:

- G4a over the scope;
- D1 to D3, with exact order and unit multiplicity;
- Theorem 13 at the symbolic tier.

The logical tier stays PROPOSED, because it speaks of an I, J and K decomposition. NestRecord
picks a candidate in this order: first one with a symbolic logical tier, then one with an
exact unit domain, then one with a verified fold.

**Gate and lowering.**

- A pointwise record (P three levels, I, J and K empty) is taken only by the product
  scatter-add. That needs Theorem 13 VERIFIED at SYMBOLIC and the domain VERIFIED, exact and
  UNIT at SYMBOLIC. Its own refusals (no I, J, K decomposition, a batch axis, roles of P) are
  accepted.
- The roles come from the nest: K is the loop A and B both read, I the other loop of A, J the
  other loop of B. A and B must be affine.
- At run time, every target of (i, j, k) and every table index is checked, as for the
  scatter-add.
- When all are inside: A and B are packed. For each k, the tiles of the K = 1 GEMM of A's
  column k and B's row k write `products[k]` (the plan of M x N x 1, K times, announced as
  `regions × K`).
- `bucket_offsets` (count and prefix sum) and `sorted_src_idx` over the sources (k, i, j) add
  every product into its cell, in source order, each cell written once.
- Otherwise the nest runs as written. The shim checks the targets, by the C text of φ over
  the counters it reads (i and k), before the kernel runs.

**Validation.**

- `test/SystolicCC/generalized_k_scatter.c`, bit for bit:
  - `C[i * i + k][j]` and `C[(i * i * i + 2 * k) % 16][j]` (no table), and `C[dp[i + k]][j]`
    (a table with repeated targets): 12 tile invocations each, and FPGA Executed with the mock
    hardware;
  - a target past C's bound: sent to the CPU by the shim.
- Outside systolic-cc, the kernel's own fallback: 0 invocations, and the result right.

**Limits.**

- The array runs M × N × K multiply-accumulates in K rank-1 GEMMs, so its use is poor next to
  a dense GEMM of the same size. The reduction runs on the host.
- The products buffer is K × M × N.
- φ must not read memory that the nest writes.
- A and B must be affine in (i, k) and (k, j).

**Gathered inputs with an output that reads k (2026-11-21).** The product scatter-add also
takes A or B gathered by one level, `C[i * i + k][j] += A[dpA[i]][k] * B[k][j]` or
`… * B[k][dpB[j]]`. It reuses the gather path:

- The research verifies the nest over the pointwise family. The refusals for a gathered view
  of A or B on P are accepted for pointwise records only.
- The lowering reads the gather off the nest: a read-only one-dimensional table at an affine
  form of the operand's roles, inside the table. It packs the operand as
  `materializeGatherPack` packs every gather, `A_packed[i][k] = A[dpA[i]][k]`, before the
  K = 1 GEMMs and the two-layer CSR.
- The shim checks dpA's entries against A's bound, as for every gather, and C's targets
  against C's bound. Either check failing sends the call to the CPU.
- The front end (`--gather-subscripts`) now takes the gather in any one dimension. An inner
  dimension's extent is C's type, so `B[k][dpB[j]]` needs no literal bound; a gathered row
  still needs one.
- `generalized_k_scatter.c` checks, bit for bit, both gathers and a row of A past A's bound,
  which the shim sends to the CPU.
