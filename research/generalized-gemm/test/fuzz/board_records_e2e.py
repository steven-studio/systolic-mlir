#!/usr/bin/env python3
"""Generalized GEMM records mode (formulas / tables) on the real FPGA board.

Reuses the research E2E's kept work (nest_*_e2e.py --keep): the `driven` variant's module was
lowered with --affine-matmul-to-systolic="nest-records=... record-driven=true", then
--lower-systolic-matmul-tile-to-fpga (sync / async=true), --systolic-runtime-to-llvm,
mlir-translate, llc -> 70-{sync,async}.o. Nothing is recompiled here. The harness:

  1. copies driver.c (the research driver: data, C reference, per-cell comparison) and keeps only
     the selected functions in its main, then calls board_probe() (stats + close) before return;
  2. links 70-<mode>.o with the REAL runtime exactly as systolic-cc --runtime=board does
     (board_stats.c + libsystolic_fpga_runtime.a + libftdi1), not soft_runtime.c;
  3. checks ldd (system libftdi1, no libftdi_cosim), runs with SYSTOLIC_FPGA_CONFIG of the
     programmed image, and checks: exit 0, no BAD line, every RESULT with failed=0 and the
     invocations the pass's remarks announce, is_hardware=1, FPGA invocations = the sum of the
     RESULT invocations, bytes on the DPTI link.

The lowered kernels have no CPU fallback: a runtime failure is counted in `failed`, never hidden.
is_hardware is a link-time constant of board_stats.c; the evidence that the FPGA computed is the
completions and the DPTI bytes the runtime counts at the FTDI layer. Makespans and output blocks
are the pass's plan (its remarks), not a measurement.

Not a CTest: it needs the programmed board (REPRODUCE.md, section 18). Usage:

  for t in nest_driven_e2e nest_scf_e2e nest_while_e2e nest_carried_e2e; do
    python3 research/generalized-gemm/test/fuzz/$t.py <the CTest's arguments> --keep WORK/work-$t
  done
  then, with the board programmed:
  python3 research/generalized-gemm/test/fuzz/board_records_e2e.py --research-work WORK \
    --repo . --build BUILD --clang CLANG --llvm-lib LLVM_BUILD/lib --config FPGA_CONFIG --out NEW_DIR
"""
import argparse, json, os, re, subprocess, sys
from pathlib import Path

SELECTED = [  # (suite, function)
    ("nest_driven_e2e", "drv_formulas"), ("nest_driven_e2e", "drv_tables_flat"),
    ("nest_scf_e2e", "scf_gemm"), ("nest_scf_e2e", "scf_tables_perm"), ("nest_scf_e2e", "scf_chain"),
    ("nest_while_e2e", "whl_ne_bigger"), ("nest_while_e2e", "whl_chain"), ("nest_while_e2e", "whl_tables_flat"),
    ("nest_carried_e2e", "car_iter"), ("nest_carried_e2e", "car_tables_while"),
]
REMARK = re.compile(r"remark: GEMM (\d+x\d+x\d+) of nest (\d+) of (\w+) lowered from its record as (\d+) exact "
                    r"region\(s\) \((formulas|tables): .*?\) on \d+ accelerator\(s\): makespan = (\d+) cycles "
                    r"\(read \+ compute \+ write\), (\d+) output block\(s\)")
RESULT = re.compile(r"RESULT (\w+) exact=(\d) invocations=(\d+) failed=(\d+) cells=(\d+) same=(\d+) differing=(\d+) worst=([\d.]+)")
PROBE = re.compile(r"PROBE backend=(.*) is_hardware=(\d) dev=(\d+),(\d+),(\d+),(\d+) failed=(\d+) first_error=(-?\d+) to=(\d+) from=(\d+)")
PROBE_C = r'''
static void board_probe(void) {
  systolic_offload_stats_t s; systolic_offload_stats(&s);
  printf("PROBE backend=%s is_hardware=%d dev=%llu,%llu,%llu,%llu failed=%llu first_error=%d to=%llu from=%llu\n",
         s.backend ? s.backend : "?", s.is_hardware, s.per_device[0], s.per_device[1], s.per_device[2],
         s.per_device[3], s.failed, s.first_error, s.bytes_to_fpga, s.bytes_from_fpga);
  fflush(stdout);
  systolic_offload_close();
}
'''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--research-work", required=True, help="dir holding work-<suite>/driven from --keep")
    ap.add_argument("--repo", required=True)
    ap.add_argument("--build", required=True, help="production build with runtime/libsystolic_fpga_runtime.a")
    ap.add_argument("--clang", required=True)
    ap.add_argument("--llvm-lib", required=True)
    ap.add_argument("--config", required=True, help="fpga_config.txt of the programmed image")
    ap.add_argument("--out", required=True, help="a new directory")
    ap.add_argument("--cases", default="", help="comma-separated functions of SELECTED (default: all)")
    a = ap.parse_args()
    out = Path(a.out); out.mkdir(parents=True, exist_ok=False)
    offload = Path(a.repo) / "runtime/offload"
    rows, failures = [], []
    chosen = [c for c in SELECTED if not a.cases or c[1] in a.cases.split(",")]
    unknown = set(filter(None, a.cases.split(","))) - {f for _, f in SELECTED}
    if unknown or not chosen:
        sys.exit(f"--cases: unknown {sorted(unknown)} (known: {[f for _, f in SELECTED]})")
    suites = sorted({s for s, _ in chosen}, key=[s for s, _ in chosen].index)
    for suite in suites:
        src = Path(a.research_work) / f"work-{suite}" / "driven"
        names = [f for s, f in chosen if s == suite]
        plan = {}  # function -> list of (shape, nest, regions, path, makespan, blocks)
        for m in REMARK.finditer((src / "remarks.txt").read_text()):
            shape, nest, fn, regions, path, ms, blk = m.groups()
            plan.setdefault(fn, []).append((shape, int(nest), int(regions), path, int(ms), int(blk)))
        for fn in names:
            if fn not in plan:
                failures.append(f"{suite} {fn}: no record-lowered remark in {src}/remarks.txt")
        driver = (src / "driver.c").read_text()
        head, _, _ = driver.partition("int main(void) {")
        body = "".join(f"  run_{fn}(1);\n  run_{fn}(0);\n" for fn in names)
        driver = head + PROBE_C + "int main(void) {\n" + body + "  board_probe();\n  return bad ? 1 : 0;\n}\n"
        for mode in ("sync", "async"):
            d = out / f"{suite}-{mode}"; d.mkdir()
            (d / "driver.c").write_text(driver)
            exe = d / "program"
            link = [a.clang, "-std=c11", "-O1", "-ffp-contract=off", "-Wno-unused-function", f"-I{offload}",
                    f"-I{Path(a.repo) / 'runtime'}", str(d / "driver.c"), str(src / f"70-{mode}.o"),
                    str(offload / "systolic_cc_report.c"), str(offload / "board_stats.c"),
                    str(Path(a.build) / "runtime/libsystolic_fpga_runtime.a"), "-lftdi1", "-lusb-1.0", "-lstdc++",
                    f"-L{a.llvm_lib}", "-lmlir_c_runner_utils", f"-Wl,-rpath,{a.llvm_lib}", "-lm", "-o", str(exe)]
            (d / "link.txt").write_text(" ".join(link) + "\n")
            r = subprocess.run(link, capture_output=True, text=True)
            if r.returncode:
                failures.append(f"{suite} {mode}: link failed:\n{r.stderr[-2000:]}"); continue
            ldd = subprocess.run(["ldd", str(exe)], capture_output=True, text=True).stdout
            (d / "ldd.txt").write_text(ldd)
            if "libftdi1.so" not in ldd or "cosim" in ldd:
                failures.append(f"{suite} {mode}: not linked to the system libftdi1:\n{ldd}")
            env = {"HOME": os.environ["HOME"], "PATH": "/usr/local/bin:/usr/bin:/bin",
                   "SYSTOLIC_FPGA_CONFIG": a.config, "SYSTOLIC_RUNTIME_TIMING": "1"}
            run = subprocess.run([str(exe)], cwd=d, env=env, capture_output=True, text=True, timeout=600)
            (d / "run.txt").write_text(run.stdout + run.stderr)
            bad = [l for l in run.stdout.splitlines() if l.startswith("BAD")]
            probe = PROBE.search(run.stdout)
            if run.returncode or bad:
                failures.append(f"{suite} {mode}: exit {run.returncode}, {len(bad)} BAD line(s): {bad[:3]}")
            if not probe:
                failures.append(f"{suite} {mode}: no PROBE line"); continue
            backend, hw = probe.group(1), int(probe.group(2))
            dev = [int(probe.group(i)) for i in range(3, 7)]
            total_inv = 0
            for m in RESULT.finditer(run.stdout):
                fn, exact, inv, failed, cells, same, differing, worst = m.groups()
                inv, failed = int(inv), int(failed); total_inv += inv
                announced = sum(p[2] for p in plan.get(fn, []))
                if inv != announced or failed:
                    failures.append(f"{suite} {mode} {fn}: {inv} invocations, {failed} failed; remarks announce {announced}")
                fn_bad = sum(1 for l in bad if f" {fn} " in l or l.startswith(f"BAD {fn} "))
                rows.append(dict(suite=suite, mode=mode, function=fn, exact=int(exact), invocations=inv, failed=failed,
                                 cells=int(cells), same=int(same), differing=int(differing), worst=float(worst),
                                 bad=fn_bad, plan=plan.get(fn, []), is_hardware=hw, backend=backend))
            if hw != 1 or sum(dev) != total_inv or int(probe.group(7)) or int(probe.group(9)) == 0 or int(probe.group(10)) == 0:
                failures.append(f"{suite} {mode}: probe {probe.group(0)} vs {total_inv} invocations")
            timing = [l for l in (run.stdout + run.stderr).splitlines() if "timing" in l]
            (d / "probe.json").write_text(json.dumps(dict(backend=backend, is_hardware=hw, per_device=dev,
                                                          failed=int(probe.group(7)), bytes_to=int(probe.group(9)),
                                                          bytes_from=int(probe.group(10)), timing=timing), indent=1))
    # Two data sets x two modes per function: fewer RESULT lines means the check is vacuous.
    if len(rows) != 4 * len(chosen):
        failures.append(f"{len(rows)} RESULT lines for {len(chosen)} functions, expected {4 * len(chosen)}")
    (out / "results.json").write_text(json.dumps(dict(rows=rows, failures=failures), indent=1))
    for r in rows:
        print(f"{r['suite']:17s} {r['mode']:5s} {r['function']:18s} exact={r['exact']} inv={r['invocations']:2d} "
              f"failed={r['failed']} cells={r['cells']} same={r['same']} differing={r['differing']} bad={r['bad']} "
              f"hw={r['is_hardware']}")
    for f in failures:
        print("FAIL:", f, file=sys.stderr)
    print(f"{len(rows)} function runs, {len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
