// RUN: %python %S/Inputs/kernel_coset_multi.py systolic-opt %t

// Several kernel-coset candidates in one module (finding F-KC-5). A successful
// lowering erases its nest, and the dispatcher used to re-test isolation against
// the whole candidate list afterwards, reading freed operations: a heap
// use-after-free from the second candidate on, a segmentation fault from the
// third. The runner compiles modules of up to eighteen functions, mixing nests the
// kernel-coset lowers, refuses and never reaches with nests other routes own, and
// checks that each lowers exactly as it does alone, exactly once and in source
// order, that everything else matches the flag-off output, and that the emitted
// MACs are exact against a brute enumeration of the source.
