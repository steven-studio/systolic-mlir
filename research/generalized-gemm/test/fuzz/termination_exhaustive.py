#!/usr/bin/env python3
"""Exact finite-state oracle for G2a termination verdicts.

This file parses the MLIR text and executes it with Python integers. It shares
no code with the C++ library and does not know about terms, transitions, cut
points or solvers. For small widths it runs every entry to one of:

  RET      the function returned
  UB       undefined behaviour stopped the run
  STOP     an underspecified or model-limit stop
  DIVERGE  a complete configuration repeated, which is exact

and then holds the analyzer to it:

  PROVEN_TERMINATING             every entry in scope is RET
  PROVEN_TERMINATING_IF_DEFINED  no entry in scope is DIVERGE
  REFUTED_NONTERMINATING         the witness entry diverges, and its prefix and
                                 period are replayed exactly, here
  stop UNREACHABLE               no entry stops that way
  stop REACHABLE                 the witness entry stops that way
  certificate                    every invariant holds at each cut visit and
                                 every rank decreases on each step a run takes

UNKNOWN always passes: this measures soundness, not coverage. The report still
counts what was proven, so a change that proves nothing is visible.
"""
import argparse
import collections
from concurrent.futures import ProcessPoolExecutor
import itertools
import json
from pathlib import Path
import random
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parent))
from differential import compare, integer_binary, signed, wrap  # noqa: E402

INDEX_BITS = 8
MAX_STEPS = 400000
MODULE_SIZE = 40


class Refused(Exception):
    """Text outside what this oracle executes."""


class Stop(Exception):
    def __init__(self, kind):
        self.kind = kind


# ---------------------------------------------------------------- parsing

BLOCK = re.compile(r"\^(\w+)(?:\((.*)\))?:$")
FUNC = re.compile(r"func\.func (private )?@(\w+)\((.*?)\)(?: -> .*)? *\{?$")


def split_list(text):
    return [t.strip() for t in text.split(",") if t.strip()]


def width_of(text):
    text = text.strip()
    if text == "index":
        return INDEX_BITS
    return int(text[1:])


def parse_target(text):
    match = re.fullmatch(r"\^(\w+)(?:\((.*) : (.*)\))?", text.strip())
    return match.group(1), split_list(match.group(2) or "")


class Function:
    def __init__(self, name, params):
        self.name, self.params = name, params  # [(name, width)] or memory roots.
        self.blocks, self.order = {}, []
        self.memory = []  # (name, extent, width)
        self.fresh = []

    def add_block(self, label, args):
        self.blocks[label] = dict(args=args, ops=[])
        self.order.append(label)


def parse_module(text):
    functions = []
    current = None
    block = None
    for raw in text.splitlines():
        line = raw.split("//")[0].strip()
        if not line or line == "}":
            continue
        head = FUNC.match(line)
        if head:
            current = None
            if head.group(1):
                continue
            params, roots = [], []
            for item in split_list(head.group(3)):
                name, kind = [x.strip() for x in item.split(":", 1)]
                root = re.match(r"memref<(\d+)xi(\d+)>", kind)
                if root:
                    roots.append((name, int(root.group(1)), int(root.group(2))))
                elif kind.startswith("i"):
                    params.append((name, int(kind[1:])))
                else:
                    raise Refused(kind)
            current = Function(head.group(2), params)
            current.memory = roots
            current.add_block("entry", [])
            functions.append(current)
            block = "entry"
            continue
        if current is None:
            continue
        label = BLOCK.match(line)
        if label:
            args = []
            for item in split_list(label.group(2) or ""):
                name, kind = [x.strip() for x in item.split(":")]
                args.append((name, width_of(kind)))
            current.add_block(label.group(1), args)
            block = label.group(1)
            continue
        current.blocks[block]["ops"].append(line)
    return functions


def parse_op(line):
    result = None
    match = re.match(r"(%\w+) = (.*)", line)
    if match:
        result, line = match.group(1), match.group(2)
    return result, line


def successors(line):
    if line.startswith("cf.br "):
        return [parse_target(line[6:])]
    if line.startswith("cf.cond_br "):
        _, rest = line[11:].split(",", 1)
        targets, depth, piece = [], 0, ""
        for c in rest:
            depth += c == "("
            depth -= c == ")"
            if c == "," and depth == 0:
                targets.append(piece)
                piece = ""
            else:
                piece += c
        targets.append(piece)
        return [parse_target(t) for t in targets]
    if line.startswith("cf.switch "):
        return [parse_target(t.split(":", 1)[1]) for t in switch_cases(line)]
    return []


def switch_cases(line):
    """"key: target" items of a cf.switch, in slot order: default first."""
    body = line[line.index("[") + 1:line.rindex("]")]
    items, depth, piece = [], 0, ""
    for c in body:
        depth += c == "("
        depth -= c == ")"
        if c == "," and depth == 0:
            items.append(piece)
            piece = ""
        else:
            piece += c
    items.append(piece)
    return items


def layout(function):
    """Reachable blocks in text order, and the op numbering G1 uses."""
    seen, work = {"entry"}, ["entry"]
    while work:
        last = function.blocks[work.pop()]["ops"][-1]
        for label, _ in successors(last):
            if label not in seen:
                seen.add(label)
                work.append(label)
    reachable = [b for b in function.order if b in seen]
    location = {b: i for i, b in enumerate(reachable)}
    numbering, count = {}, 0
    for b in reachable:
        for op in function.blocks[b]["ops"]:
            result, _ = parse_op(op)
            if result:
                numbering[count] = result
            count += 1
    return reachable, location, numbering


# ---------------------------------------------------------------- execution


def poison_of(env, name):
    return env[name][1]


def value_of(env, name):
    return env[name][0]


def binary(op, flags, a, b, width):
    (x, px), (y, py) = a, b
    value, nsw, nuw = integer_binary(op, x, y, width)
    poison = px or py
    if "nsw" in flags and nsw:
        poison = True
    if "nuw" in flags and nuw:
        poison = True
    if op in ("divsi", "divui", "remsi", "remui"):
        if py or y == 0:
            raise Stop("UB")
        minimum = 1 << (width - 1)
        minus_one = y == wrap(-1, width)
        if op in ("divsi", "remsi") and minus_one and (px or x == minimum):
            raise Stop("UB" if op == "divsi" else "STOP")
        return value, px
    if op in ("shli", "shrsi", "shrui") and y >= width:
        return value, True
    return value, poison


def enter(function, env, label, operands):
    arguments = function.blocks[label]["args"]
    values = [env[o] for o in operands]
    for (name, _), value in zip(arguments, values):
        env[name] = value
    return label


def execute_block(function, env, memory, label):
    """Returns ("ret") or (next block, slot). Raises Stop."""
    for line in function.blocks[label]["ops"]:
        result, op = parse_op(line)
        if op.startswith("arith.constant"):
            match = re.fullmatch(r"arith\.constant (\S+)(?: : (\w+))?", op)
            text, kind = match.group(1), match.group(2)
            if text in ("true", "false"):
                env[result] = (int(text == "true"), False)
            else:
                width = width_of(kind)
                env[result] = (wrap(int(text), width), False)
        elif op.startswith("arith.cmpi"):
            match = re.fullmatch(r"arith\.cmpi (\w+), (%\w+), (%\w+) : (\w+)", op)
            pred, a, b, kind = match.groups()
            width = width_of(kind)
            env[result] = (int(compare(pred, value_of(env, a), value_of(env, b), width)),
                           poison_of(env, a) or poison_of(env, b))
        elif op.startswith("arith.select"):
            match = re.fullmatch(r"arith\.select (%\w+), (%\w+), (%\w+) : (\w+)", op)
            c, a, b, _ = match.groups()
            chosen = a if value_of(env, c) else b
            env[result] = (value_of(env, chosen), poison_of(env, c) or poison_of(env, chosen))
        elif op.startswith("arith.index_cast"):
            match = re.fullmatch(r"arith\.index_cast (%\w+) : (\w+) to index", op)
            a, kind = match.groups()
            env[result] = (wrap(signed(value_of(env, a), width_of(kind)), INDEX_BITS),
                           poison_of(env, a))
        elif op.startswith("arith."):
            match = re.fullmatch(r"arith\.(\w+) (%\w+), (%\w+)(?: overflow<(.*)>)? : (\w+)", op)
            name, a, b, flags, kind = match.groups()
            env[result] = binary(name, split_list(flags or ""), env[a], env[b], width_of(kind))
        elif op.startswith("memref.alloca"):
            pass  # Laid out by the caller.
        elif op.startswith("memref.load"):
            match = re.fullmatch(r"memref\.load (%\w+)\[(%\w+)\] : .*", op)
            root, index = match.groups()
            at, poison = env[index]
            if poison or at >= len(memory[root]):
                raise Stop("UB")
            if memory[root][at] is None:
                raise Stop("STOP")
            env[result] = (memory[root][at], False)
        elif op.startswith("memref.store"):
            match = re.fullmatch(r"memref\.store (%\w+), (%\w+)\[(%\w+)\] : .*", op)
            value, root, index = match.groups()
            at, poison = env[index]
            if poison or at >= len(memory[root]):
                raise Stop("UB")
            if poison_of(env, value):
                raise Stop("STOP")
            memory[root][at] = value_of(env, value)
        elif op.startswith("cf.br"):
            label2, operands = parse_target(op[6:])
            return enter(function, env, label2, operands), 0
        elif op.startswith("cf.cond_br"):
            condition = op[11:].split(",", 1)[0].strip()
            if poison_of(env, condition):
                raise Stop("UB")
            targets = successors(op)
            slot = 0 if value_of(env, condition) else 1
            label2, operands = targets[slot]
            return enter(function, env, label2, operands), slot
        elif op.startswith("cf.switch"):
            flag, kind = re.match(r"cf\.switch (%\w+) : (\w+),", op).groups()
            if poison_of(env, flag):
                raise Stop("UB")
            items = switch_cases(op)
            slot = 0
            for position, item in enumerate(items):
                key = item.split(":", 1)[0].strip()
                if position and wrap(int(key), width_of(kind)) == value_of(env, flag):
                    slot = position
                    break
            label2, operands = parse_target(items[slot].split(":", 1)[1])
            return enter(function, env, label2, operands), slot
        elif op.startswith("return"):
            return "ret", 0
        else:
            raise Refused(op)
    raise Refused("block without terminator")


class Run:
    """One entry, block by block, with the trace the checks need."""

    def __init__(self, function, parameters, memory, numbering):
        self.function = function
        self.env = {}
        for (name, width), value in zip(function.params, parameters):
            self.env[name] = (value, False)
        self.memory = {name: list(cells) for name, cells in memory.items()}
        self.label = "entry"
        self.steps = 0

    def key(self):
        env = tuple(sorted(self.env.items()))
        memory = tuple(sorted((k, tuple(v)) for k, v in self.memory.items()))
        return self.label, env, memory

    def step(self):
        out = execute_block(self.function, self.env, self.memory, self.label)
        self.steps += 1
        return out


def outcome(function, parameters, memory, numbering):
    run = Run(function, parameters, memory, numbering)
    seen = set()
    while True:
        key = run.key()
        if key in seen:
            return "DIVERGE"
        seen.add(key)
        if len(seen) > MAX_STEPS:
            return "TOO_LONG"
        try:
            label, _ = run.step()
        except Stop as stop:
            return stop.kind
        if label == "ret":
            return "RET"
        run.label = label


def entries(function, limit=1 << 14):
    """Every parameter point and every memory content of a function."""
    spaces = [range(1 << w) for _, w in function.params]
    cells = []
    for name, extent, width in function.memory:
        cells.extend([(name, i, width) for i in range(extent)])
    spaces_memory = [range(1 << width) for _, _, width in cells]
    size = 1
    for space in spaces + spaces_memory:
        size *= len(space)
    if size > limit:
        return None
    result = []
    for point in itertools.product(*spaces):
        for contents in itertools.product(*spaces_memory):
            memory = {name: [0] * extent for name, extent, _ in function.memory}
            for (name, i, _), value in zip(cells, contents):
                memory[name][i] = value
            result.append((point, memory))
    return result


def with_fresh(function, memory):
    memory = dict(memory)
    for ops in function.blocks["entry"]["ops"]:
        match = re.match(r"(%\w+) = memref\.alloca\(\) : memref<(\d+)x", ops)
        if match:
            memory[match.group(1)] = [None] * int(match.group(2))
    return memory


# ---------------------------------------------------------------- terms

TERM_BINARY = {"Add": "addi", "Sub": "subi", "Mul": "muli", "SDiv": "divsi",
               "UDiv": "divui", "SRem": "remsi", "URem": "remui", "And": "andi",
               "Or": "ori", "Xor": "xori", "Shl": "shli", "LShr": "shrui",
               "AShr": "shrsi"}


def evaluate(term, state, parameters):
    """A proof term. None when it reads a state not assigned yet."""
    op = term["op"]
    width = term["width"]
    if op == "Const":
        return int(term["value"])
    if op == "Param":
        return parameters[term["index"]]
    if op == "State":
        value = state.get(term["index"])
        return None if value is None else value[0]
    if op == "Shadow":
        value = state.get(term["index"])
        return int(value[1]) if value is not None else 0
    args = [evaluate(a, state, parameters) for a in term["args"]]
    if op == "Ite":
        if args[0] is None:
            return None
        return args[1] if args[0] else args[2]
    if any(a is None for a in args):
        return None
    if op in TERM_BINARY:
        return integer_binary(TERM_BINARY[op], args[0], args[1], width)[0]
    if op == "Not":
        return wrap(~args[0], width)
    if op == "ICmp":
        return int(compare(term["predicate"], args[0], args[1], term["args"][0]["width"]))
    if op == "ZExt":
        return args[0]
    if op == "SExt":
        return wrap(signed(args[0], term["args"][0]["width"]), width)
    if op == "Trunc":
        return wrap(args[0], width)
    raise AssertionError(op)


def walk_levels(levels):
    for level in levels:
        yield level
        yield from walk_levels(level["children"])


def path_key(path):
    return tuple(tuple(step) for step in path)


def check_certificate(function, result, parameters, memory, numbering, reachable, location):
    """Replays one entry, then checks each invariant and rank on its trace."""
    certificate = result["certificate"]
    cuts = set(certificate["cuts"])
    invariants = {i["cut"]: i["term"] for i in certificate["invariants"]}
    levels = []
    for level in walk_levels(certificate["levels"]):
        ranks = {r["cut"]: r["term"] for r in level["rank"]}
        levels.append((ranks, {path_key(p) for p in level["paths"]},
                       {path_key(p) for p in level["strict"]}))
    state_names = {}
    for s in result["state"]:
        if s["kind"] == "argument":
            label = reachable[s["location"]]
            state_names[s["id"]] = function.blocks[label]["args"][s["position"]][0]
        else:
            state_names[s["id"]] = numbering.get(s["position"])

    # The trace: the state at each cut visit, and the path between visits.
    run = Run(function, parameters, memory, numbering)
    seen, visits, paths, segment = set(), [], [], []
    while run.key() not in seen and len(seen) < MAX_STEPS:
        seen.add(run.key())
        here = location[run.label]
        try:
            label, slot = run.step()
        except Stop:
            break
        if label == "ret":
            break
        run.label = label
        to = location[label]
        segment.append((here, slot, to))
        if to in cuts:
            visits.append({i: run.env[n] for i, n in state_names.items() if n in run.env})
            paths.append((tuple(segment), to))
            segment = []

    checks = 0
    for state, (_, cut) in zip(visits, paths):
        if cut in invariants and invariants[cut] is not None:
            value = evaluate(invariants[cut], state, parameters)
            checks += 1
            if value == 0:
                raise AssertionError(f"invariant at L{cut} is false on a real run: {state}")
    # paths[k] leads to visits[k]. Step k+1 runs from visits[k] to visits[k+1],
    # and an infinite run in a level also takes a path of that level after it.
    for k in range(len(visits) - 2):
        taken, following = paths[k + 1][0], paths[k + 2][0]
        for ranks, inside, strict in levels:
            if taken not in inside or following not in inside:
                continue
            start, stop = ranks[taken[0][0]], ranks[taken[-1][2]]
            if start is None or stop is None:
                continue
            before = evaluate(start, visits[k], parameters)
            after = evaluate(stop, visits[k + 1], parameters)
            if before is None or after is None:
                continue
            checks += 1
            if not (after < before if taken in strict else after <= before):
                raise AssertionError(f"rank does not decrease on {taken}: {before} -> {after}")
    return checks


def replay_witness(function, witness, numbering):
    """prefix and period count block steps, as the model's transitions do."""
    entry = witness["entry"]
    parameters = tuple(int(v) for v in entry["parameters"])
    memory = {}
    for (name, _, _), root in zip(function.memory, entry["memory"]):
        memory[name] = [int(c) for c in root["cells"]]
    memory = with_fresh(function, memory)
    run = Run(function, parameters, memory, numbering)
    # What a later block can read: block arguments and values used outside the
    # block that defines them. A block-local temporary is no part of the state.
    home, used = {}, set()
    for label, block in function.blocks.items():
        for name, _ in block["args"]:
            used.add(name)
        for op in block["ops"]:
            result, _ = parse_op(op)
            if result:
                home[result] = label
    for label, block in function.blocks.items():
        for op in block["ops"]:
            for name in re.findall(r"%\w+", parse_op(op)[1]):
                if name in home and home[name] != label:
                    used.add(name)

    def configuration():
        values = tuple(sorted((n, run.env[n]) for n in used if n in run.env))
        cells = tuple(sorted((k, tuple(v)) for k, v in run.memory.items()))
        return run.label, values, cells

    def advance(count):
        for _ in range(count):
            label, _ = run.step()
            if label == "ret":
                raise AssertionError("the witness run returned")
            run.label = label

    advance(witness["prefix"])
    anchor = configuration()
    advance(witness["period"])
    if configuration() != anchor:
        raise AssertionError("the witness does not repeat a complete configuration")
    return parameters, memory


# ---------------------------------------------------------------- generator


class Writer:
    """Prints one function. Every value name is unique."""

    def __init__(self, rng, name, width):
        self.rng, self.name, self.width = rng, name, width
        self.type = f"i{width}"
        self.count = 0
        self.head, self.blocks = "", []
        self.lines = []

    def fresh(self):
        self.count += 1
        return f"%v{self.count}"

    def constant(self, value, lines=None):
        name = self.fresh()
        (self.lines if lines is None else lines).append(
            f"{name} = arith.constant {signed(wrap(value, self.width), self.width)} : {self.type}")
        return name

    def flag(self):
        roll = self.rng.random()
        if roll < 0.5:
            return ""
        return " overflow<nsw>" if roll < 0.8 else " overflow<nuw>" if roll < 0.95 else " overflow<nsw,nuw>"

    def binary(self, op, a, b, flag=""):
        name = self.fresh()
        self.lines.append(f"{name} = arith.{op} {a}, {b}{flag} : {self.type}")
        return name

    def compare(self, predicate, a, b):
        name = self.fresh()
        self.lines.append(f"{name} = arith.cmpi {predicate}, {a}, {b} : {self.type}")
        return name

    def block(self, header):
        self.blocks.append((header, self.lines))
        self.lines = []

    def text(self, signature):
        rng = self.rng
        first, rest = self.blocks[0], self.blocks[1:]
        rng.shuffle(rest)
        out = [f"func.func @{self.name}({signature}) {{"]
        out += ["  " + line for line in first[1]]
        for header, lines in rest:
            out.append(f"{header}:")
            out += ["  " + line for line in lines]
        return "\n".join(out + ["}"]) + "\n"


PREDICATES = ["slt", "sle", "sgt", "sge", "ult", "ule", "ugt", "uge", "ne", "eq"]


def update(w, value, step):
    rng = w.rng
    op = rng.choice(["addi", "addi", "addi", "subi", "subi", "muli"])
    return w.binary(op, value, step, w.flag())


def counter(rng, name, width):
    """A counter with one to three branches and an optional second variable."""
    w = Writer(rng, name, width)
    T = w.type
    bound = "%n"
    constants = [w.constant(rng.randint(0, (1 << width) - 1)) for _ in range(3)]
    start = rng.choice(constants + ["%n"])
    two = rng.random() < 0.3
    args = f"{start}, {constants[0]} : {T}, {T}" if two else f"{start} : {T}"
    w.lines.append(f"cf.br ^h({args})")
    w.block("entry")
    predicate = rng.choice(PREDICATES)
    guard = w.compare(predicate, "%i", rng.choice([bound, bound, constants[1]]))
    w.lines.append(f"cf.cond_br {guard}, ^body, ^done")
    w.block(f"^h(%i: {T}, %k: {T})" if two else f"^h(%i: {T})")
    branches = rng.choice([1, 2, 2, 3])
    next_k = "%k" if two else None

    def latch(value):
        k = next_k
        if two:
            k = rng.choice(["%k", update(w, "%k", constants[2]), "%i"])
        w.lines.append(f"cf.br ^h({value}, {k} : {T}, {T})" if two else f"cf.br ^h({value} : {T})")

    def step_value():
        return rng.choice(constants + (["%k"] if two else []) + ["%n"] * (rng.random() < 0.1))

    if branches == 1:
        latch(update(w, "%i", step_value()))
        w.block("^body")
    else:
        mask = w.binary("andi", "%i", constants[2])
        test = w.compare(rng.choice(["eq", "ne", "slt", "ult"]), mask, constants[0])
        w.lines.append(f"cf.cond_br {test}, ^t, ^f")
        w.block("^body")
        latch(update(w, "%i", step_value()))
        w.block("^t")
        if branches == 3:
            other = w.compare("sgt", "%i", constants[1])
            w.lines.append(f"cf.cond_br {other}, ^g, ^f2")
            w.block("^f")
            latch(update(w, "%i", step_value()))
            w.block("^g")
            latch(update(w, "%i", step_value()))
            w.block("^f2")
        else:
            latch(update(w, "%i", step_value()))
            w.block("^f")
    w.lines.append("return")
    w.block("^done")
    return w.text(f"%n: {T}")


def nest(rng, name, width):
    w = Writer(rng, name, width)
    T = w.type
    one, other = w.constant(rng.choice([1, 1, 1, 2, 3])), w.constant(rng.randint(0, 7))
    zero = w.constant(0)
    w.lines.append(f"cf.br ^o({zero} : {T})")
    w.block("entry")
    g = w.compare(rng.choice(["slt", "ult", "ne"]), "%i", "%n")
    w.lines.append(f"cf.cond_br {g}, ^pre, ^done")
    w.block(f"^o(%i: {T})")
    w.lines.append(f"cf.br ^in({rng.choice([zero, '%i', other])} : {T})")
    w.block("^pre")
    h = w.compare(rng.choice(["slt", "ult", "ne"]), "%j", rng.choice(["%n", "%i", other]))
    w.lines.append(f"cf.cond_br {h}, ^b, ^l")
    w.block(f"^in(%j: {T})")
    u = w.binary("addi", "%j", one, w.flag())
    w.lines.append(f"cf.br ^in({u} : {T})")
    w.block("^b")
    v = w.binary("addi", "%i", one, w.flag())
    w.lines.append(f"cf.br ^o({v} : {T})")
    w.block("^l")
    w.lines.append("return")
    w.block("^done")
    return w.text(f"%n: {T}")


def division(rng, name, width):
    w = Writer(rng, name, width)
    T = w.type
    one, c = w.constant(1), w.constant(rng.randint(0, (1 << width) - 1))
    w.lines.append(f"cf.br ^h(%n : {T})")
    w.block("entry")
    g = w.compare(rng.choice(["sgt", "ugt", "ne"]), "%x", w.constant(0))
    w.lines.append(f"cf.cond_br {g}, ^b, ^done")
    w.block(f"^h(%x: {T})")
    d = w.binary("subi", "%x", one)
    q = w.binary(rng.choice(["divsi", "divui", "remsi", "remui"]), c, rng.choice([d, "%x"]))
    w.binary("shli", q, rng.choice([one, d]))
    w.lines.append(f"cf.br ^h({rng.choice([d, q])} : {T})")
    w.block("^b")
    w.lines.append("return")
    w.block("^done")
    return w.text(f"%n: {T}")


def memory_loop(rng, name, width):
    width = min(width, 2)
    w = Writer(rng, name, width)
    T = w.type
    zero, one = w.constant(0), w.constant(1)
    alloca = rng.random() < 0.3
    root = "%m"
    if alloca:
        w.lines.append(f"%m = memref.alloca() : memref<2x{T}>")
    w.lines.append(f"cf.br ^h({zero} : {T})")
    w.block("entry")
    index = w.fresh()
    limit = w.constant(rng.choice([1, 2, 3]))
    position = w.binary("remui", "%i", limit) if rng.random() < 0.7 else "%i"
    w.lines.append(f"{index} = arith.index_cast {position} : {T} to index")
    loaded = w.fresh()
    w.lines.append(f"{loaded} = memref.load {root}[{index}] : memref<2x{T}>")
    g = w.compare(rng.choice(["ne", "slt", "ult"]), loaded, rng.choice(["%n", zero]))
    w.lines.append(f"cf.cond_br {g}, ^b, ^done")
    w.block(f"^h(%i: {T})")
    if rng.random() < 0.6:
        s = w.fresh()
        w.lines.append(f"{s} = arith.index_cast %i : {T} to index")
        if rng.random() < 0.5:
            v = w.binary("addi", loaded, one)
            w.lines.append(f"memref.store {v}, {root}[{s}] : memref<2x{T}>")
    u = w.binary("addi", "%i", one, w.flag())
    w.lines.append(f"cf.br ^h({u} : {T})")
    w.block("^b")
    w.lines.append("return")
    w.block("^done")
    signature = f"%n: {T}" if alloca else f"%n: {T}, %m: memref<2x{T}>"
    return w.text(signature)


def random_cfg(rng, name, width):
    w = Writer(rng, name, width)
    T = w.type
    count = rng.randint(2, 4)
    arguments = {b: rng.randint(0, 1 if width > 3 else 2) for b in range(count)}
    labels = [f"^b{b}" for b in range(count)]
    shared = ["%n", w.constant(rng.randint(0, (1 << width) - 1)), w.constant(1), w.constant(2)]

    def jump(pool):
        target = rng.randrange(count)
        operands = [rng.choice(pool) for _ in range(arguments[target])]
        if not operands:
            return labels[target]
        return f"{labels[target]}({', '.join(operands)} : {', '.join([T] * len(operands))})"

    w.lines.append(f"cf.br {jump(shared)}")
    w.block("entry")
    for b in range(count):
        names = [f"%a{b}_{k}" for k in range(arguments[b])]
        pool = shared + names
        for _ in range(rng.randint(0, 3)):
            op = rng.choice(["addi", "subi", "muli", "andi", "ori", "xori", "shli", "shrsi", "remui", "remsi", "divsi"])
            flag = w.flag() if op in ("addi", "subi", "muli") else ""
            pool.append(w.binary(op, rng.choice(pool), rng.choice(pool), flag))
        roll = rng.random()
        if roll < 0.2:
            w.lines.append("return")
        elif roll < 0.35:
            w.lines.append(f"cf.br {jump(pool)}")
        else:
            test = w.compare(rng.choice(PREDICATES), rng.choice(pool), rng.choice(pool))
            w.lines.append(f"cf.cond_br {test}, {jump(pool)}, {jump(pool)}")
        header = labels[b] + (f"({', '.join(f'{n}: {T}' for n in names)})" if names else "")
        w.block(header)
    return w.text(f"%n: {T}")


GENERATORS = [counter, counter, counter, nest, division, memory_loop, random_cfg, random_cfg]


# ---------------------------------------------------------------- comparison


def compare_function(function, result, scope):
    """Returns (entry count, certificate check count). Raises on disagreement."""
    verdict = result["verdict"]
    if result["conflict"]:
        raise AssertionError(f"self-reported conflict: {result['conflict']}")
    # G1 inlines calls, so block numbers would not match this text.
    for block in function.blocks.values():
        for op in block["ops"]:
            if parse_op(op)[1].startswith("call "):
                raise Refused(op)
    reachable, location, numbering = layout(function)
    points = entries(function)
    if points is None:
        return 0, 0
    fresh = any("memref.alloca" in op for op in function.blocks["entry"]["ops"])
    seen, checks = collections.Counter(), 0
    for parameters, memory in points:
        if scope is not None:
            low, high = scope
            if not low <= signed(parameters[0], function.params[0][1]) <= high:
                continue
        memory = with_fresh(function, memory)
        end = outcome(function, parameters, memory, numbering)
        if end == "TOO_LONG":
            continue
        seen[end] += 1
        where = f"entry {parameters} {memory}: {end}"
        if verdict == "PROVEN_TERMINATING" and end != "RET":
            raise AssertionError(f"PROVEN_TERMINATING, but {where}")
        if verdict == "PROVEN_TERMINATING_IF_DEFINED" and end == "DIVERGE":
            raise AssertionError(f"IF_DEFINED, but {where}")
        if "certificate" in result:
            checks += check_certificate(function, result, parameters, memory, numbering,
                                        reachable, location)
    stops = {s["kind"]: s for s in result["stops"]}
    if "certificate" in result and not fresh:
        if stops["UB"]["status"] == "UNREACHABLE" and seen["UB"]:
            raise AssertionError("a reachable UB was called unreachable")
        if (stops["UNDERSPECIFIED"]["status"] == "UNREACHABLE" and
                stops["MODEL_LIMIT"]["status"] == "UNREACHABLE" and seen["STOP"]):
            raise AssertionError("a reachable stop was called unreachable")
    if verdict == "REFUTED_NONTERMINATING":
        parameters, memory = replay_witness(function, result["witness"], numbering)
        if scope is not None and not scope[0] <= signed(parameters[0], function.params[0][1]) <= scope[1]:
            raise AssertionError("a witness outside the scope")
        end = outcome(function, parameters, memory, numbering)
        if end != "DIVERGE":
            raise AssertionError(f"REFUTED, but the witness ends {end}")
    for kind, report in stops.items():
        if report["status"] == "REACHABLE" and "witness" in report and kind == "UB" and not fresh:
            entry = report["witness"]
            parameters = tuple(int(v) for v in entry["parameters"])
            memory = {}
            for (name, _, _), root in zip(function.memory, entry["memory"]):
                memory[name] = [int(c) for c in root["cells"]]
            end = outcome(function, parameters, with_fresh(function, memory), numbering)
            if end != "UB":
                raise AssertionError(f"UB witness ends {end}")
    return sum(seen.values()), checks


def run_batch(job):
    driver, text, width, scope, index_bits = job
    tally = collections.Counter()
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "input.mlir"
        path.write_text(text)
        command = [driver, str(path), f"--index-bit-width={index_bits}", "--replay-fuel=30000",
                   "--timeout-ms=5000"]
        if scope is not None:
            command.append(f"--scope=0:{scope[0]}:{scope[1]}")
        done = subprocess.run(command, capture_output=True, text=True, timeout=1500)
    if done.returncode:
        return tally, [f"driver failed: {done.stderr[-800:]}\n{text}"]
    results = {r["function"]: r for r in json.loads(done.stdout)}
    failures = []
    for function in parse_module(text):
        if function.name not in results:
            continue
        result = results[function.name]
        tally[result["verdict"]] += 1
        for s in result["stops"]:
            if s["status"] == "REACHABLE":
                tally["stop reachable"] += 1
        if "certificate" in result:
            for level in walk_levels(result["certificate"]["levels"]):
                tally["rank " + level["origin"].split()[0]] += 1
        try:
            count, checks = compare_function(function, result, scope)
            tally["entries"] += count
            tally["certificate checks"] += checks
        except Refused:
            tally["outside the oracle"] += 1
        except AssertionError as error:
            source = re.search(rf"func\.func @{function.name}\(.*?\n\}}\n", text, re.S)
            failures.append(f"{function.name}: {error}\n{source.group(0) if source else ''}")
    return tally, failures


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--programs", type=int, default=200)
    parser.add_argument("--seed", type=int, default=1)
    parser.add_argument("--jobs", type=int, default=4)
    parser.add_argument("--catalogue", default="", help="gen_loops.py to include")
    args = parser.parse_args()
    rng = random.Random(args.seed)
    jobs = []
    remaining, number = args.programs, 0
    while remaining > 0:
        width = rng.choice([1, 2, 3, 3, 4, 4, 5])
        size = min(MODULE_SIZE, remaining)
        remaining -= size
        scope = None
        if rng.random() < 0.3:
            low = -(1 << (width - 1))
            high = (1 << (width - 1)) - 1
            a, b = sorted((rng.randint(low, high), rng.randint(low, high)))
            scope = (a, b)
        text = ""
        for _ in range(size):
            number += 1
            # memory_loop narrows to i2, and a scope is written for one width.
            choices = [g for g in GENERATORS if scope is None or width <= 2 or g is not memory_loop]
            text += rng.choice(choices)(rng, f"f{number}", width)
        jobs.append((args.driver, text, width, scope, INDEX_BITS))
    if args.catalogue:
        for width in (3, 4, 5):
            text = subprocess.run([sys.executable, args.catalogue, str(width)], capture_output=True,
                                  text=True, check=True).stdout
            jobs.append((args.driver, text, width, None, INDEX_BITS))
    total = collections.Counter()
    failures = []
    with ProcessPoolExecutor(max_workers=args.jobs) as pool:
        for tally, failed in pool.map(run_batch, jobs):
            total.update(tally)
            failures.extend(failed)
    for key in sorted(total):
        print(f"{key}: {total[key]}")
    for failure in failures[:5]:
        print("DISAGREEMENT", failure)
    print(f"{len(failures)} disagreements")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
