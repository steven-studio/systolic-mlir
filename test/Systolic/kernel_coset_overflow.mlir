// REQUIRES: cfront-clang
// RUN: %python %S/Inputs/kernel_coset_overflow.py %S/../../runtime/offload %t %systolic-cc

// Emitted arithmetic must fit in 64 bits (Phase F-KC-5b-0). Elimination multiplies
// coefficients, so a correct loop bound could overflow at run time although every
// coefficient and every source value fits: past 2^63 a 55-MAC nest over 10-element
// memrefs ran 0 MACs and exited 0. The IR interpreter uses unbounded integers and
// cannot see this, so the runner compares real machine code, at -O0 and -O2, with
// an independent enumeration of each nest: near INT64_MIN and INT64_MAX, with large
// and negative coefficients, equalities and two eliminations.
