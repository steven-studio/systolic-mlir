#!/usr/bin/env python3
"""Runs GenerateIdentity.cmake on small repositories built here in a known state
and checks each field it writes. The expected values come from this script's own
setup, so a field that cannot be established has to say UNKNOWN.
"""
import argparse
import hashlib
import os
import re
import shutil
import subprocess
import sys
import time
from pathlib import Path

EMPTY_SHA256 = hashlib.sha256(b"").hexdigest()
PATCH_SCOPE = "solver/lib solver/include"


def fail(message):
    sys.exit("FAIL: " + message)


def git(directory, *arguments):
    done = subprocess.run(
        ["git", "-C", str(directory), "-c", "user.name=test",
         "-c", "user.email=test@example.invalid", "-c", "commit.gpgsign=false",
         "-c", "init.defaultBranch=main", *arguments],
        capture_output=True)
    if done.returncode != 0:
        fail("git failed: " + " ".join(arguments) + "\n" + done.stderr.decode())
    return done.stdout


def commit_all(directory, message):
    git(directory, "add", "-A")
    git(directory, "commit", "-q", "-m", message)
    return git(directory, "rev-parse", "HEAD").decode().strip()


def source_tree(directory):
    """Something shaped like the project, with a diff worth hashing."""
    (directory / "include/Systolic/Exact").mkdir(parents=True)
    (directory / "lib/Systolic/Exact").mkdir(parents=True)
    (directory / "include/Systolic/Exact/One.h").write_text("int one;\n")
    (directory / "lib/Systolic/Exact/Two.cpp").write_text("int two[] = {1;2};\n")
    (directory / "elsewhere.txt").write_text("not part of the solver\n")


def llvm_tree(directory):
    (directory / "solver/lib").mkdir(parents=True)
    (directory / "solver/include").mkdir(parents=True)
    (directory / "docs").mkdir()
    (directory / "solver/lib/code.cpp").write_text("int code;\n")
    (directory / "docs/notes.txt").write_text("notes\n")


def fingerprint(root):
    paths = sorted(
        str(path.relative_to(root))
        for name in ("include/Systolic/Exact", "lib/Systolic/Exact")
        for path in (root / name).iterdir())
    lines = "".join(
        hashlib.sha256((root / path).read_bytes()).hexdigest() + "  " + path + "\n"
        for path in paths)
    return hashlib.sha256(lines.encode()).hexdigest()


class Runner:
    def __init__(self, generator, work, git_tool):
        self.generator = generator
        self.work = work
        self.git_tool = git_tool
        self.cases = 0

    def generate(self, source, llvm, revision_header, use_git=True, empty=(),
                 stop_git_at=None):
        """Writes one config, runs the generator, and parses what it wrote.
        `stop_git_at` keeps git from looking above a directory, which is what
        makes a tree truly outside git wherever this script runs."""
        self.cases += 1
        directory = self.work / f"case{self.cases}"
        directory.mkdir()
        output = directory / "SolverIdentity.inc"
        values = {
            "SOURCE_ROOT": source, "OUTPUT": output,
            "GIT": self.git_tool if use_git else "",
            "VCS_HEADER": revision_header, "LLVM_SOURCE_DIR": llvm,
            "LLVM_PATCH_SCOPE": PATCH_SCOPE, "COMPILER_ID": "GNU",
            "COMPILER_VERSION": "1.2.3", "BUILD_TYPE": "Release",
            "FLAGS_SHA256": "abc",
        }
        for name in empty:
            values[name] = ""
        config = directory / "IdentityConfig.cmake"
        config.write_text("".join(f'set({k} "{v}")\n' for k, v in values.items()))
        environment = dict(os.environ)
        if stop_git_at:
            environment["GIT_CEILING_DIRECTORIES"] = str(stop_git_at)
        done = subprocess.run(
            ["cmake", f"-DCONFIG={config}", "-P", str(self.generator)],
            capture_output=True, text=True, env=environment)
        if done.returncode != 0:
            fail("the generator failed:\n" + done.stderr)
        fields = {}
        for line in output.read_text().split("\n"):
            found = re.match(r'#define SYSTOLIC_EXACT_(\w+) "?([^"]*)"?$', line)
            if found:
                fields[found.group(1)] = found.group(2)
        return fields

    def expect(self, what, fields, **expected):
        for name, value in expected.items():
            if fields.get(name) != value:
                fail(f"{what}: {name} is {fields.get(name)!r}, not {value!r}")


def unwritten(repository, tracked_file, generate):
    """Runs `generate` and checks that it left `repository` alone. A plain git
    status takes the index lock, which moves the time of .git, and rewrites the
    index when a file's timestamp has moved, so one is moved here first."""
    later = time.time() + 1000
    os.utime(tracked_file, (later, later))
    metadata = repository / ".git"
    before = metadata.stat().st_mtime_ns, (metadata / "index").read_bytes()
    fields = generate()
    if (metadata.stat().st_mtime_ns, (metadata / "index").read_bytes()) != before:
        fail(f"the generator wrote to the repository {repository.name}")
    return fields


def header(path, revision):
    path.write_text(f'#define LLVM_REVISION R"({revision})"\n'
                    '#define LLVM_REPOSITORY R"(https://example.invalid/llvm)"\n')
    return path


def check_project(run, llvm, revision_header):
    # A clean repository: every field is known, and the diff is empty.
    source = run.work / "project"
    source.mkdir()
    git(source, "init", "-q")
    source_tree(source)
    commit = commit_all(source, "first")
    fields = unwritten(source, source / "elsewhere.txt",
                       lambda: run.generate(source, llvm, revision_header))
    run.expect("a clean project", fields, PROJECT_COMMIT=commit,
               PROJECT_TREE="Clean", PROJECT_DIFF_SHA256=EMPTY_SHA256,
               SOURCE_SHA256=fingerprint(source))

    # A change to a tracked solver file moves the state, the diff hash and the
    # fingerprint. The ; and [ in the diff are what CMake lists choke on.
    (source / "lib/Systolic/Exact/Two.cpp").write_text("int two[] = {3;4};\n")
    diff = git(source, "diff")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a modified project", fields, PROJECT_COMMIT=commit,
               PROJECT_TREE="Dirty",
               PROJECT_DIFF_SHA256=hashlib.sha256(diff).hexdigest(),
               SOURCE_SHA256=fingerprint(source))
    moved = fields["SOURCE_SHA256"]
    commit = commit_all(source, "second")

    # An untracked solver file is in the fingerprint and in the tree state. It
    # is not in the diff, which only ever covers tracked files.
    (source / "lib/Systolic/Exact/Three.cpp").write_text("int three;\n")
    fields = run.generate(source, llvm, revision_header)
    run.expect("an untracked solver file", fields, PROJECT_TREE="Dirty",
               PROJECT_DIFF_SHA256=EMPTY_SHA256,
               SOURCE_SHA256=fingerprint(source))
    if fields["SOURCE_SHA256"] == moved:
        fail("a new solver file did not move the fingerprint")
    (source / "lib/Systolic/Exact/Three.cpp").unlink()

    # A change outside the solver leaves the fingerprint alone.
    (source / "elsewhere.txt").write_text("changed\n")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a change elsewhere", fields, PROJECT_TREE="Dirty",
               SOURCE_SHA256=moved)
    commit_all(source, "third")

    # None of these can name a commit, and the fingerprint needs none of it.
    plain = run.work / "plain"
    plain.mkdir()
    source_tree(plain)
    fields = run.generate(plain, llvm, revision_header, stop_git_at=run.work)
    run.expect("a project outside git", fields, PROJECT_COMMIT="UNKNOWN",
               PROJECT_TREE="Unknown", PROJECT_DIFF_SHA256="UNKNOWN",
               SOURCE_SHA256=fingerprint(plain))
    # A tree inside another repository: git would answer for that repository.
    nested = source / "vendored"
    nested.mkdir()
    source_tree(nested)
    commit_all(source, "fourth")
    fields = run.generate(nested, llvm, revision_header)
    run.expect("a project inside another repository", fields,
               PROJECT_COMMIT="UNKNOWN", PROJECT_TREE="Unknown",
               PROJECT_DIFF_SHA256="UNKNOWN", SOURCE_SHA256=fingerprint(nested))
    fields = run.generate(source, llvm, revision_header, use_git=False)
    run.expect("no git tool", fields, PROJECT_COMMIT="UNKNOWN",
               PROJECT_TREE="Unknown", PROJECT_DIFF_SHA256="UNKNOWN",
               LLVM_PATCH_STATUS="Unknown")
    return source


def check_llvm(run, source, llvm, revision, revision_header):
    fields = unwritten(llvm, llvm / "solver/lib/code.cpp",
                       lambda: run.generate(source, llvm, revision_header))
    run.expect("the source at the built revision", fields,
               LLVM_PATCH_STATUS="NoneVerified")

    # Outside the scope is not a patch to what the solver's build reads.
    (llvm / "docs/notes.txt").write_text("changed\n")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a change out of scope", fields, LLVM_PATCH_STATUS="NoneVerified")
    git(llvm, "checkout", "-q", "--", "docs/notes.txt")

    # In scope, tracked and untracked.
    (llvm / "solver/lib/code.cpp").write_text("int patched;\n")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a tracked change in scope", fields, LLVM_PATCH_STATUS="Patched")
    git(llvm, "checkout", "-q", "--", "solver/lib/code.cpp")
    (llvm / "solver/include/new.h").write_text("int added;\n")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a new file in scope", fields, LLVM_PATCH_STATUS="Patched")
    (llvm / "solver/include/new.h").unlink()

    # A staged change is a change.
    (llvm / "solver/lib/code.cpp").write_text("int staged;\n")
    git(llvm, "add", "solver/lib/code.cpp")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a staged change in scope", fields, LLVM_PATCH_STATUS="Patched")
    git(llvm, "reset", "-q", "--hard")

    # The source has moved on from what was built: it no longer speaks for the
    # build, whichever way its files look.
    (llvm / "docs/notes.txt").write_text("a later commit\n")
    commit_all(llvm, "later")
    fields = run.generate(source, llvm, revision_header)
    run.expect("a source at another revision", fields, LLVM_PATCH_STATUS="Unknown")
    git(llvm, "reset", "-q", "--hard", revision)

    # Everything else that removes the evidence.
    missing = run.work / "no-such-directory"
    fields = run.generate(source, missing, revision_header)
    run.expect("no source tree", fields, LLVM_PATCH_STATUS="Unknown")
    plain = run.work / "plain-llvm"
    plain.mkdir()
    fields = run.generate(source, plain, revision_header, stop_git_at=run.work)
    run.expect("a source tree outside git", fields, LLVM_PATCH_STATUS="Unknown")
    # A subdirectory of the right repository, at the right revision. Its scope
    # names nothing there, so an unguarded status would come back empty.
    fields = run.generate(source, llvm / "docs", revision_header)
    run.expect("a source tree inside another repository", fields,
               LLVM_PATCH_STATUS="Unknown")
    fields = run.generate(source, llvm, missing / "VCSRevision.h")
    run.expect("no revision header", fields, LLVM_PATCH_STATUS="Unknown")
    garbled = header(run.work / "garbled.h", "not a revision")
    fields = run.generate(source, llvm, garbled)
    run.expect("a revision header with no revision", fields,
               LLVM_PATCH_STATUS="Unknown")


def check_empty_fields(run, source, llvm, revision_header):
    fields = run.generate(source, llvm, revision_header, empty=(
        "COMPILER_ID", "COMPILER_VERSION", "BUILD_TYPE", "FLAGS_SHA256",
        "LLVM_SOURCE_DIR"))
    run.expect("empty build fields", fields, COMPILER_ID="UNKNOWN",
               COMPILER_VERSION="UNKNOWN", BUILD_TYPE="UNKNOWN",
               FLAGS_SHA256="UNKNOWN", LLVM_SOURCE_DIR="UNKNOWN",
               LLVM_PATCH_STATUS="Unknown")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--generator", required=True, type=Path)
    parser.add_argument("--work", required=True, type=Path)
    args = parser.parse_args()
    git_tool = shutil.which("git")
    if not git_tool or not shutil.which("cmake"):
        fail("this check needs git and cmake")
    args.work.mkdir(parents=True)
    run = Runner(args.generator.resolve(), args.work.resolve(), git_tool)

    llvm = run.work / "llvm"
    llvm.mkdir()
    git(llvm, "init", "-q")
    llvm_tree(llvm)
    revision = commit_all(llvm, "the built revision")
    revision_header = header(run.work / "VCSRevision.h", revision)

    source = check_project(run, llvm, revision_header)
    check_llvm(run, source, llvm, revision, revision_header)
    check_empty_fields(run, source, llvm, revision_header)
    print(f"identity generator: {run.cases} cases verified")


main()
