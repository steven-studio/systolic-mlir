# Generalized GEMM recognition: scope of this term (2026-11-16)

A planning record, written at `d974400` from the documents and the code, with nothing built
or run. Where an earlier document lists M4c, M4d, G6 or G7 as the next step, this one takes
precedence. Stages 1 and 2 of the plan were then built: their sections say what was built,
what was checked and what was not.

## The decision

Two loops are **not merged** in this term. The partitioner cuts every recognized GEMM into
tiles anyway, so a GEMM assembled from two loops would be cut again; merging is no longer a
prerequisite of anything. Each loop nest is recognized by itself, as its own GEMM, and
replaced where it stands.

This does **not** make the loops independent of each other. Four things are kept as they are
in the source and never assumed away:

1. **Order.** Nests are replaced in place, in program order. Nothing is fused, split across
   loops or moved, and the statements between nests stay where they are.
2. **A shared output.** A loop that zeroes C stays as written (it is no GEMM nest). A nest
   accumulates into the values C holds when the nest starts, whatever wrote them: a second
   nest writing the same C is "C += A·B on the C it finds".
3. **Aliasing.** Inside one nest, A and B must be apart from C, by a proof (a fresh
   allocation) or by a named assumption that is recorded. Across nests nothing is assumed.
4. **Dependences across loops.** A later nest may read what an earlier one wrote, as an
   operand or as the accumulator. That is correct only because each replacement completes
   before the next statement starts. Anything that would let two nests overlap in time needs
   a dependence proof, and is outside this term.

## What exists (kept, not redone)

| Work | State | Role from now on |
| --- | --- | --- |
| G1, G2a, G3a, G3b, G4a, G5a: one nest from model to Theorem 13, with a logical tier | committed | the per-loop recognizer; used as it is |
| Grammar integration (`9bb3cf0`): statement grammar, closed-form views, iteration counters, the `recognition` summary with `lowering_eligible` | committed | per-loop views and the eligibility summary; used as it is |
| G5b M1 (multiplicity as a box) | committed | per loop |
| G5b M2 (sibling loops as segments, Theorem 15), M3 (Theorem 16), M4-0, M4a (Theorem 15 over a union), M4b-1 (Theorem 17) | committed | **joint** statements about several loops in one function. Complete as analysis, kept, not extended, and not on the lowering path |
| M4b-2a, M4b-2b-1, M4b-2b-2: reorder facts and their witness search | committed (`d974400`) | the same code serves one nest (Theorem 13's facts), so it stays in use; the set-level part is joint analysis as above |
| The UBSan fix of `DomainTest.cpp` (`dd2432e`) | committed | kept |

## M4c, M4d, G6 and G7, taken apart

Three classes: **(1)** needed for each loop to be recognized by itself; **(2)** needed to
reach the partitioner and the lowering that exist; **(3)** serving only a merge, one GEMM
out of several loops, or a reordering across loops.

| Item | What it is | Class | Decision |
| --- | --- | --- | --- |
| M4c: V0, V1 of a segment inside a multi-loop claim (Theorem 18) | per-loop coordinates, stated inside a joint claim | the coordinate check itself is (1) and exists for a nest analyzed alone (G3b's V0 to V4, Theorem 8); the joint packaging is (3) | **deferred**: nothing new is needed once a nest is analyzed alone |
| M4d: candidate verdict over a set (R and every L_s) | the joint verdict | (3) | **deferred**. For one nest the verdict exists: Theorem 13 with its logical tier, and the `recognition` summary |
| M4d: proposer over unions (axes per segment) | axes for several loops at once | (3) | **deferred**. For one nest the G3b search assigns the axes |
| M4d: driver output for end-to-end candidates | a record another tool can read | (2) | **kept**, as one record per nest (the descriptor below) |
| G6: choosing, among the valid decompositions of one nest, one the lowering supports | selection | (2) | **kept**: box, unit multiplicity, affine views, constant extents |
| G6: the argument that the lowering of one nest keeps its meaning | tiles keep each cell's contributions; the array's tree changes their order | (2) | **kept**: integers by the proved reorder fact; binary32 by the source's `reassoc` flag or the user's option, as the lowering demands today, never by a proof |
| G6: reading fixed-width bounds as extents | no wrapped bound may become an integer extent | (2) | **kept**, small: constant bounds only at first |
| G6: nests replaced in place, in order (new) | the composition that takes the place of a merge | (1) and (2) | **kept**: see "In-place composition" |
| G6: whether segments compose into one GEMM; K split over two loops | merging | (3) | **out of this term** |
| G6: whether the order of nests may change | reordering across loops | (3) | **out of this term** |
| G7: A and B apart from C within one nest | aliasing | (2) | **kept**: mapped onto the lowering's own gate |
| G7: accesses within bounds; runs that stop | bounds, stops | (2) | **kept**: the lowering's bounds check stays the authority; a run that stops in the source is outside every claim |
| G7: floating point within one nest | strict order against the tree | (2) | **kept**, as the policy above |
| G7: output roots of several loops apart (the RA gate of a set) | aliasing for joint claims | (3) | **deferred** with the joint claims |
| G7: reordering or reassociating across loops | cross-loop | (3) | **out of this term** |

## The partition and lowering that are in use

Read from the code at `d974400`; `systolic-cc` runs them in this order.

1. `systolic-cfront`: C to MLIR. Counted loops with constant or affine bounds only.
2. `--systolic-declare-fpga-fleet`, then `--affine-matmul-to-systolic`
   (`lib/Systolic/Transforms/AffineMatmulToSystolic.cpp`). For every candidate nest, in program
   order (`collectAffineMatmulCandidates`), each by itself:
   - `recognizeLogicalMatmul` gives a `LogicalMatmul`: the roles I, J, K (and batches), the
     operand views, the domain, `accumulates`, `reductionMayReassociate`;
   - `lowerOne` takes constant extents M, N, K (`getDenseLogicalGemm`), checks that A and B are
     apart from C (`provablyDisjointFromOutput`: `llvm.noalias`, a fresh allocation, or the
     option `assume-no-alias`), the bounds, the element types and the floating-point policy;
   - `planSystolicMatmul(op, M, N, K, fleet, options)` is the partitioner: the deadline search
     over the fleet and the cost model;
   - `partitionLogicalMatmul(matmul, boxes)` proves that the plan's boxes cover the domain
     exactly;
   - `emitSystolicTilePlan(builder, loc, A, B, C, plan, fleet, index)` writes the
     `systolic.matmul_tile` operations where the nest stood, and the nest is erased.
3. `--lower-systolic-matmul-tile-to-fpga`: each tile becomes a runtime call
   (`fpga_matmul_tiled_auto_scheduled`, or with `async=true` `fpga_gemm_begin`,
   `fpga_tile_submit` and one `fpga_gemm_wait` per GEMM).
4. `--systolic-runtime-to-llvm`, then LLVM.

Two nests in one function are already handled this way: each is recognized and replaced by
itself, in place and in order, with no analysis between them and no code that merges them
(`test/Systolic/affine_gemm_certified_offload.mlir`, `@siblings`: a nest on C, a load of C
stored into D, a nest on D; `test/SystolicCC/fallback.c`, `two_nests`). A second recognizer
(`analyzeSymbolicRankPartition`, with the general-affine, device-job and kernel-coset paths)
sits behind the option `general-affine`, which `systolic-cc` passes; the path above runs
first, and the others are not part of this plan.

The production code does not reference the research library, and the research build is
never part of the production build.

## What the partitioner must be given, and the gap

| The lowering needs | The research result has | Gap |
| --- | --- | --- |
| the nest to replace | sites and their places in the model, not the source loop | one record per nest: function and the nest's position |
| M, N, K as constants | the order's level bounds, as terms | export as integers; refuse a symbolic bound |
| each operand's view: array, and scale and offset per role | the proved closed forms of h, f, g over the levels, and whether each is affine | export in the view form; affine views only (Definition 4.5, V) |
| the mapping is a bijection onto the box | Theorem 12's exact order, G3b's exact coverage and box, multiplicity one | export "box, unit"; the lowering still proves its tile cover itself |
| the update reads the cell it writes; accumulation or assignment | G4a's W1; accumulation only | assignments `C = A·B` and scalar accumulators stay with the affine recognizer |
| A and B apart from C | separation by a fresh allocation (a proof) or by an authorized name (an assumption) | fresh to fresh, name to `llvm.noalias` or the option; never assumed silently |
| accesses within bounds | an access out of bounds is a stop, outside the claim | the lowering's own check |
| binary32, and permission to reassociate | the float reorder fact is never proved for two contributions; the `reassoc` flag is recorded | the permission stays the lowering's gate; `lowering_eligible` is false for such a nest today, so a second field must say "ordered fold verified, reassociation permitted by the source" |
| batches, reduction batches | the P axis, repeat levels, multiplicity boxes | not mapped at first |

Two more gaps are on the recognition side:

- **A claim about one nest.** Today every claim is about the whole function, from its entry to
  its exit. With two nests on one output each nest's own claim is refuted, correctly, and the
  joint theorems refuse a nest that reads what another wrote. What is needed is the nest as
  a program of its own: every memory at the nest's entry, every value of the scalars it
  reads. The importer takes a whole function, so the nest must be outlined first.
- **Separation carried over.** Outlined, a root allocated earlier in the function becomes an
  argument and loses its proof of separation. The facts must be computed in the function and
  handed to the nest's claim, each with its basis.

### In-place composition

If, for a nest, the local claim holds for every entry memory and every value of its live-in
scalars in scope, then replacing the nest by code with the same effect on runs that complete
leaves every completing run of the function with the same final memory, provided the
replacement finishes before the next statement starts. Other nests are not mentioned: what
they did is part of the entry memory. This is what replaces a merge, and it is why the four
points of the decision hold.

## What cannot be reached yet

1. The C front end accepts counted loops with constant or affine bounds; a loop that only
   the generalized recognizer understands never reaches the pass from C. The research driver
   reads MLIR.
2. The partitioner needs constant M, N, K.
3. The lowering erases an `affine.for` nest and builds affine views. A nest with other control
   and affine views needs a new entry, driven by the record (stage 3).
4. The device is binary32. No float fold with two contributions is ever proved reorderable;
   the permission is the source's flag or the user's option.
5. Scalar accumulators and assignments are not MAC sites of the model.
6. Batches and reduction batches are not mapped.
7. The research library stays out of the production build; a result crosses as a file.
   `authorizes_lowering` stays false: the record is evidence, and the lowering's own checks
   decide.

## The shortest path

| Stage | What | Shows |
| --- | --- | --- |
| 1 | Research side only. One claim per nest (outline, separation carried over), one record per nest; compared field by field with what the affine recognizer reports for the same nest | a single GEMM nest, then two nests that are not merged (one output shared; one nest reading the other's output): a verified record for each, equal to the partitioner's input |
| 2 | The pass takes the records behind an option that is off by default: a nest is lowered only where a record for it exists and agrees | the same two cases through the partitioner to `systolic.matmul_tile` and the runtime calls, in place and in order; a missing or disagreeing record leaves the nest on the host. The first change to production code |
| 3 | Lowering driven by the record, for a nest the affine recognizer refuses and whose views are affine | the generalized recognizer adding a case, within limits 1 to 6 |

Stages 1 and 2 are built (below). Stage 3 is not started.

### Stage 1: one verified record per nest (built 2026-11-16)

Research side only. Every file is new except the research build file and the lit
configuration, which gain the new targets and one substitution. No importer, checker,
proposer or driver that existed is changed, and none of their binaries or libraries is
rebuilt: each has the hash it had before.

| File | What it is |
| --- | --- |
| `include/GeneralizedGemm/NestOutline.h`, `lib/GeneralizedGemm/NestOutline.cpp` | the copy of a nest, the function's roots and order, how nests meet on memory |
| `include/GeneralizedGemm/NestRecord.h`, `lib/GeneralizedGemm/NestRecord.cpp` | the domain driver's pipeline on a copy, and the record read off its results |
| `research/generalized-gemm/NestReport.cpp` | the driver `generalized-gemm-nest-report`: one JSON object per function |
| `test/unit/NestTest.cpp`, `test/lit/nest_*.mlir` | unit and lit tests |
| `test/fuzz/nest_oracle.py` | the oracle: its own programs and interpreter, no library code |
| `test/fuzz/nest_affine_compare.py` | a record against the production recognizer's report, field by field |
| `test/fuzz/nest_mutation.py` | mutants of the two new library files |

**The copy.** For each outermost loop of a function (a loop written as an operation, not
inside another loop; under a condition or not), the loop is cloned into a function of its own,
in a module of its own. The function given is read and never written.

- *Inputs.* A memory root the nest uses becomes an argument: any memory at the nest's entry.
  A scalar it reads becomes an argument: any value. Only an `arith.constant` is copied in. A
  value computed before the nest is not recomputed in the copy: a load before the nest is not
  repeated, because the cell may hold another value when the nest starts.
- *Entry state.* Nothing is fixed but the constants. What ran before, other nests included, is
  part of the memory and the scalars a claim ranges over. A claim about the copy is therefore
  a claim about the nest wherever it runs, and no nest is taken to be independent of another.
- *Memory effects.* The nest's operations are cloned as they are, in their order.
- *Preconditions.* The scope of a scalar argument, the separations (below), the index width,
  and "runs that complete" where a claim says so, are listed in the record as assumptions.
- *Refused, in words, with no copy:* a loop whose value is used after it; a parallel loop,
  whose iterations have no order of their own; a call; a memref that is no root of the
  function (made before the nest by anything but an allocation, or made inside it: a view, a
  global, a memref carried round the loop); a value that is no scalar; a function whose
  control flow is by branches; a copy that does not verify. The module's, the
  function's and the arguments' attributes go with the copy, so the importer's refusal of an
  attribute it does not know still applies.

**Separation, from the function.** How two roots stand is decided in the function: a root
allocated there is apart from every other, which is a proof; two arguments are apart only by
a name somebody authorized (`--authorize`, over the function's root numbers), which is an
assumption; anything else may overlap. In the copy both are arguments, so each fact is handed
to the copy's analysis by name, and the record states the basis it had in the function. A
record never lists a proof as an assumption, and `llvm.noalias` alone is no basis: the name is
still needed, and the pair stays an assumption.

**Order and dependences.** Beside the records, the function's object lists its roots, the
order of its nests and of the statements outside them that touch memory, and each pair of
nests that meets on memory: on one root, on two roots not known apart, or by effects that are
no access to a root. The list is by root, an over-approximation, and is never a reason to
call two nests independent. Nothing reads it to decide anything.

**The record**, one per nest, in program order: the function, the nest's number, statement
and source position; the copy's inputs; each pair of roots with its basis; the model's gate
and termination verdict; for the candidate chosen, the levels with their axes (I, J, K),
bounds and directions, M, N and K when the box is constant, each operand's root and view (per
dimension the constant and the coefficient of each level, and the scale and offset over
coordinates from zero), the update (operation, operand order, element, flags), the frame of
other effects, and every claim behind it with its status and tier; the assumptions; the
refusals; and two summaries that are kept apart:

- `ordered_fold_verified`: Theorem 13 is VERIFIED at the symbolic tier and the computation
  claim's frame has no write. For every memory at the nest's entry and every value in scope,
  a run of the nest that completes leaves each cell of C with the fold of its contributions
  in the nest's own order, and writes nothing else.
- `reassociation_permitted`, with its basis: `proof` (no cell has two contributions, or a
  wrapping integer addition with no flag, each proved over the scope), `source flag` (every
  accumulation carries `reassoc`: the programmer's permission, and no proof), or `none`.

`status` is UNSUPPORTED (no copy, or the importer refuses it), NO_GEMM (no multiply-accumulate
site), NOT_VERIFIED, VERIFIED_NOT_MAPPED (the fold is verified; the nest is no constant box
with three proved affine views) or VERIFIED. Every reason short of VERIFIED is in `refusals`.
`authorizes_lowering` is false in every record and in every function object, which names
its format (`"schema": "generalized-gemm-nest/1"`; `/2` since stage 2, which added the binding,
the index width and the scope).

**Acceptance.**

| Case | Result |
| --- | --- |
| A. One product over a constant box | 32-bit integers: VERIFIED, reassociation by a proof. binary32 with `reassoc`: VERIFIED, reassociation by the source flag, the float reorder fact REFUTED beside it. binary32 with no flag: VERIFIED, reassociation not permitted. Extents, roles and views equal the production report's (below) |
| B. Two nests on one output | two records, each VERIFIED by itself, the shared root listed. Today's whole-function claims, pinned in the same lit file: each group's own claim REFUTED, the claim over both VERIFIED |
| C. A dependence between two nests | a nest on C, a value of C stored into D, a nest on D: two VERIFIED records, the statements between them in the order list. The second nest reads the first's output as an operand: two VERIFIED records, the dependence listed. Today's claim over both groups is REFUTED in both, as before |
| D. Refusals | A may overlap C: NOT_VERIFIED, the pair and the name that would assume it stated. A view that is not affine: VERIFIED_NOT_MAPPED. A bound that is an argument: VERIFIED_NOT_MAPPED, the argument named. A loop carrying a value: UNSUPPORTED. A root allocated before the nest: VERIFIED, both pairs apart by a proof, no assumption about roots |
| E. Composition | on random memories, the function run as written and the function with every completely recorded nest replaced by its record's fold from the memory at that nest's entry end with the same bits: on every one of 120 runs of 30 functions, at each of two seeds, 47 nests replaced at each. Where today's claim over both groups is refuted (a nest reads cells the other wrote), each nest's record is verified by itself and the composition holds |

**Against the production report.** `nest_affine_compare.py` runs the production tool on each
function alone, and only runs it: the discovery pass and `--affine-matmul-to-systolic=report=true`,
without and with `assume-no-alias`.

| Field | Production prints | Record |
| --- | --- | --- |
| M, N, K | `dense extents`; the report's `affine GEMM MxNxK` | `gemm.extents` |
| roles | `I/J/K on loops a/b/c` | `gemm.axes` |
| origin | `source origin` | the levels' lower bounds |
| views | `view X: [s*R + c, ...]`, placed by `layout X` | per dimension, the coefficient of the role's level and the constant |
| statement | `C = C + A * B (accumulation)` | the update, from the cell's value at the nest's entry |
| reassoc flag | `reduction: reassoc permitted` | `reassociation.source_flag_reassoc` |
| bijection | `logical bijection: verified` | the order exact, one event a point |
| alias gate | `A may overlap C` with no option | no name authorized: not verified, in the same words |
| reassociation | lowered, or `reassociation is not permitted` | `reassociation.permitted` |

For seven functions, at a 64-bit index (the three of case A; a nest with a permuted loop
order, offsets, a scale and a transposed operand, in binary32 and in integers; a nest whose C
is allocated in the function; one whose C is `llvm.noalias`): 63 fields, 60 equal, no
failure. Two differences of policy are printed and are no failure: the production tool
takes `llvm.noalias` as proof that the operands are apart, where a record needs the name and
calls it an assumption; and the production tool reads binary32 products only, so an integer
nest is compared with the record of its binary32 twin, which is compared with production.

**Limits found while building it.** None is a wrong record; each is a nest left without a
complete one.

1. A loop with one iteration. Its variable takes one value, and the existing search makes no
   coordinate of it: the fold is verified and the record is VERIFIED_NOT_MAPPED, or, with a
   stranger family, NOT_VERIFIED.
2. A view written with a subtraction (`A[i - 1, k]` by `arith.subi`) or as a multiple of a sum
   gets no closed form from the existing proposer: VERIFIED_NOT_MAPPED. The same view written
   as an affine expression is mapped.
3. An output zeroed inside the nest is an assignment: verified at the points run, not over
   the scope, as before. Scalar accumulators are no sites of the model.
4. One GEMM per nest: two products in one body are two groups, and no record.
5. A scope can be given for a scalar argument of the function only. A value computed before
   the nest, and an extent of a dynamic array, range over every value.
6. Loops written as branches, parallel loops, calls, views and globals are refused by the
   outliner, whatever the importer could do with the whole function.
7. The dependence list is by root, not by cell.
8. A record is about the index width it was computed at, which it lists as an assumption. The
   unit and lit tests use 16 bits for speed; the comparison with production runs at 64.
9. The record takes the first candidate verified over the scope, in the search's order. A
   later candidate that would map where the first does not is not looked for.
10. A nest takes about ten seconds: the pipeline runs once per nest.

**Validation.** On the tree frozen at hash `24cb6b8e7d5d…` (the SHA-256 over `include/GeneralizedGemm`,
`lib/GeneralizedGemm` and `research/generalized-gemm` that `nest_mutation.py` computes), which
is the committed one. Every phase recorded that the tree had not changed when it ended.

| Check | Result |
| --- | --- |
| Debug build; the four new sources once more with `-Wall -Wextra -Wpedantic` | 0 warnings |
| The 31 binaries and libraries that existed | not rebuilt; SHA-256 equal to the copies taken at `d974400` before the first edit |
| Unit, `generalized-gemm-nest` | 786 checks, 0 failures: outline, mapping, composition (the copies of a function's nests, run one after the other by the library's concrete evaluator, give the function's memory), product, nests, refusals, effects |
| lit, the whole suite (its configuration changed) | 35 of 35: the 29 files that existed and 6 new |
| Oracle, 30 functions, seed 1 (as CTest runs it) | 0 failures. 47 products with every field equal to the program's; 188 nests against their record's fold; 120 compositions; 12 nests that must not be verified, each with a run that shows why (an operand on the output's root, an operand not known apart from the output, another write in the nest, the output zeroed in the nest: 3 each); 30 order lists, 30 dependence lists and 75 separation lists recomputed; today's whole-function claims read for 6 functions |
| Oracle, seed 20261116 | 0 failures, the same floors met |
| Against the production report, 7 functions, 64-bit index | 63 fields, 60 equal, 3 differences of policy, 0 failures |
| Mutation, `nest_mutation.py`: 63 mutants of the two new library files | 63 KILLED, each by an assertion of a test (first by the unit sections outline 22, mapping 21, effects 9, product 7, refusals 2, nests 1, and by lit 1). 3 more are declared equivalent, with their reasons, and not run. Three parts, each with its own snapshot and its own passing baseline |
| The oracle alone, on 9 of those mutants that change what a record says (a supplement: in the run above the unit binary is asked first) | 9 of 9 KILLED, each by a disagreement of the oracle's own (a separation list, a dependence list, a view's root, the extents, a product left unverified, a nest that must not be verified, a permission by a flag the program does not have, the header) |
| ASan and UBSan: the unit binary, and the driver on the six lit inputs with and without the analyses | 786 checks, 0 failures; 12 driver runs, 26 records; no report |

Reused without a rerun, by what they depend on: the other 22 `generalized-gemm-*` CTest
suites and the 9 `polynomial-*` ones (their binaries, scripts and inputs did not change), and
every historical mutation run (the files they mutate and the tests they run did not change).
The lit suite was rerun because every lit test reads the changed configuration.

**Not run.**

- The 22 other `generalized-gemm-*` CTest suites (the unit binaries and oracles of G1 to G5b
  and the grammar phases) and the 9 `polynomial-*` ones. None of their binaries, scripts or
  inputs changed: the 31 binaries and libraries that existed have the hashes they had at
  `d974400`.
- Every historical mutation run (`domain_mutation.py` and the earlier harnesses): they mutate
  files this stage does not touch and run tests it does not touch. The four mutants left open
  at G3b and G3a, the stale EQUIVALENT entries and the mutants not rerun at `9bb3cf0` stand as
  they were recorded.
- The sanitized domain core (115 minutes): `DomainChecker.cpp` and `DomainTest.cpp` are
  unchanged.
- A Release build.
- The production test suites (`check-systolic`, the cosimulation and board flows): no
  production file is changed. `systolic-opt` was built once from this worktree's sources, out
  of tree, and only run.
- Records at index widths other than 16 (unit, lit), 16 and 32 (the oracle) and 64 (the
  comparison with production).
- Stage 2 and stage 3 are not started. Nothing is merged across loops. Nothing is pushed.

### Stage 2: the pass reads the records, behind an option (built 2026-11-17)

The first change to production code, off by default. With
`--affine-matmul-to-systolic="nest-records=FILE"`, a GEMM is lowered only where FILE, written by
`generalized-gemm-nest-report`, has a record of its loop nest that is about this nest of this
input, complete, verified, and in agreement with what the pass recognizes; the lowering then
makes every check it makes without records. A record is a gate and nothing more: it never
replaces a check of the lowering, and it can only take a lowering away.

| File | What it is |
| --- | --- |
| `include/Systolic/NestBinding.h`, `lib/Systolic/Transforms/NestBinding.cpp` | which loop of which input a record is about. One piece of code, compiled into the pass and into the research tool |
| `include/Systolic/NestRecordGate.h`, `lib/Systolic/Transforms/NestRecordGate.cpp` | the file of records, the bindings of the module's nests, and the decision for one GEMM |
| `lib/Systolic/Transforms/AffineMatmulToSystolic.cpp` | the option, and the gated path: one branch at the top of `runOnOperation` |
| `test/Systolic/nest_records.mlir`, `nest_records_refusals.mlir`, `nest_records_belts.mlir`, `Inputs/nest_records*.json`, `Inputs/edit_nest_records.py` | the production tests, and the records they read |
| `research/generalized-gemm/test/fuzz/nest_gate_records.py` | writes those records from the research tool's output, and checks they are still what it says |
| `research/generalized-gemm/test/fuzz/nest_e2e.py` | end to end: records, the pass, the FPGA lowering, a run against the software runtime model |
| `research/generalized-gemm/test/fuzz/nest_gate_mutation.py` | mutants of the gate, run against the production tests |

**The binding.** A record is about one outermost loop of one function as it stood in the input
the record was made from. Its binding is the SHA-256 of a canonical text: the scheme, the
function's name, the loop's position among the function's outermost loops, one line for each
value the loop reads (a root by its number, as argument or allocation, with `llvm.noalias`; a
scalar argument; a value the function computes, by the name of its operation), and the loop
copied into a function of its own, constants copied in, printed in the generic form without
locations. The research tool computes it of every nest and writes it into the record (format
`generalized-gemm-nest/2`, with the index width of the claims and the ranges of arguments
they are restricted to); the pass computes it of every outermost loop of the module before
anything is lowered. A different file name or line keeps the binding; any change of the
nest's operations, attributes or types, of what it reads, of the function's name or of the
nest's position changes it.

**The gate**, for one GEMM the dense recognizer found, in this order; the first that fails is
the remark, which names the nest:

1. The GEMM's outermost loop is one of the outermost loops of its function at the pass's
   input, with a binding; the file has a record of that function and of that nest.
2. The function's roots are the record's: number, argument or allocation, argument number,
   `llvm.noalias`, type.
3. No record claims to authorize a lowering. The binding's scheme is this one and its SHA-256
   is the nest's. The record's claims are at this program's index width (64 here).
4. The record is complete (every field read is there and well formed) and verified: status
   VERIFIED, ordered fold verified, no refusals, no range of any argument, Theorem 13 VERIFIED
   at SYMBOLIC with its logical tier SYMBOLIC, no other write in the frame, a box with no guard.
5. It agrees with the recognizer: no batch or demoted loop, an accumulation, three loops of
   constant bounds and step 1 from the anchor to the store, each of I, J and K the counter of
   one loop; M, N and K; the loop of each coordinate; each level's axis, direction and bounds;
   each operand's root; per dimension the loop, the scale and the constant; the element type;
   one site of `fadd` over `fmul`; the place of the accumulator in the addition; the flags of
   both operations; the source's `reassoc` flag.
6. The separations the record rests on are proofs (an allocation in this function) or the
   pairs (A, C) and (B, C), which the lowering's alias gate checks itself.

Only the dense path reads records. With the option, the steps that rewrite nests before
recognition (last-write-wins, the untiling of written tilings, the normalization of
parallelotope domains, the hoisting of repetition loops) are not run, since a record is about
a nest as written; and `general-affine`, `kernel-coset-offload`, `certified-offload-first`,
`common-fiber-offload` and `fp-distribute`, which read no records, are an error. A file that is
no file of records (another format, two objects of one function, two records of one nest, no
JSON, no file) is an error. Without the option nothing of this runs.

**In place, in order.** Each GEMM that passes is lowered by `lowerOne` where its nest stood, in
program order, as without records: two nests on one output stay two GEMMs, and the statements
between two nests stay between them. In the asynchronous lowering each GEMM's wait and the
write-back of its C come before the next statement (the test checks it for every function).

**Acceptance.**

| Case | Result |
| --- | --- |
| A. One product, binary32 with `reassoc`; the same with no flag; 32-bit integers; offsets, a scale, a transposed operand and a permuted loop order | lowered with a record (5 tiles); with no flag the record agrees and the floating-point policy refuses, as without records; integers are not recognized by the dense path and stay on the host |
| B. Two nests on one output; the same with the output zeroed by a loop before both | two GEMMs, in order, each with its record; the zeroing loop is no GEMM and stays where it is, the first GEMM reads C after it, and the two GEMMs are nests 1 and 2 of their function, as their records say |
| C. A value of C stored into D between two nests; a nest reading the other's output; three nests in a chain | each nest lowered by its own record; the load and the store stay between the two GEMMs; each GEMM is waited for before the next statement |
| D. Refusals | a stale record (an older version of the nest), a record made with no pair authorized (NOT_VERIFIED, where the lowering alone would lower by `llvm.noalias`), a record at a 16-bit index, a missing function, a missing nest, and one edited field for each check on the record (29 edits): each refused in words, the nest left as written. Another format, two objects of one function, a nest with no number, two records of one nest, no JSON, no file, and a path that reads no records: an error |
| D'. Nests of another shape | seven nests the lowering takes without records: batches, an assignment, K = 1 over two loops, a loop of step 2, a coordinate I = i + 1, a coordinate I = 2 i, a run-time offset. With the research tool's records five are refused as not VERIFIED, and the two it verifies (I = i + 1, I = 2 i) because a coordinate is no loop's counter. Each with a record carrying @single's verified content on the nest's own binding: every check on the record passes, and the check on the nest's shape refuses, in its own words |
| E. Execution | `nest_e2e.py`: four variants of records (the records; none authorized; the program changed after the records; no option) × synchronous and asynchronous lowering × 9 functions × 2 data sets, 144 runs of a function against a reference in C |

The numeric contract, by nest. Integers, and floating point left on the host: the same bits as
the reference. Floating point lowered with reassociation permitted: the device sums each tile
from zero and adds the sum to C, another order of additions, so each cell may differ from the
reference by at most 2 γ(T+2) (|c| + Σ|a b|) for its T contributions, carried through every later
statement that reads it. With small integer data every sum is exact in any order and every
cell keeps its bits; with data in [−1, 1) cells differ, and every difference is within its bound.

**Why the C compiler gets no option.** The C front end gives every array parameter an open
leading extent (`memref<?xKxf32>`). The research analysis can claim nothing of such a nest
without a finite range for that extent, so a record of it is restricted to a range, and the
gate refuses a record restricted to a range: lowering for every extent on a claim about some
would be an extrapolation. A C kernel therefore never passes the gate today, and `systolic-cc`
is unchanged; the end-to-end check runs MLIR with static shapes through the same pipeline
(`systolic-opt`, `mlir-translate`, `llc`, the software runtime model) instead. What would lift
this: a research proof that a claim does not depend on an extent that only bounds the accesses,
or a front end that knows the extent.

**Limits.**

1. Only the dense path reads records; the guarded, certified and kernel-coset paths do not,
   and are refused with the option.
2. Nests that a rewriting step would have made recognizable are not lowered with the option.
3. One GEMM per record; batches and reduction batches are not mapped (stage 1's limits stand).
4. Records are files written by the research tool; the production build does not run it. The
   tests read records checked into the tree, which `nest_gate_records.py --check` keeps equal
   to the tool's output.
5. The binding is about the nest and what it reads, not about the code around it: that is part
   of the memory and the values at the nest's entry, over which every record's claim ranges.

**Validation.** On one frozen state (`b7a30aba…`: HEAD `5c38c23` and every file this stage changes or adds,
this document excepted, which no test or build reads), each phase refusing to start on another
state and recording that it ended on the same one. Two earlier freezes were abandoned and
count for nothing: the first lacked the zeroed shared output, and the second's mutation list
declared six checks equivalent that an edited record reaches (now `nest_records_belts.mlir`).

| Check | Result |
| --- | --- |
| Production lit, the whole suite | 191 of 193; the 2 failures (`Systolic/general_affine_domains.mlir`, `SystolicCC/general_affine.c`) fail at the baseline too, with the same messages. The three gate tests pass |
| Option off | every `systolic-opt` command of the production tests, run with the baseline binary and with the new one: 199 byte for byte the same (output, messages, files written), 54 the same once the addresses of values are masked (they differ between two runs of the baseline too), 0 different; 55 commands that pass nest-records are new |
| Production unit binaries that link the changed library | `tensor-space-dependencies-test`, `rank-partition-presburger-test`: pass, with the baseline's output |
| Records | the five records files equal what the research tool says today (CTest) |
| End to end | 144 runs, 0 failures: 104 with every bit kept, 40 with lowered cells that differ, each within its bound (at most 0.232 of it) |
| Gate mutation | 48 mutants, 48 KILLED by the production tests' own checks, 1 declared equivalent |
| Research | alone on the machine (the existing lit tests use 3-second solver limits): unit 839 checks, 0 failures (53 of the binding); lit 35 of 35; the oracle, 30 functions at seeds 1 and 20261116, 0 failures, 75 bindings checked at each; against the production report, 63 fields, 60 equal, 3 differences of policy, 0 failures; mutation 68 mutants, 68 KILLED, 4 declared equivalent |
| ASan and UBSan | the gate's three tests and the end-to-end run with the sanitized `systolic-opt`; the research unit binary (839 checks, 0 failures) and the records check with the sanitized driver: no report |
| Warnings | the six sources this stage changes or adds, with `-Wall -Wextra -Wpedantic`: none but g++ 15's `-Wdangling-pointer` at the end of `toJSON(NestRecord)`, which it gives the file at `5c38c23` too |

**Not run.** The other research suites (22 `generalized-gemm-*`, 9 `polynomial-*`), whose binaries
this stage does not rebuild; every historical mutation run (the open G3b and G3a mutants, the
stale EQUIVALENT entries and the mutants not rerun at `9bb3cf0` stand as recorded); the
production unit binaries that link no changed code; `systolic-cc` beyond the production lit
suite; cosimulation and the board; a Release build of the research side. Since stage 1 the
machine's compiler changed (g++ 13 to 15): the suites not run here have not run with it.

## The thesis

The thesis text of 2026-11-06 (465 pages) makes no claim about merging loops: Definition 4.5
is "for a single, unbatched GEMM, the nest", Chapter 6 partitions one M × N region with depth
K, and neither the limitations nor the future work mention several loops. Nothing there has
to be withdrawn. What must not be added: G5b's M2 to M4b described as a capability to merge
loops, or as part of the path to the FPGA. They are joint analyses of several loops, complete
and outside the lowering.
