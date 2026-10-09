# FPGA Archive: 1x8x8 + 3x4x4 Heterogeneous Fleet

## Configuration

- Date: 2026-10-09
- Configuration: 1 x 8x8 + 3 x 4x4 systolic arrays
- K dimension: 16
- Status: timing-clean implementation
- Role: canonical tile-pipelined heterogeneous FPGA configuration

## Physical Device Mapping

| Device ID | Physical Accelerator |
|-----------|----------------------|
| 0 | 8x8 #0 |
| 1 | 4x4 #0 |
| 2 | 4x4 #1 |
| 3 | 4x4 #2 |

## Implementation Timing

Source: `systolic_dma_top_timing_summary_routed.rpt`

- WNS: +0.079 ns
- TNS: 0.000 ns
- WHS: +0.047 ns
- THS: 0.000 ns
- All user-specified timing constraints are met.

## Resource Utilization

Source: `systolic_dma_top_utilization_placed.rpt`

- Slice LUTs: 100733 / 133800 (75.29%)
- Slice Registers: 130056 / 267600 (48.60%)
- DSPs: 672 / 740 (90.81%)

## Source Checkpoint

Corresponding source checkpoint/tag:

`tile-pipelined-1x8x8-3x4x4-20261009`

## Archived Artifacts

- `systolic_dma_top_1x8x8_3x4x4.bit`
- `systolic_dma_top_timing_summary_routed.rpt`
- `systolic_dma_top_utilization_placed.rpt`
- `SHA256SUMS`
- `README.md`

## Integrity Verification

Run `sha256sum -c SHA256SUMS` from this directory.

The checksum file protects the bitstream, routed timing report, and
placed utilization report.

## Archive Policy

This archive contains the verified 1x8x8 + 3x4x4 heterogeneous fleet
implementation.

The archived bitstream must not be replaced by a newly generated
implementation result without also updating the corresponding timing
report, utilization report, archive metadata, and SHA-256 checksums.
