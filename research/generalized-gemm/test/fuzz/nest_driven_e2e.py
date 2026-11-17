#!/usr/bin/env python3
"""Stage 3 (docs/GENERALIZED_GEMM_SCOPE.md), end to end: GEMMs in loop nests the production
recognizer finds no GEMM in, lowered from their records, run, and compared with the original.

The production recognizer reads affine.load and affine.store; the nests here read and write
with memref.load and memref.store, their indices computed by arith, so it finds no GEMM in them
and leaves them as written. The research recognizer verifies them, and with
--affine-matmul-to-systolic="nest-records=FILE record-driven=true" the pass lowers them from
their records (Systolic/NestRecordLowering.h): by the record's closed forms of the views
where it has them (formulas), by tables of the nest's own addresses over its static domain
where the research proposed no closed form (tables: gather, tiles, scatter).

The machinery is nest_e2e.py's (the programs printed from specs, the C driver and its
reference in plain C, the numeric contract, the call order, the invocation counts). For each
variant of records and options:

  driven        the research tool's records, every pair of roots authorized; record-driven
  records       the same records without record-driven: stage 2 alone
  off           no records: the pass as it is without the option
  stale         the records, record-driven, and a program whose first nest changed since
  unauthorized  records made with no pair authorized: nothing verified; record-driven

the module is lowered (synchronously and asynchronously, to LLVM, to an object), linked with a
C driver and the software runtime model, run, and every function's arrays compared with the
reference: the nests lowered from their records ran on the runtime (the remarks' regions are
the invocations counted), the others on the host; results by each nest's numeric contract.
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
FUNCTIONS = [
    # Formulas: the counters themselves as indices; the research proves the views.
    {"name": "drv_formulas", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref")]},
    # Formulas, with B stored transposed and the loops in another order.
    {"name": "drv_transposed", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 8, 12, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, order="kij",
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                                       "b": [("j", 1, 0), ("k", 1, 0)]}, access="memref")]},
    # Tables: flat arrays, each index row * columns + column by arith.
    {"name": "drv_flat", "element": "f32", "flat": ("a", "b", "c"),
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref")]},
    # Tables: two-dimensional arrays at offsets and a scale computed by arith.
    {"name": "drv_offsets", "element": "f32",
     "arrays": [("a", 18, 13, False), ("b", 25, 8, False), ("c", 17, 10, True)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref",
                                views={"c": [("i", 1, 1), ("j", 1, 2)], "a": [("i", 1, 2), ("k", 1, 1)],
                                       "b": [("k", 2, 1), ("j", 1, 0)]})]},
    # Tables: flat arrays, the index constants defined before the nest (the research proposes no
    # closed form for such views; the fold, coordinates and box are verified).
    {"name": "drv_tables_flat", "element": "f32", "flat": ("a", "b", "c"),
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref", constants="outside")]},
    # Tables: A's rows read in another order, (5 i) mod 16, which no affine formula is.
    {"name": "drv_tables_perm", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref",
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("perm", "i", 5, 16), ("k", 1, 0)],
                                       "b": [("k", 1, 0), ("j", 1, 0)]})]},
    # Two nests on one output, each lowered from its own record, in order.
    {"name": "drv_shared", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [e2e.product("c", "a", "b", BOX, access="memref"),
                    e2e.product("c", "a", "d", BOX, access="memref")]},
    # An affine nest (stage 2: the recognizer and its record), then a nest only stage 3 lowers,
    # on one output.
    {"name": "drv_mixed", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [e2e.product("c", "a", "b", BOX),
                    e2e.product("c", "a", "d", BOX, access="memref")]},
    # No reassoc on the accumulation: the floating-point policy keeps it on the host.
    {"name": "drv_strict", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, flag=False, access="memref")]},
]


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
    work = Path(args.keep) if args.keep else Path(tempfile.mkdtemp(prefix="nest-driven-e2e-"))
    if args.keep:
        work.mkdir(parents=True, exist_ok=False)
    failures = []
    try:
        specs = functions()
        (work / "module.mlir").write_text(e2e.mlir_module(specs))
        e2e.records(args, work / "module.mlir", work / "records.json", authorize=True)
        e2e.records(args, work / "module.mlir", work / "unauthorized.json", authorize=False)
        strict = {("drv_strict", 0)}
        affine = products(specs, {"affine"})
        memref = products(specs, {"memref"})
        tabled = {("drv_tables_flat", 0), ("drv_tables_perm", 0)}
        driven = (affine | memref) - strict
        plan = [
            # name, specs, records, option, lowered, lowered by tables
            ("driven", specs, work / "records.json", " record-driven=true", driven, tabled),
            ("records", specs, work / "records.json", "", affine, set()),
            ("off", specs, None, "", affine, set()),
            ("stale", functions(stale=True), work / "records.json", " record-driven=true",
             driven - {("drv_formulas", 0)}, tabled),
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
