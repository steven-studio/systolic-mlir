set clock_constraint { \
    name clk \
    module matmul_4x4x4 \
    port ap_clk \
    period 10 \
    uncertainty 2.7 \
}

set all_path {}

set false_path {}

