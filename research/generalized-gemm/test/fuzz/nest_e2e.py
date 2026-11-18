#!/usr/bin/env python3
"""Stage 2 (docs/GENERALIZED_GEMM_SCOPE.md), end to end: records of loop nests, through the
production pass, to tiles, runtime calls and a run.

A module of functions is printed here as MLIR with static shapes, from specs: one product
over a box in binary32 with reassoc, the same with no flag, the same in 32-bit integers, a
product with offsets, a scale, a transposed operand and a permuted loop order, two nests on
one output, a nest on C with a value of C stored into D and a nest on D, a nest reading what
another wrote, three nests in a chain, and an output zeroed by a loop before two nests
accumulate into it. For each variant of records:

  records       the research tool's records, every pair of roots authorized, 64-bit index
  unauthorized  its records with no pair authorized: nothing is verified
  stale         the records, and a program whose first product's M changed since
  off           no records: the pass as it is without the option

the production tool lowers the module (--affine-matmul-to-systolic, then
--lower-systolic-matmul-tile-to-fpga, synchronous and asynchronous, then to LLVM), and a C
driver printed here runs every function against the software runtime model and against a
reference in plain C that runs the same loops in the same order. What is checked:

- Which nests are lowered, from the pass's remarks, against what the variant must give: a
  nest is lowered only with a record that agrees, and the lowering's own refusals stand.
- Tiles and calls: each lowered nest is systolic.matmul_tile operations where it stood;
  each GEMM's runtime calls come before the statements after it (in the asynchronous
  lowering its wait and the write-back of C come before the next statement).
- The runtime model ran exactly the invocations the remarks announce for each function,
  none failed, and none for a function whose nests all stay on the host.
- Results, by the numeric contract of each nest. Integers, and floating point on the host:
  the same bits as the reference. Floating point lowered with reassociation permitted: the
  device sums each tile from zero and adds the sum to C, another order of additions, so a
  cell may differ from the reference by at most 2 gamma_(T+2) (|c| + sum |a b|) for its T
  contributions (gamma_n = n u / (1 - n u), u = 2^-24, the bound of any order of rounded
  additions of rounded products, on each side), carried through every later statement that
  reads the cell (a value moved, an accumulator, an operand: first-order and second-order
  terms of the carried error). Cells no lowered nest reached keep the same bits. With small
  integer data every sum is exact in any order, so every cell keeps the same bits there; with
  data in [-1, 1) some cell must differ, or the comparison proves nothing.

The C driver is compiled with -ffp-contract=off, as the lowered loops are: no product is
fused into an addition on either side.
"""
import argparse
import copy
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

K_MAX = 16
FLEET = f"--systolic-declare-fpga-fleet=k-max-8x8={K_MAX} k-max-4x4={K_MAX} pipeline-depth=1"
SOLVER = ["--max-level=1", "--candidates-per-group=1", "--timeout-ms=120000", "--rlimit=20000000"]


class Failure(Exception):
    pass


# ---------------------------------------------------------------------------------------
# The programs.

def product(c, a, b, bounds, order="ijk", views=None, flag=True, first=True, access="affine",
            constants="inside", loops="affine", bounds_at="outside", compare="slt", step="counter+1",
            bound_in_before=False, carry=(), carry_step="after", counter_place="first"):
    """`access`: "affine" reads and writes with affine.load and affine.store; "memref" with
    memref.load and memref.store, each index computed by arith (a counter times its scale plus
    its constant; a flat array's index is row * columns + column). A view entry is (role,
    scale, constant), or ("perm", role, multiplier, modulus): (role * multiplier) mod modulus,
    by arith.remsi. `constants` ("inside" or "outside", memref access only): where the index
    constants are defined, in the loop body or before the nest. `loops` (memref access only):
    "affine" for affine.for, "scf" for scf.for, "while" for a counting scf.while (its counter
    the one value it carries, compared with slt in the before region, forwarded by
    scf.condition, and counter + 1 yielded at the end of the after region), or three of these,
    one per loop from the outermost in. The bounds and step of an scf.for or scf.while are
    arith.constant values, all defined before the nest (`bounds_at` "outside") or each loop's
    in the body of the loop around it ("inside"; the outermost loop's before the nest). An
    scf.while's comparison is `compare`: "slt" or "ult" (counter < bound), "sgt" or "ugt" (bound >
    counter), "ne" (counter != bound) or "en" (bound != counter); its step is `step`,
    "counter+1" or "1+counter"; with `bound_in_before` its bound is a constant of its before
    region. `carry` (memref access only): pairs (operand, dimension) whose index is not
    computed by arith from the counter but carried by the loop of its role, an scf.for
    (iter_args) or scf.while: started at the constant the view gives at the loop's first
    iteration and stepped by the view's scale each iteration (arith.addi, or arith.subi for a
    negative scale), so it has the view's value at every iteration. `carry_step` "before": the
    loop carries the index one step behind and the body steps it first, reads the stepped value
    and hands that on (the subscript is the carried value plus the step). `counter_place`
    "second" (scf.while): the carried indices come before the counter among the loop's values."""
    views = views or {"c": [("i", 1, 0), ("j", 1, 0)], "a": [("i", 1, 0), ("k", 1, 0)],
                      "b": [("k", 1, 0), ("j", 1, 0)]}
    return {"kind": "product", "c": c, "a": a, "b": b, "bounds": bounds, "order": order, "views": views,
            "flag": flag, "first": first, "access": access, "constants": constants, "loops": loops,
            "bounds_at": bounds_at, "compare": compare, "step": step, "bound_in_before": bound_in_before,
            "carry": tuple(carry), "carry_step": carry_step, "counter_place": counter_place}


def loop_kinds(statement):
    """The kind of each loop of a product, from the outermost in."""
    loops = statement.get("loops", "affine")
    kinds = [loops] * 3 if isinstance(loops, str) else list(loops)
    if len(kinds) != 3 or any(kind not in ("affine", "scf", "while") for kind in kinds):
        raise ValueError(f"loops: {loops}")
    return kinds


def move(source, target):
    return {"kind": "move", "from": source, "to": target}


def fill(array):
    """A loop nest that zeroes every cell of `array`: no GEMM, it stays on the host."""
    return {"kind": "fill", "array": array}


BOX = {"i": (0, 16), "j": (0, 8), "k": (0, 12)}
FUNCTIONS = [
    {"name": "e2e_single", "element": "f32", "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [product("c", "a", "b", BOX)]},
    {"name": "e2e_strict", "element": "f32", "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [product("c", "a", "b", BOX, flag=False)]},
    {"name": "e2e_integer", "element": "i32", "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True)],
     "statements": [product("c", "a", "b", BOX, flag=False)]},
    {"name": "e2e_skewed", "element": "f32", "arrays": [("a", 8, 8, False), ("b", 8, 8, False), ("c", 8, 8, True)],
     "statements": [product("c", "a", "b", {"i": (1, 3), "j": (0, 3), "k": (2, 4)}, order="kij",
                            views={"c": [("i", 1, 0), ("j", 1, 2)], "a": [("i", 1, 1), ("k", 1, 0)],
                                   "b": [("j", 1, 0), ("k", 2, 0)]})]},
    {"name": "e2e_shared", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [product("c", "a", "b", BOX), product("c", "a", "d", BOX)]},
    {"name": "e2e_stored", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 16, 8, True)],
     "statements": [product("c", "a", "b", BOX), move(("c", 0, 0), ("d", 1, 1)), product("d", "a", "b", BOX)]},
    {"name": "e2e_reads", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("e", 8, 8, False), ("d", 16, 8, True)],
     "statements": [product("c", "a", "b", BOX), product("d", "c", "e", {"i": (0, 16), "j": (0, 8), "k": (0, 8)})]},
    {"name": "e2e_chain", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("e", 8, 8, False),
                ("d", 16, 8, True), ("f", 8, 8, False)],
     "statements": [product("c", "a", "b", BOX), product("d", "c", "e", {"i": (0, 16), "j": (0, 8), "k": (0, 8)}),
                    product("c", "d", "f", {"i": (0, 16), "j": (0, 8), "k": (0, 8)})]},
    {"name": "e2e_initialized", "element": "f32",
     "arrays": [("a", 16, 12, False), ("b", 12, 8, False), ("c", 16, 8, True), ("d", 12, 8, False)],
     "statements": [fill("c"), product("c", "a", "b", BOX), product("c", "a", "d", BOX)]},
]


def functions(stale=False):
    """The specs. With `stale`, the first function's product runs one row fewer: the program
    changed after its records were made, and only that nest did."""
    out = copy.deepcopy(FUNCTIONS)
    if stale:
        # A new dictionary: the specs share their bounds, and only this nest changes.
        statement = out[0]["statements"][0]
        low, high = statement["bounds"]["i"]
        statement["bounds"] = dict(statement["bounds"], i=(low, high - 1))
    return out


def memref(rows, cols, element):
    return f"memref<{rows}x{cols}x{element}>"


def index(view, number):
    parts = []
    for role, scale, constant in view:
        text = f"%{role}{number}" + (f" * {scale}" if scale != 1 else "") + (f" + {constant}" if constant else "")
        parts.append(text)
    return ", ".join(parts)


def memref_of(function, name):
    """An array's MLIR type: rows x columns, or one dimension of rows * columns for the arrays
    the function lists as flat (the C driver keeps every array rows x columns, row-major)."""
    rows, cols = next((r, c) for n, r, c, _ in function["arrays"] if n == name)
    if name in function.get("flat", ()):
        return f"memref<{rows * cols}x{function['element']}>"
    return memref(rows, cols, function["element"])


def memref_body(function, statement, number, pad, constants):
    """The innermost body of a product read and written with memref.load and memref.store,
    every index computed by arith from the counters. The index constants are appended to
    `constants` (the caller puts them in the body or before the nest)."""
    e = function["element"]
    shapes = {name: (rows, cols) for name, rows, cols, _ in function["arrays"]}
    lines = []
    def constant_value(name, value):
        constants.append(f"%{name} = arith.constant {value} : index")
        return f"%{name}"
    def indices(operand):
        array = statement[operand]
        names = []
        for d, entry in enumerate(statement["views"][operand]):
            if entry[0] == "perm":
                _, role, multiplier, modulus = entry
                m = constant_value(f"{operand}m{d}c{number}", multiplier)
                q = constant_value(f"{operand}q{d}c{number}", modulus)
                lines.append(f"{pad}%{operand}p{d}{number} = arith.muli %{role}{number}, {m} : index")
                lines.append(f"{pad}%{operand}r{d}{number} = arith.remsi %{operand}p{d}{number}, {q} : index")
                names.append(f"%{operand}r{d}{number}")
                continue
            role, scale, offset = entry
            value = f"%{role}{number}"
            if (operand, d) in statement.get("carry", ()):
                names.append(f"%{operand}cx{d}n{number}")  # carried by the loop of `role`
                continue
            if scale != 1:
                c = constant_value(f"{operand}s{d}c{number}", scale)
                lines.append(f"{pad}%{operand}s{d}{number} = arith.muli {value}, {c} : index")
                value = f"%{operand}s{d}{number}"
            if offset:
                c = constant_value(f"{operand}o{d}c{number}", offset)
                lines.append(f"{pad}%{operand}o{d}{number} = arith.addi {value}, {c} : index")
                value = f"%{operand}o{d}{number}"
            names.append(value)
        if array in function.get("flat", ()):
            cols = shapes[array][1]
            w = constant_value(f"{operand}w{number}", cols)
            lines.append(f"{pad}%{operand}r{number} = arith.muli {names[0]}, {w} : index")
            lines.append(f"{pad}%{operand}x{number} = arith.addi %{operand}r{number}, {names[1]} : index")
            return f"%{operand}x{number}"
        return ", ".join(names)
    where = {operand: indices(operand) for operand in ("c", "a", "b")}
    for name, operand in (("z", "c"), ("x", "a"), ("y", "b")):
        array = statement[operand]
        lines.append(f"{pad}%{name}{number} = memref.load %{array}[{where[operand]}] : {memref_of(function, array)}")
    mul, addition = ("arith.mulf", "arith.addf") if e == "f32" else ("arith.muli", "arith.addi")
    lines.append(f"{pad}%m{number} = {mul} %x{number}, %y{number} : {e}")
    operands = f"%z{number}, %m{number}" if statement["first"] else f"%m{number}, %z{number}"
    flag = " fastmath<reassoc>" if statement["flag"] else ""
    lines.append(f"{pad}%s{number} = {addition} {operands}{flag} : {e}")
    lines.append(f"{pad}memref.store %s{number}, %{statement['c']}[{where['c']}] : {memref_of(function, statement['c'])}")
    return lines


def mlir_function(function):
    e = function["element"]
    shapes = {name: (rows, cols) for name, rows, cols, _ in function["arrays"]}
    arguments = ", ".join(f"%{name}: {memref_of(function, name)}" + (" {llvm.noalias}" if noalias else "")
                          for name, rows, cols, noalias in function["arrays"])
    lines = [f"func.func @{function['name']}({arguments}) {{"]
    for number, statement in enumerate(function["statements"]):
        if statement["kind"] == "move":
            (source, sr, sc), (target, tr, tc) = statement["from"], statement["to"]
            lines += [f"  %r{number} = arith.constant {sr} : index", f"  %q{number} = arith.constant {sc} : index",
                      f"  %u{number} = arith.constant {tr} : index", f"  %w{number} = arith.constant {tc} : index",
                      f"  %v{number} = memref.load %{source}[%r{number}, %q{number}] : {memref(*shapes[source], e)}",
                      f"  memref.store %v{number}, %{target}[%u{number}, %w{number}] : {memref(*shapes[target], e)}"]
            continue
        if statement["kind"] == "fill":
            array = statement["array"]
            rows, cols = shapes[array]
            zero = "0.000000e+00" if e == "f32" else "0"
            lines += [f"  %zero{number} = arith.constant {zero} : {e}",
                      f"  affine.for %i{number} = 0 to {rows} {{",
                      f"    affine.for %j{number} = 0 to {cols} {{",
                      f"      affine.store %zero{number}, %{array}[%i{number}, %j{number}] : {memref(rows, cols, e)}",
                      "    }", "  }"]
            continue
        bounds = statement["bounds"]
        pad = "        "
        if statement.get("access", "affine") == "memref":
            constants = []
            body = memref_body(function, statement, number, pad, constants)
            if statement.get("constants", "inside") == "outside":
                lines += ["  " + c for c in constants]
            else:
                body = [pad + c for c in constants] + body
            kinds = loop_kinds(statement)
            outside = statement.get("bounds_at", "outside") == "outside"
            def carried(role):
                """The indices the loop of `role` carries: (name, start, step)."""
                out = []
                for operand, d in statement.get("carry", ()):
                    r, scale, offset = statement["views"][operand][d]
                    if r == role:
                        out.append((f"{operand}cx{d}n{number}", scale * bounds[role][0] + offset, scale))
                return out
            early = statement.get("carry_step", "after") == "before"
            def held(name):
                """The name of a carried index as the loop hands it to the body."""
                return f"%{name}pre" if early else f"%{name}"
            def handed(name):
                """The name of the value the loop hands on."""
                return f"%{name}" if early else f"%{name}next"
            def bound_constants(role):
                low, high = bounds[role]
                out = [f"%{role}lo{number} = arith.constant {low} : index",
                       f"%{role}hi{number} = arith.constant {high} : index",
                       f"%{role}st{number} = arith.constant 1 : index"]
                for name, start, step in carried(role):
                    out += [f"%{name}start = arith.constant {start - step if early else start} : index",
                            f"%{name}step = arith.constant {abs(step)} : index"]
                return out
            def carried_steps(role, indent):
                if early:
                    return []
                return [indent + f"  %{name}next = arith.{'addi' if step >= 0 else 'subi'} %{name}, %{name}step : index"
                        for name, start, step in carried(role)]
            if early:
                body = [pad + f"%{name} = arith.{'addi' if step >= 0 else 'subi'} %{name}pre, %{name}step : index"
                        for role in statement["order"] for name, start, step in carried(role)] + body
            if outside:
                for depth, role in enumerate(statement["order"]):
                    if kinds[depth] != "affine":
                        lines += ["  " + c for c in bound_constants(role)]
            closers = []
            for depth, role in enumerate(statement["order"]):
                low, high = bounds[role]
                indent = "  " * (depth + 1)
                mine = carried(role)
                if kinds[depth] == "affine":
                    if mine:
                        raise ValueError("carry: an affine.for carries no index here")
                    lines.append(indent + f"affine.for %{role}{number} = {low} to {high} {{")
                    closers.append([indent + "}"])
                    continue
                if not outside:
                    lines += [indent + c for c in bound_constants(role)]
                if kinds[depth] == "scf" and mine:
                    results = f"%{role}res{number}" + (f":{len(mine)}" if len(mine) > 1 else "")
                    lines.append(indent + f"{results} = scf.for %{role}{number} = %{role}lo{number} to %{role}hi{number} "
                                 f"step %{role}st{number} iter_args(" +
                                 ", ".join(f"{held(name)} = %{name}start" for name, _, _ in mine) + ") -> (" +
                                 ", ".join("index" for _ in mine) + ") {")
                    closers.append(carried_steps(role, indent) +
                                   [indent + "  scf.yield " + ", ".join(handed(name) for name, _, _ in mine) + " : " +
                                    ", ".join("index" for _ in mine), indent + "}"])
                    continue
                if kinds[depth] == "scf":
                    lines.append(indent + f"scf.for %{role}{number} = %{role}lo{number} to %{role}hi{number} "
                                 f"step %{role}st{number} {{")
                    closers.append([indent + "}"])
                    continue
                # A counting scf.while: the counter is its one carried value; the body sees it as
                # the after region's argument, under the name a for loop gives its counter.
                counter, bound = f"%{role}w{number}", f"%{role}hi{number}"
                before = []
                if statement.get("bound_in_before"):
                    bound = f"%{role}hb{number}"
                    before.append(indent + f"  {bound} = arith.constant {high} : index")
                style = statement.get("compare", "slt")
                operands = {"slt": (counter, bound), "ult": (counter, bound), "ne": (counter, bound),
                            "sgt": (bound, counter), "ugt": (bound, counter), "en": (bound, counter)}[style]
                predicate = "ne" if style == "en" else style
                # The indices it carries besides: each forwarded as it is, an argument of the body.
                more = len(mine)
                types = ", ".join(["index"] * (1 + more))
                def placed(counter_text, others):
                    """The loop's values in their order: the counter first, or after the indices."""
                    values = [counter_text] + others
                    if statement.get("counter_place", "first") == "second":
                        values = others + [counter_text]
                    return ", ".join(values)
                lines += [indent + f"%{role}res{number}" + (f":{1 + more}" if more else "") +
                          " = scf.while (" +
                          placed(f"{counter} = %{role}lo{number}", [f"%{name}w = %{name}start" for name, _, _ in mine]) +
                          ") : " + f"({types}) -> " + (f"({types})" if more else "index") + " {"] + before + \
                         [indent + f"  %{role}cmp{number} = arith.cmpi {predicate}, {operands[0]}, {operands[1]} : index",
                          indent + f"  scf.condition(%{role}cmp{number}) " +
                          placed(counter, [f"%{name}w" for name, _, _ in mine]) + f" : {types}",
                          indent + "} do {",
                          indent + "^bb0(" + placed(f"%{role}{number}: index", [f"{held(name)}: index" for name, _, _ in mine]) + "):"]
                stepped = (f"%{role}{number}, %{role}st{number}" if statement.get("step", "counter+1") == "counter+1"
                           else f"%{role}st{number}, %{role}{number}")
                closers.append([indent + f"  %{role}nx{number} = arith.addi {stepped} : index"] +
                               carried_steps(role, indent) +
                               [indent + "  scf.yield " +
                                placed(f"%{role}nx{number}", [handed(name) for name, _, _ in mine]) + f" : {types}",
                                indent + "}"])
            lines += body
            for close in reversed(closers):
                lines += close
            continue
        for depth, role in enumerate(statement["order"]):
            low, high = bounds[role]
            lines.append("  " * (depth + 1) + f"affine.for %{role}{number} = {low} to {high} {{")
        for name, operand in (("z", "c"), ("x", "a"), ("y", "b")):
            array = statement[operand]
            lines.append(f"{pad}%{name}{number} = affine.load %{array}[{index(statement['views'][operand], number)}]"
                         f" : {memref(*shapes[array], e)}")
        mul, addition = ("arith.mulf", "arith.addf") if e == "f32" else ("arith.muli", "arith.addi")
        lines.append(f"{pad}%m{number} = {mul} %x{number}, %y{number} : {e}")
        operands = f"%z{number}, %m{number}" if statement["first"] else f"%m{number}, %z{number}"
        flag = " fastmath<reassoc>" if statement["flag"] else ""
        lines.append(f"{pad}%s{number} = {addition} {operands}{flag} : {e}")
        lines.append(f"{pad}affine.store %s{number}, %{statement['c']}[{index(statement['views']['c'], number)}]"
                     f" : {memref(*shapes[statement['c']], e)}")
        for depth in reversed(range(3)):
            lines.append("  " * (depth + 1) + "}")
    lines += ["  return", "}"]
    return "\n".join(lines)


def mlir_module(specs):
    return "\n".join(mlir_function(f) for f in specs) + "\n"


# ---------------------------------------------------------------------------------------
# The C driver: the reference, the carried bounds, the comparison.

def c_driver(specs, lowered):
    """`lowered`: the set of (function, statement number) the pass lowered."""
    out = ["#include <math.h>", "#include <stdint.h>", "#include <stdio.h>", "#include <string.h>",
           '#include "systolic_offload.h"', "",
           "static const double U = 5.9604644775390625e-08; /* 2^-24 */",
           "static double gamma_of(double n) { return n * U / (1.0 - n * U); }",
           "static uint64_t state = 20261117u;",
           "static double next_unit(void) { state = state * 6364136223846793005ull + 1442695040888963407ull;"
           " return (double)(state >> 11) / 9007199254740992.0; }",
           "static int bad = 0;", ""]
    main = ["int main(void) {"]
    for function in specs:
        name, e = function["name"], function["element"]
        ctype = "float" if e == "f32" else "int32_t"
        shapes = {array: (rows, cols) for array, rows, cols, _ in function["arrays"]}
        flat = function.get("flat", ())
        prototype = ", ".join(f"{ctype} *, {ctype} *, int64_t, int64_t, int64_t" if array in flat else
                              f"{ctype} *, {ctype} *, int64_t, int64_t, int64_t, int64_t, int64_t"
                              for array, _, _, _ in function["arrays"])
        out.append(f"void {name}({prototype});")
        out.append(f"static void run_{name}(int exact) {{")
        for array, rows, cols, _ in function["arrays"]:
            out.append(f"  static {ctype} {array}[{rows}][{cols}], ref_{array}[{rows}][{cols}];"
                       f" static double err_{array}[{rows}][{cols}];")
            out.append(f"  for (int r = 0; r < {rows}; ++r) for (int s = 0; s < {cols}; ++s) {{")
            if e == "f32":
                out.append(f"    {array}[r][s] = exact ? (float)((int)(next_unit() * 7.0) - 3)"
                           f" : (float)(2.0 * next_unit() - 1.0);")
            else:
                out.append(f"    {array}[r][s] = (int32_t)((int)(next_unit() * 7.0) - 3);")
            out.append(f"    ref_{array}[r][s] = {array}[r][s]; err_{array}[r][s] = 0.0; }}")
        arguments = ", ".join(f"&{array}[0][0], &{array}[0][0], 0, {rows * cols}, 1" if array in flat else
                              f"&{array}[0][0], &{array}[0][0], 0, {rows}, {cols}, {cols}, 1"
                              for array, rows, cols, _ in function["arrays"])
        out.append("  systolic_offload_stats_t before, after; systolic_offload_stats(&before);")
        out.append(f"  {name}({arguments});")
        out.append("  systolic_offload_stats(&after);")
        out.append("  unsigned long long invocations = 0;")
        out.append("  for (int d = 0; d < 4; ++d) invocations += after.per_device[d] - before.per_device[d];")
        # The reference, statement by statement, with the error each cell may carry.
        for number, statement in enumerate(function["statements"]):
            if statement["kind"] == "move":
                (source, sr, sc), (target, tr, tc) = statement["from"], statement["to"]
                out.append(f"  ref_{target}[{tr}][{tc}] = ref_{source}[{sr}][{sc}];"
                           f" err_{target}[{tr}][{tc}] = err_{source}[{sr}][{sc}];")
                continue
            if statement["kind"] == "fill":
                array = statement["array"]
                rows, cols = shapes[array]
                out.append(f"  for (int r = 0; r < {rows}; ++r) for (int s = 0; s < {cols}; ++s) {{"
                           f" ref_{array}[r][s] = 0; err_{array}[r][s] = 0.0; }}")
                continue
            c, a, b = statement["c"], statement["a"], statement["b"]
            rows, cols = shapes[c]
            device = (name, number) in lowered
            out.append(f"  {{ static double c0[{rows}][{cols}], ec[{rows}][{cols}], sum_abs[{rows}][{cols}],"
                       f" carried[{rows}][{cols}], count[{rows}][{cols}];")
            out.append(f"    for (int r = 0; r < {rows}; ++r) for (int s = 0; s < {cols}; ++s) {{"
                       f" c0[r][s] = ref_{c}[r][s]; ec[r][s] = err_{c}[r][s]; sum_abs[r][s] = 0;"
                       f" carried[r][s] = 0; count[r][s] = 0; }}")
            pad = "    "
            for role in statement["order"]:
                low, high = statement["bounds"][role]
                out.append(f"{pad}for (int {role} = {low}; {role} < {high}; ++{role})")
                pad += "  "
            def at(operand):
                def one(entry):
                    if entry[0] == "perm":
                        _, role, multiplier, modulus = entry
                        return f"({role} * {multiplier}) % {modulus}"
                    role, scale, constant = entry
                    return f"{role} * {scale} + {constant}"
                return "][".join(one(entry) for entry in statement["views"][operand])
            ca, cb, cc = at("a"), at("b"), at("c")
            out.append(f"{pad}{{")
            if e == "f32":
                product_text = f"ref_{a}[{ca}] * ref_{b}[{cb}]"
                if statement["first"]:
                    out.append(f"{pad}  ref_{c}[{cc}] = ref_{c}[{cc}] + {product_text};")
                else:
                    out.append(f"{pad}  ref_{c}[{cc}] = {product_text} + ref_{c}[{cc}];")
                out.append(f"{pad}  double va = fabs((double)ref_{a}[{ca}]), vb = fabs((double)ref_{b}[{cb}]);")
                out.append(f"{pad}  double ea = err_{a}[{ca}], eb = err_{b}[{cb}];")
                out.append(f"{pad}  sum_abs[{cc}] += (va + ea) * (vb + eb);")
                out.append(f"{pad}  carried[{cc}] += ea * vb + va * eb + ea * eb;")
                out.append(f"{pad}  count[{cc}] += 1;")
            else:
                out.append(f"{pad}  ref_{c}[{cc}] = (int32_t)((uint32_t)ref_{c}[{cc}] + "
                           f"(uint32_t)ref_{a}[{ca}] * (uint32_t)ref_{b}[{cb}]);")
            out.append(f"{pad}}}")
            if e == "f32":
                out.append(f"    for (int r = 0; r < {rows}; ++r) for (int s = 0; s < {cols}; ++s) if (count[r][s] > 0) {{")
                out.append(f"      double rounded = ({1 if device else 0} || ec[r][s] > 0 || carried[r][s] > 0)"
                           f" ? 2.0 * gamma_of(count[r][s] + 2) * (fabs(c0[r][s]) + ec[r][s] + sum_abs[r][s]) : 0.0;")
                out.append(f"      err_{c}[r][s] = ec[r][s] + carried[r][s] + rounded; }}")
            out.append("  }")
        # The comparison.
        out.append("  long cells = 0, same = 0, differing = 0; double worst = 0;")
        for array, rows, cols, _ in function["arrays"]:
            out.append(f"  for (int r = 0; r < {rows}; ++r) for (int s = 0; s < {cols}; ++s) {{")
            out.append("    ++cells;")
            out.append(f"    int equal = memcmp(&{array}[r][s], &ref_{array}[r][s], sizeof({array}[r][s])) == 0;")
            if e == "f32":
                out.append(f"    double diff = fabs((double){array}[r][s] - (double)ref_{array}[r][s]);")
                out.append(f"    double bound = err_{array}[r][s] * (1.0 + 1e-9);")
                out.append("    if (equal) ++same; else ++differing;")
                out.append("    if (bound > 0 && diff / bound > worst) worst = diff / bound;")
                out.append(f"    if (!equal && !(diff <= bound)) {{ ++bad; printf(\"BAD {name} {array}[%d][%d] "
                           f"device %.9g reference %.9g bound %.3g exact=%d\\n\", r, s, (double){array}[r][s],"
                           f" (double)ref_{array}[r][s], bound, exact); }}")
                out.append("    if (exact && !equal) { ++bad; printf(\"BAD %s exact data, other bits\\n\", \"" + name + "\"); }")
            else:
                out.append("    if (equal) ++same; else { ++differing; ++bad;"
                           f" printf(\"BAD {name} {array}[%d][%d] integer %d reference %d\\n\", r, s,"
                           f" (int){array}[r][s], (int)ref_{array}[r][s]); }}")
            out.append("  }")
        out.append(f"  printf(\"RESULT {name} exact=%d invocations=%llu failed=%llu cells=%ld same=%ld differing=%ld "
                   f"worst=%.3f\\n\", exact, invocations, after.failed - before.failed, cells, same, differing, worst);")
        out.append("}")
        main.append(f"  run_{name}(1);")
        main.append(f"  run_{name}(0);")
    main += ["  return bad ? 1 : 0;", "}"]
    return "\n".join(out + main) + "\n"


# ---------------------------------------------------------------------------------------
# The pipeline.

def run(command, cwd=None, timeout=3600):
    done = subprocess.run([str(c) for c in command], cwd=cwd, capture_output=True, text=True, timeout=timeout)
    return done


def must(command, what, cwd=None):
    done = run(command, cwd)
    if done.returncode != 0:
        raise Failure(f"{what} exits {done.returncode}: {done.stderr[-1500:]}")
    return done


def records(args, source, out, authorize, roots=None, extra=()):
    """`roots`: authorize every pair among this many roots (by default, as many as the
    functions of this file have arrays at most). `extra`: more options of the nest driver."""
    roots = roots or max(len(f["arrays"]) for f in FUNCTIONS)
    pairs = ",".join(f"roots {a} and {b} are separate" for a in range(roots) for b in range(a + 1, roots))
    command = [args.nest_driver, source, "--index-bit-width=64"] + SOLVER + ([f"--authorize={pairs}"] if authorize else []) + \
        list(extra)
    done = must(command, "the nest driver")
    out.write_text(done.stdout)


def statement_lines(text):
    """The line of each product's outermost loop in the printed module, by (function, number)."""
    places, function = {}, None
    for line_number, line in enumerate(text.splitlines(), start=1):
        found = re.match(r"func\.func @(\w+)\(", line)
        if found:
            function = found.group(1)
            continue
        found = (re.match(r"  (?:affine|scf)\.for %[a-z]+(\d+) = ", line) or
                 re.match(r"  %[a-z]+res(\d+)(?::\d+)? = scf\.(?:while|for) ", line))
        if found:
            places[(function, int(found.group(1)))] = line_number
    return places


def lowered_set(remarks, places):
    by_line = {line: key for key, line in places.items()}
    lowered, regions, refused = set(), {}, {}
    for match in re.finditer(r":(\d+):\d+: remark: (.*)", remarks):
        line, text = int(match.group(1)), match.group(2)
        key = by_line.get(line)
        if key is None:
            continue
        found = re.match(r"(?:affine GEMM \d+x\d+x\d+|GEMM \d+x\d+x\d+ of nest \d+ of \w+) lowered (?:from its record )?"
                         r"as (\d+) exact region", text)
        if found:
            lowered.add(key)
            regions[key] = int(found.group(1))
        elif text.startswith("affine GEMM not lowered") or text.startswith("GEMM not lowered from its record"):
            refused[key] = text
    return lowered, regions, refused


def tabled_set(remarks, places):
    """The products lowered from their record by tables (stage 3): gathers, the GEMM, a scatter."""
    by_line = {line: key for key, line in places.items()}
    out = set()
    for match in re.finditer(r":(\d+):\d+: remark: (.*)", remarks):
        key = by_line.get(int(match.group(1)))
        if key is not None and " lowered from its record " in match.group(2) and "(tables:" in match.group(2):
            out.add(key)
    return out


def check_calls(specs, text, asynchronous, lowered, tabled=frozenset()):
    """In the lowered module, each function's statements in order: a lowered GEMM is its runtime
    calls and the write-back of C (asynchronously: begin, submits, wait, write-back) with
    nothing of the program between them, where the nest stood. A GEMM lowered by tables (stage
    3) is the gathers of A, B and C, the GEMM, and the scatter of C, in that order."""
    patterns = ((r"@fpga_gemm_begin", "begin"), (r"@fpga_tile_submit", "submit"), (r"@fpga_gemm_wait", "wait"),
                (r"@fpga_matmul_tiled_auto_scheduled", "call"), (r"materialize_in_destination", "writeback"),
                (r"memref\.load ", "load"), (r"memref\.store ", "store"), (r"(affine|scf)\.for |scf\.while ", "loop"))
    for function in specs:
        name = function["name"]
        body = re.search(rf"func\.func @{name}\(.*?\n  \}}\n", text, re.S)
        if not body:
            raise Failure(f"{name}: no body in the lowered module")
        events = []
        for line in body.group(0).splitlines():
            if not re.match(r"    \S", line):  # the function's own statements only
                continue
            for pattern, event in patterns:
                if re.search(pattern, line):
                    events.append(event)
                    break
        squeezed, i = [], 0
        while i < len(events):
            event = events[i]
            if asynchronous and event == "begin":
                j = i + 1
                while j < len(events) and events[j] == "submit":
                    j += 1
                if j == i + 1 or events[j:j + 2] != ["wait", "writeback"]:
                    raise Failure(f"{name}: an asynchronous GEMM is not begin, submits, wait, write-back: {events}")
                squeezed.append("gemm")
                i = j + 2
            elif not asynchronous and event == "call":
                j = i
                while j < len(events) and events[j] == "call":
                    j += 1
                if events[j:j + 1] != ["writeback"]:
                    raise Failure(f"{name}: a synchronous GEMM is not calls, then the write-back: {events}")
                squeezed.append("gemm")
                i = j + 1
            elif event in ("loop", "load", "store"):
                squeezed.append(event)
                i += 1
            else:
                raise Failure(f"{name}: a {event} out of place at the top of the function: {events}")
        expected = []
        for number, statement in enumerate(function["statements"]):
            if statement["kind"] == "move":
                expected += ["load", "store"]
            elif statement["kind"] == "fill":
                expected.append("loop")
            elif (name, number) in lowered and (name, number) in tabled:
                expected += ["loop", "loop", "loop", "gemm", "loop"]
            else:
                expected.append("gemm" if (name, number) in lowered else "loop")
        if squeezed != expected:
            raise Failure(f"{name}: statements and calls in the lowered module are {squeezed}, expected {expected}")


def variant(args, work, name, specs, records_path, expected_lowered, counts, extra_option="", expected_tabled=None):
    directory = work / name
    directory.mkdir()
    source = directory / "module.mlir"
    source_text = mlir_module(specs)
    source.write_text(source_text)
    option = "report=true" + (f" nest-records={records_path}" if records_path else "") + extra_option
    done = must([args.systolic_opt, source, FLEET, f"--affine-matmul-to-systolic={option}", "-o", directory / "40.mlir"],
                f"{name}: the pass")
    (directory / "remarks.txt").write_text(done.stderr)
    places = statement_lines(source_text)
    lowered, regions, refused = lowered_set(done.stderr, places)
    if lowered != expected_lowered:
        raise Failure(f"{name}: lowered {sorted(lowered)}, expected {sorted(expected_lowered)}; refusals: {refused}")
    tabled = tabled_set(done.stderr, places)
    if expected_tabled is not None and tabled != expected_tabled:
        raise Failure(f"{name}: lowered by tables {sorted(tabled)}, expected {sorted(expected_tabled)}")
    tiles = (directory / "40.mlir").read_text()
    for function in specs:
        body = re.search(rf"func\.func @{function['name']}\(.*?\n  \}}\n", tiles, re.S).group(0)
        expected_tiles = any((function["name"], n) in lowered for n in range(len(function["statements"])))
        if ("systolic.matmul_tile" in body) != expected_tiles:
            raise Failure(f"{name}: {function['name']} has tiles: {'systolic.matmul_tile' in body}")
    counts["variants with the expected nests lowered"] += 1
    if "systolic.matmul_tile" not in tiles:
        # Nothing was lowered: the fleet's declarations are all that is left of the systolic
        # dialect, and the FPGA lowering removes them only with the tiles it lowers. (The
        # compiler driver never sends such a module down this path; the host code it keeps
        # is what runs here.)
        kept = [line for line in tiles.splitlines() if not re.match(r"\s*systolic\.device ", line)]
        (directory / "40.mlir").write_text("\n".join(kept) + "\n")
        counts["variants with nothing lowered, run on the host"] += 1
    for asynchronous in (False, True):
        mode = "async" if asynchronous else "sync"
        lowering = f"--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8={K_MAX} phys-k-max-4x4={K_MAX}" + \
                   (" async=true" if asynchronous else "")
        must([args.systolic_opt, directory / "40.mlir", lowering, "-o", directory / f"50-{mode}.mlir"], f"{name}: FPGA lowering")
        check_calls(specs, (directory / f"50-{mode}.mlir").read_text(), asynchronous, lowered, tabled)
        counts["lowered modules with calls in program order"] += 1
        must([args.systolic_opt, directory / f"50-{mode}.mlir", "--systolic-runtime-to-llvm", "-o",
              directory / f"60-{mode}.mlir"], f"{name}: to LLVM")
        must([args.mlir_translate, "--mlir-to-llvmir", directory / f"60-{mode}.mlir", "-o", directory / f"70-{mode}.ll"],
             f"{name}: mlir-translate")
        must([args.llc, "-O0", "-filetype=obj", "-relocation-model=pic", directory / f"70-{mode}.ll", "-o",
              directory / f"70-{mode}.o"], f"{name}: llc")
        driver = directory / "driver.c"
        driver.write_text(c_driver(specs, lowered))
        program = directory / f"program-{mode}"
        must([args.clang, "-std=c11", "-O1", "-ffp-contract=off", f"-I{args.runtime}", driver, directory / f"70-{mode}.o",
              Path(args.runtime) / "systolic_cc_report.c", Path(args.runtime) / "soft_runtime.c",
              f"-DSOFT_DEFAULT_K_MAX_8X8={K_MAX}", f"-DSOFT_DEFAULT_K_MAX_4X4={K_MAX}", f"-L{args.llvm_lib}",
              "-lmlir_c_runner_utils", f"-Wl,-rpath,{args.llvm_lib}", "-lm", "-o", program], f"{name}: link")
        ran = run([program], cwd=directory)
        (directory / f"run-{mode}.txt").write_text(ran.stdout + ran.stderr)
        if ran.returncode != 0:
            raise Failure(f"{name} ({mode}): the program exits {ran.returncode}:\n" +
                          "\n".join(l for l in ran.stdout.splitlines() if l.startswith("BAD"))[:3000])
        for line in ran.stdout.splitlines():
            found = re.match(r"RESULT (\w+) exact=(\d) invocations=(\d+) failed=(\d+) cells=(\d+) same=(\d+) "
                             r"differing=(\d+) worst=([\d.]+)", line)
            if not found:
                continue
            function, exact, invocations, failed = found.group(1), int(found.group(2)), int(found.group(3)), int(found.group(4))
            differing = int(found.group(7))
            expected = sum(r for (f, _), r in regions.items() if f == function)
            if invocations != expected or failed:
                raise Failure(f"{name} ({mode}) {function}: {invocations} invocations ran and {failed} failed; "
                              f"the remarks announce {expected}")
            counts["function runs with the announced invocations"] += 1
            counts["function runs compared with the reference"] += 1
            if not exact and differing:
                counts["runs where some lowered cell has other bits, within its bound"] += 1
            if not differing:
                counts["runs where every cell keeps its bits"] += 1
        counts["programs run"] += 1


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--nest-driver", required=True)
    parser.add_argument("--systolic-opt", required=True)
    parser.add_argument("--mlir-translate", required=True)
    parser.add_argument("--llc", required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--runtime", required=True, help="runtime/offload of the source tree")
    parser.add_argument("--llvm-lib", required=True, help="the LLVM build's lib, with mlir_c_runner_utils")
    parser.add_argument("--keep", default="", help="keep the work here")
    args = parser.parse_args()
    from collections import Counter
    counts = Counter()
    work = Path(args.keep) if args.keep else Path(tempfile.mkdtemp(prefix="nest-e2e-"))
    if args.keep:
        work.mkdir(parents=True, exist_ok=False)
    failures = []
    try:
        (work / "module.mlir").write_text(mlir_module(functions()))
        records(args, work / "module.mlir", work / "records.json", authorize=True)
        records(args, work / "module.mlir", work / "unauthorized.json", authorize=False)
        everything = {(f["name"], n) for f in FUNCTIONS for n, s in enumerate(f["statements"]) if s["kind"] == "product"}
        host = {("e2e_strict", 0), ("e2e_integer", 0)}
        gated = everything - host
        plan = [("records", functions(), work / "records.json", gated),
                ("unauthorized", functions(), work / "unauthorized.json", set()),
                ("stale", functions(stale=True), work / "records.json", gated - {("e2e_single", 0)}),
                ("off", functions(), None, gated)]
        for name, source_text, records_path, expected in plan:
            try:
                variant(args, work, name, source_text, records_path, expected, counts)
            except Failure as failure:
                failures.append(str(failure))
    finally:
        if not args.keep:
            shutil.rmtree(work, ignore_errors=True)
    for key in sorted(counts):
        print(f"{counts[key]:6d}  {key}")
    minimum = {"variants with the expected nests lowered": 4, "lowered modules with calls in program order": 8,
               "programs run": 8, "function runs with the announced invocations": 8 * len(FUNCTIONS) * 2,
               "runs where some lowered cell has other bits, within its bound": 2}
    for key, least in minimum.items():
        if counts[key] < least:
            failures.append(f"only {counts[key]} of '{key}', fewer than {least}: the check is vacuous")
    for failure in failures:
        print("FAIL:", failure, file=sys.stderr)
    print(f"{len(failures)} failures")
    sys.exit(1 if failures else 0)


if __name__ == "__main__":
    main()
