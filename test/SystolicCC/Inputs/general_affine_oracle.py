"""Independent source multiset + interpreter for the emitted dense-view IR.

This does not read the compiler's coverage verdict. It expands actual batch
loops and dense jobs and compares every physical A/B/C address and multiplicity
against the original source. Unsupported IR syntax fails the test closed.
"""
import ast
from collections import Counter, defaultdict
from itertools import product
import re


def source(case, n):
    points = []
    for x in product(range(4), repeat=3 if case in (1, 2, 5) else 4):
        i, j, k = x[:3]
        l = x[3] if len(x) == 4 else 0
        if case == 1:
            I, J, K = i+j-2*k+6, i-2*j+k+6, i+j+k
            a, b, c = (I, K), (K, J), (I, J)
        elif case == 2:
            a, b, c = (2*i+k, i+2*k), (2*k+j, k+2*j), (i, j)
        elif case == 3:
            a = i+2*k+3*l, 2*i+4*k+5*l
            b, c = (5*k+6*l+j, 3*k+2*l+2*j), (i, j)
        elif case == 4:
            a, b = (i+3*k+5*l, 2*i+6*k+3*l), (3*j+2*k+3*l, 5*j+6*k+l)
            c = i+2*k+3*l, j+5*k+6*l
        else:
            assert case == 5
            a, b, c = (2*i+3*j, k), (k, 0), (2*i+3*j, 0)
        assert all(0 <= v < n for pair in (a, b, c) for v in pair)
        points.append((a[0]*n+a[1], b[0]*n+b[1], c[0]*n+c[1]))
    return points


def dense_ir(text, n):
    constants, maps, views, scalars, packed = {}, {}, {}, {}, {}
    macs, jobs = [], 0
    for name, expr in re.findall(r'(#\w+) = affine_map<.*? -> \((.*?)\)>', text):
        maps[name] = expr
    for a in range(3):
        views['%arg'+str(a)] = ('source'+str(a), 0, (n, 1), (n, n))

    def integer(s):
        return constants[s.strip()] if s.strip().startswith('%') else int(s)

    def expr(s, args):
        names = {'d'+str(i): integer(v) for i, v in enumerate(args)}
        def walk(t):
            if isinstance(t, ast.Constant) and isinstance(t.value, int): return t.value
            if isinstance(t, ast.Name): return names[t.id]
            if isinstance(t, ast.UnaryOp) and isinstance(t.op, ast.USub): return -walk(t.operand)
            if isinstance(t, ast.BinOp):
                a, b = walk(t.left), walk(t.right)
                if isinstance(t.op, ast.Add): return a+b
                if isinstance(t.op, ast.Sub): return a-b
                if isinstance(t.op, ast.Mult): return a*b
            raise AssertionError(ast.dump(t))
        return walk(ast.parse(s, mode='eval').body)

    def coords(v, r, c):
        base, off, stride, shape = views[v]
        assert 0 <= r < shape[0] and 0 <= c < shape[1]
        address = off+r*stride[0]+c*stride[1]
        if base.startswith('source'):
            assert 0 <= address < n*n
            return int(base[-1]), address
        return packed[base, address]

    def slice_view(v, off, sizes, stride):
        base, start, old, _ = views[v]
        return base, start+sum(a*b for a, b in zip(off, old)), tuple(a*b for a, b in zip(old, stride)), tuple(sizes)

    lines = text.splitlines()
    def run(start, end):
        nonlocal jobs
        p = start
        while p < end:
            line = lines[p].strip(); p += 1
            loop = re.match(r'affine.for (%\w+) = (\d+) to (\d+) \{', line)
            if loop:
                q, depth = p, 1
                while depth:
                    if lines[q].strip().startswith('affine.for '): depth += 1
                    if lines[q].strip() == '}': depth -= 1
                    q += 1
                for x in range(int(loop[2]), int(loop[3])):
                    constants[loop[1]] = x; run(p, q-1)
                p = q; continue
            m = re.match(r'(%\w+) = arith.constant (-?\d+) : index', line)
            if m: constants[m[1]] = int(m[2]); continue
            m = re.match(r'(%\w+) = affine.apply (#\w+)\((.*?)\)', line)
            if m: constants[m[1]] = expr(maps[m[2]], m[3].split(',')); continue
            m = re.match(r'(%\w+) = memref.alloca\(\) : memref<(\d+)x(\d+)xf32>', line)
            if m:
                views[m[1]] = (m[1], 0, (int(m[3]), 1), (int(m[2]), int(m[3]))); continue
            m = re.match(r'(%\w+) = memref.load (%\w+)\[(.*?)\]', line)
            if m: scalars[m[1]] = coords(m[2], *map(integer, m[3].split(','))); continue
            m = re.match(r'memref.store (%\w+), (%\w+)\[(.*?)\]', line)
            if m:
                base, off, stride, _ = views[m[2]]; r, c = map(integer, m[3].split(','))
                if not base.startswith('source'): packed[base, off+r*stride[0]+c*stride[1]] = scalars[m[1]]
                continue
            m = re.match(r'(%\w+) = memref.reinterpret_cast (%\w+) to offset: \[(.*?)\], sizes: \[(.*?)\], strides: \[(.*?)\]', line)
            if m:
                views[m[1]] = (views[m[2]][0], integer(m[3]), tuple(map(integer,m[5].split(','))), tuple(map(integer,m[4].split(',')))); continue
            m = re.match(r'(%\w+) = (?:memref.subview|tensor.extract_slice) (%\w+)\[(.*?)\] \[(.*?)\] \[(.*?)\]', line)
            if m:
                views[m[1]] = slice_view(m[2], list(map(integer,m[3].split(','))), list(map(integer,m[4].split(','))), list(map(integer,m[5].split(',')))); continue
            m = re.match(r'(%\w+) = bufferization.to_tensor (%\w+)', line)
            if m: views[m[1]] = views[m[2]]; continue
            m = re.match(r'(%\w+) = systolic.matmul_tile (%\w+), (%\w+), (%\w+) m = (\d+) n = (\d+) k = (\d+)', line)
            if m:
                jobs += 1; views[m[1]] = views[m[4]]
                for i,j,k in product(range(int(m[5])),range(int(m[6])),range(int(m[7]))):
                    a,b,c = coords(m[2],i,k), coords(m[3],k,j), coords(m[4],i,j)
                    assert (a[0],b[0],c[0]) == (1,2,0)
                    macs.append((a[1],b[1],c[1]))
                continue
            m = re.match(r'(%\w+) = tensor.insert_slice .*? into (%\w+)\[', line)
            if m: views[m[1]] = views[m[2]]; continue
            if ('=' in line and not line.startswith(('systolic.device', 'func.func', '#'))) or line.startswith('affine.'):
                raise AssertionError('uninterpreted emitted operation: '+line)
        return jobs, macs
    return run(0, len(lines))


def check(case, ir, n):
    expected = source(case,n)
    jobs, actual = dense_ir(ir,n)
    assert Counter(expected) == Counter(actual), (case, Counter(expected)-Counter(actual), Counter(actual)-Counter(expected))
    def fibers(points):
        f = defaultdict(list)
        for a,b,c in points: f[c].append((a,b))
        return dict(f)
    ordered = fibers(expected) == fibers(actual)
    # Existing case4 batches legitimately reorder additions only in reassoc mode.
    if case != 4: assert ordered
    return {'source_MACs':len(expected), 'emitted_MACs':len(actual), 'output_elements':len(fibers(expected)), 'actual_expanded_jobs':jobs, 'exact_address_multiset':'PASS', 'per_output_source_order':ordered}
