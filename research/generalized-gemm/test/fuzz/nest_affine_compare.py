#!/usr/bin/env python3
"""Stage 1 (docs/GENERALIZED_GEMM_SCOPE.md): a nest's record against what the production affine
recognizer reports for the same nest, field by field.

For each named function of one loop nest, written with affine loops so that both tools read
the same text, the production tool is run on the function alone, and only run:

  --test-affine-matmul-discovery           the roles of the loops, the origin, the views, the
                                           statement, the extents, the bijection
  --affine-matmul-to-systolic=report=true  the alias gate; then with assume-no-alias, the
                                           M x N x K it lowers, or its refusal to reassociate

and the nest driver twice, with no separation authorized and with every pair authorized. The
fields are then compared:

  M, N, K           `dense extents` and the report's `affine GEMM MxNxK`   gemm.extents
  roles             `I/J/K on loops a/b/c`                                 gemm.axes
  origin            `source origin` (the loops' lower bounds)              gemm.levels[*].lower
  views             `view X: [s*R + c, ...]` over the loop variables,      gemm.views.X.dimensions:
                    role by role, placed by `layout X: roles in            the coefficient of the
                    dimensions (p, q)`                                     role's level, the constant
  statement         `C = C + A * B (accumulation)`                         gemm.accumulation
  reassoc flag      `reduction: reassoc permitted`                         reassociation.source_flag_reassoc
  bijection         `logical bijection: verified`                          the order exact, one event a point
  alias gate        `A may overlap C` with no option                       no name authorized: not verified,
                                                                           and the refusal says the same
  reassociation     lowered, or `reassociation is not permitted`           reassociation.permitted

A difference in any field fails the run. Two differences of policy are stated in the output
and are no failure: the production tool takes `llvm.noalias` as proof that the operands are
apart, where a record needs the name authorized and says it is an assumption; and the
production tool reads binary32 products only, so an integer nest is compared with the record
of its binary32 twin (--twin integer=float), which is compared with the production tool.

Nothing of the production tool is changed, and nothing here judges a program.
"""
import argparse
import json
import re
import subprocess
import sys
import tempfile

TIMEOUT = 600
SOLVER = ["--timeout-ms=20000", "--rlimit=2000000", "--max-level=1", "--candidates-per-group=1"]


class Failure(Exception):
    pass


def run(command):
    done = subprocess.run(command, capture_output=True, text=True, timeout=TIMEOUT)
    return done.returncode, done.stdout, done.stderr


def functions_of(text):
    return {match.group(1): match.group(0)
            for match in re.finditer(r"^func\.func @(\w+)\(.*?^}\n", text, re.MULTILINE | re.DOTALL)}


def production(opt, source):
    """What the production tool prints for a file of one function with one nest."""
    out = {}
    code, _, text = run([opt, source, "--test-affine-matmul-discovery", "-o", "/dev/null"])
    if code:
        raise Failure(f"the discovery pass exits {code}")
    out["recognized"] = "recognition result: success" in text
    reason = re.search(r"^\s*reason: (.*)$", text, re.MULTILINE)
    out["reason"] = reason.group(1) if reason else ""
    if out["recognized"]:
        dense = re.search(r"dense logical GEMM: yes, I/J/K on loops (\d+)/(\d+)/(\d+)", text)
        extents = re.search(r"dense extents: M (\d+), N (\d+), K (\d+)", text)
        origin = re.search(r"source origin: \[([-\d ]*)\]", text)
        if not dense or not extents or not origin:
            raise Failure("the discovery pass prints no dense GEMM")
        out["roles"] = dict(zip("IJK", (int(v) for v in dense.groups())))
        out["extents"] = dict(zip("MNK", (int(v) for v in extents.groups())))
        out["origin"] = [int(v) for v in origin.group(1).split()]
        out["views"] = {}
        for operand in "ABC":
            view = re.search(rf"^\s*view {operand}: \[(.*)\]$", text, re.MULTILINE)
            if not view:
                raise Failure(f"the discovery pass prints no view of {operand}")
            dimensions = []
            for part in view.group(1).split(","):
                one = re.fullmatch(r"\s*(-?\d+)\*([IJK]) \+ (-?\d+)\s*", part)
                if not one:
                    raise Failure(f"a dimension of the view of {operand} is no scale * role + constant: {part}")
                dimensions.append((int(one.group(1)), one.group(2), int(one.group(3))))
            # The view is printed role by role. `layout X: roles in dimensions (p, q)` says
            # which dimension of the array each is; with no such line, the first and second.
            layout = re.search(rf"^\s*layout {operand}: roles in dimensions \((\d+), (\d+)\)", text, re.MULTILINE)
            if re.search(rf"^\s*layout {operand}: general view", text, re.MULTILINE) or len(dimensions) != 2:
                raise Failure(f"the view of {operand} is none this comparison reads")
            if layout:
                placed = [None, None]
                placed[int(layout.group(1))], placed[int(layout.group(2))] = dimensions
                dimensions = placed
            out["views"][operand] = dimensions
        out["reassoc"] = "reduction: reassoc permitted" in text
        out["accumulation"] = "statement: C = C + A * B (accumulation)" in text
        out["bijection"] = "logical bijection: verified" in text
    _, _, text = run([opt, source, "--affine-matmul-to-systolic=report=true", "-o", "/dev/null"])
    out["alias_gate"] = "A may overlap C" in text or "B may overlap C" in text
    lowered = re.search(r"remark: affine GEMM (\d+)x(\d+)x(\d+) lowered as", text)
    out["lowered_plain"] = [int(v) for v in lowered.groups()] if lowered else None
    _, _, text = run([opt, source, "--affine-matmul-to-systolic=report=true assume-no-alias=true", "-o", "/dev/null"])
    lowered = re.search(r"remark: affine GEMM (\d+)x(\d+)x(\d+) lowered as", text)
    out["lowered"] = [int(v) for v in lowered.groups()] if lowered else None
    out["fp_refusal"] = "floating-point reassociation is not permitted" in text
    return out


def record(driver, source, name, width, authorize):
    command = [driver, source, f"--function={name}", f"--index-bit-width={width}"] + SOLVER
    if authorize:
        command.append("--authorize=" + ",".join(authorize))
    code, text, error = run(command)
    if code:
        raise Failure(f"the nest driver exits {code}: {error[-500:]}")
    function = json.loads(text)[0]
    if len(function["nests"]) != 1:
        raise Failure(f"{name} has {len(function['nests'])} nests, and one is compared")
    return function, function["nests"][0]


def shape(nest):
    """The fields of a record that the production tool also states."""
    gemm = nest["gemm"]
    views = {}
    for operand in "ABC":
        dimensions = []
        for dimension in gemm["views"][operand]["dimensions"]:
            if not dimension["affine"] or len(dimension["levels"]) != 1 or dimension["parameters"]:
                raise Failure(f"a dimension of the record's view of {operand} is no scale * level + constant")
            term = dimension["levels"][0]
            dimensions.append((term["coefficient"], gemm["levels"][term["level"]]["axis"], dimension["constant"]))
        views[operand] = dimensions
    return {"extents": gemm["extents"], "roles": {axis: gemm["axes"][axis][0] for axis in "IJK"},
            "origin": [level["lower"] for level in gemm["levels"]], "views": views}


def compare(name, opt, driver, text, width, twin_of, rows):
    with tempfile.NamedTemporaryFile("w", suffix=".mlir") as source:
        source.write(text)
        source.flush()
        theirs = production(opt, source.name)
        function, bare = record(driver, source.name, name, width, [])
        count = len(function["roots"])
        every = [f"roots {a} and {b} are separate" for a in range(count) for b in range(a + 1, count)]
        _, named = record(driver, source.name, name, width, every)
    fresh = any(pair["kind"] == "proof" for pair in named["separation"])
    noalias = any(root.get("noalias") for root in function["roots"])
    failures = []

    def row(field, production_says, record_says, equal, note=""):
        rows.append((name, field, production_says, record_says, "equal" if equal else "DIFFERENT", note))
        if not equal:
            failures.append(f"{name}: {field}: production {production_says}, record {record_says}")

    if named["status"] != "VERIFIED":
        raise Failure(f"{name}: the record with every pair authorized is {named['status']}: {named['refusals']}")
    mine = shape(named)
    if not theirs["recognized"]:
        # Integers: outside the production tool's fragment. The record is held to its twin's.
        rows.append((name, "recognition", "refused: " + theirs["reason"][:60] + "...", named["status"], "policy",
                     f"compared with the record of @{twin_of}" if twin_of else "no twin given"))
        return mine, failures
    row("M, N, K", theirs["extents"], mine["extents"], theirs["extents"] == mine["extents"])
    row("roles (loop of I, J, K)", theirs["roles"], mine["roles"], theirs["roles"] == mine["roles"])
    row("origin (lower bounds)", theirs["origin"], mine["origin"], theirs["origin"] == mine["origin"])
    for operand in "ABC":
        row(f"view of {operand}", theirs["views"][operand], mine["views"][operand],
            theirs["views"][operand] == mine["views"][operand])
    accumulation = named["gemm"]["accumulation"]
    row("statement", "C = C + A * B" if theirs["accumulation"] else "other",
        accumulation["statement"] + " from " + accumulation["starts_from"],
        theirs["accumulation"] and accumulation["reduction"] == "fadd" and accumulation["multiply"] == "fmul")
    flag = named["reassociation"]["source_flag_reassoc"]
    row("reassoc flag of the accumulation", theirs["reassoc"], flag, theirs["reassoc"] == flag)
    domain = named["gemm"]["tiers"]["domain"]["symbolic"]
    exact = domain["status"] == "VERIFIED" and domain["exact_order"] and domain["multiplicity"] == "UNIT"
    row("bijection onto the box", theirs["bijection"], exact, theirs["bijection"] == exact)
    # The alias gate, with nothing said by the caller.
    refused = bare["status"] not in ("VERIFIED", "VERIFIED_NOT_MAPPED") and not bare["ordered_fold_verified"]
    says = any("may overlap C" in text for text in bare["refusals"])
    if fresh:
        row("alias gate, nothing authorized", "lowered" if theirs["lowered_plain"] or theirs["fp_refusal"] or
            not theirs["alias_gate"] else "refused", "verified" if not refused else "refused",
            (not theirs["alias_gate"]) == (not refused), "C is allocated in the function: a proof for both")
    elif noalias:
        rows.append((name, "alias gate, nothing authorized", "accepted" if not theirs["alias_gate"] else "refused",
                     "refused" if refused else "verified", "policy",
                     "llvm.noalias is proof for the production tool; a record needs the name, as an assumption"))
        if not refused:
            failures.append(f"{name}: a record verified on llvm.noalias alone")
    else:
        row("alias gate, nothing authorized", "refused: may overlap C" if theirs["alias_gate"] else "accepted",
            "refused: may overlap C" if refused and says else "verified" if not refused else "refused, other words",
            theirs["alias_gate"] and refused and says)
    # Whether the order may change, with the operands taken apart.
    permitted = named["reassociation"]["permitted"]
    lowered = theirs["lowered"] is not None
    row("reassociation, operands apart", "lowered" if lowered else "refused: not permitted" if theirs["fp_refusal"]
        else "refused, other", "permitted by " + named["reassociation"]["basis"] if permitted else "not permitted",
        lowered == permitted and (lowered or theirs["fp_refusal"]))
    if lowered:
        report = dict(zip("MNK", theirs["lowered"]))
        row("M x N x K of the report", report, mine["extents"], report == mine["extents"])
    return mine, failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nest-driver", required=True)
    parser.add_argument("--systolic-opt", required=True)
    parser.add_argument("--index-bit-width", type=int, default=32)
    parser.add_argument("--functions", required=True, help="comma separated")
    parser.add_argument("--twin", action="append", default=[], help="integer=float")
    parser.add_argument("input")
    args = parser.parse_args()
    functions = functions_of(open(args.input).read())
    twins = dict(item.split("=") for item in args.twin)
    rows, failures, shapes = [], [], {}
    for name in args.functions.split(","):
        if name not in functions:
            failures.append(f"no function @{name} in {args.input}")
            continue
        try:
            shapes[name], found = compare(name, args.systolic_opt, args.nest_driver, functions[name],
                                          args.index_bit_width, twins.get(name), rows)
            failures += found
        except Failure as failure:
            failures.append(f"{name}: {failure}")
    for integer, real in twins.items():
        if integer in shapes and real in shapes:
            equal = shapes[integer] == shapes[real]
            rows.append((integer, f"record against the record of @{real}", "-", "extents, roles, origin, views",
                         "equal" if equal else "DIFFERENT", "the twin is compared with the production tool"))
            if not equal:
                failures.append(f"{integer}: its record differs from its twin's: {shapes[integer]} and {shapes[real]}")
        else:
            failures.append(f"twin {integer}={real}: a record is missing")
    widths = [max(len(str(row[n])) for row in rows + [("function", "field", "production", "record", "", "")])
              for n in range(5)]
    print("  ".join(title.ljust(widths[n]) for n, title in enumerate(["function", "field", "production", "record",
                                                                      "result"])))
    for row in rows:
        print("  ".join(str(row[n]).ljust(widths[n]) for n in range(5)) + ("  " + row[5] if row[5] else ""))
    compared = sum(row[4] == "equal" for row in rows)
    print(f"{len(rows)} fields, {compared} equal, {sum(row[4] == 'policy' for row in rows)} differences of policy, "
          f"{len(failures)} failures")
    for failure in failures:
        print("FAIL:", failure, file=sys.stderr)
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
