# Generalized GEMM recognition: G0 audit and Phase G1

Phase G1 builds and evaluates an exact semantic model of a function. It issues
no GEMM verdict, and nothing here is linked into production lowering. There is
deliberately no ACCEPT or REJECT type in the code yet.

Decisions recorded on 2026-10-31:

1. Adopted obligations: batch coordinate P, per-tile output injectivity, C0,
   non-degeneracy, data-obliviousness, parameter scope, and approximation
   polarity of D.
2. The FSM branch is not cherry-picked. Reconsider at G2.
3. Research-only build.
4. Canonical join is the first strategy when its assumptions hold. SyGuS/CEGIS
   stays in the architecture for arbitrary finite multidimensional coordinates.
5. If a hardware lowering cannot preserve the required floating-point
   semantics, that transformation is rejected.
6. A valid decomposition is distinct from a verified transformation. A mapping
   with no sound lowering does not produce ACCEPT for the transformation.
7. LLVM dialect import comes after G1.

## The model

`SemanticProgram` (include/GeneralizedGemm/SemanticProgram.h) is a finite
transition system. A configuration is `(location, state, poison shadow, memory)`.

- One `Transition` per CFG successor slot, so parallel edges stay distinct.
- State is every non-entry block argument plus every result used in another
  reachable block. The function's scalar arguments and the extents of dynamic
  memrefs are immutable parameters.
- All terms of a transition read the configuration at entry to the source block.
  Assignments are therefore simultaneous, and a store chain in the memory term
  carries in-block order into later loads.
- `Term` (SemanticTerm.h) is a hash-consed DAG over BV, Float and Mem sorts.
  There is no mathematical integer sort. Booleans are BV1.
- The model owns its data and holds no MLIR pointer.

The semantic object is the ordered sequence of MAC events of a run. D is that
sequence's index set, so order and multiplicity survive. `x` is a projection of
the state at the event (`MacSite::coordinates`, the state variables in the
support of the site's addresses), and two events may share an `x`.

Differences from the design proposal, all in the direction of precision:

- `Sort` has BV, Float and Mem. There is no separate Bool or Ptr: `i1` is BV1,
  and a root is a Mem sort.
- `Transition` has sparse `assignments`, plus `shadowAssignments` for poison and
  `memoryUpdates`, instead of one `update` vector.
- `importFunction` takes `ImportOptions`; `--lower-structured` is an option.
- `SideCondition` has three kinds, not one: undefined behaviour, underspecified,
  and model limit.

### Semantics decisions

| Case | Model | Why |
| --- | --- | --- |
| add, sub, mul, shl | Modular value; flags kept on the node; poison term computed at a width that cannot wrap | I1, I2 |
| `divsi`/`divui`/`remui` by zero, `divsi` overflow | Undefined behaviour | arith spec |
| `remsi` of the minimum by -1 | Underspecified | arith documents only division by zero, but the lowering to `llvm.srem` makes this undefined. No value is claimed |
| Poison divisor; poison dividend by -1 | Undefined behaviour | A poison dividend may be the minimum value |
| Shift by at least the width | Poison | arith spec |
| `exact`, `nneg`, `trunci` flags | Poison when violated | arith spec |
| `select` | Only the selected arm's poison reaches the result | LLVM semantics |
| Branch, switch, memory index on poison | Undefined behaviour | LLVM semantics |
| Poison stored to memory | Model limit: stops | Memory has no shadow in G1 |
| Uninitialized read | Stops as underspecified | Not a value |
| NaN result of `addf`/`mulf` | Underspecified | Payload and sign are not determined |
| `nsz` with a zero result | Underspecified | The sign is not determined |
| `nnan`, `ninf` | Poison when violated | LLVM semantics |
| `reassoc`, `contract`, `arcp`, `afn` | Preserved. Evaluated as written, reported by `strictRefinementOfPermissiveFloat` | One allowed behaviour, not the set |
| `roundingmode` | Refused | Only the default is modelled |

Division, remainder and shifts are total in the term language, with the SMT-LIB
definitions, so evaluator and encoding agree on all inputs. The undefined cases
are side conditions, never hidden in those definitions.

Side conditions are evaluated before the block's effects, in operation order.
A block that stops therefore contributes no events. An uninitialized read is
found while evaluating, not by a side condition, so a later undefined operation
in the same block can be reported first; the run has no defined result either
way.

### Evaluators

- **Concrete** (`runConcrete`): big-width bit patterns and fuel. `Returned`
  proves termination for that entry only. A repeated full configuration proves
  divergence for that entry only. `OutOfFuel` proves nothing.
- **Symbolic** (`summarizeAcyclic`): path summaries from one location. A loop is
  an explicit stop. The path conditions partition the configurations. Exhausting
  a limit returns no paths at all. It adds terms to the program's shared context,
  so a program must not be summarized from two threads at once.
- **Relational** (`encodeTransitionRelation`, `encodeStepQuery`): SMT-LIB text
  for T(c, c'), `Stuck_UB` and `Stuck_Other`. The library does not link a solver.
  The SMT FloatingPoint sort has one NaN, so the encoding identifies NaN payloads
  and the concrete model does not.

## Invariants: where each is enforced and tested

| ID | Enforced by | Tested by |
| --- | --- | --- |
| I1 exact width | Term builders refuse ill-sorted nodes; `index` needs an explicit width | `testI1`, integer/compare/cast tables, fuzz |
| I2 definedness | Importer side conditions and shadow terms | `testI2`, integer table (with flags), native differential |
| I3 fail-closed | Whitelists; `Refusal` clears the model at the API boundary | `testI3`: every `UnsupportedKind` has a case, and the test fails if one is added without a case |
| I4 oblivious flag | Taint fixpoint over state, including shadows | `testI4` |
| I5 simultaneous edges | All terms read the block-entry configuration | `testI5` |
| I6 evaluator agreement | Separate implementations, cross-checked | `testI6Symbolic`, `RelationalAgreementTest`, oracle fuzz |
| I7 read-only, deterministic | Rewrites only a private clone; ids from IR order | `testI7` |
| I8 domain exactness tag | `DomainEvidence`, `admitsClaim`, `scopeContains` | `testI8` |
| I9 assumptions | `ProofStatus` factories; no conversion exists; `conjoin`, `isDischarged` | `testI9` |

The polarity gate of I8 is implemented and tested, but no G1 code issues a claim
through it: the obligations that must call it are G3 to G7. `ExactPresburger` and
`ExactClosedForm` exist as tags only; G1 never constructs them.

## Trusted base and premises

- Upstream `lower-affine` and `convert-scf-to-cf`, used with `lowerStructured`.
  The native differential does not cover that path; the GEMM test checks one
  structured nest against a hand-computed product.
- MLIR's parser and verifier, and `llvm::APInt` and `APFloat` as arithmetic.
- Entry states have defined, non-poison parameters. The model quantifies over
  those entries.
- Roots are separate arrays. Where separation is not proven, the model carries a
  named Assumed status, and it cannot be discharged without an authorization.
  `llvm.noalias` is taken as the source language's contract.
- Calls are inlined by this library on the clone, tested by differential and
  unit cases.
- D for a parameter point describes runs that complete. Whether another memory
  content would stop on data-dependent undefined behaviour or a NaN is not part
  of the evidence.

## Known cost

Import clones the enclosing module and fingerprints its full text, once per
function, so importing every function of a module is quadratic in its size. The
fuzz test uses modules of 100 functions for this reason.

## Unsupported in G1 (all fail closed)

Nested regions without `lowerStructured`; every operation outside the whitelist,
including `subf`, `divf`, `negf`, `cmpf`, FP casts, `ceildivsi`, `floordivsi` and
vector types; `llvm` dialect input; memref views other than an identity
`memref.cast`; non-identity layouts and non-default memory spaces; memref block
arguments and results; allocation in a cycle or of dynamic shape; `dealloc`;
`cf.switch` with a repeated case, which LLVM's verifier rejects; indirect,
external, recursive and cyclic-callee calls; integers wider than 64
bits; float types other than f32 and f64.

## Build and test

```sh
L=$HOME/work/llvm-project/build
cmake -S research/generalized-gemm -B <build> -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DLLVM_DIR=$L/lib/cmake/llvm -DMLIR_DIR=$L/lib/cmake/mlir
ninja -C <build> check-generalized-gemm
python3 research/generalized-gemm/test/fuzz/mutation.py --source research/generalized-gemm \
  --scratch <empty dir> --llvm-dir $L/lib/cmake/llvm --mlir-dir $L/lib/cmake/mlir
```

Use the LLVM 24 tools first on PATH. Tests live under `research/`, so
`test/lit.cfg.py` and `check-systolic` do not see them.

## Validation (2026-10-31, LLVM 24.0.0git, base d5fb2a7)

| Check | Result |
| --- | --- |
| Unit suite, I1 to I9 | 37,739 checks, 0 failures, including 11,220 integer table cases |
| Relational agreement | 8,451 checks, 7,603 Z3 queries, 0 failures |
| Oracle fuzz, CTest | 1,500 functions, 9,000 runs, no disagreement |
| Oracle fuzz, fresh seed | 5,000 functions, 30,000 runs, no disagreement |
| Native differential | 197 runs in CTest, 451 with a fresh seed, no disagreement |
| Lit | 4 of 4 |
| ASan and UBSan | Clean: unit, relational, 1,200 fuzz runs |
| Mutation | 36 of 36 killed, with the unmodified baseline passing first |
| `check-systolic` | 188 of 190; the 2 failures are the known legacy ones |
| Polynomial suites | 13 of 13 |

The native differential compares only runs the oracle calls defined with a
non-poison result. Mutants stop at their first killer, so the count does not
show what each suite would catch alone. No FPGA or hardware path is involved.
