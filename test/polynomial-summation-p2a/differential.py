#!/usr/bin/env python3
"""P2a oracle: parse/execute generated MLIR independently using Python integers.

Reported polynomials are evaluated with Fraction/math.comb. Expected traces
come from original SSA operations and simultaneous branch transfers, and are
also checked against separately specified conventional polynomial recurrences.
"""
import argparse
from fractions import Fraction
import hashlib
import json
import math
from pathlib import Path
import random
import re
import subprocess
import tempfile


def run(*args, ok=True):
    p = subprocess.run(args, text=True, capture_output=True)
    assert (p.returncode == 0) == ok, (args, p.stdout, p.stderr)
    return p.stdout


def generate(name, width, coefficients, step, next_index):
    ty = f"i{width}"
    code = [f"func.func @{name}(%n: {ty}, %alpha: {ty}, %beta: {ty}, %gamma: {ty}) -> {ty} {{"]
    serial = 0

    def emit(op, a, b=None):
        nonlocal serial
        result = f"%v{serial}"
        serial += 1
        operands = str(a) if b is None else f"{a}, {b}"
        code.append(f"  {result} = arith.{op} {operands} : {ty}")
        return result

    zero, one = emit("constant", 0), emit("constant", 1)
    init = emit("addi", "%alpha", emit("muli", "%beta", "%gamma"))
    delta = "%gamma" if step is None else emit("constant", step)
    code += [f"  cf.br ^header({zero}, %beta, {init} : {ty}, {ty}, {ty})",
             f"^header(%t: {ty}, %i: {ty}, %s: {ty}):",
             f"  %more = arith.cmpi slt, %t, %n : {ty}",
             "  cf.cond_br %more, ^body, ^exit", "^body:"]
    new_i = emit("addi", "%i", delta)
    x = emit("addi", new_i if next_index else "%i", "%gamma")
    # Horner IR versus independent conventional sum-of-powers oracle below.
    poly = emit("constant", coefficients[-1])
    for c in reversed(coefficients[:-1]):
        poly = emit("addi", emit("muli", poly, x), emit("constant", c))
    extra = emit("muli", emit("muli", "%gamma", "%gamma"), emit("muli", x, x))
    inc = emit("addi", poly, extra)
    new_s = emit("addi", "%s", inc)
    new_t = emit("addi", "%t", one)
    code += [f"  cf.br ^header({new_t}, {new_i}, {new_s} : {ty}, {ty}, {ty})",
             "^exit:", f"  return %s : {ty}", "}"]
    return "\n".join(code)


def signed(value, width):
    value %= 1 << width
    return value - (1 << width) if value >= (1 << (width - 1)) else value


def interpret(text, arguments, width):
    """Small independent SSA interpreter, consuming text actually sent to MLIR."""
    lines = text.splitlines()
    args = re.findall(r"(%\w+): i\d+", lines[0])
    blocks = {"entry": ([], [])}
    current = "entry"
    for raw in lines[1:]:
        line = raw.strip()
        if line == "}":
            continue
        if line.startswith("^"):
            current = re.match(r"\^(\w+)", line)[1]
            blocks[current] = (re.findall(r"(%\w+): i\d+", line), [])
        else:
            blocks[current][1].append(line)
    modulus = 1 << width
    env = {name: value % modulus for name, value in zip(args, arguments)}
    current, trace = "entry", []
    for _ in range(1000):
        if current == "header":
            trace.append([env[x] for x in blocks[current][0]])
        for line in blocks[current][1]:
            if line.startswith("return"):
                return trace
            if line.startswith("cf.br"):
                match = re.fullmatch(r"cf.br \^(\w+)\((.*?) : .*\)", line)
                target, source = match[1], match[2].split(", ")
                values = [env[x] for x in source]  # simultaneous snapshot
                env.update(zip(blocks[target][0], values))
                current = target
                break
            if line.startswith("cf.cond_br"):
                match = re.fullmatch(r"cf.cond_br (%\w+), \^(\w+), \^(\w+)", line)
                current = match[2] if env[match[1]] else match[3]
                break
            match = re.fullmatch(r"(%\w+) = arith\.(\w+) (.*?) : i\d+", line)
            dest, op, operands = match.groups()
            if op == "constant":
                value = int(operands)
            elif op == "cmpi":
                pred, a, b = operands.split(", ")
                assert pred == "slt"
                env[dest] = int(signed(env[a], width) < signed(env[b], width))
                continue
            else:
                a, b = (env[x] for x in operands.split(", "))
                value = {"addi": lambda: a+b, "subi": lambda: a-b, "muli": lambda: a*b}[op]()
            env[dest] = value % modulus
        else:
            raise AssertionError("missing terminator")
    raise AssertionError("execution budget exceeded")


def evaluate(terms, iteration, assignment):
    result = Fraction(0)
    for term in terms:
        factor = math.prod(assignment[name] ** power for name, power in term["powers"].items())
        result += factor * sum(Fraction(c) * math.comb(iteration, k)
                               for k, c in enumerate(term["binomial_coefficients"]) if k <= iteration)
    assert result.denominator == 1
    return int(result)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--mlir-opt", required=True)
    args = parser.parse_args()
    rng = random.Random(0x502A)
    definitions, cases = [], []
    for j in range(40):
        width = [8, 16, 32, 128, 256][j % 5]
        coeff = [rng.randrange(-5, 6) for _ in range(1+j % 6)]
        if width == 256:
            coeff[-1] = (1 << 180) + j
        step = [None, -3, -1, 0, 2][j % 5]
        text = generate(f"case{j}", width, coeff, step, bool(j % 2))
        definitions.append(text)
        cases.append((width, coeff, step, bool(j % 2)))
    executions = values = differences = initial_checks = unknown = cli_checks = 0
    with tempfile.TemporaryDirectory(prefix="p2a-") as directory:
        path = Path(directory) / "generated.mlir"
        path.write_text("module {\n" + "\n".join(definitions) + "\n}\n")
        digest = hashlib.sha256(path.read_bytes()).hexdigest()
        run(args.mlir_opt, str(path), "--verify-each", "-o", "/dev/null")
        output = run(args.driver, str(path), "--symbolic")
        assert run(args.driver, str(path), "--symbolic") == output
        data = json.loads(output)
        assert data["read_only"] and data["deterministic"] and len(data["functions"]) == 40
        for j, record in enumerate(data["functions"]):
            width, coeff, step, next_index = cases[j]
            result = record["analysis"]
            assert result["status"] == "PROVEN", result
            assert [s["initial"] for s in result["states"]] == ["0/1", "p2", "(+ p1 (* p2 p3))"]
            assert [s["dependencies"] for s in result["states"]] == [[], [], [1]]
            assert result["states"][1]["increment"] == ("p3" if step is None else f"{step}/1")
            modulus, half = 1 << width, 1 << (width-1)
            boundary = [-half, -half+1, -17, -1, 0, 1, 17, half-1]
            for sample in range(40):
                parameters = [boundary[(sample+k) % len(boundary)] if sample < 8
                              else rng.randrange(-half, half) for k in range(3)]
                n = sample % 17
                trace = interpret(definitions[j], [n] + parameters, width)
                executions += 1
                assert len(trace) == n+1
                assignment = dict(zip(["p1", "p2", "p3"], parameters))
                alpha, beta, gamma = parameters
                i, s = beta, alpha + beta*gamma
                delta = gamma if step is None else step
                for t, actual in enumerate(trace):
                    predicted = [evaluate(state["terms"], t, assignment) for state in result["states"]]
                    assert predicted == [t, i, s]  # exact integer lifts, independent conventional recurrence
                    assert [v % modulus for v in predicted] == actual  # actual bitvector execution
                    values += len(actual)
                    if t == 0:
                        initial_checks += len(actual)
                    x = i + (delta if next_index else 0) + gamma
                    inc = sum(c*x**k for k, c in enumerate(coeff)) + gamma**2*x**2
                    future = [evaluate(state["terms"], t+1, assignment) for state in result["states"]]
                    assert [future[k]-predicted[k] for k in range(3)] == [1, delta, inc]
                    differences += 3
                    i, s = i+delta, s+inc
        assert hashlib.sha256(path.read_bytes()).hexdigest() == digest

        # New resource/semantic fixtures only; old regression sources stay intact.
        def reject(body, reason):
            nonlocal unknown
            path.write_text("module {\n" + body + "\n}\n")
            run(args.mlir_opt, str(path), "--verify-each", "-o", "/dev/null")
            result = json.loads(run(args.driver, str(path), "--symbolic"))["functions"][0]["analysis"]
            assert result["status"] == "UNKNOWN" and not result["states"] and not result["parameters"], result
            assert reason in result["reason"], result
            unknown += 1

        def resource(parameter_count, product):
            ty = "i32"
            prelude = ["  %one = arith.constant 1 : i32"]
            last = "%one"
            for k in range(parameter_count):
                prelude += [f"  %term{k} = arith.addi %p{k}, %one : i32",
                            f"  %acc{k} = arith.{'muli' if product else 'addi'} {last}, %term{k} : i32"]
                last = f"%acc{k}"
            arguments = ", ".join(f"%p{k}: {ty}" for k in range(parameter_count))
            return f"""func.func @resource(%n: i32, {arguments}) -> i32 {{
{chr(10).join(prelude)}
  %zero = arith.constant 0 : i32
  cf.br ^header(%zero, {last} : i32, i32)
^header(%t: i32, %s: i32):
  %more = arith.cmpi slt, %t, %n : i32
  cf.cond_br %more, ^body, ^exit
^body:
  %tn = arith.addi %t, %one : i32
  %sn = arith.addi %s, %one : i32
  cf.br ^header(%tn, %sn : i32, i32)
^exit:
  return %s : i32
}}"""
        reject(resource(9, True), "term count")
        reject(resource(17, False), "parameter count")
        reject(definitions[0].replace("arith.addi %i, %gamma :", "arith.addi %i, %gamma overflow<nuw> :"), "overflow flags")
        # Mode/constraint errors and explicit domains are observable CLI contracts.
        path.write_text("module {\n" + definitions[2] + "\n}\n")
        for flags in [("--mode=signed",), ("--horizon=-1",), ("--parameter-range=99:0:1",),
                      ("--parameter-range=1:3:2",), ("--parameter-range=1:-999999999999999:0",),
                      ("--assume-no-overflow=unrelated",)]:
            result=json.loads(run(args.driver,str(path),"--symbolic",*flags))["functions"][0]["analysis"]
            assert result["status"]=="UNKNOWN" and not result["states"], result
            unknown += 1
        for flags in [("--evaluate=0",), ("--parameter-range=1:0:1", "--evaluate=0", "--parameter-value=p1:2",
                                         "--parameter-value=p2:0", "--parameter-value=p3:0")]:
            failure=json.loads(run(args.driver,str(path),"--symbolic",*flags,ok=False))
            assert failure["status"]=="UNKNOWN" and not failure["states"]
            cli_checks += 1
        run(args.driver,str(path),"--parameter-range=1:0:1",ok=False)
        cli_checks += 1
    print(f"PASS: P2a Python oracle seed=0x502a; 40 verified generated functions; {executions} independent SSA executions; "
          f"{values} exact and {values} modular comparisons; {differences} finite differences; {initial_checks} initial checks; "
          f"{unknown} UNKNOWN cases; {cli_checks} CLI error checks")


if __name__ == "__main__":
    main()
