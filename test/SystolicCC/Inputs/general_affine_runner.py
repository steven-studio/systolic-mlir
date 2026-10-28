"""End-to-end tests of real jobs, full buffers, and independent IR coverage."""
import argparse
import json
import os
from pathlib import Path
import re
import subprocess
from general_affine_oracle import check

p=argparse.ArgumentParser();p.add_argument('--source',required=True);p.add_argument('--clang',required=True);p.add_argument('--out',required=True);p.add_argument('--cc',nargs=argparse.REMAINDER,required=True);a=p.parse_args()
root=Path(a.out);root.mkdir(parents=True,exist_ok=True)
assert "--general-affine" not in a.cc, "the public CLI must work without a general flag"

def run(cmd, name, env=None):
    result=subprocess.run(list(map(str,cmd)),text=True,capture_output=True,env=env)
    (root/(name+'.stdout')).write_text(result.stdout);(root/(name+'.stderr')).write_text(result.stderr)
    assert result.returncode==0,(cmd,result.returncode,result.stdout,result.stderr)
    return result

def bits(text):return re.findall(r'^[0-9a-f]{8}$',text,re.M)
def stats(log):
    m=re.search(r'AUDIT completed=(\d+) failed=(\d+) hardware=(\d+)',log);assert m,log
    return tuple(map(int,m.groups()))

results=[]
for case in range(1,5):
    n=16 if case<3 else 64
    ref=root/f'ref{case}'
    run([a.clang,'-std=c11','-O0','-ffp-contract=off',f'-DCASE={case}',a.source,'-o',ref],f'ref{case}.compile')
    reference=run([ref],f'ref{case}.run');want=bits(reference.stdout);assert len(want)==n*n
    for submission in ('sync','async'):
        name=f'case{case}-{submission}';exe=root/name;keep=root/(name+'.ir')
        run(a.cc+['--fp-reassociation','--verify','--explain',f'--submission={submission}',f'-DCASE={case}','-DAUDIT_STATS','--keep-dir',keep,a.source,'-o',exe],name+'.compile')
        plan=json.loads((keep/'25-plan.json').read_text());assert plan['offloaded']==['kernel'],plan
        coverage=check(case,(keep/'20-systolic.mlir').read_text(),n)
        if case in (2,3,4) and submission=='sync':
            # The internal fast-only control is for developer regression
            # isolation, never required by ordinary users. Compare actual IR,
            # not just invocation totals, to ensure successful plans are kept.
            fast=root/(name+'.fast');fastkeep=root/(name+'.fast.ir')
            env=os.environ.copy();env['SYSTOLIC_CC_TEST_FAST_PATH_ONLY']='1'
            run(a.cc+['--fp-reassociation','--verify','--submission=sync',
                      f'-DCASE={case}','-DAUDIT_STATS','--keep-dir',fastkeep,
                      a.source,'-o',fast],name+'.fast.compile',env)
            assert (fastkeep/'20-systolic.mlir').read_bytes()==(keep/'20-systolic.mlir').read_bytes()
        result=run([exe],name+'.run');completed,failed,hardware=stats(result.stderr)
        assert bits(result.stdout)==want,(case,submission,'full-buffer mismatch')
        assert completed==coverage['actual_expanded_jobs'] and completed>0 and failed==hardware==0
        remarks=(keep/'20-remarks.txt').read_text();assert 'Presburger exact cover verified' in remarks
        assert f'source MACs={64 if case<3 else 256}' in remarks
        if case==1:
            assert completed==37 and 'intersections AB/AC/BC/shared = 1,1,1,0' in remarks
            assert 'exact Presburger cover and source-order certificate verified' in remarks
        if case==2:assert completed==1
        if case==3:
            assert completed==4 and 'parametric partition dims=1' in remarks
        if case==4:
            assert completed==16 and 'ranks A/B/C/AB/AC/BC/ABC = 2, 2, 2, 4, 4, 4, 4' in remarks
            assert 'intersections AB/AC/BC/shared = 0,0,0,0' in remarks
            assert 'parametric partition dims=1' in remarks
        results.append(dict(case=case,submission=submission,completed=completed,failed=failed,hardware=hardware,full_output_bits=len(want),**coverage))
        if case==1:
            env=os.environ.copy();env['SOFT_RUNTIME_FAULT']='fail';env['SOFT_RUNTIME_FAULT_AT']='2'
            fault=run([exe],name+'.fault',env)
            assert bits(fault.stdout)==want,'failure did not restore output before CPU fallback'
            assert stats(fault.stderr)[1]>0
    strict=root/f'case{case}-strict';keep=root/f'case{case}-strict.ir'
    run(a.cc+['--verify',f'-DCASE={case}','-DAUDIT_STATS','--keep-dir',keep,a.source,'-o',strict],f'case{case}-strict.compile')
    assert json.loads((keep/'25-plan.json').read_text())['offloaded']==[]
    result=run([strict],f'case{case}-strict.run')
    assert stats(result.stderr)==(0,0,0) and bits(result.stdout)==want
# Exercise genuine multi-invocation fiber plans without changing the oracle.
for submission in ('sync','async'):
    name='case1-fold-'+submission;exe=root/name;keep=root/(name+'.ir')
    run(a.cc+['--fp-reassociation','--verify',f'--submission={submission}',
              '--k-max-8x8=2','--k-max-4x4=2','-DCASE=1','-DAUDIT_STATS',
              '--keep-dir',keep,a.source,'-o',exe],name+'.compile')
    coverage=check(1,(keep/'20-systolic.mlir').read_text(),16)
    result=run([exe],name+'.run')
    assert bits(result.stdout)==bits((root/'ref1.run.stdout').read_text())
    assert stats(result.stderr)==(coverage['actual_expanded_jobs'],0,0)
    assert coverage['actual_expanded_jobs']>37
    results.append(dict(case=1,submission=submission,k_max=2,completed=stats(result.stderr)[0],**coverage))
# An unused enclosing counter belongs to U and doubles source executions.
# It must neither disappear nor become an independent write-disjoint batch.
name='case1-unused';exe=root/name;keep=root/(name+'.ir')
run(a.cc+['--fp-reassociation','--verify','--submission=sync',
          '-DCASE=1','-DREPEAT','-DAUDIT_STATS','--keep-dir',keep,a.source,'-o',exe],name+'.compile')
reference=root/(name+'.reference')
run([a.clang,'-O0','-ffp-contract=off','-DCASE=1','-DREPEAT',a.source,'-o',reference],name+'.ref.compile')
result=run([exe],name+'.run');want=run([reference],name+'.ref.run')
assert bits(result.stdout)==bits(want.stdout) and stats(result.stderr)==(37,0,0)
sites=json.loads((keep/'10-gemm-sites.json').read_text())['sites']
assert len(sites)==1 and sites[0]['executions']==2 and len(sites[0]['anchors'])==2
assert len(re.findall(r'CPU Executed: GEMM #',result.stdout))==2
from general_affine_oracle import dense_ir,source
from collections import Counter
jobs,macs=dense_ir((keep/'20-systolic.mlir').read_text(),16)
assert jobs==37 and Counter(macs)==Counter(source(1,16)*2)
results.append(dict(case=1,unused_loop=True,source_MACs=128,source_executions=2,completed=37,coverage='PASS'))
# A recognized but non-dense fast-path candidate must still get a complete
# independent general certificate, not be discarded merely for recognizing it.
for submission in ('sync','async'):
    name='lowering-gap-'+submission;exe=root/name;keep=root/(name+'.ir')
    run(a.cc+['--fp-reassociation','--verify',f'--submission={submission}',
              '-DCASE=5','-DAUDIT_STATS','--keep-dir',keep,a.source,'-o',exe],name+'.compile')
    reference=root/(name+'.reference')
    run([a.clang,'-O0','-ffp-contract=off','-DCASE=5',a.source,'-o',reference],name+'.ref.compile')
    got=run([exe],name+'.run');want=run([reference],name+'.ref.run')
    coverage=check(5,(keep/'20-systolic.mlir').read_text(),16)
    assert bits(got.stdout)==bits(want.stdout) and stats(got.stderr)==(14,0,0)
    assert coverage['actual_expanded_jobs']==14
    assert 'not lowered to systolic.matmul_tile' in (keep/'20-remarks.txt').read_text()
    results.append(dict(case=5,submission=submission,completed=14,**coverage))
# These bounds exceed only the general limits; the original CPU remains valid.
for name,defines in (('iv9',['-DEXTRA_IVS']),
                     ('points576',['-DREPEAT','-DREPEAT_TRIPS=9'])):
    exe=root/name;keep=root/(name+'.ir');reference=root/(name+'.reference')
    run(a.cc+['--fp-reassociation','--verify','-DCASE=1','-DAUDIT_STATS',
              '--keep-dir',keep]+defines+[a.source,'-o',exe],name+'.compile')
    run([a.clang,'-O0','-ffp-contract=off','-DCASE=1']+defines+[a.source,'-o',reference],name+'.ref.compile')
    got=run([exe],name+'.run');want=run([reference],name+'.ref.run')
    assert json.loads((keep/'25-plan.json').read_text())['offloaded']==[]
    assert stats(got.stderr)==(0,0,0) and bits(got.stdout)==bits(want.stdout)
    results.append(dict(limit=name,completed=0,full_buffer='PASS'))
# Fast recognition succeeds here, but neither lowering path may bypass the
# alias failure. The independent CPU reference uses an actual C/A alias.
alias_source=root/'alias.c'
alias_text=Path(a.source).read_text().replace('restrict ','')
assert alias_text.count('kernel(C,A,B);')==1
alias_source.write_text(alias_text.replace('kernel(C,A,B);','kernel(C,C,B);'))
name='alias-gate';exe=root/name;keep=root/(name+'.ir');reference=root/(name+'.reference')
run(a.cc+['--fp-reassociation','--verify','-DCASE=2','-DAUDIT_STATS',
          '--keep-dir',keep,alias_source,'-o',exe],name+'.compile')
run([a.clang,'-O0','-ffp-contract=off','-DCASE=2',alias_source,'-o',reference],name+'.ref.compile')
got=run([exe],name+'.run');want=run([reference],name+'.ref.run')
assert stats(got.stderr)==(0,0,0) and bits(got.stdout)==bits(want.stdout)
assert json.loads((keep/'25-plan.json').read_text())['offloaded']==[]
remarks=(keep/'20-remarks.txt').read_text()
assert 'may overlap C' in remarks and 'general affine factors may alias the output' in remarks
results.append(dict(safety='actual alias',completed=0,full_buffer='PASS'))
(root/'results.json').write_text(json.dumps(results,indent=2))
print('PASS: four cases, sync/async real completions, full-buffer bitwise CPU reference, exact physical MAC multiset, fault rollback, and strict FP refusal')
