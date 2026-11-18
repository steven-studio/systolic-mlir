# Generalized GEMM recognition: scope of this term (2026-11-16)

A planning record, written at `d974400` from the documents and the code, with nothing built
or run. Where an earlier document lists M4c, M4d, G6 or G7 as the next step, this one takes
precedence. Stages 1, 2 and 3 of the plan were then built, and stage 3 was carried to GEMMs
written with scf.for: their sections say what was built, what was checked and what was not.

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

Stages 1, 2 and 3 are built (below). Stage 3's goal was adjusted when it was started
(2026-11-17): the lowering is driven by the research recognizer's verified coordinates and
operand mappings, and the production recognizer's success is not a condition; where the
research has no affine closed form for a view, tables of the nest's addresses over a finite,
verified domain take its place.

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

### Stage 3: the GEMM lowered from its record (built 2026-11-17)

With `--affine-matmul-to-systolic="nest-records=FILE record-driven=true"` (both off by default),
a loop nest in which the production recognizer finds no GEMM is lowered from its record. The
record is checked by itself exactly as in stage 2 (`checkRecordAtNest`, shared); what stage 2
then compared with the production recognizer is compared here with the nest itself, read from
its IR (`Systolic/NestRecordLowering.h`, `lib/Systolic/Transforms/NestRecordLowering.cpp`).
Nests the production recognizer does find stay with stage 2. Without `record-driven` nothing of
this runs; `record-driven` without `nest-records` is an error.

| File | What it is |
| --- | --- |
| `include/Systolic/NestRecordLowering.h`, `lib/Systolic/Transforms/NestRecordLowering.cpp` | the nest's own checks, the formulas, the tables |
| `include/Systolic/NestRecordGate.h`, `lib/Systolic/Transforms/NestRecordGate.cpp` | the record checks split out of stage 2's gate (`checkRecordAtNest`), and the one relaxation the tables need |
| `lib/Systolic/Transforms/AffineMatmulToSystolic.cpp` | the options `record-driven` and `record-table-budget`, the entry after stage 2's loop, and the emission |
| `test/Systolic/nest_records_driven.mlir`, `nest_records_driven_refusals.mlir`, `Inputs/nest_records_driven*.json` | the production tests and their records |
| `research/generalized-gemm/test/fuzz/nest_driven_e2e.py` | end to end: the lowered program run against the software runtime model and compared with the original |

**The first supported case.** A nest of `affine.for` loops whose body reads and writes with
`memref.load` and `memref.store`, its indices computed by `arith`. The production recognizer
reads `affine.load` and `affine.store` only, so it finds no GEMM in it and leaves it as written;
the research recognizer verifies it (I, J and K on the three loops, the views of A, B and C).
`scf.for` nests are not this case: the research recognizer proposes no candidate for them
today (the search of its coordinate families finds none), so no record of one is VERIFIED.

**What the lowering checks of the nest.** A perfect band of three `affine.for` loops, constant
bounds and step 1, the record's levels in order with the record's bounds, axes and extents; a
body of one multiply-accumulate (three loads, one `arith.mulf` of two of them, one `arith.addf`
of the product and the third, one store of the sum to the memref the third was read from) and
index arithmetic only; f32 (the device is binary32); the record's element type, update,
accumulator side and flags are the nest's; the operands are roots of the function, the record's,
of static shape and the identity layout; the separations the record rests on are proofs by an
allocation or (A, C) and (B, C). Then the lowering's own alias gate and floating-point policy,
as on the dense path, and an exact partition of the box by the plan's tiles.

**Formulas**, where the record proves an affine closed form of every view. Each index the nest
computes is read as an affine form of its loop counters (constants, the counters, `addi`,
`subi`, `muli` by a constant, `affine.apply` of such) and must be the record's form, dimension
by dimension, with as many dimensions as the memref, reading only the operand's two roles.
The C load's index must be the store's. Each dimension's indices over the box must lie inside
the memref. An operand whose two dimensions read one role each is a subview (transposed when
the roles are stored the other way), any other an explicit view (one `reinterpret_cast`, element
strides positive) whose C is checked one to one exactly over the box. Then the dense path's
planner and tile emitter.

**Tables**, where the record is VERIFIED but for the affine closed forms of some views (its
every refusal says that a view has no proposed closed form, or that a dimension of the one
proposed is not affine, as `(5 i) mod 16`), and the domain has at most `record-table-budget`
iterations (default 2^20). At compile time every iteration of the nest's static domain is
visited and the addresses of its four accesses evaluated by the nest's own index arithmetic,
in checked 64-bit arithmetic: no load, no store, no multiply-accumulate is executed, and an
address that depends on memory, on an argument or on a value computed before the nest, or that
overflows or divides by zero, is refused. The tables hold only if A's address is one per (I, K)
whatever J, B's one per (K, J), C's one per (I, J), the C load's the store's at every iteration,
every address inside its memref, no two cells (I, J) of C at one address, and the record's
proved affine forms, where it has them, agree. The forward map takes an iteration to (I, J, K)
(the counters from their lower bounds); every point has one iteration, so the map is a bijection
and each contribution is counted once. The lowering emits the tables as constant globals of
element offsets, gathers A, B and C into matrices, runs the tiles on them, and scatters C back.
A table is built at compile time for the one static domain of the nest, which every run has:
nothing is sampled and nothing is said of another domain. Tables built at run time are not made.

**Acceptance.** `nest_records_driven.mlir`: eleven nests the production recognizer leaves as
written are lowered from their records (formulas: the counters as indices, a transposed B with
the loops permuted, flat arrays, offsets and a scale, `affine.apply`; tables: flat arrays the
research gives no closed form for, A's rows in the order `(5 i) mod 16` by `arith` and by an
`affine.load` map; two nests on one output; a stage 3 nest after a stage 2 nest on one output),
and the one with no `reassoc` is refused by the floating-point policy. `nest_records_driven_refusals.mlir`:
32 nests one change away from a GEMM the lowering takes, 62 refusals, each in its own words: 5
on the research tool's own records (no `llvm.noalias`, two statuses, an explicit view that runs
backwards, binary64) and 57 with a record transplanted onto the nest's binding or edited so that
every check before the one under test passes; and one table lowering on the research's own
record (`(5 i) mod 16`). `nest_driven_e2e.py`: nine functions × five variants (the records with
`record-driven`; without it; no records; the program changed after the records; no pair
authorized) × synchronous and asynchronous lowering × two data sets, each run compared with a
reference in C by the numeric contract of stage 2.

**Limits.**

1. Only `affine.for` bands of three loops with constant bounds and step 1; `scf.for` and other
   loops, batches and reduction batches are refused (and the research verifies no `scf.for`
   nest today).
2. The body is one multiply-accumulate in f32 and index arithmetic; assignments, scalar
   accumulators, other element types are refused.
3. Formulas: explicit views with a negative or zero element stride are refused (a view that
   runs backwards is lowered by a subview when each dimension reads one role).
4. Tables: static domains of at most `record-table-budget` iterations; addresses computed from
   the counters and constants by the admitted operations in index or i64; the program's index
   width 64. No run-time tables: a domain or an address known only at run time is refused.
5. The tables are constant globals in the binary (one 64-bit offset per element of each
   operand's matrix) and the gathered matrices are temporaries the size of the operands.
6. As in stage 2: records are files written by the research tool; C kernels get no option.

**Validation.** On one frozen state (`11208045…`: the checkpoint `548aea3` and the one file
changed after it, the mutation harness; this document excepted, which no test or build reads),
each phase refusing to start on another state and recording that it ended on the same one. The
baseline binary is built from `5c38c23`, before stage 2: with both options off, the tool is the
one before stage 2.

| Check | Result |
| --- | --- |
| Production lit, the whole suite | 193 of 195; the 2 failures (`Systolic/general_affine_domains.mlir`, `SystolicCC/general_affine.c`) fail at the baseline too, with the same messages. The five gate tests pass |
| Option off | every `systolic-opt` command of the production tests, run with the baseline binary and with the new one: 200 byte for byte the same (output, messages, files written), 54 the same once the addresses of values are masked (they differ between two runs of the baseline too), 0 different; 120 commands that pass nest-records or record-driven are new. A first run counted 1 different, the command that passes record-driven without nest-records: both binaries refuse it, the baseline as an unknown option. The comparison now leaves out record-driven as it left out nest-records (both runs kept) |
| Production unit binaries that link the changed library | `tensor-space-dependencies-test`, `rank-partition-presburger-test`: pass, with the baseline's output |
| Records | the seven records files equal what the research tool says today (CTest) |
| End to end, stage 3 | 180 runs, 0 failures: 146 with every bit kept, 34 with lowered cells that differ, each within its bound; 10 lowered modules, their calls in program order |
| End to end, stage 2 | 144 runs, 0 failures: 104 with every bit kept, 40 within their bound, as at stage 2 |
| Against the production report | 63 fields, 60 equal, 3 differences of policy, 0 failures, as at stage 2 |
| Gate mutation | 107 mutants (stage 2's 48 and 59 new), 107 KILLED by the tests' own checks: 100 by a check that fails, 7 by a check whose input is empty (the remark or error it expects never came; the harness counts this as a kill since this stage, once each such run's output was seen to hold FileCheck's empty-input error for that check and no crash; before, it was a harness error); 0 crashes; 8 declared equivalent, each with its reason |
| ASan and UBSan | the five gate tests and both end-to-end runs with the sanitized `systolic-opt`: no report |
| Warnings | the three production sources this stage changes or adds, with `-Wall -Wextra -Wpedantic`: none |

**Not run.** The research unit binary, lit suite, oracle and nest mutation: no research
source changed in this stage (its build has nothing to do), only test scripts and the list of
CTests. The other research suites; every historical mutation run (the open G3b and G3a mutants,
the stale EQUIVALENT entries and the mutants not rerun at `9bb3cf0` stand as recorded); the
production unit binaries that link no changed code; `systolic-cc` beyond the production lit
suite; cosimulation and the board. Not mutated: the guards that stand only between a malformed
input and a crash (a null loop in the band walk, a band of another size reaching the role
lookup, a role of -1, the evaluator's missing value and its division by zero, an operand's
missing index forms, `admit` of no operation, and the exclusion of the nests stage 2 lowers,
without which stage 3 would read a nest stage 2 has already erased), since removing one makes the tool crash, which is
no valid kill; and the check that the tiles partition the box exactly, which no plan the planner
makes can fail. Not built: tables at run time, and a `scf.for` nest through this path (the
research verifies none).

### Stage 3 on scf.for: GEMMs written with scf.for (built 2026-11-17)

**The request.** A GEMM written with `scf.for` goes the whole way: the research recognizes and
verifies it, writes the nest's record, and the record-driven lowering turns it into tiles and
runtime calls, with no rewriting into `affine.for` asked of the programmer.

**Where it failed, stage by stage.** One GEMM (16 x 8 x 12, `memref.load` and `memref.store`,
the counters as indices, `reassoc`) written with `affine.for` and with `scf.for`:

| Stage | affine.for | scf.for, before |
| --- | --- | --- |
| Importer (affine and scf lowered to control flow) | modelled | modelled; the bounds and steps are values defined before the nest, which the importer keeps in state, each set once on entry, where `affine.for`'s lowering sets them as each loop starts |
| Outline (a private copy of the nest) | outlined | outlined, the five constants copied in |
| Termination | proven if defined | proven if defined |
| Coordinate search (G3b) | 1 candidate | none: UNKNOWN |
| Domain, computation, Theorem 13, views | verified, symbolic | not reached |
| Binding | the nest's | the nest's |
| Production | stage 2 (the recognizer) | stage 3 refused it: "scf.for is no affine.for" |

The coordinate search tells operands apart by their values at a thin sample of the events
(one in `events/128`, here one in 12). The innermost loop runs 12 times, so at every sample its
counter is 0, the value of the bound `%c0` that `scf.for` keeps in state and that is offered
first: the counter was dropped as a copy of `%c0`, and `%c0` as a constant. No three coordinates
were left, at any grammar level. When that was mended, a GEMM whose bounds do not start at 0
still failed: the coordinates' range (obligation V3) could not be proved, because the
termination certificate had no lower bound for a counter that starts at a state holding a
constant (`affine.for` starts it at the literal).

**What changed.** No check was weakened; nothing was normalized: the nest is read as written,
its record is of the nest as written (its binding), and no affine form is assumed of it.

1. `CoordinateGrammar` (research): a state variable that the thin sample cannot tell from an
   earlier operand is compared on every event; if the two differ there, it is kept under
   `GrammarSpec::exactLeaves` and counted as lost without it. The grammar is a guess; the
   checkers decide.
2. `SynthesisAnalysis` (research): a group's search runs as before. It runs once more, with
   `exactLeaves`, only when the scope is enumerated whole (a search then adds nothing to the
   driver's state), the first search lost a state variable, and it verified no candidate at the
   symbolic tier; the second outcome is taken only when it verifies one. Every other search says
   what it said, at the same cost (the synthesis unit test: 556 s, 553 s at the baseline).
3. `TerminationProposers` (research): a state that no loop carries and that takes one closed
   value stands for that value inside another state's value, so a counter that starts at an
   entry-block constant held in state gets the facts a literal start gets (`j >= 1`). These are
   proposals: each fact is checked like any other, and kept only if it holds on every path.
4. `NestRecordLowering` (production): the band is `affine.for` or `scf.for` loops. For
   `scf.for`: bounds that are constants (`arith.constant`), step 1, an unsigned comparison only
   of bounds that are not negative; between two loops, constants only. A loop of the nest that
   carries values (`iter_args`) is refused before the band is read. Everything after the band is
   stage 3's: the record by itself, the levels, the body, the operands, the formulas or tables,
   the alias gate, the floating-point policy.

**Acceptance.** `nest_records_scf.mlir`: one GEMM written with `scf.for`, with `affine.for` and
`affine.load` (stage 2) and with `affine.for` and `memref.load` (stage 3), each with its own
record and tiles; `scf.for` nests lowered by formulas (bounds before the nest or between the
loops, B transposed with the loops permuted, flat arrays, offsets and a scale, bounds that do not
start at 0) and by tables (flat arrays whose index constants are defined before the nest, A's
rows in the order `(5 i) mod 16`); two `scf.for` nests on one output; a nest that reads what the
one before wrote (T = T + A B, then C = C + T D), the second GEMM begun after T is written back;
and the refusal of the floating-point policy. `nest_records_scf_refusals.mlir`: on the research
tool's own records, the alias gate, the status of the records the research does not verify
(`iter_args`, a read outside A, a second write, a call, a bound known at run time, a step of 2,
`scf.parallel`, unsigned loops from or to a negative bound), the floating-point contract; and on
records transplanted onto each nest, the refusal of each check of the band and of the body
(`iter_args`, a bound that is no constant, a step other than 1, unsigned from and to a negative
bound, `scf.parallel`, a second store, a call, a read outside A by the formulas). Refused nests
stay as written. `nest_scf_e2e.py`: thirteen functions x the five variants of stage 3 x
synchronous and asynchronous lowering x two data sets, run on the software runtime model and
compared with a reference in C that the original program meets too (the variant with no
records runs it). Research: `nest_scf.mlir` (the search and the record of the `scf.for` GEMM,
the shifted one, and a search the second pass leaves as it was); the nest oracle with every
product written with `scf.for` (`generalized-gemm-nest-programs-scf`).

**Limits.**

1. `scf.for` with a step other than 1 is refused; the research proposes the coordinates of a
   step of 2 but does not verify them symbolically.
2. Bounds known only at run time are refused (the research does not check Theorem 13 for them).
3. `iter_args` (a reduction held in a value) is refused; the research makes no claim of such a
   nest ("a scalar accumulator has no exact output view").
4. `scf.parallel`, `scf.while` and other loops are refused; an unsigned `scf.for` is taken only
   when its bounds are not negative. (Counting `scf.while` loops are taken since the next stage,
   below.)
5. The limits of stage 3 hold: one f32 multiply-accumulate, three loops, static shapes, no batch.

**Validation.** On one frozen state (`b9bb409d…`: HEAD `7547bbb` and every file this stage
changes or adds, this document excepted), each phase refusing to start on another state and
recording that it ended on the same one. A first freeze (`e461b243…`) was abandoned in its
research phase: the synthesis unit test, 553 s alone at the baseline against its limit of 600
s, timed out beside another test, and the second search, then run after every search that
verified nothing symbolically, cost such searches 40 to 50 per cent; it was gated as said above
(556 s afterwards). The termination, coordinate and identity binaries are byte-identical in the
two freezes, so their CTests' results in the first stand.

| Check | Result |
| --- | --- |
| Production lit, the whole suite | 195 of 197; the 2 failures (`Systolic/general_affine_domains.mlir`, `SystolicCC/general_affine.c`) fail at the baseline too, with the same messages; the seven `nest_records` tests pass |
| Option off, against HEAD (`7547bbb`, built from its archive) | every `systolic-opt` command of the production tests that does not pass record-driven: 259 byte for byte the same, 54 the same once the addresses of values are masked, 0 different; 79 pass record-driven |
| Production unit binaries | `tensor-space-dependencies-test`, `rank-partition-presburger-test`: the baseline's output |
| Research lit | 36 of 36 (`nest_scf.mlir` among them) |
| Research CTests the change reaches | 18 rerun on this freeze: all pass, `generalized-gemm-synthesis` (alone, 472 s), the four nest oracles and end-to-end runs among them; from the first freeze, binaries unchanged: 6 pass, `generalized-gemm-termination` fails as at the baseline (the same 3 of 119442 checks: `t3_swap` UNKNOWN at 8 bits) |
| Records | the nine records files equal what the research tool says today |
| End to end | the three CTests pass with the plain tool; their counts, from the same scripts run with the sanitized tool: stage 2, 144 runs, 0 failures; stage 3, 180, 0; `scf.for`, 260 runs, 0 failures, 210 with every bit kept, 50 with lowered cells that differ within their bound, 10 lowered modules with their calls in program order |
| The nest oracle, products written with `scf.for` | 30 programs, 0 failures |
| Research outputs against the baseline | 101 research lit commands: 97 byte for byte the same; 3 are `nest_scf.mlir`'s `scf.for` searches; `dom_phase4.mlir`'s `@rotated`, itself an `scf.for` program, is the same once term numbers are masked |
| Gate mutation | 29 mutants (12 new, 2 brought to the new code, 5 others of the band and level checks, 10 that stage 3's changed checks had killed): 29 KILLED by the tests' own checks |
| Search mutation | 13 mutants of the search change: 10 KILLED by `nest_scf.mlir`'s checks; 3 SURVIVED, all three about when the second search runs and what it carries (a state variable equal on every event counted as lost; the second search run after a symbolic first one; run on a sampled scope): the second outcome is still taken only when the unchanged checkers verify it symbolically, so they bear on cost and completeness, not on a check, but no test pins them and their equivalence is not proved; 2 declared equivalent with their reasons |
| ASan and UBSan | the seven `nest_records` tests and the three end-to-end runs with the sanitized `systolic-opt`; the sanitized nest driver remade the nine records files, each equal to the file in the tree, and the sanitized drivers passed the checks of `nest_scf.mlir`: no report |
| Warnings | the four sources this stage changes, with `-Wall -Wextra -Wpedantic`: none, as at HEAD |

**Not run.** The research CTests the change does not reach (`generalized-gemm-unit`,
`-relational`, `-oracle-fuzz`, `-native-differential`: their binaries hold none of the changed
sources) and the polynomial suites; the 89 other gate mutants of stages 2 and 3 (killed at
`7547bbb`; neither their code nor the checks that killed them changed) and every historical
mutation run; the research's older mutation harnesses (`synthesis_mutation.py` and the others
count any failure as a kill, a crash included, and build with more jobs than this stage
allowed); `systolic-cc` beyond the production lit suite; cosimulation and the board. Not built:
steps other than 1, bounds known only at run time, `iter_args`, `scf.parallel` and `scf.while`
(`scf.while`: the next stage).

### Stage 3 on scf.while: GEMMs written with scf.while (built 2026-11-18)

**Request.** After `scf.for`, a GEMM written with `scf.while` must go the same way to the end:
verified by the research, recorded, lowered from its record, tiled, run. The first target: three
counting `scf.while` loops with constant bounds and step 1, `memref.load` and `memref.store`, the
f32 multiply-accumulate under the existing floating-point contract, bounds that do not start at
0, and nests that mix `scf.while` with for loops; nobody rewrites the loops as `affine.for`.
What an `scf.while` carries from one iteration to the next is part of its meaning and is read,
not refused wholesale: its counter is taken, and every other carried value is judged for what
it is. Nothing is normalized: the nest is read as written, in both tools, so the record stays
bound to the input.

**Where it stopped.** One GEMM written three ways (`@whl_gemm`, `@whl_twin_scf`,
`@whl_twin_affine` of `nest_records_while.mlir`), stage by stage, at `771b700`:

| Stage | `scf.while` |
| --- | --- |
| Importer | lowers `scf.while` to control flow as it does `scf.for`. The after region's argument is one more state variable, which no loop carries and which holds the counter in the body |
| Outline | one nest per outermost loop (`"loop": "scf.while"`); a loop whose result is used after it has no faithful copy and is refused |
| Termination | a certificate (guard distance) for `counter < bound` loops; for `counter != bound` loops too, where the accesses bound the counter (limit 1) |
| Coordinate search, domain and computation checkers | VERIFIED, with the second search of the `scf.for` stage; bounds that do not start at 0 and mixed nests included |
| Views | **gap.** The body's indices are written over the after region's argument, a state that is no level. A counter used as it is got its level by the events; an index computed in the body (an offset, a scale, a flat array's `row * columns + column`) got no closed form: VERIFIED_NOT_MAPPED |
| Record and binding | as for any nest |
| Production lowering | **gap.** Every loop with a result was refused as carrying values (`iter_args`), the counter of an `scf.while` with them |

**The changes.**

1. `NestRecordLowering` (production): a loop of the band may be a counting `scf.while`
   (`readCountingWhile`), read as the loop it is:
   - it carries one value, of type `index`, its counter, started at an `arith.constant`;
   - its before region holds constants and one `arith.cmpi`; `scf.condition` goes on by that
     comparison and hands the region's argument, the counter, on as it is;
   - the comparison is `counter < bound` (`slt` or `ult`, the counter first) or
     `bound > counter` (`sgt` or `ugt`, the bound first); the bound is an `arith.constant`; an
     unsigned comparison is taken only when neither the start nor the bound is negative. (The
     reader also reads `counter != bound`, with start <= bound; that is no supported form, and
     is recorded apart below);
   - its after region, whose argument is the counter the body reads, yields `counter + 1`, an
     `arith.addi` of that argument and the constant 1 (either way round) in that region;
   - its result, the counter's last value, has no use.
   Each condition that fails refuses the nest with its own reason. The addition that steps the
   counter is the one operation, beside constants, that a loop's body may hold next to the
   inner loop. Everything after the band is stage 3's, unchanged: the record by itself, the
   levels, the body, the operands, the formulas or tables, the alias gate, the floating-point
   policy, the emission.
2. `DomainProposer` (research): a view's index term that reads a state that is no level (at
   most three) is tried with each such state read as each level of its sort, and the first
   reading under which the term is the recorded index at every sampled event of every site is
   proposed. It is a proposal like the others: the checker proves the form over the scope, or
   the view is not proved (and the nest goes by tables, or not at all).
3. Nothing else changed: the importer, the outline, the termination certificate, the search,
   the checkers and the binding are `771b700`'s.

**Why this keeps the meaning.** The lowering replaces the nest by tiles over the box of its
counters. For a loop read as above: the before region has no effect (constants and one
comparison); the body sees exactly the compared value; the counter starts at `lower`, grows by
1, and the loop stops the first time the comparison fails, so the body runs once for each value
`lower, ..., upper - 1`, in that order, and no value of the counter leaves `[lower, upper]`, so
nothing wraps (with `ne`, because start <= bound; unsigned, because neither is negative); the
loop's only result is unused, so removing the loop removes nothing the program reads. That is
what an `scf.for` from `lower` to `upper` with step 1 does to memory, which stage 3 lowers. No
operation is rewritten to reach this reading, so the binding (the function's name, its roots,
the nest's own text and location) is computed on the input in both tools, and a record made
for another text does not bind. What allows the replacement is proved, not sampled, on the
`scf.while` program itself: the termination certificate (invariants and ranks checked on every
path), the coordinates' validity and coverage over the scope, the views' closed forms, the
ordered fold (Theorem 13), the frame (no other write), on the research side; and, read again
from the IR by the lowering, the band, the one multiply-accumulate, the accesses inside their
arrays, the alias gate and the floating-point policy.

**Carried values, one by one.**

| The loop carries | Research | Lowering |
| --- | --- | --- |
| its counter only, as above | VERIFIED | taken |
| nothing (it goes on by a flag read from memory) | no claim: the trace depends on memory contents | refused: no counter |
| a second index stepped by a constant, never read (`@second_induction`) | VERIFIED | refused at `acb801a`: "value 1 (index) is stepped by a constant". Taken since the lowering reads carried indices (below, "Subscripts a loop carries") |
| a second index handed round unchanged, the bound (`@invariant`) | VERIFIED | refused: the comparison reads two carried values; "value 1 (index) is carried unchanged" |
| the reduction, in an f32 beside the counter (`@accumulator`) | no claim: a scalar accumulator has no exact output view | refused: "value 1 (f32) is updated by the loop: a reduction or datum held in a value, not in memory" |
| a counter of type `i64`, cast to `index` for the accesses (`@i64`) | VERIFIED | refused: the counter is no `index` |
| a counter whose last value is used after the loop (`@result_used`, `@inner_result_used`) | refused: no faithful copy, or a write in the frame | refused: the lowering would remove the loop that makes it |

**Acceptance.** `nest_records_while.mlir` (22 functions): the GEMM written with `scf.while`,
`scf.for` and `affine.for`, each lowered from its own record; pure `scf.while` nests with the
bounds before the nest or in the loop bodies; mixed nests (`scf.for` around `scf.while` around
`scf.for`, `scf.while` around `scf.for` around `scf.while`, `affine.for` around `scf.while`
around `scf.for`); the other spellings of the counting loop (`bound > counter` with
`1 + counter`, unsigned comparisons, the bound a constant of the before region); bounds that
do not start at 0, pure and mixed; B stored transposed with the loops in the order k, i, j;
flat arrays and offsets with a scale, by formulas; tables (flat
arrays whose index constants are defined before the nest, A's rows in the order `(5 i) mod 16`);
two `scf.while` nests on one output; a nest that reads what the one before wrote (T = T + A B,
then C = C + T D), the second GEMM begun after T is written back; and the nest the
floating-point policy keeps as written. Those are 20 functions; the two others, `@whl_en` and
`@whl_ne_shifted`, are the `!=` loops recorded apart below, and are no part of the acceptance.
`nest_records_while_refusals.mlir` (30 functions, each one change away from the GEMM): on the
research tool's own records, a loop that never ends, wrong updates, other conditions, side
effects, reads outside A, bounds known at run time, results used after the loop, other carried
values, the alias gate and the floating-point contract; and, on a verified record transplanted
onto each nest, the refusal of each check of the counting loop and of the band. Refused nests
stay as written. `nest_while_e2e.py`: the 22 functions x the five variants of stage 3 x
synchronous and asynchronous lowering x two data sets, run on the software runtime model and
compared with a reference in C that the original program meets too (the variant with no
records runs it). Research: `nest_while.mlir` (the record of the `scf.while` GEMM, shifted,
mixed, with computed indices; and the forms it does not verify); the nest oracle with every
product written with `scf.while`, and with each loop one of the three kinds
(`generalized-gemm-nest-programs-while`, `-mixed`): every nest is held to its own record there,
two nests on one output and a nest that reads what the one before wrote included.

**Recorded apart: forms that are not supported.** They do not bear on the acceptance above, and
nothing here is to be read as support. (As this stage left them. Both are verified and lowered
since the next one, below, which is also where limit 4 ends for small nests.)

1. *Loops that go on by `counter != bound`.* The lowering's reader reads them (start <= bound).
   The research verifies one only where the accesses bound the counters as the loops do: a
   start of 0 and bounds that are the arrays' own sizes (`@whl_en`: an access outside its array
   stops the run, which gives the range), and such a nest is then lowered, every check having
   passed. With another start (`@whl_ne_shifted`, `@while_ne_shifted`) or arrays larger than
   the loops' bounds, the research has no candidate and the nest stays as written: that the
   counter stays inside its bounds needs the fact that it stays at or below its bound, which
   the termination certificate does not hold for a `!=` loop.
2. *A counter stepped before the inner loop* (`@step_first`, `@while_step_first`). The lowering's
   reader reads it (the addition may stand anywhere in the loop's body). The research has no
   termination certificate: the stepped value is held across the inner loop, and no invariant
   ties it to the counter. The nest stays as written.

**Limits.**

1. Carried values other than the counter are refused, as in the table above: the research
   verifies three of those forms (a second induction, an invariant, an `i64` counter) and the
   lowering does not read them; a reduction carried in a value is not modelled by the research.
2. Arithmetic in the before region (`@before_compute`: the research verifies it), conditions
   other than the two above (`<=`, `>=`, a counter counted down), steps other than 1, bounds
   or starts known only at run time: refused.
3. A view whose index reads constants defined before the nest still has no closed form (the
   constant is a state, as in the `scf.for` stage) and goes by tables.
4. A function of several `scf.while` nests has no termination certificate *as a whole* (below),
   so the whole-function driver makes no claim over two nests on one output (UNKNOWN; with for
   loops, VERIFIED). No record and no lowering rests on that claim: each nest is outlined and
   verified by itself, and `@whl_shared` and `@whl_chain` are lowered nest by nest. The nest
   oracle counts such a claim under its own name, as no verification, and asks instead, in
   every mode, for two nests on one output each with its own complete VERIFIED record.
5. A nest with a loop of one iteration has no complete record, as with for loops (stage 1's
   limit); for one such product of the nest oracle the research stops earlier with `scf.while`
   (NOT_VERIFIED, Theorem 13 not checked) than with `scf.for` (VERIFIED_NOT_MAPPED). Neither is
   lowered.
6. The limits of stage 3 hold: one f32 multiply-accumulate, three loops, static shapes, no batch.

**For the invariant work, when it is joined.** The two forms recorded apart and limit 4 are the
termination certificate's proposer (`Proposer::invariantCandidates`, `TerminationProposers.cpp`)
not proposing a fact. The certificate checks every proposed conjunct on every path and drops
what does not hold, and the finished certificate is checked again by itself, so the interface
is a list of candidate conjuncts per cut, from any source; no check changes. Each was tried on
a scratch copy of the tree, which is no part of this stage (patches, inputs and outputs are
kept with the stage's report, under `followups/`):

| Case | What is missing | Candidate | With it, on the scratch copy |
| --- | --- | --- | --- |
| `@whl_ne_shifted`, `@while_ne_shifted` (and `!=` loops in arrays larger than their bounds) | `counter <= bound` for a `!=` loop | for a path condition `x != y`: `x <= y` and `y <= x`, signed and unsigned, at every cut | `counter <= bound` and `counter >= start` are kept; `@whl_ne_shifted` is VERIFIED |
| `@step_first`, `@while_step_first` | `temporary = counter + step` | for a state given the sum of two other states along a path: that equality | `PROVEN_TERMINATING_IF_DEFINED`; the nest is VERIFIED |
| a function of two `scf.while` nests (the nest oracle's `p0`, `p10`, `p20` with `--loops while`, `p10` with `--loops mixed`; `@whl_shared`, `@whl_chain` taken whole) | in an `scf.while` body, `after region's argument = counter` | for a state that a path gives exactly another state's value: that equality, before the orderings | all six terminate with both limits as they are; the claim over both groups is VERIFIED in the oracle's four programs |

The third was measured (`followups/candidate-limit/`). An `scf.while` has two variables that a
loop carries, its counter and the after region's argument, and the rank needs their equality in
the body. The proposer has that fact only among its orderings of pairs of such variables: it
makes them last, cuts the list at 600 candidates, and orders the first eight such variables
only. The oracle's `p0` written with `scf.for`: 6 such variables, 514 candidates, none cut,
terminating. Written with `scf.while`: 12, of which 8 are ordered; 764 candidates, 164 cut, all
of them orderings (the second nest's outer loop keeps none); no rank at L10. With the limit at
2000 alone the failure moves to L12 (the second nest's inner loops are not ordered at all);
with 16 ordered variables alone more is cut (1372 candidates) and L10 fails as before; with
both, it terminates. So the cut at 600 is the first obstacle and not the only one, and raising
limits is not the remedy: 20 equalities, made before the orderings, do it with both limits
unchanged. A single nest stays inside both limits (171 to 402 candidates over the 20 single
nests of `nest_records_while.mlir`, none cut), which is why every record above is verified.
A reduction carried in a value (`@accumulator`) needs more than an invariant: a model of the
carried reduction as the output's cell.

**Validation.** On one frozen state (`dae79d79…`: HEAD `771b700` and the 16 files this stage
changes or adds, this document excepted), each phase refusing to start on another state and
recording that it ended on the same one. A first freeze (`40f21efc…`) was stopped by hand after
two minutes, three phases in: two comment lines of the new test files named a script that is not
in the repository; they were rewritten and the tree frozen again. Builds, scripts and logs are in
a directory outside the repository (`~/work/integration-backups-20261118/gemm-while`), two build
jobs at most.

| Check | Result |
| --- | --- |
| Production lit, the whole suite | 197 of 199; the 2 failures (`Systolic/general_affine_domains.mlir`, `SystolicCC/general_affine.c`) fail at HEAD too, with the same messages; the nine `nest_records` tests pass |
| Against HEAD (`771b700`, built from its archive) | every `systolic-opt` command of the production tests HEAD has, the records they read from temporaries made first: without record-driven, 259 byte for byte the same, 54 the same once the addresses of values are masked (they differ between two runs of HEAD's tool too), 0 different; with record-driven, 78 the same and 1 different, the message that names the loops the lowering takes (`scf.parallel`'s refusal); the 32 commands of the two new files are not compared |
| Production unit binaries | `tensor-space-dependencies-test`, `rank-partition-presburger-test`: HEAD's output |
| Research lit | 37 of 37, alone on the machine (`nest_while.mlir` among them) |
| Research CTests the change reaches | 16, all pass: `nest`, `domain`, `domain-holes` (the unit tests of the two changed drivers), `domain-programs`, `setdomain-programs`, `setreduction-programs`, the nest oracle four times, `nest-affine`, `nest-gate-records`, the four end-to-end runs |
| Records | the eleven records files equal what the research tool says today (and what the sanitized driver says) |
| End to end, plain tool | `scf.while`: 440 function runs, 0 failures, 362 with every bit kept, 78 with lowered cells that differ within their bound, 10 lowered modules with their calls in program order; the 20 functions of the supported scope are 400 of the runs, the two `!=` functions 40. Unchanged from the stages before: stage 2, 144 runs, 0 failures; stage 3, 180, 0; `scf.for`, 260, 0 |
| The nest oracle, 30 programs, each spelling of the loops | `affine.for`, `scf.for`, `scf.while`, mixed: 0 failures each; in all four, 47 records VERIFIED with every field the program's, 188 nests equal to their record's fold, 120 compositions in place equal to the function, 3 shared outputs with each nest verified by its own record. The joint claim over a shared output: verified 3, 3, 0, 2; not made (counted apart, no verification) 0, 0, 3, 1. One product with a loop of one iteration differs (limit 5) |
| Research outputs against the baseline | 111 research lit commands, baseline binaries (HEAD's sources) against the new: 110 byte for byte the same; the one that differs is `nest_while.mlir`'s `@while_offsets`, VERIFIED_NOT_MAPPED before (no proposed closed form) and VERIFIED with three proved views now |
| Gate mutation | 38 mutants of the band (24 new, of the `scf.while` reader; 3 brought to the new code; 11 others of the band): 38 KILLED by the tests' own checks, none by a crash |
| Mutation of the research change | 2 mutants of the proposer's new reading: 2 KILLED (`nest_while.mlir`'s OFFSETS; `dom_tables.mlir` and `nest_while.mlir`) |
| ASan and UBSan | the nine `nest_records` tests, the `scf.while` and `scf.for` end-to-end runs (440 and 260 runs, 0 failures) with the sanitized `systolic-opt`; the sanitized nest driver remade the eleven records files, each equal to the file in the tree, and passed the ten commands of `nest_while.mlir`: no report |
| Warnings | the two sources this stage changes, with `-Wall -Wextra -Wpedantic`: none, as at HEAD |

**Open, from the stage before.** The three search mutants that survived at `771b700` (a state
variable equal on every event counted as lost; the second search run after a symbolic first one;
run on a sampled scope) are as they were: this stage changes neither their code nor a test that
bears on them, they were not rerun, and no test pins them yet.

**Not run.** The research CTests the change does not reach (`unit`, `relational`, `termination`,
`coordinates`, `synthesis`, `computation`, the oracles of those drivers and `table-programs`:
their binaries hold none of the changed sources; `termination` fails at the baseline, as said
above) and the polynomial suites; the 104 other gate mutants (killed at `771b700` or before;
neither their code nor the checks that killed them changed) and the 13 other search mutants;
the stage 2 and stage 3 end-to-end runs under the sanitizers (run with the plain tool); the
research's older mutation harnesses; `systolic-cc` beyond the production lit suite;
cosimulation and the board. Not built: the forms recorded apart and the limits above.

### Stage 3 on scf.while, continued: the certificate's relations (built 2026-11-18)

**Request.** The three facts that the stage before left to the invariant work are to be made by
the termination certificate's proposer itself: `counter <= bound` for a loop that goes on while
its counter differs from its bound, `temporary = counter + step` for a counter stepped before
the loop inside, and the equality of an `scf.while`'s body argument with its counter, which a
function of several `scf.while` nests needs as a whole. Research side only. Whether the added
candidates run into the limit of 600 on the candidates, and whether a case that passed turns
UNKNOWN, is to be watched, and the limit or the order looked at if so.

**What was added** (`Proposer::invariantCandidates`, `TerminationProposers.cpp`). Three kinds of
relation, read off the program's own paths between cuts:

1. *A copy.* A state whose every change, along every path, gives it the value of one other
   state that holds no constant: the equality of the two. The argument of an `scf.while`'s
   after region is given the loop's counter at each iteration, and the rank of the loop needs
   them equal in the body.
2. *A stepped counter.* A state that some path copies into a loop-carried variable, and that a
   path gives the sum of another state and a state or a constant: that equality. A counter
   stepped before the loop inside is held as such a sum across that loop.
3. *A `!=` comparison.* For a comparison `x != y` (or `x == y`) of the path conditions between
   a state and a state, a parameter or a constant: `x <= y` and `y <= x`, signed and unsigned.
   That a counter stays inside its bounds needs the fact that it stays at or below its bound,
   and nothing else says so of a `!=` loop.

They are candidates like the others: the refinement keeps one only where the solver shows it
on every path into a cut, and the finished certificate is checked again by itself. No check
changed, and no limit was raised for the candidates that existed. Each rule is narrower than
the experiment it came from, so that a program with none of these shapes gets exactly the
candidates it had: a copy from a state that holds a constant gets nothing (the candidates
already hold its equality with the constant), nor does a sum that steps no counter, nor a
comparison of computed values (the overflow check of an index's multiplication is one).

**The limit of 600.** The relations are made *after* the cut at 600 and have a bound of their
own (a quarter of the limit, 150). So the candidates made before them are made, and cut,
exactly as they were: no case can lose a candidate it had, whatever the relations add. Placed
before the cut instead, as in the experiment of the stage before, each relation would push one
ordering past the limit in a program already over it. Measured with an instrumented scratch
copy, relations off and on (`measure/` of the round's report):

| Programs | Candidates made before the relations | Cut at 600 | Relations made (bound 150) | Queries of one proposer run (budget 2500) | Result |
| --- | --- | --- | --- | --- | --- |
| the 111 driver commands of the research lit tests as they were at `acb801a` | at most 500 | none | at most 34; 98 commands make none | largest 962 -> 344; sum 6510 -> 5596 | the output of 3 commands differs: the two nests this stage is for, and the proposer's own statistics for `term_verdicts.mlir`'s `@piecewise` (more invariants kept; UNKNOWN as before) |
| the nest oracle's functions of two products, taken whole, with `affine.for` and with `scf.for` (12) | 506 to 526 | none | 0 | the same number in each | the same claims |
| the same with `scf.while` (6) | 764 to 776 | 164 to 176, the same with the relations as without | 20 | 411..1012 -> 218..241 | the claim, not made before, is VERIFIED (3 shared outputs) or REFUTED (3 with a cross read), as with for loops |
| the same with mixed loops (6) | 611 to 746 | 11 to 146, the same with the relations as without | 4 to 10 | 233..244 -> 230..246; `p10`: 1240 -> 221 | five keep their claims; `p10`'s, not made before, is VERIFIED |
| `@ne_carried` of `term_relations.mlir` | 679 | 79 | 22 | 123 -> 124 | no "if defined" left in the verdict |

Several programs were over the limit before this stage and still are; in each the cut removes
the same number of candidates with the relations as without, and no case that passed turns
UNKNOWN. So the limit stays at 600 and the order above it as it was. The first form of the third
relation, which answered every `!=` comparison, also made 8 to 16 candidates in affine programs
from the overflow check of an index's multiplication (no result differed); it was narrowed to
comparisons between a state and a state, a parameter or a constant, and the measurements above
are of the narrowed rule (the first ones are kept beside them).

**What it gives.**

- Loops that go on while the counter differs from its bound are verified from any start and in
  arrays larger than the loops' bounds, and lowered: `@whl_ne_shifted`, `@whl_ne_bigger`
  (`@whl_en`, from 0 in arrays of the loops' sizes, was verified before).
- A counter stepped before the loop inside is verified and lowered: `@whl_step_first`,
  `@whl_step_first_shifted` (mixed with an `scf.for`, bounds that do not start at 0); the step
  a constant from before the nest or of the after region itself.
- A function of two nests of `scf.while` loops has its termination certificate as a whole
  (`@two_nests`, and `@whl_shared` and `@whl_chain` taken whole). For small nests the
  whole-function driver's claim over both is then made and verified, and the nest oracle asks
  for it again with `scf.while` and mixed loops as it does with for loops: the count of claims
  not made is gone with the claims.
- The stage before's two forms recorded apart are supported forms now, and its limit 4 holds
  only as said next.

**Limits.**

1. Over two *larger* nests of `scf.while` loops (16 x 8 x 12 each, `@two_nests`) the
   whole-function claim is still not made, the gate being open: the proposer of a segment's
   guard stops ("too many patterns for a piecewise guard"); the same function written with
   `scf.for` gets its claim. That is another part of the joint analysis, not looked into here.
   No record and no lowering rests on a claim over several nests.
2. Two conditions of the relations are pinned by no test, because no result depends on them:
   that a copy has one source over all paths, and the relations' own bound of 150 (34 is the
   most any measured program makes). Every other condition has a test that fails without it.
3. Carried values other than the counter, the conditions and steps the lowering does not read,
   and every other limit of the stage before stand.
4. One sentence of a comment was left out of date by this stage, which changed research-side
   files only: above `struct CountingWhile` in `NestRecordLowering.cpp` it said that the
   research verifies a `counter != upper` loop only where the accesses bound its counter. It
   is corrected in the commit after this stage's; the production tool built from the corrected
   source is byte for byte the tool validated here.

**Acceptance.** Research: `term_relations.mlir` (the termination driver on two nests of
`scf.while` loops taken whole; the claim over two small nests on one output, made and verified;
a counter stepped first, its step a constant from before the nest or of the after region; a
`!=` between two loop-carried variables whose orderings the limit cut; and three programs that
must get no relation, with the invariants they keep pinned: a for nest that holds a sum in a
state, a state given a constant state's value, an affine index whose multiplication has an
overflow check); `nest_while.mlir` (the records
of the stepped counter, of `!=` from other starts and of `!=` in larger arrays: VERIFIED, where
the stage before recorded NOT_VERIFIED); the nest oracle with `scf.while` and with mixed loops,
under the requirement it has with for loops. Lowering, with the production tool unchanged:
`nest_records_while.mlir` (25 functions: `@whl_ne_shifted` lowered where it was kept, and
`@whl_ne_bigger`, `@whl_step_first`, `@whl_step_first_shifted` added, each by formulas with its
tiles and runtime calls); `nest_records_while_refusals.mlir` (29 functions: `@step_first` is no
refusal any more, and the record of every other nest says what it said); `nest_while_e2e.py`
(the 25 functions x the five variants x synchronous and asynchronous lowering x two data sets,
compared with the reference).

**Validation.** On frozen states (HEAD `acb801a` and the 11 files this stage changes or adds,
this document excepted), each phase refusing to start on another state and recording that it
ended on the same one. The whole validation ran on `6ac41980…`. Before it, a first freeze
(`d2729fde…`) was stopped by hand after twelve minutes, when the measurements showed the third
relation answering overflow checks; the relation was narrowed. After it, one mutant of the
relations was alive (an equality made for a copy from a state that holds a constant): the
program written to pin that condition did not exercise it. The program was rewritten, in
`term_relations.mlir` alone, and the four phases that read that file (the research lit suite,
the comparison with the baseline, the sanitized lit commands, the mutation of the relations)
ran again on `7debf70e…`; every binary there is the one validated before, by its hash. The
baseline is `acb801a`: its research binaries, kept
from the stage before, and its production tool, which this stage does not rebuild (no
production source changes). Builds, scripts and logs are outside the repository
(`~/work/integration-backups-20261118/gemm-invariants`), two build jobs at most. Another
session was using the machine during the research phases (load 2 to 9); no phase failed on a
time limit.

| Check | Result |
| --- | --- |
| Research lit, alone | 38 of 38, on both freezes (`term_relations.mlir` and `nest_while.mlir` among them) |
| Research CTests, all of them | 33 of 34 pass: the synthesis unit test alone (497 s, 6977 checks), the four end-to-end runs, and 28 others two at a time. `generalized-gemm-termination` fails as at the baseline, line for line: `t3_swap` UNKNOWN, the same 3 of 119442 checks |
| Research outputs against the baseline | 120 research lit commands, baseline binaries against the new: 111 byte for byte the same, on both freezes. The 9 others: the three forms this stage verifies (`nest_while.mlir`), the five commands of `term_relations.mlir` that need the relations (its three programs that must get none are the same), and the proposer's statistics for `term_verdicts.mlir`'s `@piecewise` |
| Records | the eleven records files equal what the research tool says today (and what the sanitized driver says); against `acb801a`, one record changes status (`@whl_ne_shifted`, NOT_VERIFIED to VERIFIED), three are new and `@step_first`'s is gone |
| The nest oracle, 30 programs, each spelling of the loops | 0 failures each; in all four, 47 records VERIFIED, 188 nests equal to their record's fold, 120 compositions equal to the function, 3 shared outputs each nest verified by itself, and 3 joint claims verified (with `scf.while` 0, and with mixed loops 2, at the stage before) |
| End to end, the production tool unchanged | `scf.while`: 500 function runs (25 functions), 0 failures, 406 with every bit kept, 94 within their bound, 10 lowered modules with their calls in program order. Unchanged: stage 2, 144 runs, 0 failures; stage 3, 180, 0; `scf.for`, 260, 0 |
| Production lit, the whole suite, with the tool of `acb801a` | 197 of 199, the two known failures; the nine `nest_records` tests pass with the changed tests and records |
| Gate mutation (the production band; its tests changed) | 38 mutants: 38 KILLED by the tests' own checks |
| Mutation of the relations | 9 mutants, one for each condition of the three relations, for the place of the relations after the cut and for the re-making of a relation the cut removed: 9 KILLED by the checks of `term_relations.mlir` and `nest_while.mlir` (on the second freeze 8, the ninth leading to the rewritten program) |
| ASan and UBSan, research drivers | the sanitized nest driver remade the eleven records files, each equal to the file in the tree; the sanitized drivers passed the 19 commands of `nest_while.mlir` and `term_relations.mlir`, on both freezes: no report |
| Warnings | `TerminationProposers.cpp` with `-Wall -Wextra -Wpedantic`: none, as at HEAD |

**Open, from the stages before.** The three search mutants that survived at `771b700` are as they
were: not rerun, and no test pins them yet.

**Not run.** The polynomial suites (their targets are not part of the research build's default
and hold none of the changed source); the 104 other gate mutants and the 15 other mutants of the
research harness; the production unit binaries and the comparison of the production tool with
HEAD's (the tool is HEAD's); the end-to-end runs under a sanitized production tool (unchanged
since the stage before); the research's older mutation harnesses; `systolic-cc` beyond the
production lit suite; cosimulation and the board.

### Subscripts a loop carries (built 2026-11-18 on the index-invariant line)

Built on `integrate/index-invariants-into-nest-records`, which merged `acb801a` and the index
invariants of `docs/GENERALIZED_GEMM_STATEMENT_BV.md`, Section 14; on this line since the two
were merged (the section after this one).

The lowering takes a second kind of index beside a counter: one that an `scf.for`
(`iter_args`) or a counting `scf.while` carries, of type `index`, started at a constant,
stepped by a constant (plus or minus), its last value unread. It reads the recurrence from
the IR as start + step * (counter - lower bound) and treats the value as an affine form of
the counter, in the formulas (the record's view must be that form) and in the tables (the
nest's own addresses are evaluated with it). The record never supplies the recurrence.

What decides whether such a nest is lowered is therefore the record's status, and for the
2-D arrays that needs the research run with `--index-invariants`: without it the search
proposes no view of a carried subscript and the record is NOT_VERIFIED. The cases, the
refusals and the validation are in Section 14.9 of that document. A level of the record
may be a loop's counter plus a constant (the research takes a subscript such as `k + 2`
for the level): the lowering rewrites the record's forms to the counter and compares them
with the nest's as before (Section 14.10). Tests: `nest_records_carried.mlir`,
`nest_records_carried_refusals.mlir`, `nest_records_carried_tables.mlir`,
`nest_carried_e2e.py`.

Changed for the tests of the stage above: `@second_induction` is lowered; the messages of
`@invariant` and `@accumulator` are reworded. An `affine.for` with `iter_args` is still
refused.

### The two lines together (merged 2026-11-18)

Branch `integrate/index-invariants-into-generalized-gemm`: this line at `d3f3337` (the
relations of the section before the last, and one comment of the lowering made true again)
merged with `integrate/index-invariants-into-nest-records` at `075c047` (the section above:
seven changes and two merges of this line's earlier states since the two parted at
`acb801a`). An ordinary merge, since neither is an ancestor of the other; no commit is
picked a second time. Nothing is built here. What follows is what the merge had to decide,
and the one place where the two lines meet.

**Two files changed on both sides.**

- `nest_e2e.py`, the generator of the end-to-end programs. This line gave a loop
  `step_at` (the counter of an `scf.while` stepped first); the other gave it `carry`,
  `carry_step` and `counter_place` (indices a loop carries, stepped before or after they are
  read; the counter as the second carried value). Both are kept: one signature, and one
  printing of the lines that close an `scf.while`, where `step_at` places the counter's
  increment and the other three the carried steps and what is yielded. For every spec that
  a parent has, the merged generator prints what that parent's prints, text for text: the
  modules, the C drivers and the announced lines of `nest_e2e.py`, `nest_driven_e2e.py`,
  `nest_scf_e2e.py` and `nest_while_e2e.py` against `d3f3337`, and of the first three and
  `nest_carried_e2e.py` against `075c047` (32 texts each).
- This document: both sections are kept, this line's first.

**One file both sides changed that merged by itself**, `nest_records_while_refusals.mlir`:
this line took `@step_first` out of it (verified and lowered since the relations); the other
lowered `@second_induction` and reworded the messages of `@invariant` and `@accumulator`.
The merged file has both (29 functions), and its records file, made again by the merged
research tool, is the one in the tree, byte for byte.

**No production source differs from the other line's** but for the comment above
`CountingWhile` (`d3f3337`); the research source is the other line's with
`TerminationProposers.cpp` as it is here.

**Where the two lines meet.** In one place, and it shows in the record only. The search for
index invariants (`docs/GENERALIZED_GEMM_STATEMENT_BV.md`, Section 14.4) starts from the
termination certificate and lists what it adds to it. Since the relations, the certificate of
a nest with an `scf.while` already holds the equality of the body's argument and the loop's
counter wherever it is inductive. The search no longer offers it, the record's
`index_invariants` lists fewer invariants, and the identity that the view proposer is handed
for such a copy is the one made for copies (Section 14.8), once, where the search's equality
gave one for each cut. Measured on `@car_while` of `nest_records_carried.mlir`: the
certificate has 262 conjuncts where the other line's has 256, the six more being
`s13 == s12` at two cuts and `s15 == s14` at one, each plain and under the two variables'
poison flags, and no conjunct of the other line's is missing at any cut; the record lists 2
invariants where it listed 5, and the three gone are those three equalities.

So two records files are made again by the merged tool: `nest_records_carried.json` (5 of
its 13 functions: `car_while`, `car_mixed`, `car_outer`, `car_step2`, `car_shifted`) and
`nest_records_carried_tables.json` (`car_tables_while`, of 3). They differ from the other
line's in the `index_invariants` block of those six records and in nothing else: every
status, level, view, closed form, tier, refusal and binding is what it was. The other
fourteen records files, the three made without the search among them, are what the merged
tool says, unchanged. The lowering does not read that block, and no test's expectation
changes.

**Validation.** On one frozen state: the merge commit `031d267`, its worktree clean, each phase
refusing to start on another state and recording that it ended on the same one. The baselines
are the two parents: `075c047` built from a git archive, `d3f3337` from the builds of the
sections above. Builds, scripts and logs are outside the repository
(`~/work/integration-backups-20261118/gemm-integration`), two build jobs at most, never two
builds at once. Another session was using the machine (load 1 to 7 in the research phases,
which ran one after another); no phase failed on a time limit.

`systolic-opt` and `systolic-translate` built from the merged tree are, byte for byte, the ones
built from `075c047`.

| Check | Result |
| --- | --- |
| Production lit, the whole suite | 200 of 202: the two known failures (`Systolic/general_affine_domains.mlir`, `SystolicCC/general_affine.c`), their output the same as at `075c047` line for line; the twelve `nest_records` files pass |
| The merged tool against each parent's, every `systolic-opt` command of the tests | against `075c047`: 396 commands the same, none different (54 more differ only in the addresses of values, as two runs of one tool do). Against `d3f3337`: the 262 commands without `record-driven` the same; of those with it, 105 the same and 3 different, all in `nest_records_while_refusals.mlir` and all the other line's (`@second_induction` lowered, two messages reworded); the three files of carried subscripts are new to that parent |
| Production unit binaries that link the pass | both pass, the same output as at `075c047` |
| End to end: lowered, linked, run and compared with the original program | stage 2: 144 function runs; stage 3: 180; `scf.for`: 260; `scf.while`: 500 (25 functions, the counter stepped first and `!=` among them); carried subscripts: 336 (14 functions, `car_plain`, the counter as the second carried value and an index stepped before it is read among them). 0 failures in each; every run with the announced device invocations, every lowered module with its calls in program order; 1148 runs with every bit kept, 272 within their bound |
| Records | the sixteen records files equal what the merged research tool says |
| Research lit, alone | 40 of 40 |
| Research CTests | 36 of 37 pass under their limits: the five end-to-end runs, the lit suite, the records, 28 others two at a time, and the synthesis unit test in a Release build (7337 checks, 604 s; at `075c047` the same count, 600 s). `generalized-gemm-termination` fails as at both parents, line for line: `t3_swap` UNKNOWN, the same 3 of 119442 checks |
| The synthesis unit test in a Debug build | not a result under CTest: its sections of the index-invariant line need more than the 900 s limit there (set with a Release build). Run to its end with no limit: 7337 checks, 0 failures, 1364 s; at `075c047` 7337 checks, 0 failures, 1363 s |
| The nest oracle, 30 programs, each spelling of the loops | 0 failures each; in all four, 47 records VERIFIED, 3 shared outputs each nest verified by itself, 3 joint claims verified |
| Invariants into a nest's record | the nest unit test 917 checks, the domain unit test 1580, their sections `indexinvariants` and `indexwiring` among them; the two invariant oracles; `synth_index_invariants.mlir` and `synth_invariants.mlir`: all pass |
| Research outputs against both parents | 138 research lit commands, each run with the drivers of `d3f3337`, of `075c047` and of the merged tree: 114 the same from all three; 9 as `d3f3337` gives them (the nine of the relations); 15 as `075c047` gives them (commands of the two files that this line's drivers do not know); none that neither parent gives |
| Gate mutation: the band, the carried indices, the levels | 68 mutants, 68 KILLED, each by a test's own FileCheck (61 by an expected line that is absent, 7 by a remark that never came); none by a crash or a time limit |
| Gate mutation: the rest of `NestRecordLowering.cpp` | the 40 other mutants of the source the other line changed, which neither line had run since: 40 KILLED the same way (35 and 5). Run after the rest, on the commit that adds this section (the same tree but for this document) |
| Mutation of the index-invariant checker and search | 21 mutants, 21 KILLED, each by failed checks of the unit section `indexinv`, which passes unmutated (216 checks); judged outside the catalogue's own harness, which counts a crash or a timeout as a kill |
| Mutation of the relations | 9 mutants, 9 KILLED by the checks of `term_relations.mlir` and `nest_while.mlir` |
| Mutation of the wiring of index invariants into a nest's record | 7 mutants, 7 KILLED by failed checks of the unit sections written for them: 3 of the domain catalogue (the identities not handed to the proposer, not gathered from the search; the certificate handed on unstrengthened) by `indexwiring`, 4 of the nest catalogue (the search never run; the certificate handed on as it was, or without the identities; copies given no identity) by `indexinvariants` |
| ASan and UBSan | the sanitized `systolic-opt`: the twelve `nest_records` files, and the `scf.while`, carried and `scf.for` end-to-end runs (500, 336 and 260 function runs), 0 failures. The sanitized research drivers: the sixteen records files remade, each equal to the file in the tree, and the 37 commands of `nest_while.mlir`, `term_relations.mlir`, `synth_index_invariants.mlir` and `synth_invariants.mlir` pass. No report |
| Warnings | the merged builds have the warnings of `075c047`'s builds and no other (production 47, research 1 in Debug and 3 in Release) |

Two phases were started twice, by faults of the validation's own scripts and not of a test: the
sanitizers first started sixteen sanitized drivers at once and were stopped (three at a time
after that), and their last part replaced `%s` before the drivers' names, so sixteen commands
named no program and were run again, with the rest, by a corrected script.

**Known failures, as before the merge.** The two production lit tests and
`generalized-gemm-termination`, each the same at both parents. No other test fails, and none
that passed at a parent fails here.

**Open, from the stages before.** The three search mutants that survived at `771b700`; the
pattern of one older domain mutant (`views proposed without the sample check`), which occurs
twice in `DomainProposer.cpp` since `acb801a` at both parents; the joint claim over two larger
`scf.while` nests, which is not made.

**Not run.** The polynomial suites (no source of theirs is changed by either line); the 58
other gate mutants (of `NestRecordGate.cpp`, `NestBinding.cpp` and the pass, which neither
line has changed since `acb801a`), the 15 other mutants of the search harness, and the older
research catalogues but for the mutants named above; sanitized unit binaries; `systolic-cc`
beyond the production lit suite; cosimulation and the board.

## The thesis

The thesis text of 2026-11-06 (465 pages) makes no claim about merging loops: Definition 4.5
is "for a single, unbatched GEMM, the nest", Chapter 6 partitions one M × N region with depth
K, and neither the limitations nor the future work mention several loops. Nothing there has
to be withdrawn. What must not be added: G5b's M2 to M4b described as a capability to merge
loops, or as part of the path to the FPGA. They are joint analyses of several loops, complete
and outside the lowering.
