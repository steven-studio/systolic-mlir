"""Finding F-KC-4: the lowered kernel-coset offload must run in bounded stack.

The fix sits in --lower-systolic-matmul-tile-to-fpga, so the emission (and every
IR-level oracle) cannot see it. This test therefore executes the lowered kernel,
through mlir-translate, llc and a C driver, on the software runtime (NOT the
FPGA), and checks what the machine code does.

Structure, on hand-written fixtures through the lowering pass alone: a tile
slab leaves sequential loops only when one buffer can serve every iteration.
  moved    affine.for; scf.for with scf.if and affine.if around the slab
  bounded  scf.parallel and scf.while stop the move: iterations keep their own
  stays    a dynamic size, the address passed to a call or stored, a tensor
           view yielded out of its iteration, a memref.copy, no loop at all,
           and every slab under the asynchronous lowering, which keeps C until
           the wait

Execution, each case compared with an independent brute enumeration of the
source statement in this file, at several Kdev (a K=1 remainder, a K above k_max
that folds):
  * trace build: the runtime entry point is replaced by a recorder and every
    element of A, B and C holds its own unique value, so the operands the
    kernel really hands over name their physical addresses. The recorded
    (A, B, C) multiset must equal the source's, per output too, and C must
    still hold its ids afterwards: nothing written anywhere else;
  * compute build: real software runtime, full C bitwise equal to the source
    loop run in the same process;
  * stack: an exact high-water mark (paint below the frame, scan up after the
    call) must not grow with the iteration count;
  * constrained run: 66560 MACs inside a 1 MiB stack, which the former code
    needed about 2.1 MiB for.
Async: the lowering still refuses these tiles (they carry no systolic.gemm).
"""
import argparse
from collections import Counter, defaultdict
from pathlib import Path
import re
import resource
import subprocess
import sys

p = argparse.ArgumentParser()
p.add_argument('offload'); p.add_argument('out'); p.add_argument('cc')
p.add_argument('--runtime', default=''); p.add_argument('--clang', required=True)
p.add_argument('--build-dir', required=True); p.add_argument('--llvm-build', required=True)
args = p.parse_args()
root = Path(args.out); root.mkdir(parents=True, exist_ok=True)
OPT = Path(args.build_dir) / 'bin' / 'systolic-opt'
TRANSLATE = Path(args.llvm_build) / 'bin' / 'mlir-translate'
LLC = Path(args.llvm_build) / 'bin' / 'llc'
CLANG = args.clang


def run(cmd, what, **kw):
    r = subprocess.run(list(map(str, cmd)), capture_output=True, text=True, **kw)
    assert r.returncode == 0, (what, r.returncode, r.stdout[-400:], r.stderr[-800:])
    return r


# ---- structure ------------------------------------------------------------
DEVICES = '\n'.join(
    f'  systolic.device @{n} rows = {r} cols = {r} dataflow = output_stationary '
    'k_max = 16 tile_overhead = 95'
    for n, r in (('acc_8x8_0', 8), ('acc_4x4_0', 4), ('acc_4x4_1', 4), ('acc_4x4_2', 4)))
T = 'tensor<1x1xf32>'; M = 'memref<1x1xf32>'
ALLOCAS = '\n'.join(f'%{n} = memref.alloca() : {M}' for n in ('sa', 'sb', 'sc'))


def tile(a='%sa', aty=M, attrs=''):
    """Fill the slabs, one tile, write C back: the emission's shape."""
    ty = aty.replace('memref', 'tensor')
    return f"""memref.store %x, {a}[%c0, %c0] : {aty}
memref.store %x, %sb[%c0, %c0] : {M}
memref.store %x, %sc[%c0, %c0] : {M}
%ta = bufferization.to_tensor {a} restrict : {aty} to {ty}
%tb = bufferization.to_tensor %sb restrict : {M} to {T}
%tc = bufferization.to_tensor %sc restrict writable : {M} to {T}
%xa = tensor.extract_slice %ta[0, 0] [1, 1] [1, 1] : {ty} to {T}
%xb = tensor.extract_slice %tb[0, 0] [1, 1] [1, 1] : {T} to {T}
%xc = tensor.extract_slice %tc[0, 0] [1, 1] [1, 1] : {T} to {T}
%r = systolic.matmul_tile %xa, %xb, %xc m = 1 n = 1 k = 1 on @acc_4x4_0 est_cycles = 102 start_cycle = 0 {attrs}: ({T}, {T}, {T}) -> {T}
%i = tensor.insert_slice %r into %tc[0, 0] [1, 1] [1, 1] : {T} into {T}
bufferization.materialize_in_destination %i in restrict writable %sc : ({T}, {M}) -> ()"""


LOOP = 'scf.for %q = %lb to %ub step %st {'
PLAIN = ALLOCAS + '\n' + tile()
BODIES = {
    'moved_affine': f'affine.for %q = 0 to 4 {{\naffine.for %w = 0 to 4 {{\n{PLAIN}\n}}\n}}',
    'moved_conditionals': f'{LOOP}\nscf.if %b {{\naffine.if affine_set<(d0) : (d0 >= 0)>(%n) {{\n{PLAIN}\n}}\n}}\n}}',
    'bounded_parallel': f'scf.parallel (%p) = (%lb) to (%ub) step (%st) {{\n{LOOP}\n{PLAIN}\n}}\nscf.reduce\n}}',
    'bounded_while': f'scf.while : () -> () {{\nscf.condition(%b)\n}} do {{\n{LOOP}\n{PLAIN}\n}}\nscf.yield\n}}',
    'stays_parallel_body': f'scf.parallel (%p) = (%lb) to (%ub) step (%st) {{\n{PLAIN}\nscf.reduce\n}}',
    'stays_dynamic': f'{LOOP}\n%sa = memref.alloca(%n) : memref<?x1xf32>\n%sb = memref.alloca() : {M}\n%sc = memref.alloca() : {M}\n{tile(aty="memref<?x1xf32>")}\n}}',
    'stays_call': f'{LOOP}\n{PLAIN}\nfunc.call @keep(%sa) : ({M}) -> ()\n}}',
    'stays_stored': f'{LOOP}\n{PLAIN}\nmemref.store %sa, %box[] : memref<{M}>\n}}',
    'stays_copied': f'{LOOP}\n{PLAIN}\nmemref.copy %sa, %other : {M} to {M}\n}}',
    'stays_no_loop': f'%before = arith.constant 7 : i32\n{PLAIN}',
}
# A view that leaves its iteration: the buffer must stay this iteration's.
YIELD = f"""%init = tensor.empty() : {T}
%out = scf.for %q = %lb to %ub step %st iter_args(%acc = %init) -> ({T}) {{
{ALLOCAS}
{tile()}
%view = bufferization.to_tensor %sa restrict : {M} to {T}
scf.yield %view : {T}
}}"""
BODIES['stays_yielded'] = YIELD
HEADER = (f'%x: f32, %n: index, %lb: index, %ub: index, %st: index, %b: i1, '
          f'%box: memref<{M}>, %other: {M}')
fixture = 'module {\n' + DEVICES + f'\n  func.func private @keep({M})\n'
for name, body in BODIES.items():
    fixture += f'  func.func @{name}({HEADER}) {{\n%c0 = arith.constant 0 : index\n{body}\nreturn\n}}\n'
fixture += '}\n'
src = root / 'structure.mlir'; src.write_text(fixture)
lowered = run([OPT, '--lower-systolic-matmul-tile-to-fpga', src], 'structure lowering').stdout


def slabs(text):
    """{function: [(shape, [enclosing ops below the function])]}, for the slabs
    (the pass's own 128/64 scratch buffers are not tile slabs)."""
    found = defaultdict(list); fn = None; stack = []
    for line in text.splitlines():
        s = line.strip()
        m = re.match(r'func\.func @(\w+)', s)
        if m:
            fn = m[1]
            if s.endswith('{'):
                stack = []
            continue
        if fn is None:
            continue
        if s.startswith('}'):
            if not stack:
                fn = None
                continue
            closed = stack.pop()
            if s.endswith('{'):            # "} do {", "} else {": the same op
                stack.append(closed)
            continue
        a = re.search(r'memref\.alloca\((.*?)\) : memref<([^>]*)>', s)
        if a and a[2] in ('1x1xf32', '?x1xf32'):
            found[fn].append((a[2], list(stack)))
        if s.endswith('{'):
            stack.append(s.split()[0] if '=' not in s.split()[:2] else s.split()[2])
    return found


got = slabs(lowered)
MOVED = [('1x1xf32', [])] * 3
expect = {
    'moved_affine': MOVED,
    'moved_conditionals': MOVED,
    'bounded_parallel': [('1x1xf32', ['scf.parallel'])] * 3,
    'bounded_while': [('1x1xf32', ['scf.while'])] * 3,
    'stays_parallel_body': [('1x1xf32', ['scf.parallel'])] * 3,
    'stays_dynamic': [('?x1xf32', ['scf.for']), ('1x1xf32', []), ('1x1xf32', [])],
    'stays_call': [('1x1xf32', []), ('1x1xf32', []), ('1x1xf32', ['scf.for'])],
    'stays_stored': [('1x1xf32', []), ('1x1xf32', []), ('1x1xf32', ['scf.for'])],
    'stays_copied': [('1x1xf32', []), ('1x1xf32', []), ('1x1xf32', ['scf.for'])],
    'stays_no_loop': MOVED,
    'stays_yielded': [('1x1xf32', []), ('1x1xf32', []), ('1x1xf32', ['scf.for'])],
}
for name, want in expect.items():
    assert sorted(got[name], key=str) == sorted(want, key=str), (name, got[name], want)
body = lowered[lowered.index('@stays_no_loop'):]
assert body.index('arith.constant 7') < body.index('memref.alloca() : ' + M), 'a slab outside any loop must not move'
print(f'structure: {len(expect)} fixtures: moved out of sequential loops only, stopped by parallel and while regions, '
      'left alone when dynamic, escaping, yielded or outside a loop: PASS')

# The asynchronous lowering keeps C until the wait: it must move nothing.
ATTRS = '{systolic.gemm = 0 : i64, systolic.k_count = 1 : i64, systolic.k_index = 0 : i64, systolic.tile = 0 : i64} '
asy = root / 'async.mlir'
asy.write_text('module {\n' + DEVICES + f'\n  func.func @a(%x: f32) {{\n%c0 = arith.constant 0 : index\n'
               f'affine.for %q = 0 to 1 {{\n{ALLOCAS}\n{tile(attrs=ATTRS)}\n}}\nreturn\n}}\n}}\n')
out = run([OPT, '--lower-systolic-matmul-tile-to-fpga=async=true', asy], 'async lowering').stdout
assert slabs(out)['a'] == [('1x1xf32', ['affine.for'])] * 3, slabs(out)
out = run([OPT, '--lower-systolic-matmul-tile-to-fpga', asy], 'sync lowering').stdout
assert slabs(out)['a'] == MOVED
print('async: no slab moved under the asynchronous lowering (the same module hoists when synchronous): PASS')


# ---- execution ------------------------------------------------------------
def shape(s):
    return tuple(int(v) for v in s.split('x'))


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        assert 0 <= v < sz, (idx, shp)
        a += v * stride; stride *= sz
    return a


class Case:
    def __init__(self, name, cshape, ashape, bshape, loops, closes, cidx, aidx, bidx, points, cfn, afn, bfn, cloops):
        self.name, self.shapes = name, (cshape, ashape, bshape)
        self.loops, self.closes, self.idx = loops, closes, (cidx, aidx, bidx)
        self.points, self.fns, self.cloops = points, (cfn, afn, bfn), cloops

    def mlir(self):
        c, a, b = self.shapes; ci, ai, bi = self.idx
        return f"""module {{
  func.func @kern(%C: memref<{c}xf32> {{llvm.noalias}}, %A: memref<{a}xf32> {{llvm.noalias}}, %B: memref<{b}xf32> {{llvm.noalias}}) {{
    {self.loops}
      %t0 = affine.load %C[{ci}] : memref<{c}xf32>
      %t1 = affine.load %A[{ai}] : memref<{a}xf32>
      %t2 = affine.load %B[{bi}] : memref<{b}xf32>
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[{ci}] : memref<{c}xf32>
    {self.closes}
    func.return
  }}
}}
"""

    def brute(self):
        cs, asp, bs = (shape(s) for s in self.shapes)
        macs = Counter()
        for x in self.points():
            macs[(lin(self.fns[1](*x), asp), lin(self.fns[2](*x), bs), lin(self.fns[0](*x), cs))] += 1
        return macs

    def driver(self):
        cs, asp, bs = (shape(s) for s in self.shapes)
        def desc(n, s):
            st, acc = [], 1
            for v in reversed(s):
                st.insert(0, acc); acc *= v
            return ', '.join([f'(float *){n}', f'(float *){n}', '0'] + [str(v) for v in s] + [str(v) for v in st])
        def proto(s):
            return ', '.join(['float *', 'float *'] + ['int64_t'] * (1 + 2 * len(s)))
        count = lambda s: ' * '.join(map(str, s))
        dims = lambda s: ''.join(f'[{v}]' for v in s)
        ci, ai, bi = self.c_index()
        return f"""#include <alloca.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
void kern({proto(cs)}, {proto(asp)}, {proto(bs)});
static float A{dims(asp)}, B{dims(bs)}, C{dims(cs)}, Ref{dims(cs)};
#define NA ({count(asp)})
#define NB ({count(bs)})
#define NC ({count(cs)})
#ifdef TRACE
int fpga_matmul_tiled_auto_scheduled(int m, int k, int n, const float *a, const float *b, float *c,
                                     int d, int s, int e) {{
  for (int t = 0; t < k; ++t) printf("%d %d %d\\n", (int)a[t], (int)b[t], (int)c[0]);
  return 0;
}}
int main(void) {{
  for (int i = 0; i < NA; ++i) ((float *)A)[i] = 1 + i;
  for (int i = 0; i < NB; ++i) ((float *)B)[i] = 1 + i;
  for (int i = 0; i < NC; ++i) ((float *)C)[i] = 1 + i;
  kern({desc('C', cs)}, {desc('A', asp)}, {desc('B', bs)});
  int intact = 1;
  for (int i = 0; i < NC; ++i) intact &= ((float *)C)[i] == 1 + i;
  printf("intact %d\\n", intact);
  return 0;
}}
#else
#include "systolic_offload.h"
#ifndef NOPAINT
#define PAINT (4u << 20)
static volatile char *low;
__attribute__((noinline)) static void paint(void) {{ char *b = alloca(PAINT); memset(b, 0xA5, PAINT); low = b; }}
#endif
int main(void) {{
  for (int i = 0; i < NA; ++i) ((float *)A)[i] = (float)((i * 3 + 1) % 7 - 3);
  for (int i = 0; i < NB; ++i) ((float *)B)[i] = (float)((i * 5 + 2) % 11 - 5);
  for (int i = 0; i < NC; ++i) ((float *)C)[i] = ((float *)Ref)[i] = (float)((i * 2) % 5 - 2);
  {self.cloops} Ref{ci} += A{ai} * B{bi};
#ifndef NOPAINT
  char *top = __builtin_frame_address(0);
  paint();
#endif
  kern({desc('C', cs)}, {desc('A', asp)}, {desc('B', bs)});
  size_t used = 0;
#ifndef NOPAINT
  size_t i = 0;
  while (i < PAINT && low[i] == (char)0xA5) ++i;
  used = (size_t)(top - (const char *)low - i);
#endif
  systolic_offload_stats_t st; systolic_offload_stats(&st);
  unsigned long long jobs = 0;
  for (int d = 0; d < 4; ++d) jobs += st.per_device[d];
  printf("%llu %llu %d %zu\\n", jobs, (unsigned long long)st.failed, memcmp(C, Ref, sizeof C) == 0, used);
  systolic_offload_close();
  return 0;
}}
#endif
"""

    def c_index(self):
        return tuple(''.join(f'[{re.sub(r"%", "", p.strip())}]' for p in i.split(',')) for i in self.idx)


def triangle(R, K):
    """C[2i+3j] over j <= i: a holey image, the guarded rank-1 path."""
    return Case(f'tri{R}x{K}', str(5 * R - 4), f'{R}x{K}', f'{K}x{R}',
                f'affine.for %i = 0 to {R} {{ affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {{ affine.for %k = 0 to {K} {{',
                '}}}', '2 * %i + 3 * %j', '%i, %k', '%k, %j',
                lambda: ((i, j, k) for i in range(R) for j in range(i + 1) for k in range(K)),
                lambda i, j, k: (2 * i + 3 * j,), lambda i, j, k: (i, k), lambda i, j, k: (k, j),
                f'for (int i = 0; i < {R}; ++i) for (int j = 0; j <= i; ++j) for (int k = 0; k < {K}; ++k)')


def skew(R, K):
    """C[i+j-2k+o] over a box: a contiguous image, the committed M1 path."""
    o = 2 * (K - 1)
    return Case(f'skew{R}x{K}', str(2 * (R - 1) + o + 1), f'{R}x{K}', f'{K}x{R}',
                f'affine.for %i = 0 to {R} {{ affine.for %j = 0 to {R} {{ affine.for %k = 0 to {K} {{',
                '}}}', f'%i + %j - 2 * %k + {o}', '%i, %k', '%k, %j',
                lambda: ((i, j, k) for i in range(R) for j in range(R) for k in range(K)),
                lambda i, j, k: (i + j - 2 * k + o,), lambda i, j, k: (i, k), lambda i, j, k: (k, j),
                f'for (int i = 0; i < {R}; ++i) for (int j = 0; j < {R}; ++j) for (int k = 0; k < {K}; ++k)')


def rank2(R, K):
    """C[2i+3j][i-j+o] over j <= i: a rank-2 image with its own scan levels."""
    o = R - 1
    return Case(f'rank2_{R}x{K}', f'{5 * R - 4}x{2 * R - 1}', f'{R}x{K}', f'{K}x{R}',
                f'affine.for %i = 0 to {R} {{ affine.for %j = 0 to affine_map<(d0) -> (d0 + 1)>(%i) {{ affine.for %k = 0 to {K} {{',
                '}}}', f'2 * %i + 3 * %j, %i - %j + {o}', '%i, %k', '%k, %j',
                lambda: ((i, j, k) for i in range(R) for j in range(i + 1) for k in range(K)),
                lambda i, j, k: (2 * i + 3 * j, i - j + o), lambda i, j, k: (i, k), lambda i, j, k: (k, j),
                f'for (int i = 0; i < {R}; ++i) for (int j = 0; j <= i; ++j) for (int k = 0; k < {K}; ++k)')


COMMON = root / 'soft_runtime.o'
run([CLANG, '-std=c11', '-O1', '-c', f'-I{args.offload}', '-DSOFT_DEFAULT_K_MAX_8X8=16', '-DSOFT_DEFAULT_K_MAX_4X4=16',
     Path(args.offload) / 'soft_runtime.c', '-o', COMMON], 'soft runtime')


def compile_kernel(case, kdev):
    stem = root / f'{case.name}_k{kdev}'
    src = stem.with_suffix('.mlir'); src.write_text(case.mlir())
    ir = run([OPT, '--systolic-declare-fpga-fleet',
              '--affine-matmul-to-systolic=general-affine=true allow-fp-reassociation=true report=true '
              f'kernel-coset-offload=true kernel-coset-kdev={kdev}', src], 'emission')
    assert 'lowered by the experimental symbolic kernel-coset offload' in ir.stderr, (case.name, ir.stderr[-500:])
    assert 'systolic.matmul_tile' in ir.stdout and 'arith.mulf' not in ir.stdout
    em = Path(f'{stem}.em.mlir'); em.write_text(ir.stdout)
    low = run([OPT, '--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=16 phys-k-max-4x4=16', em], 'tile lowering').stdout
    assert all(not chain for _, chain in slabs(low)['kern']) and 'memref.alloca' in low, (case.name, 'a slab stayed in a loop')
    fpga = Path(f'{stem}.fpga.mlir'); fpga.write_text(low)
    llvm = run([OPT, '--systolic-runtime-to-llvm', fpga], 'runtime to llvm').stdout
    ll = run([TRANSLATE, '--mlir-to-llvmir'], 'translate', input=llvm).stdout
    assert 'stacksave' not in ll
    entry = ll[ll.index('define'):]
    after_entry = re.split(r'\n\d+:', entry, maxsplit=1)
    assert len(after_entry) == 1 or ' alloca ' not in after_entry[1], (case.name, 'an alloca outside the entry block')
    obj = Path(f'{stem}.o')
    run([LLC, '-O0', '-filetype=obj', '-relocation-model=pic', '-o', obj], 'llc', input=ll)
    return obj


def link(case, kdev, obj, *defs, runtime=True):
    stem = root / f'{case.name}_k{kdev}'
    drv = Path(f'{stem}.c'); drv.write_text(case.driver())
    exe = Path(f'{stem}{"".join(d.replace("-D", ".") for d in defs)}.exe')
    run([CLANG, '-std=c11', '-O1', '-ffp-contract=off', '-fno-stack-protector', f'-I{args.offload}', *defs, drv, obj]
        + ([COMMON] if runtime else []) + ['-o', exe], 'link')
    return exe


def stack_limit(bytes_):
    return lambda: resource.setrlimit(resource.RLIMIT_STACK, (bytes_, resource.getrlimit(resource.RLIMIT_STACK)[1]))


def exercise(case, kdev):
    obj = compile_kernel(case, kdev)
    # What the machine code hands over, against the source.
    out = run([link(case, kdev, obj, '-DTRACE', runtime=False)], 'trace run').stdout.splitlines()
    assert out[-1] == 'intact 1', (case.name, kdev, 'C changed outside the recorder')
    got_macs = Counter(tuple(int(v) - 1 for v in l.split()) for l in out[:-1])
    want = case.brute()
    assert got_macs == want, (case.name, kdev, 'MAC multiset', sum((got_macs - want).values()), sum((want - got_macs).values()))
    per_got, per_want = Counter(), Counter()
    for (a, b, c), n in got_macs.items():
        per_got[c] += n
    for (a, b, c), n in want.items():
        per_want[c] += n
    assert per_got == per_want, (case.name, kdev, 'per-output multiplicities')
    # The reduction itself, and the stack it took.
    r = run([link(case, kdev, obj)], 'compute run', preexec_fn=stack_limit(64 << 20)).stdout.split()
    jobs, failed, same, used = (int(v) for v in r)
    assert failed == 0 and same == 1 and jobs > 0, (case.name, kdev, r)
    print(f'{case.name} Kdev {kdev}: {sum(want.values())} MACs on {len(per_want)} outputs in {jobs} jobs: physical multiset, '
          f'per-output counts, C bitwise: PASS (stack {used} bytes)')
    return used


high = {}
for case in (triangle(16, 24), skew(10, 24), rank2(12, 24)):
    for kdev in (1, 2, 5, 16, 20):
        high[case.name, kdev] = exercise(case, kdev)

# Bounded: the same shape with far more iterations takes the same stack.
for kdev in (1, 4):
    small = exercise(triangle(12, 6), kdev)
    large = exercise(triangle(64, 32), kdev)
    assert abs(large - small) <= 256, (kdev, small, large, 'stack grew with the iteration count')
    print(f'bounded, Kdev {kdev}: {small} bytes for 468 MACs, {large} bytes for 66560 MACs: PASS')

# The fault itself: 66560 MACs at 32 bytes each is 2.1 MiB of the former stack.
case = triangle(64, 32)
exe = link(case, 1, Path(root / f'{case.name}_k1.o'), '-DNOPAINT')
jobs, failed, same, _ = (int(v) for v in run([exe], 'constrained run', preexec_fn=stack_limit(1 << 20)).stdout.split())
assert jobs == 66560 and failed == 0 and same == 1
print('constrained: 66560 MACs complete, bitwise equal, inside a 1 MiB stack: PASS')

# The asynchronous lowering still refuses kernel-coset tiles.
em = root / f'{triangle(12, 6).name}_k1.em.mlir'
r = subprocess.run([str(OPT), '--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=16 phys-k-max-4x4=16 async=true', str(em)],
                   capture_output=True, text=True)
assert r.returncode != 0 and 'needs the systolic.gemm' in r.stderr, r.stderr[-300:]
print('async: kernel-coset tiles stay refused by the asynchronous lowering: PASS')
