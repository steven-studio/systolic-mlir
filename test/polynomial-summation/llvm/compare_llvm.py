#!/usr/bin/env python3
"""Record actual LLVM analysis/optimization and test generated code independently.

No LLVM source or build directory is written. --output must name a directory in
this worktree (or /tmp). The retained text/IR is evidence, not assumed golden
output: behavior may change with another LLVM version.
"""
import argparse
import ctypes
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile


def sha256(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def sections(text):
    parts = re.split(r"Printing analysis 'Scalar Evolution Analysis' for function '([^']+)':\n", text)
    if not parts[0].strip() and len(parts) > 1:
        return dict(zip(parts[1::2], parts[2::2]))
    raise RuntimeError("unexpected ScalarEvolution report format")


def observations(text):
    result = {}
    for name, block in sections(text).items():
        phis = dict(re.findall(r"  %([a-z]+\.0) = phi[^\n]*\n  -->  ([^\n]+)", block))
        result[name] = {
            "loops": len(re.findall(r"^(?:Parallel )?Loop at depth ", block, re.MULTILINE)),
            "header_phis": phis,
            "backedge_counts": re.findall(r"^Loop .*backedge-taken count.*$", block, re.MULTILINE),
        }
    return result


def runtime_checks(library):
    lib = ctypes.CDLL(str(library))
    mask = 2**64 - 1
    checks = 0
    functions = [f"sum_p{k}" for k in range(1, 9)] + ["general_cubic", "triangular",
                "nonlinear_self", "exponential"]
    for name in functions:
        function = getattr(lib, name)
        function.argtypes = [ctypes.c_uint64]
        function.restype = ctypes.c_uint64
        for n in list(range(101)) + [127, 128, 255, 256, 257, 511, 512, 1024]:
            if name.startswith("sum_p"):
                k = int(name[5:])
                expected = sum(pow(i, k) for i in range(1, n + 1)) & mask
            elif name == "general_cubic":
                expected = sum(7*i**3 - 3*i**2 + 11*i - 17 for i in range(1, n+1)) & mask
            elif name == "triangular":
                x = y = z = 0
                for _ in range(n):
                    x, y, z = (x + 1) & mask, (y + x*x) & mask, (z + y) & mask
                expected = z
            elif name == "nonlinear_self":
                expected = 2
                for _ in range(n):
                    expected = (expected * expected + 1) & mask
            else:
                expected = pow(2, n, 2**64)
            actual = function(n)
            if actual != expected:
                raise AssertionError((name, n, actual, expected))
            checks += 1
    shifted = lib.shifted_square
    shifted.argtypes = [ctypes.c_uint64] * 4
    shifted.restype = ctypes.c_uint64
    for start, step, initial in [(-17, 3, 91), (2**63+7, 2**62+3, 2**64-1),
                                  (0, 1, 0), (99, 0, 27)]:
        for n in range(101):
            expected = (initial + sum((start + step*i)**2 for i in range(n))) & mask
            actual = shifted(n, start, step, initial)
            if actual != expected:
                raise AssertionError(("shifted_square", n, actual, expected))
            checks += 1
    fixed = lib.fixed_square
    fixed.argtypes = []
    fixed.restype = ctypes.c_uint64
    assert fixed() == sum(i*i for i in range(1, 11))
    checks += 1
    observable = lib.observable_square
    observable.argtypes = [ctypes.c_uint64, ctypes.POINTER(ctypes.c_uint64)]
    observable.restype = ctypes.c_uint64
    for n in range(101):
        out = ctypes.c_uint64(12345)
        expected = sum(i*i for i in range(1, n+1)) & mask
        assert observable(n, ctypes.byref(out)) == expected
        assert out.value == (expected if n else 12345)
        checks += 1
    # unknown_exit is intentionally NOT executed: no termination analysis.
    return checks


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--clang", required=True, type=Path)
    parser.add_argument("--opt", required=True, type=Path)
    parser.add_argument("--llvm-source", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[3]
    output = args.output.resolve()
    if not output.is_relative_to(root) and not output.is_relative_to(Path("/tmp")):
        parser.error("--output must be in this worktree or /tmp")
    output.mkdir(parents=True, exist_ok=True)
    source = Path(__file__).with_name("recurrences.c").resolve()
    commands = []

    def run(command):
        argv = list(map(str, command))
        commands.append(argv)
        return subprocess.run(argv, text=True, capture_output=True, check=True)

    versions = {"clang": run([args.clang, "--version"]).stdout,
                "opt": run([args.opt, "--version"]).stdout}
    revision = run(["git", "-C", args.llvm_source, "rev-parse", "HEAD"]).stdout.strip()
    source_status = run(["git", "-C", args.llvm_source, "status", "--porcelain"]).stdout
    header = args.opt.resolve().parents[1] / "include/llvm/Support/VCSRevision.h"
    frontend, canonical, optimized = [output / f"{name}.ll" for name in
                                       ["frontend", "canonical", "optimized"]]
    run([args.clang, "-O0", "-Xclang", "-disable-O0-optnone", "-fno-discard-value-names",
         "-S", "-emit-llvm", source, "-o", frontend])
    run([args.opt, "-S", "-passes=mem2reg,loop-simplify,lcssa", "-verify-each",
         frontend, "-o", canonical])
    before = run([args.opt, "-passes=print<scalar-evolution>,print<loops>",
                  "-disable-output", canonical]).stderr
    (output / "scev-before.txt").write_text(before)
    run([args.opt, "-S", "-passes=default<O2>", "-verify-each", canonical, "-o", optimized])
    after = run([args.opt, "-passes=print<scalar-evolution>,print<loops>",
                 "-disable-output", optimized]).stderr
    (output / "scev-after.txt").write_text(after)
    before_obs, after_obs = observations(before), observations(after)
    if before_obs.keys() != after_obs.keys():
        raise AssertionError("functions changed unexpectedly")
    # Isolate the observed degree-8 default limit from mathematical support.
    # This is a separate, explicitly non-default LLVM experiment.
    larger_limit = "-scalar-evolution-max-add-rec-size=16"
    larger_before = run([args.opt, larger_limit,
                        "-passes=print<scalar-evolution>,print<loops>",
                        "-disable-output", canonical]).stderr
    larger_ir = output / "optimized-size16.ll"
    run([args.opt, larger_limit, "-S", "-passes=default<O2>", "-verify-each",
         canonical, "-o", larger_ir])
    larger_after = run([args.opt, larger_limit,
                       "-passes=print<scalar-evolution>,print<loops>",
                       "-disable-output", larger_ir]).stderr
    # Retain the relevant complete function reports, avoiding duplicated logs.
    (output / "sum-p8-size16.txt").write_text(
        "BEFORE O2 (max AddRec size = 16):\n" + sections(larger_before)["sum_p8"] +
        "\nAFTER O2 (max AddRec size = 16):\n" + sections(larger_after)["sum_p8"])
    with tempfile.TemporaryDirectory(prefix="llvm-runtime-", dir=output) as tmp:
        library = Path(tmp) / "recurrences.so"
        run([args.clang, "-O0", "-shared", "-fPIC", optimized, "-o", library])
        comparisons = runtime_checks(library)
        larger_library = Path(tmp) / "recurrences-size16.so"
        run([args.clang, "-O0", "-shared", "-fPIC", larger_ir, "-o", larger_library])
        larger_comparisons = runtime_checks(larger_library)
    report = {
        "llvm_source_commit": revision, "llvm_source_status_porcelain": source_status,
        "opt_build_revision_header": header.read_text() if header.exists() else None,
        "versions": versions,
        "sha256": {str(path): sha256(path) for path in [source, args.clang.resolve(),
                  args.opt.resolve(), canonical, optimized,
                  args.llvm_source / "llvm/lib/Analysis/ScalarEvolution.cpp",
                  args.llvm_source / "llvm/include/llvm/Analysis/ScalarEvolutionExpressions.h"]},
        "commands": commands, "before": before_obs, "after": after_obs,
        "optimized_runtime_oracle_comparisons": comparisons,
        "max_add_rec_size_16": {
            "before_sum_p8": observations(larger_before)["sum_p8"],
            "after_sum_p8": observations(larger_after)["sum_p8"],
            "optimized_runtime_oracle_comparisons": larger_comparisons,
            "optimized_ir_sha256": sha256(larger_ir),
        },
        "not_executed": ["unknown_exit (no termination guarantee)"],
    }
    (output / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    for name in before_obs:
        state = before_obs[name]["header_phis"].get("s.0",
                before_obs[name]["header_phis"].get("z.0",
                before_obs[name]["header_phis"].get("x.0", "")))
        scev = state.split(" U:")[0]
        print(f"{name}: loops {before_obs[name]['loops']} -> {after_obs[name]['loops']}; {scev}")
    print(f"PASS: {comparisons} optimized LLVM runtime comparisons against Python integers")
    print(f"With {larger_limit}: sum_p8 loops -> "
          f"{observations(larger_after)['sum_p8']['loops']}; "
          f"{larger_comparisons} additional runtime comparisons passed")
    print(f"Evidence: {output}")


if __name__ == "__main__":
    main()
