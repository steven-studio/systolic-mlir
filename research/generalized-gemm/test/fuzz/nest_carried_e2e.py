#!/usr/bin/env python3
"""GEMMs whose subscripts are carried by their loops (docs/GENERALIZED_GEMM_STATEMENT_BV.md,
Section 14.9; stage 3 of docs/GENERALIZED_GEMM_SCOPE.md), end to end: the research recognizer
verifies the nests with index invariants, the pass lowers them from their records, the
programs run, and every result is compared with the original.

A subscript here is no arithmetic of a counter: an scf.for (iter_args) or a counting scf.while
carries it, an index started at a constant on every entry of the loop and stepped by a constant
each iteration, whose last value nobody reads. Without index invariants the research proposes no
closed form of such a view. With them (generalized-gemm-nest-report --index-invariants) the
relation subscript == start + step * counter is found, proved by induction and handed to the
view proposer, and the record is VERIFIED with affine closed forms. The lowering does not take
that on the record's word: it reads the recurrence from the IR (Systolic/NestRecordLowering.h),
and the record's form must be the form it read.

The machinery is nest_e2e.py's (the programs printed from specs, the C driver and its
reference in plain C, the numeric contract, the call order, the invocation counts). The C
reference computes each subscript from the view (scale * counter + constant), never from a
carried value, so a lowered result is compared with what the view says, and the variant with
no records, which runs the original program with its carried subscripts, with the same. For
each variant of records and options:

  driven        records made with --index-invariants, every pair of roots authorized;
                record-driven: every nest but the strict one is lowered, by formulas,
                the three of flat arrays by tables
  searchless    records made without --index-invariants; record-driven. No nest is lowered by
                formulas. A carrying nest whose record is VERIFIED but for the closed forms
                even so (the one whose carried subscript is its counter's twin: same start,
                same step) is lowered by tables, from the nest's own addresses with the
                recurrence read from the IR. The others stay on the host
  records       the driven records without record-driven: stage 2 alone, nothing lowered
  off           no records: the original program
  stale         the driven records, record-driven, and a program whose first nest's carried
                subscript starts one further on: that nest stays, the others are lowered
  unauthorized  records made with no pair authorized: nothing verified; record-driven
"""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import shutil
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
import nest_e2e as e2e  # noqa: E402

BOX = {"i": (0, 16), "j": (0, 8), "k": (0, 12)}
SHIFTED = {"i": (2, 18), "j": (1, 9), "k": (3, 15)}
# A read at [i, k + 2], B at [k + 2, j]: the K subscripts start at 2.
OFFSET = {"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 2)], "b": [("k", 1, 2), ("j", 1, 0)]}
OFFSET_ABC = [("a", 16, 15, False), ("b", 15, 8, False), ("c", 16, 8, True)]
K_CARRIED = (("a", 1), ("b", 0))


def memref(loops, **more):
    return dict(access="memref", loops=loops, **more)


FUNCTIONS = [
    # The K subscripts of A and B carried by the k loop: an scf.for, an scf.while, and the
    # control, the same views by arith of the counter.
    {"name": "car_iter", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, **memref("scf"))]},
    {"name": "car_while", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, **memref("while"))]},
    # (Of the control the research takes the subscript k + 2 itself for the K level, from 2 to
    # 14; the lowering takes that level for the counter plus 2 and checks it through the views.)
    {"name": "car_plain", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, **memref("scf"))]},
    # Mixed loops: affine.for around scf.while around scf.for, the innermost carrying.
    {"name": "car_mixed", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED,
                                **memref(("affine", "while", "scf")))]},
    # The outermost loop carries the row of C and of A (from 1): its step is between two loops.
    {"name": "car_outer", "element": "f32",
     "arrays": [("a", 17, 12, False), ("b", 12, 8, False), ("c", 17, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, carry=(("c", 0), ("a", 0)),
                                views={"c": [("i", 1, 1), ("j", 1, 0)], "a": [("i", 1, 1), ("k", 1, 0)],
                                       "b": [("k", 1, 0), ("j", 1, 0)]}, **memref(("while", "scf", "scf")))]},
    # A step of 2: B's rows 1, 3, 5, ... carried by an scf.while.
    {"name": "car_step2", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 24, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, carry=(("b", 0),),
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                                       "b": [("k", 2, 1), ("j", 1, 0)]}, **memref("while"))]},
    # Bounds that do not start at 0: the carried subscripts start at the view of the lower bound.
    {"name": "car_shifted", "element": "f32",
     "arrays": [("a", 18, 17, False), ("b", 17, 9, False), ("c", 18, 9, True)],
     "statements": [e2e.product("c", "a", "b", SHIFTED, views=OFFSET, carry=K_CARRIED,
                                **memref(("scf", "while", "scf")))]},
    # The counter is the second value the scf.while carries, the subscript the first: the
    # counter is the one the comparison reads.
    {"name": "car_second", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, counter_place="second",
                                **memref("while"))]},
    # The carried subscript is stepped before it is read: it starts at 1, and the body reads
    # the carried value plus 1. An scf.for, and an scf.while.
    {"name": "car_step_first", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, carry_step="before",
                                **memref("scf"))]},
    {"name": "car_step_first_while", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, carry_step="before",
                                **memref("while"))]},
    # Tables: flat arrays, the index constants before the nest (the research proposes no closed
    # form for such views, with index invariants or without). The flat index is arith of a
    # carried subscript, which the tables take at each iteration from the recurrence in the IR.
    {"name": "car_tables_iter", "element": "f32", "flat": ("a", "b", "c"), "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, constants="outside",
                                **memref("scf"))]},
    # The same with a carried subscript that is the counter's twin: started at 0, stepped by 1.
    {"name": "car_tables_twin", "element": "f32", "flat": ("a", "b", "c"),
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, carry=K_CARRIED, constants="outside", **memref("scf"))]},
    {"name": "car_tables_while", "element": "f32", "flat": ("a", "b", "c"),
     "arrays": [("a", 16, 12, False), ("b", 24, 8, False), ("c", 16, 8, True)],
     "statements": [e2e.product("c", "a", "b", BOX, carry=(("b", 0),), constants="outside",
                                views={"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                                       "b": [("k", 2, 1), ("j", 1, 0)]}, **memref("while"))]},
    # No reassoc on the accumulation: the floating-point policy keeps it on the host.
    {"name": "car_strict", "element": "f32", "arrays": OFFSET_ABC,
     "statements": [e2e.product("c", "a", "b", BOX, views=OFFSET, carry=K_CARRIED, flag=False, **memref("scf"))]},
]
# Not lowered, and run on the host: by the floating-point policy.
KEPT = {("car_strict", 0)}
CARRYING = {(f["name"], 0) for f in FUNCTIONS if f["statements"][0]["carry"]}
# Verified but for the closed forms, with index invariants or without, and lowered by tables.
TABLED = {("car_tables_iter", 0), ("car_tables_while", 0), ("car_tables_twin", 0)}


def functions(stale=False):
    """The specs. With `stale`, the first function's carried subscripts start at 3: the program
    changed after its records were made, in the constant a recurrence starts at and nothing else."""
    out = copy.deepcopy(FUNCTIONS)
    if stale:
        statement = out[0]["statements"][0]
        statement["views"] = dict(statement["views"], a=[("i", 1, 0), ("k", 1, 3)], b=[("k", 1, 3), ("j", 1, 0)])
    return out


def statuses(path):
    """Each nest's status in a file of records, by (function, nest)."""
    out = {}
    for function in json.loads(Path(path).read_text()):
        for nest in function.get("nests", []):
            out[(function["function"], nest["nest"])] = nest.get("status")
    return out


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
    work = Path(args.keep) if args.keep else Path(tempfile.mkdtemp(prefix="nest-carried-e2e-"))
    if args.keep:
        work.mkdir(parents=True, exist_ok=False)
    failures = []
    try:
        specs = functions()
        (work / "module.mlir").write_text(e2e.mlir_module(specs))
        roots = max(len(f["arrays"]) for f in specs)
        search = ["--index-invariants"]
        e2e.records(args, work / "module.mlir", work / "records.json", authorize=True, roots=roots, extra=search)
        e2e.records(args, work / "module.mlir", work / "searchless.json", authorize=True, roots=roots)
        e2e.records(args, work / "module.mlir", work / "unauthorized.json", authorize=False, roots=roots,
                    extra=search)
        everything = {(f["name"], 0) for f in specs}
        driven = everything - KEPT
        # What the records say, before anything is lowered from them.
        with_search, without = statuses(work / "records.json"), statuses(work / "searchless.json")
        for key in sorted(everything):
            want = "VERIFIED_NOT_MAPPED" if key in TABLED else "VERIFIED"
            if with_search.get(key) != want:
                failures.append(f"records: {key} is {with_search.get(key)} with index invariants, expected {want}")
            if key in CARRYING and without.get(key) == "VERIFIED":
                failures.append(f"records: {key} is VERIFIED without index invariants: the search is not what "
                                f"gives its views")
        counts["records VERIFIED with index invariants"] = sum(1 for k in everything if with_search.get(k) == "VERIFIED")
        counts["records VERIFIED_NOT_MAPPED with index invariants"] = \
            sum(1 for k in everything if with_search.get(k) == "VERIFIED_NOT_MAPPED")
        counts["records of carrying nests not VERIFIED without them"] = \
            sum(1 for k in CARRYING if without.get(k) != "VERIFIED")
        # Without the search: a carrying nest is lowered only by tables, and only where its
        # record is VERIFIED but for the closed forms.
        tabled = {k for k in CARRYING - KEPT if without.get(k) == "VERIFIED_NOT_MAPPED"}
        searchless = tabled | {k for k in driven - CARRYING if without.get(k) in ("VERIFIED", "VERIFIED_NOT_MAPPED")}
        plan = [
            # name, specs, records, option, lowered, lowered by tables
            ("driven", specs, work / "records.json", " record-driven=true", driven, set(TABLED)),
            ("searchless", specs, work / "searchless.json", " record-driven=true", searchless,
             {k for k in searchless if without.get(k) == "VERIFIED_NOT_MAPPED"}),
            ("records", specs, work / "records.json", "", set(), set()),
            ("off", specs, None, "", set(), set()),
            ("stale", functions(stale=True), work / "records.json", " record-driven=true",
             driven - {("car_iter", 0)}, set(TABLED)),
            ("unauthorized", specs, work / "unauthorized.json", " record-driven=true", set(), set()),
        ]
        for name, source, records_path, option, expected, expected_tabled in plan:
            try:
                e2e.variant(args, work, name, source, records_path, expected, counts, extra_option=option,
                            expected_tabled=expected_tabled)
            except e2e.Failure as failure:
                failures.append(str(failure))
        print(f"{len(tabled):6d}  carrying nests lowered by tables without index invariants: "
              f"{', '.join(sorted(k[0] for k in tabled)) or 'none'}")
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    for key in sorted(counts):
        print(f"{counts[key]:6d}  {key}")
    print(f"{len(FUNCTIONS):6d}  functions, {len(CARRYING)} with carried subscripts")
    for failure in failures:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
