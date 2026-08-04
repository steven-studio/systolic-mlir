import numpy as np, reference as R

def ulp(a, b):
    ai = a.astype(np.float32).view(np.int32).astype(np.int64)
    bi = b.astype(np.float32).view(np.int32).astype(np.int64)
    ai = np.where(ai < 0, np.int64(0x80000000) - ai, ai)
    bi = np.where(bi < 0, np.int64(0x80000000) - bi, bi)
    return np.abs(ai - bi)

# name: (N,H,W,Cin,Kh,Kw,Cout, sH,sW, dH,dW, pT,pB,pL,pR)
cfgs = {
  "004": (2,10,10,8,3,3,16, 1,1, 1,1, 1,1,1,1),
  "029": (1, 8, 8,8,5,5,16, 1,2, 1,1, 1,1,1,1),
  "032": (1,10,10,8,3,5,16, 1,1, 1,1, 0,0,0,0),
  "042": (2,10,10,8,3,3,16, 1,2, 1,1, 0,0,0,0),
}

for tag, p in cfgs.items():
    X = np.fromfile(f"X_conv_sweep_{tag}.bin", dtype=np.float32)
    K = np.fromfile(f"K_conv_sweep_{tag}.bin", dtype=np.float32)
    Yhw = np.fromfile(f"Y_conv_sweep_{tag}.bin", dtype=np.float32)
    ref = R.reference_conv2d_f32(X, K, *p).ravel()
    if ref.size != Yhw.size:
        print(f"{tag}: 大小不符 ref={ref.size} hw={Yhw.size}"); continue
    u = ulp(Yhw, ref)
    nz = int((u != 0).sum())
    print(f"{tag}: max_ulp={u.max():>8}  非零元素={nz}/{u.size}"
          f"  {'BIT-EXACT' if nz == 0 else ''}")
