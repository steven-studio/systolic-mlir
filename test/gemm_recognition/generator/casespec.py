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
import os
import itertools
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
    ub: int            # exclusive bound for cmp '<', inclusive for '<='
    step: int = 1
    cmp: str = "<"     # '<' or '<='
    role: str = "X"    # I, J, K, P, R, X
    ctype: str = "int"
    inc: str = "++"    # '++' (prefix), 'post++', '+=', '=+'

    def exclusive_ub(self):
        return self.ub + 1 if self.cmp == "<=" else self.ub

    def values(self):
        return list(range(self.lb, self.exclusive_ub(), self.step))

    def trip(self):
        return len(self.values())

    def c_header(self):
        if self.inc == "++":
            inc = "++%s" % self.var if self.step == 1 else "%s += %d" % (self.var, self.step)
        elif self.inc == "post++":
            inc = "%s++" % self.var if self.step == 1 else "%s += %d" % (self.var, self.step)
        elif self.inc == "+=":
            inc = "%s += %d" % (self.var, self.step)
        elif self.inc == "=+":
            inc = "%s = %s + %d" % (self.var, self.var, self.step)
        else:
            raise ValueError(self.inc)
        return "for (%s %s = %d; %s %s %d; %s)" % (
            self.ctype, self.var, self.lb, self.var, self.cmp, self.ub, inc)


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
    n_used = len(used_vars)
    if n_used < 3:
        ex["recognition"] = "reject"
        return stop("recognition", "fewer than three loop counters are used by the accesses: no "
                    "three-deep affine.for candidate whose loops are all GEMM dimensions")
    nr = [len(forms[r]) for r in ("a", "b", "c")]
    if len(set(nr)) != 1 or nr[0] < 2:
        ex["recognition"] = "reject"
        return stop("recognition", "A, B and C must have the same number of subscripts (>= 2); "
                    "they have %s" % nr)
    npart = nr[0] - 2
    fa, fb, fc = forms["a"], forms["b"], forms["c"]

    def matches(a, b):
        for p in range(npart):
            if a[p] != fc[p] or b[p] != fc[p]:
                return False
        I, K, J = a[npart], a[npart + 1], b[npart + 1]
        return (I == fc[npart] and J == fc[npart + 1] and K == b[npart]
                and I != J and I != K and J != K)

    if not (matches(fa, fb) or matches(fb, fa)):
        ex["recognition"] = "reject"
        return stop("recognition", "the accesses are not A(P*,I,K), B(P*,K,J), C(P*,I,J) with "
                    "the same affine expression for each shared coordinate")
    if matches(fa, fb):
        I, K, J = fa[npart], fa[npart + 1], fb[npart + 1]
    else:
        I, K, J = fb[npart], fb[npart + 1], fa[npart + 1]
    # repetition loops: outside the used loops they are fine; between them
    # they are hoisted outward when the interchange is legal
    # (affine::isValidLoopInterchangePermutation) -- it is not when a
    # reduction loop of more than one iteration encloses a repetition loop
    # of more than one iteration: moving it out would reorder the
    # accumulations into one C element.  (With a single-iteration reduction
    # or repetition nothing is reordered and the hoist is legal: measured,
    # see failures/resolved/NOTE-001.)
    used_idx = [i for i, lp in enumerate(loops) if lp.var in used_vars]
    rep_idx = [i for i, lp in enumerate(loops) if lp.var not in used_vars]
    k_idx = [i for i in used_idx if loops[i].var in K.coef and loops[i].trip() >= 2]
    for ri in rep_idx:
        if loops[ri].trip() >= 2 and any(ki < ri for ki in k_idx):
            ex["recognition"] = "reject"
            return stop("recognition", "loop %d of the nest repeats the accumulation inside it, and "
                        "moving it outward is not a legal loop interchange (it would reorder "
                        "dependent iterations)" % ri)
    if any(ri > used_idx[0] for ri in rep_idx):
        ex["hoisting_required"] = True
    ex["recognition"] = "accept"

    # ---- dense partition: getDenseLogicalGemm
    if npart:
        ex["dense_partition"] = "unsupported"
        return stop("dense_partition", "batched GEMM (a partition prefix P* of %d coordinates) "
                    "is not supported" % npart)
    used_loops = [loops[i] for i in used_idx]
    for s, lp in enumerate(used_loops):
        drives = [name for name, f in (("I", I), ("J", J), ("K", K)) if lp.var in f.coef]
        if len(drives) > 1:
            ex["dense_partition"] = "unsupported"
            return stop("dense_partition", "two logical coordinates are driven by the same loop "
                        "(loop %d drives %s)" % (s, " and ".join(drives)))
        if not drives:
            ex["dense_partition"] = "unsupported"
            return stop("dense_partition", "loop %d drives no logical coordinate" % s)
    for name, f in (("I", I), ("J", J), ("K", K)):
        scaled = []
        for lp in used_loops:
            if lp.var in f.coef:
                sc = f.coef[lp.var] * lp.step
                if sc < 0:
                    ex["dense_partition"] = "unsupported"
                    return stop("dense_partition", "logical coordinate %s decreases along its loop" % name)
                scaled.append((sc, lp.trip()))
        scaled.sort()
        expected = scaled[0][0]
        for sc, ext in scaled:
            if sc != expected:
                ex["dense_partition"] = "unsupported"
                return stop("dense_partition", "logical coordinate %s is driven by %d loops that do "
                            "not enumerate it densely (scales must grow by the finer loops' "
                            "extents)" % (name, len(scaled)))
            expected *= ext
    ex["dense_partition"] = "supported"

    # ---- lowering: --affine-matmul-to-systolic
    for r, nm in (("a", "A"), ("b", "B"), ("c", "C")):
        if len(case.arrays[r].dims) != 2:
            ex["lowering"] = "unsupported"
            return stop("lowering", "%s is not a rank-2 memref with the identity layout" % nm)

    def dense_of(f):
        lps = [lp for lp in used_loops if lp.var in f.coef]
        off = f.const + sum(f.coef[lp.var] * lp.lb for lp in lps)
        scale = min(f.coef[lp.var] * lp.step for lp in lps)
        ext = volume([lp.trip() for lp in lps])
        return off, scale, ext

    dI, dJ, dK = dense_of(I), dense_of(J), dense_of(K)
    for r, nm, (d0, d1) in (("a", "A", (dI, dK)), ("b", "B", (dK, dJ)), ("c", "C", (dI, dJ))):
        arr = case.arrays[r]
        for pos, (off, scale, ext) in enumerate((d0, d1)):
            last = off + scale * (ext - 1)
            # the leading dimension of a C array parameter is dynamic in the
            # memref (not checked by the pass); the trailing one is static
            if off < 0 or (pos == 1 and last >= arr.dims[pos]):
                ex["lowering"] = "unsupported"
                return stop("lowering", "%s is accessed out of bounds along dimension %d" % (nm, pos))
    if case.a.elem != case.c.elem or case.b.elem != case.c.elem:
        ex["lowering"] = "unsupported"
        return stop("lowering", "A, B and C have different element types")
    for r, nm in (("a", "A"), ("b", "B")):
        arr = case.arrays[r]
        if arr.name == case.c.name:
            ex["lowering"] = "unsupported"
            return stop("lowering", "%s may overlap C" % nm)
        if not (arr.restrict or case.c.restrict):
            ex["lowering"] = "unsupported"
            return stop("lowering", "%s may overlap C; declare the arguments llvm.noalias (C restrict) "
                        "or pass assume-no-alias" % nm)
    if case.a.name == case.b.name and (dI, dK) != (dK, dJ):
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

SPEC_BASIS = {
    "dense_gemm": "a dense GEMM with affine operand views: include/Systolic/AffineMatmulPartitioning.h "
                  "('loops in any order, steps, nonzero origins, access offsets and strides, and "
                  "coordinates split over several loops ... are all of this form'); include/Systolic/"
                  "AffineMatmulAnalysis.h (F_A = phiA o (P,I,K), F_B = phiB o (P,K,J), F_C = phiC o "
                  "(P,I,J): one factor map per operand); the task statement's index family "
                  "c_i*i + c_j*j + c_k*k + c_0 per operand",
    "batched": "include/Systolic/AffineMatmulPartitioning.h: 'batched GEMMs (a partition prefix P*) "
               "... are not' of the dense form; getDenseLogicalGemm refuses them with a message; "
               "test/Systolic/affine_to_systolic_refused.mlir @batched",
    "transposed": "the documented operand convention A = (I, K), B = (K, J), C = (I, J) as row-major "
                  "views of identity-layout rank-2 memrefs (AffineMatmulAnalysis.cpp recognizeLogicalMatmul, "
                  "AffineMatmulToSystolic.cpp getLogicalOperand); the linalg path refuses transposed "
                  "indexing maps as well (test/Systolic/bad_linalg_to_systolic.mlir)",
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
    "negative_scale": "AffineMatmulPartitioning.cpp getDenseLogicalGemm: 'logical coordinate decreases along "
                      "its loop' (a memref.subview has no negative stride)",
    "repeat_in_reduction": "AffineMatmulToSystolic.cpp hoistRepetitionLoops: a repeating loop inside a "
                           "reduction loop (both of more than one iteration) cannot be moved outward -- "
                           "affine::isValidLoopInterchangePermutation refuses it, the accumulation order "
                           "would change; test/SystolicCC/depth.c d4_illegal",
    "same_memref_views": "AffineMatmulToSystolic.cpp: 'A and B are different views of the same memref'",
    "matvec": "AffineMatmulAnalysis.cpp collectAffineMatmulCandidates expects a three-deep affine.for "
              "nest with rank-2 (or batched rank-3) operands; a GEMV has two index dimensions",
    "not_gemm": "the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)",
}


def spec_expect(case: Case) -> dict:
    an = case.analysis or analyse(case)
    tags = set(an.get("gemm_tags", []))
    out = {"recognition": None, "basis_key": None, "basis": None, "legal_gemm": bool(an.get("is_gemm"))}

    def done(rec, key):
        out["recognition"] = rec
        out["basis_key"] = key
        out["basis"] = SPEC_BASIS[key]
        out["offload"] = "offloaded" if rec == "ACCEPT" else "stays_on_host"
        case.spec = out
        return out

    if case.kernel_style == "scalar_acc":
        return done("OUT_OF_SCOPE", "scalar_acc")
    if not an.get("is_gemm"):
        if an["math_class"] == "matrix_vector":
            return done("OUT_OF_SCOPE", "matvec")
        return done("REJECT", "not_gemm")
    if "batched" in tags:
        return done("OUT_OF_SCOPE", "batched")
    if tags & {"A_transposed", "B_transposed", "C_transposed"}:
        return done("OUT_OF_SCOPE", "transposed")
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
    # a repetition loop (of more than one iteration) inside a reduction loop
    # of more than one iteration: hoisting it would reorder the accumulation
    loops = case.loops
    used = set()
    for a in case.arrays.values():
        for f in a.forms():
            used |= f.vars()
    k_idx = [i for i, lp in enumerate(loops) if lp.role == "K" and lp.trip() >= 2]
    for ri, lp in enumerate(loops):
        if lp.var not in used and lp.trip() >= 2 and any(ki < ri for ki in k_idx):
            return done("OUT_OF_SCOPE", "repeat_in_reduction")
    # a coordinate that decreases along a loop
    for a in case.arrays.values():
        for f in a.forms():
            if any(c < 0 for c in f.coef.values()):
                return done("OUT_OF_SCOPE", "negative_scale")
    if case.a.name == case.b.name:
        # one array as both operands: the lowering serves both through ONE
        # read-only view, so A's (I, K) view must be B's (K, J) view
        v = an["views"]
        ext = an["extents"]

        def key(role, g):
            return (v[role][g]["first"], v[role][g]["last"], ext[g])

        if key("a", "I") != key("b", "K") or key("a", "K") != key("b", "J"):
            return done("OUT_OF_SCOPE", "same_memref_views")
    return done("ACCEPT", "dense_gemm")


def classify_case(case: Case) -> str:
    spec = case.spec or spec_expect(case)
    cur = case.expect or predict(case)
    offloaded = cur["driver"] == "offloaded"
    if spec["recognition"] == "ACCEPT":
        c = "TRUE_POSITIVE" if offloaded else "FALSE_NEGATIVE"
    elif spec["recognition"] == "REJECT":
        c = "FALSE_POSITIVE" if offloaded else "TRUE_NEGATIVE"
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
        hdr = "for (long %s = %d; %s < %d; %s += %d)" % (
            lp.var, lp.lb, lp.var, lp.exclusive_ub(), lp.var, lp.step)
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
                out.append("  " * dd + "for (long %s = %d; %s < %d; %s += %d)" % (
                    lp.var, lp.lb, lp.var, lp.exclusive_ub(), lp.var, lp.step))
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


def math_description(case: Case) -> List[str]:
    """Header lines: the statement over its loop domain, and the logical view."""
    an = case.analysis
    A, B, C = case.a, case.b, case.c
    a, b, c = A.name + _sub_text(A), B.name + _sub_text(B), C.name + _sub_text(C)
    d = case.arrays["d"].name + _sub_text(case.arrays["d"]) if "d" in case.arrays else None
    out = []
    dom = ", ".join("%s in {%d, %d, ..., %d}" % (lp.var, lp.lb, lp.lb + lp.step, lp.values()[-1])
                    if lp.step != 1 else "%s in [%d, %d)" % (lp.var, lp.lb, lp.exclusive_ub())
                    for lp in case.loops)
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
    h.append("Mathematical GEMM: %s" % ("YES" if an.get("is_gemm") else "NO"))
    h.append("Expected recognition (specification): %s" % spec["recognition"])
    h.append("  basis: %s" % spec["basis"])
    h.append("Current implementation (commit %s, predicted from its sources; measured by" % compiler_commit)
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
    h.append("Classification: %s%s" % (cls.replace("_", " "), " -- see failures/issues/%s" % case.issue if case.issue else ""))
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


def manifest_entry(case: Case, subdir: str) -> dict:
    an = case.analysis or analyse(case)
    ex = case.expect or predict(case)
    spec = case.spec or spec_expect(case)
    cls = case.classification or classify_case(case)
    path = "%s/%s.c" % (subdir, case.id)
    entry = {
        "id": case.id,
        "file": path,
        "name": case.name,
        "set": case.set,
        "category": case.category,
        "kernel": case.kernel,
        "mathematical_class": an["math_class"],
        "is_valid_gemm": bool(an.get("is_gemm")),
        "gemm_tags": an.get("gemm_tags", []),
        "c_program_valid": True,
        "in_affine_c_subset": ex["frontend"] == "translated",
        "element_type": case.elem,
        "statement": case.stmt,
        "kernel_style": case.kernel_style,
        "loops": [{"var": lp.var, "role": lp.role, "lb": lp.lb, "ub_exclusive": lp.exclusive_ub(),
                   "step": lp.step, "cmp": lp.cmp, "trip": lp.trip()} for lp in case.loops],
        "arrays": {role: {"name": arr.name, "dims": list(arr.dims), "subscripts": list(arr.subs),
                          "restrict": arr.restrict, "elem": arr.elem}
                   for role, arr in case.arrays.items()},
        "repetitions": an.get("repetitions", 1),
        "logical_extents": an.get("extents"),
        "expected": {
            "_meaning": "the specification: set by the mathematics and the support "
                        "specification (README.md), never by the current compiler's output",
            "mathematical_gemm": bool(an.get("is_gemm")),
            "recognition": spec["recognition"],
            "offload": spec["offload"],
            "basis": spec["basis"],
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
