#!/usr/bin/env python3
"""
DPTI register-write emitter.

This layer intentionally does not assume a particular physical host
transport. It converts a DPTIJob into the exact register writes required
by axi4lite_dpti_bridge.sv.

A real AXI4-Lite/MMIO backend can consume these writes later.
"""

from dpti_job import DPTIJob


def emit_register_writes(job: DPTIJob):
    return job.register_writes()


def print_register_writes(job: DPTIJob):
    for addr, data in emit_register_writes(job):
        print(f"WRITE32 0x{addr:02x} 0x{data:08x}")


def submit_register_writes(job: DPTIJob, write32):
    """
    Submit one compiler-generated job through a supplied write32 backend.

    write32(addr, data) is intentionally injected so this file does not
    assume /dev/mem, PCIe, AXI UART, Xilinx XSDB, etc.
    """
    for addr, data in emit_register_writes(job):
        write32(addr, data)
