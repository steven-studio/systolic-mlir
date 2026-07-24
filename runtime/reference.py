import numpy as np


def reference_matmul(A: np.ndarray, B: np.ndarray) -> np.ndarray:
    """A: (M,K) float64, B: (K,N) float64 -> (M,N) float64.

    Rounds A and B to float32 first (matching what actually gets sent
    to hardware -- every caller writes A.astype(np.float32) to the
    wire), then upcasts back to float64 before the dot product. This
    matters: comparing the hardware's output (computed from
    float32-rounded inputs) against a reference computed from the
    RAW, un-rounded float64 inputs bakes in a spurious mismatch that
    has nothing to do with the accelerator's own precision -- it is
    simply the reference and the hardware not agreeing on what the
    input values even were. Rounding first removes that mismatch,
    isolating whatever error the hardware/tiling pipeline itself
    actually introduces.
    """
    assert A.dtype == np.float64 and B.dtype == np.float64
    A32 = A.astype(np.float32).astype(np.float64)
    B32 = B.astype(np.float32).astype(np.float64)
    return A32 @ B32


def make_test_case(M, K, N, seed):
    rng = np.random.default_rng(seed)
    A = rng.uniform(-10, 10, size=(M, K)).astype(np.float64)
    B = rng.uniform(-10, 10, size=(K, N)).astype(np.float64)
    return A, B