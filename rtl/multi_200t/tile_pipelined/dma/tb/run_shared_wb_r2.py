#!/usr/bin/env python3
"""Reproducible RTL-only Revision 2 regression. Artifacts and exact commands
are written to a caller-selected directory; no existing files are removed.
"""
import argparse
import json
from pathlib import Path
import re
import shlex
import subprocess

HERE = Path(__file__).resolve().parent
DMA = HERE.parent
ROOT = DMA.parent
REPO = ROOT.parents[2]
COSIM = REPO / "test/e2e/cosim"
parser = argparse.ArgumentParser()
parser.add_argument("suite", choices=["unit", "context", "fleet", "legacy", "pipeline"])
parser.add_argument("out", type=Path)
args = parser.parse_args()
args.out.mkdir(parents=True, exist_ok=True)
records = []

def execute(name, command, marker=None):
    command = list(map(str, command))
    print(shlex.join(command), flush=True)
    result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    (args.out / (name + ".log")).write_text(result.stdout)
    passed = result.returncode == 0 and (marker is None or marker in result.stdout)
    if marker and ("FAIL" in result.stdout or "%Error" in result.stdout or "mismatch job" in result.stdout):
        passed = False
    records.append(dict(name=name, command=shlex.join(command), result="PASS" if passed else "FAIL", returncode=result.returncode))
    (args.out / "commands.json").write_text(json.dumps(records, indent=2) + "\n")
    print(name, records[-1]["result"], flush=True)
    if not passed:
        print(result.stdout[-5000:])
        raise SystemExit(1)
    if marker:
        print("\n".join(line for line in result.stdout.splitlines() if re.match(r"^(PASS|==|CHECKED|WB_COVER|PDEV|PSUM)", line)), flush=True)


def verilate(name, top, sources, extra=()):
    directory = args.out / name
    execute(name + "-build", ["verilator", "--binary", "--timing", "--assert", "--public-flat-rw", "-Wno-fatal", "-j", "4",
        "--top-module", top, "--Mdir", directory, "-o", "run", *extra, *sources])
    return directory / "run"


def top_sources():
    text = (HERE / "run_pipeline_sim.sh").read_text()
    mapping = {"HERE": HERE, "DMA": DMA, "ROOT": ROOT, "COSIM": COSIM}
    files = re.findall(r'"\$(HERE|DMA|ROOT|COSIM)/([^"\n]+\.(?:sv|cpp))"', text)
    return [mapping[key] / value for key, value in files if value != "tb_pipeline_overlap.sv"]


if args.suite == "unit":
    cases = {
        "reader": ("tb_dma_result_reader", [DMA / "dma_result_reader.sv", ROOT / "core/tx_source.sv"]),
        "fifo": ("tb_dma_cdc_fifo", [DMA / "dma_cdc_fifo.sv"]),
        "path": ("tb_dma_writeback_path", [DMA / "dma_result_reader.sv", DMA / "dma_cdc_fifo.sv", DMA / "dma_writeback_engine.sv", ROOT / "core/tx_source.sv"]),
        "engine16": ("tb_dma_writeback_engine", [DMA / "dma_writeback_engine.sv"]),
        "engine2": ("tb_dma_writeback_engine", [DMA / "dma_writeback_engine.sv"]),
    }
    for name, (top, sources) in cases.items():
        binary = args.out / (name + ".vvp")
        parameters = ["-Ptb_dma_writeback_engine.BURST_LEN=2"] if name == "engine2" else []
        execute(name + "-build", ["iverilog", "-g2012", "-s", top, *parameters, "-o", binary, HERE / (top + ".sv"), *sources])
        execute(name, ["vvp", binary, *(["+b_delay=17"] if name.startswith("engine") else [])], "PASS")
elif args.suite == "context":
    sources = [HERE / "tb_shared_wb_context.sv", DMA / "systolic_accel_context.sv"]
    sources += [DMA / (n + ".sv") for n in ["dma_operand_writer", "dma_operand_writer_v2", "dma_wr_checksum_v2", "dma_result_reader", "dma_cdc_fifo"]]
    sources += [ROOT / "core" / (n + ".sv") for n in ["operand_buffer", "operand_buffer_v2", "tile_feeder"]]
    for n in [8, 4]:
        binary = verilate("context" + str(n), "tb_shared_wb_context", sources, ["-GN=" + str(n)])
        execute("context" + str(n), [binary], "PASS context")
elif args.suite == "fleet":
    (args.out / "pipe_jobs.svh").write_text((HERE / "shared_wb_jobs.svh").read_text())
    for variant in [0, 1]:
        binary = verilate("fleet-v" + str(variant + 1), "tb_pipeline_overlap",
            [HERE / "tb_pipeline_overlap.sv", *top_sources()],
            ["-I" + str(HERE), "-I" + str(args.out), "-GUSE_V2=" + str(variant)])
        execute("fleet-v" + str(variant + 1), [binary, "+wb_region=4096", "+fourway=1", "+aw_stall=5", "+w_stall=3", "+b_delay=80"], "== PASS")
        execute("boundary-v" + str(variant + 1),
            [binary, "+c_base=4080", "+wb_region=4080", "+fourway=1", "+aw_stall=5", "+w_stall=3", "+b_delay=80"], "== PASS")
        for mode in [1, 2, 3]:
            extra = [] if mode == 1 else ["+wb_error_addr=4096", "+wb_error_resp=" + str(mode)]
            execute("error-v" + str(variant + 1) + "-" + str(mode),
                [binary, "+wb_region=4096", "+error_mode=" + str(mode), "+fourway=1", "+b_delay=80", *extra], "== PASS")
elif args.suite == "pipeline":
    for case in ["dev0", "dev1", "chain", "gapbound"]:
        directory=args.out / case
        directory.mkdir(parents=True, exist_ok=True)
        (directory / "pipe_jobs.svh").write_text((REPO / "eval/pipeline_overlap/jobs" / (case + ".svh")).read_text())
        extra=["-I" + str(directory), "-I" + str(HERE), "-GUSE_V2=1"]
        if case=="gapbound": extra += ["-GK_MAX_8X8=32", "-GK_MAX_4X4=32"]
        binary=verilate(case, "tb_pipeline_overlap", [HERE / "tb_pipeline_overlap.sv", *top_sources()],extra)
        execute(case,[binary,"+wb_region=4096",*(["+expect_min_gap=26", "+min_feed_overlaps=1"] if case=="gapbound" else [])],"== PASS")
else:
    sources = [HERE / "tb_systolic_dma_top.sv", *[s for s in top_sources() if s.name not in ["fp_ieee.sv", "dpti_cosim.cpp"]], ROOT / "tb/fp_model.sv"]
    binary = verilate("legacy", "tb_systolic_dma_top", sources, ["-GK_MAX_8X8=16", "-GK_MAX_4X4=16", "-GK_DIM=16"])
    execute("legacy", [binary, "+n_inv=3", "+wb_region=7168"], "== PASS")
