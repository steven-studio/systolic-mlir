#!/usr/bin/env python3
"""e2e_verdict.py -- the verdict on one run_e2e.sh case, from what it left.

    e2e_verdict.py CASE_DIR --exit N --k8 K8 --k4 K4 --slab BYTES --source PATH
                   [--mode sync|async] [--header 0|1]
                   [--cosim RECEIVED CONSUMED SENT SESSION | --cosim-error TEXT]

CASE_DIR is <out>/<case>, named MxNxK_<values> (gen_cases.py), holding
output.txt (systolic-matmul's stdout and stderr) and ir/10-systolic.mlir;
--exit is systolic-matmul's exit status. K8, K4 and BYTES are the
configuration as the harness resolved it (runtime-dispatch-test
--print-config) and PATH the file it came from. --cosim gives the case's
co-simulation session: the bytes the simulated FT2232H received from the
host, the bytes the RTL consumed (RD#) and sent (WR#); --cosim-error says why
there is no such session. --mode names the runtime the case was meant to run
on (ir/20-fpga.mlir must call that runtime: one
fpga_matmul_tiled_auto_scheduled per tile, or one fpga_tile_submit per tile
between fpga_gemm_begin and fpga_gemm_wait); --header 1 says the bitstream
sends the 16-byte completion record ahead of every result tile, so the bytes
back are counted with it.

Prints one line, tab-separated:

    CLASS  RT_RC  TILES  INVOCATIONS  BYTES_TO  BYTES_BACK  COVERAGE  NOTE

CLASS is one of
    PASS        the FPGA result equals the CPU reference, every tile was one
                FPGA invocation, every invocation carried the configured
                slab, the bytes back are the result tiles (with the
                completion record when --header 1), the lowered IR calls the
                runtime of --mode, and (co-simulation) the RTL consumed and
                sent exactly the bytes the runtime counted
    MISMATCH    the FPGA result differs from the CPU reference (exit 1)
    CONFIG      the configuration: refused by systolic-matmul (exit 4,
                SYSTOLIC_FPGA_CONFIG=...), runtime rc -16, or it did not
                reach a layer intact -- systolic-matmul's runtime read other
                K_MAX values or another file, the compiler folded K by other
                k_max values, a tile is deeper than its device holds, K was
                not folded by the device's K_MAX, or an invocation carried a
                slab of another length
    TRANSPORT   the link failed: runtime rc -3, -4, -6, -7, -8 (or -14, a
                call refused after such a failure); on the co-simulation
                also the RTL moving other bytes than the runtime counted
    SUBMISSION  the runtime refused or could not submit an invocation:
                rc -5, -10, -11, -12, -13, -15 (asynchronous runtime also
                -17, -18, -19)
    INPUT       systolic-matmul refused the input (exit 2)
    COMPILE     the compiler failed (exit 3)
    FAIL        anything else (no verdict line, tiles != invocations, ...)
RT_RC is the runtime's raw return code (its first error), 0 when every call
returned 0, "-" when the run did not get to the runtime. COVERAGE is
d0..d3=<tiles per device>,edge=<edge tiles>,fold8/fold4=<1 if K was folded
on that geometry>,full8/full4=<1 if a tile ran k = K_MAX of that geometry>.
"""

import argparse
import re
import sys
from pathlib import Path

# Per invocation the runtime sends one MEM_WRITE (a 9-byte header and the
# slab) and one descriptor of 14 WRITE32 packets of 6 bytes
# (runtime/fpga_matmul_tiled.cpp, runtime/dpti_runtime.c).
MEM_WRITE_HEADER = 9
DESCRIPTOR_BYTES = 14 * 6
# ... and gets back the P x P fp32 result tile, preceded by the 16-byte
# completion record on a bitstream that has it (COMPLETION_HEADER=1).
COMPLETION_BYTES = 16

DEVICES = {"acc_8x8_0": (0, 8), "acc_4x4_0": (1, 4), "acc_4x4_1": (2, 4), "acc_4x4_2": (3, 4)}
TRANSPORT_RC = {-3, -4, -6, -7, -8, -9, -14}
SUBMISSION_RC = {-5, -10, -11, -12, -13, -15, -17, -18, -19}

# The runtime calls the lowering emits (ir/20-fpga.mlir), per mode.
SYNC_CALL_RE = re.compile(r"llvm\.call @fpga_matmul_tiled_auto_scheduled\(")
ASYNC_SUBMIT_RE = re.compile(r"llvm\.call @fpga_tile_submit\(")
ASYNC_BEGIN_RE = re.compile(r"llvm\.call @fpga_gemm_begin\(")
ASYNC_WAIT_RE = re.compile(r"llvm\.call @fpga_gemm_wait\(")

NAME_RE = re.compile(r"^(\d+)x(\d+)x(\d+)_")
TILE_RE = re.compile(r"systolic\.matmul_tile .* m = (\d+) n = (\d+) k = (\d+) on @(\w+)")
HEADER_RE = re.compile(r"^  K_MAX_8X8 = (\d+), K_MAX_4X4 = (\d+) \((.*)\)$", re.M)
FOLD_RE = re.compile(r"K folded by k_max = (\d+) on the 8x8, (\d+) on the 4x4s")
INV_RE = re.compile(r"FPGA invocations = (\d+)")
RUNTIME_RE = re.compile(r"\[5\] Runtime\s+(\d+) FPGA invocation")
BYTES_RE = re.compile(r"DPTI: (\d+) B to the FPGA, (\d+) B back")
MISMATCH_RE = re.compile(r"mismatches = (\d+) / (\d+)")
FIRST_RE = re.compile(r"^first mismatch: (.*)$", re.M)
DISPATCH_RE = re.compile(r"^FPGA dispatch: FAIL -- (\d+) of (\d+) runtime call\(s\) failed "
                         r"\(first error (-?\d+)\)", re.M)
CONFIG_RE = re.compile(r"^systolic-matmul: (SYSTOLIC_FPGA_CONFIG=.*)$", re.M)
TOOL_RE = re.compile(r"^systolic-matmul: (?!C\[)(.*)$", re.M)
ERROR_RE = re.compile(r"^.*\berror\b.*$", re.M)


def rc_class(rc):
    if rc == -16:
        return "CONFIG"
    if rc in TRANSPORT_RC:
        return "TRANSPORT"
    if rc in SUBMISSION_RC:
        return "SUBMISSION"
    return "FAIL"


def one_line(text, limit=160):
    text = " ".join(text.split())
    return text if len(text) <= limit else text[: limit - 3] + "..."


def verdict(a):
    d = Path(a.case_dir)
    out = (d / "output.txt").read_text(errors="replace") if (d / "output.txt").exists() else ""
    ir_path = d / "ir" / "10-systolic.mlir"
    ir = ir_path.read_text(errors="replace") if ir_path.exists() else ""
    tiles = [(int(m), int(n), int(k), dev) for m, n, k, dev in TILE_RE.findall(ir)]
    fpga_path = d / "ir" / "20-fpga.mlir"
    fpga_ir = fpga_path.read_text(errors="replace") if fpga_path.exists() else None

    inv = INV_RE.search(out) or RUNTIME_RE.search(out)
    inv = int(inv.group(1)) if inv else None
    by = BYTES_RE.search(out)
    to, back = (int(by.group(1)), int(by.group(2))) if by else (None, None)
    disp = DISPATCH_RE.search(out)
    rt_rc = "-"
    if disp:
        rt_rc = disp.group(3)
    elif a.exit in (0, 1):
        rt_rc = "0"

    def row(cls, note):
        return "\t".join(str(x) for x in (
            cls, rt_rc, len(tiles) if ir else "-", "-" if inv is None else inv,
            "-" if to is None else to, "-" if back is None else back, coverage(), note))

    def coverage():
        if not tiles:
            return "-"
        per = [0, 0, 0, 0]
        edge = fold8 = fold4 = full8 = full4 = 0
        for m, n, k, dev in tiles:
            if dev not in DEVICES:
                continue
            i, p = DEVICES[dev]
            per[i] += 1
            edge += m < p or n < p
            if p == 8:
                fold8 |= big_k > a.k8
                full8 |= k == a.k8
            else:
                fold4 |= big_k > a.k4
                full4 |= k == a.k4
        return (f"d0={per[0]},d1={per[1]},d2={per[2]},d3={per[3]},edge={edge},"
                f"fold8={int(fold8)},fold4={int(fold4)},full8={int(full8)},full4={int(full4)}")

    name = NAME_RE.match(d.name)
    big_m, big_n, big_k = (int(x) for x in name.groups()) if name else (0, 0, 0)

    # ---- the configuration, before anything else: it explains the rest ----
    cfg = CONFIG_RE.search(out)
    if a.exit == 4 and cfg:
        return row("CONFIG", "systolic-matmul refused the configuration: " + one_line(cfg.group(1)))
    head = HEADER_RE.search(out)
    if head:
        k8, k4, src = int(head.group(1)), int(head.group(2)), head.group(3)
        if (k8, k4) != (a.k8, a.k4) or src != a.source:
            return row("CONFIG", f"propagation: systolic-matmul's runtime read K_MAX_8X8 = {k8}, "
                                 f"K_MAX_4X4 = {k4} from {src}; the configuration is "
                                 f"{a.k8} / {a.k4} from {a.source}")
    elif a.exit in (0, 1):
        return row("FAIL", "no K_MAX_8X8 / K_MAX_4X4 line in output.txt: cannot tell which "
                           "configuration systolic-matmul used")
    fold = FOLD_RE.search(out)
    if fold and (int(fold.group(1)), int(fold.group(2))) != (a.k8, a.k4):
        return row("CONFIG", f"propagation: the compiler folded K by k_max = {fold.group(1)} on "
                             f"the 8x8, {fold.group(2)} on the 4x4s; the configuration is "
                             f"{a.k8} / {a.k4}")
    if tiles:
        deepest = {8: 0, 4: 0}
        for m, n, k, dev in tiles:
            if dev not in DEVICES:
                return row("FAIL", f"a tile on @{dev}, not one of the four devices")
            p = DEVICES[dev][1]
            cap = a.k8 if p == 8 else a.k4
            if not 1 <= k <= cap:
                return row("CONFIG", f"a tile on @{dev} has k = {k}, outside 1..K_MAX_"
                                     f"{p}X{p} = {cap}")
            deepest[p] = max(deepest[p], k)
        if name:
            for p, cap in ((8, a.k8), (4, a.k4)):
                if deepest[p] and deepest[p] != min(big_k, cap):
                    return row("CONFIG", f"the {p}x{p} tiles reach k = {deepest[p]}, not "
                                         f"min(K, K_MAX_{p}X{p}) = {min(big_k, cap)}: K was not "
                                         f"folded by the configured K_MAX")
            macs = sum(m * n * k for m, n, k, _ in tiles)
            if macs != big_m * big_n * big_k:
                return row("FAIL", f"the tiles cover {macs} multiply-adds; "
                                   f"{big_m}x{big_n}x{big_k} has {big_m * big_n * big_k}")

    # ---- the lowered IR calls the runtime of the chosen mode ----------------
    if fpga_ir is not None and tiles:
        sync_calls = len(SYNC_CALL_RE.findall(fpga_ir))
        submits = len(ASYNC_SUBMIT_RE.findall(fpga_ir))
        begins = len(ASYNC_BEGIN_RE.findall(fpga_ir))
        waits = len(ASYNC_WAIT_RE.findall(fpga_ir))
        if a.mode == "async":
            if submits != len(tiles) or begins != 1 or waits != 1 or sync_calls:
                return row("FAIL", f"--mode async, but the lowered IR has {submits} fpga_tile_submit, "
                                   f"{begins} fpga_gemm_begin, {waits} fpga_gemm_wait and {sync_calls} "
                                   f"fpga_matmul_tiled_auto_scheduled call(s) for {len(tiles)} tile(s)")
        elif sync_calls != len(tiles) or submits or begins or waits:
            return row("FAIL", f"--mode sync, but the lowered IR has {sync_calls} "
                               f"fpga_matmul_tiled_auto_scheduled and {submits} fpga_tile_submit "
                               f"call(s) for {len(tiles)} tile(s)")

    # ---- what the run itself says ------------------------------------------
    if a.exit == 0:
        if not re.search(r"^Verification: PASS", out, re.M):
            return row("FAIL", "exit 0 without 'Verification: PASS'")
        if inv != len(tiles):
            return row("FAIL", f"tiles {len(tiles)} != invocations {inv if inv is not None else '-'}")
        per = MEM_WRITE_HEADER + a.slab + DESCRIPTOR_BYTES
        if to is None or to != inv * per:
            return row("CONFIG", f"operand slab: {to} B to the FPGA for {inv} invocation(s), "
                                 f"not {inv} x (9 + {a.slab} + 84) = {inv * per} B")
        # Every tile comes back as its device's P x P fp32 tile, with the
        # completion record ahead of it on a bitstream that has one.
        want_back = sum(DEVICES[dev][1] ** 2 * 4 + (COMPLETION_BYTES if a.header else 0)
                        for _, _, _, dev in tiles)
        if back is None or back != want_back:
            return row("FAIL", f"{back} B back for {inv} invocation(s), not the {want_back} B of "
                               f"their result tiles{' and completion records' if a.header else ''}")
        if a.cosim_error:
            return row("FAIL", a.cosim_error)
        if a.cosim:
            recv, cons, sent, session = a.cosim
            if cons != to or recv != cons or sent != back:
                return row("TRANSPORT", f"runtime counted {to}/{back} B; RTL received {recv}, "
                                        f"consumed {cons}, sent {sent} B")
            return row("PASS", f"RTL consumed {cons} B, sent {sent} B (session {session})")
        return row("PASS", "")
    if a.exit == 1:
        mm = MISMATCH_RE.search(out)
        first = FIRST_RE.search(out)
        note = f"mismatches = {mm.group(1)} / {mm.group(2)}" if mm else "Verification: FAIL"
        if first:
            note += "; first mismatch: " + one_line(first.group(1), 100)
        return row("MISMATCH", note)
    if a.exit == 4:
        if disp:
            rc = int(disp.group(3))
            note = (f"{disp.group(1)} of {disp.group(2)} runtime call(s) failed, "
                    f"first error {rc}")
            if rc == -14:
                note += " (refused after an earlier link failure)"
            return row(rc_class(rc), note)
        tool = TOOL_RE.search(out)
        return row("FAIL", "exit 4: " + one_line(tool.group(1) if tool else "no runtime report"))
    if a.exit == 2:
        tool = TOOL_RE.search(out)
        return row("INPUT", one_line(tool.group(1) if tool else "exit 2"))
    if a.exit == 3:
        tool = TOOL_RE.search(out)
        err = ERROR_RE.search(out)
        return row("COMPILE", one_line(tool.group(1) if tool else err.group(0) if err else "exit 3"))
    if a.exit > 128:
        return row("FAIL", f"killed by signal {a.exit - 128}")
    return row("FAIL", f"exit {a.exit}")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("case_dir")
    ap.add_argument("--exit", type=int, required=True)
    ap.add_argument("--k8", type=int, required=True)
    ap.add_argument("--k4", type=int, required=True)
    ap.add_argument("--slab", type=int, required=True)
    ap.add_argument("--source", required=True)
    ap.add_argument("--mode", choices=("sync", "async"), default="sync")
    ap.add_argument("--header", type=int, choices=(0, 1), default=0)
    ap.add_argument("--cosim", nargs=4, type=int, metavar=("RECEIVED", "CONSUMED", "SENT", "SESSION"))
    ap.add_argument("--cosim-error")
    a = ap.parse_args()
    if a.k8 < 1 or a.k4 < 1 or a.slab < 1:
        print("--k8, --k4 and --slab must be positive", file=sys.stderr)
        return 2
    print(verdict(a))
    return 0


if __name__ == "__main__":
    sys.exit(main())
