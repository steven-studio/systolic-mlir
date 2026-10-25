# FPGA Archive: Shared Writeback B+ Revision 2

## Configuration

- Build and hardware validation date: 2026-10-25 (Asia/Taipei).
- Nexys Video / XC7A200T; operand variant v1; K8=16, K4=16.
- Fleet: 1 x 8x8 + 3 x 4x4; project tag `shared_wb_r2_etopt`.
- Placement: ExtraTimingOpt; routing: Explore.
- WNS: +0.098709 ns; WHS: +0.015890 ns.
- TIMING_MET=1; DPTI_IO_GATE=PASS.
- Exact validated image SHA-256: bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5.

## Archived Artifacts

- `systolic_dma_top_1x8x8_3x4x4.bit`: exact programmed/tested image, copied without rebuilding.
- `systolic_dma_top_1x8x8_3x4x4.ltx`: matching run's systolic_dma_top.ltx.
- `fpga_config.txt`: original build metadata with BITSTREAM relocated to the adjacent archived image. Resolve BITSTREAM relative to this directory.
- Routed timing, placed utilization and DPTI I/O gate reports.
- `hardware-report.md`: archive-relative evidence index and numerical case inventory.
- `hardware-validation/`: original report, actual commands, session logs, generated inputs, numerical outputs and suite summaries. Executables and generated compiler IR are not included. Paths inside original logs are historical provenance.
- `SHA256SUMS`: checksums for every archived file other than the manifest itself.

## Hardware Validation

- Stress: 2,560/2,560 jobs completed, per-context counts 512/684/684/680; zero numerical mismatches.
- Sync quick: 26/26 cases, 1,068 CPU-compared output elements, zero mismatches.
- Async regression: 52/52 cases, 15,875 CPU-compared output elements, zero mismatches.
- Full tiles passed individually on all four accelerators. Mixed workloads, interleaved GEMMs, host queue backpressure and completion matching passed.
- No unexpected timeouts, transport failures or deadlocks. Expected API timeout/refusal checks passed; simulation-only transport injection was not applicable.
- Internal simultaneous compute, early bank reuse, multiple pending DDR writes and round-robin grant fairness were not directly observed on hardware. See the report for limits; numerical correctness alone does not establish these properties.
- Runtime tests were compiled from the Revision 2 checkout. Compiler-driven cases used the sibling compiler binary with different scheduling cost-model support; they do not qualify an exact checkout compiler build.

## Provenance and Integrity

Source branch: `feat/heterogeneous-tile-pipelined`. Build metadata records RTL revision `229fe2431c0c72d25e6654a88e8e5d47c697301b` and RTL_TREE_DIRTY=1; the validated Revision 2 changes were uncommitted at build time. Do not interpret the recorded base revision alone as the complete implementation.

Original build directory:
`/home/steven-studio/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4_shared_wb_r2_etopt/systolic_dma.runs/impl_1/`.
The original Vivado project and temporary evidence are untouched. Archive operation ran no Vivado or programming.

Run `sha256sum -c SHA256SUMS` from this directory. Existing repository policy ignores *.bit and *.log globally; archive commits must explicitly force-add only the intended archived bitstream and evidence logs, as appropriate. No ignore rules were changed.
