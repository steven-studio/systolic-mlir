import numpy as np


def reference_matmul(A: np.ndarray, B: np.ndarray) -> np.ndarray:
    """A: (M,K) float64, B: (K,N) float64 -> (M,N) float64"""
    assert A.dtype == np.float64 and B.dtype == np.float64
    return A @ B


def make_test_case(M, K, N, seed):
    rng = np.random.default_rng(seed)
    A = rng.uniform(-10, 10, size=(M, K)).astype(np.float64)
    B = rng.uniform(-10, 10, size=(K, N)).astype(np.float64)
    return A, B
