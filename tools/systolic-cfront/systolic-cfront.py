#!/usr/bin/env python3
"""systolic-cfront -- C to MLIR (func / affine / arith / memref) for the
affine subset of C the systolic-array compiler takes.

    systolic-cfront.py --clang CLANG [--clang-major N] [-DNAME[=VAL]] [-IDIR]
                       [-std=STD] [--function NAME] FILE.c [-o OUT.mlir]

Clang parses and type-checks the C source (``-fsyntax-only -Xclang
-ast-dump=json``): preprocessing, declarations, implicit conversions and
types are clang's. Only the clang *executable* is run; nothing of it is
linked. The clang must be the one the project is standardized on -- LLVM 24
(--clang-major, default 24): another major version is refused, never used
silently, since its AST JSON is a different format. This program translates clang's AST, construct by
construct, into MLIR. Nothing in it knows about matrix multiplication; a GEMM
is recognized later, from the affine IR, by --affine-matmul-to-systolic.

The subset is the one whose meaning is an affine loop nest over arrays, and it
is translated exactly; anything outside it is an error with its source
location, never an approximation:

  functions     void, defined in FILE.c. Parameters: arrays of float/double
                (``float A[M][K]``, ``float A[restrict M][K]``, which C adjusts
                to ``float (*restrict)[K]``) become memref<?xKxf32> -- the
                leading extent is not part of a C parameter's type; restrict
                becomes {llvm.noalias}. Scalar float/double parameters become
                f32/f64 arguments.
  statements    compound statements, ``for``, expression statements that
                assign an array element (``=``, ``+=``, ``-=``, ``*=``,
                ``/=``), a final ``return;``.
  for           ``for (int i = LB; i < UB; ++i)`` -- also ``<=``, ``i++``,
                ``i += S`` (S > 0), ``i = i + S`` -- with LB, UB, S integer
                constant expressions; i declared in the for, never written in
                the body. Becomes affine.for LB to UB step S (``<=``: UB + 1).
  subscripts    every array access subscripts all dimensions; each subscript
                is an affine expression of the enclosing loop counters and
                integer constants (+, -, constant *).
  arithmetic    + - * / and unary - on float/double, floating literals,
                integer constants converted to floating point, and C's own
                implicit float <-> double conversions (arith.extf/truncf).

Floating-point contraction: every C operator becomes its own MLIR op (no
a * b + c is fused), i.e. the semantics of -ffp-contract=off; compile a CPU
reference with -ffp-contract=off to compare bit for bit.

Exit status: 0 success, 1 the source is outside the subset (diagnostic on
stderr, clang style), 2 usage error or clang failed.
"""

import argparse
import json
import os
import re
import struct
import subprocess
import sys


class Unsupported(Exception):
    def __init__(self, node, message):
        super().__init__(message)
        self.node = node
        self.message = message


FLOAT_TYPES = {"float": "f32", "double": "f64"}
INT_TYPES = {"int", "long", "long long", "short", "unsigned int",
             "unsigned long", "unsigned long long", "unsigned short",
             "unsigned", "char", "signed char", "unsigned char"}
INT_MAX = {"int": 2**31 - 1, "long": 2**63 - 1, "long long": 2**63 - 1,
           "short": 2**15 - 1, "unsigned int": 2**32 - 1, "unsigned": 2**32 - 1,
           "unsigned long": 2**64 - 1, "unsigned long long": 2**64 - 1,
           "unsigned short": 2**16 - 1, "char": 2**7 - 1,
           "signed char": 2**7 - 1, "unsigned char": 2**8 - 1}


def qual_type(node):
    return node.get("type", {}).get("qualType", "")


def strip_qualifiers(t):
    return re.sub(r"\b(const|volatile)\b\s*", "", t).strip()


def scale_form(form, c):
    coeffs, const = form
    return {k: v * c for k, v in coeffs.items() if v * c}, const * c


def add_forms(a, b):
    coeffs = dict(a[0])
    for k, v in b[0].items():
        coeffs[k] = coeffs.get(k, 0) + v
        if coeffs[k] == 0:
            del coeffs[k]
    return coeffs, a[1] + b[1]


class SourceMap:
    """Byte offset -> (line, column) of the main file."""

    def __init__(self, path):
        with open(path, "rb") as f:
            self.text = f.read()
        self.line_starts = [0]
        for i, byte in enumerate(self.text):
            if byte == 0x0A:
                self.line_starts.append(i + 1)

    def line_col(self, offset):
        lo, hi = 0, len(self.line_starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if self.line_starts[mid] <= offset:
                lo = mid
            else:
                hi = mid - 1
        return lo + 1, offset - self.line_starts[lo] + 1


def node_offset(node):
    """The expansion offset of a node's beginning (macros expand where used)."""
    for key in ("range", "loc"):
        loc = node.get(key)
        if not loc:
            continue
        if key == "range":
            loc = loc.get("begin", {})
        loc = loc.get("expansionLoc", loc)
        if "offset" in loc:
            return loc["offset"]
    return None


def strip_parens_and_casts(node, casts=("LValueToRValue",)):
    while True:
        kind = node.get("kind")
        if kind == "ParenExpr":
            node = node["inner"][0]
        elif kind == "ImplicitCastExpr" and node.get("castKind") in casts:
            node = node["inner"][0]
        else:
            return node


class Translator:
    def __init__(self, source_path, display_name, source_map):
        self.source_path = source_path
        self.display_name = display_name
        self.source_map = source_map
        self.lines = []
        self.counter = 0
        self.ssa_names = set()
        self.symbol_suffix = ""
        self.interfaces = []

    # ---------------------------------------------------------------- helpers
    def loc(self, node):
        offset = node_offset(node)
        if offset is None:
            return 'loc("%s":0:0)' % self.display_name
        line, col = self.source_map.line_col(offset)
        return 'loc("%s":%d:%d)' % (self.display_name, line, col)

    def fresh(self, stem="t"):
        while True:
            name = "%%%s%d" % (stem, self.counter)
            self.counter += 1
            if name not in self.ssa_names:
                self.ssa_names.add(name)
                return name

    def unique(self, stem):
        stem = re.sub(r"[^A-Za-z0-9_]", "_", stem) or "v"
        if stem[0].isdigit():
            stem = "v" + stem
        name = "%" + stem
        n = 0
        while name in self.ssa_names:
            n += 1
            name = "%%%s_%d" % (stem, n)
        self.ssa_names.add(name)
        return name

    def emit(self, indent, text):
        self.lines.append("  " * indent + text)

    # --------------------------------------------------------------- function
    def function(self, fn):
        ret = qual_type(fn).split("(", 1)[0].strip()
        if ret != "void":
            raise Unsupported(fn, "only functions returning void are "
                                  "supported (got '%s')" % ret)
        params = [n for n in fn.get("inner", []) if n["kind"] == "ParmVarDecl"]
        body = [n for n in fn.get("inner", []) if n["kind"] == "CompoundStmt"]
        if len(body) != 1:
            raise Unsupported(fn, "function has no body")

        self.ssa_names = set()   # SSA names are scoped to the function
        self.counter = 0
        self.env = {}       # decl id -> ("memref", name, rank, elem, type) |
                            #   ("scalar", name, elem) | ("intscalar", name) |
                            #   ("iv", name, (first, last) or None)
        self.accesses = {}  # param decl id -> read / written / ranges
        args = []
        for p in params:
            if "name" not in p:
                raise Unsupported(p, "unnamed parameters are not supported")
            args.append(self.parameter(p))
        self.emit(1, "func.func @%s%s(%s) {" % (fn["name"], self.symbol_suffix,
                                                 ", ".join(args)))
        self.statements(body[0]["inner"] if "inner" in body[0] else [], 2,
                        top_level=True)
        self.emit(2, "func.return %s" % self.loc(body[0]))
        self.emit(1, "} %s" % self.loc(fn))
        self.interfaces.append(self.interface(fn, params, body[0]))

    def interface(self, fn, params, body):
        """What a caller-side shim needs to call the translated function
        from C: the C parameters, the memref shape each becomes (with the
        exact extent of the dimensions the C type leaves open), which arrays
        are read and written, and where the definition is in the source."""
        offset = node_offset(fn)
        line, col = self.source_map.line_col(offset) if offset is not None \
            else (0, 0)
        loc = fn.get("loc", {})
        rng = fn.get("range", {})
        out = {
            "name": fn["name"],
            "symbol": fn["name"] + self.symbol_suffix,
            "line": line, "col": col,
            "end_line": self.source_map.line_col(
                rng.get("end", {}).get("expansionLoc", rng.get("end", {}))
                .get("offset", offset or 0))[0],
            "storage_class": fn.get("storageClass", ""),
            "inline": bool(fn.get("inline")),
            # A definition written by a macro cannot be rewritten in place.
            "from_macro": any("spellingLoc" in x for x in
                              (loc, rng.get("begin", {}), rng.get("end", {}))),
            "def_begin": rng.get("begin", {}).get("offset"),
            "def_end": rng.get("end", {}).get("offset", -1) +
                       rng.get("end", {}).get("tokLen", 0),
            "name_offset": loc.get("offset"),
            "name_length": loc.get("tokLen"),
            "body_begin": node_offset(body),
            "params": [],
        }
        for p in params:
            entry = self.env[p["id"]]
            param = {"name": p["name"], "ctype": qual_type(p),
                     "kind": entry[0]}
            if entry[0] == "memref":
                param["elem"] = entry[3]
                dims = re.findall(r"x(\d+|\?)", entry[4][len("memref<?"):])
                inner = [int(d) for d in dims]
                info = self.accesses.get(p["id"], {})
                ranges = info.get("ranges")
                # Rows [0, extent): the open leading dimension is as long as
                # the accesses reach (0: never accessed).
                param["leading_extent"] = ranges[0][1] + 1 if ranges else 0
                param["leading_min"] = ranges[0][0] if ranges else 0
                param["inner_dims"] = inner
                param["read"] = info.get("read", False)
                param["written"] = info.get("written", False)
                param["noalias"] = "restrict" in qual_type(p)
            elif entry[0] == "scalar":
                param["elem"] = entry[2]
            out["params"].append(param)
        return out

    def parameter(self, p):
        t = strip_qualifiers(qual_type(p))
        # float (*restrict)[16][8]  /  float (*)[16]  /  float *restrict
        m = re.fullmatch(r"(float|double) \(\*\s*(restrict|__restrict)?\)"
                         r"((?:\[\d+\])+)", t)
        if m:
            elem = FLOAT_TYPES[m.group(1)]
            dims = ["?"] + re.findall(r"\[(\d+)\]", m.group(3))
            noalias = bool(m.group(2))
        else:
            m = re.fullmatch(r"(float|double) \*\s*(restrict|__restrict)?", t)
            if m:
                elem = FLOAT_TYPES[m.group(1)]
                dims = ["?"]
                noalias = bool(m.group(2))
            elif t in FLOAT_TYPES:
                name = self.unique(p["name"])
                self.env[p["id"]] = ("scalar", name, FLOAT_TYPES[t])
                return "%s: %s %s" % (name, FLOAT_TYPES[t], self.loc(p))
            elif t in INT_TYPES:
                # Accepted as a parameter, so that a use of it is reported
                # where it is used (a symbolic bound, a subscript) -- every
                # use is outside the subset.
                name = self.unique(p["name"])
                self.env[p["id"]] = ("intscalar", name)
                return "%s: i%d %s" % (name, 64 if "long" in t else 32,
                                       self.loc(p))
            else:
                raise Unsupported(p, "parameter '%s' of type '%s' is not a "
                                     "float/double array or scalar" %
                                  (p["name"], qual_type(p)))
        name = self.unique(p["name"])
        memref = "memref<%sx%s>" % ("x".join(dims), elem)
        self.env[p["id"]] = ("memref", name, len(dims), elem, memref)
        attrs = " {llvm.noalias}" if noalias else ""
        return "%s: %s%s %s" % (name, memref, attrs, self.loc(p))

    # ------------------------------------------------------------- statements
    def statements(self, stmts, indent, top_level=False):
        for index, s in enumerate(stmts):
            kind = s["kind"]
            if kind == "NullStmt":
                continue
            if kind == "CompoundStmt":
                self.statements(s.get("inner", []), indent)
            elif kind == "ForStmt":
                self.for_stmt(s, indent)
            elif kind == "ReturnStmt":
                if not top_level or index != len(stmts) - 1 or s.get("inner"):
                    raise Unsupported(s, "only a final 'return;' of the "
                                         "function body is supported")
            elif kind in ("BinaryOperator", "CompoundAssignOperator"):
                self.assignment(s, indent)
            elif kind == "DeclStmt":
                raise Unsupported(s, "local variables are not supported "
                                     "(only for-loop counters)")
            else:
                raise Unsupported(s, "statement '%s' is not supported" % kind)

    def int_constant(self, node):
        """Value of an integer constant expression (literals, + - *, unary -,
        parentheses, integer casts)."""
        node = strip_parens_and_casts(node, ("IntegralCast", "NoOp",
                                             "LValueToRValue"))
        kind = node["kind"]
        if kind == "IntegerLiteral":
            return int(node["value"])
        if kind == "DeclRefExpr":
            raise Unsupported(node, "'%s' is not an integer constant (loop "
                                    "bounds and steps must be static)" %
                              node.get("referencedDecl", {}).get("name", "?"))
        if kind == "UnaryOperator" and node["opcode"] in ("-", "+"):
            v = self.int_constant(node["inner"][0])
            return -v if node["opcode"] == "-" else v
        if kind == "BinaryOperator" and node["opcode"] in ("+", "-", "*"):
            a = self.int_constant(node["inner"][0])
            b = self.int_constant(node["inner"][1])
            return {"+": a + b, "-": a - b, "*": a * b}[node["opcode"]]
        raise Unsupported(node, "expected an integer constant expression "
                                "(loop bounds and steps must be static)")

    def is_ref_to(self, node, decl_id):
        node = strip_parens_and_casts(node)
        return (node["kind"] == "DeclRefExpr" and
                node.get("referencedDecl", {}).get("id") == decl_id)

    def writes_to(self, node, decl_id):
        """Whether `node` (a statement or expression tree) can modify the
        variable `decl_id`: assigned, incremented, or its address taken."""
        kind = node.get("kind")
        inner = node.get("inner", [])
        if kind in ("BinaryOperator", "CompoundAssignOperator") and \
                node.get("opcode", "").endswith("=") and \
                node.get("opcode") not in ("==", "!=", "<=", ">=") and inner:
            if self.is_ref_to(inner[0], decl_id):
                return True
        if kind == "UnaryOperator" and node.get("opcode") in \
                ("++", "--", "&") and inner and \
                self.is_ref_to(inner[0], decl_id):
            return True
        return any(self.writes_to(c, decl_id) for c in inner
                   if isinstance(c, dict))

    def for_stmt(self, s, indent):
        # ForStmt children: init, condition variable (unused), cond, inc, body
        init, _, cond, inc, body = (s.get("inner", []) + [{}] * 5)[:5]
        if init.get("kind") != "DeclStmt" or len(init.get("inner", [])) != 1:
            raise Unsupported(s, "the for loop must declare exactly one "
                                 "integer counter in its initializer")
        var = init["inner"][0]
        vtype = strip_qualifiers(qual_type(var))
        if var["kind"] != "VarDecl" or vtype not in INT_TYPES or \
                not var.get("inner"):
            raise Unsupported(var, "the loop counter must be an initialized "
                                   "integer variable")
        decl = var["id"]
        lb = self.int_constant(var["inner"][0])

        if cond.get("kind") != "BinaryOperator" or \
                cond.get("opcode") not in ("<", "<=") or \
                not self.is_ref_to(cond["inner"][0], decl):
            raise Unsupported(cond or s, "the loop condition must be "
                                         "'counter < bound' or "
                                         "'counter <= bound'")
        ub = self.int_constant(cond["inner"][1])
        if cond["opcode"] == "<=":
            ub += 1

        step = None
        if inc.get("kind") == "UnaryOperator" and inc.get("opcode") == "++" \
                and self.is_ref_to(inc["inner"][0], decl):
            step = 1
        elif inc.get("kind") == "CompoundAssignOperator" and \
                inc.get("opcode") == "+=" and \
                self.is_ref_to(inc["inner"][0], decl):
            step = self.int_constant(inc["inner"][1])
        elif inc.get("kind") == "BinaryOperator" and inc.get("opcode") == "=" \
                and self.is_ref_to(inc["inner"][0], decl):
            rhs = strip_parens_and_casts(inc["inner"][1])
            if rhs.get("kind") == "BinaryOperator" and rhs["opcode"] == "+" \
                    and self.is_ref_to(rhs["inner"][0], decl):
                step = self.int_constant(rhs["inner"][1])
        if step is None:
            raise Unsupported(inc or s, "the loop increment must be '++i', "
                                        "'i++', 'i += S' or 'i = i + S'")
        if step <= 0:
            raise Unsupported(inc, "the loop step must be positive")

        # The counter's last increment must not overflow its type (that would
        # be undefined behaviour or wrap-around, neither of which affine.for
        # models).
        if ub - 1 + step > INT_MAX.get(vtype, 2**31 - 1) or lb < -2**31:
            raise Unsupported(s, "the loop counter could overflow '%s'" % vtype)

        if self.writes_to(body, decl):
            raise Unsupported(body, "the loop counter is modified in the "
                                    "loop body")

        name = self.unique(var.get("name", "iv"))
        # The counter's first and last values, or None for a loop that runs
        # zero times.
        values = (lb, lb + step * ((ub - 1 - lb) // step)) if ub > lb else None
        self.env[decl] = ("iv", name, values)
        step_text = "" if step == 1 else " step %d" % step
        self.emit(indent, "affine.for %s = %d to %d%s {" %
                  (name, lb, ub, step_text))
        if body.get("kind") == "CompoundStmt":
            self.statements(body.get("inner", []), indent + 1)
        else:
            self.statements([body], indent + 1)
        self.emit(indent, "} %s" % self.loc(s))
        del self.env[decl]

    # ------------------------------------------------------------ expressions
    def affine_index(self, node):
        """An affine subscript: (text usable inside affine.load brackets,
        linear form ({counter decl id: coefficient}, constant))."""
        node = strip_parens_and_casts(node, ("LValueToRValue", "IntegralCast",
                                             "NoOp"))
        kind = node["kind"]
        if kind == "IntegerLiteral":
            v = int(node["value"])
            return str(v), ({}, v)
        if kind == "DeclRefExpr":
            decl = node.get("referencedDecl", {}).get("id")
            entry = self.env.get(decl)
            if entry and entry[0] == "iv":
                return entry[1], ({decl: 1}, 0)
            raise Unsupported(node, "subscript uses '%s', which is not an "
                                    "enclosing loop counter" %
                              node.get("referencedDecl", {}).get("name", "?"))
        if kind == "UnaryOperator" and node["opcode"] == "-":
            text, form = self.affine_index(node["inner"][0])
            return "-(%s)" % text, scale_form(form, -1)
        if kind == "BinaryOperator" and node["opcode"] in ("+", "-"):
            ta, fa = self.affine_index(node["inner"][0])
            tb, fb = self.affine_index(node["inner"][1])
            sign = 1 if node["opcode"] == "+" else -1
            return "(%s %s %s)" % (ta, node["opcode"], tb), \
                add_forms(fa, scale_form(fb, sign))
        if kind == "BinaryOperator" and node["opcode"] == "*":
            a, b = node["inner"]
            for const, other, const_first in ((a, b, True), (b, a, False)):
                try:
                    c = self.int_constant(const)
                except Unsupported:
                    continue
                text, form = self.affine_index(other)
                text = ("(%d * %s)" % (c, text)) if const_first else \
                    ("(%s * %d)" % (text, c))
                return text, scale_form(form, c)
            raise Unsupported(node, "subscript multiplies two non-constant "
                                    "terms; not an affine expression")
        raise Unsupported(node, "subscript is not an affine expression of "
                                "the loop counters")

    def form_range(self, form):
        """Exact (min, max) of a linear form over the enclosing loops' box,
        or None if some enclosing loop runs zero times (the access never
        executes)."""
        coeffs, const = form
        lo = hi = const
        for decl, c in coeffs.items():
            entry = self.env[decl]
            if entry[2] is None:
                return None
            first, last = entry[2]
            lo += min(c * first, c * last)
            hi += max(c * first, c * last)
        return lo, hi

    def array_access(self, node, read=False, written=False):
        """(memref entry, [subscripts]) of a fully subscripted array element.
        Records the access (read / written, the exact range of each
        subscript) for the function's interface."""
        indices = []
        cur = node
        while True:
            cur = strip_parens_and_casts(cur, ("ArrayToPointerDecay", "NoOp"))
            if cur["kind"] != "ArraySubscriptExpr":
                break
            base, index = cur["inner"]
            indices.append(index)
            cur = base
        base = strip_parens_and_casts(cur, ("LValueToRValue", "NoOp"))
        if base["kind"] != "DeclRefExpr":
            raise Unsupported(node, "array accesses must subscript a "
                                    "parameter directly (no pointer "
                                    "arithmetic)")
        entry = self.env.get(base.get("referencedDecl", {}).get("id"))
        if not entry or entry[0] != "memref":
            raise Unsupported(base, "'%s' is not an array parameter" %
                              base.get("referencedDecl", {}).get("name", "?"))
        if len(indices) != entry[2]:
            raise Unsupported(node, "'%s' has %d dimensions but is "
                                    "subscripted %d times" %
                              (base["referencedDecl"]["name"], entry[2],
                               len(indices)))
        indices.reverse()
        subscripts = [self.affine_index(i) for i in indices]
        param = base["referencedDecl"]["id"]
        info = self.accesses.setdefault(
            param, {"read": False, "written": False, "ranges": None})
        info["read"] |= read
        info["written"] |= written
        ranges = [self.form_range(form) for _, form in subscripts]
        if all(r is not None for r in ranges):
            if info["ranges"] is None:
                info["ranges"] = ranges
            else:
                info["ranges"] = [(min(a[0], b[0]), max(a[1], b[1]))
                                  for a, b in zip(info["ranges"], ranges)]
        return entry, [text for text, _ in subscripts]

    def float_constant(self, value, elem, node):
        if elem == "f32":
            bits = struct.unpack("<I", struct.pack("<f", value))[0]
            return "0x%08X" % bits
        bits = struct.unpack("<Q", struct.pack("<d", value))[0]
        return "0x%016X" % bits

    def value(self, node, indent):
        """Emit `node` (an rvalue of floating type); return (ssa, elem)."""
        kind = node["kind"]
        t = strip_qualifiers(qual_type(node))
        if kind == "ParenExpr":
            return self.value(node["inner"][0], indent)
        if kind == "ImplicitCastExpr" and node["castKind"] == "LValueToRValue":
            target = strip_parens_and_casts(node["inner"][0], ("NoOp",))
            if target["kind"] == "ArraySubscriptExpr":
                return self.load(target, indent)
            if target["kind"] == "DeclRefExpr":
                entry = self.env.get(target.get("referencedDecl", {}).get("id"))
                if entry and entry[0] == "scalar":
                    return entry[1], entry[2]
            raise Unsupported(node, "only array elements and scalar float "
                                    "parameters can be read")
        if t not in FLOAT_TYPES:
            raise Unsupported(node, "expression of type '%s' is not "
                                    "supported in arithmetic (float/double "
                                    "only)" % qual_type(node))
        elem = FLOAT_TYPES[t]
        if kind == "FloatingLiteral":
            name = self.fresh()
            self.emit(indent, "%s = arith.constant %s : %s %s" %
                      (name, self.float_constant(float(node["value"]), elem,
                                                 node), elem, self.loc(node)))
            return name, elem
        if kind == "ImplicitCastExpr" and node["castKind"] == \
                "IntegralToFloating":
            try:
                v = self.int_constant(node["inner"][0])
            except Unsupported:
                raise Unsupported(node, "converting an integer variable to "
                                        "floating point is not supported "
                                        "(only integer constants)")
            if abs(v) > 2**24:
                raise Unsupported(node, "integer constant %d is not exactly "
                                        "representable here" % v)
            name = self.fresh()
            self.emit(indent, "%s = arith.constant %s : %s %s" %
                      (name, self.float_constant(float(v), elem, node), elem,
                       self.loc(node)))
            return name, elem
        if kind == "ImplicitCastExpr" and node["castKind"] == "FloatingCast":
            src, src_elem = self.value(node["inner"][0], indent)
            return self.convert(src, src_elem, elem, node, indent), elem
        if kind == "UnaryOperator" and node["opcode"] == "-":
            src, _ = self.value(node["inner"][0], indent)
            name = self.fresh()
            self.emit(indent, "%s = arith.negf %s : %s %s" %
                      (name, src, elem, self.loc(node)))
            return name, elem
        if kind == "BinaryOperator" and node["opcode"] in "+-*/":
            a, _ = self.value(node["inner"][0], indent)
            b, _ = self.value(node["inner"][1], indent)
            return self.arith(node["opcode"], a, b, elem, node, indent), elem
        raise Unsupported(node, "expression '%s' is not supported" % kind)

    def arith(self, opcode, a, b, elem, node, indent):
        op = {"+": "addf", "-": "subf", "*": "mulf", "/": "divf"}[opcode]
        name = self.fresh()
        self.emit(indent, "%s = arith.%s %s, %s : %s %s" %
                  (name, op, a, b, elem, self.loc(node)))
        return name

    def convert(self, value, src, dst, node, indent):
        if src == dst:
            return value
        op = "extf" if (src, dst) == ("f32", "f64") else "truncf"
        name = self.fresh()
        self.emit(indent, "%s = arith.%s %s : %s to %s %s" %
                  (name, op, value, src, dst, self.loc(node)))
        return name

    def load(self, node, indent):
        entry, idx = self.array_access(node, read=True)
        name = self.fresh()
        self.emit(indent, "%s = affine.load %s[%s] : %s %s" %
                  (name, entry[1], ", ".join(idx), entry[4], self.loc(node)))
        return name, entry[3]

    def assignment(self, s, indent):
        opcode = s.get("opcode")
        if opcode not in ("=", "+=", "-=", "*=", "/="):
            raise Unsupported(s, "expression statement '%s' is not supported "
                                 "(only assignments to array elements)" %
                              (opcode or s["kind"]))
        lhs, rhs = s["inner"]
        lhs_node = strip_parens_and_casts(lhs, ("NoOp",))
        if lhs_node["kind"] != "ArraySubscriptExpr":
            raise Unsupported(lhs, "only array elements can be assigned")
        entry, idx = self.array_access(lhs_node, written=True,
                                       read=(opcode != "="))
        elem = entry[3]
        if opcode == "=":
            v, v_elem = self.value(rhs, indent)
            if v_elem != elem:
                raise Unsupported(rhs, "assigned value has type %s, element "
                                       "is %s" % (v_elem, elem))
        else:
            # E1 op= E2 is E1 = E1 op E2 with E1 evaluated once, computed in
            # clang's computation type and converted back to E1's type.
            compute = FLOAT_TYPES.get(strip_qualifiers(
                s.get("computeResultType", {}).get("qualType", "")))
            if compute is None:
                raise Unsupported(s, "compound assignment on a non-floating "
                                     "type")
            old = self.fresh()
            self.emit(indent, "%s = affine.load %s[%s] : %s %s" %
                      (old, entry[1], ", ".join(idx), entry[4],
                       self.loc(lhs_node)))
            old = self.convert(old, elem, compute, s, indent)
            r, r_elem = self.value(rhs, indent)
            r = self.convert(r, r_elem, compute, s, indent)
            v = self.arith(opcode[0], old, r, compute, s, indent)
            v = self.convert(v, compute, elem, s, indent)
        self.emit(indent, "affine.store %s, %s[%s] : %s %s" %
                  (v, entry[1], ", ".join(idx), entry[4], self.loc(s)))


def main():
    parser = argparse.ArgumentParser(
        description="Translate the affine subset of C into MLIR "
                    "(func/affine/arith/memref).")
    parser.add_argument("source")
    parser.add_argument("-o", "--output", default="-")
    parser.add_argument("--clang", default=os.environ.get("SYSTOLIC_CFRONT_CLANG"),
                        help="the clang executable (default: "
                             "$SYSTOLIC_CFRONT_CLANG); required")
    parser.add_argument("--clang-major", type=int, default=24,
                        help="the clang major version required (default 24, "
                             "the project's LLVM/MLIR)")
    parser.add_argument("--function", action="append", default=[],
                        help="translate only these functions (default: every "
                             "function defined in the file)")
    parser.add_argument("-D", dest="defines", action="append", default=[])
    parser.add_argument("-I", dest="includes", action="append", default=[])
    parser.add_argument("-std", dest="std", default="c11")
    parser.add_argument("--skip-unsupported", action="store_true",
                        help="translate the functions inside the subset and "
                             "report the others as notes (they stay C), "
                             "instead of failing on the first one")
    parser.add_argument("--symbol-suffix", default="",
                        help="append this to every emitted function's name")
    parser.add_argument("--interface",
                        help="write the translated functions' C interface "
                             "(parameters, shapes, accesses, source ranges) "
                             "to this JSON file")
    args = parser.parse_args()

    if not args.clang:
        print("systolic-cfront: no clang given (--clang or "
              "SYSTOLIC_CFRONT_CLANG)", file=sys.stderr)
        return 2
    try:
        version = subprocess.run([args.clang, "--version"], capture_output=True,
                                 text=True).stdout.splitlines()[0]
    except (OSError, IndexError) as e:
        print("systolic-cfront: cannot run %s --version: %s" % (args.clang, e),
              file=sys.stderr)
        return 2
    m = re.search(r"clang version (\d+)\.", version)
    if not m or int(m.group(1)) != args.clang_major:
        print("systolic-cfront: %s is '%s', not clang %d; refusing to use it "
              "(--clang-major selects another version deliberately)" %
              (args.clang, version, args.clang_major), file=sys.stderr)
        return 2

    cmd = [args.clang, "-x", "c", "-std=" + args.std, "-fsyntax-only",
           "-Xclang", "-ast-dump=json"]
    cmd += ["-D" + d for d in args.defines] + ["-I" + i for i in args.includes]
    cmd.append(args.source)
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True)
    except OSError as e:
        print("systolic-cfront: cannot run %s: %s" % (args.clang, e),
              file=sys.stderr)
        return 2
    if proc.returncode != 0:
        sys.stderr.write(proc.stderr)
        print("systolic-cfront: clang rejected %s" % args.source,
              file=sys.stderr)
        return 2
    if proc.stderr:
        sys.stderr.write(proc.stderr)   # clang's warnings
    ast = json.loads(proc.stdout)

    source_map = SourceMap(args.source)
    display = os.path.basename(args.source)
    main_file = os.path.realpath(args.source)

    # Top-level declarations of the main file. The dumper names a file only
    # when it changes from the previous node's, so track it in order.
    current_file = None
    functions = []
    for decl in ast.get("inner", []):
        loc = decl.get("loc", {})
        loc = loc.get("expansionLoc", loc)
        if "file" in loc:
            current_file = os.path.realpath(loc["file"])
        if decl.get("isImplicit") or current_file != main_file:
            continue
        if decl.get("kind") != "FunctionDecl":
            continue
        if not any(n["kind"] == "CompoundStmt" for n in decl.get("inner", [])):
            continue   # a prototype
        if args.function and decl["name"] not in args.function:
            continue
        functions.append(decl)

    missing = set(args.function) - {f["name"] for f in functions}
    if missing:
        print("systolic-cfront: no definition of %s in %s" %
              (", ".join(sorted(missing)), args.source), file=sys.stderr)
        return 2

    tr = Translator(args.source, display, source_map)
    tr.emit(0, "// Generated by systolic-cfront from %s." % display)
    tr.emit(0, "// C front end: %s (-std=%s%s)." %
            (version, args.std,
             "".join(" -D" + d for d in args.defines)))
    tr.emit(0, "module {")
    tr.symbol_suffix = args.symbol_suffix
    for fn in functions:
        mark = len(tr.lines)
        try:
            tr.function(fn)
        except Unsupported as e:
            offset = node_offset(e.node)
            line, col = source_map.line_col(offset) if offset is not None \
                else (0, 0)
            if not args.skip_unsupported:
                print("%s:%d:%d: error: %s" % (args.source, line, col,
                                               e.message), file=sys.stderr)
                return 1
            # The function stays C; nothing of it is emitted.
            del tr.lines[mark:]
            print("%s:%d:%d: note: '%s' is outside the supported C subset "
                  "and is not translated: %s" %
                  (args.source, line, col, fn["name"], e.message),
                  file=sys.stderr)
    tr.emit(0, "}")

    if args.interface:
        with open(args.interface, "w") as f:
            json.dump({"source": os.path.realpath(args.source),
                       "clang": version, "functions": tr.interfaces}, f,
                      indent=1)

    text = "\n".join(tr.lines) + "\n"
    if args.output == "-":
        sys.stdout.write(text)
    else:
        with open(args.output, "w") as f:
            f.write(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
