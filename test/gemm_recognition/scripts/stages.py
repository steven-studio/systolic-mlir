"""
stages.py -- run one generated test program through the compiler, stage by
stage, and parse what each stage says.  Shared by run_showcase.py (the fixed
set) and loop.py (the generated corpus).

Stages, each a real tool invocation, each recorded with its command, exit
status and diagnostics:

  frontend    systolic-cfront --function=<kernel> FILE.c          -> MLIR or a diagnostic
  discovery   systolic-opt --systolic-declare-fpga-fleet
                           --test-affine-matmul-discovery          -> recognition / dense / fleet partition
  lowering    systolic-opt --systolic-declare-fpga-fleet
                           --affine-matmul-to-systolic=report=true -> lowered / remark / nothing
  driver      systolic-cc --runtime=soft|board --explain --verify  -> offloaded / stays on host / not translated
  execute     the compiled program                                  -> RESULT PASS/FAIL, invocations, verify lines

Nothing is inferred: a stage that did not run is reported as 'not_run' with
the reason, never as a pass.
"""

import json
import os
import re
import shlex
import subprocess
import sys
import time

FLEET = "--systolic-declare-fpga-fleet=k-max-8x8=%d k-max-4x4=%d"


class Tools:
    """Where the compiler and the LLVM tool chain are."""

    def __init__(self, repo, build_dir=None, llvm_build=None, clang=None, clang_major=None):
        self.repo = os.path.abspath(repo)
        self.build_dir = os.path.abspath(build_dir or os.environ.get("SYSTOLIC_BUILD")
                                         or os.path.join(self.repo, "build"))
        self.llvm_build = os.path.abspath(llvm_build or os.environ.get("LLVM_BUILD")
                                          or os.path.expanduser("~/work/llvm-project/build"))
        self.clang = clang or os.environ.get("SYSTOLIC_CFRONT_CLANG") or os.path.join(self.llvm_build, "bin", "clang")
        self.clang_major = clang_major
        self.cfront = os.path.join(self.build_dir, "bin", "systolic-cfront")
        self.opt = os.path.join(self.build_dir, "bin", "systolic-opt")
        self.cc = os.path.join(self.build_dir, "bin", "systolic-cc")

    def missing(self):
        out = []
        for name, path in (("systolic-cfront", self.cfront), ("systolic-opt", self.opt),
                           ("systolic-cc", self.cc), ("clang", self.clang),
                           ("mlir-translate", os.path.join(self.llvm_build, "bin", "mlir-translate")),
                           ("llc", os.path.join(self.llvm_build, "bin", "llc"))):
            if not os.path.isfile(path) or not os.access(path, os.X_OK):
                out.append("%s (%s)" % (name, path))
        return out

    def describe(self):
        info = {"build_dir": self.build_dir, "llvm_build": self.llvm_build, "clang": self.clang}
        try:
            info["clang_version"] = subprocess.run([self.clang, "--version"], capture_output=True,
                                                   text=True, timeout=30).stdout.splitlines()[0]
        except Exception as e:  # noqa: BLE001
            info["clang_version"] = "unavailable: %s" % e
        try:
            info["compiler_commit"] = subprocess.run(["git", "-C", self.repo, "rev-parse", "--short", "HEAD"],
                                                     capture_output=True, text=True, timeout=30).stdout.strip()
            info["compiler_dirty"] = bool(subprocess.run(
                ["git", "-C", self.repo, "status", "--porcelain", "--", "lib", "include", "tools", "runtime"],
                capture_output=True, text=True, timeout=30).stdout.strip())
        except Exception as e:  # noqa: BLE001
            info["compiler_commit"] = "unknown: %s" % e
        return info


def run(cmd, cwd=None, env=None, timeout=600, stdin_text=None):
    t0 = time.time()
    try:
        p = subprocess.run(cmd, cwd=cwd, env=env, capture_output=True, text=True,
                           timeout=timeout, input=stdin_text)
        return {"cmd": " ".join(shlex.quote(c) for c in cmd), "rc": p.returncode,
                "stdout": p.stdout, "stderr": p.stderr, "seconds": round(time.time() - t0, 3)}
    except subprocess.TimeoutExpired as e:
        return {"cmd": " ".join(shlex.quote(c) for c in cmd), "rc": None, "timeout": True,
                "stdout": (e.stdout or b"").decode() if isinstance(e.stdout, bytes) else (e.stdout or ""),
                "stderr": (e.stderr or b"").decode() if isinstance(e.stderr, bytes) else (e.stderr or ""),
                "seconds": round(time.time() - t0, 3)}
    except FileNotFoundError as e:
        return {"cmd": " ".join(shlex.quote(c) for c in cmd), "rc": None, "error": str(e),
                "stdout": "", "stderr": "", "seconds": 0}


def stage_frontend(tools, src, kernel, workdir, extra_flags=()):
    out = os.path.join(workdir, "10-frontend.mlir")
    cmd = [tools.cfront, "--clang", tools.clang]
    if tools.clang_major:
        cmd += ["--clang-major", str(tools.clang_major)]
    cmd += ["--function=%s" % kernel, *extra_flags, src, "-o", out]
    r = run(cmd, cwd=workdir)
    res = {"stage": "frontend", "run": r}
    if r["rc"] == 0 and os.path.exists(out):
        res["result"] = "translated"
        res["mlir"] = out
    elif r["rc"] == 1:
        res["result"] = "rejected"
        m = re.search(r"error: (.*)", r["stderr"])
        res["reason"] = m.group(1).strip() if m else r["stderr"].strip()[-300:]
    else:
        res["result"] = "tool_error"
        res["reason"] = (r.get("error") or r["stderr"].strip()[-500:])
    return res


def stage_discovery(tools, mlir, workdir, k8=16, k4=16):
    cmd = [tools.opt, FLEET % (k8, k4), "--test-affine-matmul-discovery", mlir, "-o", "/dev/null"]
    r = run(cmd, cwd=workdir)
    res = {"stage": "discovery", "run": r}
    if r["rc"] != 0:
        res["result"] = "tool_error"
        res["reason"] = r["stderr"].strip()[-500:]
        return res
    err = r["stderr"]
    m = re.search(r"unique logical matmul count: (\d+)", err)
    res["unique_matmuls"] = int(m.group(1)) if m else None
    res["candidate_count"] = int(re.search(r"candidate count: (\d+)", err).group(1)) if re.search(r"candidate count: (\d+)", err) else None
    res["recognition"] = "accept" if "recognition result: success" in err else "reject"
    m = re.search(r"dense logical GEMM: (yes|no)(?: \((.*)\))?", err)
    if m:
        res["dense"] = "supported" if m.group(1) == "yes" else "unsupported"
        res["dense_reason"] = m.group(2) or ""
        mm = re.search(r"dense logical GEMM: yes, I/J/K on loops (\S+)", err)
        res["dense_loops"] = mm.group(1) if mm else None
    else:
        res["dense"] = "not_reached"
    m = re.search(r"fleet partition: (\d+) regions, exact", err)
    if m:
        res["fleet_partition"] = "exact"
        res["fleet_regions"] = int(m.group(1))
    else:
        m = re.search(r"fleet partition: not exact \((.*)\)", err)
        res["fleet_partition"] = "not_exact" if m else "not_reached"
        if m:
            res["fleet_reason"] = m.group(1)
    for key in ("row", "column", "reduction"):
        m = re.search(r"%s extent: (\S+(?: \S+)?)" % key, err)
        res["%s_extent" % key] = m.group(1) if m else None
    # each operand's own view of its coordinates, e.g. "view C: [1*I + 8, 1*J + 16]"
    res["views"] = {m.group(1): m.group(2) for m in re.finditer(r"view ([ABC]): (\[[^\]]*\])", err)}
    mT = re.findall(r"T\[\d+\] = \[([^\]]*)\]", err)
    res["T"] = [[int(x) for x in row.split()] for row in mT] if mT else None
    m = re.search(r"c = \[([^\]]*)\]", err)
    res["c"] = [int(x) for x in m.group(1).split()] if m else None
    res["result"] = "ok"
    return res


def stage_lowering(tools, mlir, workdir, k8=16, k4=16):
    out = os.path.join(workdir, "20-systolic.mlir")
    # The cases are judged with the reassociation permission: the showcase's
    # predictions (and the specification, README section 2) are about the
    # mapping; the strict floating-point default (docs/AFFINE_GEMM_MAPPING.md
    # section 6, since 2026-10-15) keeps every fp GEMM on the host and is
    # tested on its own (test/Systolic/affine_gemm_p0_views.mlir,
    # test/SystolicCC/affine_p0.c).
    cmd = [tools.opt, FLEET % (k8, k4),
           "--affine-matmul-to-systolic=report=true allow-fp-reassociation=true", mlir, "-o", out]
    r = run(cmd, cwd=workdir)
    res = {"stage": "lowering", "run": r}
    if r["rc"] != 0:
        res["result"] = "tool_error"
        res["reason"] = r["stderr"].strip()[-500:]
        return res
    err = r["stderr"]
    m = re.search(r"remark: affine GEMM (\d+)x(\d+)x(\d+) lowered as (\d+) exact region\(s\)(.*)", err)
    if m:
        res["result"] = "lowered"
        res["M"], res["N"], res["K"] = int(m.group(1)), int(m.group(2)), int(m.group(3))
        res["regions"] = int(m.group(4))
        mm = re.search(r"makespan = (\d+) cycles", m.group(5))
        res["makespan"] = int(mm.group(1)) if mm else None
        mm = re.search(r"the enclosing loops run it (\d+) time", m.group(5))
        res["enclosing_repetitions"] = int(mm.group(1)) if mm else 1
    else:
        m = re.search(r"remark: affine GEMM not lowered to systolic.matmul_tile: (.*)", err)
        if m and m.group(1).strip().startswith("loop ") and "repeats the accumulation inside it" in m.group(1):
            # hoistRepetitionLoops' remark: the nest was left as it is and is
            # then not recognized (a repetition loop inside the reduction)
            res["result"] = "not_recognized"
            res["reason"] = m.group(1).strip()
        elif m:
            res["result"] = "recognized_not_lowered"
            res["reason"] = m.group(1).strip()
        else:
            res["result"] = "not_recognized"
            res["reason"] = ""
    try:
        with open(out) as f:
            text = f.read()
        res["matmul_tiles"] = text.count("systolic.matmul_tile")
        res["affine_for_left"] = text.count("affine.for")
    except OSError:
        pass
    return res


# A manifest entry's 'runtime_check' (generator/casespec.py Case.runtime_check):
# absent -- systolic-cc --verify compares bitwise (its default); 'signed_zero'
# -- the case accepts an element that differs from the CPU reference in the
# sign of zero only (an offloaded assignment's -0 product comes back as +0,
# docs/AFFINE_GEMM_MAPPING.md sections 6 and 20), so --verify compares
# numerically. systolic-cc's --verify-rtol R accepts |got - want| <= R *
# max(|got|, |want|), and R <= 0 means bitwise; with R = 1e-300 that is
# numerical equality for float data: -0 == +0, while any two different floats
# differ by at least 2^-149 > 1e-300 * FLT_MAX (and a NaN fails).
NUMERICAL_VERIFY_RTOL = "1e-300"


def verify_flags(entry):
    """The systolic-cc flags the entry's runtime check needs (none: bitwise)."""
    if (entry or {}).get("runtime_check") == "signed_zero":
        return ("--verify-rtol", NUMERICAL_VERIFY_RTOL)
    return ()


def stage_driver(tools, src, kernel, workdir, runtime="soft", extra_flags=(), env=None):
    exe = os.path.join(workdir, "prog")
    keep = os.path.join(workdir, "prog.systolic")
    cmd = [tools.cc, "--runtime=%s" % runtime, "--fp-reassociation", "--explain", "--verify", "-DGEMM_RECOGNITION_STATS",
           "--clang", tools.clang, "--build-dir", tools.build_dir, "--llvm-build", tools.llvm_build,
           "--keep-dir", keep, *extra_flags, src, "-o", exe]
    r = run(cmd, cwd=workdir, env=env)
    res = {"stage": "driver", "run": r, "runtime": runtime}
    if r["rc"] != 0:
        res["result"] = "compile_error"
        res["reason"] = r["stderr"].strip()[-800:]
        return res
    err = r["stderr"]
    pat = re.compile(r"note: '%s' (is offloaded: (\d+) systolic tile invocation\(s\)[^\n]*"
                     r"|stays on the host: ([^\n]*)"
                     r"|is outside the supported C subset and is not translated: ([^\n]*))" % re.escape(kernel))
    m = pat.search(err)
    if not m:
        res["result"] = "no_note_for_kernel"
        res["reason"] = err.strip()[-500:]
    elif m.group(2) is not None:
        res["result"] = "offloaded"
        res["planned_invocations"] = int(m.group(2))
        mm = re.search(r"; (\d+) invocation\(s\) per call", m.group(1))
        res["planned_per_call"] = int(mm.group(1)) if mm else int(m.group(2))
    elif m.group(3) is not None:
        res["result"] = "stays_on_host"
        res["reason"] = m.group(3).strip()
    else:
        res["result"] = "not_translated"
        res["reason"] = m.group(4).strip()
    res["exe"] = exe
    m = re.search(r"wrote .* \((\d+) function\(s\) offloaded, runtime: ([^;]*);", err)
    if m:
        res["functions_offloaded"] = int(m.group(1))
        res["runtime_description"] = m.group(2)
    return res


def stage_execute(exe, workdir, env=None, timeout=1800):
    r = run([exe], cwd=workdir, env=env, timeout=timeout)
    res = {"stage": "execute", "run": r}
    out = r["stdout"] + "\n" + r["stderr"]
    m = re.search(r"^RESULT (\S+) (PASS|FAIL)(.*)$", r["stdout"], re.M)
    if r.get("timeout"):
        res["result"] = "timeout"
    elif m:
        res["result"] = "pass" if m.group(2) == "PASS" else "fail"
        if "test bug" in m.group(3):
            res["result"] = "test_bug"
    elif r["rc"] == 4 or "NUMERICAL MISMATCH" in out:
        res["result"] = "verify_mismatch"
    else:
        res["result"] = "crashed" if r["rc"] not in (0, 1, 3) else "no_result_line"
    res["rc"] = r["rc"]
    m = re.search(r"invocations: (\d+) \(backend (.*), hardware=(\d), failed=(\d+)\)", r["stdout"])
    if m:
        res["invocations"] = int(m.group(1))
        res["backend"] = m.group(2)
        res["hardware"] = int(m.group(3))
        res["failed_invocations"] = int(m.group(4))
    res["verified_lines"] = re.findall(r"verified: .*", out)
    res["mismatch_lines"] = re.findall(r".*MISMATCH.*", out)
    res["gemm_labels"] = re.findall(r"\[systolic-cc\] (\w+) Executed: GEMM #\d+", r["stdout"])
    m = re.search(r"kernel: (\d+) element\(s\) differ from the reference in the sign of zero only.*?, (\d+) the other way",
                  r["stdout"])
    if m:
        res["sign_of_zero_only"] = int(m.group(1))
        res["sign_of_zero_wrong_way"] = int(m.group(2))
    m = re.search(r"first mismatch at flat element (\d+): got (\S+), reference (\S+)", r["stdout"])
    if m:
        res["first_mismatch"] = {"element": int(m.group(1)), "got": m.group(2), "reference": m.group(3)}
    return res


def stage_cpu(src, workdir, cc="cc"):
    """The program compiled by an ordinary C compiler: the kernel as written
    against its own references.  A failure here is a bug in the test."""
    exe = os.path.join(workdir, "cpu")
    r = run([cc, "-std=c11", "-O1", "-Wall", "-Wextra", src, "-o", exe], cwd=workdir)
    res = {"stage": "cpu", "run": r}
    if r["rc"] != 0:
        res["result"] = "compile_error"
        res["reason"] = r["stderr"].strip()[-500:]
        return res
    ex = stage_execute(exe, workdir)
    res["result"] = ex["result"]
    res["execute"] = ex
    return res


ALL_STAGES = ("cpu", "frontend", "discovery", "lowering", "driver", "execute")


def run_case(tools, src, kernel, workdir, runtime="soft", k8=16, k4=16, env=None, cpu_cc="cc",
             with_cpu=True, extra_cc_flags=(), only=None):
    """Every stage for one program (or the subset `only`, a collection of
    stage names).  Returns the dict of stage results; a stage not run is
    absent, with its reason under out['not_run']."""
    os.makedirs(workdir, exist_ok=True)
    only = set(only) if only else set(ALL_STAGES)
    out = {"source": src, "kernel": kernel, "workdir": workdir, "not_run": {}}
    for st in ALL_STAGES:
        if st not in only:
            out["not_run"][st] = "not selected"
    if src.endswith(".mlir"):
        # An MLIR-level case: the affine nest itself.  Only the recognizer,
        # the dense partition and the lowering pass apply; there is no C
        # program to compile with cc or systolic-cc, nothing to execute, and
        # the result says so instead of claiming those stages.
        out["kind"] = "mlir"
        for st in ("cpu", "frontend", "driver", "execute"):
            out["not_run"][st] = "MLIR-level case: no C program (recognition, dense partition and lowering only)"
        if "discovery" in only:
            out["discovery"] = stage_discovery(tools, src, workdir, k8, k4)
        if "lowering" in only:
            out["lowering"] = stage_lowering(tools, src, workdir, k8, k4)
        return out
    out["kind"] = "c"
    if with_cpu and "cpu" in only:
        out["cpu"] = stage_cpu(src, workdir, cpu_cc)
    if "frontend" not in only:
        return out
    fe = stage_frontend(tools, src, kernel, workdir)
    out["frontend"] = fe
    if fe["result"] == "translated":
        if "discovery" in only:
            out["discovery"] = stage_discovery(tools, fe["mlir"], workdir, k8, k4)
        if "lowering" in only:
            out["lowering"] = stage_lowering(tools, fe["mlir"], workdir, k8, k4)
    else:
        for st in ("discovery", "lowering"):
            if st in only:
                out["not_run"][st] = "the front end did not translate the kernel"
    if "driver" not in only:
        return out
    drv = stage_driver(tools, src, kernel, workdir, runtime=runtime, extra_flags=extra_cc_flags, env=env)
    out["driver"] = drv
    if "execute" in only and "exe" in drv:
        out["execute"] = stage_execute(drv["exe"], workdir, env=env)
    elif "execute" in only:
        out["not_run"]["execute"] = "no executable (driver: %s)" % drv.get("result")
    return out


def summarize(stages):
    """One line per stage, for logs."""
    parts = []
    for key in ("cpu", "frontend", "discovery", "lowering", "driver", "execute"):
        s = stages.get(key)
        if not s:
            parts.append("%s=-" % key)
            continue
        if key == "discovery":
            parts.append("discovery=%s/%s" % (s.get("recognition"), s.get("dense")))
        else:
            parts.append("%s=%s" % (key, s.get("result")))
    return " ".join(parts)


def strip_runs(stages, keep_output=False):
    """A copy without the full tool output (for compact reports)."""
    def strip(d):
        if isinstance(d, dict):
            out = {}
            for k, v in d.items():
                if k == "run" and not keep_output:
                    out[k] = {"cmd": v.get("cmd"), "rc": v.get("rc"), "seconds": v.get("seconds")}
                    if v.get("rc") not in (0,):
                        out[k]["stderr_tail"] = (v.get("stderr") or "")[-600:]
                else:
                    out[k] = strip(v)
            return out
        return d
    return strip(stages)


if __name__ == "__main__":
    # a quick manual check: python3 stages.py REPO FILE.c KERNEL [soft|board]
    repo, src, kernel = sys.argv[1:4]
    runtime = sys.argv[4] if len(sys.argv) > 4 else "soft"
    t = Tools(repo)
    print(json.dumps(strip_runs(run_case(t, os.path.abspath(src), kernel,
                                         os.path.join(os.getcwd(), "stages-out"), runtime)), indent=1))
