#!/usr/bin/env python3
"""Independent source-SSA execution, exhaustive finite-state checks, native LLVM
execution and SMT-LIB replay. No P0/P2a polynomial evaluator is used by the oracle.
Native execution runs in a child with a strict timeout; only already terminating
oracle cases are invoked. A detected full-state cycle is an oracle observation,
never substituted for the analyzer's recurrent-set certificate.
"""
import argparse
import ctypes
import ctypes.util
import itertools
import json
from pathlib import Path
import random
import re
import subprocess
import sys
import tempfile


def run(*args, timeout=120):
    result = subprocess.run([str(x) for x in args], text=True, capture_output=True, timeout=timeout)
    assert result.returncode == 0, (args, result.returncode, result.stdout[-4000:], result.stderr[-4000:])
    return result.stdout


def signed(n, width):
    n %= 1 << width
    return n - (1 << width) if n >= 1 << (width - 1) else n


def generate(name, width, pred, step):
    ty = f"i{width}"
    guard = f"arith.cmpi {pred}, %i, %bound : {ty}" if pred not in ("true", "false") else f"arith.constant {pred}"
    return f"""func.func @{name}(%start: {ty}, %bound: {ty}) -> {ty} {{
  %step = arith.constant {step} : {ty}
  cf.br ^header(%start : {ty})
^header(%i: {ty}):
  %g = {guard}
  cf.cond_br %g, ^body, ^exit
^body:
  %next = arith.addi %i, %step : {ty}
  cf.br ^header(%next : {ty})
^exit:
  return %i : {ty}
}}"""


def bounded_clone(name, source, width):
    """Execution-only clone: force exit after 64 continuing transitions.
    The original guard and counter operations are preserved. This trace check
    cannot establish nontermination and is never fed to the analyzer.
    """
    return (source.replace("@"+name+"(", "@bounded_"+name+"(")
            .replace("  cf.br ^header(%start", "  %budget = arith.constant 64 : i32\n  %zero = arith.constant 0 : i32\n  %one = arith.constant 1 : i32\n  cf.br ^header(%zero, %start")
            .replace(f"%start : i{width})", f"%start : i32, i{width})")
            .replace("^header(%i:", "^header(%visits: i32, %i:")
            .replace("  cf.cond_br %g,", "  %within = arith.cmpi ult, %visits, %budget : i32\n  %continue = arith.andi %g, %within : i1\n  cf.cond_br %continue,")
            .replace("  cf.br ^header(%next", "  %visits_next = arith.addi %visits, %one : i32\n  cf.br ^header(%visits_next, %next")
            .replace(f"%next : i{width})", f"%next : i32, i{width})"))


class SSA:
    def __init__(self, source, width):
        self.width = width
        self.args = re.findall(r"(%\w+): i\d+", source.splitlines()[0])
        self.blocks = {"entry": ([], [])}
        current = "entry"
        for raw in source.splitlines()[1:]:
            line = raw.strip()
            if not line or line == "}":
                continue
            if line.startswith("^"):
                current = re.match(r"\^(\w+)", line)[1]
                self.blocks[current] = (re.findall(r"(%\w+): i\d+", line), [])
            else:
                self.blocks[current][1].append(line)

    def execute(self, values):
        mask = (1 << self.width) - 1
        env = dict(zip(self.args, [n & mask for n in values]))
        block, visited, count = "entry", set(), 0
        for _ in range(100000):  # strict oracle watchdog; never a proof
            if block == "header":
                state = tuple(env[v] for v in self.blocks[block][0])
                if state in visited:
                    return "cycle", None, count
                visited.add(state)
            for line in self.blocks[block][1]:
                if line.startswith("return"):
                    return "exit", env[line.split()[1]], count
                if line.startswith("cf.cond_br"):
                    m = re.fullmatch(r"cf.cond_br (%\w+), \^(\w+), \^(\w+)", line)
                    block = m[2] if env[m[1]] else m[3]
                    if block != "exit":
                        count += 1
                    break
                if line.startswith("cf.br"):
                    m = re.fullmatch(r"cf.br \^(\w+)(?:\((.*?) : .*\))?", line)
                    block = m[1]
                    values = [env[x.strip()] for x in m[2].split(",")] if m[2] else []
                    env.update(zip(self.blocks[block][0], values))  # simultaneous
                    break
                m = re.fullmatch(r"(%\w+) = arith\.(\w+) (.*?)(?: : i\d+)?", line)
                dest, op, operands = m.groups()
                if op == "constant":
                    value = {"true": 1, "false": 0}.get(operands)
                    if value is None:
                        value = int(operands)
                elif op == "cmpi":
                    pred, a, b = operands.split(", ")
                    a, b = env[a], env[b]
                    if pred.startswith("s"):
                        a, b = signed(a, self.width), signed(b, self.width)
                    value = {"lt": a < b, "le": a <= b, "gt": a > b, "ge": a >= b}[pred[1:]]
                else:
                    a, b = (env[x] for x in operands.split(", "))
                    value = {"addi": lambda: a+b, "subi": lambda: a-b, "muli": lambda: a*b}[op]()
                env[dest] = int(value) & mask
            else:
                raise AssertionError("no terminator")
        raise AssertionError("oracle watchdog exhausted, not a termination result")


class Replay:
    """Separate Z3 C-API consumer of the recorded script, no project verifier."""
    def __init__(self, library):
        self.z = ctypes.CDLL(library)
        signatures = {
            "Z3_mk_config": ([], ctypes.c_void_p),
            "Z3_del_config": ([ctypes.c_void_p], None),
            "Z3_mk_context": ([ctypes.c_void_p], ctypes.c_void_p),
            "Z3_del_context": ([ctypes.c_void_p], None),
            "Z3_mk_solver": ([ctypes.c_void_p], ctypes.c_void_p),
            "Z3_solver_inc_ref": ([ctypes.c_void_p, ctypes.c_void_p], None),
            "Z3_solver_dec_ref": ([ctypes.c_void_p, ctypes.c_void_p], None),
            "Z3_solver_from_string": ([ctypes.c_void_p, ctypes.c_void_p, ctypes.c_char_p], None),
            "Z3_solver_check": ([ctypes.c_void_p, ctypes.c_void_p], ctypes.c_int),
        }
        for name, (arguments, result) in signatures.items():
            getattr(self.z, name).argtypes = arguments
            getattr(self.z, name).restype = result
        self.count = 0

    def check(self, report):
        for q in report["obligations"]:
            assert q["outcome"] in ("sat", "unsat"), q
            config = self.z.Z3_mk_config()
            context = self.z.Z3_mk_context(config)
            self.z.Z3_del_config(config)
            solver = self.z.Z3_mk_solver(context)
            self.z.Z3_solver_inc_ref(context, solver)
            try:
                # API parameters govern the implementation; put explicit replay
                # limits in the script as recorded. An outer CTest timeout is
                # also imposed on this entire separate consumer.
                self.z.Z3_solver_from_string(context, solver, q["smtlib"].encode())
                result = self.z.Z3_solver_check(context, solver)
                assert {1: "sat", -1: "unsat", 0: "unknown"}[result] == q["outcome"], q["name"]
                self.count += 1
            finally:
                self.z.Z3_solver_dec_ref(context, solver)
                self.z.Z3_del_context(context)
        if report["status"] != "UNKNOWN":
            last_start = max(i for i, q in enumerate(report["obligations"]) if q["name"] == "nonempty_domain")
            assert all(q["outcome"] == q["expected"] for q in report["obligations"][last_start:])
        else:
            assert report["certificate"] is None


def native_child(library, cases):
    lib = ctypes.CDLL(library)
    inputs = json.loads(Path(cases).read_text())
    for name, values, expected in inputs:
        fn = getattr(lib, "wrap_"+name)
        fn.argtypes = [ctypes.c_uint64] * len(values)
        fn.restype = ctypes.c_uint64
        actual = fn(*values)
        assert actual == expected, (name, values, expected, actual)
    print(f"{len(inputs)} native LLVM executions PASS")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--mlir-opt", required=True)
    parser.add_argument("--mlir-translate", required=True)
    parser.add_argument("--clang", required=True)
    parser.add_argument("--z3-library", required=True)
    args = parser.parse_args()
    cases = {}
    for width in (3, 4):
        for pred, step in itertools.product(("slt", "sle", "sgt", "sge", "ult", "ule", "ugt", "uge"), (1, -1, 2, -2, 0)):
            name = f"w{width}_{pred}_{str(step).replace('-', 'm')}"
            cases[name] = (generate(name, width, pred, step), width, pred, step)
        for pred in ("true", "false"):
            name = f"w{width}_{pred}"
            cases[name] = (generate(name, width, pred, 1), width, pred, 1)
    # Boundary-domain probes avoid impractically long native runs at large widths.
    for width in (8, 16, 32, 64):
        for pred, step in (("slt", 1), ("sgt", -1), ("ult", 1), ("ugt", -1)):
            name = f"w{width}_{pred}_unit"
            cases[name] = (generate(name, width, pred, step), width, pred, step)
    source = (Path(__file__).parent/"Inputs/loops.mlir").read_text()
    forwarded = re.search(r"  func.func @forwarded\(.*?\n  }", source, re.S)[0].strip()
    # Dead unreachable arithmetic is kept in the original source. The source
    # interpreter follows only executed edges, like the native program.
    cases["forwarded"] = (forwarded, 8, "forwarded", 1)
    counts = {"PROVEN_TERMINATING": 0, "PROVEN_NONTERMINATING": 0, "UNKNOWN": 0}
    executions = cycles = exits = mixed = 0
    native, bounded_definitions = [], {}
    bounded_executions = 0
    rng = random.Random(0x502B)
    replay = Replay(args.z3_library)
    with tempfile.TemporaryDirectory(prefix="polynomial-termination-") as directory:
        directory = Path(directory)
        inp = directory/"loops.mlir"
        inp.write_text("module {\n" + "\n".join(c[0] for c in cases.values()) + "\n}\n")
        run(args.mlir_opt, inp, "--verify-each", "-o", directory/"verified.mlir")
        raw = run(args.driver, inp)
        assert raw == run(args.driver, inp), "deterministic independent processes"
        document = json.loads(raw)
        assert document["read_only"]
        reports = {r["function"]: r["analysis"] for r in document["functions"]}
        # External textual IR preservation, independently of the tool's assertion.
        printed = run(args.driver, inp, "--print-ir")
        assert printed == run(args.mlir_opt, inp), "IR unchanged"
        probes = [(name, {}, reports[name]) for name in cases]
        for name, ranges in [
            ("w3_sle_1", {1: (3, 3)}), ("w3_sle_1", {1: (-4, 2)}),
            ("w3_sge_m1", {1: (-4, -4)}), ("w3_sge_m1", {1: (-3, 3)}),
            ("w3_slt_2", {1: (-4, 2)}), ("w3_slt_2", {1: (3, 3)}),
            ("w3_ult_2", {1: (0, 3)}), ("w3_ult_2", {1: (-1, -1)}),
            ("w3_slt_0", {0: (0, 0), 1: (1, 1)}),
            ("w3_slt_0", {0: (1, 1), 1: (1, 1)}),
            ("w3_sgt_0", {0: (1, 1), 1: (0, 0)})]:
            flags = [f"--parameter-range={index}:{lo}:{hi}" for index, (lo, hi) in ranges.items()]
            report = json.loads(run(args.driver, inp, "--function="+name, *flags))["functions"][0]["analysis"]
            probes.append((name, ranges, report))
        for name, ranges, report in probes:
            text, width, pred, step = cases[name]
            counts[report["status"]] += 1
            replay.check(report)
            interpreter = SSA(text, width)
            if width <= 4:
                choices = [[x for x in range(1 << width) if ranges.get(i, (-(1 << (width-1)), (1 << (width-1))-1))[0] <= signed(x, width) <= ranges.get(i, (-(1 << (width-1)), (1 << (width-1))-1))[1]] for i in range(len(interpreter.args))]
                assignments = itertools.product(*choices)
            elif name == "forwarded":
                assignments = [(s, n, a) for s, n, a in itertools.product((-128, -3, 0, 2, 126, 127), (-128, 0, 4, 127), (-128, 0, 127))]
            else:
                boundary = [0, 1, (1 << (width-1))-1, 1 << (width-1), (1 << width)-1]
                values = [(x, x) for x in boundary]
                # Only short terminating traces; arbitrary huge trip counts are
                # universally covered by SMT, not native execution.
                for x in boundary + [rng.randrange(1 << width) for _ in range(5)]:
                    for length in (1, 2, 5):
                        bound = (x + step*length) % (1 << width)
                        a, b = (signed(x, width), signed(bound, width)) if pred[0] == "s" else (x, bound)
                        if (step > 0 and a <= b) or (step < 0 and a >= b):
                            values.append((x, bound))
                assignments = values
            behaviors = set()
            for values in assignments:
                outcome, final, length = interpreter.execute(values)
                executions += 1
                behaviors.add(outcome)
                if outcome == "exit":
                    exits += 1
                else:
                    cycles += 1
                if report["status"] == "PROVEN_TERMINATING":
                    assert outcome == "exit", (name, values, report)
                    native.append((name, values, final))
                elif report["status"] == "PROVEN_NONTERMINATING":
                    assert outcome == "cycle", (name, values, report)
                    # Every nontermination fixture here is a single counter.
                    # Compare 64 actual source updates with independent exact
                    # modular addition. The watchdog result is not a proof.
                    bounded_definitions["bounded_"+name] = (bounded_clone(name, text, width), width, pred, step)
                    native.append(("bounded_"+name, values, (values[0]+64*step) % (1 << width)))
                    bounded_executions += 1
            if len(behaviors) == 2:
                mixed += 1
                assert report["status"] == "UNKNOWN", (name, ranges, report)
            if not ranges:
                expected = "PROVEN_NONTERMINATING" if pred == "true" else "PROVEN_TERMINATING" if (pred in ("false", "forwarded") or pred in ("slt", "ult") and step == 1 or pred in ("sgt", "ugt") and step == -1) else "UNKNOWN"
                assert report["status"] == expected, (name, expected, report["status"], report["reason"])
        # Native wrappers preserve original function bodies. Explicit conversions
        # at the wrapper ABI avoid relying on unspecified small-integer C ABI bits.
        wrappers = []
        native_definitions = dict(cases, **bounded_definitions)
        for name in sorted({x[0] for x in native}):
            text, width, _, _ = native_definitions[name]
            arity = len(SSA(text, width).args)
            signature = ", ".join(f"%a{i}: i64" for i in range(arity))
            lines = [f"func.func @wrap_{name}({signature}) -> i64 {{"]
            for i in range(arity):
                if width != 64:
                    lines.append(f"  %v{i} = arith.trunci %a{i} : i64 to i{width}")
            operands = ", ".join(f"%{'a' if width == 64 else 'v'}{i}" for i in range(arity))
            types = ", ".join([f"i{width}"]*arity)
            lines.append(f"  %r = func.call @{name}({operands}) : ({types}) -> i{width}")
            if width != 64:
                lines.append(f"  %wide = arith.extui %r : i{width} to i64")
            lines += [f"  return %{'r' if width == 64 else 'wide'} : i64", "}"]
            wrappers.append("\n".join(lines))
        inp.write_text("module {\n" + "\n".join(c[0] for c in native_definitions.values()) + "\n" + "\n".join(wrappers) + "\n}\n")
        lowered, llvm, library = directory/"lowered.mlir", directory/"loops.ll", directory/"loops.so"
        # The local Func-to-LLVM conversion does not convert func.return in
        # unreachable blocks. Canonicalization removes those blocks in this
        # separate execution artifact; the analyzed source above is untouched.
        run(args.mlir_opt, inp, "--canonicalize", "--convert-arith-to-llvm", "--convert-cf-to-llvm", "--convert-func-to-llvm", "--reconcile-unrealized-casts", "--verify-each", "-o", lowered)
        run(args.mlir_translate, lowered, "--mlir-to-llvmir", "-o", llvm)
        run(args.clang, "-O0", "-shared", "-fPIC", llvm, "-o", library)
        native_inputs = directory/"native.json"
        native_inputs.write_text(json.dumps(native))
        print(run(sys.executable, __file__, "--native-child", library, native_inputs, timeout=10).strip())
        print(f"Native breakdown: {len(native)-bounded_executions} original terminating executions; {bounded_executions} watchdog-bounded nonterminating prefixes (not proofs)")
    print(f"PASS: {len(cases)} verified MLIR functions; {len(probes)} status probes {counts}; {executions} independent source-SSA executions ({exits} exits, {cycles} exact full-state cycles); {mixed} mixed domains correctly UNKNOWN; {replay.count} independently replayed SMT obligations; deterministic reports and unchanged IR")


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--native-child":
        native_child(sys.argv[2], sys.argv[3])
    else:
        main()
