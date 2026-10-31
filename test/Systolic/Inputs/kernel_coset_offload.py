"""Phase M4b.2a: independent check of the experimental kernel-coset offload.

Verifies, with no reliance on the compiler's own report:
  - positive: the flag emits a nest whose interpreted MAC multiset, per-output
    grouping (source order), physical A/B/C addresses, MAC count (64) and fiber
    count (13) equal an independent brute-force enumeration -- with no source
    enumeration and no tile/fiber materialization in the IR (symbolic loops);
  - default (flag off): the source nest is left exactly as written;
  - strict FP without reassociation: declined, source left as written;
  - reassociation authorized: emits the same exact nest;
  - possible operand aliasing (no llvm.noalias): declined, left as written.
"""
from collections import Counter, defaultdict
from pathlib import Path
import subprocess
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / 'SystolicCC' / 'Inputs'))
from general_affine_oracle import kernel_coset_macs, skew_source

opt, fixture, temp = sys.argv[1:4]
root = Path(temp); root.mkdir(parents=True, exist_ok=True)
base = '\n'.join(x for x in Path(fixture).read_text().splitlines()
                 if not x.startswith('//'))


def run(name, text, extra, fleet=True):
    src = root / (name + '.mlir'); dst = root / (name + '.out.mlir')
    src.write_text(text)
    flags = (['--systolic-declare-fpga-fleet'] if fleet else []) + \
            ['--affine-matmul-to-systolic=general-affine=true ' + extra]
    p = subprocess.run([opt] + flags + [str(src), '-o', str(dst)],
                       capture_output=True, text=True)
    (root / (name + '.log')).write_text(p.stderr)
    assert p.returncode == 0, (name, p.stderr)
    return dst.read_text()


def grouping(points):
    # Per physical-C element, the MULTISET of (A,B) products accumulated into it.
    # The device peel/tile traversal is a reassociation of the source reduction
    # (M3 certified this skew target NotSourceLexEquivalent), which the offload
    # is only reached for under FP-reassociation permission -- so correctness is
    # the per-fiber multiset, not the within-fiber order.
    f = defaultdict(Counter)
    for a, b, c in points:
        f[c][(a, b)] += 1
    return dict(f)


# ---- positive: the flag emits the exact symbolic nest ---------------------
ir = run('kc-on', base, 'kernel-coset-offload=true report=true')
assert 'systolic.matmul_tile' in ir, 'kernel-coset did not emit'
# Symbolic: exactly one static matmul_tile, and no enumerated per-point copy.
assert ir.count('systolic.matmul_tile') == 1, 'emission is not a single symbolic tile'
expected = skew_source()
jobs, macs = kernel_coset_macs(ir)
assert jobs == len(expected), ('expanded tile count', jobs, len(expected))
assert Counter(macs) == Counter(expected), (
    'address multiset', Counter(expected) - Counter(macs), Counter(macs) - Counter(expected))
assert grouping(macs) == grouping(expected), 'per-output accumulated multiset'
assert len(macs) == 64 and len(grouping(macs)) == 13, (len(macs), len(grouping(macs)))
# No source-point enumeration: the emitted nest keeps symbolic affine loops.
assert 'affine.for' in ir and ir.count('affine.for') == 3
print('kernel-coset positive: 64 exact MACs, 13 fibers, 1 symbolic tile: PASS')

# ---- default (flag off): unchanged ----------------------------------------
off = run('kc-off', base, 'report=true')
assert 'systolic.matmul_tile' not in off, 'default must not offload'
assert 'affine.for %arg3 = 0 to 4' in off, 'source nest must be preserved'
print('kernel-coset default off: source unchanged: PASS')

# ---- unsupported cases: declined, source left exactly as written ----------
negatives = {
    'strict': (base.replace(' fastmath<reassoc>', ''),
               'kernel-coset-offload=true report=true'),
    'alias': (base.replace(' {llvm.noalias}', ''),
              'kernel-coset-offload=true report=true'),
}
for name, (text, extra) in negatives.items():
    ir = run(name, text, extra)
    assert 'systolic.matmul_tile' not in ir, (name, 'unsafe offload')
    assert 'affine.for %arg3 = 0 to 4' in ir, (name, 'source nest mutated on refusal')
    log = (root / (name + '.log')).read_text()
    assert 'kernel-coset offload not applied' in log, (name, 'missing refusal remark')
    print('kernel-coset refusal', name, 'safe decline: PASS')

# ---- reassociation authorized: emits the exact nest -----------------------
ir = run('reassoc', base.replace(' fastmath<reassoc>', ''),
         'kernel-coset-offload=true allow-fp-reassociation=true')
jobs, macs = kernel_coset_macs(ir)
assert Counter(macs) == Counter(skew_source()) and len(grouping(macs)) == 13
print('kernel-coset reassociation authorized: 64 exact MACs: PASS')

# ---- no device fleet: the device-availability gate declines ----------------
nofleet = run('nofleet', base, 'kernel-coset-offload=true report=true', fleet=False)
assert 'systolic.matmul_tile' not in nofleet, 'offload without a fleet'
assert 'affine.for %arg3 = 0 to 4' in nofleet, 'source nest mutated without a fleet'
assert 'requires a declared fleet' in (root / 'nofleet.log').read_text()
print('kernel-coset no-fleet device gate: safe decline: PASS')
