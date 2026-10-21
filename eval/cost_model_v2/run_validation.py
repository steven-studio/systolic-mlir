#!/usr/bin/env python3
"""Cost Model V2 against the RTL: the whole validation, reproducible.

    python3 eval/cost_model_v2/run_validation.py [--out DIR] [--jobs N] [STEP ...]

STEP (default: all, in order):
  build     Verilator builds of tb_fleet_jobs: K_MAX 16/16, 24/12, 32/32,
            64/64, 256/64 on the v1 operand path, 16/16 and 32/32 on v2
  cases     the case matrix (gen_cases), one job file per case
  rtl       every case on the RTL bench (v1; the 16/16 and 32/32 cases on v2)
  exact     the fleet simulator (systolic-cost fleet-sim) against every RTL
            run: every event of every job, the AXI read request count and the
            readback completions must be identical -- with the bench's own
            clock phase between the array and DPTI clocks (read from the log)
  skip      the simulator with and without its idle skip on every case, both
            port policies, both operand paths: identical output required
  credit    the compiler's prediction (systolic-cost schedule, decomposition
            order, PortPolicy::Credit) replayed on the RTL bench, every event
  report    V1 and V2 predictions against the RTL: per case, the error of the
            device makespan, V2 predicted with the DEFAULT clock phase (no
            per-case input); summary.md
  scan      the binary search's feasibility predicate at every deadline
            0..2T* (systolic-cost scan-feasible) on a problem grid, V2

Needs build/bin/systolic-cost and Verilator >= 5 on PATH. Writes only under
--out (default build/cost_model_v2).
"""
import argparse, json, math, os, random, subprocess, sys
from concurrent.futures import ThreadPoolExecutor, ProcessPoolExecutor

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TOOL = os.path.join(ROOT, 'build', 'bin', 'systolic-cost')
CONFIGS = [(16, 16), (24, 12), (32, 32), (64, 64), (256, 64)]
V2_CONFIGS = [(16, 16), (32, 32)]
BRUTE = {(32, 32), (64, 64)}
GAPS = [0, 3, 40, 300]
COMBOS = {'8x8': [0], '4x4': [1], '8x8+4x4': [0, 1], '8x8+3x4x4': [0, 1, 2, 3], '3x4x4': [1, 2, 3]}
KEYS = ('fire', 'desc', 'last', 'land', 'fold', 'cv', 'wbacc', 'done')
UI_PS, BENCH_DPTI_PS = 10000, 16666


# ----------------------------------------------------------------------------- cases
def klist(k8, k4):
    ks = {1, 7, 8, 9, 15, 16, 17}
    for km in {k8, k4}:
        ks |= {km - 1, km, km + 1, 2 * km - 1, 2 * km, 2 * km + 1}
    return sorted(k for k in ks if k >= 1)


def fold(K, km):
    out = [km] * (K // km)
    if K % km:
        out.append(K % km)
    return out


def interleave(lists):
    out, idx = [], [0] * len(lists)
    while True:
        moved = False
        for n, (dev, lst) in enumerate(lists):
            if idx[n] < len(lst):
                out.append((dev, lst[idx[n]])); idx[n] += 1; moved = True
        if not moved:
            return out


def gen_cases(out):
    manifest, rnd = [], random.Random(20261021)
    for (k8, k4) in CONFIGS:
        cfg = f'{k8}_{k4}'
        d = os.path.join(out, 'cases', cfg); os.makedirs(d, exist_ok=True)
        ks, brute = klist(k8, k4), (k8, k4) in BRUTE
        tile = lambda dv, K: fold(K, k8 if dv == 0 else k4)
        cases = []
        for K in ks:                                   # one tile, one accelerator
            for dev in (0, 1):
                cases.append((f'single_{"8x8" if dev == 0 else "4x4"}_K{K}', 'single',
                              [(dev, k, 0, -1, 0) for k in tile(dev, K)]))
        for K in ks:                                   # concurrent, submission gap
            for cname in ('8x8+4x4', '8x8+3x4x4', '3x4x4'):
                for gap in GAPS:
                    if not brute and gap == 3 and K not in (1, 16, k8, k8 + 1):
                        continue
                    seq = interleave([(dv, tile(dv, K)) for dv in COMBOS[cname]])
                    cases.append((f'combo_{cname}_K{K}_gap{gap}', 'concurrent',
                                  [(dv, k, n * gap, -1, 0) for n, (dv, k) in enumerate(seq)]))
        for K in ([7, k4, 2 * k4 + 1] if not brute else [1, 9, k4, k4 + 1, 2 * k4 + 1]):
            for cname in ('8x8+3x4x4', '3x4x4', '8x8'):  # long streams
                for T in ((6,) if not brute else (4, 10)):
                    lists = [(dv, [k for _ in range(T) for k in tile(dv, K)]) for dv in COMBOS[cname]]
                    seq = interleave(lists)
                    if len(seq) <= 600:
                        cases.append((f'stream_{cname}_K{K}_T{T}', 'stream',
                                      [(dv, k, 0, -1, 0) for dv, k in seq]))
        for K in ([16, 2 * k4 + 1] if not brute else [1, 8, 17, k4, 2 * k4 + 1]):
            for cname in ('8x8+3x4x4', '4x4'):          # one outstanding job per accelerator
                for gap in (0, 40):
                    lists = [(dv, [k for _ in range(3) for k in tile(dv, K)]) for dv in COMBOS[cname]]
                    jobs, last = [], {}
                    for dv, k in interleave(lists):
                        dep = last.get(dv, -1)
                        jobs.append((dv, k, 0, dep, gap if dep >= 0 else 0)); last[dv] = len(jobs) - 1
                    cases.append((f'serial_{cname}_K{K}_gap{gap}', 'serial', jobs))
        shapes = [(8, 16, 16), (16, 16, 16), (4, 12, 16), (12, 12, 16), (16, 8, 33), (24, 24, 32),
                  (32, 32, 16), (8, 8, 65), (20, 12, 7), (32, 16, 64), (16, 16, 129)]
        if brute:
            shapes += [(m, n, K) for (m, n) in ((8, 16), (16, 16), (12, 20), (32, 32)) for K in ks]
        for (M, N, K) in shapes:                        # V1 schedules, start order
            r = subprocess.run([TOOL, 'schedule', '--m', str(M), '--n', str(N), '--k', str(K),
                                '--k8', str(k8), '--k4', str(k4), '--pipelined', '--tiles',
                                '--cost-model', 'v1'], capture_output=True, text=True, check=True)
            tiles = sorted((int(f['start']), int(f['accelerator']))
                           for f in (dict(x.split('=') for x in l.split()[1:])
                                     for l in r.stdout.splitlines() if l.startswith('tile ')))
            jobs = [(a, k, 0, -1, 0) for (_, a) in tiles for k in tile(a, K)]
            if 0 < len(jobs) <= 600:
                cases.append((f'gemm_{M}x{N}x{K}', 'gemm', jobs))
        for n in range(30 if brute else 12):           # random mixes
            jobs = []
            for _ in range(rnd.randint(3, 40)):
                dv = rnd.randint(0, 3); km = k8 if dv == 0 else k4
                k = rnd.choice([1, rnd.randint(1, km), km, max(1, km - 1)])
                present = max(rnd.choice([0, 0, rnd.randint(0, 2000)]), jobs[-1][2] if jobs else 0)
                jobs.append((dv, k, present, -1, 0))
            cases.append((f'random_{n}', 'random', jobs))
        for name, fam, jobs in cases:
            path = os.path.join(d, name + '.jobs')
            with open(path, 'w') as f:
                f.write(f'# {cfg} {fam} {name}\n' + ''.join('%d %d %d %d %d\n' % j for j in jobs))
            manifest.append(dict(config=cfg, k8=k8, k4=k4, name=name, family=fam, jobs=path, njobs=len(jobs)))
    json.dump(manifest, open(os.path.join(out, 'manifest.json'), 'w'), indent=1)
    print(len(manifest), 'cases,', sum(m['njobs'] for m in manifest), 'jobs')


# ----------------------------------------------------------------------------- logs
def read_jobs(path):
    jobs = []
    for line in open(path):
        if line.startswith('#') or not line.strip():
            continue
        d, k, p, dep, gap = map(int, line.split()[:5])
        jobs.append(dict(dev=d, k=k, present=p, dep=dep, gap=gap))
    return jobs


def read_log(path, jobs):
    """Per-job RTL stamps from a tb_fleet_jobs log."""
    per_dev = [[i for i, j in enumerate(jobs) if j['dev'] == d] for d in range(4)]
    meas, cnt, fires = [dict() for _ in jobs], {}, 0
    descs = {s: [] for s in range(5)}; lasts = {s: [] for s in range(5)}
    ars, takes, rbdones, base, dp = [], [], [], None, None
    for line in open(path):
        f = line.split()
        if not f or not f[0].startswith('V'):
            continue
        if f[0] == 'VJ':
            meas[fires]['fire'] = int(f[2]); fires += 1
        elif f[0] == 'VF':
            ev, c, d = f[1], int(f[2]), int(f[3])
            n = cnt.get((ev, d), 0); cnt[(ev, d)] = n + 1
            meas[per_dev[d][n]][ev] = c
        elif f[0] == 'VD':
            descs[int(f[2])].append(int(f[1]))
        elif f[0] == 'VL':
            lasts[int(f[2])].append(int(f[1]))
        elif f[0] == 'VA':
            ars.append((int(f[1]), int(f[2]), int(f[3]), int(f[4])))
        elif f[0] == 'VR':
            (takes if f[1] == 'take' else rbdones).append((int(f[2]), int(f[3])))
        elif f[:2] == ['VO', 'push'] and base is None:
            base = int(f[3]) - int(f[2]) * UI_PS
        elif f[:2] == ['VO', 'pop'] and dp is None:
            dp = int(f[3])
    for s in range(4):
        for n, c in enumerate(descs[s]):
            meas[per_dev[s][n]]['desc'] = c
        for n, c in enumerate(lasts[s]):
            meas[per_dev[s][n]]['last'] = c
    for lst, key in ((takes, 'take'), (rbdones, 'rbdone')):
        cnt2 = [0] * 4
        for c, d in lst:
            meas[per_dev[d][cnt2[d]]][key] = c; cnt2[d] += 1
    phase = (dp - base) % BENCH_DPTI_PS if base is not None and dp is not None else 0
    return meas, ars, phase


def fleet_sim(jobfile, k8, k4, variant='v1', port='inorder', phase=0, period=16667, extra=()):
    a = [TOOL, 'fleet-sim', '--jobs', jobfile, '--k8', str(k8), '--k4', str(k4), '--port', port,
         '--dpti-phase-ps', str(phase), '--dpti-period-ps', str(period)] + list(extra)
    if variant == 'v2':
        a.append('--v2-operand-path')
    r = subprocess.run(a, capture_output=True, text=True)
    if r.returncode != 0:
        raise RuntimeError(r.stderr)
    jobs, fleet, bursts = [], {}, {}
    for line in r.stdout.splitlines():
        f = line.split()
        if f[0] == 'job':
            jobs.append({k: int(v) for k, v in (x.split('=') for x in f[2:])})
        elif f[0] == 'fleet':
            fleet = {k: int(v) for k, v in (x.split('=') for x in f[1:])}
        elif f[0] == 'bursts':
            bursts = {int(k): int(v) for k, v in (x.split(':') for x in f[1:])}
    return jobs, fleet, bursts, r.stdout


# ----------------------------------------------------------------------------- V1
def simulate_v1(jobs, v2=False, depth=4, H=95, reduce=69, gap=26):
    """Cost Model V1 (CostModel.cpp AcceleratorTimeline, pipelined fleet) per
    accelerator on its own; acceptance = the offer; no shared engines."""
    P = [8, 4, 4, 4]; tw = {8: 91, 4: 31}
    tread = lambda k: (32 if v2 else 128) * math.ceil(k / 8) + 11
    st = [dict(a=0, fe=0, fs=0, k=0, cv=0, w=0, cvs=[], ws=[], n=0) for _ in range(4)]
    out = []
    for j in jobs:
        s, p, k = st[j['dev']], P[j['dev']], j['k']
        offer = max(j['present'], 1)
        if j['dep'] >= 0:
            offer = max(offer, out[j['dep']]['done'] + 1, out[j['dep']]['done'] + j['gap'])
        accept = max(s['a'], s['ws'][-depth] + 2 if len(s['ws']) >= depth else 0, offer)
        fill = max(accept, s['fe'], s['fs'])
        fe = fill + tread(k)
        feed = fe
        if s['n'] > 0:
            feed = max(feed, s['fs'] + s['k'] + gap)
        if s['n'] >= 2:
            feed = max(feed, s['cvs'][-2] + 3, s['ws'][-2] + 3)
        cv = feed + k + 2 * p - 2 + H
        if s['n'] > 0:
            cv = max(cv, s['cv'] + reduce)
        done = max(cv, s['w']) + tw[p]
        s.update(a=accept, fe=fe, fs=feed, k=k, cv=cv, w=done)
        s['cvs'].append(cv); s['ws'].append(done); s['n'] += 1
        out.append(dict(fire=accept, fold=feed, cv=cv, done=done))
    return out


# ----------------------------------------------------------------------------- steps
def bench(out, k8, k4, variant):
    return os.path.join(out, 'bench', f'{k8}_{k4}_{variant}', 'obj', 'tbrun')


def step_build(out, jobs):
    for (k8, k4) in CONFIGS:
        subprocess.run(['bash', os.path.join(HERE, 'build_bench.sh'), str(k8), str(k4), 'v1',
                        os.path.join(out, 'bench', f'{k8}_{k4}_v1')], check=True)
    for (k8, k4) in V2_CONFIGS:
        subprocess.run(['bash', os.path.join(HERE, 'build_bench.sh'), str(k8), str(k4), 'v2',
                        os.path.join(out, 'bench', f'{k8}_{k4}_v2')], check=True)


def manifest_for(out, variant):
    man = json.load(open(os.path.join(out, 'manifest.json')))
    if variant == 'v2':
        man = [m for m in man if (m['k8'], m['k4']) in V2_CONFIGS]
    return man


def step_rtl(out, jobs):
    def run(mv):
        m, variant = mv
        log = m['jobs'][:-5] + f'.{variant}.log'
        with open(log, 'w') as f:
            r = subprocess.run([bench(out, m['k8'], m['k4'], variant), '+jobs=' + m['jobs'],
                                '+wb_region=4096'], stdout=f, stderr=subprocess.STDOUT)
        return r.returncode == 0 and 'PASS' in open(log).read()[-400:]
    work = [(m, 'v1') for m in manifest_for(out, 'v1')] + [(m, 'v2') for m in manifest_for(out, 'v2')]
    with ThreadPoolExecutor(jobs) as ex:
        ok = list(ex.map(run, work))
    print(f'rtl: {sum(ok)} / {len(ok)} bench runs PASS (every result checked exactly by the bench)')
    return all(ok)


def exact_one(mv):
    m, variant = mv
    jobs = read_jobs(m['jobs'])
    log = m['jobs'][:-5] + f'.{variant}.log'
    meas, ars, phase = read_log(log, jobs)
    pred, fleet, _, _ = fleet_sim(m['jobs'], m['k8'], m['k4'], variant, phase=phase, period=BENCH_DPTI_PS)
    bad = [(i, k, meas[i].get(k), pred[i].get(k)) for i in range(len(jobs))
           for k in KEYS + ('take', 'rbdone') if meas[i].get(k) != pred[i].get(k)]
    return dict(config=m['config'], name=m['name'], variant=variant,
                ok=not bad and len(ars) == fleet['ar'], first_bad=bad[:1],
                ar_rtl=len(ars), ar_model=fleet['ar'])


def step_exact(out, jobs):
    allok = True
    for variant in ('v1', 'v2'):
        work = [(m, variant) for m in manifest_for(out, variant)]
        with ThreadPoolExecutor(jobs) as ex:
            res = list(ex.map(exact_one, work))
        json.dump(res, open(os.path.join(out, f'exact_{variant}.json'), 'w'), indent=0)
        n = sum(r['ok'] for r in res)
        print(f'exact ({variant} operand path): {n} / {len(res)} cases identical to the RTL '
              f'(every event, AR count, readback)')
        for r in res:
            if not r['ok']:
                print('  MISMATCH', r)
        allok &= n == len(res)
    return allok


def step_skip(out, jobs):
    def one(m):
        for port in ('inorder', 'credit'):
            for variant in ('v1', 'v2'):
                a = fleet_sim(m['jobs'], m['k8'], m['k4'], variant, port)[3]
                b = fleet_sim(m['jobs'], m['k8'], m['k4'], variant, port, extra=('--no-skip',))[3]
                if a != b:
                    return False
        return True
    man = manifest_for(out, 'v1')
    with ThreadPoolExecutor(jobs) as ex:
        ok = list(ex.map(one, man))
    print(f'skip: {sum(ok)} / {len(ok)} cases x 4 modes identical with and without the idle skip')
    return all(ok)


CREDIT_SHAPES = [(8, 16, 16), (16, 16, 16), (4, 12, 16), (12, 12, 16), (16, 8, 33), (24, 24, 32),
                 (32, 32, 16), (8, 8, 65), (20, 12, 7), (32, 16, 64), (16, 16, 129), (24, 20, 40),
                 (16, 16, 33), (40, 24, 17)]


def credit_one(args):
    out, k8, k4, M, N, K = args
    d = os.path.join(out, 'credit'); os.makedirs(d, exist_ok=True)
    tag = f'{k8}_{k4}_{M}x{N}x{K}'
    r = subprocess.run([TOOL, 'schedule', '--m', str(M), '--n', str(N), '--k', str(K), '--k8', str(k8),
                        '--k4', str(k4), '--pipelined', '--tiles'], capture_output=True, text=True, check=True)
    jl, pred = [], None
    for line in r.stdout.splitlines():
        if line.startswith('tile '):
            f = dict(x.split('=') for x in line.split()[1:])
            a = int(f['accelerator']); km = k8 if a == 0 else k4
            jl += [(a, min(km, K - k0)) for k0 in range(0, K, km)]
        elif line.startswith('cost_model v2'):
            pred = dict(x.split('=') for x in line.split()[2:])
    src = os.path.join(d, tag + '.decomp.jobs')
    open(src, 'w').write(''.join(f'{a} {k} 0 -1 0\n' for a, k in jl))
    cj, cf, _, _ = fleet_sim(src, k8, k4, port='credit')
    order = sorted(range(len(jl)), key=lambda i: cj[i]['fire'])
    rep = os.path.join(d, tag + '.replay.jobs')
    open(rep, 'w').write(''.join(f'{jl[i][0]} {jl[i][1]} 0 -1 0\n' for i in order))
    log = rep[:-5] + '.v1.log'
    with open(log, 'w') as f:
        subprocess.run([bench(out, k8, k4, 'v1'), '+jobs=' + rep, '+wb_region=4096'],
                       stdout=f, stderr=subprocess.STDOUT)
    passed = 'PASS' in open(log).read()[-400:]
    meas, ars, phase = read_log(log, read_jobs(rep))
    cj2, cf2, _, _ = fleet_sim(src, k8, k4, port='credit', phase=phase, period=BENCH_DPTI_PS)
    mism = sum(1 for n, i in enumerate(order) for k in KEYS if meas[n].get(k) != cj2[i].get(k))
    ms = max(x['done'] for x in meas) - min(x['fire'] for x in meas)
    return dict(tag=tag, ok=passed and mism == 0 and len(ars) == cf2['ar']
                and int(pred['predicted_device_makespan']) == cf['device_makespan'],
                njobs=len(jl), rtl=ms, compiler_prediction=cf['device_makespan'],
                planning=int(pred['planning_makespan']))


def step_credit(out, jobs):
    cases = [(out, k8, k4, *s) for (k8, k4) in CONFIGS for s in CREDIT_SHAPES]
    with ThreadPoolExecutor(jobs) as ex:
        res = list(ex.map(credit_one, cases))
    json.dump(res, open(os.path.join(out, 'credit.json'), 'w'), indent=1)
    n = sum(r['ok'] for r in res)
    print(f'credit: {n} / {len(res)} compiler schedules identical to the RTL replay')
    return n == len(res)


def report_one(m):
    jobs = read_jobs(m['jobs'])
    log = m['jobs'][:-5] + '.v1.log'
    meas, ars, _ = read_log(log, jobs)
    pred, fleet, bursts, _ = fleet_sim(m['jobs'], m['k8'], m['k4'])      # default phase: no per-case input
    v1 = simulate_v1(jobs)
    span = lambda L: max(x['done'] for x in L) - min(x['fire'] for x in L)
    dev = {}
    for d in range(4):
        idx = [i for i, j in enumerate(jobs) if j['dev'] == d]
        if idx:
            sub = lambda L: max(L[i]['done'] for i in idx) - min(L[i]['fire'] for i in idx)
            dev[d] = (sub(meas), sub(pred), sub(v1))
    read_rtl = sum(meas[i]['land'] - meas[i]['desc'] for i in range(len(jobs)))
    read_v2 = sum(p['land'] - p['desc'] for p in pred)
    return dict(config=m['config'], name=m['name'], family=m['family'], njobs=len(jobs),
                rtl=span(meas), v2=span(pred), v1=span(v1), per_device=dev,
                read_rtl=read_rtl, read_v2=read_v2, contention_v2=fleet['contention_cycles'],
                ar_rtl=len(ars), ar_v2=fleet['ar'], bursts_v2=bursts,
                max_outstanding_rtl=max((a[3] + 1 for a in ars), default=0),
                max_outstanding_v2=fleet['max_outstanding'], operand_bytes_v2=fleet['operand_bytes'])


def pct(a, b):
    return abs(a - b) / b * 100.0 if b else 0.0


def step_report(out, jobs):
    man = manifest_for(out, 'v1')
    with ProcessPoolExecutor(jobs) as ex:
        res = list(ex.map(report_one, man, chunksize=4))
    for r in res:
        r['err_v2'] = pct(r['v2'], r['rtl']); r['err_v1'] = pct(r['v1'], r['rtl'])
    json.dump(res, open(os.path.join(out, 'report.json'), 'w'), indent=0)

    def stats(xs):
        xs = sorted(xs)
        return dict(n=len(xs), mean=sum(xs) / len(xs), max=xs[-1], p95=xs[int(math.ceil(0.95 * len(xs))) - 1])
    lines = ['# Cost Model V2 vs RTL co-simulation (device makespan)', '',
             'Source: RTL simulation of the bitstream RTL (tb_fleet_jobs, Verilator). NOT board',
             'measurements. V2 predicted with the default clock phase (no per-case input).', '']
    s1, s2 = stats([r['err_v1'] for r in res]), stats([r['err_v2'] for r in res])
    lines += ['| model | cases | mean error | P95 error | max error |', '|---|---|---|---|---|',
              f"| V1 | {s1['n']} | {s1['mean']:.3f}% | {s1['p95']:.3f}% | {s1['max']:.3f}% |",
              f"| V2 | {s2['n']} | {s2['mean']:.4f}% | {s2['p95']:.4f}% | {s2['max']:.4f}% |", '']
    lines += ['## Per configuration', '', '| K_MAX | cases | V1 mean / max | V2 mean / max | V2 exact |',
              '|---|---|---|---|---|']
    for cfg in sorted({r['config'] for r in res}, key=lambda c: tuple(map(int, c.split('_')))):
        sub = [r for r in res if r['config'] == cfg]
        a, b = stats([r['err_v1'] for r in sub]), stats([r['err_v2'] for r in sub])
        lines.append(f"| {cfg.replace('_', '/')} | {len(sub)} | {a['mean']:.2f}% / {a['max']:.2f}% | "
                     f"{b['mean']:.4f}% / {b['max']:.4f}% | {sum(1 for r in sub if r['v2'] == r['rtl'])} |")
    lines += ['', '## Largest V2 errors (top 20)', '', '| K_MAX | case | jobs | RTL | V2 | V1 | V2 err | V1 err |',
              '|---|---|---|---|---|---|---|---|']
    for r in sorted(res, key=lambda r: -r['err_v2'])[:20]:
        lines.append(f"| {r['config']} | {r['name']} | {r['njobs']} | {r['rtl']} | {r['v2']} | {r['v1']} | "
                     f"{r['err_v2']:.4f}% | {r['err_v1']:.2f}% |")
    lines += ['', '## Largest V1 errors (top 20)', '', '| K_MAX | case | jobs | RTL | V2 | V1 | V2 err | V1 err |',
              '|---|---|---|---|---|---|---|---|']
    for r in sorted(res, key=lambda r: -r['err_v1'])[:20]:
        lines.append(f"| {r['config']} | {r['name']} | {r['njobs']} | {r['rtl']} | {r['v2']} | {r['v1']} | "
                     f"{r['err_v2']:.4f}% | {r['err_v1']:.2f}% |")
    for cfg in ('32_32', '64_64'):
        lines += ['', f'## Every {cfg.replace("_", "/")} case', '',
                  '| case | family | jobs | RTL | V2 | V1 | V2 err | V1 err | AR RTL/V2 | read cycles RTL/V2 |',
                  '|---|---|---|---|---|---|---|---|---|---|']
        for r in [r for r in res if r['config'] == cfg]:
            lines.append(f"| {r['name']} | {r['family']} | {r['njobs']} | {r['rtl']} | {r['v2']} | {r['v1']} | "
                         f"{r['err_v2']:.4f}% | {r['err_v1']:.2f}% | {r['ar_rtl']}/{r['ar_v2']} | "
                         f"{r['read_rtl']}/{r['read_v2']} |")
    open(os.path.join(out, 'summary.md'), 'w').write('\n'.join(lines) + '\n')
    print(f"report: {len(res)} cases; V2 mean {s2['mean']:.4f}% P95 {s2['p95']:.4f}% max {s2['max']:.4f}%; "
          f"V1 mean {s1['mean']:.2f}% P95 {s1['p95']:.2f}% max {s1['max']:.2f}% -> {out}/summary.md")
    return s2['max'] <= 10.0


def step_scan(out, jobs):
    cmds = []
    for m in (4, 8, 12, 16, 24, 32):
        for n in (4, 8, 12, 16, 24, 32):
            for k in (1, 9, 16, 33, 64):
                for k8, k4 in ((16, 16), (24, 12), (32, 32)):
                    for pipe in ([], ['--pipelined']):
                        for sh in ('0', '3.51'):
                            cmds.append(['--m', str(m), '--n', str(n), '--k', str(k), '--k8', str(k8),
                                         '--k4', str(k4), '--shared-read-words-per-cycle', sh] + pipe)

    def one(a):
        r = subprocess.run([TOOL, 'scan-feasible'] + a, capture_output=True, text=True)
        return r.returncode == 0, ' '.join(a), r.stdout.strip().splitlines()[-1:]
    with ThreadPoolExecutor(jobs) as ex:
        res = list(ex.map(one, cmds))
    open(os.path.join(out, 'scan.txt'), 'w').write('\n'.join(f'{ok} {a} {o}' for ok, a, o in res) + '\n')
    n = sum(ok for ok, _, _ in res)
    print(f'scan: {n} / {len(res)} problems monotone over every deadline 0..2T* (V2)')
    for ok, a, o in res:
        if not ok:
            print('  COUNTEREXAMPLE', a, o)
    return n == len(res)


STEPS = ['build', 'cases', 'rtl', 'exact', 'skip', 'credit', 'report', 'scan']


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--out', default=os.path.join(ROOT, 'build', 'cost_model_v2'))
    ap.add_argument('--jobs', type=int, default=6)
    ap.add_argument('steps', nargs='*', default=STEPS)
    a = ap.parse_args()
    out = os.path.abspath(a.out); os.makedirs(out, exist_ok=True)
    ok = True
    for s in a.steps:
        if s == 'cases':
            gen_cases(out)
        else:
            r = globals()['step_' + s](out, a.jobs)
            ok &= r is not False
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
