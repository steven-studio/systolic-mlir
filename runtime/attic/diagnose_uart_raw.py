"""
Raw UART diagnostic for the Arty A7 matmul core, bypassing run_shape and
fpga_matmul4x4.c entirely. Sends a fixed, known 192-byte test pattern
(three 4x4 float32 matrices: A=all 1.0, B=identity-ish, C_init=all 0.0)
directly over the serial port with a long timeout, and reports exactly
what -- if anything -- comes back.

This distinguishes two very different failure modes that both surface as
rc=-2 in the existing C code:
  1. TRUE SILENCE: zero bytes come back even after 15s. Consistent with
     the FSM being stuck in a non-S_RX state, or the core's clock not
     locked (Section 4.2's MMCME2_BASE-divided clock -- if the lock
     signal isn't gating ap_start correctly, a post-power-glitch
     relock could leave ap_done never asserting).
  2. PARTIAL / GARBLED RESPONSE: some bytes come back but not 64, or
     they arrive but look wrong. This points to a baud-rate or framing
     issue (e.g. the clock the UART baud generator itself depends on
     not being stable), not a fully stuck FSM -- a different fix.

Usage:
    pip install pyserial   # if not already installed
    python3 diagnose_uart_raw.py
"""
import serial
import struct
import time
import random

PORT = "/dev/ttyUSB1"
BAUD = 115200

def main():
    print(f"Opening {PORT} at {BAUD} baud...")
    ser = serial.Serial(PORT, BAUD, timeout=15, bytesize=8,
                         parity='N', stopbits=1)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Random values in the same range as reference.py's make_test_case
    # (-10, 10), NOT a uniform constant pattern -- rules out the
    # possibility that an all-identical-bytes payload is hitting some
    # edge case the design was never actually exercised against, since
    # every prior evaluation (Table 1, this paper's own methodology)
    # always used random, non-uniform test data.
    random.seed(1234)
    A = [random.uniform(-10, 10) for _ in range(16)]
    B = [random.uniform(-10, 10) for _ in range(16)]
    C_init = [0.0] * 16
    payload = struct.pack('<16f', *A) + struct.pack('<16f', *B) + struct.pack('<16f', *C_init)
    assert len(payload) == 192, f"expected 192 bytes, got {len(payload)}"

    # Reference: compute expected C = A @ B + C_init in double precision,
    # treating A, B as 4x4 row-major matrices, to compare against
    # whatever comes back.
    import numpy as np
    A_np = np.array(A, dtype=np.float64).reshape(4, 4)
    B_np = np.array(B, dtype=np.float64).reshape(4, 4)
    C_ref = (A_np @ B_np).flatten()
    print(f"Expected reference result (float64): {C_ref}")

    print(f"Writing {len(payload)} bytes...")
    t0 = time.time()
    n_written = ser.write(payload)
    ser.flush()
    print(f"Wrote {n_written} bytes in {time.time()-t0:.3f}s")

    print("Reading response (up to 15s timeout)...")
    t0 = time.time()
    response = ser.read(64)
    elapsed = time.time() - t0

    print(f"\nGot {len(response)} bytes back after {elapsed:.3f}s")
    if len(response) == 0:
        print("-> TRUE SILENCE. No bytes came back at all. This points to")
        print("   the FSM being stuck (not in S_RX) or the core's clock")
        print("   not locked/running -- not a baud-rate or framing issue.")
    elif len(response) < 64:
        print(f"-> PARTIAL response: only {len(response)}/64 bytes arrived.")
        print(f"   Raw bytes: {response.hex()}")
        print("   This suggests the link is alive but something is cutting")
        print("   the transmission short -- worth checking framing/baud.")
    else:
        values = struct.unpack('<16f', response)
        print(f"-> FULL 64-byte response received in {elapsed*1000:.3f} ms.")
        if elapsed < 0.005:
            print("   NOTE: under 5ms is faster than 64 bytes can physically")
            print("   transmit at 115200 baud (~5.5ms minimum) -- if so, this")
            print("   response did NOT come from a genuine round trip; it is")
            print("   likely stale/buffered data, not a live computation.")
        print(f"   Decoded as 16 floats: {values}")
        print(f"   Reference (float64):  {list(C_ref)}")

    ser.close()


if __name__ == "__main__":
    main()
