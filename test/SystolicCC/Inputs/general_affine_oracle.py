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


# ---------------------------------------------------------------------------
# Phase M4b.2a: independent interpreter for the experimental symbolic
# kernel-coset emission. Separate from dense_ir/source above (which stay exactly
# as validated for the enumeration path): this one interprets AffineMap loop
# bounds (multi-result min/max with floordiv/ceildiv), rank-generic memref
# load/store, and the 1x1 gather slabs, recovering every physical A/B/C address.
# ---------------------------------------------------------------------------


def _kc_tokenize(s):
    toks, i = [], 0
    while i < len(s):
        c = s[i]
        if c.isspace():
            i += 1
        elif c in '()+-*,':
            toks.append(c); i += 1
        elif c.isdigit():
            j = i
            while j < len(s) and s[j].isdigit():
                j += 1
            toks.append(('num', int(s[i:j]))); i = j
        elif c.isalpha() or c == '_':
            j = i
            while j < len(s) and (s[j].isalnum() or s[j] == '_'):
                j += 1
            w = s[i:j]
            toks.append(w if w in ('floordiv', 'ceildiv', 'mod') else ('name', w))
            i = j
        else:
            raise AssertionError('affine token near: ' + s[i:])
    return toks


def aff_eval(s, env):
    """Evaluate one MLIR affine expression string under env {dimname: int}.

    + - have lowest precedence; * floordiv ceildiv mod are multiplicative,
    left-associative; unary minus binds tightest. floordiv rounds toward -inf
    (Python //), mod matches MLIR for positive divisors, ceildiv rounds up.
    """
    toks = _kc_tokenize(s)
    pos = 0

    def peek():
        return toks[pos] if pos < len(toks) else None

    def take():
        nonlocal pos
        t = toks[pos]; pos += 1; return t

    def expr():
        v = term()
        while peek() in ('+', '-'):
            op = take(); r = term()
            v = v + r if op == '+' else v - r
        return v

    def term():
        v = factor()
        while peek() in ('*', 'floordiv', 'ceildiv', 'mod'):
            op = take(); r = factor()
            if op == '*':
                v = v * r
            elif op == 'floordiv':
                v = v // r
            elif op == 'mod':
                v = v % r
            else:
                assert r > 0
                v = -((-v) // r)
        return v

    def factor():
        t = peek()
        if t == '-':
            take(); return -factor()
        if t == '(':
            take(); v = expr(); assert take() == ')'; return v
        t = take()
        if isinstance(t, tuple) and t[0] == 'num':
            return t[1]
        if isinstance(t, tuple) and t[0] == 'name':
            return env[t[1]]
        raise AssertionError('affine parse near: ' + str(t))

    v = expr()
    assert pos == len(toks), (s, toks[pos:])
    return v


def _kc_split_results(s):
    parts, depth, cur = [], 0, ''
    for c in s:
        if c == '(':
            depth += 1; cur += c
        elif c == ')':
            depth -= 1; cur += c
        elif c == ',' and depth == 0:
            parts.append(cur); cur = ''
        else:
            cur += c
    if cur.strip():
        parts.append(cur)
    return [p.strip() for p in parts]


def kernel_coset_macs(text):
    """Interpret the kernel-coset emission; return (static matmul_tile ops,
    [(A_addr, B_addr, C_addr)] expanded over every loop iteration)."""
    maps = {}
    for m in re.finditer(r'(#\w+) = affine_map<\(([^)]*)\) -> \((.*)\)>', text):
        maps[m[1]] = _kc_split_results(m[3])

    # Source memref operands, in signature order: arg0 = C (output), 1 = A, 2 = B.
    src = {}
    for i, (nm, dims) in enumerate(
            re.findall(r'(%arg\d+): memref<([0-9x]+)xf32>', text)):
        shape = tuple(int(d) for d in dims.split('x'))
        strides = [1] * len(shape)
        for d in range(len(shape) - 2, -1, -1):
            strides[d] = strides[d + 1] * shape[d + 1]
        src[nm] = (i, shape, tuple(strides))

    env, scalars, packed, views, macs, jobs = {}, {}, {}, {}, [], [0]

    def integer(tok):
        tok = tok.strip()
        return env[tok] if tok.startswith('%') else int(tok)

    def apply_map(app):
        mm = re.match(r'(#\w+)\((.*)\)', app.strip())
        args = [a for a in mm[2].split(',') if a.strip()]
        denv = {'d' + str(i): integer(a) for i, a in enumerate(args)}
        return [aff_eval(r, denv) for r in maps[mm[1]]]

    def bound(s):
        s = s.strip()
        if s.startswith('max '):
            return max(apply_map(s[4:]))
        if s.startswith('min '):
            return min(apply_map(s[4:]))
        if s.startswith('#'):
            return apply_map(s)[0]
        return int(s)

    def addr(view, idxs):
        if view[0] == 'slab':
            return packed[view[1]]
        role, shape, strides = src[view[1]]
        assert len(idxs) == len(shape), (view, idxs)
        a = 0
        for v, st, sz in zip(idxs, strides, shape):
            assert 0 <= v < sz, (view, idxs, shape)
            a += v * st
        return (role, a)

    lines = text.splitlines()

    def run(start, end):
        p = start
        while p < end:
            line = lines[p].strip(); p += 1
            m = re.match(r'affine\.for (%\w+) = (.+?) to (.+?) \{', line)
            if m:
                q, depth = p, 1
                while depth:
                    t = lines[q].strip()
                    if t.endswith('{'):
                        depth += 1
                    if t == '}':
                        depth -= 1
                    q += 1
                for x in range(bound(m[2]), bound(m[3])):
                    env[m[1]] = x
                    run(p, q - 1)
                p = q
                continue
            m = re.match(r'(%\w+) = arith.constant (-?\d+) : index', line)
            if m:
                env[m[1]] = int(m[2]); continue
            m = re.match(r'(%\w+) = affine.apply (#\w+\(.*?\))', line)
            if m:
                env[m[1]] = apply_map(m[2])[0]; continue
            m = re.match(r'(%\w+) = memref.alloca\(\) : memref<', line)
            if m:
                views[m[1]] = ('slab', m[1]); continue
            m = re.match(r'(%\w+) = memref.load (%\w+)\[(.*?)\] :', line)
            if m:
                idxs = [integer(x) for x in m[3].split(',')] if m[3].strip() else []
                scalars[m[1]] = addr(views.get(m[2], ('src', m[2])), idxs)
                continue
            m = re.match(r'memref.store (%\w+), (%\w+)\[(.*?)\] :', line)
            if m:
                tgt = views.get(m[2], ('src', m[2]))
                if tgt[0] == 'slab':
                    packed[tgt[1]] = scalars[m[1]]
                continue
            m = re.match(r'(%\w+) = bufferization.to_tensor (%\w+)', line)
            if m:
                views[m[1]] = views[m[2]]; continue
            m = re.match(r'(%\w+) = tensor.extract_slice (%\w+)\[', line)
            if m:
                views[m[1]] = views[m[2]]; continue
            m = re.match(r'(%\w+) = tensor.insert_slice .*? into (%\w+)\[', line)
            if m:
                views[m[1]] = views[m[2]]; continue
            m = re.match(
                r'(%\w+) = systolic.matmul_tile (%\w+), (%\w+), (%\w+) '
                r'm = (\d+) n = (\d+) k = (\d+)', line)
            if m:
                jobs[0] += 1
                views[m[1]] = views[m[4]]
                A, B, C = views[m[2]], views[m[3]], views[m[4]]
                for i, j, k in product(range(int(m[5])), range(int(m[6])),
                                       range(int(m[7]))):
                    a, b, c = addr(A, [i, k]), addr(B, [k, j]), addr(C, [i, j])
                    assert (a[0], b[0], c[0]) == (1, 2, 0), (a, b, c)
                    macs.append((a[1], b[1], c[1]))
                continue
            if line.startswith('bufferization.materialize_in_destination'):
                continue
            if line in ('}', 'return', '') or line.startswith(
                    ('module', 'func.func', 'systolic.device', '#', '//')):
                continue
            if '=' in line or line.startswith('affine.'):
                raise AssertionError('uninterpreted emitted op: ' + line)
        return jobs[0], macs

    return run(0, len(lines))


def skew_source():
    """Brute-force MACs of C[i+j-2k+6] += A[i,k]*B[k,j], 0<=i,j,k<4.

    The physical-C index is a single rank-1 (singular-output) linear form, so
    the box domain folds into 13 ragged fibers over 64 source MACs. A and B are
    16x16 row-major; C is 1-D (16). This mirrors the kernel-coset target and
    shares no coefficient with the compiler."""
    pts = []
    for i, j, k in product(range(4), repeat=3):
        pts.append((i * 16 + k, k * 16 + j, i + j - 2 * k + 6))
    return pts
