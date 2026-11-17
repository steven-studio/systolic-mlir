#!/usr/bin/env python3
"""Stage 2 (docs/GENERALIZED_GEMM_SCOPE.md): the records the production gate's tests read.

The production tests cannot run the research tool, so the records they read are files in
test/Systolic/Inputs, written by this script from the tool's own output and nothing else:

  nest_records.json               what generalized-gemm-nest-report says of
                                  test/Systolic/nest_records.mlir, every pair of roots
                                  authorized, at a 64-bit index
  nest_records_unauthorized.json  its function @single with no pair authorized
  nest_records_width16.json       @single at a 16-bit index
  nest_records_stale.json         @single as it was before its accumulation was marked
                                  reassoc: a record of an older version of the nest
  nest_records_belts.json         what it says of test/Systolic/nest_records_belts.mlir,
                                  every pair of roots authorized, at a 64-bit index
  nest_records_driven.json        of test/Systolic/nest_records_driven.mlir (stage 3), the same way
  nest_records_driven_refusals.json  of test/Systolic/nest_records_driven_refusals.mlir, the same way

Each file holds the tool's JSON, one function object per line. The solver's resource limit,
not its time limit, decides every query, so the tool says the same of the same input on a
loaded machine. test/Systolic/Inputs/edit_nest_records.py derives the other refusals from
nest_records.json, one edited field each.

  --write   make the files in the tree
  --check   make them in a scratch directory; each must equal the file in the tree, byte for
            byte, or the production tests no longer read what the research tool says today
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import subprocess
import sys
import tempfile

TEST = "test/Systolic/nest_records.mlir"
BELTS = "test/Systolic/nest_records_belts.mlir"
DRIVEN = "test/Systolic/nest_records_driven.mlir"
DRIVEN_REFUSALS = "test/Systolic/nest_records_driven_refusals.mlir"
INPUTS = "test/Systolic/Inputs"
OPTIONS = ["--max-level=1", "--candidates-per-group=1", "--timeout-ms=120000", "--rlimit=20000000"]
EVERY_PAIR = "--authorize=" + ",".join(f"roots {a} and {b} are separate" for a in range(5) for b in range(a + 1, 5))
NAMES = ["nest_records.json", "nest_records_unauthorized.json", "nest_records_width16.json",
         "nest_records_stale.json", "nest_records_belts.json", "nest_records_driven.json",
         "nest_records_driven_refusals.json"]


def records(driver, cwd, arguments, test=TEST):
    """The tool's records of `test` under cwd, the path given relative, as it is in the tree."""
    done = subprocess.run([driver, test] + arguments + OPTIONS, cwd=cwd, capture_output=True, text=True,
                          timeout=7200)
    if done.returncode != 0:
        sys.exit(f"the nest driver exits {done.returncode} on {test} {arguments}:\n{done.stderr[-2000:]}")
    return "[\n" + ",\n".join(json.dumps(f, separators=(",", ":")) for f in json.loads(done.stdout)) + "\n]\n"


def stale(text):
    """The test file with @single as it was before its accumulation was marked reassoc."""
    start = text.index("func.func @single(")
    end = text.index("\nfunc.func @", start)
    body = text[start:end]
    if body.count(" fastmath<reassoc>") != 1:
        sys.exit("@single is not the function this script knows")
    return text[:start] + body.replace(" fastmath<reassoc>", "") + text[end:]


def make(driver, root, out):
    with tempfile.TemporaryDirectory() as scratch:
        older = Path(scratch)
        (older / "test/Systolic").mkdir(parents=True)
        (older / TEST).write_text(stale((root / TEST).read_text()))
        jobs = [(root, ["--index-bit-width=64", EVERY_PAIR]),
                (root, ["--function=single", "--index-bit-width=64"]),
                (root, ["--function=single", "--index-bit-width=16", EVERY_PAIR]),
                (older, ["--function=single", "--index-bit-width=64", EVERY_PAIR]),
                (root, ["--index-bit-width=64", EVERY_PAIR], BELTS),
                (root, ["--index-bit-width=64", EVERY_PAIR], DRIVEN),
                (root, ["--index-bit-width=64", EVERY_PAIR], DRIVEN_REFUSALS)]
        with ThreadPoolExecutor(max_workers=len(jobs)) as pool:
            texts = list(pool.map(lambda job: records(driver, *job), jobs))
    for name, text in zip(NAMES, texts):
        (out / name).write_text(text)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True, help="generalized-gemm-nest-report")
    parser.add_argument("--root", required=True, help="the repository root")
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--write", action="store_true")
    mode.add_argument("--check", action="store_true")
    args = parser.parse_args()
    root = Path(args.root).resolve()
    if args.write:
        make(args.driver, root, root / INPUTS)
        print("wrote", ", ".join(NAMES))
        return
    with tempfile.TemporaryDirectory() as scratch:
        make(args.driver, root, Path(scratch))
        differ = [name for name in NAMES
                  if (Path(scratch) / name).read_bytes() != (root / INPUTS / name).read_bytes()]
    for name in differ:
        print(f"FAIL: {INPUTS}/{name} is not what the research tool says of its input today", file=sys.stderr)
    print(f"{len(NAMES)} files, {len(differ)} differ")
    sys.exit(1 if differ else 0)


if __name__ == "__main__":
    main()
