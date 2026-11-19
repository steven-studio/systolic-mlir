#!/usr/bin/env python3
"""GEMMs written with scf.for (docs/GENERALIZED_GEMM_SCOPE.md, stage 3 on scf.for), end to end:
the research recognizer verifies the nests, the pass lowers them from their records, the
programs run, and every result is compared with the original.

The nests are written as a programmer writes them with scf.for: memref.load and memref.store,
the loops' bounds and steps arith.constant values, defined before the nest or in the body of
the loop around. The production recognizer reads affine loops only, so it finds no GEMM in
them; with --affine-matmul-to-systolic="nest-records=FILE record-driven=true" the pass
lowers them from their records (Systolic/NestRecordLowering.h): by the record's closed forms
of the views where it has them (formulas), by tables of the nest's own addresses over its
static domain where it has none (tables: gather, tiles, scatter). The first three functions
are one GEMM written three ways, affine.for with affine.load, affine.for with memref.load,
and scf.for, each with its own record.

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
The variant with no records runs the original scf.for program, so each lowered result is
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
SCF = {"access": "memref", "loops": "scf"}
ABC = [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)]
FUNCTIONS = [
    # scf.for, the counters as indices, the bounds before the nest: formulas.
    {"name": "scf_gemm", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **SCF)]},
    # The same GEMM with affine.for and affine.load (the production recognizer and stage 2),
    # and with affine.for and memref.load (stage 3).
    {"name": "scf_twin_affine", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX)]},
    {"name": "scf_twin_memref", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, access="memref")]},
    # Each loop's bounds in the body of the loop around it.
    {"name": "scf_inside", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, bounds_at="inside", **SCF)]},
    # B stored transposed, the loops in the order k, i, j.
    {"name": "scf_transposed", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 8, 12, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, order="kij",
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                                       "b": [("j", 1, 0), ("k", 1, 0)]}, **SCF)]},
    # Flat arrays, each index row * columns + column by arith in the body.
    {"name": "scf_flat", "element": "f32", "flat": ("a", "b", "c"), "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, **SCF)]},
    # Offsets and a scale computed by arith.
    {"name": "scf_offsets", "element": "f32",
     "arrays": [("a", 18, 13, False), ("b", 25, 8, False), ("c", 17, 10, True)],
     "statements": [e2e.product("c", "a", "b", BOX,
                                views={"c": [("i", 1, 1), ("j", 1, 2)], "a": [("i", 1, 2), ("k", 1, 1)],
                                       "b": [("k", 2, 1), ("j", 1, 0)]}, **SCF)]},
    # Bounds that do not start at 0: I from 2, J from 1, K from 3.
    {"name": "scf_shifted", "element": "f32",
     "arrays": [("a", 18, 15, False), ("b", 15, 9, False), ("c", 18, 9, True)],
     "statements": [e2e.product("c", "a", "b", {"i": (2, 18), "j": (1, 9), "k": (3, 15)}, **SCF)]},
    # Tables: flat arrays, the index constants before the nest (the research proposes no
    # closed form for such views; the fold, coordinates and box are verified).
    {"name": "scf_tables_flat", "element": "f32", "flat": ("a", "b", "c"), "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, constants="outside", **SCF)]},
    # Tables: A's rows read in the order (5 i) mod 16, which no affine formula is.
    {"name": "scf_tables_perm", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX,
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("perm", "i", 5, 16), ("k", 1, 0)],
                                       "b": [("k", 1, 0), ("j", 1, 0)]}, **SCF)]},
    # Two scf.for nests on one output, each lowered from its own record, in order.
    {"name": "scf_shared", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [e2e.product("c", "a", "b", BOX, **SCF), e2e.product("c", "a", "d", BOX, **SCF)]},
    # The second nest reads what the first wrote: T = T + A B, then C = C + T D.
    {"name": "scf_chain", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("t", 16, 8, True), ("d", 8, 8, False),
                ("c", 16, 8, True)],
     "statements": [e2e.product("t", "a", "b", BOX, **SCF),
                    e2e.product("c", "t", "d", {"i": (0, 16), "j": (0, 8), "k": (0, 8)}, **SCF)]},
    # No reassoc on the accumulation: the floating-point policy keeps it on the host.
    {"name": "scf_strict", "element": "f32", "arrays": ABC,
     "statements": [e2e.product("c", "a", "b", BOX, flag=False, **SCF)]},
]
TABLED = {("scf_tables_flat", 0), ("scf_tables_perm", 0)}


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
    work = Path(args.keep) if args.keep else Path(tempfile.mkdtemp(prefix="nest-scf-e2e-"))
    if args.keep:
        work.mkdir(parents=True, exist_ok=False)
    failures = []
    try:
        specs = functions()
        (work / "module.mlir").write_text(e2e.mlir_module(specs))
        roots = max(len(f["arrays"]) for f in specs)
        e2e.records(args, work / "module.mlir", work / "records.json", authorize=True, roots=roots)
        e2e.records(args, work / "module.mlir", work / "unauthorized.json", authorize=False, roots=roots)
        strict = {("scf_strict", 0)}
        affine = products(specs, {"affine"})
        memref = products(specs, {"memref"})
        driven = (affine | memref) - strict
        plan = [
            # name, specs, records, option, lowered, lowered by tables
            ("driven", specs, work / "records.json", " record-driven=true", driven, TABLED),
            ("records", specs, work / "records.json", "", affine, set()),
            ("off", specs, None, "", affine, set()),
            ("stale", functions(stale=True), work / "records.json", " record-driven=true",
             driven - {("scf_gemm", 0)}, TABLED),
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
