#!/usr/bin/env python3
"""gen_cases.py -- the E2E regression inputs, in systolic-matmul's stdin format.

    python3 gen_cases.py OUT_DIR        writes OUT_DIR/<case>.txt, prints names

Every shape the regression runs, with every value class:

    zero      all of A and B are 0
    positive  integers 1..9
    negative  integers -9..-1
    mixed     integers -9..9, zeros included

Values are drawn from a fixed-seed generator per case, so a failure
reproduces with the same file. All of them stay exact in binary32
(K * 9 * 9 < 2^24 for every K here), so the FPGA must match the integer
reference element for element.
"""

import os
import random
import sys

# (M, N, K): the six required shapes, then cases that need more than one
# invocation per tile (K > k_max = 16) or more than one round per device.
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


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    for (m, n, k) in SHAPES:
        for cls, draw in CLASSES.items():
            name = f"{m}x{n}x{k}_{cls}"
            rng = random.Random(f"{name}-20261011")
            lines = [f"{m} {n} {k}"]
            lines += [" ".join(str(draw(rng)) for _ in range(k)) for _ in range(m)]
            lines += [" ".join(str(draw(rng)) for _ in range(n)) for _ in range(k)]
            with open(os.path.join(out, name + ".txt"), "w") as f:
                f.write("\n".join(lines) + "\n")
            print(name)
    return 0


if __name__ == "__main__":
    sys.exit(main())
