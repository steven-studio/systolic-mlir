#!/usr/bin/env python3
"""gen_cases.py -- the E2E regression inputs, in systolic-matmul's stdin format.

    python3 gen_cases.py OUT_DIR                  the fixed regression: writes
                                                  OUT_DIR/<case>.txt, prints
                                                  names
    python3 gen_cases.py OUT_DIR --kmax K8 K4     only the cases that put K at
                                                  the edges of K_MAX_8X8 = K8
                                                  and K_MAX_4X4 = K4
    python3 gen_cases.py OUT_DIR --kmax K8 K4 K,K,...
                                                  the same, plus 12 x 12 x K
                                                  for each K listed
    python3 gen_cases.py OUT_DIR --full K8 K4 [K,K,...]
                                                  the fixed regression, then
                                                  the --kmax cases
    python3 gen_cases.py OUT_DIR --quick K8 K4    the six required shapes, then
                                                  8 x 8 x (K8 + 1) and
                                                  4 x 4 x (K4 + 1)

K8 and K4 are the bitstream's two capacities, as the runtime resolved them
(run_e2e.sh passes what runtime-dispatch-test --print-config printed); they
are independent and neither has a default here. With --max-k N added at the
end of any form, a case with K > N (systolic-matmul's own input limit) is not
written; it is listed in OUT_DIR/not_run.txt as "NAME K > N", so the caller
can report it as not run instead of losing it.

Every shape the regression runs, with every value class:

    zero      all of A and B are 0
    positive  integers 1..9
    negative  integers -9..-1
    mixed     integers -9..9, zeros included

Values are drawn from a fixed-seed generator per case, so a failure
reproduces with the same file. All of them stay exact in binary32
(K * 9 * 9 < 2^24 for every K here), so the FPGA must match the integer
reference element for element.

--kmax builds the cases from the two capacities of the bitstream instead of
the fixed list: for every K_MAX k of the fleet, K = k - 1, k, k + 1 (K below,
at and just past one invocation, the last not divisible by k) and one deep K,
2 * max(K8, K4) + 3, that folds into several invocations on every device and
is divisible by neither; K = 1 always. The 12 x 12 output puts one tile on
the 8x8 and the rest on the 4x4s, so every K is folded by both capacities at
once and the two geometries are scheduled side by side; 4 x 4 x K4 and
8 x 8 x K8 are single tiles at exactly each device's own capacity. These run
with the mixed value class only. Extra K values (a comma-separated list
after K8 K4) add 12 x 12 x K cases, e.g. the depths of a K-folding check.

--quick is the short run: the six required shapes with every value class
(both geometries, edge tiles, at most 16 deep), and one case one step past
each capacity, 8 x 8 x (K8 + 1) and 4 x 4 x (K4 + 1), mixed values: whichever
capacity is the larger, its case is deeper than every device holds, so at
least one of the two is folded into two chained invocations (the second one
accumulating onto a C that is no longer zero) on any configuration.
"""

import os
import random
import sys

# (M, N, K): the six required shapes, then three that need more than one
# round per device. The shapes are fixed; how K folds is the configuration's:
# at the archived bitstream's 16 / 16, 16x8x40 folds 16 + 16 + 8 and 17x9x33
# 16 + 16 + 1, while at K_MAX 512 nothing here folds at all -- the cases that
# follow the capacities are kmax_shapes() and the --quick additions.
REQUIRED = [
    (1, 1, 1),
    (2, 3, 4),
    (4, 4, 4),
    (8, 8, 8),
    (8, 4, 16),
    (16, 8, 4),
]
SHAPES = REQUIRED + [
    (16, 8, 40),    # every device busy
    (17, 9, 33),    # edge tiles in M and N
    (32, 32, 32),   # several rounds on every device
]

CLASSES = {
    "zero": lambda rng: 0,
    "positive": lambda rng: rng.randint(1, 9),
    "negative": lambda rng: rng.randint(-9, -1),
    "mixed": lambda rng: rng.randint(-9, 9),
}


def kmax_shapes(k8, k4):
    ks = {1, 2 * max(k8, k4) + 3}
    for k in (k8, k4):
        ks.update(x for x in (k - 1, k, k + 1) if x >= 1)
    shapes = [(12, 12, k) for k in sorted(ks)]
    shapes += [(4, 4, k4), (8, 8, k8)]
    return shapes


def quick_shapes(k8, k4):
    return [(8, 8, k8 + 1), (4, 4, k4 + 1)]


def case_name(m, n, k, cls):
    return f"{m}x{n}x{k}_{cls}"


def write_case(out, m, n, k, cls, draw):
    name = case_name(m, n, k, cls)
    rng = random.Random(f"{name}-20261011")
    lines = [f"{m} {n} {k}"]
    lines += [" ".join(str(draw(rng)) for _ in range(k)) for _ in range(m)]
    lines += [" ".join(str(draw(rng)) for _ in range(n)) for _ in range(k)]
    with open(os.path.join(out, name + ".txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print(name)


def positive(text):
    """A decimal integer >= 1, or None."""
    return int(text) if text.isdigit() and int(text) >= 1 else None


def main():
    args = sys.argv[1:]
    max_k = None
    if len(args) >= 3 and args[-2] == "--max-k":
        max_k = positive(args[-1])
        if max_k is None:
            print("--max-k takes a positive integer", file=sys.stderr)
            return 2
        args = args[:-2]
    if not args or args[0].startswith("-"):
        print(__doc__)
        return 2
    out, mode = args[0], args[1] if len(args) > 1 else None

    # (M, N, K, value classes) in order, each once.
    cases = []
    if mode is None and len(args) == 1:
        cases = [(m, n, k, list(CLASSES)) for (m, n, k) in SHAPES]
    elif mode in ("--kmax", "--full", "--quick") and len(args) in (4, 5):
        k8, k4 = positive(args[2]), positive(args[3])
        extra = []
        if len(args) == 5:
            extra = [positive(x) for x in args[4].split(",")]
        if mode == "--quick" and len(args) == 5:
            print(__doc__)
            return 2
        if k8 is None or k4 is None or None in extra:
            print("K8, K4 and every extra K must be positive integers", file=sys.stderr)
            return 2
        boundary = [(m, n, k, ["mixed"])
                    for (m, n, k) in kmax_shapes(k8, k4) + [(12, 12, k) for k in extra]]
        if mode == "--kmax":
            cases = boundary
        elif mode == "--full":
            cases = [(m, n, k, list(CLASSES)) for (m, n, k) in SHAPES] + boundary
        else:
            cases = [(m, n, k, list(CLASSES)) for (m, n, k) in REQUIRED]
            cases += [(m, n, k, ["mixed"]) for (m, n, k) in quick_shapes(k8, k4)]
    else:
        print(__doc__)
        return 2

    os.makedirs(out, exist_ok=True)
    seen = set()
    not_run = []
    for (m, n, k, classes) in cases:
        for cls in classes:
            name = case_name(m, n, k, cls)
            if name in seen:
                continue
            seen.add(name)
            if max_k is not None and k > max_k:
                not_run.append(f"{name} K > {max_k}")
                continue
            write_case(out, m, n, k, cls, CLASSES[cls])
    if max_k is not None:
        with open(os.path.join(out, "not_run.txt"), "w") as f:
            f.write("".join(line + "\n" for line in not_run))
    return 0


if __name__ == "__main__":
    sys.exit(main())
