#!/usr/bin/env python3
"""Prints a file of nest records (nest_records.json) with one named field edited, for
nest_records_refusals.mlir. Each edit makes a record that must not let a lowering go
ahead; the edits are made here, on the research tool's own output, because the tool
never writes such a record. Usage: edit_nest_records.py FILE EDIT.

transplant-NAME, for nest_records_belts.mlir: FILE is nest_records_belts.json, and the
output is function NAME's object from it with one record, of its nest 0: @single's record
from nest_records.json beside it (complete and VERIFIED), with nest 0's own binding. The
function's roots are its own. Every check on the record then passes, and what refuses is a
check on the nest itself."""
import json
from pathlib import Path
import sys


def single(functions):
    return next(f for f in functions if f["function"] == "single")


def nest(functions):
    return single(functions)["nests"][0]


def swap_axes(functions):
    axes = nest(functions)["gemm"]["axes"]
    axes["I"], axes["J"] = axes["J"], axes["I"]


def edit(functions, name):
    record = nest(functions)
    gemm = record["gemm"]
    if name == "extent":
        gemm["extents"]["M"] = 15
    elif name == "level":
        gemm["levels"][0]["upper"] = 15
    elif name == "axes":
        swap_axes(functions)
    elif name == "view":
        gemm["views"]["A"]["dimensions"][0]["constant"] = 1
    elif name == "root":
        gemm["views"]["A"]["root"]["root"] = 1
    elif name == "flags":
        gemm["reduction_flags"] = []
    elif name == "flag-number":
        gemm["reduction_flags"] = gemm["reduction_flags"] + [5]
    elif name == "source-flag":
        record["reassociation"]["source_flag_reassoc"] = False
    elif name == "accumulator":
        gemm["accumulation"]["accumulator_first"] = False
    elif name == "element":
        gemm["accumulation"]["element"] = "f64"
    elif name == "incomplete":
        del gemm["views"]
    elif name == "authorizes":
        record["authorizes_lowering"] = True
    elif name == "status":
        record["status"] = "VERIFIED_NOT_MAPPED"
        record["refusals"] = ["the view of A has no proposed closed form"]
    elif name == "scope":
        record["scope"] = [{"argument": 0, "lower": 1, "upper": 2}]
    elif name == "separation":
        for pair in record["separation"]:
            if pair["roots"] == [0, 1]:
                pair["used"] = True
                pair["apart"] = True
                pair["kind"] = "assumption"
    elif name == "proof":
        for pair in record["separation"]:
            if pair["roots"] == [0, 2]:
                pair["kind"] = "proof"
    elif name == "theorem13":
        gemm["tiers"]["theorem13"]["tier"] = "SCOPE_ENUMERATED"
    elif name == "frame":
        gemm["frame"]["other_writes"] = 1
    elif name == "roots":
        single(functions)["roots"][2]["noalias"] = False
    elif name == "scheme":
        record["binding"]["scheme"] = "another-binding/1"
    elif name == "fold":
        record["ordered_fold_verified"] = False
    elif name == "refusals":
        record["refusals"] = ["a refusal"]
    elif name == "box":
        gemm["box"] = False
    elif name == "extra-level":
        gemm["levels"].append(json.loads(json.dumps(gemm["levels"][2])))
    elif name == "unproved":
        gemm["views"]["A"]["proved"] = False
    elif name == "extra-dimension":
        dimensions = gemm["views"]["A"]["dimensions"]
        dimensions.append(json.loads(json.dumps(dimensions[1])))
    elif name == "reduction":
        gemm["accumulation"]["reduction"] = "add"
    elif name == "kind":
        for pair in record["separation"]:
            if pair["roots"] == [0, 2]:
                pair["kind"] = "witness"
    elif name == "extra-root":
        roots = single(functions)["roots"]
        roots.append(json.loads(json.dumps(roots[2])))
        roots[-1]["root"] = 3
    elif name == "nonumber":
        del record["nest"]
    elif name == "twice":
        single(functions)["nests"].append(json.loads(json.dumps(record)))
    elif name == "missing-function":
        functions.remove(single(functions))
    elif name == "missing-nest":
        shared = next(f for f in functions if f["function"] == "shared")
        shared["nests"] = [n for n in shared["nests"] if n["nest"] != 1]
    elif name == "schema":
        for function in functions:
            function["schema"] = "generalized-gemm-nest/1"
    elif name == "duplicate":
        functions.append(json.loads(json.dumps(single(functions))))
    else:
        sys.exit(f"no edit named {name}")


def transplant(path, target):
    with open(path) as source:
        functions = json.load(source)
    with open(Path(path).parent / "nest_records.json") as source:
        template = single(json.load(source))["nests"][0]
    function = next(f for f in functions if f["function"] == target)
    own = next(n for n in function["nests"] if n["nest"] == 0)
    record = json.loads(json.dumps(template))
    record["binding"] = own["binding"]
    function["nests"] = [record]
    return [function]


def main():
    path, name = sys.argv[1], sys.argv[2]
    if name.startswith("transplant-"):
        print(json.dumps(transplant(path, name[len("transplant-"):])))
        return
    with open(path) as source:
        functions = json.load(source)
    edit(functions, name)
    print(json.dumps(functions))


if __name__ == "__main__":
    main()
