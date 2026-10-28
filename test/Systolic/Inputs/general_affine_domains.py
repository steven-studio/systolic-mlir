"""Finite general-path coverage and conservative refusal, independent of reports."""
from collections import Counter,defaultdict
from pathlib import Path
import re
import subprocess
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[2]/'SystolicCC'/'Inputs'))
# The /tmp development copy also imports the already validated oracle.
from general_affine_oracle import dense_ir,source
opt,fixture,temp=sys.argv[1:4];root=Path(temp);root.mkdir(parents=True,exist_ok=True)
base=Path(fixture).read_text();base='\n'.join(x for x in base.splitlines() if not x.startswith('//'))
lines=base.splitlines();a=next(i for i,l in enumerate(lines) if '%t0 = affine.load' in l);b=next(i for i,l in enumerate(lines) if 'affine.store' in l)
guard=lines[:a]+['          affine.if affine_set<(d0,d1) : (-d0-d1+3 >= 0)>(%i,%j) {']+lines[a:b+1]+['          }']+lines[b+1:];guard='\n'.join(guard)
def repeated(count):
    start=base.index('    affine.for %i');end=base.index('    func.return')
    return base[:start]+f'    affine.for %p = 0 to {count} {{\n'+base[start:end]+'    }\n'+base[end:]
def call(name,text,lower=True):
    src=root/(name+'.mlir');dst=root/(name+'.out.mlir');src.write_text(text)
    flags=['--systolic-declare-fpga-fleet','--affine-matmul-to-systolic=general-affine=true report=true classify=true'] if lower else ['--test-affine-matmul-discovery=general-affine=true']
    p=subprocess.run([opt]+flags+[str(src),'-o',str(dst)],capture_output=True,text=True)
    (root/(name+'.log')).write_text(p.stderr);assert p.returncode==0,(name,p.stderr)
    return dst.read_text(),p.stderr

def positive(name,text,expected,points,fibers,n=16):
    ir,log=call(name,text);jobs,actual=dense_ir(ir,n)
    assert jobs>0 and Counter(actual)==Counter(expected),(name,jobs)
    def ordered(xs):
        f=defaultdict(list)
        for a,b,c in xs:f[c].append((a,b))
        return dict(f)
    assert ordered(actual)==ordered(expected),name
    assert f'source MACs={points}' in log and f'output fibers={fibers}' in log
    print(name,points,'exact MACs;',jobs,'dense jobs: PASS')

expected=source(1,16)
positive('base',base,expected,64,37)
g=[p for p in expected if ((p[0]//16+p[2]%16+p[0]%16-12)//3+(p[0]%16-p[2]%16+6)//3)<4]
positive('guard',guard,g,40,31)
positive('unused-repetition',repeated(2),expected*2,128,37)
# Replace C's second index only; no source witness is lost by the singular map.
singular=re.sub(r'(%C\[)(.*?)(\])',lambda m:m[1]+m[2].rsplit(',',1)[0]+', 0'+m[3],base)
positive('singular-output',singular,[(a,b,(c//16)*16) for a,b,c in expected],64,13)
# Different non-unimodular frames and independent constant offsets. These
# coefficients live only in this oracle; the compiler has no pattern table.
for variant,T in enumerate(([[2,1,-1],[1,-2,1],[1,1,2]],
                            [[1,2,1],[2,-1,1],[1,1,-2]])):
    def form(row,offset):
        terms=[str(offset)]
        for name,c in zip(('%i','%j','%k'),row):
            if c:terms.append((' + ' if c>0 else ' - ')+name+' * '+str(abs(c)))
        return ''.join(terms)
    forms={'%C':(form(T[0],12),form(T[1],12)),
           '%A':(form(T[0],13),form(T[2],12)),
           '%B':(form(T[2],14),form(T[1],15))}
    transformed=base.replace('16x16xf32','64x64xf32')
    transformed=re.sub(r'(%[CAB])\[.*?\]',lambda m:m[1]+'['+', '.join(forms[m[1]])+']',transformed)
    from itertools import product
    exp=[]
    for x in product(range(4),repeat=3):
        y=[sum(c*v for c,v in zip(row,x)) for row in T]
        exp.append(((y[0]+13)*64+y[2]+12,(y[2]+14)*64+y[1]+15,(y[0]+12)*64+y[1]+12))
    positive('frame-'+str(variant),transformed,exp,64,len({c for a,b,c in exp}),64)
_,discovery=call('discovery',base,False)
assert 'general recognition result: success' in discovery and 'source MACs: 64; output fibers: 37' in discovery
def dimensions(count):
    start=base.index('    affine.for %i');end=base.index('    func.return')
    prefix=''.join(f'    affine.for %p{d} = 0 to 1 {{\n' for d in range(count))
    return base[:start]+prefix+base[start:end]+'    }\n'*count+base[end:]
positive('iv-limit8',dimensions(5),expected,64,37)
negatives={
 'iv-limit9':dimensions(6),
 'alias':base.replace(' {llvm.noalias}',''),
 'strict':base.replace(' fastmath<reassoc>',''),
 'bounds':base.replace('16x16xf32','8x8xf32'),
 'budget':repeated(9),
 'nonunit':base.replace('affine.for %k = 0 to 4 {','affine.for %k = 0 to 4 step 2 {'),
 'symbol':base.replace(') {\n    affine.for',', %n: index) {\n    affine.for',1).replace('affine.for %i = 0 to 4 {','affine.for %i = 0 to %n {'),
 'side-effect':base.replace('module {','module {\n  func.func private @side_effect()').replace('          %t0 =','          func.call @side_effect() : () -> ()\n          %t0 ='),
 'else':guard.replace('          }','          } else {\n          }',1),
}
for name,text in negatives.items():
    ir,log=call(name,text)
    assert 'systolic.matmul_tile' not in ir,(name,'unsafe offload')
    assert 'general affine' in log,name
    print(name,'safe refusal: PASS')
