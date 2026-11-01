"""Phase F-KC-5b-0: emitted arithmetic must fit in 64 bits.

Fourier-Motzkin elimination multiplies coefficients. From P*k <= i and j <= Q*k
it derives P*j <= Q*i, so the j loop gets the upper face floor(Q*i / P). Every
coefficient fits in 64 bits and the affine map is right, but Q*i exists only at
run time, as `mul nsw i64`. Past 2^63 the emission lost all 55 MACs of the nest
below, with exit 0, although nothing the source computes exceeds 43 bits and the
memrefs are 10 elements on a side.

The IR interpreter uses Python integers, so it cannot see this: it calls that
emission exact. So this test judges machine code. Each nest is enumerated
independently here, with exact integers, then:

  source    the flag-off IR, run as machine code, must give the enumerated C;
  lowered   the emission, run as machine code at -O0 and -O2. A recorder replaces
            the runtime entry point, and unique element values name the operands
            really handed over, so the physical (A, B, C) multiset and the
            per-output multiplicities are compared with the enumeration. A second
            build computes C on the software runtime, compared bitwise. The
            interpreter adds the C load and store addresses: each output exactly
            once, nothing else;
  refused   the expected reason, IR byte-identical to the flag-off output.

An expectation can depend on Kdev: the full-block test adds a term of its own.
"""
import argparse
from collections import Counter
from itertools import product
import os
from pathlib import Path
import re
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs

p = argparse.ArgumentParser()
p.add_argument('offload'); p.add_argument('out'); p.add_argument('cc')
p.add_argument('--runtime', default=''); p.add_argument('--clang', required=True)
p.add_argument('--build-dir', required=True); p.add_argument('--llvm-build', required=True)
args = p.parse_args()
root = Path(args.out); root.mkdir(parents=True, exist_ok=True)
OPT = Path(os.environ.get('KERNEL_COSET_OVERFLOW_OPT', Path(args.build_dir) / 'bin' / 'systolic-opt'))
TRANSLATE = Path(args.llvm_build) / 'bin' / 'mlir-translate'
LLC = Path(args.llvm_build) / 'bin' / 'llc'
CLANG = args.clang

LOWERED = 'lowered by the experimental symbolic kernel-coset offload'
RANGE = 'emitted arithmetic not proven to fit in 64 bits'
BOUND = 'a bound of reduced column'
TEST = 'the full-block test'
FLAGS = 'general-affine=true report=true'
INT64_MAX = 2**63 - 1
INT64_MIN = -2**63
W, D = 10, 6


def run(cmd, what, **kw):
    r = subprocess.run(list(map(str, cmd)), capture_output=True, text=True, **kw)
    assert r.returncode == 0, (what, r.returncode, r.stderr[-500:])
    return r


def form(coeffs, constant, names):
    terms = [f'{c} * {names[v]}' for v, c in coeffs.items() if c]
    return ' + '.join(terms + [str(constant)])


class Case:
    """loops: (variable, low, high) outermost first. ge, eq: ({var: coeff}, constant)
    rows of the guard, >= 0 and == 0. c, a, b: one such form per memref dimension.
    expect: 'lowered', or a reason text; or {kdev: either}. hide_outer puts a
    redundant affine.if under the outer loop, which hides that loop from the
    candidate scan: the way a four-loop nest reaches this path."""

    def __init__(self, name, loops, ge, eq, c, a, b, expect, reason=None, hide_outer=False):
        self.name, self.loops, self.ge, self.eq = name, loops, ge, eq
        self.c, self.a, self.b, self.expect, self.reason = c, a, b, expect, reason
        self.hide_outer = hide_outer
        self.vars = [v for v, _, _ in loops]
        self.points = [dict(zip(self.vars, x)) for x in product(*[range(lo, hi) for _, lo, hi in loops])
                       if all(self.value(r, dict(zip(self.vars, x))) >= 0 for r in ge)
                       and all(self.value(r, dict(zip(self.vars, x))) == 0 for r in eq)]
        assert self.points, name
        self.shapes = tuple(tuple(max(self.value(r, x) for x in self.points) + 1 for r in op) for op in (c, a, b))

    @staticmethod
    def value(row, x):
        return sum(c * x[v] for v, c in row[0].items()) + row[1]

    def text(self):
        names = {v: f'%{v}' for v in self.vars}
        dims = {v: f'd{i}' for i, v in enumerate(self.vars)}
        ty = [f'memref<{"x".join(map(str, s))}xf32>' for s in self.shapes]
        idx = [', '.join(form(c, k, names) for c, k in op) for op in (self.c, self.a, self.b)]
        rows = [form(c, k, dims) + ' >= 0' for c, k in self.ge] + [form(c, k, dims) + ' == 0' for c, k in self.eq]
        guard = ''
        if rows:
            ds = ', '.join(dims[v] for v in self.vars); ss = ', '.join(names[v] for v in self.vars)
            guard = f'affine.if affine_set<({ds}) : ({", ".join(rows)})>({ss}) {{'
        loops = [f'affine.for %{v} = {lo} to {hi} {{' for v, lo, hi in self.loops]
        if self.hide_outer:
            v, lo, _ = self.loops[0]
            loops.insert(1, f'affine.if affine_set<(d0) : (d0 + {-lo} >= 0)>(%{v}) {{')
        loops = ' '.join(loops)
        closes = '}' * (len(self.loops) + (1 if rows else 0) + (1 if self.hide_outer else 0))
        return f"""module {{
  func.func @kern(%C: {ty[0]} {{llvm.noalias}}, %A: {ty[1]} {{llvm.noalias}}, %B: {ty[2]} {{llvm.noalias}}) {{
    {loops} {guard}
      %t0 = affine.load %C[{idx[0]}] : {ty[0]}
      %t1 = affine.load %A[{idx[1]}] : {ty[1]}
      %t2 = affine.load %B[{idx[2]}] : {ty[2]}
      %t3 = arith.mulf %t1, %t2 fastmath<reassoc> : f32
      %t4 = arith.addf %t0, %t3 fastmath<reassoc> : f32
      affine.store %t4, %C[{idx[0]}] : {ty[0]}
    {closes}
    func.return
  }}
}}
"""

    def macs(self):
        cs, asp, bs = self.shapes
        m = Counter()
        for x in self.points:
            m[(lin([self.value(r, x) for r in self.a], asp), lin([self.value(r, x) for r in self.b], bs),
               lin([self.value(r, x) for r in self.c], cs))] += 1
        return m

    def expected(self, kdev):
        return self.expect[kdev] if isinstance(self.expect, dict) else self.expect


def lin(idx, shp):
    a, stride = 0, 1
    for v, sz in zip(reversed(idx), reversed(shp)):
        assert 0 <= v < sz, (idx, shp)
        a += v * stride; stride *= sz
    return a


def size(shp):
    n = 1
    for v in shp:
        n *= v
    return n


def driver(case):
    cs, asp, bs = case.shapes

    def desc(n, s):
        st, acc = [], 1
        for v in reversed(s):
            st.insert(0, acc); acc *= v
        return ', '.join([f'(float *){n}', f'(float *){n}', '0'] + [str(v) for v in s] + [str(v) for v in st])

    def proto(s):
        return ', '.join(['float *', 'float *'] + ['int64_t'] * (1 + 2 * len(s)))

    dims = lambda s: ''.join(f'[{v}]' for v in s)
    return f"""#include <stdint.h>
#include <stdio.h>
void kern({proto(cs)}, {proto(asp)}, {proto(bs)});
static float A{dims(asp)}, B{dims(bs)}, C{dims(cs)};
#define NA {size(asp)}
#define NB {size(bs)}
#define NC {size(cs)}
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
int main(void) {{
  for (int i = 0; i < NA; ++i) ((float *)A)[i] = (float)((i * 3 + 1) % 7 - 3);
  for (int i = 0; i < NB; ++i) ((float *)B)[i] = (float)((i * 5 + 2) % 11 - 5);
  for (int i = 0; i < NC; ++i) ((float *)C)[i] = (float)((i * 2) % 5 - 2);
  kern({desc('C', cs)}, {desc('A', asp)}, {desc('B', bs)});
  for (int i = 0; i < NC; ++i) printf("%.0f\\n", ((float *)C)[i]);
  systolic_offload_close();
  return 0;
}}
#endif
"""


def reference(case):
    cs, asp, bs = case.shapes
    A = [(i * 3 + 1) % 7 - 3 for i in range(size(asp))]
    B = [(i * 5 + 2) % 11 - 5 for i in range(size(bs))]
    C = [(i * 2) % 5 - 2 for i in range(size(cs))]
    for (a, b, c), n in case.macs().items():
        C[c] += n * A[a] * B[b]
    return C


SOFT = root / 'soft_runtime.o'
run([CLANG, '-std=c11', '-O1', '-c', f'-I{args.offload}', '-DSOFT_DEFAULT_K_MAX_8X8=16', '-DSOFT_DEFAULT_K_MAX_4X4=16',
     Path(args.offload) / 'soft_runtime.c', '-o', SOFT], 'soft runtime')


def machine(stem, ir):
    """One module as machine code at -O0 and at -O2."""
    src = Path(f'{stem}.ir.mlir'); src.write_text(ir)
    low = run([OPT, '--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=16 phys-k-max-4x4=16', src], 'tile lowering').stdout
    fpga = Path(f'{stem}.fpga.mlir'); fpga.write_text(low)
    llvm = run([OPT, '--systolic-runtime-to-llvm', fpga], 'runtime to llvm').stdout
    ll = Path(f'{stem}.ll'); ll.write_text(run([TRANSLATE, '--mlir-to-llvmir'], 'translate', input=llvm).stdout)
    o0, o2 = Path(f'{stem}.o0.o'), Path(f'{stem}.o2.o')
    run([LLC, '-O0', '-filetype=obj', '-relocation-model=pic', ll, '-o', o0], 'llc')
    run([CLANG, '-O2', '-Wno-override-module', '-c', '-x', 'ir', ll, '-o', o2], 'clang -O2')
    return o0, o2


def execute(stem, case, obj, *defs, runtime=True):
    drv = Path(f'{stem}.c')
    drv.write_text(driver(case))
    exe = Path(f'{obj}{"".join(d.replace("-D", ".") for d in defs)}.exe')
    run([CLANG, '-std=c11', '-O1', '-ffp-contract=off', f'-I{args.offload}', *defs, drv, obj]
        + ([SOFT] if runtime else []) + ['-o', exe], 'link')
    return run([exe], 'execution').stdout.split('\n')


def compile_(path, flag, kdev):
    opts = FLAGS + (f' kernel-coset-offload=true kernel-coset-kdev={kdev}' if flag else '')
    return run([OPT, '--systolic-declare-fpga-fleet', '--affine-matmul-to-systolic=' + opts, path], 'compile')


def check(case, kdev):
    stem = root / f'{case.name}_k{kdev}'
    path = Path(f'{stem}.mlir'); path.write_text(case.text())
    on, off = compile_(path, True, kdev), compile_(path, False, kdev)
    expected = case.expected(kdev)
    want = case.macs()
    C = [str(v) for v in reference(case)]

    # The source: the flag-off IR, as machine code, against the enumeration. With no
    # tile in it, the fleet's declarations have no lowering and mean nothing.
    for obj in machine(f'{stem}.src', re.sub(r'^\s*systolic\.device .*\n', '', off.stdout, flags=re.M)):
        got = [s for s in execute(f'{stem}.src', case, obj) if s]
        assert got == C, (case.name, kdev, 'source machine code differs from the enumeration', obj.name)

    if expected != 'lowered':
        assert LOWERED not in on.stderr, (case.name, kdev, 'lowered, expected a refusal', on.stderr[-300:])
        assert expected in on.stderr, (case.name, kdev, 'reason', expected, on.stderr[-400:])
        assert on.stdout == off.stdout, (case.name, kdev, 'a refusal changed the IR')
        return f'refused, IR identical, source machine code exact, {sum(want.values())} MACs kept'

    assert LOWERED in on.stderr, (case.name, kdev, 'expected the lowering', on.stderr[-500:])
    assert 'arith.mulf' not in on.stdout
    trace = {'load': [], 'store': []}
    _, macs = kernel_coset_macs(on.stdout, trace)
    assert Counter(macs) == want, (case.name, kdev, 'interpreter multiset')
    image = {c for _, _, c in want}
    stores = Counter(a for role, a in trace['store'] if role == 0)
    loads = Counter(a for role, a in trace['load'] if role == 0)
    assert set(stores) == image and set(loads) == image, (case.name, kdev, 'C addresses')
    assert all(v == 1 for v in stores.values()) and all(v == 1 for v in loads.values()), (case.name, kdev, 'C visits')

    per_output = Counter()
    for (_, _, c), n in want.items():
        per_output[c] += n
    for obj in machine(str(stem), on.stdout):
        lines = [s for s in execute(str(stem), case, obj, '-DTRACE', runtime=False) if s]
        assert lines[-1] == 'intact 1', (case.name, kdev, obj.name, 'C changed outside the recorder')
        got = Counter(tuple(int(v) - 1 for v in s.split()) for s in lines[:-1])
        assert got == want, (case.name, kdev, obj.name, 'machine-code multiset',
                             sum((want - got).values()), 'lost', sum((got - want).values()), 'extra')
        seen = Counter()
        for (_, _, c), n in got.items():
            seen[c] += n
        assert seen == per_output, (case.name, kdev, obj.name, 'per-output multiplicity')
        computed = [s for s in execute(str(stem), case, obj) if s]
        assert computed == C, (case.name, kdev, obj.name, 'computed C')
    return f'lowered, {sum(want.values())} MACs on {len(image)} outputs exact at -O0 and -O2'


P, Q = 4194301, 4194319


def base(name, I0, J0, KL, ge, eq, expect, reason=None, cc=None, cut=None):
    """A non-box nest with a transposed A, over the enumeration budget, so that no
    earlier route takes it. The windows are small, the coordinates need not be."""
    loops = [('i', I0, I0 + W), ('j', J0, J0 + W), ('k', KL, KL + D)]
    tri = cut or ({'i': 1, 'j': -1}, J0 - I0)
    c = cc or [({'i': 1}, -I0), ({'j': 1}, -J0)]
    return Case(name, loops, [tri] + ge, eq, c, [({'k': 1}, -KL), ({'i': 1}, -I0)],
                [({'k': 1}, -KL), ({'j': 1}, -J0)], expect, reason)


def upper_face(name, numerator, expect, p_=P, q_=Q):
    """j <= floor(Q*i / P): a wrapped UPPER face loses outputs."""
    I0 = numerator // q_ + 1
    K0 = I0 // p_
    return base(name, I0, q_ * K0 - (W - 1), K0 - D + 1,
                [({'k': q_, 'j': -1}, 0), ({'i': 1, 'k': -p_}, 0)], [], expect)


def lower_face(name, numerator, expect):
    """j >= ceil(Q*i / P) with negative coordinates: the mirror, near INT64_MIN."""
    I0 = -(numerator // Q) - W
    K0 = -(-(I0 + W - 1) // P)
    return base(name, I0, Q * K0, K0, [({'j': 1, 'k': -Q}, 0), ({'k': P, 'i': -1}, 0)], [], expect)


def coordinate(name, I0, expect):
    """Unit rows only, the coordinates themselves near a limit."""
    return base(name, I0, I0 + 1, 0, [({'j': 1, 'k': 1}, -(I0 + 2))], [], expect)


def equality(name, numerator, expect):
    """3*k == i pins the tile, and the elimination substitutes it: the face is
    floor(Q*i / 3), from a substitution and not from a pair of rows."""
    I0 = numerator // Q + 1
    I0 += -I0 % 3
    K0 = I0 // 3
    return base(name, I0, Q * K0 - (W - 1), K0, [({'k': Q, 'j': -1}, 0)], [({'k': 3, 'i': -1}, 0)], expect)


def two_steps(name, scale, expect):
    """C[i+j], kappa 2: two eliminations, the second through the first one's rows.
    The diagonal cut keeps a unit coefficient on the first."""
    I0, J0 = scale, scale + 3
    K0 = 5 * (I0 + J0) // (2 * P) - 3
    return base(name, I0, J0, K0, [({'i': 3, 'j': 3, 'k': -P}, 0), ({'k': Q, 'i': -2, 'j': -2}, 2 * W)], [], expect,
                cc=[({'i': 1, 'j': 1}, -(I0 + J0))], cut=({'i': -1, 'j': -2}, I0 + 2 * J0 + 2 * W))


def chain(name, numerator, expect):
    """Two eliminations that compound: l goes first, by a unit step, and k then
    combines the row it left behind, so j gets floor((Q*i + P*l) / P)."""
    I0 = numerator // Q + 1
    K0 = I0 // P
    L0 = 7
    J0 = Q * K0 + L0 - (W - 1)
    loops = [('i', I0, I0 + W), ('j', J0, J0 + W), ('k', K0 - D + 1, K0 + 1), ('l', L0 - D + 1, L0 + 1)]
    return Case(name, loops,
                [({'i': 1, 'j': -1}, J0 - I0), ({'i': 1, 'k': -P}, 0), ({'k': Q, 'l': 1, 'j': -1}, 0)], [],
                [({'i': 1}, -I0), ({'j': 1}, -J0)], [({'k': 1}, D - 1 - K0), ({'l': 1}, D - 1 - L0), ({'i': 1}, -I0)],
                [({'k': 1}, D - 1 - K0), ({'l': 1}, D - 1 - L0), ({'j': 1}, -J0)], expect, hide_outer=True)


def mirrored(name):
    """Negative coefficients and negative numerators: ceil and floor must round
    the right way, and the C index runs backwards."""
    loops = [('i', -12, -2), ('j', -12, -2), ('k', -9, 0)]
    return Case(name, loops, [({'i': -1, 'j': 1}, 0), ({'k': -3, 'i': 2}, -4), ({'k': 5, 'j': -3}, 40)], [],
                [({'i': -1}, -3), ({'j': -1}, -3)], [({'k': 1}, 9), ({'i': 1}, 12)], [({'k': 1}, 9), ({'j': 1}, 12)],
                'lowered')


def only_block_test(name):
    """A large divisor on the tile: the block test adds d * (K - 1), so this one
    nest fits at Kdev 1 and not at Kdev 2. With limit L = 64 * d and the box capping
    k at 31, the address costs 2 * I0 + 9 = L - 1, the block test I0 + 9 + 32 * d =
    L + 4."""
    d = 1 << 55
    I0 = 32 * d - 5
    K0 = I0 // d
    return base(name, I0, K0 - 9, K0 - D + 1, [({'i': 1, 'k': -d}, 0), ({'k': 1, 'j': -1}, 0)], [],
                {1: 'lowered', 2: TEST})


CASES = [
    # The reported nest: 55 MACs became 0 past 2^63. Refused from 2^61 on.
    upper_face('upper_2_50', 2**50, 'lowered'),
    upper_face('upper_2_60', 2**60, 'lowered'),
    upper_face('upper_past_limit', 2**61 + 2**41, BOUND),
    upper_face('upper_under_2_63', 2**63 - 2**30, BOUND),
    upper_face('upper_past_2_63', 2**63 + 2**30, BOUND),
    upper_face('upper_past_2_64', 2**64 + 2**33, BOUND),
    # The mirror: a wrapped lower face past INT64_MIN.
    lower_face('lower_2_50', 2**50, 'lowered'),
    lower_face('lower_past_limit', 2**61 + 2**41, BOUND),
    lower_face('lower_past_int64_min', 2**63 + 2**30, BOUND),
    # Coordinates at the limits of int64.
    coordinate('coordinate_2_59', 2**59, 'lowered'),
    coordinate('coordinate_past_limit', 2**60 + 2**30, RANGE),
    coordinate('coordinate_near_int64_max', INT64_MAX - W - 8, RANGE),
    coordinate('coordinate_near_int64_min', INT64_MIN + 8, RANGE),
    # Large coefficients.
    upper_face('large_coefficients_ok', 2**58, 'lowered', 2147483647, 2147483629),
    upper_face('large_coefficients', 2**64, BOUND, 2147483647, 2147483629),
    # Equalities: the face comes from a substitution, not from a pair of rows.
    equality('equality_small', 2**50, 'lowered'),
    equality('equality_past_2_63', 2**63 + 2**30, BOUND),
    # Two eliminations. With one output the last rows hold a single variable, so
    # the coefficients they multiply never reach a product at run time.
    two_steps('two_steps_small', 2**30, 'lowered'),
    two_steps('two_steps_large_coordinates', 2**43, 'lowered'),
    two_steps('two_steps_past_limit', 2**60, RANGE),
    # Two eliminations that compound, in a face over two variables.
    chain('chain_small', 2**50, 'lowered'),
    chain('chain_past_limit', 2**61 + 2**41, BOUND),
    chain('chain_past_2_64', 2**64 + 2**33, BOUND),
    # Negative coefficients, ceildiv and floordiv of negative numerators.
    mirrored('mirrored'),
    # The block term.
    only_block_test('only_block_test'),
]

for case in CASES:
    for kdev in (1, 2):
        print(f'{case.name} Kdev {kdev}: {check(case, kdev)}: PASS', flush=True)
