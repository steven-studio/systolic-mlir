#!/usr/bin/env python3
"""Generated real-MLIR cases, independently checked by Python int execution.

The oracle adds conventional powers directly. It does not use finite
differences, discrete integration, recurrence solving, or P0 arithmetic.
math.comb/Fraction are used only to read/evaluate the returned representation.
Existing fixtures are never edited; generated inputs live in a temporary dir.
"""
import argparse
from collections import Counter
from fractions import Fraction
import json
import math
from pathlib import Path
import random
import subprocess
import tempfile


counts = Counter()


def check(condition, description, category="assertions"):
    counts[category] += 1
    if not condition:
        raise AssertionError(description)


def run(command, expected=0):
    p = subprocess.run(command, text=True, capture_output=True, timeout=120)
    check(p.returncode == expected,
          f"{command}: exit {p.returncode}, expected {expected}\n{p.stdout}\n{p.stderr}", "commands")
    return p.stdout


def report(driver, path, *options):
    data = json.loads(run([driver, str(path), *options]))
    check(data["read_only"] and data["deterministic"], "report invariants")
    return {f["function"]: f["analysis"] for f in data["functions"]}


def evaluate(coefficients, t):
    return sum((c * math.comb(t, k) for k, c in enumerate(coefficients) if k <= t), Fraction(0))


def emit(case):
    name, width, coefficients, start, step, initial, triangular, next_sum = case
    ty = f"i{width}"
    lines = [f"func.func @{name}(%n: {ty}) -> {ty} {{"]
    constants = dict(zero=0, one=1, start=start, step=step, initial=initial, zinit=-9)
    constants.update({f"c{k}": c for k, c in enumerate(coefficients)})
    for label, value in constants.items():
        lines.append(f"  %{label} = arith.constant {value} : {ty}")
    args = ["%zero", "%start", "%initial"] + (["%zinit"] if triangular else [])
    types = ", ".join([ty] * len(args))
    lines += [f"  cf.br ^header({', '.join(args)} : {types})",
              f"^header(%t: {ty}, %i: {ty}, %s: {ty}" + (f", %z: {ty}" if triangular else "") + "):",
              f"  %more = arith.cmpi ult, %t, %n : {ty}",
              "  cf.cond_br %more, ^body, ^exit", "^body:"]
    # Emit Horner arithmetic; the oracle below uses a separate power sum.
    value = f"%c{len(coefficients)-1}"
    for k in reversed(range(len(coefficients)-1)):
        lines += [f"  %m{k} = arith.muli {value}, %i : {ty}",
                  f"  %p{k} = arith.addi %m{k}, %c{k} : {ty}"]
        value = f"%p{k}"
    lines += [f"  %sn = arith.addi %s, {value} : {ty}",
              f"  %in = arith.addi %i, %step : {ty}",
              f"  %tn = arith.addi %t, %one : {ty}"]
    if triangular:
        lines.append(f"  %zn = arith.addi %z, {'%sn' if next_sum else '%s'} : {ty}")
    updates = ["%tn", "%in", "%sn"] + (["%zn"] if triangular else [])
    lines += [f"  cf.br ^header({', '.join(updates)} : {types})", "^exit:",
              f"  return {'%z' if triangular else '%s'} : {ty}", "}"]
    return "\n".join(lines)


def check_case(case, analysis):
    name, width, coefficients, start, step, initial, triangular, next_sum = case
    check(analysis["status"] == "PROVEN", f"{name}: {analysis}", "accepted_functions")
    states = analysis["states"]
    direct = [0, start, initial] + ([-9] if triangular else [])
    check(len(states) == len(direct), name + " state count")
    forms = [[Fraction(x) for x in s["binomial_coefficients"]] for s in states]
    expected_dependencies = [[], [], [1]] + ([[1, 2] if next_sum and len(coefficients) > 1 else [2]] if triangular else [])
    # Constant polynomials do not reference i; Horner expressions do, even if
    # their coefficients later cancel. Dependencies are structural.
    if len(coefficients) == 1:
        expected_dependencies[2] = []
    for j, state in enumerate(states):
        check(Fraction(state["initial"]) == direct[j], name + " initial")
        check(state["dependencies"] == expected_dependencies[j], name + " dependencies")
    check(states[0]["increment"] == "1/1", name + " count increment")
    check(states[1]["increment"] == f"{step}/1", name + " induction increment")
    modulus = 1 << width
    bit_state = [x % modulus for x in direct]
    for t in range(81):
        for j, form in enumerate(forms):
            symbolic = evaluate(form, t)
            check(symbolic == direct[j], f"{name} state{j} t{t}: exact lift", "exact_values")
            check(symbolic.denominator == 1 and int(symbolic) % modulus == bit_state[j],
                  f"{name} state{j} t{t}: residue", "modular_values")
        # Independent simultaneous execution with arbitrary-precision ints.
        # Both integer lifts and wrapping source arithmetic are checked.
        increment = sum(c * direct[1] ** k for k, c in enumerate(coefficients))
        bit_increment = sum((c % modulus) * bit_state[1] ** k for k, c in enumerate(coefficients)) % modulus
        next_direct = [direct[0]+1, direct[1]+step, direct[2]+increment]
        next_bits = [(bit_state[0]+1) % modulus, (bit_state[1]+step) % modulus,
                     (bit_state[2]+bit_increment) % modulus]
        if triangular:
            next_direct.append(direct[3] + (next_direct[2] if next_sum else direct[2]))
            next_bits.append((bit_state[3] + (next_bits[2] if next_sum else bit_state[2])) % modulus)
        for j, form in enumerate(forms):
            check(evaluate(form, t+1) - evaluate(form, t) == next_direct[j] - direct[j],
                  f"{name} finite difference at {t}", "numeric_differences")
        direct, bit_state = next_direct, next_bits


def resource_loop(name, operations, update="%sum", width=32):
    return f"""func.func @{name}(%n: i{width}) -> i{width} {{
  %zero = arith.constant 0 : i{width}
  %one = arith.constant 1 : i{width}
  cf.br ^header(%zero, %zero : i{width}, i{width})
^header(%i: i{width}, %s: i{width}):
  %more = arith.cmpi ult, %i, %n : i{width}
  cf.cond_br %more, ^body, ^exit
^body:
{operations}
  %sum = arith.addi %s, %i : i{width}
  %next = arith.addi %i, %one : i{width}
  cf.br ^header(%next, {update} : i{width}, i{width})
^exit:
  return %s : i{width}
}}"""


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--driver", required=True)
    parser.add_argument("--mlir-opt", required=True)
    args = parser.parse_args()
    rng = random.Random(0x5031)
    cases = [(f"power{k}", 128, [0]*k+[1], 1, 1, 0, False, False) for k in range(1, 11)]
    for j in range(80):
        width = rng.choice([8, 16, 32, 128, 256])
        degree = rng.randrange(9)
        coefficients = [rng.randrange(-100, 101) for _ in range(degree+1)]
        cases.append((f"random{j}", width, coefficients, rng.randrange(-70, 71),
                      rng.randrange(-7, 8), rng.randrange(-100, 101), j % 2 == 0, j % 4 == 0))
    for j in range(8):
        cases.append((f"large{j}", 256, [(-1)**k * ((1 << (180+k))+k) for k in range(j+3)],
                      -17, 3, (1 << 150)+19, True, j % 2 == 0))
    with tempfile.TemporaryDirectory(prefix="polynomial-p1-") as directory:
        path = Path(directory) / "generated.mlir"
        source = "module {\n" + "\n".join(emit(case) for case in cases) + "\n}\n"
        path.write_text(source)
        run([args.mlir_opt, str(path), "--verify-each", "-o", "/dev/null"])
        first = run([args.driver, str(path)])
        second = run([args.driver, str(path)])
        check(first == second, "cross-process output determinism")
        data = json.loads(first)
        check(data["read_only"] and data["deterministic"], "read-only generated analysis")
        reports = {f["function"]: f["analysis"] for f in data["functions"]}
        check(len(reports) == len(cases), "no missing generated functions")
        for case in cases:
            check_case(case, reports[case[0]])
        check(path.read_text() == source, "input file bytes unchanged")

        # Shared SSA expressions must not expand exponentially in the report.
        operations, previous = [], "%one"
        for j in range(22):
            operations.append(f"  %v{j} = arith.muli {previous}, {previous} : i32")
            previous = f"%v{j}"
        operations.append(f"  %bad = arith.addi %s, {previous} : i32")
        resources = [resource_loop("exponential_report", "\n".join(operations), "%bad")]
        operations, previous = [], "%i"
        for j in range(7):
            operations.append(f"  %v{j} = arith.muli {previous}, {previous} : i32")
            previous = f"%v{j}"
        operations.append(f"  %bad = arith.addi %s, {previous} : i32")
        resources.append(resource_loop("excess_degree", "\n".join(operations), "%bad"))
        operations, previous = [], "%i"
        for j in range(140):
            operations.append(f"  %v{j} = arith.addi {previous}, %one : i32")
            previous = f"%v{j}"
        operations.append(f"  %bad = arith.addi %s, {previous} : i32")
        resources.append(resource_loop("excess_depth", "\n".join(operations), "%bad"))
        resources.append(resource_loop("excess_width", "", width=4097))
        resources.append(resource_loop("excess_operations", "\n".join(
            f"  %c{j} = arith.constant 0 : i32" for j in range(4100))))
        resources.append(resource_loop("unknown_effects", '  "opaque.effect"() : () -> ()'))
        resource_path = Path(directory) / "resources.mlir"
        resource_path.write_text("module {\n" + "\n".join(resources) + "\n}\n")
        run([args.mlir_opt, str(resource_path), "--allow-unregistered-dialect", "--verify-each", "-o", "/dev/null"])
        for name, analysis in report(args.driver, resource_path).items():
            check(analysis["status"] == "UNKNOWN" and not analysis["states"] and analysis["reason"],
                  name + " must fail closed", "resource_rejections")

        # Lower a separate copy using MLIR's existing pass, then analyze actual
        # resulting CFG. This is not a transformation performed by P1.
        fixture = Path(__file__).parent / "Inputs" / "structured.mlir"
        lowered = Path(directory) / "lowered.mlir"
        run([args.mlir_opt, str(fixture), "--convert-scf-to-cf", "--verify-each", "-o", str(lowered)])
        lowered_result = report(args.driver, lowered)["structured"]
        check(lowered_result["status"] == "PROVEN", "externally lowered while")
        form = [Fraction(x) for x in lowered_result["states"][1]["binomial_coefficients"]]
        total = 0
        for t in range(201):
            check(evaluate(form, t) == total, "lowered while direct sum", "lowered_values")
            total += t*t
    print("PASS: P1 independent Python int oracle; seed=0x5031")
    print(", ".join(f"{k}={v}" for k, v in sorted(counts.items())))


if __name__ == "__main__":
    main()
