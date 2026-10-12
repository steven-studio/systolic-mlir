#!/usr/bin/env python3
"""check_variable_k.py -- did every FPGA invocation of a co-simulated
run_e2e.sh pass run exactly the reduction depth the compiler gave it?

For each case directory of a run_e2e.sh --backend cosim output (run with
SYSTOLIC_RUNTIME_TRACE=1, on a co-simulation built from the split-K_MAX RTL,
whose bench prints COSIM_STEPS per job), three records of the same
invocations, in program order, are compared:

  compiler  ir/10-systolic.mlir: systolic.matmul_tile ... k = .. on @dev
            est_cycles = .. start_cycle = ..
  runtime   output.txt, SYSTOLIC_RUNTIME_TRACE: "[fpga-runtime] job .. ->
            device D (PxP array, K_MAX k): tile MxN, job.k=K, start_cycle=S
            est_cycles=E, ..." -- the descriptor it sent
  RTL       cosim.log, the case's DPTI session: "COSIM_STEPS dev=D job_k=K
            steps_pe_min=.. steps_pe_max=.. feed_a=.. feed_b=.. k_dim=.."
            -- the descriptor's k as the top accepted it, and the reduction
            steps counted in every PE (tb/reduction_probe.svh)

and every invocation must agree: the IR's k = the runtime's job.k = the k the
RTL accepted = the steps every PE executed = the operands fed in = k_dim, on
the same device, with 1 <= k <= K_MAX of that device; and the IR's
start_cycle / est_cycles = the runtime's.

With --cost (build/bin/systolic-cost), each invocation's FPGA-internal
cycles in the co-simulation -- descriptor accepted (fire) to the result tile
in DDR (done), the phases of eval/cost_model_validation -- are also compared
with the cost model's read + compute + write for that k. The runtime issues
one invocation at a time, so nothing contends; host time (the DPTI transfers,
the runtime's 1 ms wait) lies between invocations and is never compared.

  python3 test/e2e/check_variable_k.py E2E_OUT [E2E_OUT ...] [--cost TOOL] [--report FILE]

E2E_OUT is a run_e2e.sh --out directory (its summary.txt names each case's
session). Exit status 0 only if every invocation of every case agrees.
"""

import argparse
import functools
import re
import subprocess
import sys
from pathlib import Path

DEVICE_ID = {"acc_8x8_0": 0, "acc_4x4_0": 1, "acc_4x4_1": 2, "acc_4x4_2": 3}
TILE_RE = re.compile(r"systolic\.matmul_tile .* m = (\d+) n = (\d+) k = (\d+) on @(\w+) "
                     r"est_cycles = (\d+) start_cycle = (\d+)")
TRACE_RE = re.compile(r"\[fpga-runtime\] job (\d+) -> device (\d+) \((\d+)x\d+ array, K_MAX (\d+)\): "
                      r"tile (\d+)x(\d+), job\.k=(\d+), start_cycle=(\d+) est_cycles=(\d+)")
SUMMARY_RE = re.compile(r"^(\S+)\s+(PASS|FAIL)\s.*\(session (\d+)\)")
SESSION_RE = re.compile(r"^cosim: session (\d+) (opened|closed)")


def cosim_sessions(log):
    """session number -> list of COSIM_STEPS dicts, in order."""
    sessions, current = {}, None
    for line in log.read_text(errors="replace").splitlines():
        m = SESSION_RE.match(line)
        if m:
            n = int(m.group(1))
            if m.group(2) == "opened":
                current = n
                sessions.setdefault(n, [])
            elif current == n:
                current = None
            continue
        if line.startswith("COSIM_STEPS ") and current is not None:
            f = dict(kv.split("=") for kv in line.split()[1:])
            sessions[current].append({k: int(v) for k, v in f.items()})
    return sessions


@functools.lru_cache(maxsize=None)
def model_total(tool, geometry, kmax, k):
    """read + compute + write of one invocation of depth k (systolic-cost)."""
    r = subprocess.run([tool, "invocation", "--geometry", str(geometry), "--k-max", str(kmax), "--k", str(k)],
                       capture_output=True, text=True, check=True)
    m = re.search(r"^invocation j=0 k=(\d+) .* total=(\d+)$", r.stdout, re.M)
    assert m and int(m.group(1)) == k
    return int(m.group(2))


def check_run(out, tool=None):
    log = out / "cosim.log"
    summary = out / "summary.txt"
    if not log.exists() or not summary.exists():
        return [], [f"{out}: no cosim.log or summary.txt"]
    sessions = cosim_sessions(log)
    rows, problems = [], []
    names = (out / "case_list.txt").read_text().split()
    verdicts = {}
    for line in summary.read_text().splitlines():
        m = SUMMARY_RE.match(line)
        if m:
            verdicts[m.group(1)] = (m.group(2), int(m.group(3)))
    for name in names:
        if name not in verdicts:
            problems.append(f"{out.name}/{name}: no co-simulation session in summary.txt (case failed or not run)")
            continue
        verdict, session = verdicts[name]
        d = out / name
        ir = [mt.groups() for mt in map(TILE_RE.search, (d / "ir/10-systolic.mlir").read_text().splitlines()) if mt]
        trace = [mt.groups() for mt in map(TRACE_RE.search, (d / "output.txt").read_text().splitlines()) if mt]
        rtl = sessions.get(session, [])
        bad = []
        cycles_ok, cycles_n, cycles_maxerr = 0, 0, 0
        if not (len(ir) == len(trace) == len(rtl)):
            bad.append(f"{len(ir)} IR tiles, {len(trace)} traced jobs, {len(rtl)} RTL jobs")
        for i, (t, r, s) in enumerate(zip(ir, trace, rtl)):
            m_, n_, k, dev, est, start = int(t[0]), int(t[1]), int(t[2]), DEVICE_ID[t[3]], int(t[4]), int(t[5])
            job_id, rdev, p, kmax, tm, tn, jk, rstart, rest = map(int, r)
            ok = (k == jk == s["job_k"] == s["steps_pe_min"] == s["steps_pe_max"] == s["feed_a"] ==
                  s["feed_b"] == s["k_dim"]) and dev == rdev == s["dev"] and 1 <= jk <= kmax and \
                (m_, n_) == (tm, tn) and (start, est) == (rstart, rest)
            if tool:
                pred = model_total(tool, p, kmax, jk)
                err = (s["done"] - s["fire"]) - pred
                cycles_n += 1
                cycles_ok += err == 0
                cycles_maxerr = max(cycles_maxerr, abs(err))
            if not ok:
                bad.append(f"invocation {i}: IR k={k} dev={dev} start/est={start}/{est}; runtime job.k={jk} "
                           f"dev={rdev} K_MAX={kmax} start/est={rstart}/{rest}; RTL job_k={s['job_k']} "
                           f"steps/PE={s['steps_pe_min']}..{s['steps_pe_max']} feed={s['feed_a']}/{s['feed_b']} "
                           f"k_dim={s['k_dim']} dev={s['dev']}")
        kmaxes = sorted({(int(r[1]), int(r[3])) for r in trace})
        folded = sum(1 for r in trace if int(r[6]) < int(r[3]))
        rows.append({"run": out.name, "case": name, "verdict": verdict, "session": session,
                     "invocations": len(rtl), "ks": ",".join(str(s["job_k"]) for s in rtl),
                     "below_kmax": folded, "kmax": kmaxes, "agree": not bad,
                     "cycles_n": cycles_n, "cycles_ok": cycles_ok, "cycles_maxerr": cycles_maxerr})
        problems += [f"{out.name}/{name}: {b}" for b in bad]
    return rows, problems


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("outs", nargs="+")
    ap.add_argument("--cost", help="build/bin/systolic-cost: also compare each invocation's cycles with the model")
    ap.add_argument("--report")
    a = ap.parse_args()
    all_rows, all_problems = [], []
    for o in a.outs:
        rows, problems = check_run(Path(o), a.cost)
        all_rows += rows
        all_problems += problems
    n_inv = sum(r["invocations"] for r in all_rows)
    n_below = sum(r["below_kmax"] for r in all_rows)
    lines = [f"{len(all_rows)} case(s), {n_inv} invocation(s), {n_below} of them with job.k < K_MAX; "
             f"{sum(1 for r in all_rows if r['agree'])} case(s) where IR k = runtime job.k = RTL k = steps "
             f"executed by every PE, for every invocation"]
    if a.cost:
        n = sum(r["cycles_n"] for r in all_rows)
        ok = sum(r["cycles_ok"] for r in all_rows)
        worst = max((r["cycles_maxerr"] for r in all_rows), default=0)
        lines.append(f"FPGA-internal cycles (descriptor accepted -> result tile in DDR) = the cost model's "
                     f"read + compute + write for {ok} of {n} invocation(s); largest |error| {worst} cycles")
    lines.append("")
    lines.append(f"{'run':24s} {'case':22s} {'e2e':5s} {'inv':>4s} {'<K_MAX':>6s} agree  job.k sequence")
    for r in all_rows:
        ks = r["ks"] if len(r["ks"]) <= 60 else r["ks"][:57] + "..."
        lines.append(f"{r['run']:24s} {r['case']:22s} {r['verdict']:5s} {r['invocations']:4d} {r['below_kmax']:6d} "
                     f"{'yes' if r['agree'] else 'NO':5s}  {ks}")
    if all_problems:
        lines.append("")
        lines += ["PROBLEM " + p for p in all_problems]
    text = "\n".join(lines) + "\n"
    print(text, end="")
    if a.report:
        Path(a.report).write_text(text)
    return 0 if all_rows and not all_problems and all(r["verdict"] == "PASS" for r in all_rows) else 1


if __name__ == "__main__":
    sys.exit(main())
