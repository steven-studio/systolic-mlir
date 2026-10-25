#!/usr/bin/env python3
"""Exact Shared Writeback R2 differential validation (no production RTL edits).

Build: bash eval/cost_model_v2/build_bench.sh 16 16 v1 /tmp/r2-v1
Run: python3 eval/cost_model_v2/validate_r2.py --bench /tmp/r2-v1/obj/tbrun --out /tmp/r2-check

The default run predicts the ideal bench memory, including the write-bus
ownership handshake. Stress runs can replay *external* AXI ready/B inputs
observed on RTL; AW/W handshakes, grants, resource transitions and completion
are still compared exactly. This isolates the writeback model from the memory
controller, whose board timing is deliberately uncalibrated. The reference
values always come from the unchanged Revision 2 RTL, never this model.
"""
import argparse
import json
from pathlib import Path
import random
import subprocess
import sys
import run_validation as old

ROOT = Path(__file__).resolve().parents[2]


def run(cmd, log):
    p = subprocess.run(list(map(str, cmd)), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=180)
    log.write_text(p.stdout)
    if p.returncode:
        raise RuntimeError(f'{cmd}: exit {p.returncode}\n{p.stdout[-3000:]}')
    return p.stdout


def validate(bench, out, name, jobs, variant, k8, k4, extra=(), host_block=0, replay=False):
    path = out / (name + '.jobs')
    path.write_text(''.join(' '.join(map(str, j)) + '\n' for j in jobs))
    rtl_path = out / (name + '.rtl.log')
    rtl = run([bench, f'+jobs={path}', '+wb_region=4096', '+trace_results', f'+host_block_until_ps={host_block}', *extra], rtl_path)
    assert '== PASS:' in rtl and 'VERR' not in rtl, name
    measured, ars, phase = old.read_log(rtl_path, old.read_jobs(path))
    cmd = [ROOT/'build/bin/systolic-cost', 'fleet-sim', '--jobs', path, '--k8', k8, '--k4', k4,
           '--dpti-phase-ps', phase, '--dpti-period-ps', 16666, '--trace-results',
           '--host-block-until-ps', host_block]
    if name == 'output-paused':
        cmd += ['--host-pause-start-ps', 6_000_000, '--host-pause-end-ps', 20_000_000]
    if variant == 'v2': cmd += ['--v2-operand-path']
    if replay:
        inputs = out / (name + '.bus')
        inputs.write_text(''.join(l[3:]+'\n' for l in rtl.splitlines() if l.startswith('BI ')))
        cmd += ['--writeback-inputs', inputs]
    model = run(cmd, out/(name+'.model.log'))
    pred = []
    enqueues = {}
    fleet = {}
    for line in model.splitlines():
        f = line.split()
        if f[0] == 'job': pred.append({k:int(v) for k,v in (x.split('=') for x in f[2:])})
        if f[0] == 'enqueue': enqueues[int(f[1])] = int(f[2].split('=')[1])
        if f[0] == 'fleet': fleet = {k:int(v) for k,v in (x.split('=') for x in f[1:])}
    diffs = []
    if len(pred) != len(measured): diffs.append("job event count mismatch")
    for i,(a,b) in enumerate(zip(measured,pred)):
        for key in (*old.KEYS, 'take', 'rbdone'):
            actual = enqueues[i] if key == 'enqueue' else b[key]
            if a[key] != actual: diffs.append(f'job {i} {key}: RTL={a[key]} model={actual}')
    rt = {tuple(map(int,l.split()[1:3])):tuple(map(int,l.split()[3:])) for l in rtl.splitlines() if l.startswith('RT ')}
    mt = {tuple(map(int,l.split()[1:3])):tuple(map(int,l.split()[3:])) for l in model.splitlines() if l.startswith('RT ')}
    # Model may skip provably idle cycles; each recorded model state must
    # match the same pre-edge RTL state. --no-skip is separately compared.
    fields = ('fifo','metadata','banks','full','empty','reserved','push','pop','reserve','commit','retire','eligible','owner','aw','w','b')
    for key, vals in mt.items():
        if key not in rt:
            diffs.append(f'missing RTL cycle/context {key}'); continue
        for field,a,b in zip(fields,rt[key],vals):
            if a!=b: diffs.append(f'cycle/context {key} {field}: RTL={a} model={b}')
        if len(diffs)>40: break
    vh = [l for l in rtl.splitlines() if l.startswith('VH ')]
    assert len(vh)==1, (name,vh)
    output = dict(x.split('=') for x in vh[0].split()[1:])
    first = measured[0]['fire']
    push = next(l.split() for l in rtl.splitlines() if l.startswith('VO push '))
    origin = int(push[3]) - int(push[2])*10000
    expected_last = (int(output['last_abs_ps']) - origin + 9999)//10000 - first
    reported_output = next(l for l in model.splitlines() if l.startswith('output '))
    actual_output = dict(x.split('=') for x in reported_output.split()[1:])
    expected_ps = int(output['last_abs_ps']) - origin - first*10000
    if int(actual_output['last_byte_ps']) != expected_ps:
        diffs.append(f'host final event ps: RTL={expected_ps} model={actual_output["last_byte_ps"]}')
    if int(actual_output['emitted']) != int(output['bytes']): diffs.append('host byte count mismatch')
    if fleet['last_byte_to_host']!=expected_last:
        diffs.append(f'host last byte: RTL={expected_last} model={fleet["last_byte_to_host"]}')
    if fleet['ar']!=len(ars): diffs.append(f'AR bursts: RTL={len(ars)} model={fleet["ar"]}')
    # Explicitly compare skip/no-skip events and all non-trace output.
    noskip = run([*cmd, '--no-skip'], out/(name+'.noskip.log'))
    stable = lambda text: [l for l in text.splitlines() if not l.startswith('RT ')]
    if stable(model)!=stable(noskip): diffs.append('skip changed events or metrics')
    # Compare *every* cycle in the no-skip run (including full/empty edges).
    nt = [l for l in noskip.splitlines() if l.startswith('RT ')]
    if len(nt) != 4*fleet['simulated_cycles']: diffs.append('missing no-skip cycle states')
    for l in nt:
        f=tuple(map(int,l.split()[1:])); key=f[:2]
        if rt.get(key)!=f[2:]:
            diffs.append(f'no-skip state mismatch {key}: RTL={rt.get(key)} model={f[2:]}')
            if len(diffs)>50: break
    (out/(name+'.diff.txt')).write_text('\n'.join(diffs)+'\n' if diffs else '')
    coverage = dict(fifo_full=sum(v[3] for v in rt.values()),
                    metadata_full=sum(v[1] == 2 for v in rt.values()),
                    simultaneous_push_pop=sum(v[6] and v[7] for v in rt.values()),
                    simultaneous_commit_retire=sum(v[9] and v[10] for v in rt.values()))
    early = any(a['enqueue'] < a['done'] and any(
        jobs[k][0] == jobs[i][0] and b['fold'] > a['enqueue'] and b['fold'] < a['done']
        for k,b in enumerate(measured) if k>i+1) for i,a in enumerate(measured))
    coverage['early_feed_before_DDR'] = early
    if name in ('output-blocked', 'output-paused'):
        stats = next(l for l in rtl.splitlines() if l.startswith('VS '))
        observed = dict(x.split('=') for x in stats.split()[1:])
        if int(observed['rb_fifo_full_cycles']) == 0: diffs.append('RTL output FIFO never saturated')
    if name == 'FIFO-saturation':
        if not coverage['fifo_full']: diffs.append('RTL did not exercise result FIFO full')
        if not coverage['metadata_full']: diffs.append('RTL did not exercise metadata full')
    if name == 'FIFO-push-pop-same-edge' and not coverage['simultaneous_push_pop']:
        diffs.append('RTL did not reach simultaneous FIFO push and pop')
    if name == 'commit-retire-same-edge' and not coverage['simultaneous_commit_retire']:
        diffs.append('RTL did not reach simultaneous commit and retirement')
    if name == 'delayed-B' and not early: diffs.append('RTL did not exercise early bank reuse')
    if diffs: raise AssertionError(name+'\n'+'\n'.join(diffs[:30]))
    print(f'PASS {name}: {len(jobs)} jobs, {len(nt)} context-cycles, output {output["bytes"]} bytes', flush=True)
    return dict(name=name,jobs=len(jobs),context_cycles=len(nt),bytes=int(output['bytes']),external_axi_replay=replay,coverage=coverage)


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--bench',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    p.add_argument('--variant',choices=['v1','v2'],default='v1')
    p.add_argument('--k8',type=int,default=16); p.add_argument('--k4',type=int,default=16)
    p.add_argument('--only')
    a=p.parse_args(); a.out.mkdir(parents=True,exist_ok=True)
    cases=[]
    for d in [0,1]:
        for k in sorted({1,7,8,9,min(16,a.k8 if d==0 else a.k4)}):
            if k <= (a.k8 if d==0 else a.k4): cases.append((f'single-d{d}-k{k}',[(d,k,0,-1,0)],[],0,False))
    for gap in [0,3,40,300,1500]:
        cases.append((f'fourway-gap{gap}',[(d,1 if d%2 else 16,i*gap,-1,0) for i,d in enumerate([0,1,2,3]*4)],[],0,False))
    cases += [('partial-folds',[(d,k,0,-1,0) for k in [16,16,1] for d in range(4)],[],0,False),
              ('output-paused',[(d,1,0,-1,0) for d in list(range(4))*4],['+host_pause_start_ps=6000000','+host_pause_end_ps=20000000'],0,False),
              ('output-blocked',[(d,1,0,-1,0) for d in list(range(4))*4],[],30_000_000,False),
              ('delayed-B',[(d,1,0,-1,0) for d in list(range(4))*8],['+b_delay=600'],0,True),
              ('FIFO-saturation',[(d,1,0,-1,0) for d in list(range(4))*8],['+aw_stall=1000','+w_stall=20','+b_delay=80'],0,True),
              ('AW-W-stalls',[(d,1,0,-1,0) for d in list(range(4))*6],['+aw_stall=5','+w_stall=3','+b_delay=80'],0,True)]
    rng=random.Random(20261025)
    for i in range(8):
        cases.append((f'random-{i}',[(d:=rng.randrange(4),rng.randint(1,a.k8 if d==0 else a.k4),n*rng.choice([0,3,40]),-1,0) for n in range(24)],[],0,False))
    # Selected by sweeping RTL, not the model; coverage is asserted below.
    cases.append(('FIFO-push-pop-same-edge',[(0,1,0,-1,0)]*4,
                  [f'+w_stall={5 if a.variant == "v2" else 20}'],0,True))
    delay = 64 if a.variant == 'v2' else 117
    cases.append(('commit-retire-same-edge',[(0,1,0,-1,0)]*4,[f'+b_delay={delay}'],0,True))
    records=[]
    for name,jobs,extra,block,replay in cases:
        if a.only and name!=a.only: continue
        records.append(validate(a.bench,a.out,name,jobs,a.variant,a.k8,a.k4,extra,block,replay))
        (a.out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
    print(f'PASS {len(records)} exact Revision 2 differential cases')

if __name__=='__main__': main()
