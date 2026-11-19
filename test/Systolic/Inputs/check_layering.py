#!/usr/bin/env python3
"""Architecture rules as a check: what a file may not depend on.

The records mode's file, lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp, is the
high-level one: it decides which nest goes which way and in which order a GEMM's steps are
emitted. How a step is emitted (the operand views, the tables, the tiles of the plan, the
proof that they partition the box) is behind include/Systolic/MatmulTileLowering.h. That
boundary was drawn by moving code out of the file; this keeps it drawn. A header listed in
FORBIDDEN below must not be a compile dependency of the file, whether the file includes it
itself or a header it includes does.

Two ways of looking, both used:

  the includes   the file's own #include lines, and those of every header of this project
                 that it includes, followed to the end. Needs the sources and nothing else,
                 and says through which line of which file a header comes.
  the compiler   the dependency list the build recorded for the file's object
                 (`ninja -t deps`): every header the compiler really opened, those that come
                 through LLVM's and MLIR's own headers among them.

A header in PLANNED is one that should be forbidden and cannot be yet: it is reported and not
refused. When it no longer reaches the file, the check fails once, to say that the entry is
to be moved to FORBIDDEN, so that what was gained is kept.

Exit status: 0 the boundary holds; 1 a rule is broken (or a planned header is gone and its
entry is not moved); 2 the check could not be made (the file is not there, or the compiler's
list was asked for and cannot be read).

Usage:
  check_layering.py --source-root ROOT                     the includes only
                    [--build-root DIR]                     and the compiler's list of that build
                    [--find-build-root-from PATH]          ... of the build PATH is inside
                    [--deps-file FILE]                     ... read from FILE (ninja -t deps OBJECT)
                    [--require-deps]                       exit 2 when that list cannot be read
"""
import argparse
from pathlib import Path
import re
import subprocess
import sys

ADVICE = [
    "The high-level dispatch file must not include low-level IR or partitioning headers, itself",
    "or through a header it includes. Put the detail behind MatmulTileLowering",
    "(include/Systolic/MatmulTileLowering.h, lib/Systolic/Transforms/MatmulTileLowering.cpp)",
    "and call it from there.",
    "高層調度檔不應直接包含底層 IR/Partitioning 標頭，請將細節封裝進 `MatmulTileLowering`。",
]

RULES = [
    {
        "file": "lib/Systolic/Transforms/GeneralizedMatmulToSystolic.cpp",
        "role": "the high-level dispatch file of the records mode",
        # header (as it is included) -> what it would bring into the file
        "FORBIDDEN": {
            "Systolic/AffineMatmulPartitioning.h":
                "the partition of a domain into boxes (LogicalBox): planMatmulTiles proves the partition",
            "mlir/Dialect/MemRef/IR/MemRef.h":
                "memref operations: views, tables and matrices are made by materializeOperandView and "
                "materializeTableOperands",
            "mlir/Dialect/Arith/IR/Arith.h":
                "arith operations: index arithmetic belongs to the emission, not to the walk",
        },
        # header -> what has to happen before it can be forbidden
        "PLANNED": {
            "mlir/Dialect/Affine/IR/AffineOps.h":
                "the walk's stage 2 reads LogicalMatmul (Systolic/AffineMatmulAnalysis.h), which holds affine "
                "operations; forbid it once the walk sees the affine recognizer through an interface that names "
                "no affine type",
        },
    },
]

INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*[<"]([^>"]+)[>"]', re.M)


def includes_of(path):
    """(header, line number) for every #include of a file."""
    text = path.read_text(errors="replace")
    return [(m.group(1), text.count("\n", 0, m.start()) + 1) for m in INCLUDE.finditer(text)]


def resolve(root, name, including):
    """The project's file that `name` is when `including` includes it, or None (a header of
    LLVM, MLIR, the system, or one the build generates)."""
    for candidate in (including.parent / name, root / "include" / name, root / name):
        if candidate.is_file():
            return candidate
    return None


def scan(root, start, wanted):
    """Follows the includes from `start` through the project's headers, breadth first. For
    each wanted header that is met: the shortest chain to it, as (file, line, header) hops."""
    found, seen, frontier = {}, {start}, [(start, [])]
    while frontier:
        following = []
        for path, chain in frontier:
            for name, line in includes_of(path):
                hop = chain + [(path, line, name)]
                for header in wanted:
                    if (name == header or name.endswith("/" + header)) and header not in found:
                        found[header] = hop
                target = resolve(root, name, path)
                if target is not None and target not in seen:
                    seen.add(target)
                    following.append((target, hop))
        frontier = following
    return found


def compiler_list(root, relative, build, deps_file):
    """The files the compiler opened for the object of `relative`, and why not when it cannot
    be had: (list or None, reason). A record of a build is of an earlier text when one of the
    files it lists is newer than the object, which is when ninja itself would build again."""
    object_name = Path(relative).name + ".o"
    if deps_file is not None:
        try:
            text = Path(deps_file).read_text()
        except OSError as error:
            return None, f"{deps_file}: {error.strerror}"
        source = f"{deps_file}"
    else:
        if build is None:
            return None, "no build directory was given"
        if not (build / "build.ninja").is_file():
            return None, f"{build} is not a Ninja build (no build.ninja)"
        try:
            done = subprocess.run(["ninja", "-C", str(build), "-t", "deps"], capture_output=True, text=True, timeout=600)
        except (OSError, subprocess.TimeoutExpired) as error:
            return None, f"ninja -t deps could not be run: {error}"
        if done.returncode != 0:
            return None, "ninja -t deps failed: " + done.stderr.strip()[-200:]
        text, source = done.stdout, f"ninja -C {build} -t deps"
    # An entry: `OBJECT: #deps N, deps mtime T (VALID)`, then one indented path a line.
    entries, current = {}, None
    for line in text.splitlines():
        if line and not line[0].isspace():
            head = line.split(": #deps", 1)
            current = head[0] if len(head) == 2 else None
            if current is not None:
                entries[current] = {"valid": "(STALE)" not in line, "deps": []}
        elif current is not None and line.strip():
            entries[current]["deps"].append(line.strip())
    mine = [name for name in entries if name.endswith("/" + object_name) or name == object_name]
    if not mine:
        return None, f"{source} has no entry for {object_name}: the file has not been built there"
    deps = []
    for name in mine:
        if not entries[name]["valid"]:
            return None, f"the dependency record of {name} is stale: build again"
        if build is not None and deps_file is None and (build / name).is_file():
            built = (build / name).stat().st_mtime
            listed = [build / d for d in entries[name]["deps"]]  # an absolute path stays what it is
            newer = sorted(said(root, f) for f in listed if f.is_file() and f.stat().st_mtime > built)
            if newer:
                return None, (f"{name} is older than {newer[0]}" + (f" and {len(newer) - 1} more" if len(newer) > 1 else "") +
                              ": its dependency record is of an earlier text; build again")
        deps += entries[name]["deps"]
    return deps, source


def said(root, path):
    try:
        return str(path.relative_to(root))
    except ValueError:
        return str(path)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--source-root", required=True)
    parser.add_argument("--build-root")
    parser.add_argument("--find-build-root-from")
    parser.add_argument("--deps-file")
    parser.add_argument("--require-deps", action="store_true")
    args = parser.parse_args()
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")
    root = Path(args.source_root).resolve()
    build = Path(args.build_root).resolve() if args.build_root else None
    if build is None and args.find_build_root_from:
        for parent in Path(args.find_build_root_from).resolve().parents:
            if (parent / "CMakeCache.txt").is_file():
                build = parent
                break

    status = 0
    for rule in RULES:
        relative = rule["file"]
        path = root / relative
        if not path.is_file():
            print(f"CANNOT CHECK: {relative} is not under {root}. If the file was moved or renamed, the rule that guards "
                  f"it must follow it (RULES in test/Systolic/Inputs/check_layering.py).")
            status = max(status, 2)
            continue
        forbidden, planned = rule["FORBIDDEN"], rule["PLANNED"]
        chains = scan(root, path, list(forbidden) + list(planned))
        deps, reason = compiler_list(root, relative, build, args.deps_file)
        unread = deps is None and args.require_deps
        opened = {header for header in list(forbidden) + list(planned)
                  if deps is not None and any(d.replace("\\", "/").endswith("/" + header) for d in deps)}

        def chain_text(header):
            return "; ".join(f"{said(root, file)}:{line} includes {name}" for file, line, name in chains[header])

        broken = [header for header in forbidden if header in chains or header in opened]
        for header in broken:
            print(f"ARCHITECTURE VIOLATION: {relative} ({rule['role']}) depends on {header}")
            if header in chains:
                where = "its own #include" if len(chains[header]) == 1 else "a header it includes"
                print(f"  through {where}: {chain_text(header)}")
                if header in opened:
                    print("  and the compiler's dependency list of its object has it as well")
            else:
                print("  the compiler opened it for this file's object, though no #include of the file or of this "
                      "project's headers names it: it comes through a header of LLVM or MLIR that one of them includes "
                      "(the compiler's -H on the file shows the chain)")
            print(f"  forbidden because it brings {forbidden[header]}")
        if broken:
            for line in ADVICE:
                print(line)
            status = max(status, 1)

        gone = [header for header in planned if header not in chains and (deps is None or header not in opened)]
        for header in planned:
            if header in gone:
                continue
            via = chain_text(header) if header in chains else "the compiler's dependency list"
            print(f"not forbidden yet: {relative} still depends on {header} ({via}): {planned[header]}.")
        for header in gone:
            if deps is None:
                print(f"note: {header} is no longer reached by the includes of {relative}; whether the compiler still "
                      f"opens it is not known here ({reason}).")
                continue
            print(f"RULE TO TIGHTEN: {relative} no longer depends on {header}. Keep it so: move the entry from PLANNED to "
                  f"FORBIDDEN in test/Systolic/Inputs/check_layering.py.")
            status = max(status, 1)

        if unread:
            # What the includes show has been said above; the rest is not known, and not passed.
            print(f"CANNOT CHECK: the compiler's dependency list of {relative} cannot be read: {reason}.")
            status = max(status, 2) if not broken else status
        elif not broken:
            looked = "its includes and the compiler's dependency list" if deps is not None else "its includes"
            extra = "" if deps is not None else f" (the compiler's dependency list was not read: {reason})"
            print(f"{relative}: {looked} hold none of the {len(forbidden)} forbidden headers{extra}")
    return status


if __name__ == "__main__":
    sys.exit(main())
