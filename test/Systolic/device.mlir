module {
  systolic.device @acc_8x8
    rows = 8
    cols = 8
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_0
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_1
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10

  systolic.device @acc_4x4_2
    rows = 4
    cols = 4
    depth = 1
    dataflow = output_stationary
    tile_overhead = 10
}
