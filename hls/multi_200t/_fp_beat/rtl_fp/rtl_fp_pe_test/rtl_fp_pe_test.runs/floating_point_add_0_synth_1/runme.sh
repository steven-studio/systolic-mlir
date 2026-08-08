#!/bin/bash

# 
# Vivado(TM)
# runme.sh: a Vivado-generated Runs Script for UNIX
# Copyright 1986-2022 Xilinx, Inc. All Rights Reserved.
# Copyright 2022-2026 Advanced Micro Devices, Inc. All Rights Reserved.
# 

if [ -z "$PATH" ]; then
  PATH=/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vitis/bin:/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vivado/ids_lite/ISE/bin/lin64:/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vivado/bin
else
  PATH=/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vitis/bin:/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vivado/ids_lite/ISE/bin/lin64:/home/steven-studio/tools/Xilinx/2026.1/2026.1/Vivado/bin:$PATH
fi
export PATH

if [ -z "$LD_LIBRARY_PATH" ]; then
  LD_LIBRARY_PATH=
else
  LD_LIBRARY_PATH=:$LD_LIBRARY_PATH
fi
export LD_LIBRARY_PATH

HD_PWD='/home/steven-studio/systolic-mlir/hls/multi_200t/_fp_beat/rtl_fp/rtl_fp_pe_test/rtl_fp_pe_test.runs/floating_point_add_0_synth_1'
cd "$HD_PWD"

HD_LOG=runme.log
/bin/touch $HD_LOG

ISEStep="./ISEWrap.sh"
EAStep()
{
     $ISEStep $HD_LOG "$@" >> $HD_LOG 2>&1
     if [ $? -ne 0 ]
     then
         exit
     fi
}

EAStep vivado -log floating_point_add_0.vds -m64 -product Vivado -mode batch -messageDb vivado.pb -notrace -source floating_point_add_0.tcl
