# P2a: symbolic initial-state polynomial recurrence recognition

Worktree: `systolic-mlir-polynomial-summation`; branch
`feat/polynomial-summation`. Protected checkpoints remain:

- P0: `057ac9e482ba9cb6c6cba7cd82b1c4d3f5e8bde1`.
- P1: `5d0a5bdbed665ccd7d76cbd014c37e972b038f63`, annotated tag
  `polynomial-summation-p1-20261029`.

P2a is implemented for the subset below. It is an opt-in, read-only research
analysis. P0's implementation is unchanged; P1's constant-only public API and
all existing regression files remain unchanged in behavior. There is no
production transformation, lowering integration, termination analysis, or
exit-value analysis. No P2b/P2c work, commit, tag, merge, or push is included.

## 1. Representation and reuse of P0

`SymbolicPolynomial` represents a rational polynomial in invariant parameters
`p` and the iteration variable `t` as

```
F(t;p) = sum_m p^m * Q_m(t)
Q_m(t) = sum_k a[m,k] * binomial(t,k),  a[m,k] in Q.
```

The outer map is a sparse canonical map from parameter monomials (sorted maps
of parameter names to positive exponents) to **existing P0 `Polynomial`
objects**. Zero terms are removed. P0 normalizes trailing zero binomial
coefficients, and Boost rational normalizes numerators/denominators. Therefore
map equality is exact equality in the rational polynomial ring, not equality
at sampled points or equality of expression spellings.

Addition/subtraction merge sparse maps. Multiplication adds parameter
exponents and invokes P0 multiplication on the corresponding iteration
polynomials. Scaling, shifting, finite differences, and discrete integration
invoke P0 coefficientwise. Substituting parameter integers produces a P0
polynomial. `atIteration(0)` returns a parameter-only polynomial and is used
for the initial identity. The standalone algebra supports rational
coefficients; the MLIR boundary conservatively requires integral Newton
coefficients for every parameter monomial, a sufficient condition for
integer-valued sequences on all integer assignments.

There are no per-power summation formulas in the implementation. Tests use
an independently specified conventional square-sum formula as one oracle.
There is no floating-point arithmetic, division of modular residues, or
division-based IR transformation.

For the requested example the extracted system is

```
i(0) = beta;   i(t+1) = i(t) + 1
s(0) = alpha;  s(t+1) = s(t) + i(t)^2
```

The engine derives

```
i(t) = beta + t
s(t) = alpha + beta^2*C(t,1) + (2*beta+1)*C(t,2) + 2*C(t,3)
     = alpha + t*beta^2 + beta*t*(t-1) + t*(t-1)*(2*t-1)/6.
```

Both equalities are checked as exact symbolic identities in the C++ suite.

## 2. API and supported parameters

The existing `cfg::analyze(function, Options)` remains the P1 constant-only
entry point. P2a adds `cfg::analyzeSymbolic(function, SymbolicOptions)` and a
separate `SymbolicResult`. Symbolic initial expressions are never passed off
as concrete `Rational` initial values in a returned P1 result.

Supported parameter leaves are scalar signless integer function arguments
of the same width as the loop-carried states. The name `pN` means function
argument number N, independently of SSA names and traversal order. `sN`
means header argument number N. Reported parameters include those needed
only to check dead/canceled original arithmetic. Arguments used only as
direct exit-guard operands need not become recurrence parameters.

Supported invariant expressions contain constants and `arith.addi/subi/muli`
over supported invariant values. They may be defined on the unique entry
path and forwarded through block arguments. Invariant arithmetic inside the
loop is expanded through its invariant operands; it is not promoted to a
new unconstrained symbol. Symbolic induction steps and polynomial parameter
coefficients are supported in addition to symbolic initial states.

All symbolic casts remain UNKNOWN, including `arith.extui nneg` on a
symbolic operand even with a nonnegative parameter interval. Constant-only
casts retain P1's exact bit semantics and source-width `nneg` check. There
is no general cast, poison, or memory-invariance inference.

`SymbolicOptions::parameterRanges` supplies closed integer domains indexed
by function argument number. Defaults are the full signed representative
domain `[-2^(w-1), 2^(w-1)-1]`. These cover every bit pattern in modular mode;
they do not assert that signless MLIR operations have signed semantics.
Explicit domains must be nonempty subsets of that domain and name used
parameters. They are caller constraints, printed in the result, not facts
inferred from comparisons or loop guards. PROVEN means proven for **all
assignments in the reported domains** and the stated iteration domain.

`SymbolicResult::valueAt` requires exactly the reported parameter names,
valid parameter assignments, nonnegative iteration, and compliance with any
explicit horizon. UNKNOWN results cannot be evaluated. Modular values are
residues; integer values are checked against the state width. CONDITIONAL
evaluation still requires the caller's full original-intermediate contract;
checking the returned state is not a runtime validation of that assumption.

## 3. Shared CFG extraction and invariance proof

P2a uses the same verified SSACFG traversal, MLIR `DominanceInfo` and
`CFGLoopInfo`, SCC checks, branch interfaces, edge mapping, and unique-path
requirements as P1. The source function must have one supported natural
loop, a unique entry/backedge, one conditional exit, and an unconditional
cycle update path. Noncanonical block layout and argument permutations are
supported. Unreachable blocks are verified but excluded from analysis.

Invariance is established recursively:

1. Entry arguments and constants are leaves.
2. A non-entry prelude block argument follows its unique reachable incoming
   edge. Its actual successor operand is inspected.
3. A pure supported arithmetic definition on the unconditional prelude is
   expanded recursively. Verified SSA dominance establishes availability.
4. A header argument is always a current state, never an invariant leaf.
   Initial-value extraction cannot depend on a loop state. Unknown effects,
   loads, calls, unsupported operations, and symbolic casts fail closed.

Backedge expressions reuse P1's immutable P0 `Expression` DAG. All state
references denote the same old header visit, so simultaneous updates are
preserved. A source use of an already computed next-value SSA definition is
expanded explicitly; it is not confused with an old-state reference.

Exactly one additive old-self occurrence is removed using P1's existing
structural check. Increment dependencies include only `sN` state references,
not `pN` parameters. A deterministic dependency schedule solves each state
after all states in its increment. Cycles/nonlinear self recurrences return
UNKNOWN. Parameter polynomials are substituted into the existing P0
expression structure; the sparse extension delegates iteration integration
and polynomial operations to P0.

## 4. Exact proof obligations

For each solved state, define `G(t;p)` by substituting solved predecessor
states into its additive increment. Integration gives

```
S(t;p) = S0(p) + sum_m p^m * integrate(Q_m)(t).
```

P0's identity
`C(t+1,k+1) - C(t,k+1) = C(t,k)` gives `Delta S = G`, coefficientwise in the
parameter monomial basis. Every integrated term vanishes at zero, giving
`S(0;p)=S0(p)`. The acyclic schedule makes this construction inductive over
the recurrence dependency graph.

Before returning any accepted result, the implementation additionally checks
all three identities by canonical symbolic polynomial equality:

```
S(0;p)   == extracted_initial(p)
Delta S  == extracted_increment(S(t;p),p)
S(t+1;p) == extracted_full_next_state(S(t;p),p).
```

The last check uses the full original backedge expression, not just the
structurally removed increment. This catches an extraction/old-next mismatch
even if the integrated increment is internally consistent. Random tests are
supplementary and do not establish these identities.

On any unsupported shape, failed proof, or caught algebra resource limit,
UNKNOWN clears states, parameters, polynomial forms, and the header pointer.
No partial proof is returned.

## 5. Integer and poison semantics

**Modular:** unflagged `iW` addition/subtraction/multiplication are interpreted
in `Z/(2^W)`. Exact integer lifts preserve their operations under the residue
homomorphism. A symbolic initial expression that wraps is represented by its
integer lift; its residue equals the actual initial bit pattern. The exact
initial/transition identities therefore imply the original bitvector state
identities by induction. Default domains include all source bit patterns.

**Mathematical and signed-no-overflow:** finite-width MLIR is not silently
treated as unbounded integers. Both modes require an explicit finite
iteration horizon and proof that every original arithmetic intermediate and
state fits its signed width, or an explicit CONDITIONAL contract for
unflagged arithmetic. Mathematical symbols are unbounded in the standalone
algebra; accepted MLIR interpretation is restricted by these width/domain
contracts. No termination or reachability inference supplies the horizon.

The range enclosure multiplies each P0 `Q_m.bounds(N)` interval by the
parameter-power intervals, then sums the intervals using exact rational
arithmetic. It encloses all rectangular parameter assignments and every
integer `t` in `[0,N]`. Dependencies/correlations may be lost, so a safe
program can return UNKNOWN. All original source arithmetic is checked,
including prelude, dead/canceled operations, guard arithmetic and exit tails.
As in P1, the check includes a hypothetical body at the final header visit.

**Flags:** modular arithmetic flags, `nuw`, combined flags, and flagged
truncations remain unsupported. P2a accepts `nsw` only on the fully proved
source/state range path. Caller no-overflow assumptions cannot discharge
unproved flag preconditions. It is insufficient to bound a flagged result
after cancellation if an earlier operand may have wrapped. The new
`flag_operand_wrap` fixture checks this case: a preceding `alpha+INT_MAX`
can wrap before a flagged subtraction whose lifted result is `INT_MAX`.
Full parameter domains yield UNKNOWN even with an assumption; a proved
safe restricted domain is accepted and independently executed.

`arith.extui nneg` still tests `source.isNegative()` at the **original APInt
width**, before `zext`. Negative constants return UNKNOWN in all three modes.
Unproven symbolic sources also return UNKNOWN. General no-overflow
assumptions cannot legalize a poisoned cast.

## 6. Validation and reproducibility

The initial unchanged baseline passed all six P0/P1 CTest entries. A SHA-256
manifest covers all 31 existing P0/P1 regression files; they remain byte for
byte unchanged. The final P2a build uses GNU 13.3.0, Boost 1.83, Python 3.13.9,
and local LLVM/MLIR 24.0.0git commit
`7615da10b75c57286a4032eac8bb43c1b8d2ba46`.

```sh
cmake -S research/polynomial-summation-mlir \
  -B build/polynomial-summation-p2a -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DMLIR_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/mlir \
  -DLLVM_DIR=/home/steven-studio/work/llvm-project/build/lib/cmake/llvm
cmake --build build/polynomial-summation-p2a -j3
ctest --test-dir build/polynomial-summation-p2a -V
```

The fresh standalone build and final combined suite passed: **9/9 CTest
entries, 0 failures**. Counts below are per suite; included checks overlap.

| Suite | Actual final result |
| --- | --- |
| Existing P0 C++ | 42,277 checks PASS |
| Existing P0 Python | 13,443 comparisons PASS |
| Existing P1 C++ | 517,759 checks PASS; 1,961 actual-MLIR APInt executions; 233,023 state comparisons; 14 accepted/14 rejected fixture functions |
| Existing P1 `nneg` | 1,244 checks PASS; 27 accepted mode cases; 65 expected UNKNOWN cases; 594 direct comparisons |
| Existing P1 Python | 98 accepted functions; 985 assertions; 7 subprocess checks; 27,702 exact values, 27,702 modular values, 27,702 differences; 201 lowered while-loop values; 6 expected UNKNOWN resource/effect cases |
| Existing P1 lit | 7/7 PASS |
| New P2a C++ | 41,653 checks PASS; 766 actual-MLIR APInt executions; 16,484 state comparisons; 64 PROVEN, 22 CONDITIONAL, 121 expected UNKNOWN analysis cases |
| New P2a Python | 40 verified generated functions; 1,600 independent SSA executions; 39,240 exact and 39,240 modular comparisons; 39,240 differences; 4,800 initial checks; 9 expected UNKNOWN analysis cases; 3 expected CLI error cases |
| New P2a lit | 4/4 PASS |

All required validation categories ran. Full production compiler, FPGA RTL,
FPGA Runtime, full LLVM/MLIR suites, and a new LLVM optimization comparison
were **NOT RUN**. No new LLVM optimization claim is made; P1's recorded
comparison remains historical evidence, not a P2a test run.

The C++ oracle executes the actual MLIR with arbitrary-width APInt, including
simultaneous branch operand snapshots and independent `nsw` overflow checks
for accepted flagged cases. Independently specified initial expressions,
increments, and dependency sets cover all ten new accepted fixtures. Cases
include symbolic steps, quadratic invariant coefficients, old/next triangular
updates, i8 wraparound, permuted block forwarding, noncanonical layout, and
unreachable unsupported arithmetic. The signed/mathematical cases exercise
both proved parameter intervals and explicit conditional contracts. All 22
P1 cast fixtures are additionally replayed through the P2a API in three modes.

The Python oracle generates degrees 0..5 plus a parameter-quadratic term,
widths 8/16/32/128/256, negative/zero/positive and symbolic steps, old/next
index variants, and coefficients exceeding 180 bits. It parses and executes
the generated original SSA arithmetic/control flow using Python integers
modulo the actual width. It independently checks those traces against direct
conventional sum-of-powers updates and the returned forms evaluated using
`Fraction`/`math.comb`. Assignments include negative values, signed minima,
maxima, and deterministic random values (seed `0x502a`). Every generated
module is verified by local `mlir-opt --verify-each` before analysis.

Rejected cases cover self/cyclic dependence, memory, conditional updates,
loop-variant casts, symbolic `nneg`, poisoned constant `nneg`, excessive
parameter degree/term count/parameter count, invalid parameter domains,
missing horizons, flags, and invalid evaluation assignments. Algebra tests
also reject excessive coefficient bits and integration beyond P0 degree 64.
Every API rejection asserts empty forms/parameters/states and non-evaluability.

Read-only checks snapshot complete generic IR with debug locations for
accepted, rejected, overflow, and cast fixtures. The reporting tool analyzes
twice and compares deterministic output. Lit compares printed IR before and
after analysis; Python checks source bytes and independent-process output.

Example CLI:

```sh
build/polynomial-summation-p2a/polynomial-cfg-report \
  test/polynomial-summation-p2a/Inputs/accepted.mlir \
  --symbolic --function=shifted_square --evaluate=3 \
  --parameter-value=p1:7 --parameter-value=p2:-2
```

This reports header values `(3,1,12)`. For proved integer semantics add
`--mode=signed --horizon=5 --parameter-range=1:-10:10
--parameter-range=2:-10:10`. Domains are explicit API contracts, not guard
inferences. Omitting `--symbolic` preserves the P1 interface.

## 7. Resource limits, unsupported cases, and residual risks

New algebra limits are 16 distinct parameters, total parameter monomial
degree 16, 256 sparse parameter monomials per intermediate, and 16,384 bits
per rational numerator/denominator. P0 retains iteration degree 64. A single
symbolic product considers at most 256 squared term pairs before rejection;
intermediate term growth is checked during convolution. Substitution bounds
expression traversal to depth 128 and 4,096 distinct nodes.

P1's limits remain: 512 blocks, 4,096 reachable operations, 128 header states,
scalar widths 1..4,096, SSA traversal depth 128, and 65,536 characters per
expanded reported expression. Resource-limit exceptions clear the analysis.
These are conservative limits, not claims of completeness or fixed wall time.
Concrete arbitrary-precision iteration/range values still carry bit costs.

With T parameter terms and iteration degree d, sparse addition is a map
merge plus P0 additions; multiplication is at most T squared P0 polynomial
products (`O(d^3)` arithmetic operations each). Difference/integration/shift
delegate respectively to P0's `O(d)`, `O(d)`, and `O(d^2)` routines per term.
Exact bit costs depend on coefficient and evaluation sizes. Different source
expressions are substituted separately, so no global optimality claim is made.

Unsupported cases include symbolic casts, mixed-width symbolic states,
non-integer parameters, non-polynomial operations, loads/calls/unknown
effects, nonlinear self recurrences, cyclic increment dependencies, arbitrary
conditional updates, multiple/nested loops, irreducible/multi-entry cycles,
and unsupported structured regions. Raw `scf` remains unsupported; proven
external CFG conversion is separate. Parameter inequalities beyond closed
independent intervals, disjunctions, path-sensitive guards, and automatic
invariant lifting of loop-carried states are not implemented.

No unresolved soundness defect was found in the focused review. Remaining
verification limits are that this is not a mechanized proof of the C++/MLIR
implementation, explicit caller contracts require external justification,
and interval proofs can reject valid programs. The pinned MLIR operation
semantics are part of the trusted boundary; future semantic attributes need
review. Clients must preserve UNKNOWN and CONDITIONAL distinctions and must
not use these forms as proof of termination, exit selection, or permission to
rewrite finite-width IR.

P2a is ready for checkpoint review for this documented subset. Changes remain
uncommitted; checkpoint creation requires separate user authorization.

## 8. Exact file inventory

Modified research files:

```
include/Systolic/MLIRPolynomialRecurrence.h
lib/Systolic/Analysis/MLIRPolynomialRecurrence.cpp
research/polynomial-summation-mlir/CMakeLists.txt
research/polynomial-summation-mlir/PolynomialCFGReport.cpp
```

New files:

```
docs/POLYNOMIAL_SUMMATION_P2A.md
include/Systolic/SymbolicPolynomial.h
lib/Systolic/Analysis/SymbolicPolynomial.cpp
research/polynomial-summation-mlir/p2a-lit.site.cfg.py.in
research/polynomial-summation-mlir/p2a-lit/lit.cfg.py
research/polynomial-summation-mlir/p2a-lit/readonly.mlir
research/polynomial-summation-mlir/p2a-lit/rejected.mlir
research/polynomial-summation-mlir/p2a-lit/semantics.mlir
research/polynomial-summation-mlir/p2a-lit/symbolic.mlir
test/polynomial-summation-p2a/Inputs/accepted.mlir
test/polynomial-summation-p2a/Inputs/rejected.mlir
test/polynomial-summation-p2a/Inputs/semantics.mlir
test/polynomial-summation-p2a/SymbolicCFGTest.cpp
test/polynomial-summation-p2a/differential.py
```

Production build files, GEMM lowering, Affine Partitioning, `systolic.matmul_tile`,
FPGA Device Job Lowering, RTL, Runtime, other worktrees, and all P0/P1 tests
are unchanged.
