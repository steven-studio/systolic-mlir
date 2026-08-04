import struct
import numpy as np


def _monotonic_ordinal(f32: np.float32) -> int:
    """Map an IEEE-754 float32 to a monotonically increasing signed integer,
    so that ordinal(b) - ordinal(a) counts ULPs between a and b."""
    i = struct.unpack('<i', struct.pack('<f', float(f32)))[0]
    if i < 0:
        i = 0x80000000 - i
    return i


def ulp_distance(a, b) -> int:
    """a, b: values representable as float32."""
    return abs(_monotonic_ordinal(np.float32(a)) - _monotonic_ordinal(np.float32(b)))


def max_ulp_error(hw_result: np.ndarray, sw_reference_f64: np.ndarray) -> int:
    """hw_result: float32 array from FPGA. sw_reference_f64: float64 reference.
    Reference is rounded to float32 first (that rounding is the expected ~0.5 ULP)."""
    ref_f32 = sw_reference_f64.astype(np.float32)
    diffs = [ulp_distance(h, r) for h, r in zip(hw_result.flatten(), ref_f32.flatten())]
    return max(diffs)
