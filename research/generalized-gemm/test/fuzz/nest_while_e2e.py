#!/usr/bin/env python3
"""GEMMs written with scf.while (docs/GENERALIZED_GEMM_SCOPE.md, stage 3 on scf.while), end to
end: the research recognizer verifies the nests, the pass lowers them from their records, the
programs run, and every result is compared with the original.

The nests are counting loops written with scf.while: each loop carries one value, its counter,
from a constant; the before region compares it with a constant (slt) and forwards it with
scf.condition; the after region receives it as its argument, holds the body, and yields
counter + 1. The loads and stores are memref.load and memref.store. Some nests mix scf.while
with scf.for and affine.for loops. The production recognizer reads affine loops only, so it
finds no GEMM in them; with --affine-matmul-to-systolic="nest-records=FILE record-driven=true"
the pass lowers them from their records (Systolic/NestRecordLowering.h): by the record's closed
forms of the views where it has them (formulas), by tables of the nest's own addresses over its
static domain where it has none (tables: gather, tiles, scatter). The first three functions
are one GEMM written with scf.while, with scf.for and with affine.for, each with its own
record.

The machinery is nest_e2e.py's (the programs printed from specs, the C driver and its
reference in plain C, the numeric contract, the call order, the invocation counts). For each
variant of records and options:

  driven        the research tool's records, every pair of roots authorized; record-driven
  records       the same records without record-driven: stage 2 alone
  off           no records: the pass as it is without the option, the original program
  stale         the records, record-driven, and a program whose first nest changed since
  unauthorized  records made with no pair authorized: nothing verified; record-driven

the module is lowered (synchronously and asynchronously, to LLVM, to an object), linked with a
C driver and the software runtime model, run, and every function's arrays compared with the
reference: the nests lowered from their records ran on the runtime (the remarks' regions are
the invocations counted), the others on the host; results by each nest's numeric contract.
The variant with no records runs the original scf.while program, so each lowered result is
compared with the reference the original program meets too.
"""
import argparse
from collections import Counter
import copy
from pathlib import Path
import shutil
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import nest_e2e as e2e  # noqa: E402

BOX = {"i": (0, 16), "j": (0, 8), "k": (0, 12)}
SHIFTED = {"i": (2, 18), "j": (1, 9), "k": (3, 15)}
WHILE = {"access": "memref", "loops": "while"}
ABC = [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)]
SHIFTED_ABC = [("a", 18, 15, False), ("b", 15, 9, False), ("c", 18, 9, True)]


def mixed(*kinds):
    return {"access": "memref", "loops": kinds}


FUNCTIONS = [
    # Three scf.while loops, the counters as indices, the bounds before the nest: formulas.
    {"name": "whl_gemm", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **WHILE)]},
    # The same GEMM with scf.for, and with affine.for (memref.load): the forms already taken.
    {"name": "whl_twin_scf", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, access="memref", loops="scf")]},
    {"name": "whl_twin_affine", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, access="memref")]},
    # Each loop's bounds in the body of the loop around it (the after region of a while).
    {"name": "whl_inside", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, bounds_at="inside", **WHILE)]},
    # Mixed nests: for around while around for; while around for around while; affine.for
    # around while around scf.for.
    {"name": "whl_for_while_for", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **mixed("scf", "while", "scf"))]},
    {"name": "whl_while_for_while", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **mixed("while", "scf", "while"))]},
    {"name": "whl_affine_while_for", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **mixed("affine", "while", "scf"))]},
    # The other ways a counting loop is written: bound > counter and 1 + counter; an unsigned
    # comparison, the bound a constant of the before region; bound > counter unsigned.
    {"name": "whl_sgt_swapped", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, compare="sgt", step="1+counter", **WHILE)]},
    {"name": "whl_ult_before", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, compare="ult", bound_in_before=True, **WHILE)]},
    {"name": "whl_ugt", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, compare="ugt", **WHILE)]},
    # Loops that go on while the counter differs from its bound: bound != counter from 0 to the
    # arrays' own sizes; counter != bound from bounds that do not start at 0; counter != bound
    # in arrays larger than the loops' bounds. In the first the accesses bound the counters; in
    # the two others it is the termination certificate that holds counter <= bound, a fact its
    # proposer makes for a != comparison.
    {"name": "whl_en", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, compare="en", **WHILE)]},
    {"name": "whl_ne_shifted", "element": "f32", "arrays": SHIFTED_ABC,
     "statements": [e2e.product("c", "a", "b", SHIFTED, compare="ne", **WHILE)]},
    {"name": "whl_ne_bigger", "element": "f32", "arrays": SHIFTED_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, compare="ne", **WHILE)]},
    # Each counter stepped first: the addition is the first operation of the after region, and
    # counter + 1 is held across the loop inside. Pure scf.while from 0, and from other starts
    # with an scf.for between two scf.while loops.
    {"name": "whl_step_first", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, step_at="first", **WHILE)]},
    {"name": "whl_step_first_shifted", "element": "f32", "arrays": SHIFTED_ABC,
     "statements": [e2e.product("c", "a", "b", SHIFTED, step_at="first", **mixed("while", "scf", "while"))]},
    # Bounds that do not start at 0: I from 2, J from 1, K from 3; pure while, and mixed.
    {"name": "whl_shifted", "element": "f32", "arrays": SHIFTED_ABC,
     "statements": [e2e.product("c", "a", "b", SHIFTED, **WHILE)]},
    {"name": "whl_shifted_mixed", "element": "f32", "arrays": SHIFTED_ABC,
     "statements": [e2e.product("c", "a", "b", SHIFTED, **mixed("while", "scf", "while"))]},
    # B stored transposed, the loops in the order k, i, j.
    {"name": "whl_transposed", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 8, 12, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, order="kij",
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                                       "b": [("j", 1, 0), ("k", 1, 0)]}, **WHILE)]},
    # Flat arrays, each index row * columns + column by arith in the body: formulas, by an
    # explicit view (the research reads the body's counters as its levels).
    {"name": "whl_flat", "element": "f32", "flat": ("a", "b", "c"), "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **WHILE)]},
    # Offsets and a scale computed by arith.
    {"name": "whl_offsets", "element": "f32",
     "arrays": [("a", 18, 13, False), ("b", 25, 8, False), ("c", 17, 10, True)],
     "statements": [e2e.product("c", "a", "b", BOX,
                                views={"c": [("i", 1, 1), ("j", 1, 2)], "a": [("i", 1, 2), ("k", 1, 1)],
                                       "b": [("k", 2, 1), ("j", 1, 0)]}, **WHILE)]},
    # Tables: flat arrays, the index constants before the nest (the research proposes no
    # closed form for such views; the fold, coordinates and box are verified).
    {"name": "whl_tables_flat", "element": "f32", "flat": ("a", "b", "c"), "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, constants="outside", **WHILE)]},
    # Tables: A's rows read in the order (5 i) mod 16, which no affine formula is.
    {"name": "whl_tables_perm", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX,
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("perm", "i", 5, 16), ("k", 1, 0)],
                                       "b": [("k", 1, 0), ("j", 1, 0)]}, **WHILE)]},
    # Two scf.while nests on one output, each lowered from its own record, in order.
    {"name": "whl_shared", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [e2e.product("c", "a", "b", BOX, **WHILE), e2e.product("c", "a", "d", BOX, **WHILE)]},
    # The second nest reads what the first wrote: T = T + A B, then C = C + T D.
    {"name": "whl_chain", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("t", 16, 8, True), ("d", 8, 8, False),
                ("c", 16, 8, True)],
     "statements": [e2e.product("t", "a", "b", BOX, **WHILE),
                    e2e.product("c", "t", "d", {"i": (0, 16), "j": (0, 8), "k": (0, 8)},
                                **mixed("while", "while", "scf"))]},
    # No reassoc on the accumulation: the floating-point policy keeps it on the host.
    {"name": "whl_strict", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, flag=False, **WHILE)]},
]
TABLED = {("whl_tables_flat", 0), ("whl_tables_perm", 0)}
# Not lowered with verified records either: the floating-point policy keeps a strict
# accumulation on the host.
KEPT = {("whl_strict", 0)}


def functions(stale=False):
    out = copy.deepcopy(FUNCTIONS)
    if stale:
        statement = out[0]["statements"][0]
        low, high = statement["bounds"]["i"]
        statement["bounds"] = dict(statement["bounds"], i=(low, high - 1))
    return out


def products(specs, accesses):
    return {(f["name"], n) for f in specs for n, s in enumerate(f["statements"])
            if s["kind"] == "product" and s.get("access", "affine") in accesses}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nest-driver", required=True)
    parser.add_argument("--systolic-opt", required=True)
    parser.add_argument("--mlir-translate", required=True)
    parser.add_argument("--llc", required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--runtime", required=True, help="runtime/offload of the source tree")
    parser.add_argument("--llvm-lib", required=True, help="the LLVM build's lib, with mlir_c_runner_utils")
    parser.add_argument("--keep", default="", help="keep the work here")
    args = parser.parse_args()
    counts = Counter()
    work = Path(args.keep) if args.keep else Path(tempfile.mkdtemp(prefix="nest-while-e2e-"))
    if args.keep:
        work.mkdir(parents=True, exist_ok=False)
    failures = []
    try:
        specs = functions()
        (work / "module.mlir").write_text(e2e.mlir_module(specs))
        roots = max(len(f["arrays"]) for f in specs)
        e2e.records(args, work / "module.mlir", work / "records.json", authorize=True, roots=roots)
        e2e.records(args, work / "module.mlir", work / "unauthorized.json", authorize=False, roots=roots)
        memref = products(specs, {"memref"})
        driven = memref - KEPT
        plan = [
            # name, specs, records, option, lowered, lowered by tables
            ("driven", specs, work / "records.json", " record-driven=true", driven, TABLED),
            ("records", specs, work / "records.json", "", set(), set()),
            ("off", specs, None, "", set(), set()),
            ("stale", functions(stale=True), work / "records.json", " record-driven=true",
             driven - {("whl_gemm", 0)}, TABLED),
            ("unauthorized", specs, work / "unauthorized.json", " record-driven=true", set(), set()),
        ]
        for name, source, records_path, option, expected, expected_tabled in plan:
            try:
                e2e.variant(args, work, name, source, records_path, expected, counts, extra_option=option,
                            expected_tabled=expected_tabled)
            except e2e.Failure as failure:
                failures.append(str(failure))
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    for key in sorted(counts):
        print(f"{counts[key]:6d}  {key}")
    for failure in failures:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
