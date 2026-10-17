"""
casespec.py -- the data model of a GEMM-recognition test case, its independent
Python evaluation (the oracle), its mathematical classification, a predictor
of what the compiler at 6e25528 is expected to do with it, and the emitter of
the self-contained C test program.

A case is a loop nest around one multiply-accumulate statement:

    for <loops, outer to inner>:
        C[sC0][sC1]... (op) A[sA0][sA1]... * B[sB0][sB1]...

where every subscript is an affine expression (C syntax) of the enclosing loop
counters.  Each loop counter has a role:

    I, J, K   a loop that enumerates (part of) the output row, output column or
              reduction coordinate of the GEMM the author has in mind
    P         a batch / partition coordinate (its own subscript of A, B and C)
    R         a repetition loop: no subscript uses it
    X         anything else (the author declares the nest is not a GEMM, or
              its structure does not follow the groups above)

The roles are the author's *claim*; analyse() checks that claim against the
subscripts and the loop domain and derives the mathematical class.  predict()
then applies the rules of the compiler's front end, recognizer, dense-GEMM
view, lowering and driver (read from the sources at commit 6e25528) to say
which stage is expected to accept or reject the nest.  Both are independent
re-implementations used only to state expectations; the compiler's actual
behaviour is measured by scripts/run_showcase.py and never assumed.

Nothing here depends on the compiler or on numpy.
"""

import ast
import itertools
import math
import os
import re
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple

# --------------------------------------------------------------------------
# affine index expressions
# --------------------------------------------------------------------------


class LinearForm:
    """sum(coef[v] * v) + const over loop counters v."""

    def __init__(self, coef=None, const=0):
        self.coef = {k: v for k, v in (coef or {}).items() if v != 0}
        self.const = const

    def __add__(self, o):
        c = dict(self.coef)
        for k, v in o.coef.items():
            c[k] = c.get(k, 0) + v
        return LinearForm(c, self.const + o.const)

    def __sub__(self, o):
        return self + o.scale(-1)

    def scale(self, s):
        return LinearForm({k: v * s for k, v in self.coef.items()}, self.const * s)

    def is_const(self):
        return not self.coef

    def vars(self):
        return set(self.coef)

    def eval(self, env):
        return self.const + sum(v * env[k] for k, v in self.coef.items())

    def key(self):
        return (tuple(sorted(self.coef.items())), self.const)

    def __eq__(self, o):
        return isinstance(o, LinearForm) and self.key() == o.key()

    def __hash__(self):
        return hash(self.key())

    def __repr__(self):
        return "LinearForm(%s)" % self.c_text()

    def c_text(self):
        """A canonical C spelling (coef*var terms then the constant)."""
        parts = []
        for k, v in sorted(self.coef.items()):
            if v == 1:
                parts.append(k)
            elif v == -1:
                parts.append("-" + k)
            else:
                parts.append("%d*%s" % (v, k))
        if self.const or not parts:
            parts.append(str(self.const))
        s = " + ".join(parts)
        return s.replace("+ -", "- ")


class NotAffine(Exception):
    pass


def parse_affine(text: str) -> LinearForm:
    """Parse a C-syntax affine expression ('2*i + 3', '(7)-i', 'i*2+k') into
    a LinearForm.  Raises NotAffine."""
    try:
        tree = ast.parse(text.strip(), mode="eval")
    except SyntaxError as e:
        raise NotAffine("cannot parse %r: %s" % (text, e))

    def go(n):
        if isinstance(n, ast.Expression):
            return go(n.body)
        if isinstance(n, ast.Constant) and isinstance(n.value, int) and not isinstance(n.value, bool):
            return LinearForm({}, n.value)
        if isinstance(n, ast.Name):
            return LinearForm({n.id: 1}, 0)
        if isinstance(n, ast.UnaryOp):
            v = go(n.operand)
            if isinstance(n.op, ast.USub):
                return v.scale(-1)
            if isinstance(n.op, ast.UAdd):
                return v
            raise NotAffine("unary operator in %r" % text)
        if isinstance(n, ast.BinOp):
            a, b = go(n.left), go(n.right)
            if isinstance(n.op, ast.Add):
                return a + b
            if isinstance(n.op, ast.Sub):
                return a - b
            if isinstance(n.op, ast.Mult):
                if a.is_const():
                    return b.scale(a.const)
                if b.is_const():
                    return a.scale(b.const)
                raise NotAffine("product of two non-constant terms in %r" % text)
            raise NotAffine("operator in %r is not affine" % text)
        raise NotAffine("unsupported syntax in %r" % text)

    return go(tree)


# --------------------------------------------------------------------------
# the case model
# --------------------------------------------------------------------------

ROLE_NAMES = {"I": "row I", "J": "column J", "K": "reduction K", "P": "batch P",
              "R": "repetition", "X": "other"}

STMT_TEXT = {"+=": "C[..] += A[..] * B[..]",
             "=C+AB": "C[..] = C[..] + A[..] * B[..]",
             "=AB+C": "C[..] = A[..] * B[..] + C[..]",
             "+=BA": "C[..] += B[..] * A[..]",
             "-=": "C[..] -= A[..] * B[..]",
             "=AB": "C[..] = A[..] * B[..]",
             "+=alpha": "C[..] += alpha * A[..] * B[..]",
             "+=sum": "C[..] += A[..] + B[..]",
             "+=triple": "C[..] += A[..] * B[..] * D[..]"}


@dataclass
class Loop:
    var: str
    lb: int
    ub: int            # the bound as written: exclusive for '<' / '>', inclusive for '<=' / '>='
    step: int = 1      # negative for a decreasing loop (cmp '>' or '>=')
    cmp: str = "<"     # '<', '<=', '>' or '>='
    role: str = "X"    # I, J, K, P, R, X
    ctype: str = "int"
    inc: str = "++"    # '++' (prefix), 'post++', '+=', '=+'; for step < 0: '--', 'post--', '-=' (i -= |step|)
    cond: str = ""     # the loop condition as written in C when it is not 'var cmp ub' -- an
                       # affine expression of the counter compared with a constant, e.g.
                       # '2 * i - 3 > 0'; cmp / ub are then its declared equivalent (i > 1),
                       # and values() checks the two against each other

    def __post_init__(self):
        if self.cmp not in ("<", "<=", ">", ">="):
            raise ValueError("%s: comparison %r" % (self.var, self.cmp))
        if self.step == 0:
            raise ValueError("%s: step 0" % self.var)
        if (self.step < 0) != (self.cmp in (">", ">=")):
            raise ValueError("%s: a loop with step %d and condition %r never terminates"
                             % (self.var, self.step, self.cmp))
        if self.inc not in ("++", "post++", "+=", "=+", "--", "post--", "-="):
            raise ValueError("%s: increment %r" % (self.var, self.inc))
        if self.cond and not re.search(r"\b%s\b" % re.escape(self.var), self.cond):
            raise ValueError("%s: condition %r does not use the counter" % (self.var, self.cond))

    def decreasing(self):
        return self.step < 0

    def exclusive_ub(self):
        """The bound in range() terms: the first value NOT visited, in the
        direction of the step."""
        if self.cmp == "<=":
            return self.ub + 1
        if self.cmp == ">=":
            return self.ub - 1
        return self.ub

    def values(self):
        declared = list(range(self.lb, self.exclusive_ub(), self.step))
        if self.cond:
            # run the condition as written (integer + - * and comparisons
            # mean the same in Python as in C over these values): the
            # declared equivalent must visit exactly the same values
            written, v = [], self.lb
            while eval(self.cond, {"__builtins__": {}}, {self.var: v}):
                written.append(v)
                v += self.step
                if len(written) > 10 ** 6:
                    raise ValueError("%s: condition %r runs for more than 10^6 iterations"
                                     % (self.var, self.cond))
            if written != declared:
                raise ValueError("%s: the condition %r visits %d value(s), the declared %s %s %d "
                                 "visits %d" % (self.var, self.cond, len(written), self.var,
                                                self.cmp, self.ub, len(declared)))
            return written
        return declared

    def trip(self):
        return len(self.values())

    def c_header(self):
        s, v = abs(self.step), self.var
        if self.inc in ("++", "--"):
            inc = ("++%s" if self.step > 0 else "--%s") % v if s == 1 else self._compound(s)
        elif self.inc in ("post++", "post--"):
            inc = ("%s++" if self.step > 0 else "%s--") % v if s == 1 else self._compound(s)
        elif self.inc in ("+=", "-="):
            inc = self._compound(s)
        elif self.inc == "=+":
            inc = "%s = %s %s %d" % (v, v, "+" if self.step > 0 else "-", s)
        else:
            raise ValueError(self.inc)
        cond = self.cond or "%s %s %d" % (self.var, self.cmp, self.ub)
        return "for (%s %s = %d; %s; %s)" % (self.ctype, self.var, self.lb, cond, inc)

    def _compound(self, s):
        return "%s %s %d" % (self.var, "+=" if self.step > 0 else "-=", s)

    def reference_header(self, ctype="long"):
        """The same iteration set as a reference loop: for (v = lb; v < / >
        exclusive bound; v += step)."""
        return "for (%s %s = %d; %s %s %d; %s += %d)" % (
            ctype, self.var, self.lb, self.var, "<" if self.step > 0 else ">",
            self.exclusive_ub(), self.var, self.step)


@dataclass
class Array:
    name: str
    dims: Tuple[int, ...]
    subs: Tuple[str, ...]          # C-syntax subscripts, outermost first
    restrict: bool = True
    elem: str = "float"

    def forms(self):
        return [parse_affine(s) for s in self.subs]


@dataclass
class Case:
    id: str
    name: str
    set: str                        # 'positive' | 'negative' | 'extended'
    category: str
    loops: List[Loop]
    arrays: Dict[str, Array]        # keyed by operand role: 'a', 'b', 'c' (+ 'd' for a third factor)
    stmt: str = "+="                # see STMT_TEXT
    elem: str = "float"
    kernel_style: str = "plain"     # 'plain' | 'init_inside' | 'scalar_acc' | 'ptr_param' | 'braces'
    alpha: Optional[float] = None   # for '+=alpha'
    math_class: str = ""            # filled by analyse() unless given
    math_note: str = ""             # author's description of the mathematics
    notes: str = ""
    seed: int = 1
    issue: str = ""                 # e.g. 'ISSUE-001' when the case reproduces a recorded compiler issue
    redecided: str = ""             # a specification change that moved the expectation: when, where
                                    # (docs / issue) and what the expectation was before
    # filled in
    analysis: dict = field(default_factory=dict)
    expect: dict = field(default_factory=dict)      # predict(): the current implementation's behaviour
    spec: dict = field(default_factory=dict)        # spec_expect(): what the specification requires
    classification: str = ""                        # classify_case()

    @property
    def kernel(self):
        return "%s_kernel" % self.id

    @property
    def a(self):
        return self.arrays["a"]

    @property
    def b(self):
        return self.arrays["b"]

    @property
    def c(self):
        return self.arrays["c"]

    def array_names(self):
        """Distinct array names in parameter order (a, b, c, d)."""
        out = []
        for role in ("a", "b", "c", "d"):
            if role in self.arrays and self.arrays[role].name not in out:
                out.append(self.arrays[role].name)
        return out

    def role_of_name(self, name):
        for role in ("a", "b", "c", "d"):
            if role in self.arrays and self.arrays[role].name == name:
                return role
        raise KeyError(name)


# --------------------------------------------------------------------------
# data: small integers, identical in C and Python
# --------------------------------------------------------------------------


def elem_value(e: int, seed: int) -> int:
    """The value of flat element e of an array initialised with `seed`:
    integers in [-3, 3].  C: (e * 37 + seed * 101) % 103 % 7 - 3, e, seed >= 0."""
    return (e * 37 + seed * 101) % 103 % 7 - 3


def array_seed(case: Case, name: str) -> int:
    role = case.role_of_name(name)
    return {"a": 1, "b": 2, "c": 3, "d": 4}[role] * 10 + case.seed


def checksum(flat: List[int]) -> int:
    """sum_e C[e] * ((e % 97) + 1); C computes the same in double."""
    return sum(v * ((e % 97) + 1) for e, v in enumerate(flat))


def flat_index(dims, idx):
    off = 0
    for d, i in zip(dims, idx):
        off = off * d + i
    return off


def volume(dims):
    v = 1
    for d in dims:
        v *= d
    return v


# --------------------------------------------------------------------------
# the Python oracle: evaluate the kernel's statement exactly, in integers
# --------------------------------------------------------------------------


def evaluate(case: Case):
    """Return {'c0': C before, 'c': C after, <name>: flat int arrays},
    evaluating the statement over the loop domain exactly as the C kernel
    does (in exact integer arithmetic, which is what makes the float results
    exact).  Raises IndexError on an out-of-bounds subscript."""
    data = {}
    for name in case.array_names():
        arr = case.arrays[case.role_of_name(name)]
        data[name] = [elem_value(e, array_seed(case, name)) for e in range(volume(arr.dims))]
    c0 = list(data[case.c.name])
    forms = {role: arr.forms() for role, arr in case.arrays.items()}

    def addr(role, env):
        arr = case.arrays[role]
        idx = [f.eval(env) for f in forms[role]]
        for d, i in zip(arr.dims, idx):
            if i < 0 or i >= d:
                raise IndexError("%s%s out of bounds for dims %s at %s" % (
                    arr.name, idx, arr.dims, env))
        return flat_index(arr.dims, idx)

    A, B, C = data[case.a.name], data[case.b.name], data[case.c.name]
    D = data[case.arrays["d"].name] if "d" in case.arrays else None
    stmt = case.stmt
    alpha = int(case.alpha) if case.alpha is not None else None
    env = {}
    loops = case.loops
    zero_before_inner = case.kernel_style in ("init_inside", "scalar_acc")

    def rec(depth):
        if depth == len(loops):
            ia, ib, ic = addr("a", env), addr("b", env), addr("c", env)
            if stmt == "+=alpha":
                p = alpha * A[ia] * B[ib]
            elif stmt == "+=sum":
                p = A[ia] + B[ib]
            elif stmt == "+=triple":
                p = A[ia] * B[ib] * D[addr("d", env)]
            elif stmt in ("+=", "=C+AB", "=AB+C", "+=BA", "=AB", "-="):
                p = A[ia] * B[ib]
            else:
                raise ValueError("unknown stmt %r" % stmt)
            if stmt == "=AB":
                C[ic] = p
            elif stmt == "-=":
                C[ic] = C[ic] - p
            else:
                C[ic] = C[ic] + p
            return
        lp = loops[depth]
        for v in lp.values():
            env[lp.var] = v
            if zero_before_inner and depth == len(loops) - 2:
                env[loops[-1].var] = loops[-1].lb   # C's subscripts do not use it
                C[addr("c", env)] = 0
            rec(depth + 1)
        env.pop(lp.var, None)

    rec(0)
    out = dict(data)
    out["c0"] = c0
    out["c"] = C
    return out


# --------------------------------------------------------------------------
# mathematical analysis: is the nest a (dense) GEMM, of what kind?
# --------------------------------------------------------------------------


def analyse(case: Case) -> dict:
    """Classify the nest from its subscripts and loop domain.  Returns a dict
    with 'math_class' and the supporting facts, and sets case.analysis."""
    loops = case.loops
    roles = {lp.var: lp.role for lp in loops}
    used = set()
    forms = {}
    affine_ok = True
    for role, arr in case.arrays.items():
        try:
            forms[role] = arr.forms()
        except NotAffine:
            affine_ok = False
            forms[role] = None
            continue
        for f in forms[role]:
            used |= f.vars()
    for v in used:
        if v not in roles:
            raise ValueError("%s: subscript uses %r which is not a loop counter" % (case.id, v))

    result = {
        "loop_roles": roles,
        "repetition_loops": [lp.var for lp in loops if lp.var not in used],
        "affine": affine_ok,
        "perfect_nest": case.kernel_style not in ("init_inside", "scalar_acc"),
    }
    if not affine_ok:
        result["math_class"] = case.math_class or "not_affine"
        result["is_gemm"] = False
        result["gemm_tags"] = []
        case.analysis = result
        return result

    for lp in loops:
        if lp.var not in used and lp.role not in ("R", "X"):
            raise ValueError("%s: loop %s is declared %s but no subscript uses it"
                             % (case.id, lp.var, lp.role))
        if lp.var in used and lp.role == "R":
            raise ValueError("%s: loop %s is declared R but a subscript uses it"
                             % (case.id, lp.var))
    groups = {}
    for lp in loops:
        groups.setdefault(lp.role, []).append(lp)
    repeat = 1
    for lp in groups.get("R", []):
        repeat *= lp.trip()
    result["repetitions"] = repeat

    def subscript_group(f: LinearForm):
        gs = {roles[v] for v in f.vars()}
        if len(gs) == 1:
            return gs.pop()
        if not gs:
            return "const"
        return "mixed:" + "+".join(sorted(gs))

    operand_groups = {}
    dense = True
    reasons = []
    expected_groups = {"a": {"I", "K"}, "b": {"K", "J"}, "c": {"I", "J"}}
    if case.stmt == "+=sum":
        dense = False
        reasons.append("the statement accumulates a sum A + B, not a product")
    if case.stmt == "+=triple":
        dense = False
        reasons.append("the product has three factors (A * B * D)")
    if case.stmt == "=AB":
        dense = False
        reasons.append("the statement overwrites C instead of accumulating into it")
    for role in ("a", "b", "c"):
        gl = [subscript_group(f) for f in forms[role]]
        operand_groups[role] = gl
        want = set(expected_groups[role])
        have = [g for g in gl if g != "P"]
        if any(g.startswith("mixed") for g in gl):
            dense = False
            reasons.append("%s has a subscript that couples loops of different "
                           "coordinates (%s)" % (case.arrays[role].name, ", ".join(gl)))
        elif sorted(have) != sorted(want):
            dense = False
            reasons.append("%s is indexed by %s; the operand in that position of a GEMM "
                           "is indexed by exactly %s" % (case.arrays[role].name, gl, sorted(want)))
    p_loops = groups.get("P", [])
    if p_loops and dense:
        # C must carry every batch coordinate; A or B may be shared by all
        # batches (no batch subscript at all), never partially
        for role in ("a", "b", "c"):
            cnt = operand_groups[role].count("P")
            if cnt != len(p_loops) and (role == "c" or cnt != 0):
                dense = False
                reasons.append("the batch coordinate(s) are not subscripts of %s"
                               % case.arrays[role].name)
    for g in ("I", "J", "K"):
        if g not in groups and dense:
            dense = False
            reasons.append("no loop enumerates coordinate %s" % g)

    views = {}
    if dense:
        def ordinals(g):
            ls = groups[g]
            return ls, list(itertools.product(*[lp.values() for lp in ls]))

        for role in ("a", "b", "c"):
            arr = case.arrays[role]
            for pos, (f, g) in enumerate(zip(forms[role], operand_groups[role])):
                if g == "P":
                    continue
                ls, tuples = ordinals(g)
                seen = {}
                for t in tuples:
                    env = dict(zip([lp.var for lp in ls], t))
                    v = f.eval(env)
                    if v in seen:
                        dense = False
                        reasons.append("%s's subscript %d (%s) is not injective on "
                                       "coordinate %s: %s and %s both give %d" %
                                       (arr.name, pos, arr.subs[pos], g, seen[v], t, v))
                        break
                    seen[v] = t
                views.setdefault(role, {})[g] = {
                    "position": pos, "subscript": arr.subs[pos],
                    "first": min(seen) if seen else None, "last": max(seen) if seen else None}
    if dense:
        extents = {g: volume([lp.trip() for lp in groups[g]]) for g in ("I", "J", "K")}
        result["extents"] = extents
        result["views"] = views
        tags = []
        if p_loops:
            tags.append("batched")
        if operand_groups["a"][-2:] != ["I", "K"]:
            tags.append("A_transposed")
        if operand_groups["b"][-2:] != ["K", "J"]:
            tags.append("B_transposed")
        if operand_groups["c"][-2:] != ["I", "J"]:
            tags.append("C_transposed")

        def form_of(role, g):
            return forms[role][views[role][g]["position"]]

        if form_of("a", "I") != form_of("c", "I"):
            tags.append("I_views_differ")
        if form_of("b", "J") != form_of("c", "J"):
            tags.append("J_views_differ")
        if form_of("a", "K") != form_of("b", "K"):
            tags.append("K_views_differ")
        if case.stmt == "+=alpha":
            tags.append("alpha_scaled")
        if case.stmt == "-=":
            tags.append("alpha_minus_one")
        if case.kernel_style in ("init_inside", "scalar_acc"):
            tags.append("beta_zero")
        if repeat > 1:
            tags.append("repeated_%d" % repeat)
        if case.a.name == case.b.name:
            tags.append("A_times_A")
        if case.elem != "float":
            tags.append(case.elem)
        result["gemm_tags"] = tags
        result["math_class"] = case.math_class or ("gemm_batched" if p_loops else "gemm")
        result["is_gemm"] = True
    else:
        result["gemm_tags"] = []
        result["not_dense_reasons"] = reasons
        result["is_gemm"] = False
        result["math_class"] = case.math_class or "not_gemm"
    case.analysis = result
    return result


# --------------------------------------------------------------------------
# the expectation predictor: the compiler's rules at 6e25528
# --------------------------------------------------------------------------

STAGES = ["frontend", "recognition", "dense_partition", "lowering", "driver"]


def _gcd_content(f: LinearForm) -> int:
    g = 0
    for v in f.coef.values():
        g = math.gcd(g, abs(v))
    return g


def shared_coordinate(a: LinearForm, b: LinearForm) -> Optional[LinearForm]:
    """The logical coordinate two operand subscripts share, or None.

    Identical expressions share it trivially (the coordinate is that
    expression).  Otherwise both must be s * g + o for one linear form g over
    the loop counters with integer s != 0: the same loop combination, in the
    same or the opposite direction (since 2026-10-15, P0: a negative-stride
    view, docs/AFFINE_GEMM_MAPPING.md section 5), each operand with its own
    constant offset and stride -- then the coordinate is the primitive form
    g.  Different loop combinations (4 i0 + i1 against 8 i0 + i1) and
    constants are not one coordinate.  This mirrors matchSharedCoordinate in
    AffineMatmulAnalysis.cpp."""
    if a == b:
        return a
    ga, gb = _gcd_content(a), _gcd_content(b)
    if ga == 0 or gb == 0:
        return None
    pa = {k: v // ga for k, v in a.coef.items()}
    pb = {k: v // gb for k, v in b.coef.items()}
    if pa != pb and pa != {k: -v for k, v in pb.items()}:
        return None
    return LinearForm(pa, 0)


def _domain_form(f: LinearForm, loops) -> LinearForm:
    """f on the iteration domain: a loop that runs once is a constant there
    (its counter has one value) -- the compiler's domain form."""
    once = {lp.var: lp.lb for lp in loops if lp.trip() == 1}
    coef = {k: v for k, v in f.coef.items() if k not in once}
    return LinearForm(coef, f.const + sum(v * once[k] for k, v in f.coef.items() if k in once))


def _form_range(f: LinearForm, loops):
    """The smallest and largest value of f over the box of the loops' values."""
    vals = {lp.var: lp.values() for lp in loops}
    lo = hi = f.const
    for k, v in f.coef.items():
        ends = (v * min(vals[k]), v * max(vals[k]))
        lo += min(ends)
        hi += max(ends)
    return lo, hi


def predict_by_groups(case, forms, loops):
    """The compiler's analysis by groups and roles (AffineMatmulAnalysis.cpp,
    recognizeLogicalMatmul, docs/AFFINE_GEMM_MAPPING.md section 2), used when
    the positional form A = (P*, I, K), B = (P*, K, J), C = (P*, I, J) does
    not apply: loops that occur together in a subscript form one coordinate;
    its role is fixed by the operands that read it; each role's two
    subscripts must be multiples (either sign) of one linear form.  Returns
    ((I, J, K, role_dims, batch_vars, lowering), None, False) with None for a
    role of extent one, or (None, reason, relaxable): relaxable for the three
    refusals the readers analysis lifts (a subscript that is not role-pure:
    AffineMatmulAnalysis.cpp, relaxableRefusal)."""
    trip = {lp.var: lp.trip() for lp in loops}
    dom = {r: [_domain_form(f, loops) for f in forms[r]] for r in ("a", "b", "c")}
    parent = {lp.var: lp.var for lp in loops}

    def find(v):
        while parent[v] != v:
            parent[v] = parent[parent[v]]
            v = parent[v]
        return v

    def support(f):
        # a loop that runs once is a constant on the domain (its term is in
        # the domain form's constant); a loop of zero iterations keeps its
        # coefficient and counts as a reader, as in the C++ (domainForm)
        return [v for v in f.coef if trip.get(v, 2) != 1]

    # The batch loops: read by all three operands (ISSUE-007). C reads them
    # alone; a factor's batch terms are offsets that join nothing.
    read_by = {}
    for r in ("a", "b", "c"):
        for f in dom[r]:
            for v in support(f):
                read_by.setdefault(v, set()).add(r)
    batch_loops = {v for v, rs in read_by.items() if rs == {"a", "b", "c"}}
    index = {lp.var: i for i, lp in enumerate(loops)}
    for d, f in enumerate(dom["c"]):
        sup = support(f)
        mixed = [v for v in sup if v in batch_loops]
        if mixed and any(v not in batch_loops for v in sup):
            return None, ("C's dimension %d mixes a batch coordinate (loop %d, read by all three "
                          "operands) with another coordinate: the output position would move with "
                          "the batch (a coupled or skewed placement, not one dense GEMM)"
                          % (d, min(index[v] for v in mixed))), True

    def grouped(r, f):
        return [v for v in support(f) if v not in batch_loops or r == "c"]

    for r in ("a", "b", "c"):
        for f in dom[r]:
            sup = grouped(r, f)
            for v in sup[1:]:
                parent[find(v)] = find(sup[0])
    # the groups in the order their first subscript appears, the factors in
    # the order of the product (the compiler's operands 0, 1, 2)
    product = ("b", "a", "c") if case.stmt == "+=BA" else ("a", "b", "c")
    factor = {product[0]: "the first factor", product[1]: "the second factor", "c": "C"}
    readers = {}
    for r in product:
        for d, f in enumerate(dom[r]):
            sup = grouped(r, f)
            if not sup:
                continue
            root = find(sup[0])
            g = readers.setdefault(root, {})
            if r in g:
                if root in batch_loops or find(root) in {find(v) for v in batch_loops}:
                    continue    # C may read a batch coordinate in several dimensions
                return None, ("%s reads one logical coordinate in two of its dimensions "
                              "(%d and %d)" % (factor[r], g[r], d)), True
            g[r] = d
    first, second = product[0], product[1]
    groups_first, groups_second, reduction, batch_groups = [], [], [], []
    for root, g in readers.items():
        if root in batch_loops or any(find(v) == root for v in batch_loops):
            batch_groups.append(g)
            continue
        ops = frozenset(g)
        if ops == frozenset("abc"):
            return None, ("a subscript couples loops of different coordinates (their group is read "
                          "by both factors and C, but none of its loops is read by all three): a "
                          "coupled mapping, not one dense GEMM"), True
        if ops == frozenset((first, "c")):
            groups_first.append(g)
        elif ops == frozenset((second, "c")):
            groups_second.append(g)
        elif ops == frozenset((first, second)):
            reduction.append(g)
        else:
            return None, ("a coordinate is read by %s only: the other operands would be "
                          "broadcast along it" % factor[next(iter(ops))]), False
    # A batch coordinate (read by all three): lowered with the batch loops
    # around the tiles (ISSUE-006) when the three operands read it through
    # one and the same expression on the domain.
    if batch_groups and not (groups_first or groups_second or reduction):
        return None, ("%d coordinate(s) are read by all three operands and none by two: an "
                      "element-wise product" % len(batch_groups)), False
    batch_vars = set(batch_loops)
    for groups in (groups_first, groups_second, reduction):
        if len(groups) > 1:
            return None, ("%d separate coordinates are read by one GEMM role" % len(groups)), False
    # Which factor is A (AffineMatmulAnalysis.cpp byGroups): the assignment
    # that transposes least -- C's two role dimensions in order weigh 2, each
    # factor's 1 --, the factors as written on a tie.

    def dim_of(groups, operand):
        return groups[0].get(operand, -1) if groups else -1

    def transposed(x, y):
        return x >= 0 and y >= 0 and x > y

    def cost(akey, bkey, igroups, jgroups):
        return (2 * transposed(dim_of(igroups, "c"), dim_of(jgroups, "c"))
                + transposed(dim_of(igroups, akey), dim_of(reduction, akey))
                + transposed(dim_of(reduction, bkey), dim_of(jgroups, bkey)))
    as_written = (first, second, groups_first, groups_second)
    swapped = (second, first, groups_second, groups_first)
    akey, bkey, igroups, jgroups = swapped if cost(*swapped) < cost(*as_written) else as_written
    roles = {"I": igroups, "J": jgroups, "K": reduction}

    def without_batch(f):
        return LinearForm({v: c for v, c in f.coef.items() if v not in batch_loops}, f.const)
    coords, role_dims = {}, {}
    for role, (x, y) in (("I", (akey, "c")), ("J", (bkey, "c")), ("K", (akey, bkey))):
        if not roles[role]:
            coords[role] = None
            continue
        g = roles[role][0]
        fx, fy = without_batch(forms[x][g[x]]), without_batch(forms[y][g[y]])
        c = shared_coordinate(fx, fy) or shared_coordinate(without_batch(dom[x][g[x]]),
                                                           without_batch(dom[y][g[y]]))
        if c is None:
            return None, ("logical coordinate %s: the two subscripts are not multiples of one "
                          "linear form of the loop counters" % role), False
        coords[role] = c
        role_dims[role] = g
    lowering = {"order": ((akey, "A"), (bkey, "B"), ("c", "C")),
                "general": {"a": False, "b": False, "c": False}}
    return (coords["I"], coords["J"], coords["K"], role_dims, batch_vars, lowering), None, False


def _primitive(v):
    """v divided by the gcd of its entries, its first nonzero entry positive
    (lattice::primitive)."""
    g = 0
    for x in v:
        g = math.gcd(g, abs(x))
    if g == 0:
        return list(v)
    sign = next((1 if x > 0 else -1 for x in v if x != 0), 1)
    return [x // g * sign for x in v]


def _mixed_radix_extent(magnitudes, trips):
    """lattice::mixedRadixExtent: the number of values when
    z -> sum_s c_s z_s on prod_s [0, n_s) is a bijection onto consecutive
    integers (the magnitudes, sorted ascending with their extents, are 1,
    n_(1), n_(1) n_(2), ...; digits of extent 1 ignored), else None."""
    if any(n < 1 for n in trips):
        return None
    expected = 1
    for c, n in sorted((abs(c), n) for c, n in zip(magnitudes, trips) if n != 1):
        if c != expected:
            return None
        expected *= n
    return expected


def _injective_on_box(lam_i, lam_j, M, N):
    """lattice::injectiveOnBox: None when (I, J) -> lam_i I + lam_j J is
    injective on [0, M) x [0, N), else the compiler's collision (dI, dJ): a
    coordinate the map ignores collides along itself ((1, 0) / (0, 1)); rank
    2 is injective; for lam_i = a u, lam_j = b u the smallest collision is
    (b/g, -a/g) with g = gcd(a, b), normalized to dI > 0 or dI = 0 < dJ, a
    collision iff |dI| < M and |dJ| < N."""
    zi, zj = not any(lam_i), not any(lam_j)
    if zi and M > 1:
        return (1, 0)
    if zj and N > 1:
        return (0, 1)
    if zi or zj:
        return None
    if any(a * d != b * c for a, b in zip(lam_i, lam_j) for c, d in zip(lam_i, lam_j)):
        return None             # rank 2
    u = _primitive(lam_i)
    lead = next(i for i, x in enumerate(u) if x != 0)
    a, b = lam_i[lead] // u[lead], lam_j[lead] // u[lead]
    g = math.gcd(abs(a), abs(b))
    di, dj = b // g, -(a // g)
    if di < 0 or (di == 0 and dj < 0):
        di, dj = -di, -dj
    return (di, dj) if abs(di) < M and abs(dj) < N else None


def predict_by_readers(case, forms, loops):
    """The compiler's readers analysis (AffineMatmulAnalysis.cpp, byReaders,
    docs/AFFINE_GEMM_MAPPING.md section 19), run when the analysis by groups
    refuses a nest only because a subscript is not role-pure: every loop's
    role is its readers -- the first factor and C: I, the second factor and
    C: J, both factors: K, all three: a batch loop, one: a broadcast
    (refused) --; each role's subscripts, restricted to the role's loops, are
    multiples of one direction (else separate coordinates, or SQ-1) -- every
    role's direction first --; then every role is a mixed radix of its loops
    and C's view is injective on the (I, J) box (G2); an empty domain (a loop
    of zero iterations) skips those two checks (the dense partition reports
    the empty GEMM, SQ-2).  The coefficients are the front end's: a loop that
    counts down is emitted as an ordinal loop whose counter is lb - |step| t,
    so its coefficient is negated (times |step|).  A is the first factor of
    the product as written.  Returns ((I, J, K, role_dims, batch_vars,
    lowering), None) -- lowering names the operands A, B, C and which of them
    have a general (skewed or merged) view -- or (None, reason)."""
    first, second = ("b", "a") if case.stmt == "+=BA" else ("a", "b")
    ops = (first, second, "c")
    name = {first: "the first factor", second: "the second factor", "c": "C"}
    ir_step = {lp.var: (1 if lp.step < 0 else lp.step) for lp in loops}
    ir_coef = {lp.var: (lp.step if lp.step < 0 else 1) for lp in loops}
    dom = {r: [_domain_form(f, loops) for f in forms[r]] for r in ops}
    # the subscripts in the compiler's order: the first factor's dimensions,
    # the second's, C's
    subs = [(r, d, f) for r in ops for d, f in enumerate(dom[r])]
    readers = {}
    for r, _, f in subs:
        for v, c in f.coef.items():
            if c != 0:
                readers.setdefault(v, set()).add(r)
    role_loops = {"I": [], "J": [], "K": []}
    batch = set()
    for lp in loops:
        rs = readers.get(lp.var)
        if not rs:
            continue
        if rs == set(ops):
            batch.add(lp.var)
        elif rs == {first, "c"}:
            role_loops["I"].append(lp)
        elif rs == {second, "c"}:
            role_loops["J"].append(lp)
        elif rs == {first, second}:
            role_loops["K"].append(lp)
        else:
            return None, ("a coordinate is read by %s only: the other operands would be broadcast "
                          "along it (a stride-0 view, not lowered: docs/AFFINE_GEMM_MAPPING.md "
                          "section 10)" % name[next(iter(rs))])
    if not any(role_loops.values()):
        return None, ("every loop is read by all three operands: no row, column or reduction "
                      "coordinate -- an element-wise product, not a GEMM "
                      "(docs/AFFINE_GEMM_MAPPING.md section 19.1)")
    # each role's direction and every subscript's multiplier of it
    coords, mult, direction = {}, {}, {}
    for role, lps in role_loops.items():
        if not lps:
            coords[role] = None
            mult[role] = [0] * len(subs)
            continue
        rest = [[f.coef.get(lp.var, 0) * ir_coef[lp.var] for lp in lps] for _, _, f in subs]
        for p in range(len(rest)):
            for q in range(p + 1, len(rest)):
                u, w = rest[p], rest[q]
                if all(u[x] * w[y] == u[y] * w[x] for x in range(len(lps)) for y in range(len(lps))):
                    continue
                (rp, dp, _), (rq, dq, _) = subs[p], subs[q]
                if rp == rq:
                    return None, ("%s reads logical coordinate %s through two different "
                                  "combinations of its loops (dimensions %d and %d): separate "
                                  "coordinates in one GEMM role need a merged (reshaped) view, "
                                  "not lowered" % (name[rp], role, dp, dq))
                return None, ("logical coordinate %s (%s's dimension %d %s, %s's dimension %d %s): "
                              "the subscripts are not multiples of one linear form of the role's "
                              "loops -- operand-specific decompositions of one coordinate are not "
                              "lowered (spec question SQ-1)"
                              % (role, name[rp], dp, case.arrays[rp].subs[dp], name[rq], dq,
                                 case.arrays[rq].subs[dq]))
        g = _primitive(next(x for x in rest if any(x)))
        lead = next(i for i, x in enumerate(g) if x != 0)
        direction[role] = g
        coords[role] = LinearForm({lp.var: c for lp, c in zip(lps, g)}, 0)
        mult[role] = [x[lead] // g[lead] for x in rest]
    # every operand's layout: per dimension the multipliers of its two roles
    roles_of = {first: ("I", "K"), second: ("K", "J"), "c": ("I", "J")}
    layout = {r: [] for r in ops}
    for index, (r, d, _) in enumerate(subs):
        layout[r].append(tuple(mult[role][index] for role in roles_of[r]))
    general, role_dims = {}, {"I": {}, "J": {}, "K": {}}
    for r in ops:
        dims_of = {0: [], 1: []}
        for d, cs in enumerate(layout[r]):
            for slot in (0, 1):
                if cs[slot] != 0:
                    dims_of[slot].append(d)
        general[r] = (any(cs[0] != 0 and cs[1] != 0 for cs in layout[r])
                      or any(len(v) > 1 for v in dims_of.values()))
        if not general[r]:
            for slot in (0, 1):
                if dims_of[slot]:
                    role_dims[roles_of[r][slot]][r] = dims_of[slot][0]
    if all(lp.trip() != 0 for lp in loops):
        # the roles' ordinals: scale * a mixed radix of the loops' ordinals
        ordinal = {}
        for role in "IJK":
            lps = role_loops[role]
            if not lps:
                ordinal[role] = (1, 1)
                continue
            magnitudes = [c * ir_step[lp.var] for lp, c in zip(lps, direction[role])]
            scale = 0
            for c in magnitudes:
                scale = math.gcd(scale, abs(c))
            extent = _mixed_radix_extent([c // scale for c in magnitudes], [lp.trip() for lp in lps])
            if extent is None:
                return None, ("logical coordinate %s is driven by %d loops that do not enumerate it "
                              "densely (or its loops' bounds are not constant): no box of %s values "
                              "(with all but one of them as batch loops it would be a batched GEMM, "
                              "which is not constructed)" % (role, len(lps), role))
            ordinal[role] = (scale, extent)
        # G2: C's view injective on the (I, J) box
        lam_i = [cs[0] * ordinal["I"][0] for cs in layout["c"]]
        lam_j = [cs[1] * ordinal["J"][0] for cs in layout["c"]]
        collision = _injective_on_box(lam_i, lam_j, ordinal["I"][1], ordinal["J"][1])
        if collision is not None:
            return None, ("C's view is not injective on the (I, J) box: the outputs with ordinals "
                          "(I, J) and (I + %d, J + %d) write the same element of C, so one tile would "
                          "lose an update (G2, docs/AFFINE_GEMM_MAPPING.md section 19.3); a "
                          "decomposition with the J (or I) loops as batch loops would be valid, and "
                          "is not constructed" % collision)
    role_dims = {role: g for role, g in role_dims.items() if g}
    lowering = {"order": ((first, "A"), (second, "B"), ("c", "C")), "general": general}
    return (coords["I"], coords["J"], coords["K"], role_dims, batch, lowering), None


def groups_or_readers(case, forms, loops):
    """The analysis by groups, then -- when it refuses only because a
    subscript is not role-pure -- the readers analysis
    (AffineMatmulAnalysis.cpp, groupsOrReaders).  Returns (result, None) or
    (None, reason)."""
    grouped, why, relaxable = predict_by_groups(case, forms, loops)
    if grouped is not None or not relaxable:
        return grouped, why
    return predict_by_readers(case, forms, loops)


def _hoist_blocked(case, loops, used_vars, max_points=200000):
    """Whether hoistRepetitionLoops leaves the nest alone: the index of the
    loop the compiler names (its last repetition loop) when some repetition
    loop of two or more iterations cannot be moved outward, else None.  The
    interchange is illegal iff two iterations that write the same element of
    C differ in a used loop enclosing the repetition loop: the dependence
    between them then runs backwards through the repetition loop once it is
    outermost (affine::isValidLoopInterchangePermutation).  Decided by
    enumerating C's elements on the domain of the used loops (a domain too
    large to enumerate falls back to the reduction-loop rule of the dense
    GEMMs: a K loop -- read by both factors, not by C -- of two or more
    iterations encloses the repetition loop)."""
    import itertools
    rep = [i for i, lp in enumerate(loops) if lp.var not in used_vars and lp.trip() >= 2]
    # an empty nest (any loop of zero iterations, used or not) has no
    # dependence: every interchange is legal
    if not rep or any(lp.trip() == 0 for lp in loops):
        return None
    used = [lp for lp in loops if lp.var in used_vars]
    total = 1
    for lp in used:
        total *= lp.trip()
    if total == 0:
        return None
    if total > max_points:
        readers = {}
        for r in ("a", "b", "c"):
            for f in case.arrays[r].forms():
                for v in f.vars():
                    readers.setdefault(v, set()).add(r)
        k_idx = [i for i, lp in enumerate(loops) if lp.trip() >= 2 and readers.get(lp.var) == {"a", "b"}]
        return rep[-1] if any(any(ki < ri for ki in k_idx) for ri in rep) else None
    cforms = case.c.forms()
    names = [lp.var for lp in used]
    by_element = {}
    for x in itertools.product(*[lp.values() for lp in used]):
        env = dict(zip(names, x))
        by_element.setdefault(tuple(f.eval(env) for f in cforms), []).append(env)
    for ri in rep:
        before = [lp.var for i, lp in enumerate(loops) if i < ri and lp.var in used_vars and lp.trip() >= 2]
        if not before:
            continue
        if any(len({tuple(e[v] for v in before) for e in pts}) > 1 for pts in by_element.values()):
            return rep[-1]
    return None


def predict(case: Case) -> dict:
    """What each stage of the compiler is expected to do, from its rules as
    read in the sources (README.md, 'What the compiler supports').  Fields:
      frontend:          'translated' | 'rejected'
      recognition:       'accept' | 'reject' | 'not_reached'
      dense_partition:   'supported' | 'unsupported' | 'not_reached'
      lowering:          'supported' | 'unsupported' | 'not_reached'
      driver:            'offloaded' | 'stays_on_host' | 'not_translated'
      hoisting_required: a repetition loop between the GEMM's loops must be
                         (and can be) moved outward first; the discovery test
                         pass does not do that, --affine-matmul-to-systolic does
      reject_stage:      None or the first stage that stops the offload
      reject_reason:     text (the compiler's own wording where it has one)
    """
    an = case.analysis or analyse(case)
    ex = {s: "not_reached" for s in STAGES}
    ex["hoisting_required"] = False
    ex["reject_stage"] = None
    ex["reject_reason"] = ""

    def stop(stage, reason):
        ex["reject_stage"] = stage
        ex["reject_reason"] = reason
        ex["driver"] = "stays_on_host" if stage != "frontend" else "not_translated"
        case.expect = ex
        return ex

    # ---- front end: the affine C subset
    if case.kernel_style == "scalar_acc":
        ex["frontend"] = "rejected"
        return stop("frontend", "local variables are not supported (only for-loop counters)")
    # A loop that counts down ('>' / '>=' with --, -=, = i - S) is translated
    # since 2026-10-15: systolic-cfront emits the ordinal affine.for 0 to N
    # and the counter LB - S * t, i.e. the coordinate's coefficient times the
    # (negative) step below -- the same model as before for the later stages.
    if not an["affine"]:
        ex["frontend"] = "rejected"
        return stop("frontend", "subscript is not an affine expression of the loop counters")
    ex["frontend"] = "translated"

    # ---- recognition: recognizeLogicalMatmul (after hoistRepetitionLoops in
    #      the lowering pass)
    loops = case.loops
    forms = {r: a.forms() for r, a in case.arrays.items()}
    used_vars = set()
    for fl in forms.values():
        for f in fl:
            used_vars |= f.vars()
    # The compiler compares subscripts on the iteration domain: a loop that
    # runs once is a constant there (docs/AFFINE_GEMM_MAPPING.md, Lemma 0).
    # (used_vars stays syntactic: a loop is a repetition loop when no access
    # mentions it.)
    sforms = forms
    forms = {r: [_domain_form(f, loops) for f in fl] for r, fl in forms.items()}
    if case.stmt in ("+=alpha", "+=triple"):
        ex["recognition"] = "reject"
        return stop("recognition", "both operands of the product must be affine.load results; "
                    "a scalar factor or a third factor makes one of them an arith.mulf")
    if case.stmt == "+=sum":
        ex["recognition"] = "reject"
        return stop("recognition", "the accumulated value must be an arith.mulf of two loads, "
                    "not an arith.addf")
    if case.stmt == "=AB":
        ex["recognition"] = "reject"
        return stop("recognition", "the stored value must be an arith.addf of the accumulator "
                    "load and a product; there is no accumulation")
    if case.stmt == "-=":
        ex["recognition"] = "reject"
        return stop("recognition", "the stored value is an arith.subf, not an arith.addf")
    if case.kernel_style == "init_inside":
        ex["recognition"] = "reject"
        return stop("recognition", "the nest is not a perfect chain of affine.for ops around the "
                    "MAC (C is initialised between the j and k loops)")
    if len({a.elem for a in case.arrays.values()}) > 1:
        ex["recognition"] = "reject"
        return stop("recognition", "the MAC body contains arith.extf/truncf conversions between "
                    "element types; only affine.load/store, arith.mulf and arith.addf are allowed")
    # Any number of loops (a role of extent one needs none, P0 since
    # 2026-10-15): the positional form first -- the checkpoint's rule --,
    # then the analysis by groups and roles (transposes, extent-one roles,
    # other ranks).
    if not used_vars:
        ex["recognition"] = "reject"
        return stop("recognition", "no loop counter is used by the accesses")
    nr = [len(forms[r]) for r in ("a", "b", "c")]
    positional = len(set(nr)) == 1 and nr[0] >= 2
    npart = nr[0] - 2 if positional else 0
    fa, fb, fc = forms["a"], forms["b"], forms["c"]

    def share(x, y, dx, dy):
        """matchSharedCoordinate: the expressions first; when they do not
        match and a loop that runs once is involved, their domain forms --
        refused when either is constant there."""
        c = shared_coordinate(x, y)
        if c is not None or (x == dx and y == dy):
            return c
        if dx.is_const() or dy.is_const():
            return None
        return shared_coordinate(dx, dy)

    def matches(ka, kb):
        """The shared coordinates (I, J, K), or None: the positional form
        A = (P*, I, K), B = (P*, K, J), C = (P*, I, J) on the subscripts as
        written (AffineMatmulAnalysis.cpp, tried first), with the factors
        ka, kb.  Returned on the iteration domain (for the dense checks)."""
        a, b, c = sforms[ka], sforms[kb], sforms["c"]
        da, db, dc = forms[ka], forms[kb], forms["c"]
        for p in range(npart):
            if a[p] != c[p] or b[p] != c[p]:
                return None
        I = share(a[npart], c[npart], da[npart], dc[npart])
        J = share(b[npart + 1], c[npart + 1], db[npart + 1], dc[npart + 1])
        K = share(a[npart + 1], b[npart], da[npart + 1], db[npart])
        if I is None or J is None or K is None:
            return None
        if I == J or I == K or J == K:
            return None
        return tuple(_domain_form(f, loops) for f in (I, J, K))

    ka, kb = "a", "b"       # the arrays that are logical A and B
    m = matches("a", "b") if positional else None
    if m is None and positional:
        m = matches("b", "a")
        if m is not None:
            fa, fb = fb, fa   # the product's operands swapped: logical A is the C-side's B
            ka, kb = "b", "a"
    grouped = None
    # The loops that drive a batch coordinate (re-emitted around the tiles,
    # ISSUE-006): with the positional form the prefix subscripts' loops.
    batch_vars = set()
    for p in range(npart):
        batch_vars |= forms["c"][p].vars()
    # A positional reading in which one loop drives two of its coordinates --
    # the prefix (as one), I, J, K -- is never lowerable (a prefix loop in
    # I, J or K; A[i][k + j] with B[k + j][j], j in K and J): the analysis by
    # groups (then the readers analysis) decides such a nest, and its refusal
    # is the reason (ISSUE-007, AffineMatmulAnalysis.cpp, positionalIsCoupled;
    # since 2026-10-16 the positional reading is no longer restored).
    def coupled(m):
        drives = {}
        for cls in [batch_vars] + [f.vars() for f in m if f is not None]:
            for v in cls:
                drives[v] = drives.get(v, 0) + 1
        return any(n >= 2 for n in drives.values())
    if m is not None and coupled(m):
        m = None
    if m is None:
        grouped, why = groups_or_readers(case, forms, loops)
        if grouped is None:
            ex["recognition"] = "reject"
            return stop("recognition", why)
        npart = 0
        m = grouped[:3]
        batch_vars = grouped[4]
    I, J, K = m
    # repetition loops: outside the used loops they are fine; between them
    # they are hoisted outward when the interchange is legal
    # (affine::isValidLoopInterchangePermutation): illegal exactly when two
    # iterations writing one C element differ in a loop that encloses the
    # repetition loop (the dependence then runs backwards through it) --
    # a reduction loop, or a batch or role loop whose C windows overlap
    # (C[i + k][j], since C may read batch loops in its role dimensions).
    # Decided by enumeration, _hoist_blocked; a single-iteration repetition
    # or reduction reorders nothing (failures/resolved/NOTE-001).
    used_idx = [i for i, lp in enumerate(loops) if lp.var in used_vars]
    rep_idx = [i for i, lp in enumerate(loops) if lp.var not in used_vars]
    blocked = _hoist_blocked(case, loops, used_vars)
    if blocked is not None:
        ex["recognition"] = "reject"
        return stop("recognition", "loop %d of the nest repeats the accumulation inside it, and "
                    "moving it outward is not a legal loop interchange (it would reorder "
                    "dependent iterations)" % blocked)
    if any(ri > used_idx[0] for ri in rep_idx):
        ex["hoisting_required"] = True
    ex["recognition"] = "accept"

    # ---- dense partition: getDenseLogicalGemm
    used_loops = [loops[i] for i in used_idx]
    if any(lp.trip() == 0 for lp in used_loops):
        # normalizeLogicalCoordinates refuses an empty loop (upper <= lower)
        # and records it (LogicalMatmul::normalizationEmptyLoop), so
        # getDenseLogicalGemm reports an empty GEMM -- SQ-2 leaves open
        # whether it should be offloaded instead
        ex["dense_partition"] = "unsupported"
        return stop("dense_partition", "a loop of the nest has no iteration (an empty GEMM: M, N or K is 0)")
    for s, lp in enumerate(used_loops):
        if lp.trip() == 1:
            continue      # constant on the domain: drives nothing (Theorem 3)
        drives = [name for name, f in (("I", I), ("J", J), ("K", K)) if f is not None and lp.var in f.coef]
        if lp.var in batch_vars:
            # a batch loop: it must drive no I, J or K (the batches would be
            # coupled); it is not required to enumerate anything densely
            if drives:
                ex["dense_partition"] = "unsupported"
                return stop("dense_partition", "two logical coordinates are driven by the same loop "
                            "(loop %d drives a batch coordinate and %s)" % (s, " and ".join(drives)))
            continue
        if len(drives) > 1:
            ex["dense_partition"] = "unsupported"
            return stop("dense_partition", "two logical coordinates are driven by the same loop "
                        "(loop %d drives %s)" % (s, " and ".join(drives)))
        if not drives:
            ex["dense_partition"] = "unsupported"
            return stop("dense_partition", "loop %d drives no logical coordinate" % s)
    for name, f in (("I", I), ("J", J), ("K", K)):
        if f is None:
            continue      # a role of extent one (SQ-4, decided 2026-10-15)
        scaled = []
        for lp in used_loops:
            if lp.var in f.coef:
                # a coordinate that decreases along the loop (a negative
                # coefficient or a loop that counts down) is read backwards
                # by getDenseLogicalGemm: its magnitude enumerates it
                sc = abs(f.coef[lp.var] * lp.step)
                scaled.append((sc, lp.trip()))
        if not scaled:
            continue      # constant on the domain: an extent-one coordinate
        scaled.sort()
        expected = scaled[0][0]
        for sc, ext in scaled:
            if sc != expected:
                ex["dense_partition"] = "unsupported"
                return stop("dense_partition", "logical coordinate %s is driven by %d loops that do "
                            "not enumerate it densely (scales must grow by the finer loops' "
                            "extents)" % (name, len(scaled)))
            expected *= ext
    # a batched nest whose per-batch GEMM has two unit extents (a batch of
    # dot products, or of scaled vectors) is lowered since SQ-5 was decided
    # (2026-10-16); all three unit extents is an element-wise product, refused
    # (docs/AFFINE_GEMM_MAPPING.md section 19.1; the positional form reads
    # such a nest when the role subscripts are distinct constants)
    if batch_vars and all(f is None or not any(lp.trip() >= 2 for lp in used_loops if lp.var in f.coef)
                          for f in (I, J, K)):
        ex["dense_partition"] = "unsupported"
        return stop("dense_partition", "every loop is read by all three operands: a batched product "
                    "with M = N = K = 1 is an element-wise product, not a GEMM "
                    "(docs/AFFINE_GEMM_MAPPING.md section 19.1)")
    ex["dense_partition"] = "supported"

    # ---- lowering: --affine-matmul-to-systolic (any rank since P0)
    if grouped is not None:
        # every dimension's index range inside the array: the fixed
        # (constant) dimensions first, then the roles' -- getLogicalOperand's
        # order; the leading dimension of a C array parameter is dynamic
        role_dims = grouped[3]
        lowering = grouped[5]
        order = {lowering["order"][0][0]: ("I", "K"), lowering["order"][1][0]: ("K", "J"),
                 "c": ("I", "J")}
        for r, nm in lowering["order"]:
            arr = case.arrays[r]
            if lowering["general"][r]:
                # a skewed or merged view (docs/AFFINE_GEMM_MAPPING.md section
                # 19): recognized, its lowering is not implemented yet
                ex["lowering"] = "unsupported"
                return stop("lowering", "%s has a skewed or merged view (a dimension combining "
                            "both of its roles, or a role across several dimensions: "
                            "docs/AFFINE_GEMM_MAPPING.md section 19), which is not lowered yet" % nm)
            rdims = [role_dims[g][r] for g in order[r] if g in role_dims and r in role_dims[g]]
            dims = [d for d in range(len(forms[r])) if d not in rdims] + rdims
            for d in dims:
                lo, hi = _form_range(forms[r][d], loops)
                if lo < 0 or (d >= 1 and hi >= arr.dims[d]):
                    ex["lowering"] = "unsupported"
                    return stop("lowering", "%s is accessed out of bounds along dimension %d" % (nm, d))
        if case.a.elem != case.c.elem or case.b.elem != case.c.elem:
            ex["lowering"] = "unsupported"
            return stop("lowering", "A, B and C have different element types")
        for r, nm in lowering["order"][:2]:
            arr = case.arrays[r]
            if arr.name == case.c.name or not (arr.restrict or case.c.restrict):
                ex["lowering"] = "unsupported"
                return stop("lowering", "%s may overlap C; declare the arguments llvm.noalias (C restrict) "
                            "or pass assume-no-alias" % nm)
        if case.a.name == case.b.name and forms["a"] != forms["b"]:
            ex["lowering"] = "unsupported"
            return stop("lowering", "A and B are different views of the same memref")
        ex["lowering"] = "supported"
        if case.elem != "float":
            ex["driver"] = "stays_on_host"
            ex["reject_stage"] = "driver"
            ex["reject_reason"] = "the FPGA datapath is fp32 and it uses double arrays"
            case.expect = ex
            return ex
        ex["driver"] = "offloaded"
        case.expect = ex
        return ex

    def dense_of(f):
        """The view an operand's own subscript f (over the loops of one
        coordinate) spans: (first index, stride, extent) -- getLogicalOperand
        builds the memref.subview from the operand's view of the coordinate."""
        lps = [lp for lp in used_loops if lp.var in f.coef]
        if not lps:
            return f.const, 1, 1      # an extent-one coordinate: one index
        # the smallest index: each loop at its first value if the subscript
        # grows along it, at its last value if it shrinks (read backwards)
        off = f.const + sum(f.coef[lp.var] * (lp.lb if f.coef[lp.var] * lp.step > 0
                                              else lp.lb + lp.step * (lp.trip() - 1))
                            for lp in lps)
        scale = min(abs(f.coef[lp.var] * lp.step) for lp in lps)
        ext = volume([lp.trip() for lp in lps])
        return off, scale, ext

    # per operand: A = (I, K), B = (K, J), C = (I, J) through its own subscripts
    vA = (dense_of(fa[npart]), dense_of(fa[npart + 1]))
    vB = (dense_of(fb[npart]), dense_of(fb[npart + 1]))
    vC = (dense_of(fc[npart]), dense_of(fc[npart + 1]))
    for r, nm in ((ka, "A"), (kb, "B"), ("c", "C")):
        arr = case.arrays[r]
        for p in range(npart):
            # a batch dimension: the batch index's range on the domain (the
            # leading dimension of a C array parameter is dynamic, unchecked)
            lo, hi = _form_range(forms[r][p], loops)
            if lo < 0 or (p >= 1 and hi >= arr.dims[p]):
                ex["lowering"] = "unsupported"
                return stop("lowering", "%s is accessed out of bounds along dimension %d" % (nm, p))
    for r, nm, (d0, d1) in ((ka, "A", vA), (kb, "B", vB), ("c", "C", vC)):
        arr = case.arrays[r]
        for pos, (off, scale, ext) in enumerate((d0, d1)):
            last = off + scale * (ext - 1)
            # the leading dimension of a C array parameter is dynamic in the
            # memref (not checked by the pass); the others are static
            dim = npart + pos
            if off < 0 or (dim >= 1 and last >= arr.dims[dim]):
                ex["lowering"] = "unsupported"
                return stop("lowering", "%s is accessed out of bounds along dimension %d" % (nm, dim))
    if case.a.elem != case.c.elem or case.b.elem != case.c.elem:
        ex["lowering"] = "unsupported"
        return stop("lowering", "A, B and C have different element types")
    for r, nm in ((ka, "A"), (kb, "B")):
        arr = case.arrays[r]
        if arr.name == case.c.name:
            ex["lowering"] = "unsupported"
            return stop("lowering", "%s may overlap C" % nm)
        if not (arr.restrict or case.c.restrict):
            ex["lowering"] = "unsupported"
            return stop("lowering", "%s may overlap C; declare the arguments llvm.noalias (C restrict) "
                        "or pass assume-no-alias" % nm)
    if case.a.name == case.b.name and vA != vB:
        ex["lowering"] = "unsupported"
        return stop("lowering", "A and B are different views of the same memref")
    ex["lowering"] = "supported"

    # ---- driver: systolic-cc policy
    if case.elem != "float":
        ex["driver"] = "stays_on_host"
        ex["reject_stage"] = "driver"
        ex["reject_reason"] = "the FPGA datapath is fp32 and it uses double arrays"
        case.expect = ex
        return ex
    ex["driver"] = "offloaded"
    case.expect = ex
    return ex


# --------------------------------------------------------------------------
# the specification: what a correct recognizer must do with the case
# --------------------------------------------------------------------------
#
# The expectation of a test is set by the mathematics of the loop nest and by
# the support specification (README.md, 'Support specification'), never by
# what the current compiler prints.  spec_expect() returns
#
#   recognition: ACCEPT        a dense GEMM inside the specified envelope
#                REJECT        not a dense GEMM of the given operands
#                OUT_OF_SCOPE  a valid GEMM that the specification excludes
#                              explicitly (a documented limitation)
#   basis:       where the rule comes from (file, test, or the task statement)
#
# classify_case() then compares it with predict() -- the current
# implementation -- and names the case:
#
#   TRUE_POSITIVE          spec ACCEPT, implementation offloads
#   FALSE_NEGATIVE         spec ACCEPT, implementation refuses   <- a compiler issue
#   TRUE_NEGATIVE          spec REJECT, implementation refuses
#   FALSE_POSITIVE         spec REJECT, implementation offloads  <- must never happen
#   DOCUMENTED_LIMITATION  spec OUT_OF_SCOPE, implementation refuses
#   UNEXPECTED_ACCEPT      spec OUT_OF_SCOPE, implementation offloads

# Questions the specification (README.md section 2) does not settle.  A case
# whose expectation depends on one is UNDETERMINED: it is run, its behaviour
# is recorded, its program must still compute the right result, but neither
# 'offloaded' nor 'refused' counts as a failure until the question is decided
# (in README.md, in writing) -- never by the output of the compiler.
# Decided questions leave this table: SQ-4 (2026-10-15, a role of extent one
# is offloaded) and SQ-5 (2026-10-16, a batched GEMM whose per-batch GEMM has
# two of M, N, K equal to 1 is offloaded -- basis 'sq5_decided').
SPEC_QUESTIONS = {
    "SQ-1": "a coordinate split over several loops with a different mixed radix per operand "
            "(A[4*i0 + i1][k] with C[8*i0 + i1][j]): the definition's 'injective affine function "
            "of the coordinate's loops' admits it, the 'inside' list does not name it, and the "
            "operand views are not strided sub-matrices (a rank-2 memref.subview cannot express "
            "them) -- inside the specification (with a copy or a rank-3 view) or a documented "
            "limitation?",
    "SQ-2": "an empty GEMM (a loop of zero iterations, M, N or K = 0): the nest computes nothing; "
            "must the compiler offload it (zero invocations) or refuse it?  Either keeps the "
            "result correct; the specification does not say which",
    "SQ-3": "a loop with a symbolic bound (affine.for 0 to %n: a GEMM extent that is a runtime "
            "value): unreachable from the C front end, whose bounds must be constants; is the MLIR "
            "path's refusal ('no static normalized extent') a documented limitation, or must dynamic "
            "extents be supported?",
}

SPEC_BASIS = {
    "dense_gemm": "a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h "
                  "('loops in any order, steps, nonzero origins, access offsets and strides, and "
                  "coordinates split over several loops ... are all of this form'); include/Systolic/"
                  "AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o "
                  "(P,I,J): one factor map per operand); the task statement's index family "
                  "c_i*i + c_j*j + c_k*k + c_0 per operand",
    "batched": "(supported since 2026-10-15, ISSUE-006) a batched GEMM, every operand carrying the "
               "batch coordinate(s) through one and the same expression: the batch loops are "
               "re-emitted around the tiles, one GEMM per batch iteration on its own slice of each "
               "operand (docs/AFFINE_GEMM_MAPPING.md section 2; AffineMatmulToSystolic.cpp)",
    "batched_views": "(supported since 2026-10-16, ISSUE-007) a batched GEMM whose operands read "
                     "the batch coordinate(s) through their own expressions -- in dimensions of "
                     "their own or as affine offsets of role dimensions (A[4i - j][5k], C[i + k][j] "
                     "since section 19.3): per batch value an ordinary GEMM of strided views whose "
                     "offsets are affine in the batch counters, C injective per batch "
                     "(docs/AFFINE_GEMM_MAPPING.md sections 16, 19)",
    "sq5_decided": "(SQ-5, decided 2026-10-16: docs/AFFINE_GEMM_MAPPING.md section 19.8) a batched "
                   "GEMM whose per-batch GEMM has two of M, N, K equal to 1 -- a batch of dot "
                   "products (C[i][j] += A[i][j + k] B[i + k][j]) or of scaled vectors (the "
                   "reduction index in the output) -- is a batched GEMM of affine views: lowered, "
                   "one small tile per batch value",
    "generalized_views": "(since 2026-10-16, docs/AFFINE_GEMM_MAPPING.md section 19) a GEMM, or a "
                         "batched GEMM, of general affine operand views -- a dimension combining "
                         "both of an operand's roles (skewed: A[i][i + k]), both roles in one "
                         "dimension (merged: C[4 i + j]), C reading batch loops in its role "
                         "dimensions -- decided by enumeration (casespec.generalized_gemm_on_domain: "
                         "roles by readers, every role one dense coordinate of its loops, so (P, I, "
                         "J, K) a bijection onto the box, C injective per batch)",
    "demotion": "(docs/AFFINE_GEMM_MAPPING.md section 19.2) the canonical decomposition (the roles "
                "by readers) fails -- two coordinates in one role, a role that is not one dense "
                "coordinate, C not injective per batch -- but another decomposition of section 19.1 "
                "is valid (found by casespec.generalized_gemm_on_domain's search: loops demoted to "
                "batch loops, a batched GEMM of smaller tiles); the compiler constructs only the "
                "canonical decomposition: a documented limitation",
    "broadcast": "(docs/AFFINE_GEMM_MAPPING.md sections 19.1, 19.2, 10) a loop read by one operand "
                 "only: a GEMM of affine views in which that loop is a role the other operand's view "
                 "is constant along (a stride-0 view, C[i][j] += A[i][k] * B[k][i]: j as J with A and "
                 "B constant along it is excluded by G2 unless N = 1, so j is K with A constant along "
                 "it, or a batch loop), found by casespec.generalized_gemm_on_domain's search; the "
                 "compiler refuses the broadcast (section 10: a stride-0 view is not lowered): a "
                 "documented limitation",
    "batched_shared": "a batched GEMM with an operand shared by every batch (no batch subscript: "
                      "C[b] += A[b] B): valid -- B broadcast over the batch, or one stacked GEMM "
                      "with A reshaped -- but the operand would need a stride-0 or merged view, "
                      "not lowered (the recognizer: two coordinates read by one GEMM role)",
    "transposed": "(no longer a limitation since 2026-10-15, P0.2) a transposed operand is a "
                  "permuted view (memref.transpose of its subview), docs/AFFINE_GEMM_MAPPING.md "
                  "section 5; a dense GEMM like any other",
    "alpha": "lib/Systolic/Transforms/AffineMatmulToSystolic.cpp header: the recognized form is "
             "C[..] += A[..] * B[..]; systolic.matmul_tile computes C_in + A B, there is no alpha",
    "init_inside": "AffineMatmulAnalysis.cpp recognizeLogicalMatmul: 'The anchored nest must be a perfect "
                   "chain of affine.for ops around the MAC and nothing else ... an initialization of C "
                   "between the j and k loops'; test/Systolic/affine_to_systolic_refused.mlir @initialized",
    "scalar_acc": "tools/systolic-cfront/systolic-cfront.c: local variables are not supported; "
                  "test/CFront/unsupported.c accumulator",
    "double": "tools/systolic-cc/systolic-cc.c: 'the FPGA datapath is fp32 and it uses double arrays'; "
              "test/SystolicCC/program.c matmul_f64",
    "mixed_types": "recognizeLogicalMatmul: the MAC body may contain only affine.load/store, arith.mulf "
                   "and arith.addf (no conversions); the datapath is fp32",
    "aliasing": "AffineMatmulToSystolic.cpp provablyDisjointFromOutput: A and B must be provably disjoint "
                "from C (restrict / fresh allocations); test/Systolic/affine_to_systolic_refused.mlir @may_alias",
    "negative_scale": "(no longer a limitation since 2026-10-15, P0.3) two operands that run through a "
                      "shared coordinate in opposite directions (A[7 - i] with C[i]): one operand's "
                      "view has a negative stride (memref.subview with a negative static stride, its "
                      "offset the largest index), docs/AFFINE_GEMM_MAPPING.md section 5",
    "repeat_in_reduction": "AffineMatmulToSystolic.cpp hoistRepetitionLoops: a repeating loop of more than "
                           "one iteration cannot be moved outward when two iterations writing one element "
                           "of C differ in a loop enclosing it -- a reduction loop, or (since C may read "
                           "batch loops in its role dimensions, 2026-10-16) a batch or role loop whose C "
                           "windows overlap, C[i + k][j]: affine::isValidLoopInterchangePermutation "
                           "refuses it, the accumulation order would change; test/SystolicCC/depth.c "
                           "d4_illegal; decided by enumeration (casespec.repeat_in_reduction)",
    "same_memref_views": "AffineMatmulToSystolic.cpp: 'A and B are different views of the same memref'",
    "matvec": "(no longer a limitation since 2026-10-15, P0) a matrix-vector product is the GEMM "
              "with N = 1: its rank-1 operands get a unit dimension (memref.expand_shape), "
              "docs/AFFINE_GEMM_MAPPING.md section 2",
    "not_gemm": "the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)",
    "dense_on_domain": "(P0.4, P0.5) not a GEMM as written, but its loops that run once are constants "
                       "on the iteration domain, and there the nest is a dense GEMM of strided views "
                       "(decided by enumerating the points: casespec.gemm_on_domain; "
                       "docs/AFFINE_GEMM_MAPPING.md Lemma 0, section 3)",
    "decreasing_loop": "(no longer a limitation since 2026-10-15, ISSUE-004) a loop that counts down "
                       "enumerates its coordinate in decreasing order (i = 5, 3, 1 is I = 5 - 2 "
                       "ordinal); the front end translates it to an ordinal loop and the recognizer "
                       "reads the coordinate backwards",
    "out_of_bounds": "the subscripts leave a static array dimension: the nest has no defined "
                     "meaning as a program, so there is no GEMM to lower and offloading it would be "
                     "wrong code; the lowering must refuse it and say so (AffineMatmulToSystolic.cpp "
                     "getLogicalOperand: 'accessed out of bounds along dimension'; "
                     "AffineMatmulPartitioning.cpp getDenseLogicalGemm when the coordinate's values "
                     "overflow 64-bit arithmetic -- ISSUE-002)",
    "SQ-1": "UNDETERMINED -- " + SPEC_QUESTIONS["SQ-1"],
    "SQ-2": "UNDETERMINED -- " + SPEC_QUESTIONS["SQ-2"],
    "SQ-3": "UNDETERMINED -- " + SPEC_QUESTIONS["SQ-3"],
}


def gemm_on_domain(case: Case, max_points: int = 200000) -> Optional[bool]:
    """Whether the nest is a GEMM of strided views ON ITS ITERATION DOMAIN, by
    enumerating the points (docs/AFFINE_GEMM_MAPPING.md sections 3, 5; the
    method of test/Systolic/Inputs/affine_gemm_mapping_oracle.py, independent
    of the compiler): every non-constant subscript's values form an arithmetic
    progression; two subscripts address one coordinate iff their ranks agree
    or mirror; each coordinate is read by exactly two operands (A and C, B and
    C, A and B -- the factors either way); (I, J, K) is a bijection onto the
    box.  None when the domain is too large to enumerate."""
    import itertools
    # Only the loops an access uses: a repetition loop repeats the whole
    # GEMM (in scope outside the reduction; the caller checks that).
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    loops = [lp for lp in case.loops if lp.var in used]
    vals = [lp.values() for lp in loops]
    total = 1
    for v in vals:
        total *= len(v)
    if total == 0 or total > max_points:
        return None
    names = [lp.var for lp in loops]
    pts = [dict(zip(names, x)) for x in itertools.product(*vals)]
    found = _gemm_on_points(case, pts)
    if found is None:
        return False
    # a repetition loop whose hoist would reorder the accumulations into one
    # element of C (the rule of spec_expect, repeat_in_reduction)
    if repeat_in_reduction(case):
        return "repeat_in_reduction"
    return True


def _gemm_on_points(case: Case, pts):
    """The enumeration of gemm_on_domain on a set of points (the whole
    domain, or one batch iteration): (extents, roles, ordinals) when the
    points form a GEMM of strided views -- `roles` names each role's
    (operand, dimension, step) readers -- else None."""
    readers = []
    for r in ("a", "b", "c"):
        for d, f in enumerate(case.arrays[r].forms()):
            vs = [f.eval(env) for env in pts]
            dist = sorted(set(vs))
            if len(dist) == 1:
                continue
            step = dist[1] - dist[0]
            if any(b2 - a2 != step for a2, b2 in zip(dist, dist[1:])):
                return None
            readers.append((r, tuple((v - dist[0]) // step for v in vs), len(dist), d, step))
    classes = []
    for rd in readers:
        for cl in classes:
            r0 = cl[0]
            if r0[2] == rd[2] and (r0[1] == rd[1] or r0[1] == tuple(r0[2] - 1 - v for v in rd[1])):
                cl.append(rd)
                break
        else:
            classes.append([rd])
    for a, b in (("a", "b"), ("b", "a")):
        role, ok = {}, True
        for cl in classes:
            ops = [rd[0] for rd in cl]
            key = {frozenset((a, "c")): "I", frozenset((b, "c")): "J",
                   frozenset((a, b)): "K"}.get(frozenset(ops))
            if len(set(ops)) != len(ops) or key is None or key in role:
                ok = False
                break
            role[key] = cl
        if not ok:
            continue
        ext = {k: (role[k][0][2] if k in role else 1) for k in "IJK"}
        ordv = {k: (role[k][0][1] if k in role else (0,) * len(pts)) for k in "IJK"}
        if len(set(zip(ordv["I"], ordv["J"], ordv["K"]))) == len(pts) == ext["I"] * ext["J"] * ext["K"]:
            roles = tuple((k, tuple(sorted((rd[0], rd[3], rd[4]) for rd in role[k])))
                          for k in "IJK" if k in role)
            return ext, roles, ordv
    return None


def batch_loops_of(case: Case):
    """The batch loops (docs/AFFINE_GEMM_MAPPING.md section 16): the loops of
    two or more iterations that all three operands read."""
    readers = {}
    for r in ("a", "b", "c"):
        for f in case.arrays[r].forms():
            for v in f.vars():
                readers.setdefault(v, set()).add(r)
    return [lp.var for lp in case.loops
            if lp.trip() >= 2 and readers.get(lp.var) == {"a", "b", "c"}]


def batched_gemm_on_domain(case: Case, max_points: int = 200000) -> Optional[bool]:
    """Whether the nest is a batched GEMM of batch-dependent views
    (ISSUE-007), by enumeration: its batch loops are the loops all three
    operands read; any operand may read them anywhere (C included, since
    2026-10-16); for every value of the batch loops the remaining points form
    a GEMM of strided views
    (_gemm_on_points), every batch iteration the same shape. The factors'
    subscripts may add batch terms anywhere. Returns the per-batch extents
    {I, J, K}; False when the nest is not such a GEMM; None when there is no
    batch loop or the domain is too large to enumerate."""
    import itertools
    batch = batch_loops_of(case)
    if not batch:
        return None
    # C may read the batch loops in its role dimensions too (no C purity
    # since 2026-10-16, docs/AFFINE_GEMM_MAPPING.md section 19.3): per batch
    # value its view must still be injective, which _gemm_on_points checks.
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    loops = [lp for lp in case.loops if lp.var in used]
    total = 1
    for lp in loops:
        total *= len(lp.values())
    if total == 0 or total > max_points:
        return None
    names = [lp.var for lp in loops]
    groups = {}
    for x in itertools.product(*[lp.values() for lp in loops]):
        env = dict(zip(names, x))
        groups.setdefault(tuple(env[v] for v in batch), []).append(env)
    shape = None
    for pts in groups.values():
        found = _gemm_on_points(case, pts)
        if found is None or not found[1]:
            return False        # not a GEMM per batch, or element-wise
        if shape is None:
            shape = found[:2]
        elif found[:2] != shape:
            return False
    return shape[0]       # the per-batch extents (truthy)


def _role_options(readers):
    """The roles a loop may play in a decomposition of docs/AFFINE_GEMM_MAPPING.md
    section 19.1, from the operands that read it (G1: an operand's view
    depends only on its own two roles and the batch coordinates, so a loop
    read by B is not I, one read by A is not J, one read by C is not K; an
    operand that does not read a role's loop has a stride-0 view along it).
    The canonical role -- the readers rule of 19.2 -- comes first."""
    canonical = {frozenset("ac"): "I", frozenset("bc"): "J", frozenset("ab"): "K",
                 frozenset("abc"): "P"}.get(frozenset(readers))
    options = [canonical] if canonical else []
    for role, excluded in (("P", None), ("I", "b"), ("J", "a"), ("K", "c")):
        if role not in options and (excluded is None or excluded not in readers):
            options.append(role)
    if not readers:
        options = ["P"]
    return options, canonical


def _check_decomposition(loops, forms, assignment):
    """One assignment loop -> role (P, I, J, K) on the box: the roles
    partition the loops, so (P, I, J, K) is a bijection onto a box iff each
    role is one dense coordinate of its own loops -- the nonzero restrictions
    of every subscript to the role's loops proportional to one primitive
    direction whose values on the role's box are consecutive integers taken
    once (Theorem 3, S2) --; C's element injective on the (I, J) box for fixed
    batch and K values (G2; batch terms are constants per batch).  Returns
    {"ext", "general", "stride0", "why"} with why None when valid, else the
    failing condition ("sq1" when two operands' restrictions to one role are
    not proportional, the operand-specific decomposition of SQ-1)."""
    import itertools
    by_role = {r: [lp for lp in loops if assignment.get(lp.var) == r] for r in "PIJK"}
    out = {"ext": {}, "general": False, "stride0": False, "why": None}
    ordinal = {}
    for role in "IJK":
        lps = by_role[role]
        if not lps:
            out["ext"][role] = 1
            ordinal[role] = lambda env: 0
            continue
        rest = [(r, [f.coef.get(lp.var, 0) for lp in lps]) for r in "abc" for f in forms[r]]
        nonzero = [(r, x) for r, x in rest if any(x)]
        for (ra, a), (rb, b) in itertools.combinations(nonzero, 2):
            if any(a[s] * b[t] != a[t] * b[s] for s in range(len(lps)) for t in range(len(lps))):
                out["why"] = ("sq1" if ra != rb else
                              "role %s is read through two different combinations of its loops" % role)
                return out
        u = _primitive(nonzero[0][1])
        values = sorted({sum(c * x for c, x in zip(u, pt))
                         for pt in itertools.product(*[lp.values() for lp in lps])})
        count = 1
        for lp in lps:
            count *= lp.trip()
        if len(values) != count or any(b2 - a2 != values[1] - values[0]
                                       for a2, b2 in zip(values, values[1:])):
            out["why"] = "role %s is not one dense coordinate of its loops" % role
            return out
        out["ext"][role] = len(values)
        step = values[1] - values[0] if len(values) > 1 else 1
        ordinal[role] = (lambda u, lps, lo, st: lambda env: (sum(c * env[lp.var] for c, lp in zip(u, lps)) - lo) // st)(u, lps, values[0], step)
    # G2 on the (I, J) box with the other loops at their first value (the
    # batch and K loops, and the loops that run once -- constants)
    fixed = {lp.var: lp.values()[0] for lp in loops if assignment.get(lp.var, "P") in "PK"}
    ij = by_role["I"] + by_role["J"]
    elements = set()
    for pt in itertools.product(*[lp.values() for lp in ij]):
        env = dict(fixed)
        env.update(zip([lp.var for lp in ij], pt))
        elements.add(tuple(f.eval(env) for f in forms["c"]))
    if len(elements) != out["ext"]["I"] * out["ext"]["J"]:
        out["why"] = "C's view is not injective on the (I, J) box"
        return out
    # the views: general (a dimension on both roles, or a role on several
    # dimensions), stride-0 (a role of extent >= 2 the operand does not read)
    for r, rl in (("a", "IK"), ("b", "KJ"), ("c", "IJ")):
        dims_of = {role: [] for role in rl}
        for d, f in enumerate(forms[r]):
            here = [role for role in rl if any(f.coef.get(lp.var, 0) for lp in by_role[role])]
            out["general"] = out["general"] or len(here) > 1
            for role in here:
                dims_of[role].append(d)
        for role in rl:
            if len(dims_of[role]) > 1:
                out["general"] = True
            if not dims_of[role] and out["ext"][role] >= 2:
                out["stride0"] = True
    return out


def generalized_gemm_on_domain(case: Case, max_points: int = 200000):
    """The generalized definition (docs/AFFINE_GEMM_MAPPING.md section 19), by
    enumeration on the box domain -- independent of the compiler.  A loop of
    two or more iterations is a batch loop (P), I, J or K; the canonical
    decomposition is the readers rule of 19.2 (all three: P, A and C: I,
    B and C: J, A and B: K); every other assignment allowed by G1
    (_role_options) is searched too.  Each is checked by _check_decomposition
    (every role one dense coordinate of its loops, C injective per batch).
    Returns
      {"verdict": "ACCEPT", "ext": {...}, "batched": bool, "general": bool,
       "two_unit": bool}: the canonical decomposition is valid;
      {"verdict": "SQ1", "why": ...}: the canonical decomposition fails because
          two operands read one role through different combinations of its
          loops (the operand-specific decomposition of SQ-1, open);
      {"verdict": "DEMOTION", "why": ..., "how": ...}: the canonical
          decomposition fails (a loop read by one operand, two coordinates in
          one role, a role that is not one dense coordinate, C not injective
          -- section 19.2) but another one is valid -- loops demoted to batch
          loops, or a one-reader loop as a role the other operand's view is
          constant along (stride 0) -- which the compiler does not construct;
      {"verdict": "NOT_GEMM", "why": ...}: no decomposition is valid (every
          loop read by all three operands: an element-wise product);
    or None when the domain is too large or empty.  A loop no access uses
    repeats the whole GEMM (a repetition loop: the caller applies the rule
    of repeat_in_reduction); it is not enumerated."""
    import itertools
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    loops = [lp for lp in case.loops if lp.var in used]
    total = 1
    for lp in loops:
        total *= len(lp.values())
    if total == 0 or total > max_points:
        return None
    forms = {r: case.arrays[r].forms() for r in ("a", "b", "c")}
    many = [lp for lp in loops if lp.trip() >= 2]
    if not many:
        # every loop runs once: the 1 x 1 x 1 GEMM (SQ-4), no batch loop
        return {"verdict": "ACCEPT", "ext": {"I": 1, "J": 1, "K": 1}, "batched": False,
                "general": False, "two_unit": False}
    readers = {lp.var: {r for r in forms if any(f.coef.get(lp.var, 0) for f in forms[r])} for lp in many}
    options = {lp.var: _role_options(readers[lp.var]) for lp in many}
    once = [lp for lp in loops if lp.trip() == 1]
    canonical = {lp.var: options[lp.var][1] for lp in many}
    canonical_defined = all(canonical.values())
    first_failure = None
    if canonical_defined:
        if all(r == "P" for r in canonical.values()):
            return {"verdict": "NOT_GEMM", "why": "every loop is read by all three operands (an "
                    "element-wise product)"}
        res = _check_decomposition(many + once, forms, canonical)
        if res["why"] is None:
            batched = any(r == "P" for r in canonical.values())
            ext = res["ext"]
            return {"verdict": "ACCEPT", "ext": ext, "batched": batched, "general": res["general"],
                    "two_unit": batched and sum(e == 1 for e in ext.values()) >= 2}
        if res["why"] == "sq1":
            return {"verdict": "SQ1", "why": "two operands read one role through different "
                    "combinations of its loops (an operand-specific decomposition, SQ-1)"}
        first_failure = res["why"]
    else:
        lonely = [lp.var for lp in many if len(readers[lp.var]) == 1]
        first_failure = "loop(s) %s read by one operand only (a broadcast)" % ", ".join(lonely)
    # the other decompositions, fewest batch loops first
    choices = [options[lp.var][0] for lp in many]
    for assignment in sorted(itertools.product(*choices),
                             key=lambda a: (sum(1 for r in a if r == "P"), a)):
        a = dict(zip([lp.var for lp in many], assignment))
        if a == canonical or all(r == "P" for r in assignment):
            continue
        res = _check_decomposition(many + once, forms, a)
        if res["why"] is None:
            demoted = [v for v in a if a[v] == "P" and canonical.get(v) != "P"]
            how = []
            if demoted:
                how.append("loop(s) %s as batch loops" % ", ".join(demoted))
            if res["stride0"]:
                how.append("a stride-0 view along a role the operand does not read")
            return {"verdict": "DEMOTION", "why": first_failure, "how": "; ".join(how),
                    "ext": res["ext"]}
    return {"verdict": "NOT_GEMM", "why": first_failure}


def repeat_in_reduction(case: Case) -> bool:
    """A repetition loop (no access uses it) of two or more iterations that
    cannot be moved outward without reordering the accumulations into one
    element of C: two iterations writing the same C element differ in a used
    loop enclosing it -- a reduction loop, or a batch or role loop whose C
    windows overlap (C[i + k][j]: docs/AFFINE_GEMM_MAPPING.md section 19.3).
    The rule of the dense GEMMs (README section 2, 'repeat_in_reduction'),
    decided by enumeration (_hoist_blocked), the same test the compiler's
    hoist makes (affine::isValidLoopInterchangePermutation)."""
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    return _hoist_blocked(case, case.loops, used) is not None


def spec_expect(case: Case) -> dict:
    an = case.analysis or analyse(case)
    tags = set(an.get("gemm_tags", []))
    out = {"recognition": None, "basis_key": None, "basis": None, "legal_gemm": bool(an.get("is_gemm"))}

    def done(rec, key):
        out["recognition"] = rec
        out["basis_key"] = key
        out["basis"] = SPEC_BASIS[key]
        out["offload"] = {"ACCEPT": "offloaded", "UNDETERMINED": "undetermined"}.get(rec, "stays_on_host")
        if rec == "UNDETERMINED":
            out["spec_question"] = key
        case.spec = out
        return out

    if case.kernel_style == "scalar_acc":
        return done("OUT_OF_SCOPE", "scalar_acc")
    if not an.get("is_gemm"):
        # legal_gemm: the nest is a GEMM, or a batched GEMM, of affine views
        # under the generalized definition (docs/AFFINE_GEMM_MAPPING.md
        # section 19) -- set below when one of the decisions finds one, also
        # when the specification then excludes it (double, aliasing, a
        # decomposition the compiler does not construct)
        # a matrix-vector product is the N = 1 GEMM (P0, 2026-10-15)
        if an["math_class"] == "matrix_vector":
            out["legal_gemm"] = True
            return done("ACCEPT", "matvec")
        # (The reduction index in the output with every J loop of one
        # iteration -- a batched GEMM of scaled vectors, SQ-5 -- is decided by
        # the batched path below with the guards every ACCEPT applies; with
        # the I loops running once too it is an element-wise product.)
        # A loop all three operands read is a batch loop (ISSUE-007): the
        # factors may read it through their own expressions or as an offset
        # of a role's subscript (A[4i - j][5k]), C alone. Decided per batch
        # iteration on the domain.
        batched_ext = (batched_gemm_on_domain(case)
                       if case.kernel_style not in ("init_inside", "scalar_acc")
                       and case.stmt in ("+=", "=C+AB", "=AB+C", "+=BA") else None)
        if batched_ext:
            out["legal_gemm"] = True
            two_unit = sum(1 for k in "IJK" if batched_ext[k] == 1) >= 2
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if not (case.a.restrict or case.c.restrict) or not (case.b.restrict or case.c.restrict):
                return done("OUT_OF_SCOPE", "aliasing")
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            # two of M, N, K equal to 1 per batch: SQ-5, decided 2026-10-16
            return done("ACCEPT", "sq5_decided" if two_unit else "batched_views")
        # The syntactic analysis and the iteration domain differ only through
        # loops that run once (constants on the domain, Lemma 0): decide such
        # a nest on its domain (P0.4, P0.5).
        if (any(lp.trip() == 1 for lp in case.loops)
                and case.kernel_style not in ("init_inside", "scalar_acc")
                and case.stmt in ("+=", "=C+AB", "=AB+C", "+=BA") and gemm_on_domain(case)):
            out["legal_gemm"] = True
            if gemm_on_domain(case) == "repeat_in_reduction":
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            # the rules that do not depend on the coordinates still apply
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if not (case.a.restrict or case.c.restrict) or not (case.b.restrict or case.c.restrict):
                return done("OUT_OF_SCOPE", "aliasing")
            return done("ACCEPT", "dense_on_domain")
        # The generalized definition (docs/AFFINE_GEMM_MAPPING.md section
        # 19): skewed and merged views, C reading batch loops in its role
        # dimensions; a nest whose canonical decomposition fails but a
        # batched one exists is a documented limitation (the compiler does
        # not demote loops to batch loops).
        # A sum, a third factor or an overwrite is not a GEMM of A and B in
        # any statement form; the other forms are decided like the MAC forms
        # and then excluded by their own rule (alpha, init_inside), so that a
        # scaled or re-initialised GEMM of affine views is 'a valid GEMM the
        # specification excludes', never 'not a GEMM'.
        if case.stmt in ("+=sum", "+=triple", "=AB"):
            return done("REJECT", "not_gemm")
        g = generalized_gemm_on_domain(case)
        if g and g["verdict"] in ("ACCEPT", "DEMOTION", "SQ1"):
            out["legal_gemm"] = True
        if g and g["verdict"] in ("ACCEPT", "DEMOTION", "SQ1") and case.stmt in ("+=alpha", "-="):
            return done("OUT_OF_SCOPE", "alpha")
        if g and g["verdict"] in ("ACCEPT", "DEMOTION", "SQ1") and case.kernel_style == "init_inside":
            return done("OUT_OF_SCOPE", "init_inside")
        if g and g["verdict"] == "ACCEPT":
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if not (case.a.restrict or case.c.restrict) or not (case.b.restrict or case.c.restrict):
                return done("OUT_OF_SCOPE", "aliasing")
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            return done("ACCEPT", "sq5_decided" if g["two_unit"] else "generalized_views")
        if g and g["verdict"] == "SQ1":
            return done("UNDETERMINED", "SQ-1")
        if g and g["verdict"] == "DEMOTION":
            return done("OUT_OF_SCOPE", "broadcast" if "broadcast" in (g.get("why") or "") else "demotion")
        return done("REJECT", "not_gemm")
    if any(lp.trip() == 0 for lp in case.loops):
        return done("UNDETERMINED", "SQ-2")
    if "batched" in tags:
        # a batched product whose I, J and K loops all run once is an
        # element-wise product, not a GEMM (docs/AFFINE_GEMM_MAPPING.md
        # section 19.1, non-degeneracy; SQ-5 decided only two unit extents)
        if all(lp.trip() == 1 for lp in case.loops if lp.role in "IJK"):
            out["legal_gemm"] = False
            return done("REJECT", "not_gemm")
        # a batched GEMM is inside the specification since 2026-10-15
        # (ISSUE-006) when every operand carries every batch coordinate; an
        # operand shared by all batches (no batch subscript) is a broadcast
        # over the batch, not lowered
        p_count = sum(1 for lp in case.loops if lp.role == "P")
        for r in ("a", "b"):
            if sum(1 for f in case.arrays[r].forms() if f.vars() and
                   f.vars() <= {lp.var for lp in case.loops if lp.role == "P"}) != p_count:
                return done("OUT_OF_SCOPE", "batched_shared")
    if "alpha_scaled" in tags or "alpha_minus_one" in tags:
        return done("OUT_OF_SCOPE", "alpha")
    if case.kernel_style == "init_inside":
        return done("OUT_OF_SCOPE", "init_inside")
    if case.elem != "float":
        return done("OUT_OF_SCOPE", "double")
    if len({a.elem for a in case.arrays.values()}) > 1:
        return done("OUT_OF_SCOPE", "mixed_types")
    if not (case.a.restrict or case.c.restrict) or not (case.b.restrict or case.c.restrict):
        return done("OUT_OF_SCOPE", "aliasing")
    # a repetition loop (of more than one iteration) whose hoist would reorder
    # the accumulations into one element of C (inside a reduction loop, or
    # inside a batch loop whose C windows overlap)
    if repeat_in_reduction(case):
        return done("OUT_OF_SCOPE", "repeat_in_reduction")
    # A coordinate that decreases along its loop -- a loop that counts down,
    # or a reversed subscript such as 7 - i -- is a dense GEMM like any other
    # since 2026-10-15 (ISSUE-004); since the P0 views (2026-10-15), two
    # operands that run through a shared coordinate in OPPOSITE directions
    # (A[7 - i] with C[i]) are too: one operand's view has a negative stride
    # (docs/AFFINE_GEMM_MAPPING.md section 5). Transposed operands likewise
    # (section 5: a permuted view).
    forms = {r: case.arrays[r].forms() for r in ("a", "b", "c")}
    v = an["views"]
    # two operands spelling a shared coordinate with different loop
    # combinations (not two views of one strided coordinate): open question
    for ra, rb, g in (("a", "c", "I"), ("b", "c", "J"), ("a", "b", "K")):
        fa_, fb_ = forms[ra][v[ra][g]["position"]], forms[rb][v[rb][g]["position"]]
        if shared_coordinate(fa_, fb_) is None:
            return done("UNDETERMINED", "SQ-1")
    if case.a.name == case.b.name:
        # one array as both operands: the lowering serves both through ONE
        # read-only view, so A's (I, K) view must be B's (K, J) view
        v = an["views"]
        ext = an["extents"]

        def key(role, g):
            return (v[role][g]["first"], v[role][g]["last"], ext[g])

        if key("a", "I") != key("b", "K") or key("a", "K") != key("b", "J"):
            return done("OUT_OF_SCOPE", "same_memref_views")
    return done("ACCEPT", "batched" if "batched" in tags else "dense_gemm")


def classify_case(case: Case) -> str:
    spec = case.spec or spec_expect(case)
    cur = case.expect or predict(case)
    offloaded = cur["driver"] == "offloaded"
    if spec["recognition"] == "ACCEPT":
        c = "TRUE_POSITIVE" if offloaded else "FALSE_NEGATIVE"
    elif spec["recognition"] == "REJECT":
        c = "FALSE_POSITIVE" if offloaded else "TRUE_NEGATIVE"
    elif spec["recognition"] == "UNDETERMINED":
        # the specification does not decide; the implementation's behaviour
        # is recorded, not judged (verdict 'spec_open' when it is as predicted)
        c = "SPEC_OPEN"
    else:
        c = "UNEXPECTED_ACCEPT" if offloaded else "DOCUMENTED_LIMITATION"
    case.classification = c
    return c


# --------------------------------------------------------------------------
# C emission
# --------------------------------------------------------------------------


def _sub_text(arr: Array):
    return "".join("[%s]" % s for s in arr.subs)


def _flat_text(arr: Array, fname: str):
    """Af[(s0) * d1 * d2 + (s1) * d2 + (s2)]: the row-major offset, explicit."""
    dims = arr.dims
    terms = []
    for pos, s in enumerate(arr.subs):
        mult = volume(dims[pos + 1:])
        terms.append("(%s)" % s if mult == 1 else "(%s) * %d" % (s, mult))
    return "%s[%s]" % (fname, " + ".join(terms))


def _dims_text(dims):
    return "".join("[%d]" % d for d in dims)


def _param_text(case: Case, arr: Array):
    if case.kernel_style == "ptr_param":
        return "%s (*%s%s)%s" % (arr.elem, "restrict " if arr.restrict else "", arr.name,
                                 _dims_text(arr.dims[1:]))
    return "%s %s[%s%d]%s" % (arr.elem, arr.name, "restrict " if arr.restrict else "",
                              arr.dims[0], _dims_text(arr.dims[1:]))


def _statement(case: Case, a, b, c, d=None):
    prod = "%s * %s" % (a, b)
    return {"+=": "%s += %s;" % (c, prod),
            "=C+AB": "%s = %s + %s;" % (c, c, prod),
            "=AB+C": "%s = %s + %s;" % (c, prod, c),
            "+=BA": "%s += %s * %s;" % (c, b, a),
            "-=": "%s -= %s;" % (c, prod),
            "=AB": "%s = %s;" % (c, prod),
            "+=alpha": "%s += alpha * %s * %s;" % (c, a, b),
            "+=sum": "%s += %s + %s;" % (c, a, b),
            "+=triple": "%s += %s * %s * %s;" % (c, a, b, d)}[case.stmt]


def kernel_text(case: Case) -> str:
    """The function under test, in the affine C subset (or deliberately
    outside it, for some negative cases)."""
    params = [_param_text(case, case.arrays[case.role_of_name(n)]) for n in case.array_names()]
    if case.stmt == "+=alpha":
        params.append("%s alpha" % case.elem)
    A, B, C = case.a, case.b, case.c
    a, b, c = A.name + _sub_text(A), B.name + _sub_text(B), C.name + _sub_text(C)
    d = case.arrays["d"].name + _sub_text(case.arrays["d"]) if "d" in case.arrays else None
    st = _statement(case, a, b, c, d)
    suffix = "f" if case.elem == "float" else ""
    lines = ["void %s(%s) {" % (case.kernel, ", ".join(params))]
    n = len(case.loops)
    if case.kernel_style == "braces":
        for dd, lp in enumerate(case.loops):
            lines.append("  " * (dd + 1) + lp.c_header() + " {")
        lines.append("  " * (n + 1) + st)
        for dd in range(n - 1, -1, -1):
            lines.append("  " * (dd + 1) + "}")
    elif case.kernel_style == "init_inside":
        for dd, lp in enumerate(case.loops[:-1]):
            lines.append("  " * (dd + 1) + lp.c_header() + (" {" if dd == n - 2 else ""))
        lines.append("  " * n + "%s = 0.0%s;" % (c, suffix))
        lines.append("  " * n + case.loops[-1].c_header())
        lines.append("  " * (n + 1) + st)
        lines.append("  " * (n - 1) + "}")
    elif case.kernel_style == "scalar_acc":
        for dd, lp in enumerate(case.loops[:-1]):
            lines.append("  " * (dd + 1) + lp.c_header() + (" {" if dd == n - 2 else ""))
        lines.append("  " * n + "%s acc = 0.0%s;" % (case.elem, suffix))
        lines.append("  " * n + case.loops[-1].c_header())
        lines.append("  " * (n + 1) + "acc += %s * %s;" % (a, b))
        lines.append("  " * n + "%s = acc;" % c)
        lines.append("  " * (n - 1) + "}")
    else:
        for dd, lp in enumerate(case.loops):
            lines.append("  " * (dd + 1) + lp.c_header())
        lines.append("  " * (n + 1) + st)
    lines.append("}")
    return "\n".join(lines)


def _ref_params(case: Case, with_alpha=True):
    E = case.elem
    params = []
    for n in case.array_names():
        et = case.arrays[case.role_of_name(n)].elem
        params.append("%s%s *%sf" % ("" if n == case.c.name else "const ", et, n))
    if case.stmt == "+=alpha" and with_alpha:
        params.append("%s alpha" % E)
    return ", ".join(params)


def reference_direct_text(case: Case) -> str:
    """Reference 1: the statement evaluated over the same loop domain on flat
    arrays with explicit row-major offsets; the product through a call."""
    A, B, C = case.a, case.b, case.c
    a, b, c = _flat_text(A, A.name + "f"), _flat_text(B, B.name + "f"), _flat_text(C, C.name + "f")
    if case.stmt in ("+=", "=C+AB", "=AB+C"):
        st = "%s = %s + mul(%s, %s);" % (c, c, a, b)
    elif case.stmt == "+=BA":
        st = "%s = %s + mul(%s, %s);" % (c, c, b, a)
    elif case.stmt == "-=":
        st = "%s = %s - mul(%s, %s);" % (c, c, a, b)
    elif case.stmt == "=AB":
        st = "%s = mul(%s, %s);" % (c, a, b)
    elif case.stmt == "+=alpha":
        st = "%s = %s + mul(mul(alpha, %s), %s);" % (c, c, a, b)
    elif case.stmt == "+=sum":
        st = "%s = %s + (%s + %s);" % (c, c, a, b)
    elif case.stmt == "+=triple":
        D = case.arrays["d"]
        st = "%s = %s + mul(mul(%s, %s), %s);" % (c, c, a, b, _flat_text(D, D.name + "f"))
    else:
        raise ValueError(case.stmt)
    lines = ["static void reference_direct(%s) {" % _ref_params(case)]
    n = len(case.loops)
    zero = case.kernel_style in ("init_inside", "scalar_acc")
    for dd, lp in enumerate(case.loops):
        hdr = lp.reference_header()
        if zero and dd == n - 2:
            lines.append("  " * (dd + 1) + hdr + " {")
            lines.append("  " * (dd + 2) + "%s = 0;   /* the kernel zeroes C here */" % c)
        else:
            lines.append("  " * (dd + 1) + hdr)
    lines.append("  " * (n + 1) + st)
    if zero:
        lines.append("  " * (n - 1) + "}")
    lines.append("}")
    return "\n".join(lines)


def reference_logical_text(case: Case) -> Optional[str]:
    """Reference 2, for nests analyse() found to be (batched) GEMMs: gather the
    logical operands A'(I,K), B'(K,J) from the arrays through each operand's
    own view, run a plain GEMM on them, scatter into C' -- once per batch
    value, `repetitions` times.  None when the nest is not a GEMM."""
    an = case.analysis
    if not an.get("is_gemm"):
        return None
    groups = {}
    for lp in case.loops:
        groups.setdefault(lp.role, []).append(lp)
    ext = an["extents"]
    M, N, K = ext["I"], ext["J"], ext["K"]
    if min(M, N, K) == 0:
        # an empty GEMM: there is no logical operand to gather (a zero-size
        # array is not standard C); reference 1 and the oracle still apply
        return None
    A, B, C = case.a, case.b, case.c
    pl = groups.get("P", [])
    E = case.elem
    zero = case.kernel_style in ("init_inside", "scalar_acc")

    def ordinal_expr(g):
        ls = groups[g]
        terms = []
        mult = 1
        for lp in reversed(ls):
            terms.append("((%s - %d) / %d) * %d" % (lp.var, lp.lb, lp.step, mult)
                         if (lp.lb or lp.step != 1 or mult != 1) else lp.var)
            mult *= lp.trip()
        return " + ".join(reversed(terms))

    def loops_text(gs, depth):
        out = []
        dd = depth
        for g in gs:
            for lp in groups.get(g, []):
                out.append("  " * dd + lp.reference_header())
                dd += 1
        return out, dd

    lines = ["static void reference_logical(%s) {" % _ref_params(case)]
    lines.append("  static %s Av[%d][%d], Bv[%d][%d], Cv[%d][%d];" % (E, M, K, K, N, M, N))
    if pl:
        lines.append("  /* one logical GEMM per value of the batch coordinate(s) %s */" %
                     ", ".join(lp.var for lp in pl))
    lt, dd = loops_text(["P"], 1)
    lines += lt
    body = dd
    ind = "  " * body
    ind2 = "  " * (body + 1)
    lines.append(ind + "{")
    lines.append(ind2 + "/* A'(I,K) = %s%s */" % (A.name, _sub_text(A)))
    lt, d2 = loops_text(["I", "K"], body + 1)
    lines += lt
    lines.append("  " * d2 + "Av[%s][%s] = %s;" % (ordinal_expr("I"), ordinal_expr("K"), _flat_text(A, A.name + "f")))
    lines.append(ind2 + "/* B'(K,J) = %s%s */" % (B.name, _sub_text(B)))
    lt, d2 = loops_text(["K", "J"], body + 1)
    lines += lt
    lines.append("  " * d2 + "Bv[%s][%s] = %s;" % (ordinal_expr("K"), ordinal_expr("J"), _flat_text(B, B.name + "f")))
    lines.append(ind2 + "/* C'(I,J) = %s%s%s */" % (C.name, _sub_text(C), " -- starts at zero (beta = 0)" if zero else ""))
    lt, d2 = loops_text(["I", "J"], body + 1)
    lines += lt
    lines.append("  " * d2 + "Cv[%s][%s] = %s;" % (ordinal_expr("I"), ordinal_expr("J"),
                                                  "0" if zero else _flat_text(C, C.name + "f")))
    reps = an.get("repetitions", 1)
    lines.append(ind2 + "/* the GEMM C' += A' x B' (M=%d, N=%d, K=%d)%s */" % (
        M, N, K, ", repeated %d times" % reps if reps > 1 else ""))
    lines.append(ind2 + "for (long rep = 0; rep < %d; ++rep)" % reps)
    lines.append(ind2 + "  for (long I = 0; I < %d; ++I)" % M)
    lines.append(ind2 + "    for (long J = 0; J < %d; ++J)" % N)
    lines.append(ind2 + "      for (long Kx = 0; Kx < %d; ++Kx)" % K)
    if case.stmt == "-=":
        lines.append(ind2 + "        Cv[I][J] = Cv[I][J] - mul(Av[I][Kx], Bv[Kx][J]);")
    elif case.stmt == "+=alpha":
        lines.append(ind2 + "        Cv[I][J] = Cv[I][J] + mul(alpha, mul(Av[I][Kx], Bv[Kx][J]));")
    else:
        lines.append(ind2 + "        Cv[I][J] = Cv[I][J] + mul(Av[I][Kx], Bv[Kx][J]);")
    lines.append(ind2 + "/* scatter C' back through C's own view */")
    lt, d2 = loops_text(["I", "J"], body + 1)
    lines += lt
    lines.append("  " * d2 + "%s = Cv[%s][%s];" % (_flat_text(C, C.name + "f"), ordinal_expr("I"), ordinal_expr("J")))
    lines.append(ind + "}")
    lines.append("}")
    return "\n".join(lines)


def _span(view):
    return "%d..%d" % (view["first"], view["last"])


def _wrap(text, width):
    """Greedy word wrap (no textwrap: keeps the output byte-stable across
    Python versions)."""
    lines, cur = [], ""
    for w in text.split():
        if cur and len(cur) + 1 + len(w) > width:
            lines.append(cur)
            cur = w
        else:
            cur = (cur + " " + w) if cur else w
    if cur:
        lines.append(cur)
    return lines


def math_description(case: Case) -> List[str]:
    """Header lines: the statement over its loop domain, and the logical view."""
    an = case.analysis
    A, B, C = case.a, case.b, case.c
    a, b, c = A.name + _sub_text(A), B.name + _sub_text(B), C.name + _sub_text(C)
    d = case.arrays["d"].name + _sub_text(case.arrays["d"]) if "d" in case.arrays else None
    out = []
    def dom_text(lp):
        if lp.trip() == 0:
            return "%s in {} (the loop runs zero times)" % lp.var
        if lp.step == 1:
            return "%s in [%d, %d)" % (lp.var, lp.lb, lp.exclusive_ub())
        if lp.trip() == 1:
            return "%s in {%d}" % (lp.var, lp.lb)
        return "%s in {%d, %d, ..., %d}" % (lp.var, lp.lb, lp.lb + lp.step, lp.values()[-1])

    dom = ", ".join(dom_text(lp) for lp in case.loops)
    out.append("for " + dom + " (loops outer to inner):")
    if case.kernel_style in ("init_inside", "scalar_acc"):
        out.append("  %s = 0 before the innermost loop, then" % c)
    out.append("  " + _statement(case, a, b, c, d).rstrip(";"))
    if an.get("is_gemm"):
        ext = an["extents"]
        v = an["views"]
        out.append("")
        out.append("Logical GEMM: C'(I,J) += A'(I,K) * B'(K,J)   M=%d N=%d K=%d%s%s" % (
            ext["I"], ext["J"], ext["K"],
            ", one per value of batch coordinate %s" % ", ".join(lp.var for lp in case.loops if lp.role == "P")
            if an["math_class"] == "gemm_batched" else "",
            ", repeated %d times" % an["repetitions"] if an.get("repetitions", 1) > 1 else ""))
        groups = {}
        for lp in case.loops:
            groups.setdefault(lp.role, []).append(lp)
        for g in ("I", "J", "K"):
            out.append("  %s is enumerated by loop(s) %s: extent %d" % (
                g, ", ".join("%s (%d values)" % (lp.var, lp.trip()) for lp in groups[g]), ext[g]))
        for role, nm, g0, g1 in (("a", "A'(I,K)", "I", "K"), ("b", "B'(K,J)", "K", "J"), ("c", "C'(I,J)", "I", "J")):
            arr = case.arrays[role]
            vv = v[role]
            txt = arr.name + _sub_text(arr)
            if vv[g0]["first"] is None or vv[g1]["first"] is None:
                out.append("  %s = %s   (an empty coordinate: no element is touched)" % (nm, txt))
                continue
            if vv[g0]["position"] < vv[g1]["position"]:
                out.append("  %s = %s   (%s index %s..%s via '%s'; %s index %s..%s via '%s')" % (
                    nm, txt, g0, vv[g0]["first"], vv[g0]["last"], vv[g0]["subscript"],
                    g1, vv[g1]["first"], vv[g1]["last"], vv[g1]["subscript"]))
            else:
                out.append("  %s = %s   (TRANSPOSED: %s is the trailing subscript '%s' %s..%s, %s the leading '%s' %s..%s)" % (
                    nm, txt, g0, vv[g0]["subscript"], vv[g0]["first"], vv[g0]["last"],
                    g1, vv[g1]["subscript"], vv[g1]["first"], vv[g1]["last"]))
        if an["gemm_tags"]:
            out.append("  tags: " + ", ".join(an["gemm_tags"]))
    elif an.get("not_dense_reasons"):
        out.append("")
        out.append("Not a dense GEMM of these operands because:")
        for r in an["not_dense_reasons"]:
            out.append("  - " + r)
    return out


def c_file_text(case: Case, compiler_commit: str = "6e25528") -> str:
    an = case.analysis or analyse(case)
    ex = case.expect or predict(case)
    data = evaluate(case)
    cs = checksum(data["c"])
    cs0 = checksum(data["c0"])
    A, B, C = case.a, case.b, case.c
    E = case.elem
    suffix = "f" if E == "float" else ""
    rl = reference_logical_text(case)
    cz = "[0]" * len(C.dims)

    # ---------------------------------------------------------------- header
    h = []
    h.append("Case ID: %s" % case.id)
    h.append("Name: %s" % case.name)
    h.append("Set / category: %s / %s" % (case.set, case.category))
    h.append("Mathematical class: %s" % an["math_class"])
    h.append("C program valid: yes (every subscript in bounds, no undefined behaviour)")
    h.append("")
    h.append("Mathematical operation:")
    for ln in math_description(case):
        h.append(("  " + ln) if ln else "")
    if case.math_note:
        h.append("")
        for ln in case.math_note.strip("\n").split("\n"):
            h.append("  " + ln)
    h.append("")
    decl = []
    for n in case.array_names():
        arr = case.arrays[case.role_of_name(n)]
        decl.append("%s: %s%s" % (n, arr.elem, _dims_text(arr.dims)))
    h.append("Array dimensions: " + "   ".join(decl))
    h.append("Loop order (outer to inner): " + ", ".join(
        "%s [%s]" % (lp.var, ROLE_NAMES[lp.role]) for lp in case.loops))
    h.append("Index formulas: A: %s%s   B: %s%s   C: %s%s" % (
        A.name, _sub_text(A), B.name, _sub_text(B), C.name, _sub_text(C)))
    h.append("Statement form: " + STMT_TEXT[case.stmt])
    if case.kernel_style != "plain":
        h.append("Kernel style: " + {"init_inside": "C[..] = 0 before the reduction loop (imperfect nest)",
                                     "scalar_acc": "scalar accumulator (a local variable)",
                                     "ptr_param": "pointer-to-array parameters",
                                     "braces": "every loop body in braces"}[case.kernel_style])
    h.append("Element type: %s" % E)
    h.append("")
    spec = case.spec or spec_expect(case)
    cls = case.classification or classify_case(case)
    h.append("Mathematical GEMM: %s" % ("YES" if spec.get("legal_gemm") else "NO"))
    h.append("Expected recognition (specification): %s" % spec["recognition"])
    h.append("  basis: %s" % spec["basis"])
    if case.redecided:
        h.append("  re-decided: %s" % case.redecided)
    commit_text = compiler_commit
    if case.issue and cls != "FALSE_NEGATIVE":
        commit_text = "%s with the %s of %s" % (compiler_commit,
                                                 "changes" if case.redecided else "fix", case.issue)
    h.append("Current implementation (commit %s, predicted from its sources; measured by" % commit_text)
    h.append("scripts/run_showcase.py, never assumed):")
    h.append("  front end (systolic-cfront):             %s" % ex["frontend"])
    h.append("  recognition (recognizeLogicalMatmul):    %s%s" % (
        ex["recognition"].upper(),
        " (after the lowering pass hoists the repetition loop outward)" if ex["hoisting_required"] else ""))
    h.append("  dense partition (getDenseLogicalGemm):   %s" % ex["dense_partition"])
    h.append("  lowering (--affine-matmul-to-systolic):  %s" % ex["lowering"])
    h.append("  driver (systolic-cc):                    %s" % ex["driver"])
    if ex["reject_stage"]:
        h.append("  rejection at: %s" % ex["reject_stage"])
        h.append("  reason: %s" % ex["reject_reason"])
    if case.issue and cls != "FALSE_NEGATIVE" and case.redecided:
        # a specification change moved the expectation; the case was no
        # false negative under the specification of its time
        issue_note = " -- see failures/issues/%s (a specification change)" % case.issue
    elif case.issue and cls != "FALSE_NEGATIVE":
        issue_note = " -- was a FALSE NEGATIVE of failures/issues/%s (fixed)" % case.issue
    elif case.issue:
        issue_note = " -- see failures/issues/%s" % case.issue
    else:
        issue_note = ""
    h.append("Classification: %s%s" % (cls.replace("_", " "), issue_note))
    if cls == "FALSE_NEGATIVE":
        h.append("  This nest IS a GEMM and the specification requires it to be recognized;")
        h.append("  the current recognizer refuses it.  The expectation is not lowered to match")
        h.append("  the implementation: the test fails until the recognizer is fixed.")
        h.append("  Lowering support: to be verified once recognized.")
        h.append("  FPGA execution support: to be verified once lowered.")
    elif cls == "DOCUMENTED_LIMITATION":
        h.append("  A mathematically valid GEMM that the specification excludes explicitly;")
        h.append("  'refused' must never be read as 'not a GEMM'.")
    elif cls == "TRUE_NEGATIVE":
        h.append("  Not a dense GEMM of these operands; the compiler must keep refusing it.")
    elif cls == "SPEC_OPEN":
        h.append("  The specification does not decide this case (%s, README.md section 2):" % spec["spec_question"])
        for ln in _wrap(SPEC_QUESTIONS[spec["spec_question"]], 74):
            h.append("    " + ln)
        h.append("  The compiler's behaviour is recorded, not judged, until the question is")
        h.append("  settled in writing; the program's own result check must pass either way.")
    h.append("")
    refs = ["(1) direct evaluation of the statement over the loop domain on flat arrays",
            "    with explicit row-major offsets, the product through a function call;"]
    if rl:
        refs += ["(2) the logical GEMM view: gather A', B' through each operand's own index",
                 "    map, plain GEMM, scatter into C' (must agree with (1));"]
    refs += ["(%d) a checksum of the expected result computed independently in Python" % (3 if rl else 2),
             "    (generator/casespec.py) at generation time: EXPECTED_CHECKSUM below."]
    h.append("Independent CPU reference: yes --")
    h += ["  " + r for r in refs]
    h.append("The references never call the kernel or any compiler-generated code and share")
    h.append("no index code with it.  Data: small integers in [-3, 3], C starts non-zero;")
    h.append("every partial sum is an integer far below 2^24, so the float results are")
    h.append("exact and compared for exact equality -- no tolerance.")
    h.append("")
    h.append("How to run:")
    h.append("  CPU only (the kernel as written vs the references):")
    h.append("    cc -std=c11 -O1 %s.c -o %s && ./%s" % (case.id, case.id, case.id))
    h.append("  Through the compiler (software runtime model, NOT the FPGA):")
    h.append("    build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS \\")
    h.append("      %s.c -o %s && ./%s" % (case.id, case.id, case.id))
    h.append("  On the board: the same with --runtime=board and SYSTOLIC_FPGA_CONFIG set.")
    h.append("  Stage by stage, results collected: scripts/run_showcase.py --case %s" % case.id)
    h.append("Exit status: 0 = PASS (the kernel equals every reference), 1 = FAIL, 3 = the")
    h.append("references disagree among themselves (a bug in this test, not in the compiler).")
    if case.notes:
        h.append("")
        for ln in case.notes.strip("\n").split("\n"):
            h.append(ln)

    out = ["/*"]
    out += [(" * " + ln).rstrip() for ln in h]
    out.append(" */")
    out.append("")
    out.append("#include <stdio.h>")
    out.append("#include <string.h>")
    out.append("")
    out.append("#ifdef GEMM_RECOGNITION_STATS")
    out.append("#include \"systolic_offload.h\"   /* invocation counts (available when systolic-cc links the runtime) */")
    out.append("#endif")
    out.append("")
    out.append("/* ------------------------------------------------------------------ kernel")
    out.append(" * The loop nest under test.  It is the only function the C front end is")
    out.append(" * meant to translate; everything below it is host code (int return values,")
    out.append(" * pointers, calls) and stays on the CPU. */")
    out.append("")
    out.append(kernel_text(case))
    out.append("")
    out.append("/* -------------------------------------------------------------------- data */")
    out.append("")
    for n in case.array_names():
        arr = case.arrays[case.role_of_name(n)]
        out.append("static %s %s%s;" % (arr.elem, n, _dims_text(arr.dims)))
    out.append("static %s C_ref%s;   /* reference 1 */" % (E, _dims_text(C.dims)))
    if rl:
        out.append("static %s C_log%s;   /* reference 2 */" % (E, _dims_text(C.dims)))
    out.append("")
    out.append("/* small integers in [-3, 3]; the same formula as generator/casespec.py */")
    out.append("static %s elem(long e, long seed) { return (%s)((e * 37 + seed * 101) %% 103 %% 7 - 3); }" % (E, E))
    if case.stmt != "+=sum":
        out.append("/* the product through a call: the references are not GEMM statements */")
        out.append("static %s mul(%s x, %s y) { return x * y; }" % (E, E, E))
    out.append("")
    out.append("static void init(void) {")
    for n in case.array_names():
        arr = case.arrays[case.role_of_name(n)]
        out.append("  for (long e = 0; e < %d; ++e) (&%s%s)[e] = elem(e, %d);" % (
            volume(arr.dims), n, "[0]" * len(arr.dims), array_seed(case, n)))
    out.append("}")
    out.append("")
    out.append("/* checksum: sum of C[e] * ((e % 97) + 1) over the flat array (exact in double) */")
    out.append("static double checksum(const %s *p, long n) {" % E)
    out.append("  double s = 0;")
    out.append("  for (long e = 0; e < n; ++e) s += (double)p[e] * (double)((e % 97) + 1);")
    out.append("  return s;")
    out.append("}")
    out.append("#define EXPECTED_CHECKSUM %d.0   /* final C, from the Python oracle */" % cs)
    out.append("#define INITIAL_CHECKSUM  %d.0   /* C before the kernel runs */" % cs0)
    out.append("")
    out.append("/* ------------------------------------------------- reference 1: direct")
    out.append(" * The statement over the same loop domain, on flat arrays with explicit")
    out.append(" * row-major offsets.  It shares no index code with the kernel. */")
    out.append("")
    out.append(reference_direct_text(case))
    if rl:
        out.append("")
        out.append("/* ------------------------------------------------ reference 2: logical view")
        out.append(" * The nest as the GEMM it is: the logical operands gathered through each")
        out.append(" * operand's own index map, a plain GEMM on them, the result scattered back")
        out.append(" * through C's map.  Ordinals: each coordinate's loops, outer to inner,")
        out.append(" * numbered densely from 0. */")
        out.append("")
        out.append(rl)
    out.append("")
    out.append("/* -------------------------------------------------------------------- main */")
    out.append("")
    out.append("static long first_diff(const %s *x, const %s *y, long n) {" % (E, E))
    out.append("  for (long e = 0; e < n; ++e)")
    out.append("    if (x[e] != y[e]) return e;")
    out.append("  return -1;")
    out.append("}")
    out.append("")
    out.append("int main(void) {")
    out.append("  const long n = %d;" % volume(C.dims))
    out.append("  init();")
    out.append("  memcpy(C_ref, %s, sizeof C_ref);" % C.name)
    if rl:
        out.append("  memcpy(C_log, %s, sizeof C_log);" % C.name)
    out.append("  printf(\"[gemm_recognition] %s %s: %s / %s, class %s\\n\");" % (
        case.id, case.name, case.set, case.category, an["math_class"]))
    out.append("  if (checksum(&%s%s, n) != INITIAL_CHECKSUM) {" % (C.name, cz))
    out.append("    printf(\"[gemm_recognition] REFERENCE INCONSISTENT: initial data differs from the Python oracle\\n\");")
    out.append("    printf(\"RESULT %s FAIL (test bug)\\n\"); return 3;" % case.id)
    out.append("  }")

    def refargs(cbuf):
        args = []
        for n in case.array_names():
            arr = case.arrays[case.role_of_name(n)]
            args.append("&%s%s" % (cbuf if n == C.name else n, "[0]" * len(arr.dims)))
        if case.stmt == "+=alpha":
            args.append("%r%s" % (case.alpha, suffix))
        return ", ".join(args)

    out.append("  reference_direct(%s);" % refargs("C_ref"))
    if rl:
        out.append("  reference_logical(%s);" % refargs("C_log"))
        out.append("  if (first_diff(&C_ref%s, &C_log%s, n) >= 0) {" % (cz, cz))
        out.append("    printf(\"[gemm_recognition] REFERENCE INCONSISTENT: the direct and logical-view references differ\\n\");")
        out.append("    printf(\"RESULT %s FAIL (test bug)\\n\"); return 3;" % case.id)
        out.append("  }")
        out.append("  printf(\"[gemm_recognition] references: direct and logical-view agree (%ld elements)\\n\", n);")
    out.append("  if (checksum(&C_ref%s, n) != EXPECTED_CHECKSUM) {" % cz)
    out.append("    printf(\"[gemm_recognition] REFERENCE INCONSISTENT: C reference checksum %.0f, Python oracle %.0f\\n\",")
    out.append("           checksum(&C_ref%s, n), EXPECTED_CHECKSUM);" % cz)
    out.append("    printf(\"RESULT %s FAIL (test bug)\\n\"); return 3;" % case.id)
    out.append("  }")
    out.append("  printf(\"[gemm_recognition] references: checksum %.0f matches the Python oracle\\n\", EXPECTED_CHECKSUM);")
    out.append("")
    out.append("#ifdef GEMM_RECOGNITION_STATS")
    out.append("  systolic_offload_stats_t before, after;")
    out.append("  systolic_offload_stats(&before);")
    out.append("#endif")
    callargs = ", ".join(case.array_names())
    if case.stmt == "+=alpha":
        callargs += ", %r%s" % (case.alpha, suffix)
    out.append("  %s(%s);   /* the kernel under test */" % (case.kernel, callargs))
    out.append("#ifdef GEMM_RECOGNITION_STATS")
    out.append("  systolic_offload_stats(&after);")
    out.append("  printf(\"[gemm_recognition] invocations: %llu (backend %s, hardware=%d, failed=%llu)\\n\",")
    out.append("         (after.per_device[0] + after.per_device[1] + after.per_device[2] + after.per_device[3]) -")
    out.append("         (before.per_device[0] + before.per_device[1] + before.per_device[2] + before.per_device[3]),")
    out.append("         after.backend, after.is_hardware, after.failed - before.failed);")
    out.append("#endif")
    out.append("")
    out.append("  long d = first_diff(&%s%s, &C_ref%s, n);" % (C.name, cz, cz))
    out.append("  double got = checksum(&%s%s, n);" % (C.name, cz))
    out.append("  if (d < 0 && got == EXPECTED_CHECKSUM) {")
    out.append("    printf(\"[gemm_recognition] kernel: equal to the reference (%ld elements, exact), checksum %.0f\\n\", n, got);")
    out.append("    printf(\"RESULT %s PASS\\n\");" % case.id)
    out.append("    return 0;")
    out.append("  }")
    out.append("  if (d >= 0)")
    out.append("    printf(\"[gemm_recognition] kernel: first mismatch at flat element %ld: got %g, reference %g\\n\",")
    out.append("           d, (double)(&%s%s)[d], (double)(&C_ref%s)[d]);" % (C.name, cz, cz))
    out.append("  printf(\"[gemm_recognition] kernel: checksum %.0f, expected %.0f\\n\", got, EXPECTED_CHECKSUM);")
    out.append("  printf(\"RESULT %s FAIL\\n\");" % case.id)
    out.append("  return 1;")
    out.append("}")
    out.append("")
    return "\n".join(out)


# --------------------------------------------------------------------------
# MLIR-level cases: nests the C front end cannot express (its subset has
# static, counter-independent bounds and in-bounds programs are the only
# valid ones), written directly in the affine dialect.  They exercise the
# recognizer, the dense partition and the lowering pass only -- nothing is
# compiled by systolic-cc or executed, and no verdict claims otherwise.
# --------------------------------------------------------------------------


@dataclass
class MlirCase:
    id: str
    name: str
    set: str                 # 'positive' | 'negative' | 'extended'
    category: str
    kernel: str              # the func.func symbol
    mlir: str                # the module text (func.func ... with the nest)
    math_note: str           # the mathematics and the author's reasoning
    is_gemm: bool            # mathematically a (dense) GEMM?
    math_class: str
    spec_recognition: str    # ACCEPT | REJECT | OUT_OF_SCOPE | UNDETERMINED
    spec_basis_key: str      # a key of SPEC_BASIS
    # the current implementation, predicted from the sources: what
    # --test-affine-matmul-discovery and --affine-matmul-to-systolic do
    cur_recognition: str     # accept | reject
    cur_dense: str           # supported | unsupported | not_reached
    cur_lowering: str        # supported | unsupported | not_reached
    cur_reject_stage: Optional[str] = None   # recognition | dense_partition | lowering | None
    cur_reject_reason: str = ""              # the compiler's wording (a substring is checked)
    notes: str = ""
    issue: str = ""
    expected_views: Dict[str, str] = field(default_factory=dict)   # e.g. {"C": "[1*I + 8, 1*J + 16]"}

    @property
    def classification(self):
        offloaded = self.cur_lowering == "supported"
        if self.spec_recognition == "ACCEPT":
            return "TRUE_POSITIVE" if offloaded else "FALSE_NEGATIVE"
        if self.spec_recognition == "REJECT":
            return "FALSE_POSITIVE" if offloaded else "TRUE_NEGATIVE"
        if self.spec_recognition == "UNDETERMINED":
            return "SPEC_OPEN"
        return "UNEXPECTED_ACCEPT" if offloaded else "DOCUMENTED_LIMITATION"


def mlir_file_text(case: MlirCase) -> str:
    h = []
    h.append("Case ID: %s" % case.id)
    h.append("Name: %s" % case.name)
    h.append("Set / category: %s / %s" % (case.set, case.category))
    h.append("Kind: MLIR-level case (the affine nest as the C front end would emit it, or as")
    h.append("  the front end cannot express it); stages: recognition, dense partition, lowering.")
    h.append("  No C program, nothing is executed: the verdict covers those stages only.")
    h.append("Mathematical class: %s" % case.math_class)
    h.append("")
    h.append("Mathematical operation:")
    for ln in case.math_note.strip("\n").split("\n"):
        h.append("  " + ln if ln else "")
    h.append("")
    h.append("Mathematical GEMM: %s" % ("YES" if case.is_gemm else "NO"))
    h.append("Expected recognition (specification): %s" % case.spec_recognition)
    h.append("  basis: %s" % SPEC_BASIS[case.spec_basis_key])
    h.append("Current implementation (predicted from the compiler sources; measured by")
    h.append("scripts/run_showcase.py, never assumed):")
    h.append("  recognition (recognizeLogicalMatmul):    %s" % case.cur_recognition.upper())
    h.append("  dense partition (getDenseLogicalGemm):   %s" % case.cur_dense)
    h.append("  lowering (--affine-matmul-to-systolic):  %s" % case.cur_lowering)
    if case.cur_reject_stage:
        h.append("  rejection at: %s" % case.cur_reject_stage)
        h.append("  reason: %s" % case.cur_reject_reason)
    cls = case.classification
    h.append("Classification: %s%s" % (cls.replace("_", " "),
                                       " -- see failures/issues/%s" % case.issue if case.issue else ""))
    if cls == "SPEC_OPEN":
        h.append("  The specification does not decide this case (%s, README.md section 2):" % case.spec_basis_key)
        for ln in _wrap(SPEC_QUESTIONS[case.spec_basis_key], 74):
            h.append("    " + ln)
        h.append("  The compiler's behaviour is recorded, not judged, until the question is settled.")
    elif cls == "TRUE_NEGATIVE":
        h.append("  Not a dense GEMM the compiler may lower; it must keep refusing it, for the")
        h.append("  reason above (the lowering pass's remark is compared with it).")
    elif cls == "DOCUMENTED_LIMITATION":
        h.append("  A mathematically valid GEMM that the specification excludes explicitly;")
        h.append("  'refused' must never be read as 'not a GEMM'.")
    h.append("")
    h.append("Independent reference: none at this level (no program to run); the checks are the")
    h.append("recognizer's result, the dense-partition result, the operand views it reports and")
    h.append("the lowering pass's outcome and remark, compared with the lines above.")
    h.append("")
    h.append("How to run:")
    h.append("  build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery \\")
    h.append("    showcase/%s/%s.mlir -o /dev/null" % (case.set, case.id))
    h.append("  build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true \\")
    h.append("    showcase/%s/%s.mlir -o /dev/null" % (case.set, case.id))
    h.append("  Stage by stage, results collected: scripts/run_showcase.py --case %s" % case.id)
    if case.notes:
        h.append("")
        for ln in case.notes.strip("\n").split("\n"):
            h.append(ln)
    out = ["// " + ln if ln else "//" for ln in h]
    out.append("")
    out.append(case.mlir.strip("\n"))
    out.append("")
    return "\n".join(out)


def mlir_manifest_entry(case: MlirCase, subdir: str) -> dict:
    cls = case.classification
    path = "%s/%s.mlir" % (subdir, case.id)
    cur_driver = "offloaded" if case.cur_lowering == "supported" else "stays_on_host"
    return {
        "id": case.id,
        "kind": "mlir",
        "file": path,
        "name": case.name,
        "set": case.set,
        "category": case.category,
        "kernel": case.kernel,
        "mathematical_class": case.math_class,
        "is_valid_gemm": case.is_gemm,
        "stages": ["discovery", "lowering"],
        "expected": {
            "_meaning": "the specification: set by the mathematics and the support "
                        "specification (README.md), never by the current compiler's output",
            "mathematical_gemm": case.is_gemm,
            "recognition": case.spec_recognition,
            "offload": {"ACCEPT": "offloaded", "UNDETERMINED": "undetermined"}.get(case.spec_recognition, "stays_on_host"),
            "basis": SPEC_BASIS[case.spec_basis_key],
            "spec_question": case.spec_basis_key if case.spec_recognition == "UNDETERMINED" else None,
            "views": case.expected_views,
            "runtime": "not applicable: an MLIR-level case is not compiled by systolic-cc nor executed",
        },
        "current_implementation": {
            "_meaning": "the behaviour predicted from the compiler sources; what scripts/run_showcase.py "
                        "measures (discovery + lowering) is compared with both layers",
            "frontend": "not_applicable",
            "recognition": case.cur_recognition,
            "hoisting_required": False,
            "dense_partition": case.cur_dense,
            "lowering": case.cur_lowering,
            "driver": cur_driver,
            "reject_stage": case.cur_reject_stage,
            "reject_reason": case.cur_reject_reason,
        },
        "classification": cls,
        "issue": case.issue or None,
        "has_independent_reference": False,
        "reference_kinds": [],
        "math_note": case.math_note.strip(),
        "tested": False,
        "how_to_run": {
            "discovery": "build/bin/systolic-opt --systolic-declare-fpga-fleet --test-affine-matmul-discovery "
                         "showcase/%s -o /dev/null" % path,
            "lowering": "build/bin/systolic-opt --systolic-declare-fpga-fleet --affine-matmul-to-systolic=report=true "
                        "showcase/%s -o /dev/null" % path,
            "stages": "python3 scripts/run_showcase.py --case %s" % case.id,
        },
    }


# --------------------------------------------------------------------------
# writing a generated test program: read-only once written
# --------------------------------------------------------------------------
#
# Every generated test source (.c / .mlir under showcase/, generated/,
# regression/corpus/, failures/) is made mode 0o555 the moment it is complete,
# so that a tool fixing the compiler cannot edit the test data by accident.
# The generator remains its only author: it never rewrites a protected file
# with different content unless told to (force), and a file that would be
# regenerated identically is left alone (only its mode is enforced).
# Metadata (manifest.json, rejected.json, registry, coverage, reports) stays
# writable: it is updated across runs and is not test data.

TEST_SOURCE_MODE = 0o555


class ProtectedTestSource(Exception):
    """The destination is an existing, protected test source with different
    content; regenerating it must be explicit."""


def write_test_source(path: str, text: str, force: bool = False) -> str:
    """Write `text` to `path` and make it read-only (0o555).  Returns
    'written', or 'unchanged' when the file already holds exactly `text` (its
    mode is enforced, nothing is rewritten).  Raises ProtectedTestSource when
    the file exists with different content and `force` is false; with
    `force` the old file is unlinked and replaced."""
    if os.path.exists(path):
        with open(path) as f:
            old = f.read()
        if old == text:
            os.chmod(path, TEST_SOURCE_MODE)
            return "unchanged"
        if not force:
            raise ProtectedTestSource(
                "%s exists with different content and is a protected test source; "
                "regenerate it deliberately (--force)" % path)
        os.unlink(path)   # a read-only file can be unlinked from a writable directory
    with open(path, "w") as f:
        f.write(text)
    os.chmod(path, TEST_SOURCE_MODE)
    return "written"


def protect_test_source(path: str) -> None:
    """Make an already written or copied test source read-only (0o555)."""
    os.chmod(path, TEST_SOURCE_MODE)


# --------------------------------------------------------------------------
# manifest entries
# --------------------------------------------------------------------------


COMPILER_COMMIT_DEFAULT = "6e25528"


def index_form_kind(f: LinearForm, roles: Dict[str, str]) -> str:
    """The kind of affine index expression f is (for coverage accounting):
    constant, identity (i), offset (i + c), stride (c*i), stride+offset,
    reversed (a negative coefficient), split (several loops of one
    coordinate, mixed radix), coupled (loops of different coordinates)."""
    if f.is_const():
        return "constant"
    if len(f.coef) == 1:
        (v, c), = f.coef.items()
        if c < 0:
            return "reversed"
        if c == 1:
            return "identity" if f.const == 0 else "offset"
        return "stride" if f.const == 0 else "stride+offset"
    groups = {roles.get(v, "?") for v in f.coef}
    if len(groups) == 1:
        if any(c < 0 for c in f.coef.values()):
            return "reversed"
        return "split" if f.const == 0 else "split+offset"
    return "coupled"


def index_forms(case: Case) -> dict:
    """Per operand, the kind of each subscript, and per shared coordinate how
    the two operands' expressions relate (identical / own offset / own stride
    / own offset and stride / different loop combination)."""
    roles = {lp.var: lp.role for lp in case.loops}
    out = {"operands": {}, "shared": {}}
    forms = {}
    for role, arr in case.arrays.items():
        try:
            forms[role] = arr.forms()
        except NotAffine:
            out["operands"][arr.name] = ["not_affine"]
            continue
        out["operands"][arr.name] = [index_form_kind(f, roles) for f in forms[role]]
    an = case.analysis or {}
    v = an.get("views")
    if v:
        for ra, rb, g in (("a", "c", "I"), ("b", "c", "J"), ("a", "b", "K")):
            fa_ = forms[ra][v[ra][g]["position"]]
            fb_ = forms[rb][v[rb][g]["position"]]
            if fa_ == fb_:
                rel = "identical"
            elif shared_coordinate(fa_, fb_) is None:
                rel = "different loop combination"
            else:
                sa, sb = _gcd_content(fa_), _gcd_content(fb_)
                rel = ("own stride and offset" if (sa != sb and fa_.const != fb_.const) else
                       "own stride" if sa != sb else "own offset")
            out["shared"][g] = {"operands": "%s/%s" % (case.arrays[ra].name, case.arrays[rb].name),
                                "relation": rel, "expressions": [fa_.c_text(), fb_.c_text()]}
    return out


def loop_summary(case: Case) -> List[str]:
    return ["%s [%s]: %d %s %s %s %d, step %d, %d iteration(s)%s" % (
        lp.var, lp.role, lp.lb, "<=" if lp.step > 0 else ">=", lp.var, lp.cmp, lp.ub, lp.step, lp.trip(),
        " (written as the condition %s)" % lp.cond if lp.cond else "")
        for lp in case.loops]


def manifest_entry(case: Case, subdir: str) -> dict:
    an = case.analysis or analyse(case)
    ex = case.expect or predict(case)
    spec = case.spec or spec_expect(case)
    cls = case.classification or classify_case(case)
    path = "%s/%s.c" % (subdir, case.id)
    entry = {
        "id": case.id,
        "kind": "c",
        "file": path,
        "name": case.name,
        "set": case.set,
        "category": case.category,
        "kernel": case.kernel,
        "mathematical_class": an["math_class"],
        # a GEMM or a batched GEMM of affine views (docs/AFFINE_GEMM_MAPPING.md
        # section 19): the specification's finding, not only the dense
        # analysis of the subscripts as written
        "is_valid_gemm": bool(spec.get("legal_gemm")),
        "gemm_tags": an.get("gemm_tags", []),
        "c_program_valid": True,
        "in_affine_c_subset": ex["frontend"] == "translated",
        "element_type": case.elem,
        "statement": case.stmt,
        "kernel_style": case.kernel_style,
        "loops": [dict({"var": lp.var, "role": lp.role, "lb": lp.lb, "ub_exclusive": lp.exclusive_ub(),
                        "step": lp.step, "cmp": lp.cmp, "trip": lp.trip()},
                       **({"cond": lp.cond} if lp.cond else {})) for lp in case.loops],
        "arrays": {role: {"name": arr.name, "dims": list(arr.dims), "subscripts": list(arr.subs),
                          "restrict": arr.restrict, "elem": arr.elem}
                   for role, arr in case.arrays.items()},
        "repetitions": an.get("repetitions", 1),
        "logical_extents": an.get("extents"),
        "index_forms": index_forms(case),
        "loop_summary": loop_summary(case),
        "expected": {
            "_meaning": "the specification: set by the mathematics and the support "
                        "specification (README.md), never by the current compiler's output",
            "mathematical_gemm": bool(spec.get("legal_gemm")),
            "recognition": spec["recognition"],
            "offload": spec["offload"],
            "basis": spec["basis"],
            "spec_question": spec.get("spec_question"),
            "runtime": "the program's own check must pass on every backend (kernel == references)",
        },
        "current_implementation": {
            "_meaning": "the behaviour predicted from reading commit %s; what scripts/run_showcase.py "
                        "measures is compared with both layers" % COMPILER_COMMIT_DEFAULT,
            "frontend": ex["frontend"],
            "recognition": ex["recognition"],
            "hoisting_required": ex["hoisting_required"],
            "dense_partition": ex["dense_partition"],
            "lowering": ex["lowering"],
            "driver": ex["driver"],
            "reject_stage": ex["reject_stage"],
            "reject_reason": ex["reject_reason"],
        },
        "classification": cls,
        "issue": case.issue or None,
        "redecided": case.redecided or None,
        "has_independent_reference": True,
        "reference_kinds": ["direct"] + (["logical_view"] if reference_logical_text(case) else [])
                           + ["python_oracle_checksum"],
        "expected_checksum": checksum(evaluate(case)["c"]),
        "math_note": case.math_note.strip(),
        "tested": False,
        "how_to_run": {
            "cpu": "cc -std=c11 -O1 showcase/%s -o %s && ./%s" % (path, case.id, case.id),
            "soft": ("build/bin/systolic-cc --runtime=soft --explain --verify -DGEMM_RECOGNITION_STATS "
                     "showcase/%s -o %s && ./%s" % (path, case.id, case.id)),
            "stages": "python3 scripts/run_showcase.py --case %s" % case.id,
        },
    }
    if "not_dense_reasons" in an:
        entry["why_not_a_dense_gemm"] = an["not_dense_reasons"]
    return entry
