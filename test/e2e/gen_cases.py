#!/usr/bin/env python3
"""gen_cases.py -- the E2E regression inputs, in systolic-matmul's stdin format.

    python3 gen_cases.py OUT_DIR                  writes OUT_DIR/<case>.txt,
                                                  prints names
    python3 gen_cases.py OUT_DIR --kmax K8 K4     only the cases that put K at
                                                  the edges of K_MAX_8X8 = K8
                                                  and K_MAX_4X4 = K4

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
with the mixed value class only.
"""

import os
import random
import sys

# (M, N, K): the six required shapes, then cases that need more than one
# invocation per tile (K > k_max at the archived bitstream's k_max = 16) or
# more than one round per device.
SHAPES = [
    (1, 1, 1),
    (2, 3, 4),
    (4, 4, 4),
    (8, 8, 8),
    (8, 4, 16),
    (16, 8, 4),
    (16, 8, 40),    # K folded 16 + 16 + 8, every device busy
    (17, 9, 33),    # edge tiles in M and N, K folded 16 + 16 + 1
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


def write_case(out, m, n, k, cls, draw):
    name = f"{m}x{n}x{k}_{cls}"
    rng = random.Random(f"{name}-20261011")
    lines = [f"{m} {n} {k}"]
    lines += [" ".join(str(draw(rng)) for _ in range(k)) for _ in range(m)]
    lines += [" ".join(str(draw(rng)) for _ in range(n)) for _ in range(k)]
    with open(os.path.join(out, name + ".txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print(name)


def main():
    args = sys.argv[1:]
    if len(args) == 1:
        out = args[0]
        os.makedirs(out, exist_ok=True)
        for (m, n, k) in SHAPES:
            for cls, draw in CLASSES.items():
                write_case(out, m, n, k, cls, draw)
        return 0
    if len(args) == 4 and args[1] == "--kmax":
        out = args[0]
        try:
            k8, k4 = int(args[2]), int(args[3])
        except ValueError:
            print(__doc__)
            return 2
        if k8 < 1 or k4 < 1:
            print("K8 and K4 must be positive integers", file=sys.stderr)
            return 2
        os.makedirs(out, exist_ok=True)
        seen = set()
        for (m, n, k) in kmax_shapes(k8, k4):
            if (m, n, k) in seen:
                continue
            seen.add((m, n, k))
            write_case(out, m, n, k, "mixed", CLASSES["mixed"])
        return 0
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
