open_project -reset fp_csim

set_top matmul_8x8_fold_pipelined

add_files design.cpp
add_files design.h
add_files -tb tb.cpp

open_solution -reset solution1

set_part {xc7a200tsbg484-1}

create_clock -period 10

csim_design

exit
