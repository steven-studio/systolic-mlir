open_project build_dual/matmul_arty_4x4_dual.xpr
add_files -fileset sim_1 -norecurse ../rtl/sim/tb_matmul_top_dual.v
set_property top tb_matmul_top_dual [get_filesets sim_1]
set_property -name {xsim.simulate.runtime} -value {150ms} -objects [get_filesets sim_1]
launch_simulation
close_sim
exit
