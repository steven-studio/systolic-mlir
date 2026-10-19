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
    # Bounds affine in the enclosing counters (docs/AFFINE_GEMM_MAPPING.md
    # section 19.12): a coefficient per enclosing loop variable, added to lb
    # and to ub respectively. Equal terms -- the two bounds shift together,
    # for (int k = i; k < i + 4; ++k) -- make the iteration set a
    # constant-extent parallelotope (family F: translated by the C front end,
    # mapped onto its box by the compiler); different terms (a triangle,
    # k in [i, 8)) are translated too since 2026-10-18 (docs section 19.13:
    # exact subscript ranges by running the loops) -- the compiler maps such
    # a set onto a box when its points are a parallelotope (family X), else
    # leaves it as written.
    lb_terms: Dict[str, int] = field(default_factory=dict)
    ub_terms: Dict[str, int] = field(default_factory=dict)

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
        if self.cond and self.ctype.split()[0] == "unsigned":
            # systolic-cfront evaluates every operation and conversion of a
            # written condition in its own C type and translates the loop
            # only when none leaves its type's range where C evaluates it --
            # for an unsigned counter, when no operation wraps around
            # (docs/AFFINE_GEMM_MAPPING.md section 19.13.1; it refused every
            # unsigned one before the review of 2026-10-18). predict() does
            # not model those types, so such a loop is not a case (#29).
            raise ValueError("%s: an unsigned counter with a written condition (%r): the C front end "
                             "translates it only when no operation of the condition wraps around in "
                             "its C type, which the predictor does not model" % (self.var, self.cond))
        self.lb_terms = {k: v for k, v in self.lb_terms.items() if v}
        self.ub_terms = {k: v for k, v in self.ub_terms.items() if v}
        if self.var in self.lb_terms or self.var in self.ub_terms:
            raise ValueError("%s: a bound reads the loop's own counter" % self.var)

    def decreasing(self):
        return self.step < 0

    def is_box(self):
        """Constant bounds."""
        return not self.lb_terms and not self.ub_terms

    def shifts_together(self):
        """Family F (docs/AFFINE_GEMM_MAPPING.md section 19.12): the two
        bounds shifted by the same form of the enclosing counters, so the
        trip count is a constant; true of a plain loop."""
        return self.lb_terms == self.ub_terms

    def bound_vars(self):
        return set(self.lb_terms) | set(self.ub_terms)

    def _shift(self, terms, env):
        if not terms:
            return 0
        if env is None:
            raise ValueError("%s: its bounds read %s; the enclosing counters' values are needed"
                             % (self.var, ", ".join(sorted(terms))))
        return sum(c * env[v] for v, c in terms.items())

    def lb_at(self, env=None):
        """The lower bound at the enclosing counters' values."""
        return self.lb + self._shift(self.lb_terms, env)

    def ub_at(self, env=None):
        """The upper bound as written, at the enclosing counters' values."""
        return self.ub + self._shift(self.ub_terms, env)

    def _exclusive(self, hi):
        if self.cmp == "<=":
            return hi + 1
        if self.cmp == ">=":
            return hi - 1
        return hi

    def exclusive_ub(self):
        """The bound in range() terms: the first value NOT visited, in the
        direction of the step (its constant part for a shifted loop)."""
        return self._exclusive(self.ub)

    @staticmethod
    def _bound_text(terms, const):
        """A bound as C text: 2 * i + k - 3 (the enclosing counters by name)."""
        parts = []
        for v, c in sorted(terms.items()):
            parts.append(v if c == 1 else "-%s" % v if c == -1 else "%d * %s" % (c, v))
        text = " + ".join(parts)
        if const or not parts:
            text = (text + " + " if text else "") + str(const)
        return text.replace("+ -", "- ")

    def values(self, env=None):
        """The counter's values: of a plain loop as declared; of a loop whose
        bounds shift together, WITHOUT the shift when `env` is None (the
        values of the box form's counter, docs section 19.12) and with it at
        the enclosing counters' values `env`; of a loop whose bounds do not
        shift together only at `env`."""
        if self.is_box() or (env is None and self.shifts_together()):
            lo, hi = self.lb, self.exclusive_ub()
        else:
            lo, hi = self.lb_at(env), self._exclusive(self.ub_at(env))
        declared = list(range(lo, hi, self.step))
        if self.cond and (env is not None or self.is_box()):
            # (a condition over the enclosing counters is checked where they
            # have values; without them the box form's values are the
            # declared ones)
            # run the condition as written (integer + - * and comparisons
            # mean the same in Python as in C over these values), the
            # enclosing counters at their values: the declared equivalent
            # must visit exactly the same values
            written, v = [], lo
            scope = dict(env or {})
            while eval(self.cond, {"__builtins__": {}}, dict(scope, **{self.var: v})):
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
        cond = self.cond or "%s %s %s" % (self.var, self.cmp, self._bound_text(self.ub_terms, self.ub))
        return "for (%s %s = %s; %s; %s)" % (self.ctype, self.var,
                                            self._bound_text(self.lb_terms, self.lb), cond, inc)

    def _compound(self, s):
        return "%s %s %d" % (self.var, "+=" if self.step > 0 else "-=", s)

    def reference_header(self, ctype="long"):
        """The same iteration set as a reference loop: for (v = lb; v < / >
        exclusive bound; v += step)."""
        return "for (%s %s = %s; %s %s %s; %s += %d)" % (
            ctype, self.var, self._bound_text(self.lb_terms, self.lb), self.var,
            "<" if self.step > 0 else ">", self._bound_text(self.ub_terms, self.exclusive_ub()),
            self.var, self.step)


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


def elem_value(e: int, seed: int, nonzero: bool = False) -> int:
    """The value of flat element e of an array initialised with `seed`:
    integers in [-3, 3].  C: (e * 37 + seed * 101) % 103 % 7 - 3, e, seed >= 0.
    With `nonzero`, a 0 is replaced by 1 (C: elem_nz)."""
    v = (e * 37 + seed * 101) % 103 % 7 - 3
    return v if v or not nonzero else 1


def factor_nonzero(case: "Case", name: str) -> bool:
    """The factors of an offloaded assignment C = A * B hold no zeros: a -0
    product (a zero times a negative factor) comes back from the tiles as +0
    -- the documented deviation of docs/AFFINE_GEMM_MAPPING.md sections 6
    and 20, which a bitwise check would report -- so the data avoids it.
    Accumulations keep the plain data (C starts non-zero, so a -0 product
    cannot change C's sign)."""
    return (case.stmt == "=AB" and case.role_of_name(name) in ("a", "b")
            and c_written_once(case))


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
# iteration domains that are not boxes (docs/AFFINE_GEMM_MAPPING.md 19.12)
# --------------------------------------------------------------------------


def is_box_case(case: "Case") -> bool:
    return all(lp.is_box() for lp in case.loops)


def shifts_together(case: "Case") -> bool:
    """Family F: every loop's two bounds shifted by the same form of the
    enclosing counters (a constant-extent parallelotope)."""
    return all(lp.shifts_together() for lp in case.loops)


def domain_points(loops):
    """The iteration points as environments, in program order, each loop's
    bounds evaluated at the enclosing counters' values (any shape of
    domain)."""
    out = []
    env = {}

    def rec(depth):
        if depth == len(loops):
            out.append(dict(env))
            return
        lp = loops[depth]
        for v in lp.values(env):
            env[lp.var] = v
            rec(depth + 1)
        env.pop(lp.var, None)

    rec(0)
    return out


def box_form(case: "Case") -> "Case":
    """The equivalent box nest of a family-F case: x = U x' with
    x_s = x'_s + l_s . x_<s (U lower unitriangular -- a bijection of the two
    iteration sets that preserves their order), the bounds constant and every
    subscript composed with U: the nest the compiler analyses after its domain
    normalization (docs/AFFINE_GEMM_MAPPING.md section 19.12), the same
    computation on the same elements in the same order. The case itself when
    it is a box; ValueError when the bounds do not shift together."""
    if is_box_case(case):
        return case
    if not shifts_together(case):
        raise ValueError("%s: a loop's bounds do not shift together (not a parallelotope)" % case.id)
    import copy
    box = copy.deepcopy(case)
    shift = {lp.var: dict(lp.lb_terms) for lp in case.loops if lp.lb_terms}
    order = [lp.var for lp in case.loops]

    def compose(f: LinearForm) -> LinearForm:
        coef = dict(f.coef)
        # innermost first: a shift's own counters are substituted after it
        for v in reversed(order):
            c = coef.get(v, 0)
            if c == 0 or v not in shift:
                continue
            for t, l in shift[v].items():
                coef[t] = coef.get(t, 0) + c * l
        return LinearForm(coef, f.const)

    for arr in box.arrays.values():
        arr.subs = tuple(compose(parse_affine(sub)).c_text() for sub in arr.subs)
    for lp in box.loops:
        if not lp.is_box():
            # a condition written over the enclosing counters (k - i < 4)
            # means the declared bound, which the box keeps
            lp.cond = ""
        lp.lb_terms, lp.ub_terms = {}, {}
    box.analysis, box.expect, box.spec, box.classification = {}, {}, {}, ""
    return box


# The C front end computes a non-parallelotope nest's subscript ranges by
# running its loops (systolic-cfront.c enumerate_points): it refuses the
# function when one run gives the counters in scope more than this many
# values -- every level counts, not only the innermost points, so the limit
# bounds the work (docs/AFFINE_GEMM_MAPPING.md section 19.13.1).
FRONTEND_ENUMERATION_LIMIT = 1 << 22


def frontend_enumeration_values(loops, limit=FRONTEND_ENUMERATION_LIMIT):
    """The values a run of the nest gives its counters, outermost first (the
    points of the outermost loop, plus those of the outer two, ... through
    the full nest), counted up to limit + 1: the front end's measure."""
    count = 0
    env = {}

    def rec(depth):
        nonlocal count
        if depth == len(loops) or count > limit:
            return
        lp = loops[depth]
        for v in lp.values(env):
            if count > limit:
                break
            count += 1
            env[lp.var] = v
            rec(depth + 1)
        env.pop(lp.var, None)

    rec(0)
    return count


def _loop_vars(case: "Case"):
    return [lp.var for lp in case.loops]


def _used_vars(case: "Case"):
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    return used


def exact_form(case: "Case"):
    """Family X (docs/AFFINE_GEMM_MAPPING.md section 19.13), decided by
    enumeration -- the twin of the compiler's Presburger search: a case whose
    bounds do not shift together but whose exact iteration set (all loops,
    domain_points) is a constant-extent parallelotope with shift rows from
    {0, the loop's lower-bound terms, its upper-bound terms}: per loop,
    outermost first, the projection onto the first s + 1 counters equals the
    parallelotope's (values a + row . x_<s + |step| t up to e). A copy of the
    case whose bounds are the parallelotope's -- family F, so box_form
    applies --, or None (also for a case already in F)."""
    if shifts_together(case):
        return None
    import copy
    pts = domain_points(case.loops)
    if not pts:
        return None
    names = _loop_vars(case)
    points = [tuple(env[v] for v in names) for env in pts]
    prev = {()}
    rows = []
    for s, lp in enumerate(case.loops):
        st = abs(lp.step)
        cands = [{}]
        for terms in (lp.lb_terms, lp.ub_terms):
            if terms and dict(terms) not in cands:
                cands.append(dict(terms))
        proj = {x[:s + 1] for x in points}
        hit = None
        for row in cands:
            coef = [row.get(v, 0) for v in names[:s]]
            vals = [x[s] - sum(c * v for c, v in zip(coef, x[:s])) for x in proj]
            a, e = min(vals), max(vals)
            if (e - a) % st:
                continue
            cand = set()
            for p in prev:
                base = sum(c * v for c, v in zip(coef, p))
                for t in range((e - a) // st + 1):
                    cand.add(p + (a + base + st * t,))
            if cand == proj:
                hit = (row, a, e)
                break
        if hit is None:
            return None
        rows.append(hit)
        prev = proj
    form = copy.deepcopy(case)
    for lp, (row, a, e) in zip(form.loops, rows):
        lp.lb_terms, lp.ub_terms = dict(row), dict(row)
        if lp.step > 0:
            lp.lb, lp.ub, lp.cmp, lp.cond = a, e + 1, "<", ""
        else:
            # a loop counting down runs from e down to a (inclusive): the
            # values it takes on the set, fewer than the declared ones when
            # an inner loop is empty at some of them (review 2026-10-18 #24;
            # the front end refuses a counting-down loop whose bounds read a
            # counter, so its row is zero in every translated nest)
            lp.lb, lp.ub, lp.cmp, lp.cond = e, a, ">=", ""
    form.analysis, form.expect, form.spec, form.classification = {}, {}, {}, ""
    return form


def _independent(case: "Case", rep):
    """('prefix' | 'interleaved', rep) when the loops `rep` (a nonempty list
    in nest order) read only each other's counters in their bounds and the
    other loops only the others' -- the iteration set is then the product of
    the two groups' sets --, else None."""
    if not rep:
        return None
    names = _loop_vars(case)
    others = set(names) - set(rep)
    for lp in case.loops:
        if not lp.bound_vars() <= (set(rep) if lp.var in rep else others):
            return None
    return ("prefix" if names[:len(rep)] == rep else "interleaved"), rep


def repetition_structure(case: "Case"):
    """The repetition loops of a case whose bounds do not shift together, when
    they are independent of the others (docs section 19.13): the loops no
    access reads (R), each with bounds over R's counters only, the others (D)
    with bounds over D's counters only -- the iteration set is then the
    product of R's and D's, every point of D runs |R| times. ('prefix' |
    'interleaved', R's vars) or None."""
    used = _used_vars(case)
    return _independent(case, [v for v in _loop_vars(case) if v not in used])


def _normalizable(case: "Case") -> bool:
    """A box, a parallelotope as written, or an exact parallelotope."""
    return is_box_case(case) or shifts_together(case) or exact_form(case) is not None


def _without(case: "Case", drop):
    """The case with the loops of `drop` removed (the band below a prefix of
    repetition loops)."""
    import copy
    sub = copy.deepcopy(case)
    sub.loops = [lp for lp in sub.loops if lp.var not in drop]
    sub.analysis, sub.expect, sub.spec, sub.classification = {}, {}, {}, ""
    return sub


def _leading(case: "Case", group):
    """The outermost loops of the nest that are in `group`, up to the first
    that is not."""
    lead = []
    for v in _loop_vars(case):
        if v not in group:
            break
        lead.append(v)
    return lead


def _in_family_r(case: "Case", rep) -> bool:
    """Whether independent repetition loops `rep` are in the documented
    family R (docs section 19.13.3), as the compiler handles them:

    - a prefix of the leading run of them around a band the normalization
      rewrites AS WRITTEN -- its inner repetition loops included (a box,
      family F or X). The compiler retries the band below every prefix of
      the nest whose counters nothing below it reads, shortest first, and
      takes the first that normalizes (normalizeParallelotopeDomains, its
      'start' loop; review 2026-10-19: below a shorter prefix too, not only
      the whole run); the normalization gives the band's loops constant
      bounds, and the hoist then moves its inner repetition loops out
      (review 2026-10-18 #21: they need not be part of the run);
    - or GEMM loops that all have constant bounds, the repetition loops of
      any shape (the hoist moves them out of the GEMM loops).

    An inner triangle of repetitions inside a shifted band is in neither: the
    band as written is no parallelotope. A loop of one iteration whose counter
    a repetition loop's bounds read stops nothing: the hoist takes it for a
    repetition loop of one iteration and moves it out with its readers
    (hoistRepetitionLoops' dimension rule since the fourth round of the
    review of 2026-10-19; it took it for a dimension before, and refused
    such nests when a repetition loop had to be moved out of the GEMM
    loops)."""
    lead = _leading(case, rep)
    if any(not any(lp.bound_vars() & set(lead[:p]) for lp in case.loops if lp.var not in lead[:p])
           and _normalizable(_without(case, lead[:p])) for p in range(1, len(lead) + 1)):
        return True
    return all(lp.is_box() for lp in case.loops if lp.var not in rep)


def _normalized_view(case: "Case"):
    """The nest as the compiler's normalization leaves it when the nest as a
    whole is no parallelotope (normalizeParallelotopeDomains' prefix retry):
    the band below the shortest prefix of loops no access reads whose
    counters nothing below it reads, rewritten onto its box when it is a
    parallelotope as written or an exact one (box_form), the prefix as
    written. None when no such prefix leaves a band to rewrite (the first
    one's band a box, or none a parallelotope). Its repetition loops may be
    independent where the nest's are not: s in [i, i + 2) below a repetition
    triangle reads the GEMM counter i as written and nothing on the box
    (review of 2026-10-19, round 4: such nests were lowered, and the
    specification called them OUT_OF_SCOPE)."""
    import copy
    names, used = _loop_vars(case), _used_vars(case)
    lead = _leading(case, set(names) - used)
    for p in range(1, len(lead) + 1):
        pre = set(lead[:p])
        if any(lp.bound_vars() & pre for lp in case.loops if lp.var not in pre):
            continue
        sub = _without(case, lead[:p])
        if is_box_case(sub):
            return None
        form = sub if shifts_together(sub) else exact_form(sub)
        if form is None:
            continue
        box = box_form(form)
        view = copy.deepcopy(case)
        view.loops = [copy.deepcopy(lp) for lp in case.loops if lp.var in pre] + box.loops
        view.arrays = box.arrays
        view.analysis, view.expect, view.spec, view.classification = {}, {}, {}, ""
        return view
    return None


def repetition_view(case: "Case"):
    """(structure, view): the documented repetition family's structure --
    (shape, repetition vars) or None -- and the case it holds on: the nest as
    written, or its _normalized_view when only that has independent
    repetition loops (the compiler hoists on the normalized band)."""
    structure = repetition_structure(case)
    if structure is not None:
        return (structure if _in_family_r(case, structure[1]) else None), case
    view = _normalized_view(case)
    if view is not None:
        structure = repetition_structure(view)
        if structure is not None and _in_family_r(view, structure[1]):
            return structure, view
    return None, case


def repetition_family(case: "Case"):
    """The documented repetition family (docs section 19.13), the
    specification's: the loops no access reads, independent of the others
    (on the nest as written or as the normalization leaves it,
    repetition_view) and in family R (_in_family_r). (shape, repetition
    vars) or None."""
    return repetition_view(case)[0]


def _compiler_dims(case: "Case"):
    """The loops the compiler reads as iteration dimensions of the GEMM: those
    an access reads, and a loop with constant bounds and one iteration (a
    constant on the domain, Lemma 0: recognizeLogicalMatmul's
    singleIterationValue, and hoistRepetitionLoops since 2026-10-19 -- a
    one-iteration loop whose bounds read a counter is a repetition loop for
    both) unless it is in the leading run of loops no access reads (the
    sub-band retry and the inner-first candidates skip those): the
    recognizer's view of a nest left as written. The hoist differs on a
    loop of one iteration whose counter a loop's bounds read (_hoist_dims)."""
    used = _used_vars(case)
    lead = set(_leading(case, set(_loop_vars(case)) - used))
    return used | {lp.var for lp in case.loops if lp.is_box() and lp.trip() == 1 and lp.var not in lead}


def _hoist_dims(case: "Case"):
    """The loops hoistRepetitionLoops takes for GEMM dimensions: those of
    _compiler_dims but a loop of one iteration whose counter a loop's bounds
    read, which it takes for a repetition loop of one iteration and moves out
    with its readers (review of 2026-10-19, round 4)."""
    read = set().union(*(lp.bound_vars() for lp in case.loops))
    once = {lp.var for lp in case.loops if lp.is_box() and lp.trip() == 1}
    return _compiler_dims(case) - ((once & read) - _used_vars(case))


def _predictor_family(case: "Case"):
    """The repetition structure as the compiler finds it (review 2026-10-18
    #22): its repetition loops are those that are no dimension for the hoist
    (_hoist_dims) -- a one-iteration loop with constant bounds that no access
    reads is a dimension unless it leads the nest or a loop's bounds read its
    counter (then it is a repetition loop of one iteration, moved out with
    its readers).
    (family structure or None, structure or None): the first when the
    compiler normalizes or hoists the nest (family R), the second whenever
    the repetition loops are independent."""
    dims = _hoist_dims(case)
    structure = _independent(case, [v for v in _loop_vars(case) if v not in dims])
    if structure is None:
        return None, None
    return (structure if _in_family_r(case, structure[1]) else None), structure


def _predictor_view(case: "Case"):
    """(family structure, structure, view): _predictor_family on the nest as
    written, or on its _normalized_view when only that has independent
    repetition loops, and the case they hold on."""
    structure, pstructure = _predictor_family(case)
    if pstructure is not None:
        return structure, pstructure, case
    view = _normalized_view(case)
    if view is not None:
        structure, pstructure = _predictor_family(view)
        if pstructure is not None:
            return structure, pstructure, view
    return None, None, case


def _surrogate(case: "Case", rep):
    """The case with its repetition loops as plain loops of two iterations at
    the same places (they repeat the GEMM; the compiler's hoist treats a
    repetition loop whose bounds do not fix its trip count -- a triangle --
    as one that may run twice) -- but a loop with constant bounds and one
    iteration as written: it repeats nothing (a constant on the domain, Lemma
    0; review 2026-10-19: given two iterations inside the reduction it made a
    GEMM repeat_in_reduction), and a loop no access reads whose bounds shift
    together and fix its trip count at 0 or 1 as one of one iteration: it
    cannot run twice, which is all the hoist asks of it (fixedTripCount,
    mayRunTwice; review 2026-10-19: k in [r1, r1 + 1) inside the reduction
    loop is moved out without a test). The count of repetitions is taken from
    the case, not from the surrogate."""
    import copy
    used = _used_vars(case)
    sur = copy.deepcopy(case)
    for lp in sur.loops:
        if lp.var in rep and not (lp.is_box() and lp.trip() == 1):
            n = 1 if lp.var not in used and lp.shifts_together() and lp.trip() <= 1 else 2
            lp.lb, lp.ub, lp.step, lp.cmp, lp.inc, lp.cond = 0, n, 1, "<", "++", ""
            lp.lb_terms, lp.ub_terms = {}, {}
    sur.analysis, sur.expect, sur.spec, sur.classification = {}, {}, {}, ""
    return sur


def centrally_symmetric(case: "Case"):
    """Whether the iteration set (all loops, enumerated) is centrally
    symmetric: equal to its reflection about the midpoint of its
    lexicographic extremes (docs section 19.7). None when empty."""
    names = _loop_vars(case)
    pts = {tuple(env[v] for v in names) for env in domain_points(case.loops)}
    if not pts:
        return None
    lo, hi = min(pts), max(pts)
    c2 = tuple(a + b for a, b in zip(lo, hi))
    return all(tuple(c - x for c, x in zip(c2, p)) in pts for p in pts)


def stacked_access_rank(case: "Case") -> int:
    """The rank of the stacked access map x -> (F_A(x), F_B(x), F_C(x)): every
    subscript of the two factors and of C as a row of its coefficients over
    ALL the nest's loops (one that runs once included), exactly (Gaussian
    elimination in fractions.Fraction). The nest's loop count when the
    accesses tell every two iterations apart -- a loop no access reads is a
    zero column; strip-mined loops read only through k = 2 k1 + k0 are two
    proportional columns (docs/AFFINE_GEMM_MAPPING.md section 19.13.4)."""
    from fractions import Fraction
    names = _loop_vars(case)
    rows = [[Fraction(f.coef.get(v, 0)) for v in names]
            for r in ("a", "b", "c") for f in case.arrays[r].forms()]
    rank = 0
    for col in range(len(names)):
        pivot = next((r for r in range(rank, len(rows)) if rows[r][col] != 0), None)
        if pivot is None:
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        for r in range(len(rows)):
            if r != rank and rows[r][col] != 0:
                f = rows[r][col] / rows[rank][col]
                rows[r] = [x - f * y for x, y in zip(rows[r], rows[rank])]
        rank += 1
    return rank


INT64_MIN, INT64_MAX = -(1 << 63), (1 << 63) - 1


def written_bounds_fit_int64(case: "Case") -> bool:
    """Whether every product (coefficient x counter) and every result of every
    loop's bounds as the front end emits them -- the lower bound and the
    exclusive upper bound, linear forms of the enclosing counters -- has its
    exact value within 64 bits at every point of the outer loops that enters
    the loop (the outer loops' own iteration set: points at which an inner
    loop is empty included). The compiler takes the exact iteration set as
    the set the program runs only then (exactIterationSet,
    writtenBoundsFitInt64: lower-affine's products are nsw, poison on
    overflow; docs section 19.13.2). The front end checks every operation of
    a bound as written in its C type (section 19.13.1), but the bounds it
    emits can be other expressions (a canonicalized condition: k - i < 4
    becomes k < i + 4), so this is checked, not assumed."""
    for s, lp in enumerate(case.loops):
        if lp.is_box():
            if not (INT64_MIN <= lp.lb <= INT64_MAX and INT64_MIN <= lp.exclusive_ub() <= INT64_MAX):
                return False
            continue
        for env in domain_points(case.loops[:s]):
            values = [c * env[v] for terms in (lp.lb_terms, lp.ub_terms) for v, c in terms.items()]
            values += [lp.lb_at(env), lp._exclusive(lp.ub_at(env))]
            if any(v < INT64_MIN or v > INT64_MAX for v in values):
                return False
    return True


def disproof_holds(case: "Case") -> bool:
    """The compiler's proof that a nest left as written is no GEMM of affine
    views in any decomposition (disproveAffineImageOfBox, docs section
    19.13.4; review 2026-10-18 #1, #9, #10-#12, #26, #35), for a C nest that
    is the whole candidate band. All of:
      (1) every bound a linear form of the nest's outer counters -- true of
          every translated C nest (the front end emits bounds that are
          constants or affine in the enclosing counters, rounds a
          condition with |a| > 1 to a constant, never a floordiv / mod or a
          symbol), so not tested here;
      (2) no stepped loop (step != 1 as emitted: a loop counting up -- one
          counting down becomes an ordinal loop of step 1) whose lower bound
          reads a counter;
      (3) the written bounds' products and results within 64 bits at every
          outer point entering their loop (written_bounds_fit_int64);
      (4) the stacked access map of rank n: the accesses tell the
          iterations apart (stacked_access_rank) -- with a lower rank an
          asymmetric set can map one to one onto a box (strip-mined loops
          with a remainder: a GEMM); a loop no access reads fails it;
      (5) the two factors and C three different arrays (two views of one
          array may exchange their roles from point to point: the product
          commutes);
      (6) the iteration set nonempty and not centrally symmetric about the
          midpoint of its lexicographic extremes (centrally_symmetric)."""
    if any(lp.step > 1 and lp.lb_terms for lp in case.loops):
        return False
    if len({case.arrays[r].name for r in ("a", "b", "c")}) != 3:
        return False
    if stacked_access_rank(case) != len(case.loops):
        return False
    if centrally_symmetric(case) is not False:
        return False
    return written_bounds_fit_int64(case)


def analysis_case(case: "Case"):
    """The box case a nest is analysed on (docs sections 19.12, 19.13): the
    case itself when it is a box; its box form for a parallelotope as
    written or an exact one; for the repetition family, that of its
    surrogate (the repetition loops as plain ones -- the repetition count is
    the analysis's own). None for a domain outside these."""
    if is_box_case(case):
        return case
    if shifts_together(case):
        return box_form(case)
    form = exact_form(case)
    if form is not None:
        return box_form(form)
    structure, view = repetition_view(case)
    if structure is not None:
        return analysis_case(_surrogate(view, structure[1]))
    return None


def repetition_points(case: "Case", rep) -> int:
    """The number of points of the repetition loops `rep` alone (independent
    of the others: their bounds read only each other)."""
    return len(domain_points([lp for lp in case.loops if lp.var in rep]))


def _aliases_output(case: "Case") -> bool:
    """A factor that may alias C: the same array (A[i][j] += A[i][k] *
    B[k][j]: the writes to C are reads of A in program order), or neither
    side restrict-qualified."""
    return (case.c.name in (case.a.name, case.b.name)
            or not (case.a.restrict or case.c.restrict) or not (case.b.restrict or case.c.restrict))


def _alias_changes_result(case: "Case") -> bool:
    """With the output array also a factor: whether the nest's result on its
    data differs from the GEMM of the factors' initial values. A cross-check
    of _alias_reads_written (a difference needs a read of a written element;
    the converse depends on the data: a product that is zero leaves the
    element as it was)."""
    return evaluate(case)["c"] != evaluate(case, snapshot=True)["c"]


def _alias_reads_written(case: "Case") -> bool:
    """With the output array also a factor: whether some product reads an
    element of that array written at an earlier point of the nest, in program
    order -- then the nest is not the GEMM of its operands' initial values,
    whatever the data. Decided by the addresses alone."""
    shared = [r for r in ("a", "b") if case.arrays[r].name == case.c.name]
    if not shared:
        return False
    forms = {r: case.arrays[r].forms() for r in ("a", "b", "c")}
    dims = case.c.dims
    written = set()
    for env in domain_points(case.loops):
        for r in shared:
            if flat_index(dims, [f.eval(env) for f in forms[r]]) in written:
                return True
        written.add(flat_index(dims, [f.eval(env) for f in forms["c"]]))
    return False


def _alias_verdict(case: "Case"):
    """The specification on a factor that may alias C: REJECT when the output
    is a factor and a product reads an element written earlier (not the GEMM
    of the operands, by the addresses), OUT_OF_SCOPE (a valid GEMM the
    compiler refuses conservatively) otherwise."""
    if case.c.name in (case.a.name, case.b.name):
        structural = _alias_reads_written(case)
        if _alias_changes_result(case) and not structural:
            raise ValueError("%s: the result differs from the GEMM of the initial values, yet no "
                             "product reads an element written earlier" % case.id)
        if structural:
            return ("REJECT", "aliasing_not_gemm")
    return ("OUT_OF_SCOPE", "aliasing")


# --------------------------------------------------------------------------
# the Python oracle: evaluate the kernel's statement exactly, in integers
# --------------------------------------------------------------------------


def c_written_once(case: Case) -> bool:
    """An assignment C = A * B is the K = 1 GEMM into a zeroed C only when
    every element of C is written exactly once: C's address is injective on
    the iteration domain (docs/AFFINE_GEMM_MAPPING.md section 20). Decided by
    enumeration (the loops are small). A loop no access uses rewrites the same
    value (the hoist rules decide it, as for an accumulation): the writes are
    keyed by its counter. True when a subscript leaves its array (the
    out-of-bounds rules decide that case) or is not affine."""
    try:
        forms = {role: arr.forms() for role, arr in case.arrays.items()}
    except NotAffine:
        return True
    used = set()
    for fl in forms.values():
        for f in fl:
            used |= f.vars()
    rep_vars = [lp.var for lp in case.loops if lp.var not in used]
    cforms, dims = forms["c"], case.c.dims
    seen, env, loops = set(), {}, case.loops
    dup = [False]

    def rec(depth):
        if dup[0]:
            return
        if depth == len(loops):
            idx = [f.eval(env) for f in cforms]
            # the leading dimension of a C array parameter is dynamic in the
            # memref (the program's guarantee, docs section 19.11 G3): an
            # overrun of it is still a distinct address, only the inner
            # dimensions bound the index
            for pos, (d, i) in enumerate(zip(dims, idx)):
                if i < 0 or (pos >= 1 and i >= d):
                    raise IndexError("out of bounds")
            key = (flat_index(dims, idx), tuple(env[v] for v in rep_vars))
            if key in seen:
                dup[0] = True
            seen.add(key)
            return
        lp = loops[depth]
        for v in lp.values(env):
            env[lp.var] = v
            rec(depth + 1)

    try:
        rec(0)
    except IndexError:
        return True
    return not dup[0]


def mac_like(case: Case) -> bool:
    """A statement the GEMM decisions apply to: a multiply-accumulate form, or
    the assignment C = A * B that writes every element of C once (the K = 1
    GEMM into a zeroed C, docs/AFFINE_GEMM_MAPPING.md section 20)."""
    return case.stmt in MAC_STMTS or (case.stmt == "=AB" and c_written_once(case))


def evaluate(case: Case, snapshot: bool = False):
    """Return {'c0': C before, 'c': C after, <name>: flat int arrays},
    evaluating the statement over the loop domain exactly as the C kernel
    does (in exact integer arithmetic, which is what makes the float results
    exact).  Raises IndexError on an out-of-bounds subscript. With
    `snapshot`, the factors are read from copies of their initial values (the
    GEMM of the operands as they were), which differs from the run as written
    only when the output array is also a factor."""
    data = {}
    for name in case.array_names():
        arr = case.arrays[case.role_of_name(name)]
        nz = factor_nonzero(case, name)
        data[name] = [elem_value(e, array_seed(case, name), nz) for e in range(volume(arr.dims))]
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
    if snapshot:
        A, B = list(A), list(B)
    D = data[case.arrays["d"].name] if "d" in case.arrays else None
    stmt = case.stmt
    alpha = int(case.alpha) if case.alpha is not None else None
    env = {}
    loops = case.loops
    zero_before_inner = case.kernel_style in ("init_inside", "scalar_acc")
    peak = [0]      # the largest |product| and |partial sum| met (float exactness, 2^24)

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
            peak[0] = max(peak[0], abs(p), abs(C[ic]))
            return
        lp = loops[depth]
        for v in lp.values(env):
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
    out["peak"] = peak[0]
    return out


# --------------------------------------------------------------------------
# mathematical analysis: is the nest a (dense) GEMM, of what kind?
# --------------------------------------------------------------------------


def analyse(case: Case) -> dict:
    """Classify the nest from its subscripts and loop domain.  Returns a dict
    with 'math_class' and the supporting facts, and sets case.analysis."""
    if not is_box_case(case):
        return _analyse_domain(case)
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
    # An assignment C = A * B is the K = 1 GEMM into a zeroed C when every
    # element of C is written exactly once (docs/AFFINE_GEMM_MAPPING.md
    # section 20, 2026-10-19); the reduction then has extent one (no K loop of
    # two or more iterations, since C does not read K).
    if case.stmt == "=AB" and not c_written_once(case):
        dense = False
        reasons.append("the statement overwrites C (C = A * B, no accumulation) and some element "
                       "of C is written more than once: only the last product survives, not the "
                       "GEMM of A and B (docs/AFFINE_GEMM_MAPPING.md section 20)")
    # A reduction no loop enumerates has extent one (SQ-4, decided
    # 2026-10-15: a role of extent one needs no loop; the K = 1 requirement of
    # 2026-10-18 -- C[i][j] += A[i][0] * B[0][j] is the K = 1 GEMM): A and B
    # read it through a constant subscript, which stands for it below. A row
    # or column coordinate without a loop is left to the enumeration on the
    # domain (spec_expect: dense_on_domain, generalized_views), as before.
    absent = ["K"] if "K" not in groups and "I" in groups and "J" in groups else []
    role_order = {"a": ("I", "K"), "b": ("K", "J"), "c": ("I", "J")}
    for role in ("a", "b", "c"):
        gl = [subscript_group(f) for f in forms[role]]
        want = set(expected_groups[role])
        missing = [g for g in role_order[role] if g in absent]
        have = [g for g in gl if g not in ("P", "const")]
        consts = [pos for pos, g in enumerate(gl) if g == "const"]
        if any(g.startswith("mixed") for g in gl):
            dense = False
            reasons.append("%s has a subscript that couples loops of different "
                           "coordinates (%s)" % (case.arrays[role].name, ", ".join(gl)))
        elif sorted(have) != sorted(want - set(missing)) or len(consts) != len(missing):
            dense = False
            reasons.append("%s is indexed by %s; the operand in that position of a GEMM "
                           "is indexed by exactly %s" % (case.arrays[role].name, gl, sorted(want)))
        else:
            for pos, g in zip(consts, missing):
                gl[pos] = g
        operand_groups[role] = gl
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
    for g in ("I", "J"):
        if g not in groups and dense:
            dense = False
            reasons.append("no loop enumerates coordinate %s" % g)
    # (a reduction without a loop is one of extent one when A and B read it
    # through a constant: handled above)
    if dense and case.c.name in (case.a.name, case.b.name):
        # the products may read elements the nest wrote: not the GEMM of the
        # operands as they were (spec_expect decides by evaluation whether
        # the result differs: _alias_verdict)
        dense = False
        reasons.append("the output array %s is also a factor: the products can read elements "
                       "written earlier in the nest" % case.c.name)

    views = {}
    if dense:
        def ordinals(g):
            ls = groups.get(g, [])
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
        extents = {g: volume([lp.trip() for lp in groups.get(g, [])]) for g in ("I", "J", "K")}
        result["extents"] = extents
        result["views"] = views
        tags = []
        if p_loops:
            tags.append("batched")
        if absent:
            tags.append("extent_one_without_loop:" + "".join(absent))
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
        if case.stmt == "=AB":
            tags.append("assignment_zero_c")
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


def _c_only_loops(forms, loops):
    """The loops of two or more iterations read by C only (domain forms): the
    compiler demotes them to batch loops before any analysis (both factors
    shared by the partitions, docs/AFFINE_GEMM_MAPPING.md section 21)."""
    dom = {r: [_domain_form(f, loops) for f in forms[r]] for r in ("a", "b", "c")}
    trip = {lp.var: lp.trip() for lp in loops}
    read_by = {}
    for r in ("a", "b", "c"):
        for f in dom[r]:
            for v, c in f.coef.items():
                if c != 0 and trip.get(v, 2) != 1:
                    read_by.setdefault(v, set()).add(r)
    return {v for v, rs in read_by.items() if rs == {"c"}}


def _choose_demotion(forms, loops, candidate):
    """The compiler's chooseDemotion (AffineMatmulAnalysis.cpp, docs section
    21): among the loops of a row or column role that is not one dense
    coordinate, the loops joined by a subscript form components; the
    component of the largest extent (the product of its trip counts; ties:
    the one read in C's innermost dimension, then the first loop of the nest)
    stays the role, the others are demoted; a single component keeps its
    loop of the largest extent (same ties).  Returns the set of demoted
    loop variables."""
    order = {lp.var: i for i, lp in enumerate(loops)}
    trip = {lp.var: lp.trip() for lp in loops}
    cand = sorted(candidate, key=order.get)
    dom = {r: [_domain_form(f, loops) for f in forms[r]] for r in ("a", "b", "c")}
    parent = {v: v for v in cand}

    def find(v):
        while parent[v] != v:
            parent[v] = parent[parent[v]]
            v = parent[v]
        return v
    for r in ("a", "b", "c"):
        for f in dom[r]:
            here = [v for v in cand if f.coef.get(v, 0) != 0]
            for v in here[1:]:
                parent[find(v)] = find(here[0])
    c_dim = {v: max([d for d, f in enumerate(dom["c"]) if f.coef.get(v, 0) != 0] or [-1])
             for v in cand}
    components = {}
    for v in cand:
        components.setdefault(find(v), []).append(v)
    comps = list(components.values())

    def key(vs):
        ext = 1
        for v in vs:
            ext *= trip[v]
        return (-ext, -max(c_dim[v] for v in vs), min(order[v] for v in vs))
    keep = min(comps, key=key)
    if len(comps) > 1:
        return {v for vs in comps if vs is not keep for v in vs}
    best = min(keep, key=lambda v: key([v]))
    return {v for v in keep if v != best}


def predict_by_groups(case, forms, loops, demoted=frozenset(), out=None):
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
    batch_loops = {v for v, rs in read_by.items() if rs == {"a", "b", "c"}} | set(demoted)
    index = {lp.var: i for i, lp in enumerate(loops)}
    # A demoted loop (section 21) is read by C alone in a dimension of its
    # own: the partitions' outputs are independent slabs of C.
    for d, f in enumerate(dom["c"]):
        sup = support(f)
        dem = [v for v in sup if v in demoted]
        if dem and any(v not in batch_loops for v in sup):
            return None, ("loop %d would partition the GEMM (dimension-intersection partitioning, "
                          "docs/AFFINE_GEMM_MAPPING.md section 21), but C reads it in dimension %d "
                          "together with a role coordinate: the partitions' outputs would not be "
                          "independent slabs of C (overlapping, or one merged dimension) -- not "
                          "constructed" % (min(index[v] for v in dem), d)), False
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
    roots_of = {}
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
        roots_of[id(g)] = root
    # A batch coordinate (read by all three): lowered with the batch loops
    # around the tiles (ISSUE-006) when the three operands read it through
    # one and the same expression on the domain.
    if batch_groups and not (groups_first or groups_second or reduction):
        return None, ("%d coordinate(s) are read by all three operands and none by two: an "
                      "element-wise product" % len(batch_groups)), False
    batch_vars = set(batch_loops)
    read_vars = {v for v in read_by if v not in batch_loops}
    for groups in (groups_first, groups_second, reduction):
        if len(groups) > 1:
            # Several coordinates in a row or column role: all but one of
            # them partition the GEMM (section 21) -- the retry in
            # groups_or_readers demotes them; a second reduction coordinate
            # is a partial-C reduction, not constructed.
            if groups is not reduction and out is not None:
                roots = {roots_of[id(g)] for g in groups}
                out["candidate"] = {v for v in read_vars if find(v) in roots}
            return None, ("%d separate coordinates are read by one GEMM role%s"
                          % (len(groups), " (requires a partial-C reduction)"
                             if groups is reduction else "")), False
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
        # a row or column role whose group has several loops must be one
        # dense coordinate of them (a mixed radix); else all but one of its
        # loops partition the GEMM (section 21) -- the retry demotes them
        if role in "IJ" and all(lp.trip() != 0 for lp in loops):
            root = roots_of[id(g)]
            group_vars = [lp.var for lp in loops if lp.var in read_vars and find(lp.var) == root]
            if len(group_vars) > 1:
                ir_step = {lp.var: (1 if lp.step < 0 else lp.step) for lp in loops}
                ir_coef = {lp.var: (lp.step if lp.step < 0 else 1) for lp in loops}
                magnitudes = [c.coef.get(v, 0) * ir_coef[v] * ir_step[v] for v in group_vars]
                scale = 0
                for mag in magnitudes:
                    scale = math.gcd(scale, abs(mag))
                extent = (_mixed_radix_extent([mag // scale for mag in magnitudes],
                                              [trip[v] for v in group_vars]) if scale else None)
                if extent is None:
                    if out is not None:
                        out["candidate"] = set(group_vars)
                    return None, ("logical coordinate %s is driven by %d loops that do not "
                                  "enumerate it densely (or its loops' bounds are not constant): "
                                  "no box of %s values" % (role, len(group_vars), role)), False
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


def predict_by_readers(case, forms, loops, demoted=frozenset(), out=None):
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
        if lp.var in demoted:
            batch.add(lp.var)      # a demoted loop is a batch loop whatever its readers
        elif rs == set(ops):
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
    # A demoted loop (section 21) is read by C alone in a dimension of its
    # own: the partitions' outputs are independent slabs of C.
    index = {lp.var: i for i, lp in enumerate(loops)}
    for d, f in enumerate(dom["c"]):
        dem = [v for v, c in f.coef.items() if c != 0 and v in demoted]
        if dem and any(c != 0 and v not in batch for v, c in f.coef.items()):
            return None, ("loop %d would partition the GEMM (dimension-intersection partitioning, "
                          "docs/AFFINE_GEMM_MAPPING.md section 21), but C reads it in dimension %d "
                          "together with a role coordinate: the partitions' outputs would not be "
                          "independent slabs of C (overlapping, or one merged dimension) -- not "
                          "constructed" % (min(index[v] for v in dem), d))
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
                # a row or column role over several coordinates: all but one
                # of them partition the GEMM (section 21) -- the retry demotes
                if role in "IJ" and out is not None:
                    out["candidate"] = {lp.var for lp in lps}
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
        pick = next(i for i, x in enumerate(rest) if any(x))
        g = _primitive(rest[pick])
        lead = next(i for i, x in enumerate(g) if x != 0)
        direction[role] = g
        # the coordinate returned to predict() in COUNTER space (the loops'
        # own counters, as every other path's): the primitive direction of
        # the same subscript's restriction to the role's loops -- the dense
        # check multiplies a coefficient by the loop's step itself, so a
        # coefficient already on the ordinal counter would fold a count-down
        # loop's step twice (review of 2026-10-18)
        counter = _primitive([subs[pick][2].coef.get(lp.var, 0) for lp in lps])
        coords[role] = LinearForm({lp.var: c for lp, c in zip(lps, counter)}, 0)
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
                if role in "IJ" and len(lps) > 1 and out is not None:
                    out["candidate"] = {lp.var for lp in lps}
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
    (AffineMatmulAnalysis.cpp, groupsOrReaders), with the dimension-
    intersection partitioning of docs section 21: the loops read by C only
    are batch loops from the start, and when an analysis refuses a row or
    column role as not one dense coordinate, the loops _choose_demotion
    names become batch loops and the analyses run again (at most once per
    loop).  Returns (result, None) or (None, reason); the result's batch
    variables include the demoted loops."""
    demoted = set(_c_only_loops(forms, loops)) if is_box_case(case) else set()
    why = None
    for _ in range(len(loops) + 1):
        out = {}
        grouped, why, relaxable = predict_by_groups(case, forms, loops, demoted, out)
        if grouped is None and relaxable:
            grouped, why = predict_by_readers(case, forms, loops, demoted, out)
        if grouped is not None:
            return grouped, None
        candidate = out.get("candidate")
        if not candidate or not is_box_case(case):
            return None, why
        chosen = _choose_demotion(forms, loops, candidate)
        if not chosen:
            return None, why
        demoted |= chosen
    return None, why


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
    if not is_box_case(case):
        return _predict_domain(case)
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
        # An assignment (docs section 20): the recognizer refuses a loop of two
        # or more iterations that no subscript of C reads (the last product
        # wins); the remaining overwrites -- batches overlapping in C -- are
        # refused by the lowering's injectivity proof (below).
        c_vars = set()
        for f in sforms["c"]:
            c_vars |= f.vars()
        if any(lp.trip() >= 2 and lp.var in used_vars and lp.var not in c_vars
               for lp in loops):
            ex["recognition"] = "reject"
            return stop("recognition", "the statement overwrites C (C = A * B, no accumulation) "
                        "and a loop of the nest, which runs more than once, is not a subscript "
                        "of C: only the last product survives")
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
    # A loop that runs once is an iteration dimension whether or not an
    # access names it (a constant on the domain, Lemma 0; the recognizer and
    # hoistRepetitionLoops read it so since 2026-10-18): an all-constant nest
    # of such loops is the 1 x 1 x 1 GEMM (SQ-4). A loop of two or more
    # iterations no access uses repeats the accumulation.
    once_vars = {lp.var for lp in loops if lp.trip() == 1}
    if not (used_vars | once_vars):
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
    used_idx = [i for i, lp in enumerate(loops) if lp.var in used_vars or lp.trip() == 1]
    rep_idx = [i for i, lp in enumerate(loops) if i not in used_idx]
    # The hoist's dependence test sees one memref at a time, so it moves a
    # repetition loop outward only when every other memref of the body is
    # provably disjoint from the one it stores to (provablyDisjointFromOutput:
    # one of the two a restrict argument), and tests that first (review
    # 2026-10-18 #3: a hoisted nest later refused for 'A may overlap C' ran
    # its updates permuted on the host). Both tests are needed only when the
    # interchange reorders iterations -- a repetition loop of two or more
    # iterations inside a dimension of two or more; past loops that run once
    # it moves without them (review 2026-10-19: a leading one-iteration loop).
    reorders = any(loops[ri].trip() >= 2 and any(si < ri and loops[si].trip() >= 2 for si in used_idx)
                   for ri in rep_idx)
    if reorders and any(
            arr.name != case.c.name and not (arr.restrict or case.c.restrict) for arr in (case.a, case.b)):
        ex["recognition"] = "reject"
        return stop("recognition", "loop %d of the nest repeats the accumulation inside it, and moving it "
                    "outward is a legal loop interchange only if the memrefs the nest reads do not "
                    "overlap the one it writes; declare the arguments llvm.noalias (C restrict)"
                    % rep_idx[-1])
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
                # 19.11): an explicit memref.reinterpret_cast view, every
                # dimension's index range checked in memref order
                dims = list(range(len(forms[r])))
            else:
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
        # (one array as both factors: since 2026-10-18 two different views
        # of one memref are two read-only operand views, lowered like any
        # other -- docs/AFFINE_GEMM_MAPPING.md section 7, 19.12)
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
    # (one array as both factors: two different views of one memref are two
    # read-only operand views since 2026-10-18, docs section 7 / 19.12)
    # An assignment whose batches overlap in C (C injective per batch, not on
    # the whole domain): the lowering's injectivity proof refuses it
    # (verifyOutputWrittenOnce, docs section 20).
    if case.stmt == "=AB" and not c_written_once(case):
        ex["lowering"] = "unsupported"
        return stop("lowering", "two iterations of the assignment C = A * B write the same "
                    "element of C (its subscripts are not injective on the iteration domain): "
                    "the later product overwrites the earlier one")
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
            "value): unreachable from the C front end, whose bounds are constants or affine in the "
            "enclosing counters, never a runtime value; is the MLIR "
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
    "generalized_views": "(since 2026-10-16, docs/AFFINE_GEMM_MAPPING.md section 19; lowered since "
                         "2026-10-18, section 19.11: one memref.reinterpret_cast per operand) a GEMM, or a "
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
                      "C[b] += A[b] B) whose partitioned decomposition is not valid (the batch "
                      "coordinate is not read by C alone in a dimension of its own, or the "
                      "per-partition GEMM is not one dense GEMM): not lowered",
    "dimension_partition": "(supported since 2026-10-19, docs/AFFINE_GEMM_MAPPING.md section 21) "
                           "the dimension-intersection partitioning: the loops read by C only, and "
                           "the extra coordinates of a row or column role (the loops of C & A beyond "
                           "M, of C & B beyond N, chosen by the rule of section 21), are batch "
                           "loops; each partitions the GEMM into independent slabs of C and the "
                           "factor that does not read it is shared by the partitions; the "
                           "per-partition GEMM is one dense GEMM of affine views "
                           "(casespec.partition_decomposition, by enumeration)",
    "partial_reduction": "(docs/AFFINE_GEMM_MAPPING.md section 21) a loop read by both factors and "
                         "not by C beyond the reduction coordinate (C[i][j] += A[r][i][k] * "
                         "B[r][k][j]): the only valid decomposition makes it a batch loop whose "
                         "iterations all accumulate into one slab of C -- a K-fold summed into C, "
                         "a partial-C reduction, which the compiler does not construct (not an "
                         "independent output partition): a documented limitation",
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
    "no_gemm_loop": "every loop of the nest is a repetition loop of two or more iterations -- no access "
                    "names a loop counter and no loop runs once --, so the body is one scalar "
                    "multiply-accumulate repeated and the nest has no loop of a GEMM to anchor on: "
                    "the recognizer decides nothing (UNKNOWN, 'loop 0 of the nest is used by no "
                    "access'); the nest of loops that run once is the 1 x 1 x 1 GEMM (SQ-4) and is "
                    "offloaded, a repetition loop around a GEMM loop re-runs the tile (README section 2; "
                    "a documented limitation, 2026-10-18)",
    "same_memref_views": "(no longer a limitation since 2026-10-18, docs/AFFINE_GEMM_MAPPING.md "
                         "sections 7 and 19.12) one array as both factors through two different "
                         "views -- A . A^T, a skew read by one factor only -- is lowered with two "
                         "read-only operand views of the shared base; one view in some orientation "
                         "of the coordinates is still served by one tensor",
    "matvec": "(no longer a limitation since 2026-10-15, P0) a matrix-vector product is the GEMM "
              "with N = 1: its rank-1 operands get a unit dimension (memref.expand_shape), "
              "docs/AFFINE_GEMM_MAPPING.md section 2",
    "not_gemm": "the nest is not a dense GEMM of its operands (analyse(): see why_not_a_dense_gemm)",
    "not_gemm_overwrite": "the assignment C = A * B writes some element of C more than once (a "
                          "reduction loop of two or more iterations, batches overlapping in C): "
                          "only the last product survives, not the GEMM of A and B -- an "
                          "assignment is the K = 1 GEMM into a zeroed C only when every element "
                          "is written once (docs/AFFINE_GEMM_MAPPING.md section 20, 2026-10-19)",
    "assignment": "(supported since 2026-10-19) the assignment C = A * B whose every element of C "
                  "is written exactly once is the K = 1 GEMM into a zeroed C: the lowering zeroes "
                  "C's logical window and the tiles accumulate into it "
                  "(docs/AFFINE_GEMM_MAPPING.md section 20)",
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
    "aliasing_not_gemm": "the output array is also a factor and a product reads an element of it "
                         "written at an earlier point of the nest (A[i][j] += A[i][k] * B[k][j]: "
                         "A[i][j], once written, is read as A[i][k'] with k' = j later on), so the "
                         "nest is not the GEMM of the factors' initial values -- decided by the "
                         "addresses in program order (casespec._alias_reads_written), cross-checked "
                         "by evaluating the nest against a run that reads the factors' initial values; "
                         "the compiler refuses the aliasing before that ('A may overlap C', docs "
                         "section 7)",
    "non_box_unenumerated": "a loop nest whose bounds do not shift together, outside the normalized "
                            "and the repetition families, whose iteration set exceeds the enumeration "
                            "limit of the executable specification (200,000 points): not decided here "
                            "(the compiler does not normalize such a set; it proves it no GEMM only when "
                            "its proof's conditions hold -- the accesses tell the iterations apart, the "
                            "factors and C are three different arrays, no stepped loop's lower bound "
                            "reads a counter, the written bounds stay within 64 bits and the set is not "
                            "centrally symmetric --, else UNKNOWN: docs/AFFINE_GEMM_MAPPING.md section "
                            "19.13.4)",
    "non_box_domain": "(docs/AFFINE_GEMM_MAPPING.md sections 19.7, 19.12, 19.13) a loop nest whose "
                      "bounds do not shift together (a triangle, k in [i, 8)) and whose integer points "
                      "are no parallelotope with the bounds' shift rows, outside the repetition "
                      "family: enumerated as written (casespec._spec_domain), the readers' roles do not "
                      "map its points onto a box, and it is provably no GEMM of affine views in any "
                      "decomposition -- its updates (F_A(x), F_B(x), F_C(x)), counted over the iteration "
                      "set, are not centrally symmetric (with one array as both factors: (F_C(x), F_A(x) "
                      "+ F_B(x)), the product commutes), while a GEMM's are an affine image of a box "
                      "(casespec._no_gemm_proof); the C "
                      "front end translates it (since 2026-10-18) and the compiler refuses it: "
                      "PROVEN_INVALID when the accesses tell the iterations apart (the stacked access "
                      "map has rank n, the number of loops of the compiler's candidate band: the nest "
                      "below a leading run of loops no access reads whose counters no loop below the "
                      "run reads, their bounds may read each other), "
                      "the factors and C are three different arrays, no stepped loop's "
                      "lower bound reads a counter, every written bound stays within 64 bits and the set "
                      "is not centrally symmetric (docs section 19.13.4), UNKNOWN otherwise",
    "non_box_unnormalized": "a loop nest whose bounds do not shift together, whose integer points are no "
                            "parallelotope with the bounds' shift rows and which is outside the "
                            "repetition family (independent repetition loops leading the nest around a "
                            "band the normalization rewrites, or around GEMM loops with constant "
                            "bounds; independent as written or once that band is normalized), but "
                            "whose points, enumerated as written, the readers' roles map "
                            "bijectively onto a box (every point repeated alike) -- strip-mined loops "
                            "with a remainder, k = 2 k1 + k0 on {(0, 0), (0, 1), (1, 0)}, whose accesses "
                            "do not tell the iterations apart: a GEMM of affine views on a domain the "
                            "compiler does not normalize, a documented limitation "
                            "(docs/AFFINE_GEMM_MAPPING.md section 19.13; the compiler cannot prove it "
                            "invalid: its disproof needs accesses that tell the iterations apart, under "
                            "which a GEMM's iteration set is centrally symmetric)",
    "non_box_undecided": "a loop nest whose bounds do not shift together, outside the normalized and the "
                         "repetition families, whose points, enumerated as written, the readers' roles "
                         "do not map bijectively onto a box (or repeat unevenly), and for which no proof "
                         "that it is no GEMM is at hand: its updates (F_A(x), F_B(x), F_C(x)), counted "
                         "over the iteration set (with one array as both factors: (F_C(x), F_A(x) + "
                         "F_B(x)), the product commutes), are centrally symmetric -- as a GEMM's are, in a "
                         "decomposition the specification does not search on a non-box domain: not "
                         "decided here (the compiler cannot prove it invalid either; "
                         "docs/AFFINE_GEMM_MAPPING.md sections 19.7, 19.13.4)",
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
    out["ordinal"] = ordinal
    out["roles"] = by_role
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


def _readers_of(case: Case):
    """Per operand, the loop variables of two or more iterations its
    subscripts read on the domain (domain forms)."""
    forms = {r: case.arrays[r].forms() for r in ("a", "b", "c")}
    trip = {lp.var: lp.trip() for lp in case.loops}
    out = {}
    for r in ("a", "b", "c"):
        out[r] = {v for f in forms[r] for v, c in _domain_form(f, case.loops).coef.items()
                  if c != 0 and trip.get(v, 2) != 1}
    return out


def _demoted_in(g):
    """The loop variables a DEMOTION verdict of generalized_gemm_on_domain
    made batch loops ('loop(s) p, q as batch loops')."""
    m = re.search(r"loop\(s\) ([^;]*) as batch loops", g.get("how") or "")
    return [v.strip() for v in m.group(1).split(",")] if m else []


def partition_decomposition(case: Case, max_points: int = 200000):
    """The dimension-intersection partitioning of docs/AFFINE_GEMM_MAPPING.md
    section 21, as the specification defines it, by enumeration on the box:
    the loops read by C only are batch loops; a row or column role (the loops
    read by one factor and C) that is not one dense coordinate keeps one
    sub-coordinate (the rule of _choose_demotion) and its other loops are
    batch loops; a loop read by both factors and not by C is never a batch
    loop (a partial-C reduction); one read by one factor only is a
    broadcast.  Valid when the resulting decomposition passes
    _check_decomposition (every role one dense coordinate, C injective per
    batch, no stride-0 view) and the partitions' outputs are disjoint: C's
    elements over (demoted loops, I, J), the other loops at their first
    values, are all distinct.  Returns {"demoted": [...], "ext": {...},
    "batched": bool} or None."""
    import itertools
    if not is_box_case(case):
        return None
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
    readers = _readers_of(case)
    many = [lp for lp in loops if lp.trip() >= 2]
    triple = {lp.var for lp in many if lp.var in readers["a"] & readers["b"] & readers["c"]}
    demoted = set(_c_only_loops(forms, loops))
    for lp in many:
        rs = {r for r in "abc" if lp.var in readers[r]}
        if len(rs) == 1 and rs != {"c"}:
            return None             # a broadcast (one factor only)
    for _ in range(len(loops) + 1):
        assignment = {}
        for lp in many:
            v = lp.var
            if v in demoted or v in triple:
                assignment[v] = "P"
            elif v in readers["a"] and v in readers["c"]:
                assignment[v] = "I"
            elif v in readers["b"] and v in readers["c"]:
                assignment[v] = "J"
            else:
                assignment[v] = "K"
        once = [lp for lp in loops if lp.trip() == 1]
        res = _check_decomposition(many + once, forms, assignment)
        if res["why"] is None:
            break
        m = re.match(r"role ([IJ]) ", res["why"]) if res["why"] != "sq1" else None
        role = m.group(1) if m else None
        if role is None and res["why"] == "sq1":
            # two operands' restrictions to a row or column role not
            # proportional: the compiler demotes there too (its readers
            # analysis); find the role
            for r in "IJ":
                lps = [lp for lp in many if assignment[lp.var] == r]
                if len(lps) >= 2:
                    role = r
                    break
        if role is None:
            return None
        cand = {lp.var for lp in many if assignment[lp.var] == role}
        if len(cand) < 2:
            return None
        chosen = _choose_demotion(forms, loops, cand)
        if not chosen:
            return None
        demoted |= chosen
    else:
        return None
    if res["stride0"]:
        return None
    if not demoted:
        return None
    # The partitions' outputs are disjoint: C's elements over (demoted, I, J)
    # distinct, with the other loops (the triple batch loops, K, the loops
    # that run once) at their first values.
    by_role = res["roles"]
    fixed = {lp.var: lp.values()[0] for lp in loops
             if lp.var not in demoted and assignment.get(lp.var, "P") in "PK"}
    vary = [lp for lp in loops if lp.var in demoted or assignment.get(lp.var) in ("I", "J")]
    elements = set()
    for pt in itertools.product(*[lp.values() for lp in vary]):
        env = dict(fixed)
        env.update(zip([lp.var for lp in vary], pt))
        elements.add(tuple(f.eval(env) for f in forms["c"]))
    count = 1
    for lp in vary:
        count *= lp.trip()
    if len(elements) != count:
        return None
    order = {lp.var: i for i, lp in enumerate(case.loops)}
    return {"demoted": sorted(demoted, key=order.get), "ext": res["ext"],
            "batched": bool(triple) or bool(demoted)}


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
    if not is_box_case(case):
        return _spec_domain(case)
    an = case.analysis or analyse(case)
    tags = set(an.get("gemm_tags", []))
    out = {"recognition": None, "basis_key": None, "basis": None, "legal_gemm": bool(an.get("is_gemm"))}

    def done(rec, key):
        # (one array as both factors needs no rule of its own since
        # 2026-10-18: two different views of one memref are lowered, docs
        # sections 7 and 19.12)
        out["recognition"] = rec
        out["basis_key"] = key
        out["basis"] = SPEC_BASIS[key]
        if rec == "ACCEPT" and case.stmt == "=AB":
            # the assignment form of that GEMM (docs section 20)
            out["basis"] += "; " + SPEC_BASIS["assignment"]
        out["offload"] = {"ACCEPT": "offloaded", "UNDETERMINED": "undetermined"}.get(rec, "stays_on_host")
        if rec == "UNDETERMINED":
            out["spec_question"] = key
        case.spec = out
        return out

    if case.kernel_style == "scalar_acc":
        return done("OUT_OF_SCOPE", "scalar_acc")
    # Every loop a repetition loop of two or more iterations (no access names
    # a counter, no loop runs once): one scalar multiply-accumulate repeated,
    # no loop of a GEMM to anchor on -- a documented limitation (the
    # compiler decides nothing: UNKNOWN).
    if (case.loops and an.get("affine") and case.kernel_style != "init_inside"
            and mac_like(case)
            and all(lp.trip() >= 2 for lp in case.loops)
            and not any(f.vars() for arr in case.arrays.values() for f in arr.forms())):
        return done("OUT_OF_SCOPE", "no_gemm_loop")
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
                       and mac_like(case) else None)
        if batched_ext:
            out["legal_gemm"] = True
            two_unit = sum(1 for k in "IJK" if batched_ext[k] == 1) >= 2
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if _aliases_output(case):
                return done(*_alias_verdict(case))
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            # two of M, N, K equal to 1 per batch: SQ-5, decided 2026-10-16
            return done("ACCEPT", "sq5_decided" if two_unit else "batched_views")
        # The syntactic analysis and the iteration domain differ only through
        # loops that run once (constants on the domain, Lemma 0): decide such
        # a nest on its domain (P0.4, P0.5).
        if (any(lp.trip() == 1 for lp in case.loops)
                and case.kernel_style not in ("init_inside", "scalar_acc")
                and mac_like(case) and gemm_on_domain(case)):
            out["legal_gemm"] = True
            if gemm_on_domain(case) == "repeat_in_reduction":
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            # the rules that do not depend on the coordinates still apply
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if _aliases_output(case):
                return done(*_alias_verdict(case))
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
        if case.stmt in ("+=sum", "+=triple"):
            return done("REJECT", "not_gemm")
        if case.stmt == "=AB" and not c_written_once(case):
            return done("REJECT", "not_gemm_overwrite")
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
            if _aliases_output(case):
                return done(*_alias_verdict(case))
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            return done("ACCEPT", "sq5_decided" if g["two_unit"] else "generalized_views")
        # The dimension-intersection partitioning (docs section 21): the
        # canonical decomposition fails, but the partitioned one -- loops read
        # by C only, and the extra coordinates of a row or column role, demoted
        # to batch loops by the rule of section 21 -- is a batched GEMM whose
        # partitions write disjoint slabs of C.
        part = partition_decomposition(case) if g and g["verdict"] in ("DEMOTION", "SQ1") else None
        if part:
            out["legal_gemm"] = True
            out["partition"] = part["demoted"]
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if _aliases_output(case):
                return done(*_alias_verdict(case))
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            return done("ACCEPT", "dimension_partition")
        if g and g["verdict"] == "SQ1":
            return done("UNDETERMINED", "SQ-1")
        if g and g["verdict"] == "DEMOTION":
            if "broadcast" in (g.get("why") or ""):
                return done("OUT_OF_SCOPE", "broadcast")
            # a decomposition that demotes a loop C does not read (read by
            # both factors): its batches would all accumulate into one slab
            # of C -- a partial-C reduction
            if any(v not in _readers_of(case)["c"] for v in _demoted_in(g)):
                return done("OUT_OF_SCOPE", "partial_reduction")
            return done("OUT_OF_SCOPE", "demotion")
        return done("REJECT", "not_gemm")
    if any(lp.trip() == 0 for lp in case.loops):
        return done("UNDETERMINED", "SQ-2")
    if "batched" in tags:
        # a batched GEMM is inside the specification since 2026-10-15
        # (ISSUE-006) when every operand carries every batch coordinate; an
        # operand shared by all batches (no batch subscript, C[b] += A[b] B)
        # is, since 2026-10-19 (docs section 21), a partition on the batch
        # coordinate the other factor and C read -- inside the specification
        # when the partitioned decomposition is valid (disjoint slabs of C)
        p_vars = {lp.var for lp in case.loops if lp.role == "P"}
        shared = [r for r in ("a", "b")
                  if sum(1 for f in case.arrays[r].forms() if f.vars() and f.vars() <= p_vars) != len(p_vars)]
        # a declared batch coordinate of two or more iterations read by C and
        # exactly one factor can be the row or column role: with the role it
        # sits next to running once it IS that role (a loop of one iteration
        # is a constant on the domain), decided below on the domain
        rd = _readers_of(case)
        role_candidate = any(v in rd["c"] and ((v in rd["a"]) != (v in rd["b"])) for v in p_vars)
        # a batched product whose I, J and K loops all run once is an
        # element-wise product, not a GEMM (docs/AFFINE_GEMM_MAPPING.md
        # section 19.1, non-degeneracy; SQ-5 decided only two unit extents);
        # a batch coordinate read by C only (both factors shared) does not
        # change that: each of its slabs would receive a 1 x 1 x 1 product
        if not role_candidate and all(lp.trip() == 1 for lp in case.loops if lp.role in "IJK"):
            out["legal_gemm"] = False
            return done("REJECT", "not_gemm")
        if shared:
            # a declared batch loop read by two operands only: a partition
            # (section 21) -- or, when the role it shares is otherwise empty
            # (the row loop runs once), simply that role (the readers'
            # canonical decomposition, a plain GEMM)
            part = partition_decomposition(case)
            g = None if part else generalized_gemm_on_domain(case)
            if not part and not (g and g["verdict"] == "ACCEPT"):
                return done("OUT_OF_SCOPE", "batched_shared")
            if part:
                out["partition"] = part["demoted"]
            # the extents of the decomposition the compiler builds (per
            # partition or batch): what the lowering remark reports, which
            # may differ from the declared roles' extents when a declared
            # batch loop is the row or column role
            out["logical_extents"] = dict((part or g)["ext"])
            if case.elem != "float":
                return done("OUT_OF_SCOPE", "double")
            if len({a.elem for a in case.arrays.values()}) > 1:
                return done("OUT_OF_SCOPE", "mixed_types")
            if _aliases_output(case):
                return done(*_alias_verdict(case))
            if repeat_in_reduction(case):
                return done("OUT_OF_SCOPE", "repeat_in_reduction")
            return done("ACCEPT", "dimension_partition" if part else "generalized_views")
    if "alpha_scaled" in tags or "alpha_minus_one" in tags:
        return done("OUT_OF_SCOPE", "alpha")
    if case.kernel_style == "init_inside":
        return done("OUT_OF_SCOPE", "init_inside")
    if case.elem != "float":
        return done("OUT_OF_SCOPE", "double")
    if len({a.elem for a in case.arrays.values()}) > 1:
        return done("OUT_OF_SCOPE", "mixed_types")
    if _aliases_output(case):
        return done(*_alias_verdict(case))
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
        if fa_.is_const() and fb_.is_const():
            # a coordinate of extent one read through constants (A[i][6] *
            # B[1][j]: K = 1, each factor's own slice): one coordinate, not
            # the split-radix question of SQ-1
            continue
        if shared_coordinate(fa_, fb_) is None:
            return done("UNDETERMINED", "SQ-1")
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
# domains whose bounds read the enclosing counters: analysis, prediction,
# specification (docs/AFFINE_GEMM_MAPPING.md section 19.12)
# --------------------------------------------------------------------------

MAC_STMTS = ("+=", "=C+AB", "=AB+C", "+=BA")


def _analyse_domain(case: Case) -> dict:
    """A nest whose bounds read the enclosing counters: analysed on its box
    form when the bounds shift together (the computation is the same, and it
    is what the compiler analyses after normalization), on the box of its
    points when they are a parallelotope (family X), on its surrogate in the
    repetition family; else recorded as a non-box domain left as written --
    no analysis on a box applies; whether some decomposition makes it a GEMM
    is the specification's decision on its enumerated points (_spec_domain;
    docs sections 19.7, 19.13)."""
    if shifts_together(case):
        result = dict(analyse(box_form(case)))
        result["domain"] = "parallelotope"
        result["shifted_loops"] = [lp.var for lp in case.loops if lp.lb_terms]
        result["gemm_tags"] = list(result.get("gemm_tags", [])) + ["parallelotope_domain"]
        case.analysis = result
        return result
    form = exact_form(case)
    if form is not None:
        # an exact domain (docs section 19.13): the same points as the
        # parallelotope, so the same analysis
        result = dict(analyse(box_form(form)))
        result["domain"] = "exact_parallelotope"
        result["gemm_tags"] = list(result.get("gemm_tags", [])) + ["exact_domain"]
        case.analysis = result
        return result
    structure, view = repetition_view(case)
    if structure is not None:
        result = dict(analyse(_surrogate(view, structure[1])))
        result["domain"] = "repetition_" + structure[0]
        result["gemm_tags"] = list(result.get("gemm_tags", [])) + ["repetition_domain"]
        # every point of the GEMM runs once per point of the repetition
        # loops (and of any other repetition loop the surrogate counted)
        others = [lp for lp in view.loops if lp.var not in structure[1] and lp.role == "R"]
        result["repetitions"] = repetition_points(view, structure[1]) * \
            volume([lp.trip() for lp in others])
        case.analysis = result
        return result
    roles = {lp.var: lp.role for lp in case.loops}
    used = set()
    affine_ok = True
    for arr in case.arrays.values():
        try:
            for f in arr.forms():
                used |= f.vars()
        except NotAffine:
            affine_ok = False
    for v in used:
        if v not in roles:
            raise ValueError("%s: subscript uses %r which is not a loop counter" % (case.id, v))
    result = {"loop_roles": roles, "repetition_loops": [lp.var for lp in case.loops if lp.var not in used],
              "affine": affine_ok, "perfect_nest": case.kernel_style not in ("init_inside", "scalar_acc"),
              "domain": "non_box", "is_gemm": False, "gemm_tags": [],
              "math_class": case.math_class or "non_box_domain",
              "not_dense_reasons": [
                  "the bounds of loop%s %s do not shift together and the integer points are no "
                  "parallelotope with the bounds' shift rows, outside the repetition family: the nest is "
                  "not rewritten onto a box (docs/AFFINE_GEMM_MAPPING.md sections 19.12, 19.13), so no "
                  "analysis on a box applies" % (
                      "s" if sum(not lp.shifts_together() for lp in case.loops) > 1 else "",
                      ", ".join(lp.var for lp in case.loops if not lp.shifts_together())),
                  "a GEMM in some decomposition only if its updates (F_A(x), F_B(x), F_C(x)), counted "
                  "over the iteration set, are those of a GEMM -- an affine image of a box, centrally "
                  "symmetric (docs section 19.7): decided on the enumerated points by the "
                  "specification below"]}
    case.analysis = result
    return result


def _refused_by_frontend(case: Case, reason: str) -> dict:
    ex = {s: "not_reached" for s in STAGES}
    ex["frontend"] = "rejected"
    ex["hoisting_required"] = False
    ex["reject_stage"] = "frontend"
    ex["reject_reason"] = reason
    ex["driver"] = "not_translated"
    case.expect = ex
    return ex


def _predict_domain(case: Case) -> dict:
    """The C front end translates a loop counting up whose bounds are affine
    in the enclosing counters with any terms (systolic-cfront.c for_stmt,
    2026-10-18; a nest that is no constant-extent parallelotope has its
    subscript ranges computed by running its loops, up to
    FRONTEND_ENUMERATION_LIMIT counter values). The compiler then (docs
    sections 19.12, 19.13):
    maps a parallelotope as written onto its box; maps a band whose exact
    integer set is a parallelotope with the bounds' shift rows onto that
    box; hoists independent repetition loops of any shape around the GEMM
    loops (box ones; or, as a leading run, around a band it normalizes as
    written, inner repetition loops included), a loop of one iteration being
    a dimension unless it leads the nest or a loop's bounds read its counter
    (_hoist_dims; on the band as the normalization leaves it,
    _predictor_view); and leaves any
    other band as written -- PROVEN_INVALID at the dense partition when the
    disproof's conditions hold (disproof_holds: no stepped loop whose lower
    bound reads a counter, three different arrays, the stacked access map of
    rank n, the set nonempty and not centrally symmetric, the written
    bounds within 64 bits), UNKNOWN otherwise; when only the outermost loops
    are read by no access and a loop below reads their counters, the band
    below them is the candidate, UNKNOWN at the dense partition (bounds over
    outer counters)."""
    if any(not lp.is_box() and lp.step < 0 for lp in case.loops):
        return _refused_by_frontend(case, "a loop whose bounds depend on an enclosing loop counter "
                                          "must count up")
    if shifts_together(case):
        ex = dict(predict(box_form(case)))
        ex["domain_normalized"] = True
        case.expect = ex
        return ex
    if frontend_enumeration_values(case.loops) > FRONTEND_ENUMERATION_LIMIT:
        return _refused_by_frontend(case, "the iteration domain is not a parallelotope and running "
                                          "its loops takes more than 4194304 counter values: its "
                                          "exact subscript ranges are not computed")
    form = exact_form(case)
    if form is not None:
        ex = dict(predict(box_form(form)))
        ex["domain_normalized"] = True
        case.expect = ex
        return ex
    structure, pstructure, view = _predictor_view(case)
    if structure is not None:
        shape, rep = structure
        ex = dict(predict(_surrogate(view, rep)))
        # a prefix leaves no repetition loop inside the GEMM loops: nothing
        # to hoist, although predict() reads a one-iteration loop of the
        # surrogate as a dimension (review of 2026-10-19, round 4)
        ex["hoisting_required"] = shape == "interleaved"
        case.expect = ex
        return ex
    structure = pstructure
    if structure is not None and structure[0] == "prefix":
        # the band below the prefix is not normalizable: the inner candidate
        # (the band alone) is the one the compiler decides
        ex = dict(predict(_without(view, structure[1])))
        case.expect = ex
        return ex
    # left as written
    ex = {s: "not_reached" for s in STAGES}
    ex["frontend"] = "translated"
    ex["hoisting_required"] = False
    names = _loop_vars(case)
    used = _compiler_dims(case)
    unused = [i for i, v in enumerate(names) if v not in used]
    band = case
    if unused and unused == list(range(len(unused))):
        # the loops no access reads are the outermost ones, and a loop below
        # reads their counters in its bounds (else the prefix branch above):
        # the compiler's candidate is the band of the loops below them
        # (collectAffineMatmulCandidates: one anchor per loop, inner ones
        # first; an anchor above an unused loop fails, one below a read loop
        # leaves an access's counter outside), whose domain has the prefix's
        # counters as parameters -- never static, and outside the symmetry
        # test (its dimensions must be the mapping's columns)
        band = _without(case, [names[i] for i in unused])
        unused = []
    if unused:
        ex["recognition"] = "reject"
        ex["reject_stage"] = "recognition"
        ex["reject_reason"] = ("loop %d of the nest is used by no access (it repeats the "
                               "accumulation; moved outward by the lowering pass when that is a legal "
                               "interchange -- here it was not, so the mapping is not decided)" % unused[0])
        ex["driver"] = "stays_on_host"
        case.expect = ex
        return ex
    # the recognizer reads the subscripts as on a box (the roles by readers);
    # the dense partition then refuses the domain. The surrogate replaces
    # only the loops whose bounds read counters: a loop with constant bounds
    # keeps them -- one of a single iteration is a constant on the domain
    # (Lemma 0; review 2026-10-18 #23) --, and a refusal of the front end
    # predicted on it (a scalar accumulator) is the nest's (#25).
    sur = _surrogate(band, [lp.var for lp in band.loops if not lp.is_box()])
    sur_ex = predict(sur)
    if sur_ex.get("frontend") == "rejected":
        return _refused_by_frontend(case, sur_ex["reject_reason"])
    if sur_ex.get("recognition") != "accept":
        ex.update({k: sur_ex[k] for k in ("recognition", "reject_stage", "reject_reason")})
        ex["driver"] = "stays_on_host"
        case.expect = ex
        return ex
    ex["recognition"] = "accept"
    ex["dense_partition"] = "unsupported"
    ex["reject_stage"] = "dense_partition"
    if band is case and disproof_holds(case):
        ex["reject_reason"] = ("the iteration domain is not centrally symmetric (a point of it has its "
                               "mirror about the midpoint of its lexicographic extremes outside it), so "
                               "it is no affine image of a box; the accesses tell the iterations apart "
                               "(their stacked map is injective), so the nest is not a GEMM of affine "
                               "views in any decomposition")
    else:
        ex["reject_reason"] = ("the loop bounds are not static, so the iteration domain has no static "
                               "normalized extent")
    ex["driver"] = "stays_on_host"
    case.expect = ex
    return ex


def _no_gemm_proof(case: Case) -> bool:
    """Whether the nest is provably no GEMM of affine views in any
    decomposition, repeated or not (docs/AFFINE_GEMM_MAPPING.md sections
    19.1, 19.7): its updates -- the multiset {(F_A(x), F_B(x), F_C(x)) : x in
    D} over every iteration point, with multiplicity -- are not centrally
    symmetric. A GEMM's updates are those of its slots: the image of a box Z
    under an affine map W, taken once per slot (and as often again per point
    of independent repetition loops), and z -> z_min + z_max - z maps Z onto
    itself, so they are symmetric about W of Z's centre; so is every affine
    projection of them. With one array as both factors the product commutes
    -- two views may exchange the factors from point to point -- so the
    projection used is the exchange-invariant (F_C(x), F_A(x) + F_B(x)).
    The compiler's own disproof (disproof_holds: the stacked access map
    injective, three arrays, the iteration set not symmetric) implies this
    one."""
    fa, fb, fc = (case.arrays[r].forms() for r in ("a", "b", "c"))
    if case.a.name == case.b.name:
        if len(fa) != len(fb):
            return False

        def update(env):
            return tuple(f.eval(env) for f in fc) + tuple(x.eval(env) + y.eval(env) for x, y in zip(fa, fb))
    else:
        def update(env):
            return tuple(f.eval(env) for f in fa + fb + fc)
    updates = {}
    for env in domain_points(case.loops):
        t = update(env)
        updates[t] = updates.get(t, 0) + 1
    if not updates:
        return False
    s = tuple(a + b for a, b in zip(min(updates), max(updates)))
    return any(updates.get(tuple(c - x for c, x in zip(s, t)), 0) != m for t, m in updates.items())


def _affine_fit(samples) -> bool:
    """Whether the integers samples[x] are one affine function of the tuples
    x: v = w0 + w . x for all of them (exact, fractions.Fraction)."""
    from fractions import Fraction
    rows = [[Fraction(1)] + [Fraction(c) for c in x] + [Fraction(v)] for x, v in samples.items()]
    cols, rank = len(rows[0]) - 1 if rows else 0, 0
    for col in range(cols):
        pivot = next((r for r in range(rank, len(rows)) if rows[r][col] != 0), None)
        if pivot is None:
            continue
        rows[rank], rows[pivot] = rows[pivot], rows[rank]
        for r in range(len(rows)):
            if r != rank and rows[r][col] != 0:
                f = rows[r][col] / rows[rank][col]
                rows[r] = [a - f * b for a, b in zip(rows[r], rows[rank])]
        rank += 1
    return all(row[-1] == 0 for row in rows[rank:])


def _is_box_image(points) -> bool:
    """Whether the tuples `points` are the image of a box under an injective
    integer affine map triangular in their coordinate order -- a box sheared
    along it: at every value of the earlier coordinates, the next one runs
    over an arithmetic progression of one step and one length, whose start is
    an affine function of the earlier ones ({(0, 0), (0, 1), (1, 1), (1, 2)}
    = {(p1, p1 + p2)}; an axis-aligned box, possibly strided, is one). A
    sufficient test: an affine image of a box sheared otherwise (both edges
    of a parallelogram along a diagonal) fails it."""
    pts = set(points)
    if not pts:
        return False
    for m in range(len(next(iter(pts)))):
        runs = {}
        for p in pts:
            runs.setdefault(p[:m], set()).add(p[m])
        shapes, starts = set(), {}
        for prefix, vals in runs.items():
            vals = sorted(vals)
            steps = {b - a for a, b in zip(vals, vals[1:])}
            if len(steps) > 1:
                return False
            shapes.add((len(vals), steps.pop() if steps else None))
            starts[prefix] = vals[0]
        if len({n for n, _ in shapes}) != 1 or len({s for _, s in shapes if s is not None}) > 1:
            return False
        if m and not _affine_fit(starts):
            return False
    return True


def _spec_domain(case: Case) -> dict:
    """The specification on a domain whose bounds read the enclosing
    counters. Bounds that shift together (family F): the iteration set is the
    image of a box under x = U x' (U lower unitriangular, order-preserving;
    docs/AFFINE_GEMM_MAPPING.md section 19.12), so the definition of 19.1 --
    a bijection from a box, the roles taken on the box (19.2, 'Non-box
    domains') -- is decided on the box form, the same computation; so is a
    set whose points are a parallelotope (family X, exact_form), and the
    repetition family on its surrogate. Otherwise the points are enumerated
    as written: a GEMM (legal, outside the normalized family) when the
    readers' roles map them bijectively onto a box, every point repeated
    alike, every batch iteration the same shape; no other parametrization is
    searched, so a REJECT needs a proof that none exists (_no_gemm_proof:
    the updates, with multiplicity, not centrally symmetric -- docs section
    19.7), else the case is not decided ('non_box_undecided'). A nest
    without iteration points computes nothing: SQ-2, as a box with a loop of
    zero iterations (review 2026-10-18 #28)."""
    if shifts_together(case):
        out = dict(spec_expect(box_form(case)))
        out["domain"] = "parallelotope"
        out["basis"] = (out["basis"] + " -- on a constant-extent parallelotope domain (the bounds "
                        "shifted together by the enclosing counters), decided on its box x = U x' "
                        "(docs/AFFINE_GEMM_MAPPING.md section 19.12)")
        case.spec = out
        return out
    form = exact_form(case)
    if form is not None:
        # the points are a parallelotope's: the same computation, decided on
        # its box (docs section 19.13)
        out = dict(spec_expect(box_form(form)))
        out["domain"] = "exact_parallelotope"
        out["basis"] = (out["basis"] + " -- on a domain whose bounds do not shift together but whose "
                        "integer points are a constant-extent parallelotope (enumerated: "
                        "casespec.exact_form), decided on its box (docs/AFFINE_GEMM_MAPPING.md section "
                        "19.13)")
        case.spec = out
        return out
    structure, view = repetition_view(case)
    if structure is not None:
        # independent repetition loops: every point of the GEMM loops runs
        # once per point of theirs -- the GEMM repeated, decided with them as
        # plain repetition loops (the rule of repeat_in_reduction included);
        # within the documented family when they are a prefix around a
        # normalizable band or the GEMM loops are a box (docs section 19.13)
        shape, rep = structure
        out = dict(spec_expect(_surrogate(view, rep)))
        if out.get("legal_gemm") and repetition_points(view, rep) == 0:
            # the surrogate's two-iteration loops hide an empty repetition
            # set: the nest computes nothing -- SQ-2, as for a box with a
            # loop of zero iterations (review 2026-10-18 #28)
            out.update({"recognition": "UNDETERMINED", "basis_key": "SQ-2", "spec_question": "SQ-2",
                        "basis": SPEC_BASIS["SQ-2"], "offload": "undetermined"})
        out["domain"] = "repetition_" + shape
        out["basis"] = (out["basis"] + " -- the repetition loops (no access reads them, their "
                        "bounds read only each other) form a set of their own, every point of the "
                        "GEMM loops runs once per point of it (docs/AFFINE_GEMM_MAPPING.md section "
                        "19.13)")
        case.spec = out
        return out
    an = case.analysis or analyse(case)
    out = {"recognition": None, "basis_key": None, "basis": None, "legal_gemm": False,
           "domain": "non_box"}

    def done(rec, key):
        out["recognition"] = rec
        out["basis_key"] = key
        out["basis"] = SPEC_BASIS[key]
        if rec == "ACCEPT" and case.stmt == "=AB":
            # the assignment form of that GEMM (docs section 20)
            out["basis"] += "; " + SPEC_BASIS["assignment"]
        out["offload"] = {"ACCEPT": "offloaded", "UNDETERMINED": "undetermined"}.get(rec, "stays_on_host")
        if rec == "UNDETERMINED":
            out["spec_question"] = key
        case.spec = out
        return out

    if not an.get("affine") or case.stmt in ("+=sum", "+=triple"):
        return done("REJECT", "not_gemm")
    if case.stmt == "=AB" and not c_written_once(case):
        return done("REJECT", "not_gemm_overwrite")
    if case.kernel_style == "scalar_acc":
        return done("OUT_OF_SCOPE", "scalar_acc")
    used = set()
    for arr in case.arrays.values():
        for f in arr.forms():
            used |= f.vars()
    # The points over the loops the accesses use, with their multiplicity: a
    # loop no access uses repeats the accumulation, which is a GEMM only when
    # it repeats every point the same number of times (a triangle in such a
    # loop accumulates each row a different number of times: not a GEMM).
    counts = {}
    pts = []
    for env in domain_points(case.loops):
        key = tuple(sorted((k, v) for k, v in env.items() if k in used))
        if key not in counts:
            pts.append(dict(key))
        counts[key] = counts.get(key, 0) + 1
    if len(pts) > 200000:
        return done("UNDETERMINED", "non_box_unenumerated")
    if not pts:
        # no iteration point: the nest computes nothing -- SQ-2, as for a box
        # with a loop of zero iterations (review 2026-10-18 #28)
        return done("UNDETERMINED", "SQ-2")
    found = None if len(set(counts.values())) != 1 else _gemm_on_points(case, pts)
    if found is None and len(set(counts.values())) == 1:
        # batched: the loops every operand reads, with two or more values,
        # fix the batch; per batch value one GEMM, all of one shape
        readers = {}
        for r in ("a", "b", "c"):
            for f in case.arrays[r].forms():
                for v in f.vars():
                    readers.setdefault(v, set()).add(r)
        batch = [v for v in _loop_vars(case)
                 if v in used and readers.get(v) == {"a", "b", "c"} and len({p[v] for p in pts}) >= 2]
        if batch and _is_box_image([tuple(p[v] for v in batch) for p in pts]):
            # the batch values must be the image of a box under the
            # parametrization: Z is a box of all the coordinates, the batch
            # ones included (docs section 19.1; review 2026-10-19: a triangle
            # of batch values was called a GEMM) -- sheared along the loops
            # (b2 in [b1, b1 + 2)) as well as axis-aligned (_is_box_image; it
            # needed an axis-aligned box before 2026-10-19, review 3)
            groups = {}
            for p in pts:
                groups.setdefault(tuple(p[v] for v in batch), []).append(p)
            shape = None
            for sub in groups.values():
                res = _gemm_on_points(case, sub)
                if res is None or not res[1] or (shape is not None and res[:2] != shape):
                    shape = False
                    break
                shape = res[:2]
            found = shape or None
    if not found:
        # the readers' roles find no GEMM; another decomposition is searched
        # nowhere, so the refusal needs its proof (docs section 19.7)
        return done("REJECT", "non_box_domain") if _no_gemm_proof(case) else \
            done("UNDETERMINED", "non_box_undecided")
    # a GEMM found has symmetric updates; the proof is applied all the same
    # (a correct proof must never meet a GEMM)
    if _no_gemm_proof(case):
        return done("REJECT", "non_box_domain")
    out["legal_gemm"] = True
    if case.stmt in ("+=alpha", "-="):
        return done("OUT_OF_SCOPE", "alpha")
    if case.kernel_style == "init_inside":
        return done("OUT_OF_SCOPE", "init_inside")
    if case.elem != "float":
        return done("OUT_OF_SCOPE", "double")
    if len({a.elem for a in case.arrays.values()}) > 1:
        return done("OUT_OF_SCOPE", "mixed_types")
    if _aliases_output(case):
        return done(*_alias_verdict(case))
    # a GEMM on a domain outside the normalized family: a documented
    # limitation (docs section 19.13), not a false negative
    return done("OUT_OF_SCOPE", "non_box_unnormalized")


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
    if not is_box_case(case):
        # a non-box domain of the normalized families: the logical operands
        # are those of its box case (the same elements, gathered over the
        # box's loops through the composed subscripts; docs/AFFINE_GEMM_MAPPING.md
        # sections 19.12, 19.13); the repetition count is the analysis's
        box = analysis_case(case)
        if box is None:
            return None
        box.analysis = an
        case = box
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
    # beta = 0: C re-initialised inside the nest, a scalar accumulator, or the
    # assignment C = A * B (the K = 1 GEMM into a zeroed C, docs section 20)
    zero = case.kernel_style in ("init_inside", "scalar_acc") or case.stmt == "=AB"

    def ordinal_expr(g):
        ls = groups.get(g, [])     # no loop: a coordinate of extent one, ordinal 0
        terms = []
        mult = 1
        for lp in reversed(ls):
            terms.append("((%s - %d) / %d) * %d" % (lp.var, lp.lb, lp.step, mult)
                         if (lp.lb or lp.step != 1 or mult != 1) else lp.var)
            mult *= lp.trip()
        return " + ".join(reversed(terms)) or "0"

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
    # an assignment's repetitions rewrite the same values (idempotent): the
    # GEMM into the zeroed C' runs once (docs section 20)
    if case.stmt == "=AB":
        lines.append(ind2 + "/* the GEMM C' = A' x B' into the zeroed C' (M=%d, N=%d, K=%d)%s */" % (
            M, N, K, "; the %d repetitions of the nest rewrite the same values" % reps if reps > 1 else ""))
        reps = 1
    else:
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
        if not lp.is_box():
            return "%s in [%s, %s)%s" % (lp.var, lp._bound_text(lp.lb_terms, lp.lb),
                                         lp._bound_text(lp.ub_terms, lp.exclusive_ub()),
                                         "" if lp.step == 1 else " step %d" % lp.step)
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
        out.append("Logical GEMM: C'(I,J) %s A'(I,K) * B'(K,J)   M=%d N=%d K=%d%s%s" % (
            "= (C zeroed first: an assignment, docs section 20)" if case.stmt == "=AB" else "+=",
            ext["I"], ext["J"], ext["K"],
            ", one per value of batch coordinate %s" % ", ".join(lp.var for lp in case.loops if lp.role == "P")
            if an["math_class"] == "gemm_batched" else "",
            ", repeated %d times" % an["repetitions"] if an.get("repetitions", 1) > 1 else ""))
        # (a non-box domain is described on its box case: docs sections
        # 19.12, 19.13)
        described = case if is_box_case(case) else (analysis_case(case) or case)
        if described is not case:
            out.append("  on the box of its iteration set: " + "; ".join(loop_summary(described)))
        groups = {}
        for lp in described.loops:
            groups.setdefault(lp.role, []).append(lp)
        for g in ("I", "J", "K"):
            out.append("  %s is enumerated by loop(s) %s: extent %d" % (
                g, ", ".join("%s (%d values)" % (lp.var, lp.trip()) for lp in groups[g])
                if g in groups else "none (a constant subscript: extent one)", ext[g]))
        for role, nm, g0, g1 in (("a", "A'(I,K)", "I", "K"), ("b", "B'(K,J)", "K", "J"), ("c", "C'(I,J)", "I", "J")):
            arr = described.arrays[role]
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
        out.append("Not analysed as a GEMM on a box:" if an.get("domain") == "non_box"
                   else "Not a dense GEMM of these operands because:")
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
        # a recorded spec question, or a case the enumeration does not decide
        # (non_box_unenumerated, non_box_undecided: their basis says why)
        q = spec["spec_question"]
        h.append("  The specification does not decide this case (%s%s):" % (
            q, ", README.md section 2" if q in SPEC_QUESTIONS else ""))
        for ln in _wrap(SPEC_QUESTIONS.get(q) or spec.get("basis") or "", 74):
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
    if any(factor_nonzero(case, n) for n in case.array_names()):
        h.append("the factors of this assignment hold no zeros (a -0 product would come back from")
        h.append("the tiles as +0: docs/AFFINE_GEMM_MAPPING.md sections 6 and 20);")
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
    nz_names = [n for n in case.array_names() if factor_nonzero(case, n)]
    if nz_names:
        out.append("/* an assignment C = A * B: its factors without zeros -- a -0 product (a zero times")
        out.append("   a negative factor) comes back from the tiles as +0, the documented deviation of")
        out.append("   docs/AFFINE_GEMM_MAPPING.md sections 6 and 20, which a bitwise check would report */")
        out.append("static %s elem_nz(long e, long seed) { %s v = elem(e, seed); return v != 0 ? v : (%s)1; }" % (E, E, E))
    if case.stmt != "+=sum":
        out.append("/* the product through a call: the references are not GEMM statements */")
        out.append("static %s mul(%s x, %s y) { return x * y; }" % (E, E, E))
    out.append("")
    out.append("static void init(void) {")
    for n in case.array_names():
        arr = case.arrays[case.role_of_name(n)]
        out.append("  for (long e = 0; e < %d; ++e) (&%s%s)[e] = %s(e, %d);" % (
            volume(arr.dims), n, "[0]" * len(arr.dims), "elem_nz" if n in nz_names else "elem",
            array_seed(case, n)))
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
    out = []
    for lp in case.loops:
        trip = ("%d iteration(s)" % lp.trip() if lp.shifts_together()
                else "a trip count that varies with the enclosing counters")
        out.append("%s [%s]: %s %s %s %s %s, step %d, %s%s%s" % (
            lp.var, lp.role, lp._bound_text(lp.lb_terms, lp.lb), "<=" if lp.step > 0 else ">=", lp.var,
            lp.cmp, lp._bound_text(lp.ub_terms, lp.ub), lp.step, trip,
            " (written as the condition %s)" % lp.cond if lp.cond else "",
            "" if lp.is_box() else " (bounds affine in the enclosing counters%s)"
            % ("" if lp.shifts_together() else ", not shifting together")))
    return out


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
                        "step": lp.step, "cmp": lp.cmp,
                        "trip": lp.trip() if lp.shifts_together() else None},
                       **({"cond": lp.cond} if lp.cond else {}),
                       **({"lb_terms": lp.lb_terms, "ub_terms": lp.ub_terms} if not lp.is_box() else {}))
                  for lp in case.loops],
        "arrays": {role: {"name": arr.name, "dims": list(arr.dims), "subscripts": list(arr.subs),
                          "restrict": arr.restrict, "elem": arr.elem}
                   for role, arr in case.arrays.items()},
        "repetitions": an.get("repetitions", 1),
        "logical_extents": spec.get("logical_extents") or an.get("extents"),
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
