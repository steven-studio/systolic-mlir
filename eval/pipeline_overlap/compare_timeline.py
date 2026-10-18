#!/usr/bin/env python3
"""compare_timeline.py -- hold the cost model's per-accelerator timeline
(systolic-cost timeline, lib/Systolic/Transforms/CostModel.cpp
AcceleratorTimeline) against the RTL bench's stamps (tb_pipeline_overlap.sv,
run_pipeline_sim.sh), job by job, to the cycle.

    compare_timeline.py --cost build/bin/systolic-cost --run OUT/run.log
                        [--v2] [--k8 16 --k4 16] [--pipelined | --serial]

For every device of the run, the jobs of that device in submission order
are one stream of invocations; each job's acceptance cycle (the bench's
`fire`) is given to the model as the cycle the descriptor arrived (the host's
pace and the DPTI readback, which gate acceptance, are outside the model),
and the model's fold start, result publish and write end must equal the
bench's `fold`, `cv` and `done`.  `grant` must be fill_start + 3 (the
descriptor latency inside c_read).

What the model does not have, and the comparison therefore reports instead
of failing on, are the shared engines' waits on other contexts and on the
result readback: a fill that lands behind a readback or another context's
fill ends later than T_read (`fill-stalled`), a write-back behind another
context's ends later than T_write (`write-stalled`).  Such a job is reported
with the measured delay and the comparison continues with the bench's own
stamp as the model's; the verdict counts exact jobs, stalled jobs and
mismatches separately, and a run passes when there is no mismatch.
"""
import argparse
import re
import subprocess
import sys


def parse_run(path):
    jobs = []
    for line in open(path):
        if line.startswith("PJOB "):
            d = dict(re.findall(r"(\w+)=(-?\w+)", line))
            jobs.append({k: (int(v) if re.fullmatch(r"-?\d+", v) else v) for k, v in d.items()})
    return jobs


def model(cost, geometry, kmax, depths, accepts, args, feeds=None, writes=None):
    cmd = [cost, "timeline", "--geometry", str(geometry), "--k-max", str(kmax),
           "--k", ",".join(map(str, depths)), "--accept", ",".join(map(str, accepts)),
           "--k8", str(args.k8), "--k4", str(args.k4)]
    if feeds:
        cmd += ["--feed", ",".join(map(str, feeds))]
    if writes:
        cmd += ["--write", ",".join(map(str, writes))]
    if args.pipelined:
        cmd.append("--pipelined")
    if args.v2:
        cmd += ["--read-bytes-per-cycle", "16"]
    out = subprocess.run(cmd, check=True, capture_output=True, text=True).stdout
    events = []
    for line in out.splitlines():
        if line.startswith("event "):
            d = dict(re.findall(r"(\w+)=(-?\d+)", line))
            events.append({k: int(v) for k, v in d.items()})
    return events


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cost", required=True)
    ap.add_argument("--run", required=True)
    ap.add_argument("--v2", action="store_true")
    ap.add_argument("--k8", type=int, default=16)
    ap.add_argument("--k4", type=int, default=16)
    ap.add_argument("--pipelined", action="store_true", default=True)
    ap.add_argument("--serial", dest="pipelined", action="store_false")
    ap.add_argument("--no-stalls", action="store_true",
                    help="count a stall as a mismatch (a run with no other context and no readback in flight)")
    args = ap.parse_args()

    jobs = parse_run(args.run)
    if not jobs:
        print("no PJOB lines in", args.run)
        return 2
    exact = stalled = mismatch = 0
    for dev in sorted({j["dev"] for j in jobs}):
        djobs = [j for j in jobs if j["dev"] == dev]
        geometry = 8 if dev == 0 else 4
        kmax = args.k8 if dev == 0 else args.k4
        depths = [j["kdesc"] for j in djobs]
        accepts = [j["fire"] for j in djobs]
        # Replay the stream job by job so that a fill stall (outside the
        # model) is absorbed at the job where it happened: the stalled job's
        # fold is given to the model as observed (--feed) and the model
        # continues from there.
        feeds = [0] * len(djobs)
        writes = [0] * len(djobs)
        # The context's own latencies of a job alone (tb_cost_phases, one job
        # at a time): last operand beat -> fold (the v1 writer unpacks the last
        # beat in four cycles, v2 lands it at once), publish -> write descriptor
        # granted (the result scan: P*P + 5), grant -> done (16 or 4 beats + 6).
        # A stall is only accepted when the job still shows them: a fold that
        # followed its (late) landing at once, a write descriptor granted late
        # whose write then ran no slower than alone.
        land_to_fold = 6 if args.v2 else 9
        cv_to_wbacc = 69 if geometry == 8 else 21
        wbacc_to_done = 22 if geometry == 8 else 10
        for idx in range(len(djobs)):
            ev = model(args.cost, geometry, kmax, depths, accepts, args, feeds, writes)
            j, e = djobs[idx], ev[idx]
            fold_ok = e["feed_start"] == j["fold"]
            grant_ok = e["fill_start"] + 3 == j["grant"]
            cv_ok = e["compute_end"] == j["cv"]
            done_ok = e["write_end"] == j["done"]
            notes = []
            if (grant_ok and not fold_ok and e["feed_start"] < j["fold"]
                    and j["fold"] - j["fill"] == land_to_fold and not args.no_stalls):
                # the descriptor went out on time, the operands landed later
                # than T_read allows and the fold followed the landing at
                # once: a shared read engine wait, not a fold rule; take the
                # bench's fold as given and re-check the rest.
                notes.append(f"fill-stalled +{j['fold'] - e['feed_start']} (shared read engine)")
                feeds[idx] = j["fold"]
                ev = model(args.cost, geometry, kmax, depths, accepts, args, feeds, writes)
                e = ev[idx]
                fold_ok = e["feed_start"] == j["fold"]
                cv_ok = e["compute_end"] == j["cv"]
                done_ok = e["write_end"] == j["done"]
            if (grant_ok and fold_ok and cv_ok and not done_ok and e["write_end"] < j["done"]
                    and j["wbacc"] - j["cv"] > cv_to_wbacc and j["done"] - j["wbacc"] <= wbacc_to_done
                    and not args.no_stalls):
                # the result was published on time, the write descriptor was
                # granted late and the write itself ran no slower than alone
                # (a descriptor already waiting when the engine frees is
                # taken a cycle or two faster): the shared write engine was
                # serving another context.
                notes.append(f"write-stalled +{j['done'] - e['write_end']} (shared write engine)")
                writes[idx] = j["done"]
                ev = model(args.cost, geometry, kmax, depths, accepts, args, feeds, writes)
                e = ev[idx]
                done_ok = e["write_end"] == j["done"]
            if fold_ok and cv_ok and done_ok and grant_ok:
                if notes:
                    stalled += 1
                    verdict = ", ".join(notes) + ", then exact"
                else:
                    exact += 1
                    verdict = "exact"
            else:
                mismatch += 1
                verdict = (f"MISMATCH: model grant {e['fill_start'] + 3} fold {e['feed_start']} "
                           f"cv {e['compute_end']} done {e['write_end']}")
            print(f"job {j['job']:3d} dev {dev} k={j['kdesc']:3d} fire={j['fire']} grant={j['grant']} "
                  f"fold={j['fold']} cv={j['cv']} done={j['done']}  {verdict}")
    ok = mismatch == 0 and exact > 0
    print(f"== {'PASS' if ok else 'FAIL'}: {exact} exact, {stalled} stalled on a shared engine, {mismatch} mismatch")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
