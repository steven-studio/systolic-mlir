#!/usr/bin/env python3

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from dpti_job import (
    DPTIJob,
    REG_CONTROL,
    REG_JOB_ID,
    REG_DEVICE_ID,
    REG_M,
    REG_N,
    REG_K,
    REG_START_CYCLE,
    REG_EST_CYCLES,
    REG_A_BASE_LO,
    REG_A_BASE_HI,
    REG_B_BASE_LO,
    REG_B_BASE_HI,
    REG_C_BASE_LO,
    REG_C_BASE_HI,
)


job = DPTIJob(
    job_id=1,
    device_id=0,

    m=8,
    n=8,
    k=16,

    start_cycle=100,
    est_cycles=125,

    a_base=0x0000000000001000,
    b_base=0x0000000000002000,
    c_base=0x0000000000003000,
)

writes = job.register_writes()

expected = [
    (REG_JOB_ID,      0x00000001),
    (REG_DEVICE_ID,   0x00000000),
    (REG_M,           0x00000008),
    (REG_N,           0x00000008),
    (REG_K,           0x00000010),
    (REG_START_CYCLE, 0x00000064),
    (REG_EST_CYCLES,  0x0000007d),
    (REG_A_BASE_LO,   0x00001000),
    (REG_A_BASE_HI,   0x00000000),
    (REG_B_BASE_LO,   0x00002000),
    (REG_B_BASE_HI,   0x00000000),
    (REG_C_BASE_LO,   0x00003000),
    (REG_C_BASE_HI,   0x00000000),
    (REG_CONTROL,     0x00000001),
]

assert writes == expected, (
    "DPTI register ABI mismatch:\n"
    f"expected={expected}\n"
    f"actual={writes}"
)

print("=" * 64)
print("PASS: compiler -> DPTI register ABI")
print("=" * 64)

for addr, data in writes:
    print(f"WRITE32 0x{addr:02x} 0x{data:08x}")

print()
print("PASS: submit register is last")
assert writes[-1] == (REG_CONTROL, 1)
