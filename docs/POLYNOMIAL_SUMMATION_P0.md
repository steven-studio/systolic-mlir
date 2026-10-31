# Polynomial summation P0

Standalone research implementation, 2026-10-29. Worktree:
`~/work/systolic-mlir-polynomial-summation`, branch `feat/polynomial-summation`,
baseline `285354c86460521b8b67c298495d55d71e6f465e`.

P0 derives polynomial sums using exact binomial-basis algebra. It recognizes
acyclic additive polynomial state transitions, distinguishes three integer
semantics, and returns `UNKNOWN` for unsupported analysis. It does not parse
CFGs, infer termination, emit IR, register a compiler pass, or change GEMM
recognition, Affine Partitioning, or production lowering. The production CMake
graph is unchanged. All new sources and evidence are left uncommitted.

## 1. Implementation and arithmetic

| File | Purpose |
| --- | --- |
| [PolynomialSummation.h](../include/Systolic/PolynomialSummation.h) | Public polynomial, symbolic range, expression, recurrence, and semantics interface |
| [PolynomialSummation.cpp](../lib/Systolic/Analysis/PolynomialSummation.cpp) | Exact algebra, dependency recognition, and signed range proof |
| [Standalone CMake](../research/polynomial-summation/CMakeLists.txt) | Independent C++17 library, driver, and CTest targets |
| [Driver](../research/polynomial-summation/PolynomialSummationDriver.cpp) | Demonstration and line protocol for the independent oracle |
| [C++ tests](../test/polynomial-summation/PolynomialSummationTest.cpp) | Exact identities, initial conditions, independent execution, and rejection tests |
| [Python oracle](../test/polynomial-summation/differential.py) | Direct summation and state simulation using Python `int` / `Fraction` |
| [LLVM fixture](../test/polynomial-summation/llvm/recurrences.c) | Unsigned C loops, including unsupported and observable cases |
| [LLVM experiment](../test/polynomial-summation/llvm/compare_llvm.py) | Reproducible analysis, optimization, provenance, and execution checks |
| [Observed evidence](../test/polynomial-summation/llvm/observed/report.json) | Tool versions, source revision, hashes, commands, and observed loop counts; adjacent files retain IR and analysis text |

Arithmetic uses `boost::multiprecision::cpp_int` and
`boost::rational<cpp_int>`, the same underlying types already used in
`include/Systolic/TensorLogicalSpaces.h`. No Affine Partitioning header needs
to be included or changed. There is no custom arbitrary-precision arithmetic
implementation, floating-point calculation, Bernoulli-number table, or
per-power summation formula in the implementation. Textbook formulas appear
only as test expectations and explanatory results.

LLVM's `DynamicAPInt` and MLIR's Presburger `Fraction` were also inspected.
The existing project's Boost types supply normalized exact rationals without
adding LLVM/MLIR link dependencies to this isolated library. Directly using
`ScalarEvolution::getAddRecExpr` would require LLVM IR types, loops, and the
analysis context, and would give bitvector SCEVs rather than this rational
polynomial domain. P0 therefore does not reimplement LLVM's CFG discovery,
trip-count analysis, SCEV interning, poison reasoning, or IR expansion.

## 2. Mathematical model and canonical binomial basis

Let `B_k(t) = binomial(t,k)`, where

```text
B_0(t) = 1
B_k(t) = t(t-1)...(t-k+1) / k!       (k > 0).
P(t)   = sum_{k=0}^d a_k B_k(t),    a_k in Q.
```

The generalized definition applies to negative integers as well. In
particular `B_k(-m) = (-1)^k binomial(m+k-1,k)` for positive `m`; replacing
negative-argument binomials with zero would be incorrect. Evaluation uses
`B_{k+1}(t) = B_k(t)*(t-k)/(k+1)` with arbitrary-precision integers. Each
division is exact, including for negative `t`. It is never performed on a
fixed-width residue.

`Polynomial` stores the vector `(a_0,...,a_d)`, strips all trailing zeros,
and uses normalized rational coefficients. Zero has an empty vector and
degree `-1`; a nonzero constant has degree `0`. As `B_k` has leading
coefficient `1/k!`, these polynomials form a basis of `Q[t]`, and canonical
coefficient equality is exact polynomial equality, not equality at samples.

The forward difference is

```text
Delta P(t) = P(t+1) - P(t).
Delta B_k(t) = B_{k-1}(t)   (k >= 1),    Delta B_0(t) = 0.
```

This follows from Pascal's identity for infinitely many nonnegative `t`,
and hence as a polynomial identity over `Q`, including negative integer
arguments. Therefore differences drop the first coefficient and
`a_k = Delta^k P(0)`. This also proves that `P` is integer-valued at every
integer exactly when all its binomial coefficients are integers: necessity
follows by taking integer finite differences at zero; sufficiency follows
from the integrality of generalized binomials. `isIntegerValued()` implements
this criterion.

### Conversion, products, and shifts

Conventional coefficients are ordered from constant to highest power.
`fromMonomial` uses Horner's rule and the identity

```text
t B_k(t) = k B_k(t) + (k+1) B_{k+1}(t).
```

Thus conversion is symbolic, without interpolation or enumeration of a
runtime iteration domain. `toMonomial` expands each falling factorial using
`B_{k+1}(t) = B_k(t)*(t-k)/(k+1)`. Addition, subtraction, and constant scaling
operate on canonical coefficients.

Multiplication uses one general identity:

```text
B_i(t) B_j(t)
  = sum_{k=max(i,j)}^{i+j} binomial(k,i) binomial(i,k-j) B_k(t).
```

For a nonnegative integer `t`, count pairs of subsets of sizes `i,j` by the
size `k` of their union. Choose the union, choose its `i`-element first
subset, and choose the `i+j-k` intersection elements from that first subset.
The coefficient is `binomial(k,i) binomial(i,i+j-k)`, equal to the expression
above. Equality for infinitely many integers proves the polynomial identity
over `Q`. The implementation builds exact Pascal coefficients, including
intermediate products larger than 64 bits. Powers are ordinary repeated
multiplication, not a dispatch on the requested exponent.

`shift(h)` uses Vandermonde's polynomial identity
`B_k(t+h) = sum_{j=0}^k B_j(t) B_{k-j}(h)` for any integer `h`.
`composeAffine(start,step)` uses symbolic Horner substitution. Neither
method assumes the start is nonnegative or that the step is one. Rational
affine arguments are also supported in the rational polynomial algebra.

## 3. Discrete integration and proof

For `P(t) = sum a_k B_k(t)`, define

```text
S(t) = sum a_k B_{k+1}(t).
```

`integrate()` constructs exactly `(0,a_0,...,a_d)`. With an initial
accumulator `c`, it constructs `(c,a_0,...,a_d)`.

**Difference obligation.** Applying Pascal's polynomial identity gives

```text
S(t+1)-S(t)
  = sum a_k [B_{k+1}(t+1)-B_{k+1}(t)]
  = sum a_k B_k(t)
  = P(t).
```

**Initial obligation.** Every `B_{k+1}(0)` is zero, so `S(0)=0`, or
`S(0)=c` when an initial accumulator is supplied.

**Summation theorem.** For every nonnegative integer `N`, telescope the
difference identity to obtain `S(N)=c+sum_{t=0}^{N-1}P(t)`. The empty sum
is `c`. This is a proof for all `N`, independent of numerical tests. A
polynomial with zero forward difference is constant, so the initial
condition also makes the polynomial solution unique.

These obligations are checked in tests by canonical coefficient equality,
both using `difference()` and the separately implemented `shift(1)-S`.
Initial values are checked exactly. Direct-execution oracles provide a
separate implementation check; they are not the proof of the theorem.

### General bounds and endpoints

For independent symbolic integer bounds `a,b` the result is

```text
sum_{i=a}^{b-1} P(i) = S(b)   - S(a)   (a <= b)
sum_{i=a}^{b}   P(i) = S(b+1) - S(a)   (a <= b+1).
```

Negative bounds use the same exact polynomial evaluation. The
`SymbolicRangeSum` returned by `sumBetween(inclusiveUpper)` represents the
two independent terms `lowerTerm(a)=-S(a)` and `upperTerm(b)=S(b)` or
`S(b+1)`, plus the ordinary empty-range guard. Its `evaluate(a,b)` returns
zero for empty or reversed ranges. This is a symbolic expression in two
bounds, without requiring a general multivariate polynomial engine.
`sumHalfOpen` and `sumInclusive` provide direct exact evaluation with the
same empty-range semantics.

`sumFrom(a,inclusiveUpper)` binds the lower bound and returns a polynomial
in the upper bound `u`. Its documented domain is `u>=a` for half-open sums
and `u>=a-1` for inclusive sums. Outside that domain, use the guarded range
APIs: a single polynomial cannot encode an arbitrary empty-range clamp.
The tests also prove the symbolic empty cases `S(a)-S(a)=0` and
`S((a-1)+1)-S(a)=0` and both bound-difference identities.

For an induction `i(t)=start+step*t`, summing `P(i(t))` for `N` updates is
`P.composeAffine(start,step).integrate(initial)`. No comparison-based trip
count, rounding of a bound by a step, or termination property is inferred.

### Required examples, derived by the same algorithm

The demo constructs `t^k`, integrates and shifts bounds, and prints these
coefficient vectors for `sum(i^k,i=1..n)`:

| k | Binomial coefficients in n | Conventional polynomial in n |
| --- | --- | --- |
| 1 | `[0,1,1]` | `n/2+n^2/2 = n(n+1)/2` |
| 2 | `[0,1,3,2]` | `n/6+n^2/2+n^3/3 = n(n+1)(2n+1)/6` |
| 3 | `[0,1,7,12,6]` | `n^2/4+n^3/2+n^4/4 = [n(n+1)/2]^2` |
| 4 | `[0,1,15,50,60,24]` | `-n/30+n^3/3+n^4/2+n^5/5` |
| 5 | `[0,1,31,180,390,360,120]` | `-n^2/12+5n^4/12+n^5/2+n^6/6` |

The factorings explain equivalence to the requested textbook expressions;
there is no factorization engine or formula lookup in P0. General integer
and rational quadratic/cubic inputs, and degrees up to 63 for summation,
follow the identical path.

## 4. Polynomial recurrence recognition

The input boundary is a vector of `Recurrence{name,initial,increment}`:

```text
state(t+1) = state(t) + increment(old states at t).
```

This is an explicit normalized additive transition contract, not a parser
for arbitrary source assignments. For example, `x'=x*x` would have increment
`x*x-x`, which is self-dependent and rejected. Expressions support exact
constants, state references, addition, subtraction, and multiplication;
`pow` constructs multiplication nodes. An explicit unsupported node records
operations such as division. Original expression nodes are retained for
intermediate-overflow checks.

The analyzer validates all references, constructs dependency sets, and
solves ready states in dependency order regardless of their input order.
For each state, substitute previously solved polynomials into its increment
and integrate with that state's initial value. A cycle, self-dependency,
unknown reference, unsupported operation, or degree/resource excess returns
`UNKNOWN` with a reason and no partial solutions. Dependencies are
syntactic: an expression such as `x-x` in its own increment is conservatively
rejected, even though a stronger simplifier could eliminate it.

**Correctness induction.** Constant increments are polynomials. If all
predecessors have correct polynomial states, substitution through `+,-,*`
gives the correct polynomial increment. The integration theorem establishes
the next state's recurrence identity and initial value. Induction over the
acyclic graph proves all accepted solutions; induction on the iteration
number establishes agreement with the simultaneous transition system.

For the requested system with arbitrary integer initial values `x0,y0,z0`:

```text
x(t) = x0 + B_1(t)
x(t)^2 = x0^2 + (2*x0+1) B_1(t) + 2 B_2(t)
y(t) = y0 + x0^2 B_1(t) + (2*x0+1) B_2(t) + 2 B_3(t)
z(t) = z0 + y0 B_1(t) + x0^2 B_2(t)
            + (2*x0+1) B_3(t) + 2 B_4(t).
```

Every update reads the old state. In particular `z` integrates `y(t)`, not
`y(t+1)`. Tests also cover a fourth level `w'=w+y*y+x*z`, whose increment
is polynomial after substitution, and initial indices below zero with
positive steps other than one. There is no claim that arbitrary polynomial
recurrences, such as repeated squaring, have polynomial closed forms.

## 5. Integer semantics and overflow obligations

The polynomial algebra itself is over `Q`; it can represent rational-valued
sums. The recurrence interface has explicitly separate integer modes.
`AnalysisResult::integerLifts` stores exact mathematical polynomials;
`valueAt` enforces the semantic mode and iteration domain. The raw lifts
must not be mistaken for fixed-width IR expressions.

| Mode | Contract and result |
| --- | --- |
| A: `Mathematical` | Unbounded integer state sequences. Rational coefficients in increments are allowed only if the solved states are integer-valued by the Newton-coefficient criterion. Noninteger initial/state sequences return `UNKNOWN`; use the separate rational polynomial algebra for those sums. No bit width may be specified. |
| B: `Modular` | All source states and operations share an explicit width `w`. Source initials/constants must be integers; operations are `+,-,*`. Evaluate the exact integer lift, then return its canonical residue in `[0,2^w)`. Initials and constants may be negative or larger than the modulus. |
| C: `SignedNoOverflow` | Requires width `w` and a finite, explicit maximum number of updates `N`. Initial values/constants must fit the signed range. Every original intermediate and state update needs a sound range proof, or the result is `UNKNOWN` unless the caller supplies a named no-overflow assumption. An assumed result is `Conditional`, never `Proven`. |

Rational source operations are rejected in fixed-width modes even if an
expression such as `x*(x-1)/2` happens to be integer-valued over mathematical
integers. Such expressions are not generally preserved by reduction of
`x` modulo `2^w`. There are no division, cast, shift, saturation, or poison
operations in the accepted transition language.

**Modular proof.** Reduction `Z -> Z/(2^w)` is a ring homomorphism for
addition, subtraction, and multiplication. Given integer initial states and
constants, a structural induction proves each increment is congruent to
its exact lift; iteration induction proves every lifted state is congruent
to source execution, including after repeated wraps. The synthesized
binomial representation is evaluated in exact arithmetic before reduction.
The mathematical iteration count is not reduced modulo the state width.

An adversarial example is the i8 sum `1+...+16 = 136`: computing
`(16*17 mod 256)/2` gives `8`, which is wrong. Another is the sum through
256, whose i8 residue is `128` despite `256 mod 256 = 0`. Both are tests.
P0 emits no division-based IR transformation and makes no claim that its
rational expressions can be directly lowered using native-width division.

**Signed proof.** On integer `0<=t<=N`, each `B_k(t)` is nonnegative and
nondecreasing. For `p(t)=sum a_k B_k(t)` a sound enclosure is

```text
lower = a_0 + sum_{k>=1, a_k<0} a_k B_k(N)
upper = a_0 + sum_{k>=1, a_k>0} a_k B_k(N).
```

The analyzer checks every original expression node on `[0,N-1]` and every
solved state on `[0,N]` against `[-2^(w-1),2^(w-1)-1]`. The latter includes
the last update. For `N=0` there are no executed increment operations.
Checking nodes before discarding them avoids accepting `x*x-x*x` merely
because its result is zero when `x*x` itself overflows. By the recurrence
identity, these checks cover all original additions and multiplications;
they do not claim safety of arbitrary rearrangements in generated IR.

The enclosure can be imprecise. For example, safe `s(t)=t*(20-t)` on
`0<=t<=20` has a loose binomial enclosure exceeding i8. Without an explicit
assumption the answer is `UNKNOWN`. With an assumption that every source
intermediate and update fits over this domain, the answer is `Conditional`,
and the assumption text is retained. Initial/constant type violations
remain rejected. `valueAt` rejects iterations outside the proof/assumption
horizon and any evaluated final value contradicting the signed range; it
does not independently discharge a caller's intermediate-overflow premise.

There is no automatic equivalence between these modes. No `nsw`, `nuw`,
finite loop trip count, or termination assumption is silently introduced.

## 6. LLVM implementation inspected

The source tree inspected was `/home/steven-studio/work/llvm-project`, clean
at commit **`7615da10b75c57286a4032eac8bb43c1b8d2ba46`**. Its
`cmake/Modules/LLVMVersion.cmake` declares **LLVM 24.0.0git**, a development
snapshot, not a released LLVM 24. The commit subject is
`[M68k] Finish implementation of MOVX (move and extend) pseudo-instructions
and fix related errors (#218938)`, committed 2026-09-26. All line numbers
below refer to this precise source revision.

The two principal sources are
[ScalarEvolutionExpressions.h at the inspected commit](https://github.com/llvm/llvm-project/blob/7615da10b75c57286a4032eac8bb43c1b8d2ba46/llvm/include/llvm/Analysis/ScalarEvolutionExpressions.h)
and
[ScalarEvolution.cpp at the inspected commit](https://github.com/llvm/llvm-project/blob/7615da10b75c57286a4032eac8bb43c1b8d2ba46/llvm/lib/Analysis/ScalarEvolution.cpp).
These findings came from the local files; the links pin the corresponding
upstream sources for review.

| Implementation | Location and observed behavior |
| --- | --- |
| `SCEVAddRecExpr` | `ScalarEvolutionExpressions.h:324`. A loop-associated n-ary recurrence whose operands are invariant with respect to that loop. Two operands are affine; more operands represent higher-degree recurrences. `getStepRecurrence` drops the first operand. |
| `getAddRecExpr(Start,Step,Loop,Flags)` | `ScalarEvolution.cpp:3685`. If the step is another AddRec for the same loop, flatten its operand chain after `Start`. Drop flags that are not valid for the flattened recurrence. |
| `getAddRecExpr(Operands,Loop,Flags)` | `ScalarEvolution.cpp:3704`. Check operand types and availability at loop entry, remove trailing zero operands, collapse one operand to a constant/invariant expression, strengthen flags when justified, canonicalize nested loops when valid, and intern the result. |
| AddRec addition | In `getAddExpr`, around lines 2876–2975. Fold invariant addends into the start and combine coefficients of same-loop AddRecs. |
| `getMulExpr` | `ScalarEvolution.cpp:3133`. General folding, flattening, and constant simplification; around 3300, distribute invariant scaling across AddRec coefficients with guarded flag propagation; around 3356–3445, multiply same-loop polynomial AddRecs using combinatorial coefficients. |
| `Choose` | `ScalarEvolution.cpp:3087`. Helper for constant combinatorial coefficients in products. Uses `uint64_t` and signals intermediate overflow; the product fold can decline. This differs from the helper for symbolic iteration arguments. |
| `BinomialCoefficient` | `ScalarEvolution.cpp:868`. Builds the SCEV for a symbolic binomial at the requested result width, accounting for modular overflow and noninvertible powers of two. Returns `CouldNotCompute` for `K>1000`. |
| `evaluateAtIteration` | `ScalarEvolution.cpp:981` and `986`. Evaluate an AddRec as the sum of operand `k` times `binomial(iteration,k)`. The coefficient multiplication occurs **after** the binomial calculation. |
| `getExitValue` | `ScalarEvolution.cpp:1008`. Evaluate at the exact backedge-taken count when available; otherwise return `CouldNotCompute`. This is not a guarantee that every loop has a computable exit count. |
| PHI recognition | `createSimpleAffineAddRec`, line 5729, handles invariant additive steps. `createAddRecFromPHI`, line 5791, symbolically analyzes the backedge value and accepts an additive accumulator that is invariant or an AddRec for the same loop. `createNodeForPHI`, line 6048, can ultimately return an unknown. |

LLVM's chain `{A,+,B,+,C,+,D}<L>` means
`A+B*B_1(t)+C*B_2(t)+D*B_3(t)` at loop iteration `t`, not the monomial
polynomial `A+B*t+C*t^2+D*t^3`. The step of this chain is `{B,+,C,+,D}`.
Flattening an accumulator's polynomial step is already a form of discrete
integration in LLVM. P0 does not claim to invent this facility or add it to
LLVM.

LLVM's multiplication fold iterates over output degree `x` and summand
indices `y,z`, using `Choose(x,2*x-y)*Choose(2*x-y,x-z)` times operands
`A[y-z]*B[z]`. This is another indexing of binomial-basis multiplication.
The default `scalar-evolution-max-add-rec-size` is **8** (line 212);
`getMulExpr` checks the proposed product size and expression complexity
before folding. It is not a universal ban on constructing an AddRec with
more than eight operands: flattening an accumulator's degree-7 step can
produce nine operands, as the experiment demonstrates. Other expression
size/depth limits and constant coefficient overflow can also prevent folds.

For modular binomials LLVM factors `k! = 2^T * O` with `O` odd. It builds
the falling-factorial product at width `W+T`, divides by `2^T`, truncates,
and multiplies by the inverse of `O` modulo `2^W`. This is why native-width
multiply followed by division by `k!` is not a valid replacement. P0 uses
arbitrary-precision exact division instead and does not copy this IR
expansion implementation.

LLVM's `nuw`, `nsw`, and AddRec `nw` flags have separate scope and poison
requirements (`ScalarEvolution.h:72–100`). In particular, `nw` alone does
not establish signed no-overflow. P0 does not import these flags as blanket
mathematical-integer assumptions.

**Already in LLVM:** polynomial AddRecs, polynomial multiplication,
canonicalization, recurrence recognition, modular binomial evaluation,
and some exact exit-value computations. **P0-specific work:** an isolated
exact rational algebra, guarded symbolic bound expressions, an explicit
additive-transition API, distinct semantic modes and conditional proofs,
and independently tested contracts for a possible CFG analysis client.
P0 has a different resource limit and a narrower input language; it is
not a replacement for ScalarEvolution or a claim that LLVM lacks summation.

## 7. Actual LLVM comparison

The experiment used these existing tools without rebuilding or modifying
their source/build directories:

```text
clang: /home/steven-studio/work/clang24-build/bin/clang
       24.0.0git, reports commit 7615da10b75c57286a4032eac8bb43c1b8d2ba46
opt:   /home/steven-studio/work/polly24-build/opt-asserts/bin/opt
       24.0.0git, optimized build with assertions, x86_64-unknown-linux-gnu
```

The `opt` build's `include/llvm/Support/VCSRevision.h` reports the same
revision. The source status, header text, executable hashes, source hashes,
IR hashes, full version output, and commands are preserved in
[report.json](../test/polynomial-summation/llvm/observed/report.json).
The PATH default is LLVM 18.1.3, so the experiment deliberately uses the
explicit LLVM 24 paths above. No result here is attributed to LLVM 18.

The fixture uses `uint64_t` arithmetic, so its results are modulo `2^64`.
The frontend is invoked with `-O0 -Xclang -disable-O0-optnone` and named
values. The analysis input is produced by
`-passes=mem2reg,loop-simplify,lcssa -verify-each`. Analysis uses
`-passes='print<scalar-evolution>,print<loops>'`. Optimization uses
`-passes='default<O2>' -verify-each`, followed by another analysis. All
fixtures have one loop before optimization.

| Function / recurrence | Relevant SCEV before O2 | Exit-value observation | Loops after default O2 |
| --- | --- | --- | ---: |
| `sum_p1` | `{0,+,1,+,1}` | Symbolic exact modular exit | 0 |
| `sum_p2` | `{0,+,1,+,3,+,2}` | Symbolic exact modular exit | 0 |
| `sum_p3` | `{0,+,1,+,7,+,12,+,6}` | Symbolic exact modular exit | 0 |
| `sum_p4` | `{0,+,1,+,15,+,50,+,60,+,24}` | Symbolic exact modular exit | 0 |
| `sum_p5` | `{0,+,1,+,31,+,180,+,390,+,360,+,120}` | Symbolic exact modular exit | 0 |
| `sum_p6`, `sum_p7` | Degree-7 and degree-8 accumulator AddRecs | Symbolic exact modular exits | 0 |
| `sum_p8` | Accumulator `%s.0` remains unknown; a product of AddRecs does not fold at the default size limit | Accumulator exit `<<Unknown>>` | 2 |
| `general_cubic` | `{0,+,-2,+,51,+,78,+,42}` for the fixture's cubic | Symbolic exact modular exit | 0 |
| `shifted_square` | `{initial,+,start*start,+,(2*start+step)*step,+,2*step*step}` | Exit depends symbolically on `n,start,step,initial` | 0 |
| `triangular` | `z={0,+,0,+,0,+,1,+,2}`; polynomial x and y also recognized | Symbolic exact modular exit | 0 |
| `nonlinear_self`: `x'=x*x+1` | `%x.0` unknown | Trip count known; x exit unknown | 2 |
| `exponential`: `x'=2*x` | `%x.0` unknown | Trip count known; x exit unknown | 2 |
| `unknown_exit`: loop while repeated-squaring x is below n | `%x.0` unknown | Backedge count unpredictable; exit unknown | 1 |
| `fixed_square`: 1 through 10 | `{0,+,1,+,3,+,2}` | Exact accumulator exit `385`; optimized IR returns `385` | 0 |
| `observable_square`: volatile store each iteration | Same accumulator AddRec as `sum_p2` | Symbolic exact modular exit | 2 |

The remaining pairs of loops are an unrolled main loop and a cleanup loop,
not evidence that the original loop was eliminated. The observable case
shows that recognizing a closed form is insufficient to remove observable
per-iteration effects. These are measured results for this fixture,
pipeline, and binary; no conclusion about arbitrary loop shapes follows.

For the pretested canonical fixture, the header's backedge-taken count is
`n`; the condition is also visited for the final exit check. Do not blindly
substitute `n-1` for the exit iteration of every AddRec. O2 changes the loop
shape and consequently the form of the expanded expressions. The raw
reports distinguish header PHIs, post-update values, and LCSSA exit values.

A separate **non-default** experiment sets
`-scalar-evolution-max-add-rec-size=16`. Then `sum_p8` has the accumulator

```text
{0,+,1,+,255,+,6050,+,46620,+,166824,+,317520,+,332640,+,181440,+,40320}
```

and its loop count after O2 is **zero**. The full function reports are in
[sum-p8-size16.txt](../test/polynomial-summation/llvm/observed/sum-p8-size16.txt),
and the resulting module is
[optimized-size16.ll](../test/polynomial-summation/llvm/observed/optimized-size16.ll).
This confirms that the observed default degree-8 failure is an analysis
resource limit, not an absence of polynomial AddRec support.

For both configurations, the optimized LLVM IR was compiled into a
temporary shared library and executed against independent Python-integer
oracles: **1,814 comparisons per configuration, all passed**. These include
zero iterations, counts around 127/128 and 255/256/257, counts through 1024,
large initial indices and steps causing 64-bit wrap, nonzero accumulators,
the triangular system, and the volatile output. `unknown_exit` was analyzed
but deliberately not executed because its termination is not established.
The temporary shared libraries are removed by the script; the LLVM sources
and builds are only read.

## 8. Validation and reproduction

Run from this worktree; the standalone build needs a C++17 compiler, Boost
headers, CMake, Ninja, and Python 3. No Python package installation or
production compiler build is needed.

```bash
cmake -S research/polynomial-summation -B build/polynomial-summation \
  -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/polynomial-summation -j 3
ctest --test-dir build/polynomial-summation --output-on-failure -V
build/polynomial-summation/polynomial-summation-driver --demo
```

Observed with GCC 13.3.0, Boost 1.83.0, and Python 3.13.9:

| Check | Result |
| --- | --- |
| C++ exact algebra, recurrence, range, and rejection checks | **42,277 PASS** |
| Independent Python `int` / `Fraction` comparisons | **13,443 PASS**, deterministic seed `0x5ce2026` |
| Release CTest | **2/2 PASS**, about 2–3 seconds |
| Debug AddressSanitizer + UndefinedBehaviorSanitizer, with leak detection | **2/2 PASS**, same 42,277 / 13,443 checks, about 50 seconds |
| LLVM default-O2 execution comparisons | **1,814 PASS** |
| LLVM O2 with AddRec-size 16 execution comparisons | **1,814 PASS** |
| Production-source/build-graph audit | Existing tracked files unchanged from baseline; new library has no production CMake registration |

The first sanitizer attempt in the execution sandbox failed because
LeakSanitizer cannot operate under its ptrace tracing. The same suite was
rerun outside that sandbox with approval and passed, including leak
detection; this environment failure is not hidden as a successful run.
The final sanitizer run includes the two-symbolic-bound API.

```bash
cmake -S research/polynomial-summation -B build/polynomial-summation-sanitize \
  -G Ninja -DCMAKE_BUILD_TYPE=Debug -DPOLYNOMIAL_SANITIZERS=ON
cmake --build build/polynomial-summation-sanitize -j 3
ctest --test-dir build/polynomial-summation-sanitize --output-on-failure -V

python3 test/polynomial-summation/llvm/compare_llvm.py \
  --clang /home/steven-studio/work/clang24-build/bin/clang \
  --opt /home/steven-studio/work/polly24-build/opt-asserts/bin/opt \
  --llvm-source /home/steven-studio/work/llvm-project \
  --output test/polynomial-summation/llvm/observed
```

The evidence directory contains `frontend.ll`, `canonical.ll`,
`optimized.ll`, `scev-before.txt`, `scev-after.txt`, the two size-16 files,
and `report.json`. The comparison script reports observed behavior; it does
not enforce the current LLVM loop counts as universal golden expectations.

### Test independence and adversarial coverage

Each sum query derives a symbolic result and checks both coefficient-based
difference identities and the initial condition before returning a value.
Each supported triangular query checks every state's initial value and
increment identity. The independent process uses conventional Horner
evaluation plus direct summation or simultaneous state updates, never a
binomial-basis conversion, finite-difference table, or Faulhaber formula.
It also checks the exported monomial coefficients against that direct
oracle. Modular simulation reduces source operations during execution,
whereas the implementation evaluates an exact lift then reduces once.

The 13,443 Python comparisons comprise 275 generalized-binomial cases,
6,527 ordinary/general/rational/shifted sum cases, 36 high-degree cases,
728 mathematical triangular cases, 5,824 modular triangular cases,
44 proven signed triangular cases, and 9 multiple-wrap cases.

Coverage includes powers 1–5 and higher powers through 63; general
quadratics and cubics; nonzero and rational initial accumulators in the
polynomial algebra; shifted inclusive/exclusive and independent symbolic
bounds; zero and reversed ranges; negative initial indices; steps 1, 2, 3,
5, 7, 11, 13, 19 as applicable, plus zero/negative-step algebra checks;
three- and four-level triangular systems; rational coefficients; integers
exceeding 250 bits; modular widths 1, 2, 3, 8, 16, 32, 65, 127; and signed
proof versus assumption behavior.

Binomial tests include `k=0`, `n=0`, `0<=n<k`, `n=k`, negative `n`, and
iteration arguments of magnitude `10^50`. Products of degree-32 basis
polynomials have coefficients exceeding 64 bits. Exact product-rule checks
use `Delta(pq)=Delta(p)*q+p*Delta(q)+Delta(p)*Delta(q)`, catching incorrect
product coefficients without relying only on evaluations. Boundary tests
exercise integration to degree 64 and reject degree growth beyond the cap.
Overflow tests cover division after an already-wrapped product, repeated
counter wraps, canceled overflowing intermediates, the final state update,
out-of-range initial values, and enforcing the signed horizon.

Rejection tests include repeated squaring, doubling, mutual dependencies,
unmodeled state references, unsupported division, rational bitvector
operations, nonintegral integer-state sequences, duplicate names, and
degree/depth limits. `UNKNOWN` cannot be evaluated and never carries a
partial successful solution.

The production compiler regression and FPGA suites were not rerun: their
sources, tests, and build registration are unchanged, and no production
target links this library. The claim of unchanged compiler behavior rests
on that integration boundary, not on a new hardware or full compiler test
run.

## 9. Complexity and limits

Let `d` bound the resulting degree. Counts below refer to exact arithmetic
operations; their bit cost depends on coefficient size and `log(|t|+1)`.
Rational normalization includes arbitrary-precision gcd work.

| Operation | Arithmetic complexity / storage |
| --- | --- |
| Normalization, addition, subtraction, scaling, difference, integration | `O(d)` time and `O(d)` output storage |
| Evaluation at one integer | `O(d)` arithmetic operations; no loop over the iteration count |
| Monomial/binomial conversion in either direction | `O(d^2)` time, `O(d)` working vectors |
| Multiplication | `O(d^3)` time, `O(d^2)` Pascal-table storage plus `O(d)` result |
| Integer shift | `O(d^2)` time, `O(d)` storage |
| Affine substitution in this implementation | `O(d^3)` time using Horner and general multiplication |
| Power | At most `O(log exponent)` polynomial multiplications, each subject to the degree cap |
| Signed range enclosure | `O(d^2)` time with the current binomial helper; independent of the number of iterations except integer bit sizes |
| System analysis | Dependency scheduling up to `O(V*(V+E))`; each unique expression node is substituted once per state, with polynomial-operation and range-proof costs above |

The degree cap is 64, so a generic summand of degree at most 63 can be
integrated. Algebra operations throw `length_error` if their resulting
degree exceeds the cap; recurrence analysis converts this to `UNKNOWN`.
Exponent arguments are also capped at 64. There are at most 128 states,
4096 unique expression nodes per increment, expression traversal depth 128,
and fixed widths 1–4096. Coefficients and iteration numbers have no fixed
machine-width limit, subject to available memory. These are resource bounds,
not per-power special cases or claims of completeness below the bounds.

Unsupported P0 cases include cyclic/self-dependent increments even when a
special initial condition would make them constant; general nonlinear
recurrences; piecewise/guarded updates; division, variable exponents,
bitwise operations, casts, mixed widths, shifts and saturation; memory,
aliasing and side effects; unknown coefficient parameters; multiple loop
iteration variables; and general termination or trip-count discovery.
The polynomial variable is the symbolic iteration count; recurrence initial
values and coefficients are supplied exact constants. The two-bound sum
representation supports symbolic endpoints but does not introduce a
general multivariate coefficient domain. Signed range analysis is sound
and deliberately incomplete.

## 10. Proposed CFG Recognition integration — future work only

A future CFG symbolic-transition client can provide pure additive updates
over a named old-state vector, exact initial values, an externally justified
iteration domain, and an explicit integer mode. It must preserve the
executed expression structure for signed overflow reasoning and distinguish
old versus newly assigned values when translating sequential statements
into a simultaneous transition. Any simplification that loses intermediate
overflow behavior needs a separate proof.

The client could consume `Proven` results directly for analysis;
`Conditional` results must retain and discharge or explicitly accept their
assumptions, and `UNKNOWN` requires a fallback. Where LLVM already has
canonical loop IR and suitable SCEVs, use ScalarEvolution directly for
recurrence discovery, trip counts, and safe expansion instead of duplicating
those facilities here. Exact rational results must not be lowered through
native-width division without a new, proved lowering strategy.

No such adapter, CFG extraction, IR expansion, production pass, or GEMM
lowering change is included in P0. Any future loop elimination must also
prove control-flow, side-effect, memory, and poison obligations; polynomial
value equality alone is insufficient. P0 stops at this tested standalone
analysis boundary.
