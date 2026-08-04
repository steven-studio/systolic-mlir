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


def reference_matmul_f32(A, B):
    """硬體語意：fp32 輸入、fp32 循序累加。與 fpga_matmul_tiled 應為 bit-exact。"""
    A = A.astype(np.float32); B = B.astype(np.float32)
    M, K = A.shape; N = B.shape[1]
    C = np.zeros((M, N), dtype=np.float32)
    for k in range(K):
        C = (C + A[:, k:k+1] * B[k:k+1, :]).astype(np.float32)
    return C


def reference_conv2d_f32(X, Kern, N,H,W,Cin,Kh,Kw,Cout,
                         sH,sW,dH,dW,pT,pB,pL,pR):
    """順序與 fpga_conv2d_im2col_padded_auto 一致。"""
    effKh = dH*(Kh-1)+1; effKw = dW*(Kw-1)+1
    Hout = (H+pT+pB-effKh)//sH + 1
    Wout = (W+pL+pR-effKw)//sW + 1
    Kdim = Kh*Kw*Cin
    Kmat = Kern.reshape(Kdim, Cout)          # HWCF 展平即為 (Kdim, Cout)
    Y = np.zeros((N, Hout, Wout, Cout), dtype=np.float32)
    Xn_all = X.reshape(N, H, W, Cin)
    for n in range(N):
        Xcol = np.zeros((Hout*Wout, Kdim), dtype=np.float32)
        for oy in range(Hout):
            for ox in range(Wout):
                row = oy*Wout + ox
                for ky in range(Kh):
                    for kx in range(Kw):
                        iy = oy*sH + ky*dH - pT
                        ix = ox*sW + kx*dW - pL
                        col = (ky*Kw + kx)*Cin
                        if 0 <= iy < H and 0 <= ix < W:
                            Xcol[row, col:col+Cin] = Xn_all[n, iy, ix, :]
        Y[n] = reference_matmul_f32(Xcol, Kmat).reshape(Hout, Wout, Cout)
    return Y
