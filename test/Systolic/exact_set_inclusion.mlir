// RUN: systolic-opt "--test-affine-matmul-discovery=exact-sets=true exact-sets-random=3000 exact-sets-seed=7" %s \
// RUN:   -o /dev/null 2>&1 | FileCheck %s
//
// docs/AFFINE_GEMM_MAPPING.md section 19.14: exactIsSubset decides a ⊆ b for
// conjunctive integer sets with emptiness tests only (no set difference). In
// the LLVM 24 build of this project PresburgerSet::subtract / isSubsetOf /
// isEqual answer wrongly with two or more equality-pinned locals; the first
// case below is that counterexample: B = [0, 7] x [0, 14] is NOT a subset of
// Y = {(y0, y1) : y1 = 3 q1 in [0, 13], b = 4 q2 in [0, 7], b <= y0 <= b + 3}
// ((0, 1) is in B, not in Y), which the library reports as a subset. Every
// exact answer is checked against the points of a box holding both sets
// ("by points"), and every "no" comes with a witness checked to be in a and
// not in b. The library's own answer is printed, not checked (it is the
// subject of a separate upstream fix).
//
// CHECK: exact-sets box in two-stepped: exact no, by points no, library {{yes|no}}, witness ({{-?[0-9]+}}, {{-?[0-9]+}}) in a: yes, in b: no
// CHECK: exact-sets two-stepped in box: exact yes, by points yes
// CHECK: exact-sets box in one-stepped: exact no, by points no, library {{yes|no}}, witness ({{-?[0-9]+}}, {{-?[0-9]+}}) in a: yes, in b: no
// CHECK: exact-sets two-stepped in its plain form: exact yes, by points yes
// CHECK: exact-sets plain form in two-stepped: exact yes, by points yes
// CHECK: exact-sets interval in floor-division set: exact yes, by points yes
// CHECK: exact-sets floor-division set in interval: exact yes, by points yes
// CHECK: exact-sets interval in existential window: exact undecided, by points yes
// CHECK: exact-sets random: {{[0-9]+}} agree with the points, {{[0-9]+}} undecided, 0 wrong, of 3000 (the library's isSubsetOf wrong on {{[0-9]+}})
// CHECK: exact-sets failures: 0

module {
}
