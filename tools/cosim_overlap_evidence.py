#!/usr/bin/env python3
"""overlap_evidence.py --cosim cosim.log [--trace host_trace.log] [--depth D] [--json out.json]

Evidence of the two integrated behaviours, from one co-simulation run:

  RTL side (cosim.log, COSIM_JOB lines written by test/e2e/cosim/tb_dpti_cosim.sv):
    per device, the jobs in fold order with their fire/fold/feedend/cv/done/rb
    cycles; for every consecutive pair (T, T+1) on one device whether
      fold(T+1) <  cv(T)      compute of T+1 began before T's reduction ended
      fold(T+1) >= feedend(T) compute of T+1 began after T's feed ended
                              (T had entered its drain / reduction)
      fold(T+1) <  done(T)    compute of T+1 began before T was written back
      fold(T+1) <  rb(T)      ... before T was read back
    and the queue occupancy seen from the stamps (jobs fired and not done).

  Host side (the runtime's SYSTOLIC_RUNTIME_TRACE=1 lines, "@<us>" stamped):
    per device the descriptors in flight over time (max), and for every GEMM
    the time of its last descriptor against its first completion.
"""
import argparse, collections, json, re, sys

JOB_RE = re.compile(r"COSIM_JOB ev=(\w+) cyc=(\d+) dev=(\d+) job=(\d+)")
TR_RE = re.compile(r"\[fpga-async\] (\w+) (.*) @(\d+)$")

SESS_RE = re.compile(r"cosim: session (\d+) (opened|closed)")

def parse_cosim(path, session=0):
    ev = collections.defaultdict(dict)          # (dev, job) -> {ev: cyc}
    order = collections.defaultdict(list)       # dev -> jobs in fold order
    cur = 0
    for line in open(path, errors="replace"):
        ms = SESS_RE.search(line)
        if ms:
            cur = int(ms.group(1)) if ms.group(2) == "opened" else 0
            continue
        if session and cur != session:
            continue
        m = JOB_RE.search(line)
        if not m:
            continue
        e, cyc, dev, job = m.group(1), int(m.group(2)), int(m.group(3)), int(m.group(4))
        key = (dev, job)
        if e == "fold" and "fold" not in ev[key]:
            order[dev].append(job)
        ev[key].setdefault(e, cyc)
    return ev, order

def analyse_cosim(ev, order):
    out = {"devices": {}, "pairs_total": 0, "fold_before_cv": 0, "fold_after_feedend": 0,
           "fold_before_done": 0, "fold_before_rb": 0, "pairs_bad": []}
    for dev, jobs in sorted(order.items()):
        rows = []
        for j in jobs:
            e = ev[(dev, j)]
            rows.append({"job": j, **{k: e.get(k) for k in ("fire", "fold", "feedend", "cv", "done", "rb", "rbdone")}})
        d = {"jobs": len(jobs), "rows": rows, "pairs": 0, "fold_before_cv": 0,
             "fold_after_feedend": 0, "fold_before_done": 0, "fold_before_rb": 0,
             "max_in_queue": 0}
        for a, b in zip(rows, rows[1:]):
            if a["cv"] is None or b["fold"] is None or a["feedend"] is None:
                continue
            d["pairs"] += 1
            if b["fold"] < a["cv"]:
                d["fold_before_cv"] += 1
            if b["fold"] >= a["feedend"]:
                d["fold_after_feedend"] += 1
            else:
                out["pairs_bad"].append((dev, a["job"], b["job"], "fold(T+1) < feedend(T)"))
            if a["done"] is not None and b["fold"] < a["done"]:
                d["fold_before_done"] += 1
            if a.get("rbdone") is not None and b["fold"] < a["rbdone"]:
                d["fold_before_rb"] += 1
            if a.get("fill") is not None and b.get("fill") is not None and b["fill"] < a["cv"]:
                d["fill_before_cv"] = d.get("fill_before_cv", 0) + 1
        # queue occupancy from fire/done stamps
        events = []
        for r in rows:
            if r["fire"] is not None: events.append((r["fire"], 1))
            if r["done"] is not None: events.append((r["done"], -1))
        occ = 0
        for _, delta in sorted(events):
            occ += delta
            d["max_in_queue"] = max(d["max_in_queue"], occ)
        for k in ("pairs", "fold_before_cv", "fold_after_feedend", "fold_before_done", "fold_before_rb"):
            out[k if k != "pairs" else "pairs_total"] += d[k]
        # cycles the array of this device was computing (fold .. cv), and the
        # device's span from its first fire to its last readback
        d["compute_cycles"] = sum(r["cv"] - r["fold"] for r in rows if r["cv"] is not None and r["fold"] is not None)
        fires = [r["fire"] for r in rows if r["fire"] is not None]
        ends = [r.get("rbdone") or r.get("done") for r in rows if (r.get("rbdone") or r.get("done")) is not None]
        d["first_fire"] = min(fires) if fires else None
        d["last_end"] = max(ends) if ends else None
        out["devices"][dev] = d
    fires = [d["first_fire"] for d in out["devices"].values() if d["first_fire"] is not None]
    ends = [d["last_end"] for d in out["devices"].values() if d["last_end"] is not None]
    if fires and ends:
        out["makespan_cycles"] = max(ends) - min(fires)
        out["compute_cycles_total"] = sum(d["compute_cycles"] for d in out["devices"].values())
    return out

def parse_trace(path):
    desc = []       # (us, dev, job)
    compl = []      # (us, dev, job, status)
    gemm_of = {}    # job -> gemm
    begins, waits = {}, {}
    for line in open(path, errors="replace"):
        m = TR_RE.search(line)
        if not m:
            continue
        kind, rest, us = m.group(1), m.group(2), int(m.group(3))
        if kind == "submit":
            j = int(re.search(r"job=0x([0-9a-f]+)", rest).group(1), 16)
            g = int(re.search(r"gemm=(\d+)", rest).group(1))
            gemm_of[j] = g
        elif kind == "descriptor":
            j = int(re.search(r"job=0x([0-9a-f]+)", rest).group(1), 16)
            d = int(re.search(r"device=(\d+)", rest).group(1))
            desc.append((us, d, j))
        elif kind == "completion":
            j = int(re.search(r"job=0x([0-9a-f]+)", rest).group(1), 16)
            d = int(re.search(r"device=(\d+)", rest).group(1))
            st = int(re.search(r"status=(\d+)", rest).group(1))
            compl.append((us, d, j, st))
        elif kind == "gemm":
            mm = re.match(r"(\d+) (begin|wait)", rest)
            if mm:
                (begins if mm.group(2) == "begin" else waits)[int(mm.group(1))] = us
    return desc, compl, gemm_of, begins, waits

def analyse_trace(desc, compl, gemm_of, begins, waits, depth):
    out = {"descriptors": len(desc), "completions": len(compl), "max_inflight": {}, "gemms": {}}
    inflight = collections.Counter(); mx = collections.Counter()
    events = sorted([(us, 1, d, j) for us, d, j in desc] + [(us, -1, d, j) for us, d, j, _ in compl])
    for us, delta, d, j in events:
        inflight[d] += delta
        mx[d] = max(mx[d], inflight[d])
    out["max_inflight"] = dict(sorted(mx.items()))
    out["inflight_within_depth"] = all(v <= depth for v in mx.values()) if depth else None
    by_gemm = collections.defaultdict(lambda: {"descriptors": [], "completions": []})
    for us, d, j in desc:
        by_gemm[gemm_of.get(j, -1)]["descriptors"].append(us)
    for us, d, j, st in compl:
        by_gemm[gemm_of.get(j, -1)]["completions"].append(us)
    for g, v in sorted(by_gemm.items()):
        ds, cs = sorted(v["descriptors"]), sorted(v["completions"])
        first_c = cs[0] if cs else None
        before = sum(1 for x in ds if first_c is None or x < first_c)
        out["gemms"][g] = {"tiles": len(ds), "descriptors_before_first_completion": before,
                           "all_submitted_before_first_collection": bool(ds) and before == len(ds),
                           "last_descriptor_us": ds[-1] if ds else None, "first_completion_us": first_c,
                           "begin_us": begins.get(g), "wait_us": waits.get(g)}
    return out

def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("--cosim"); ap.add_argument("--trace"); ap.add_argument("--depth", type=int, default=0)
    ap.add_argument("--session", type=int, default=0, help="only the COSIM_JOB lines of this cosim session (1-based)")
    ap.add_argument("--json")
    a = ap.parse_args()
    report = {}
    if a.cosim:
        ev, order = parse_cosim(a.cosim, a.session)
        r = analyse_cosim(ev, order)
        report["rtl"] = r
        print(f"RTL stamps ({a.cosim}): {sum(d['jobs'] for d in r['devices'].values())} jobs, "
              f"{r['pairs_total']} consecutive pairs on one device")
        for dev, d in r["devices"].items():
            print(f"  dev {dev}: jobs={d['jobs']} pairs={d['pairs']} fold(T+1)<cv(T)={d['fold_before_cv']} "
                  f"fold(T+1)>=feedend(T)={d['fold_after_feedend']} fold(T+1)<done(T)={d['fold_before_done']} "
                  f"fold(T+1)<rbdone(T)={d['fold_before_rb']} fill(T+1)<cv(T)={d.get('fill_before_cv', 0)} max_jobs_queued={d['max_in_queue']}")
        if "makespan_cycles" in r:
            print(f"  makespan (first fire -> last readback/done) = {r['makespan_cycles']} array cycles; "
                  f"array compute (fold->cv) summed over devices = {r['compute_cycles_total']} cycles")
        if r["pairs_bad"]:
            print("  VIOLATION:", r["pairs_bad"][:5])
    if a.trace:
        t = analyse_trace(*parse_trace(a.trace), a.depth)
        report["host"] = t
        print(f"host trace ({a.trace}): descriptors={t['descriptors']} completions={t['completions']} "
              f"max_inflight_per_device={t['max_inflight']} within_depth={t['inflight_within_depth']}")
        for g, v in t["gemms"].items():
            print(f"  gemm {g}: tiles={v['tiles']} descriptors_before_first_completion={v['descriptors_before_first_completion']} "
                  f"all_submitted_first={v['all_submitted_before_first_collection']} "
                  f"last_descriptor_us={v['last_descriptor_us']} first_completion_us={v['first_completion_us']}")
    if a.json:
        json.dump(report, open(a.json, "w"), indent=1, default=str)
    bad = bool(report.get("rtl", {}).get("pairs_bad"))
    if a.trace and a.depth and not report["host"]["inflight_within_depth"]:
        bad = True
    sys.exit(1 if bad else 0)

if __name__ == "__main__":
    main()
