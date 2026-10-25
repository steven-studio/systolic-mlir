#!/usr/bin/env python3
"""Focused model regression: bounded resources, RR policy and output drain.
Numerical timing expectations are separately checked against actual R2 RTL by
validate_r2.py. These invariants do not generate reference cycle counts.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--tool',required=True)
a=p.parse_args()
with tempfile.TemporaryDirectory(prefix='cost-r2-unit-') as tmp:
    jobs=Path(tmp)/'jobs'
    for devices in ([0], [1], list(range(4))*6):
        jobs.write_text(''.join(f'{d} 1 0 -1 0\n' for d in devices))
        for block in (0, 30_000_000):
            cmd=[a.tool,'fleet-sim','--jobs',str(jobs),'--trace-results','--host-block-until-ps',str(block)]
            text=subprocess.check_output(cmd,text=True)
            lines=text.splitlines()
            expected=sum(272 if d==0 else 80 for d in devices)
            output=dict(x.split('=') for x in next(l for l in lines if l.startswith('output ')).split()[1:])
            assert int(output['expected'])==int(output['emitted'])==expected
            fleet=dict(x.split('=') for x in next(l for l in lines if l.startswith('fleet ')).split()[1:])
            assert int(fleet['last_byte_to_host'])>=int(fleet['readback_end'])
            assert int(output['last_byte_ps'])>=block-20_000
            records=[tuple(map(int,l.split()[1:])) for l in lines if l.startswith('RT ')]
            bytime={}
            for r in records:
                t,d,fifo,meta,banks,full,empty,reserved,push,pop,reserve,commit,retire,eligible,owner,aw,w,b=r
                assert 0<=fifo<=32 and 0<=meta<=2 and 0<=banks<=2
                assert not (full and push) and not (empty and pop)
                assert not reserve or meta<2
                assert not retire or meta>0
                bytime.setdefault(t,[]).append(r)
            # On every newly acquired owner, it must be the first eligible
            # context AFTER the last granted one. Idle gaps retain the pointer.
            last=3; previous=-1
            for t,rs in sorted(bytime.items()):
                owner=rs[0][14]
                if owner>=0 and previous<0:
                    # Ownership is registered: eligibility was sampled on
                    # the preceding edge, whose state is in the trace.
                    before=bytime[t-1]
                    eligible={r[1] for r in before if r[13]}
                    want=next((last+n)%4 for n in range(1,5) if (last+n)%4 in eligible)
                    assert owner==want,(t,owner,want)
                    last=owner
                previous=owner
    # Failed B responses must not turn into successful job completions.
    jobs.write_text('0 1 0 -1 0\n')
    bus=Path(tmp)/'bus'
    bus.write_text(''.join(f'{t} 1 1 {int(t==500)} {2 if t==500 else 0}\n' for t in range(1000)))
    r=subprocess.run([a.tool,'fleet-sim','--jobs',str(jobs),'--writeback-inputs',str(bus)],text=True,capture_output=True)
    assert r.returncode!=0 and 'AXI write response failed' in r.stderr
    # A permanently stalled host must terminate with a diagnostic, rather
    # than reporting the last byte seen so far or running indefinitely.
    r=subprocess.run([a.tool,'fleet-sim','--jobs',str(jobs),
        '--host-block-until-ps','1000000000000'],text=True,capture_output=True,timeout=60)
    assert r.returncode!=0 and 'deadlock: nothing moved' in r.stderr
print('PASS R2 resource, round-robin, response-error and output-drain invariants')
