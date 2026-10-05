#!/usr/bin/env python3
"""
DPTI compiler -> FPGA job descriptor.

This file defines the software-side ABI corresponding to:

    axi4lite_dpti_bridge.sv
        ->
    dpti_descriptor_bridge.sv

The compiler/runtime must populate this descriptor before submit.
"""

from dataclasses import dataclass


REG_CONTROL     = 0x00
REG_JOB_ID      = 0x04
REG_DEVICE_ID   = 0x08
REG_M           = 0x0C
REG_N           = 0x10
REG_K           = 0x14
REG_START_CYCLE = 0x18
REG_EST_CYCLES  = 0x1C
REG_A_BASE_LO   = 0x20
REG_A_BASE_HI   = 0x24
REG_B_BASE_LO   = 0x28
REG_B_BASE_HI   = 0x2C
REG_C_BASE_LO   = 0x30
REG_C_BASE_HI   = 0x34


@dataclass(frozen=True)
class DPTIJob:
    job_id: int
    device_id: int

    m: int
    n: int
    k: int

    start_cycle: int
    est_cycles: int

    a_base: int
    b_base: int
    c_base: int

    def validate(self):
        fields = {
            "job_id": self.job_id,
            "device_id": self.device_id,
            "m": self.m,
            "n": self.n,
            "k": self.k,
            "start_cycle": self.start_cycle,
            "est_cycles": self.est_cycles,
            "a_base": self.a_base,
            "b_base": self.b_base,
            "c_base": self.c_base,
        }

        for name, value in fields.items():
            if not isinstance(value, int):
                raise TypeError(f"{name} must be int")

        if self.job_id < 0 or self.job_id > 0xffffffff:
            raise ValueError("job_id out of uint32 range")

        if self.device_id < 0 or self.device_id > 0xffffffff:
            raise ValueError("device_id out of uint32 range")

        for name in ("m", "n", "k", "start_cycle", "est_cycles"):
            value = getattr(self, name)
            if value < 0 or value > 0xffffffff:
                raise ValueError(f"{name} out of uint32 range")

        for name in ("a_base", "b_base", "c_base"):
            value = getattr(self, name)
            if value < 0 or value > 0xffffffffffffffff:
                raise ValueError(f"{name} out of uint64 range")

        if self.m == 0 or self.n == 0 or self.k == 0:
            raise ValueError("M/N/K must be non-zero")

        if self.est_cycles == 0:
            raise ValueError("est_cycles must be non-zero")

    @staticmethod
    def _lo(value: int) -> int:
        return value & 0xffffffff

    @staticmethod
    def _hi(value: int) -> int:
        return (value >> 32) & 0xffffffff

    def register_writes(self):
        """
        Return the exact AXI4-Lite register sequence expected by the RTL.

        Submit is deliberately last.
        """
        self.validate()

        return [
            (REG_JOB_ID,      self.job_id),
            (REG_DEVICE_ID,   self.device_id),

            (REG_M,           self.m),
            (REG_N,           self.n),
            (REG_K,           self.k),

            (REG_START_CYCLE, self.start_cycle),
            (REG_EST_CYCLES,  self.est_cycles),

            (REG_A_BASE_LO,   self._lo(self.a_base)),
            (REG_A_BASE_HI,   self._hi(self.a_base)),

            (REG_B_BASE_LO,   self._lo(self.b_base)),
            (REG_B_BASE_HI,   self._hi(self.b_base)),

            (REG_C_BASE_LO,   self._lo(self.c_base)),
            (REG_C_BASE_HI,   self._hi(self.c_base)),

            # CONTROL[0] = submit.
            (REG_CONTROL,     1),
        ]

    def to_dict(self):
        self.validate()

        return {
            "job_id": self.job_id,
            "device_id": self.device_id,
            "m": self.m,
            "n": self.n,
            "k": self.k,
            "start_cycle": self.start_cycle,
            "est_cycles": self.est_cycles,
            "a_base": self.a_base,
            "b_base": self.b_base,
            "c_base": self.c_base,
        }
