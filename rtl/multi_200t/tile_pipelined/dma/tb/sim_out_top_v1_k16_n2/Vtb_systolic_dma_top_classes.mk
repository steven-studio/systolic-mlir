# Verilated -*- Makefile -*-
# DESCRIPTION: Verilator output: Make include file with class lists
#
# This file lists generated Verilated files, for including in higher level makefiles.
# See Vtb_systolic_dma_top.mk for the caller.

### Switches...
# C11 constructs required?  0/1 (always on now)
VM_C11 = 1
# Timing enabled?  0/1
VM_TIMING = 1
# Coverage output mode?  0/1 (from --coverage)
VM_COVERAGE = 0
# Parallel builds?  0/1 (from --output-split)
VM_PARALLEL_BUILDS = 1
# Tracing output mode?  0/1 (from --trace/--trace-fst)
VM_TRACE = 0
# Tracing output mode in VCD format?  0/1 (from --trace)
VM_TRACE_VCD = 0
# Tracing output mode in FST format?  0/1 (from --trace-fst)
VM_TRACE_FST = 0

### Object file lists...
# Generated module classes, fast-path, compile with highest optimization
VM_CLASSES_FAST += \
	Vtb_systolic_dma_top \
	Vtb_systolic_dma_top___024root__DepSet_heb4ee87e__0 \
	Vtb_systolic_dma_top___024root__DepSet_heb4ee87e__1 \
	Vtb_systolic_dma_top___024root__DepSet_heb4ee87e__2 \
	Vtb_systolic_dma_top___024root__DepSet_hf76adfe6__0 \
	Vtb_systolic_dma_top_systolic_array__N4__DepSet_h3ca27485__0 \
	Vtb_systolic_dma_top_systolic_array__N4__DepSet_h84f133e6__0 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__0 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__1 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__2 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__3 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__4 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__5 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__6 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__7 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__8 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__9 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__10 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__11 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__12 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__13 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__14 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__15 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__16 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__17 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__18 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__19 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__20 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__21 \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__22 \
	Vtb_systolic_dma_top__main \

# Generated module classes, non-fast-path, compile with low/medium optimization
VM_CLASSES_SLOW += \
	Vtb_systolic_dma_top__ConstPool_0 \
	Vtb_systolic_dma_top___024root__Slow \
	Vtb_systolic_dma_top___024root__DepSet_heb4ee87e__0__Slow \
	Vtb_systolic_dma_top___024root__DepSet_hf76adfe6__0__Slow \
	Vtb_systolic_dma_top_systolic_array__N4__Slow \
	Vtb_systolic_dma_top_systolic_array__N4__DepSet_h3ca27485__0__Slow \
	Vtb_systolic_dma_top_systolic_pe__Slow \
	Vtb_systolic_dma_top_systolic_pe__DepSet_h808d8902__0__Slow \

# Generated support classes, fast-path, compile with highest optimization
VM_SUPPORT_FAST += \
	Vtb_systolic_dma_top__Dpi \

# Generated support classes, non-fast-path, compile with low/medium optimization
VM_SUPPORT_SLOW += \
	Vtb_systolic_dma_top__Syms \
	Vtb_systolic_dma_top__Syms__1 \

# Global classes, need linked once per executable, fast-path, compile with highest optimization
VM_GLOBAL_FAST += \
	verilated \
	verilated_dpi \
	verilated_timing \
	verilated_threads \

# Global classes, need linked once per executable, non-fast-path, compile with low/medium optimization
VM_GLOBAL_SLOW += \


# Verilated -*- Makefile -*-
