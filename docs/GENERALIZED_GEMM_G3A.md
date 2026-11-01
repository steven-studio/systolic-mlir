# Generalized GEMM recognition: Phase G3a, exact finite-domain coordinate recovery

G3a recovers candidate coordinates (P, I, J, K) for

    C[h(x)] += A[f(x)] B[g(x)]

from the exact access behaviour of a finite domain. It is research-only: nothing
here is linked into production lowering, and no G3a result can authorize one. A
candidate is a statement about finite data at one parameter point. It is no proof
of GEMM equivalence. The earlier records are `docs/GENERALIZED_GEMM_G0_G1.md` and
`docs/GENERALIZED_GEMM_G2A.md`.

Decisions recorded on 2026-11-01:

1. `genuine` is strict: a complete box, unit multiplicity, injective views, and at
   least two points on each of I, J, K in every batch. It does not say that other
   shapes are invalid. Validity, domain structure, multiplicity, shape, injectivity,
   strict non-degeneracy and eligibility are reported independently, and a valid
   candidate with K = 1 or another degenerate axis stays available to G4 to G7.
2. Complete enumeration needs a verified G2a certificate, re-checked here against
   the exact fingerprint and scope. A finite number of successful runs is not
   evidence of completeness.
3. Scalar accumulators are unsupported until their output storage is represented
   exactly. No `h` is invented.
4. Canonical failure gives Unknown with partition hints. There are no arbitrary
   residual coordinates, and no global refutation from the failure of a class.
5. Sites are grouped by the exact (C, A, B) root triple. Unrelated operands are not
   merged, and site identity and execution multiplicity are preserved.

## The problem

Fix a program, a parameter point v and a site group.

- E is the ordered event sequence of the run.
- t(e) = (h(e), f(e), g(e)), and C0 requires hRead(e) = h(e).
- T = t(E) is the set of distinct triples, and m(t) = |t⁻¹(t)| its multiplicity.

Maps P, I, J, K on T are a **decomposition** when:

- **FD:** h = H(P,I,J), f = F(P,I,K), g = G(P,K,J).
- **INJ:** H, F and G are injective.
- **BOX:** (P,I,J,K) is injective on T, and its image is ⊔ₚ {p} × Iₚ × Jₚ × Kₚ with
  nonempty factors.

Cardinalities are unconstrained, and a label may stand for a tuple, so every
coordinate may be multidimensional. A label has no fixed meaning in terms of the
source: `report.shape.support` gives the fewest entries of x that determine it, when
any do.

### What a found candidate claims

For the runs of this point that complete, the access multiset is exactly

    {{ (h,f,g)(e) : e ∈ E }} = {{ (H,F,G)(φ(t)) with multiplicity m(t) : t ∈ T }}

Nothing else. Reduction order, floating-point behaviour, aliasing, the initial output
and every other write belong to G4 to G7. `CheckedCandidate::equivalence` is always
Unknown, `Eligibility` has no value that authorizes a transformation, and the JSON
carries `"authorizes_lowering": false`.

### Evidence premises

1. The program is supported, control and address oblivious, and no integer control
   or address reads a float. Float parameters are not enumerated, and this premise
   is why that is exact.
2. The scope is finite within `maxPoints`.
3. `checkCertificate` accepts the certificate here, for this program and this scope.
   A proof over a narrower scope does not carry over, and neither does an
   authorization given to the proposer.
4. The run of this point returned, and `admitsClaim(evidence, Coverage)` admits it.

Under obliviousness the event sequence does not depend on memory, up to a stop that
does. So the domain is that of completing runs, as G1 already records. Each run uses
one memory fill; the tests compare two.

## The canonical algorithm

    ρI = ker h ∨ ker f      ρJ = ker h ∨ ker g
    ρK = ker f ∨ ker g      ρP = ker h ∨ ker f ∨ ker g

    C1  ker h = ρI ∧ ρJ     C2  ker f = ρI ∧ ρK     C3  ker g = ρK ∧ ρJ
    C4  (ρI,ρJ,ρK) is injective on T, and per block p of ρP
        |Tₚ| = |Iₚ|·|Jₚ|·|Kₚ|

The joins use union-find. That code is an untrusted proposer: `CandidateChecker`
links neither it nor union-find, works from maps over events, and is the only code
that can issue CandidateFound or CandidateRefuted.

## Theorems

**Theorem 1 (recovery).** Under FD, INJ and BOX: ker(P,I) = ker h ∨ ker f,
ker(P,J) = ker h ∨ ker g, ker(P,K) = ker f ∨ ker g, and
ker P = ker h ∨ ker f ∨ ker g.

*Proof.* By INJ, h and f each determine (P,I), so ker h and ker f refine ker(P,I), and
so does their join. Conversely, take two points with the same (p,i). BOX gives the
path (p,i,j,k) →ₕ (p,i,j,k′) →_f (p,i,j′,k′), with every point in T, so they lie in one
join block. J and K follow by symmetry. Each view determines P, so the triple join
refines ker P, and within batch p two moves connect any two points. ∎

**Theorem 2 (exact criterion and uniqueness).** C1 to C4 hold if and only if a
decomposition exists, and it is then unique up to relabelling.

*Proof.* (⇐) By Theorem 1, ρ equals the true kernels. So ker h = ker(P,I,J) = ρI ∧ ρJ,
and likewise for f and g, and BOX is C4. (⇒) Take blocks as labels. Each ρ refines ρP,
so a label of I determines P, and C1 makes H well-defined and injective; likewise F
and G. If (I,J,K) agree on two triples, C1 to C3 make h, f, g agree, so the triples are
equal. Injectivity with the count gives the box. Theorem 1 fixes every kernel, which
is the uniqueness. ∎

**Theorem 3 (without a box).** If h and f determine (P,I), then ρI refines ker(P,I).
Equality holds exactly when, for every (p,i), the j-k bipartite graph of that slice is
connected. Likewise for J and K.

*Proof.* A join is a least upper bound. In a slice each point is an edge between its
j and its k; an h-move shares j and an f-move shares k, so join blocks are the graph's
components. ∎

So ρ is the finest possibility under injective views, and passing C1 to C3 without C4
gives a candidate on a partial domain, labelled as such. It need not be unique.

**Theorem 4 (dependence does not give Cartesian structure).** I = J = K = x satisfies
every FD, and fails the box whenever |T| > 1, since |T|³ ≠ |T|.

**Theorem 5 (non-uniqueness without injectivity).** For `C[i,j] += A[i,k] B[k]` over
[0,n) × [0,n) × K, both J = j and J′ = (j + i) mod n satisfy FD and the box, with
different partitions. So no algorithm can recover "the" coordinates from partitions
alone, because there is not one answer. Failure of canonical recovery therefore gives
Unknown.

## Limits

The method is exact only within its class, and it is not complete for arbitrary
nonlinear functions. A nonlinear view works when it is injective on the domain.

| Case | Behaviour |
| --- | --- |
| `C[i,j] += A[k] B[k,j]`, `C[i,j] += A[i,k] B[k]` | C1 fails: Unknown, though a GEMM with a non-injective view exists |
| `C[p,i,j] += A[i,k] B[k,j]` | P needs non-injective F and G: Unknown |
| `C[i+j] += A[i] B[j]` | All joins reach the top: Unknown |
| `C[i,j] += A[i] B[j]` over k | Found as outer shape with uniform multiplicity. K stays hidden in m |
| `C[p,i,j] += A[i,k] B[p,k,j]` | Found with J = (p,j), which is a true single GEMM |
| Triangular, a hole, a band | Found, partial. The axes are exact when the slices are connected |
| A diagonal | Found as separate 1 x 1 x 1 tiles, because its slices are disconnected |

Non-injective views make joins too coarse, and a missing box makes them too fine.
When C1 to C4 pass, the answer is still right: Theorem 2 is about the data and not
about what the author meant. A small parameter point can pass by accident, as n = 1
does, so every result holds for its own point and none is extended.

Also unsupported or unknown:

- A program whose only sites are scalar accumulators is Unsupported. Beside a direct
  site, only its own group is.
- A site without an exact output or output read, a non-oblivious program, and every
  G1 refusal are Unsupported. G1 records a MAC only when the multiply and its loads
  share a block, so a nest that splits them has no site, and reports none.
- No proven certificate, any limit, a stopped run and an empty group are Unknown for
  their point or for the whole enumeration, with no partial table. Coverage inherits
  G2a's: `c_wrap` and `j_step_wrap` have no certificate, so they are not enumerated.
- A supported program with an unauthorized separation assumption in its proof is
  Unknown until it is authorized. Houdini can keep an invariant nobody needed whose
  consecution reads memory, which adds this premise to a proof that does not need it:
  a lone scalar accumulator does this. It is imprecise and sound.

## Status

| Status | Meaning |
| --- | --- |
| CandidateFound | The accesses decompose through this candidate, with the reported properties |
| CandidateRefuted | This candidate fails on exact events. For C0, every candidate fails, since no decomposition can be a += |
| Unsupported | The form or the program is outside G3a |
| Unknown | Evidence or class missing. Never a refutation of the program |

`CheckedCandidate` has no public constructor for the first two. Each report holds
facts that are independent of each other, and deterministic counterexamples: the
earliest event in order that conflicts, and the first absent point in label order.
Memoization covers interned tuples and joins with equal kernels. It is evidence of
nothing, and the checker runs on every point.

## How G3b extends this

SyGuS/CEGIS stays an independent synthesis mechanism for non-injective views and
symbolic coordinate functions. It is one more untrusted proposer behind the same
checker and the same tables. A counterexample here is its CEGIS feedback, and the
partition hints say which of C1 to C3 failed. Coordinates become terms over x. FD and
injectivity become universal queries, which may use an over-approximated domain,
while coverage still needs an exact one (I8). Families over parameters belong there,
since G3a never extends past its points.

## Trusted base

Everything in the G1 and G2a records, including Z3 through the certificate check. The
checker and the enumerator use no solver otherwise.

## Build and test

```sh
ninja -C <build> check-generalized-gemm
python3 research/generalized-gemm/test/fuzz/coordinate_mutation.py --source . \
  --scratch <empty dir> --llvm-dir $L/lib/cmake/llvm --mlir-dir $L/lib/cmake/mlir
```

## Validation (2026-11-01, LLVM 24.0.0git, Z3 4.8.12, base 13205db)

| Check | Result |
| --- | --- |
| `check-generalized-gemm` | 11 of 11 CTest suites, including G1, G2a and 11 lit tests |
| G3a unit | 351 checks, 0 failures |
| Theorem 2, exhaustive tables | 18,587 tables: 1,185 decompose and 17,402 do not, all equal to a search over every set partition |
| Facts, seed 20261103 | 200,000 candidates, 21,509 valid, every field and counterexample recomputed and equal |
| Theorem 1, same seed | 10,000 hidden batched boxes of unequal size, all exact, 2,473 genuine |
| Theorem 3, same seed | 40,000 subsets, 31,234 with connected slices, 0 failures |
| Programs, seed 20261103 | 331 programs, 993 points, 21,606 events in order, 0 disagreements, 0 closed gates, 0 timeouts |
| Status in that run | 743 found, 3 refuted, 247 unknown, 105 of the found genuine |
| Named catalogue | 32 hand-stated expectations in that run |
| Two memory fills | 120 tables compared, all identical |
| ASan, UBSan, leaks | Clean: unit, 20,000 candidates, 71 programs |
| Mutation | 88 of 88 killed, baseline passing first |
| G1 output | Byte-identical to b0f0814: 162 outputs |
| G1 and G2a sources | Unchanged from 13205db |
| Polynomial suites | 13 of 13 |
| `check-systolic` | 188 of 190; the 2 failures are the known legacy ones |

The first mutation run killed 81 of 87. The survivors were test gaps, and one test was
vacuous: its program had no MAC site, so it never reached the gate it named. The code
needed no fix for any of them. The oracle also found a real defect before that: a
coordinate no entry of x determines reported an empty support, which reads as needing
no entry. That is fixed, and the oracle checks it. After the mutation run, one guard
was added to the oracle's generator, for an empty name list. Seeds that did not reach
it generate the same programs. No FPGA or hardware path is involved.
