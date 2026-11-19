#!/usr/bin/env python3
"""Differential test of the G1 semantic model.

Random CFG functions are generated as Python data, printed as MLIR for the
importer, and executed three ways:

  1. By this file's interpreter, which works on the operations as generated,
     with Python integers and hardware floats. It shares no code with the C++
     library, and it does not know about terms, transitions or shadows.
  2. By the concrete evaluator, through generalized-gemm-report --run.
  3. With --native: by real machine code, through mlir-opt, mlir-translate and
     clang, for the runs the interpreter says are defined.

Any disagreement in status, step count, return values, final memory or the MAC
event sequence fails the test. The generator never multiplies two loaded values
outside the MAC template, so every event comes from a template it knows.

An uninitialized read stops this interpreter at the load. The evaluator checks a
block's side conditions first, so a later undefined operation in the same block
may be reported instead. Either way the run has no defined result, so that one
comparison accepts any of the three stopping statuses.
"""
import argparse
import ctypes
import json
import math
from pathlib import Path
import random
import struct
import subprocess
import sys
import tempfile

INDEX_BITS = 64
FUEL = 3000
MODULE_SIZE = 100
BINARY = {
    "addi": "ov", "subi": "ov", "muli": "ov", "shli": "ov",
    "divsi": "exact", "divui": "exact", "shrsi": "exact", "shrui": "exact",
    "remsi": "", "remui": "", "andi": "", "ori": "", "xori": "",
    "minsi": "", "maxsi": "", "minui": "", "maxui": "",
}
PREDICATES = ["eq", "ne", "slt", "sle", "sgt", "sge", "ult", "ule", "ugt", "uge"]


def signed(value, width):
    return value - (1 << width) if value >> (width - 1) & 1 else value


def wrap(value, width):
    return value & ((1 << width) - 1)


def compare(predicate, a, b, width):
    if predicate in ("eq", "ne"):
        return (a == b) == (predicate == "eq")
    if predicate[0] == "s":
        a, b = signed(a, width), signed(b, width)
    return {"lt": a < b, "le": a <= b, "gt": a > b, "ge": a >= b}[predicate[1:]]


def trunc_div(a, b):
    quotient = abs(a) // abs(b)
    return quotient if (a < 0) == (b < 0) else -quotient


def integer_binary(op, a, b, width):
    """Value, overflow-flag violations, and whether the amount is too large."""
    sa, sb = signed(a, width), signed(b, width)
    nsw = nuw = False
    if op in ("addi", "subi", "muli"):
        s = {"addi": sa + sb, "subi": sa - sb, "muli": sa * sb}[op]
        u = {"addi": a + b, "subi": a - b, "muli": a * b}[op]
        nsw = not -(1 << (width - 1)) <= s < (1 << (width - 1))
        nuw = not 0 <= u < (1 << width)
        return wrap(u, width), nsw, nuw
    if op == "andi":
        return a & b, False, False
    if op == "ori":
        return a | b, False, False
    if op == "xori":
        return a ^ b, False, False
    if op == "minsi":
        return (a if sa < sb else b), False, False
    if op == "maxsi":
        return (a if sa > sb else b), False, False
    if op == "minui":
        return (a if a < b else b), False, False
    if op == "maxui":
        return (a if a > b else b), False, False
    if op == "shli":
        if b >= width:
            return 0, False, False
        value = wrap(a << b, width)
        return value, signed(value, width) >> b != sa, value >> b != a
    if op == "shrui":
        return (0 if b >= width else a >> b), False, False
    if op == "shrsi":
        return wrap(sa >> min(b, width), width), False, False
    if op == "divui":
        return ((1 << width) - 1 if b == 0 else a // b), False, False
    if op == "remui":
        return (a if b == 0 else a % b), False, False
    if op == "divsi":
        if b == 0:
            return wrap(1 if sa < 0 else -1, width), False, False
        return wrap(trunc_div(sa, sb), width), False, False
    if op == "remsi":
        if b == 0:
            return a, False, False
        return wrap(sa - sb * trunc_div(sa, sb), width), False, False
    raise AssertionError(op)


def float_bits(value):
    return struct.unpack("<I", struct.pack("<f", value))[0]


def float_value(bits):
    return struct.unpack("<f", struct.pack("<I", bits))[0]


def round32(value):
    return ctypes.c_float(value).value


class Function:
    """One random function: blocks of plain operations."""

    def __init__(self, rng, name, native):
        self.rng = rng
        self.name = name
        self.width = rng.choice([8, 16, 32] if native else [1, 2, 3, 4, 5, 8, 16, 32])
        self.fresh = 0
        self.params = [(f"p{i}", self.int_type()) for i in range(rng.randint(1, 3))]
        self.roots = [
            {"name": "m0", "shape": [4], "element": self.int_type(), "dynamic": False},
            {"name": "m1", "shape": [2, 3], "element": self.int_type(), "dynamic": False},
            {"name": "m2", "shape": [3], "element": "f32", "dynamic": False},
        ]
        if rng.random() < 0.3:
            self.roots.append({"name": "m3", "shape": [rng.randint(1, 3)],
                               "element": self.int_type(), "dynamic": True})
        self.alloca = rng.random() < 0.3
        self.loaded = set()
        self.blocks = []
        self.generate()

    def int_type(self):
        return f"i{self.width}"

    def new(self):
        self.fresh += 1
        return f"v{self.fresh}"

    def emit(self, block, op, result=None, **fields):
        block["ops"].append(dict(op=op, result=result, **fields))
        return result

    def flags(self, kind):
        chance = self.rng.random()
        if kind == "ov":
            return self.rng.choice([["nsw"], ["nuw"], ["nsw", "nuw"]]) if chance < 0.12 else []
        if kind == "exact":
            return ["exact"] if chance < 0.08 else []
        if kind == "nneg":
            return ["nneg"] if chance < 0.1 else []
        if kind == "fast":
            return self.rng.sample(["reassoc", "nnan", "ninf", "nsz", "contract"], self.rng.randint(1, 2)) if chance < 0.2 else []
        return []

    def generate(self):
        rng = self.rng
        count = rng.randint(2, 6)
        for b in range(count):
            arguments = [] if b == 0 else [(f"a{b}_{i}", self.int_type()) for i in range(rng.randint(0, 2))]
            self.blocks.append(dict(args=arguments, ops=[], term=None))
        entry = self.blocks[0]
        shared = {self.int_type(): [p for p, _ in self.params], "i1": [], "index": [], "f32": []}
        for value in rng.sample([0, 1, 2, 3, -1], 3):
            shared[self.int_type()].append(self.emit(entry, "constant", self.new(), type=self.int_type(), value=wrap(value, self.width)))
        self.size = {}
        for size in (1, 2, 3, 4):
            self.size[size] = self.emit(entry, "constant", self.new(), type="index", value=size)
        shared["index"].append(self.size[1])
        if self.alloca:
            self.emit(entry, "alloca", "m4", type=self.int_type())
        for root in self.roots:
            if root["dynamic"]:
                zero = self.emit(entry, "constant", self.new(), type="index", value=0)
                shared["index"].append(self.emit(entry, "dim", self.new(), root=root["name"], index=zero))
        for b, block in enumerate(self.blocks):
            pool = {t: list(v) for t, v in shared.items()}
            pool[self.int_type()] += [a for a, _ in block["args"]]
            for _ in range(rng.randint(0, 8)):
                self.random_op(block, pool)
            if rng.random() < 0.25:
                self.mac(block, pool)
            block["term"] = self.terminator(b, pool)

    def int_value(self, pool):
        return self.rng.choice(pool[self.int_type()])

    def index_value(self, block, pool):
        rng = self.rng
        value = self.emit(block, rng.choice(["index_cast", "index_castui"]), self.new(), source=self.int_value(pool),
                          source_type=self.int_type(), type="index", flags=[])
        if block["ops"][-1]["op"] == "index_castui":
            block["ops"][-1]["flags"] = self.flags("nneg")
        return value

    def bounded(self, block, pool, size):
        value = self.index_value(block, pool)
        if self.rng.random() < 0.9:
            value = self.emit(block, "remui", self.new(), lhs=value, rhs=self.size[size], type="index", flags=[])
        return value

    def access(self, block, pool, root):
        if root == "m0":
            return [self.bounded(block, pool, 4)]
        if root == "m1":
            return [self.bounded(block, pool, 2), self.bounded(block, pool, 3)]
        if root == "m2":
            return [self.bounded(block, pool, 3)]
        if root == "m3":
            return [self.bounded(block, pool, 1)]
        return [self.bounded(block, pool, 4)]

    def memory_type(self, root):
        for r in self.roots:
            if r["name"] == root:
                return f"memref<{'x'.join('?' if r['dynamic'] else str(s) for s in r['shape'])}x{r['element']}>"
        return f"memref<4x{self.int_type()}>"

    def int_roots(self):
        names = ["m0", "m1"]
        if len(self.roots) == 4:
            names.append("m3")
        if self.alloca:
            names.append("m4")
        return names

    def random_op(self, block, pool):
        rng = self.rng
        w, t = self.width, self.int_type()
        choice = rng.random()
        if choice < 0.45:
            op = rng.choice(list(BINARY))
            a, b = self.int_value(pool), self.int_value(pool)
            if op == "muli" and a in self.loaded and b in self.loaded:
                return
            if op in ("divsi", "divui", "remsi", "remui") and rng.random() < 0.8:
                one = next(iter(shared_constant(self, block, 1)))
                b = self.emit(block, "ori", self.new(), lhs=b, rhs=one, type=t, flags=[])
                pool[t].append(b)
            pool[t].append(self.emit(block, op, self.new(), lhs=a, rhs=b, type=t, flags=self.flags(BINARY[op])))
        elif choice < 0.6:
            result = self.emit(block, "cmpi", self.new(), predicate=rng.choice(PREDICATES), lhs=self.int_value(pool), rhs=self.int_value(pool), type=t)
            pool["i1"].append(result)
            if rng.random() < 0.5:
                pool[t].append(self.emit(block, "select", self.new(), cond=result, lhs=self.int_value(pool), rhs=self.int_value(pool), type=t))
        elif choice < 0.72:
            wide = f"i{w + 3}"
            kind = rng.choice(["extsi", "extui"])
            value = self.emit(block, kind, self.new(), source=self.int_value(pool), source_type=t, type=wide,
                              flags=self.flags("nneg") if kind == "extui" else [])
            pool[t].append(self.emit(block, "trunci", self.new(), source=value, source_type=wide, type=t, flags=self.flags("ov")))
        elif choice < 0.87:
            root = rng.choice(self.int_roots())
            index = self.access(block, pool, root)
            if rng.random() < 0.5:
                result = self.emit(block, "load", self.new(), root=root, index=index, type=t)
                self.loaded.add(result)
                pool[t].append(result)
            else:
                self.emit(block, "store", None, root=root, index=index, value=self.int_value(pool))
        else:
            a = self.emit(block, "load", self.new(), root="m2", index=self.access(block, pool, "m2"), type="f32")
            b = self.emit(block, "load", self.new(), root="m2", index=self.access(block, pool, "m2"), type="f32")
            if rng.random() < 0.5:
                pool["f32"].append(a)
            c = self.emit(block, rng.choice(["addf", "mulf"]), self.new(), lhs=a, rhs=b, type="f32", flags=self.flags("fast"))
            if rng.random() < 0.5:
                self.emit(block, "store", None, root="m2", index=self.access(block, pool, "m2"), value=c)

    def mac(self, block, pool):
        rng = self.rng
        floating = rng.random() < 0.4
        if floating:
            rc, ra, rb = "m2", "m2", "m2"
        else:
            rc, ra, rb = "m0", "m1", "m0"
        t = "f32" if floating else self.int_type()
        h, f, g = self.access(block, pool, rc), self.access(block, pool, ra), self.access(block, pool, rb)
        c = self.emit(block, "load", self.new(), root=rc, index=h, type=t)
        a = self.emit(block, "load", self.new(), root=ra, index=f, type=t)
        b = self.emit(block, "load", self.new(), root=rb, index=g, type=t)
        if floating:
            product = self.emit(block, "mulf", self.new(), lhs=a, rhs=b, type=t, flags=self.flags("fast"))
            total = self.emit(block, "addf", self.new(), lhs=c, rhs=product, type=t, flags=self.flags("fast"))
        else:
            product = self.emit(block, "muli", self.new(), lhs=a, rhs=b, type=t, flags=self.flags("ov"))
            total = self.emit(block, "addi", self.new(), lhs=product, rhs=c, type=t, flags=self.flags("ov"))
        self.emit(block, "store", None, root=rc, index=h, value=total, mac=dict(f=f, g=g, h=h, a=a, b=b, update=total))

    def arguments_for(self, target, pool):
        return [self.int_value(pool) for _ in self.blocks[target]["args"]]

    def terminator(self, b, pool):
        rng = self.rng
        targets = list(range(1, len(self.blocks)))
        choice = rng.random()
        if b == len(self.blocks) - 1 and choice < 0.6 or choice < 0.1:
            return dict(kind="return", value=self.int_value(pool))
        if choice < 0.35:
            t = rng.choice(targets)
            return dict(kind="br", target=t, args=self.arguments_for(t, pool))
        if choice < 0.85:
            if not pool["i1"]:
                block = self.blocks[b]
                pool["i1"].append(self.emit(block, "cmpi", self.new(), predicate=rng.choice(PREDICATES),
                                            lhs=self.int_value(pool), rhs=self.int_value(pool), type=self.int_type()))
            x, y = rng.choice(targets), rng.choice(targets)
            return dict(kind="cond", cond=rng.choice(pool["i1"]), yes=x, yes_args=self.arguments_for(x, pool),
                        no=y, no_args=self.arguments_for(y, pool))
        d = rng.choice(targets)
        cases = []
        for value in rng.sample(range(1 << min(self.width, 3)), min(2, 1 << min(self.width, 3))):
            t = rng.choice(targets)
            cases.append((value, t, self.arguments_for(t, pool)))
        return dict(kind="switch", flag=self.int_value(pool), default=d, default_args=self.arguments_for(d, pool), cases=cases)

    # Printing.

    def render_op(self, o):
        r = f"%{o['result']} = " if o["result"] else ""
        op, t = o["op"], o.get("type")
        attrs = ""
        flags = o.get("flags", [])
        if op == "constant":
            return f"{r}arith.constant {signed_for_print(o['value'], t)} : {t}"
        if op in BINARY or op in ("addf", "mulf"):
            if op in ("addf", "mulf"):
                attrs = f" fastmath<{','.join(flags)}>" if flags else ""
            elif BINARY[op] == "ov" and flags:
                attrs = f" overflow<{','.join(flags)}>"
            elif flags:
                attrs = " exact"
            return f"{r}arith.{op} %{o['lhs']}, %{o['rhs']}{attrs} : {t}"
        if op == "cmpi":
            return f"{r}arith.cmpi {o['predicate']}, %{o['lhs']}, %{o['rhs']} : {t}"
        if op == "select":
            return f"{r}arith.select %{o['cond']}, %{o['lhs']}, %{o['rhs']} : {t}"
        if op in ("extsi", "extui", "trunci", "index_cast", "index_castui"):
            if flags:
                attrs = f" overflow<{','.join(flags)}>" if op == "trunci" else " nneg"
            return f"{r}arith.{op} %{o['source']}{attrs} : {o['source_type']} to {t}"
        if op == "load":
            return f"{r}memref.load %{o['root']}[{', '.join('%' + i for i in o['index'])}] : {self.memory_type(o['root'])}"
        if op == "store":
            return f"memref.store %{o['value']}, %{o['root']}[{', '.join('%' + i for i in o['index'])}] : {self.memory_type(o['root'])}"
        if op == "alloca":
            return f"%{o['result']} = memref.alloca() : {self.memory_type('m4')}"
        if op == "dim":
            return f"{r}memref.dim %{o['root']}, %{o['index']} : {self.memory_type(o['root'])}"
        raise AssertionError(op)

    def edge(self, target, args):
        if not args:
            return f"^b{target}"
        return f"^b{target}(" + ", ".join("%" + a for a in args) + " : " + ", ".join([self.int_type()] * len(args)) + ")"

    def render_term(self, term):
        t = self.int_type()
        if term["kind"] == "return":
            return f"return %{term['value']} : {t}"
        if term["kind"] == "br":
            return "cf.br " + self.edge(term["target"], term["args"])
        if term["kind"] == "cond":
            return f"cf.cond_br %{term['cond']}, {self.edge(term['yes'], term['yes_args'])}, {self.edge(term['no'], term['no_args'])}"
        cases = ", ".join(f"{signed_for_print(v, t)}: {self.edge(b, a)}" for v, b, a in term["cases"])
        return f"cf.switch %{term['flag']} : {t}, [default: {self.edge(term['default'], term['default_args'])}, {cases}]"

    def signature(self):
        parts = [f"%{p}: {t}" for p, t in self.params]
        parts += [f"%{r['name']}: {self.memory_type(r['name'])}" for r in self.roots]
        return ", ".join(parts)

    def render(self):
        lines = [f"func.func @{self.name}({self.signature()}) -> {self.int_type()} {{"]
        for b, block in enumerate(self.blocks):
            if b:
                arguments = ", ".join(f"%{a}: {t}" for a, t in block["args"])
                lines.append(f"^b{b}({arguments}):" if arguments else f"^b{b}:")
            lines += ["  " + self.render_op(o) for o in block["ops"]]
            lines.append("  " + self.render_term(block["term"]))
        lines.append("}")
        return "\n".join(lines)

    # The independent interpreter.

    def successors(self, term):
        if term["kind"] == "br":
            return [term["target"]]
        if term["kind"] == "cond":
            return [term["yes"], term["no"]]
        if term["kind"] == "switch":
            return [term["default"]] + [c[1] for c in term["cases"]]
        return []

    def uses(self, block):
        names = set()
        for o in block["ops"]:
            for key in ("lhs", "rhs", "cond", "source", "value"):
                if isinstance(o.get(key), str):
                    names.add(o[key])
            names.update(o.get("index", []) if isinstance(o.get("index"), list) else [])
            if o["op"] == "dim":
                names.add(o["index"])
        term = block["term"]
        for key in ("value", "cond", "flag"):
            if key in term:
                names.add(term[key])
        for key in ("args", "yes_args", "no_args", "default_args"):
            names.update(term.get(key, []))
        for case in term.get("cases", []):
            names.update(case[2])
        return names

    def layout(self):
        reachable, work = set(), [0]
        while work:
            b = work.pop()
            if b not in reachable:
                reachable.add(b)
                work += self.successors(self.blocks[b]["term"])
        carried = set()
        for b in reachable:
            if b:
                carried.update(a for a, _ in self.blocks[b]["args"])
            defined = {o["result"] for o in self.blocks[b]["ops"] if o["result"]}
            for other in reachable:
                if other != b:
                    carried |= defined & self.uses(self.blocks[other])
        return carried

    def run(self, params, memory):
        """Returns status, steps, returns, memory, events."""
        carried = self.layout()
        env = {p: (v, False) for (p, _), v in zip(self.params, params)}
        memory = {k: list(v) for k, v in memory.items()}
        if self.alloca:
            memory["m4"] = [None] * 4
        shapes = {r["name"]: r["shape"] for r in self.roots}
        shapes["m4"] = [4]
        persistent = {}
        seen = set()
        block, steps = 0, 0
        events = []

        def stop(status):
            return dict(status=status, steps=steps, returns=None, memory=None, events=None)

        while True:
            if steps >= FUEL:
                return stop("OUT_OF_FUEL")
            key = (block, tuple(sorted(persistent.items())), tuple((k, tuple(map(str, v))) for k, v in sorted(memory.items())))
            if key in seen:
                return stop("DIVERGED")
            seen.add(key)
            local = dict(env)
            local.update(persistent)
            pending_writes_events = []

            def value(name):
                return local[name]

            for o in self.blocks[block]["ops"]:
                op = o["op"]
                if op == "constant":
                    local[o["result"]] = (o["value"], False)
                elif op == "alloca":
                    pass
                elif op == "dim":
                    local[o["result"]] = (shapes[o["root"]][0], False)
                elif op in BINARY:
                    width = INDEX_BITS if o["type"] == "index" else int(o["type"][1:])
                    a, pa = value(o["lhs"])
                    b, pb = value(o["rhs"])
                    if op in ("divsi", "divui", "remsi", "remui"):
                        if pb or b == 0:
                            return stop("UB")
                        minus_one = b == (1 << width) - 1
                        overflow = minus_one and (pa or a == 1 << (width - 1))
                        if op == "divsi" and overflow:
                            return stop("UB")
                        if op == "remsi" and overflow:
                            return stop("UNDERSPECIFIED")
                    result, nsw, nuw = integer_binary(op, a, b, width)
                    poison = pa or pb
                    flags = o.get("flags", [])
                    if op in ("shli", "shrsi", "shrui") and b >= width:
                        poison = True
                    if "nsw" in flags and nsw:
                        poison = True
                    if "nuw" in flags and nuw:
                        poison = True
                    if "exact" in flags:
                        if op == "divsi" and integer_binary("remsi", a, b, width)[0] != 0:
                            poison = True
                        if op == "divui" and a % b != 0:
                            poison = True
                        if op in ("shrsi", "shrui") and wrap(a, min(b, width)) != 0:
                            poison = True
                    local[o["result"]] = (result, poison)
                elif op == "cmpi":
                    width = int(o["type"][1:])
                    a, pa = value(o["lhs"])
                    b, pb = value(o["rhs"])
                    local[o["result"]] = (int(compare(o["predicate"], a, b, width)), pa or pb)
                elif op == "select":
                    c, pc = value(o["cond"])
                    chosen = value(o["lhs"] if c else o["rhs"])
                    local[o["result"]] = (chosen[0], pc or chosen[1])
                elif op in ("extsi", "extui", "trunci", "index_cast", "index_castui"):
                    source, poison = value(o["source"])
                    width = INDEX_BITS if o["source_type"] == "index" else int(o["source_type"][1:])
                    target = INDEX_BITS if o["type"] == "index" else int(o["type"][1:])
                    extended = wrap(signed(source, width), target) if op in ("extsi", "index_cast") else wrap(source, target)
                    flags = o.get("flags", [])
                    if "nneg" in flags and source >> (width - 1) & 1:
                        poison = True
                    if op == "trunci":
                        if "nuw" in flags and extended != source:
                            poison = True
                        if "nsw" in flags and wrap(signed(extended, target), width) != source:
                            poison = True
                    local[o["result"]] = (extended, poison)
                elif op in ("load", "store"):
                    shape = shapes[o["root"]]
                    index = [value(i) for i in o["index"]]
                    if any(p for _, p in index):
                        return stop("UB")
                    if any(i >= s for (i, _), s in zip(index, shape)):
                        return stop("UB")
                    offset = 0
                    for (i, _), s in zip(index, shape):
                        offset = offset * s + i
                    if op == "load":
                        cell = memory[o["root"]][offset]
                        # Reads see writes earlier in this block.
                        for root, position, written in reversed(pending_writes_events):
                            if root == o["root"] and position == offset:
                                cell = written
                                break
                        if cell is None:
                            return stop("UNINITIALIZED")
                        local[o["result"]] = (cell, False)
                    else:
                        stored, poison = value(o["value"])
                        if poison:
                            return stop("MODEL_LIMIT")
                        pending_writes_events.append((o["root"], offset, stored))
                        if "mac" in o:
                            m = o["mac"]
                            events.append(dict(f=[str(value(i)[0]) for i in m["f"]], g=[str(value(i)[0]) for i in m["g"]],
                                               h=[str(value(i)[0]) for i in m["h"]], a=str(value(m["a"])[0]),
                                               b=str(value(m["b"])[0]), update=str(value(m["update"])[0])))
                elif op in ("addf", "mulf"):
                    a, pa = value(o["lhs"])
                    b, pb = value(o["rhs"])
                    x, y = float_value(a), float_value(b)
                    r = round32(x + y if op == "addf" else x * y)
                    flags = o.get("flags", [])
                    poison = pa or pb
                    if "nnan" in flags and any(math.isnan(v) for v in (x, y, r)):
                        poison = True
                    if "ninf" in flags and any(math.isinf(v) for v in (x, y, r)):
                        poison = True
                    if not poison and math.isnan(r):
                        return stop("UNDERSPECIFIED")
                    if not poison and "nsz" in flags and r == 0:
                        return stop("UNDERSPECIFIED")
                    local[o["result"]] = (float_bits(r) if not math.isnan(r) else 0, poison)
                else:
                    raise AssertionError(op)

            term = self.blocks[block]["term"]
            for root, position, written in pending_writes_events:
                memory[root][position] = written

            def pick(names):
                return [local[n] for n in names]

            for o in self.blocks[block]["ops"]:
                if o["result"] in carried:
                    persistent[o["result"]] = local[o["result"]]
            if term["kind"] == "return":
                result, poison = local[term["value"]]
                steps += 1
                return dict(status="RETURNED", steps=steps, returns=["poison" if poison else str(result)], memory=memory, events=events)
            if term["kind"] == "br":
                target, values = term["target"], pick(term["args"])
            elif term["kind"] == "cond":
                c, poison = local[term["cond"]]
                if poison:
                    return stop("UB")
                target, values = (term["yes"], pick(term["yes_args"])) if c else (term["no"], pick(term["no_args"]))
            else:
                flag, poison = local[term["flag"]]
                if poison:
                    return stop("UB")
                target, values = term["default"], pick(term["default_args"])
                for case, case_target, case_args in term["cases"]:
                    if case == flag:
                        target, values = case_target, pick(case_args)
                        break
            for (name, _), v in zip(self.blocks[target]["args"], values):
                persistent[name] = v
            block = target
            steps += 1


def signed_for_print(value, type_name):
    if type_name == "index":
        return value
    return signed(value, int(type_name[1:]))


def shared_constant(function, block, value):
    name = function.new()
    function.emit(block, "constant", name, type=function.int_type(), value=wrap(value, function.width))
    yield name


FLOATS = [0.0, -0.0, 1.0, -1.0, 1.5, 0.1, 1e30, -1e30, 1e-40, float("inf"), float("-inf"), float("nan")]


def random_entry(rng, f):
    params = [rng.getrandbits(f.width) if rng.random() < 0.7 else rng.choice([0, 1, wrap(-1, f.width), 1 << (f.width - 1)]) for _ in f.params]
    memory = {}
    for r in f.roots:
        cells = 1
        for s in r["shape"]:
            cells *= s
        if r["element"] == "f32":
            memory[r["name"]] = [float_bits(rng.choice(FLOATS)) for _ in range(cells)]
        else:
            memory[r["name"]] = [rng.getrandbits(f.width) for _ in range(cells)]
    return params, memory


def entry_json(f, params, memory):
    parameters = [str(p) for p in params]
    # Dimension parameters come after the scalars, in root order.
    roots = list(f.roots)
    for r in roots:
        if r["dynamic"]:
            parameters.append(str(r["shape"][0]))
    cells = [dict(shape=r["shape"], cells=[str(c) for c in memory[r["name"]]]) for r in roots]
    if f.alloca:
        cells.append(dict(shape=[4], cells=[None] * 4))
    return dict(function=f.name, parameters=parameters, memory=cells)


def fail(f, params, memory, oracle, got, why):
    print(f"MISMATCH ({why})\n{f.render()}\nparams={params} memory={memory}\noracle={oracle}\nevaluator={got}", file=sys.stderr)
    raise SystemExit(1)


def compare_run(f, params, memory, oracle, got, counts):
    status = got["run_status"]
    counts[oracle["status"]] = counts.get(oracle["status"], 0) + 1
    if oracle["status"] == "UNINITIALIZED":
        if status not in ("UNDERSPECIFIED", "UB", "MODEL_LIMIT"):
            fail(f, params, memory, oracle, got, "uninitialized")
        return
    if status != oracle["status"]:
        fail(f, params, memory, oracle, got, "status")
    if got["steps"] != oracle["steps"]:
        fail(f, params, memory, oracle, got, "steps")
    if status != "RETURNED":
        return
    if got["returns"] != oracle["returns"]:
        fail(f, params, memory, oracle, got, "returns")
    expected = [[str(c) if c is not None else None for c in oracle["memory"][r["name"]]] for r in f.roots]
    if f.alloca:
        expected.append([str(c) if c is not None else None for c in oracle["memory"]["m4"]])
    if got["memory"] != expected:
        fail(f, params, memory, oracle, got, "memory")
    if len(got["events"]) != len(oracle["events"]):
        fail(f, params, memory, oracle, got, "event count")
    for theirs, mine in zip(got["events"], oracle["events"]):
        for key in ("f", "g", "h", "a", "b", "update"):
            if theirs[key] != mine[key]:
                fail(f, params, memory, oracle, got, "event " + key)


def native_child(library, payload):
    runs = json.loads(Path(payload).read_text())
    lib = ctypes.CDLL(library)
    for run in runs:
        fn = getattr(lib, "wrap_" + run["name"])
        fn.restype = ctypes.c_uint64
        arguments = [ctypes.c_uint64(p) for p in run["params"]]
        keep = []
        for root in run["roots"]:
            ctype = {"f32": ctypes.c_uint32, 8: ctypes.c_uint8, 16: ctypes.c_uint16, 32: ctypes.c_uint32}[root["element"]]
            data = (ctype * len(root["cells"]))(*root["cells"])
            keep.append((root, data))
            arguments += [ctypes.cast(data, ctypes.c_void_p), ctypes.cast(data, ctypes.c_void_p), ctypes.c_int64(0)]
            shape = root["shape"]
            stride, strides = 1, []
            for s in reversed(shape):
                strides.insert(0, stride)
                stride *= s
            arguments += [ctypes.c_int64(s) for s in shape] + [ctypes.c_int64(s) for s in strides]
        result = fn(*arguments)
        if result != run["returns"]:
            print(f"NATIVE MISMATCH {run['name']}: return {result} expected {run['returns']}", file=sys.stderr)
            raise SystemExit(1)
        for root, data in keep:
            if list(data) != root["after"]:
                print(f"NATIVE MISMATCH {run['name']}: memory {list(data)} expected {root['after']}", file=sys.stderr)
                raise SystemExit(1)
    print(f"native runs agreeing: {len(runs)}")


def run_tool(*command):
    result = subprocess.run([str(c) for c in command], capture_output=True, text=True, timeout=300)
    if result.returncode:
        raise SystemExit(f"command failed: {command}\n{result.stderr[:4000]}")
    return result.stdout


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=500)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--entries", type=int, default=6)
    parser.add_argument("--native", action="store_true")
    parser.add_argument("--mlir-opt")
    parser.add_argument("--mlir-translate")
    parser.add_argument("--clang")
    args = parser.parse_args()

    rng = random.Random(args.seed)
    functions = [Function(rng, f"f{i}", args.native) for i in range(args.programs)]
    counts = {}
    native_runs = []
    with tempfile.TemporaryDirectory() as temporary:
        directory = Path(temporary)
        source = directory / "program.mlir"
        expected, lines = [], []
        # Import clones and fingerprints the enclosing module, per function, so
        # one huge module costs quadratic time. Small modules keep this linear.
        for start in range(0, len(functions), MODULE_SIZE):
            group = functions[start:start + MODULE_SIZE]
            source.write_text("\n".join(f.render() for f in group) + "\n")
            entries = []
            for f in group:
                for _ in range(args.entries):
                    params, memory = random_entry(rng, f)
                    entries.append(entry_json(f, params, memory))
                    expected.append((f, params, memory, f.run(params, memory)))
            runs = directory / "entries.json"
            runs.write_text(json.dumps(entries))
            lines += run_tool(args.driver, source, "--index-bit-width=64", f"--run={runs}", f"--fuel={FUEL}").strip().split("\n")
        assert len(lines) == len(expected), (len(lines), len(expected))
        for line, (f, params, memory, oracle) in zip(lines, expected):
            got = json.loads(line)
            if got["run_status"] == "UNSUPPORTED":
                raise SystemExit(f"generated function refused:\n{f.render()}")
            compare_run(f, params, memory, oracle, got, counts)
            if (args.native and oracle["status"] == "RETURNED" and oracle["returns"] != ["poison"]):
                roots = []
                for r in f.roots:
                    element = r["element"] if r["element"] == "f32" else f.width
                    roots.append(dict(element=element, shape=r["shape"], cells=memory[r["name"]], after=oracle["memory"][r["name"]]))
                native_runs.append(dict(name=f.name, params=params, roots=roots, returns=int(oracle["returns"][0])))
        print(f"PASS oracle: {len(functions)} functions, {len(expected)} runs, statuses {dict(sorted(counts.items()))}, "
              f"events {sum(len(e[3]['events'] or []) for e in expected)}")
        if args.native:
            wrappers = []
            for f in functions:
                names = [f"%a{i}: i64" for i in range(len(f.params))]
                names += [f"%{r['name']}: {f.memory_type(r['name'])}" for r in f.roots]
                lines = [f"func.func @wrap_{f.name}({', '.join(names)}) -> i64 {{"]
                operands = []
                for i, _ in enumerate(f.params):
                    if f.width != 64:
                        lines.append(f"  %v{i} = arith.trunci %a{i} : i64 to i{f.width}")
                    operands.append(f"%v{i}")
                operands += ["%" + r["name"] for r in f.roots]
                types = [t for _, t in f.params] + [f.memory_type(r["name"]) for r in f.roots]
                lines.append(f"  %r = func.call @{f.name}({', '.join(operands)}) : ({', '.join(types)}) -> i{f.width}")
                lines.append(f"  %w = arith.extui %r : i{f.width} to i64")
                lines += ["  return %w : i64", "}"]
                wrappers.append("\n".join(lines))
            source.write_text("module {\n" + "\n".join(f.render() for f in functions) + "\n" + "\n".join(wrappers) + "\n}\n")
            lowered, llvm, library = directory / "lowered.mlir", directory / "program.ll", directory / "program.so"
            run_tool(args.mlir_opt, source, "--canonicalize", "--expand-strided-metadata", "--finalize-memref-to-llvm",
                     "--convert-arith-to-llvm", "--convert-cf-to-llvm", "--convert-func-to-llvm", "--convert-index-to-llvm",
                     "--reconcile-unrealized-casts", "-o", lowered)
            run_tool(args.mlir_translate, lowered, "--mlir-to-llvmir", "-o", llvm)
            run_tool(args.clang, "-O0", "-shared", "-fPIC", "-Wno-override-module", llvm, "-o", library)
            payload = directory / "native.json"
            payload.write_text(json.dumps(native_runs))
            print(run_tool(sys.executable, __file__, "--native-child", library, payload).strip())
            if not native_runs:
                raise SystemExit("no native runs were compared")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--native-child":
        native_child(sys.argv[2], sys.argv[3])
    else:
        main()
