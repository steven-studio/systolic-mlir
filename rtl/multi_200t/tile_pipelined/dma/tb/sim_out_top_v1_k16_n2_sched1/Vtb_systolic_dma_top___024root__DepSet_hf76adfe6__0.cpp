// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top___024root.h"

VL_INLINE_OPT VlCoroutine Vtb_systolic_dma_top___024root___eval_initial__TOP__Vtiming__2(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_initial__TOP__Vtiming__2\n"); );
    // Body
    while (1U) {
        co_await vlSelf->__VdlySched.delay(0x208dULL, 
                                           nullptr, 
                                           "/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 
                                           147);
        vlSelf->tb_systolic_dma_top__DOT__dpti_clkout 
            = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dpti_clkout)));
    }
}

VL_INLINE_OPT VlCoroutine Vtb_systolic_dma_top___024root___eval_initial__TOP__Vtiming__3(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_initial__TOP__Vtiming__3\n"); );
    // Body
    while (1U) {
        co_await vlSelf->__VdlySched.delay(0x1388ULL, 
                                           nullptr, 
                                           "/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 
                                           146);
        vlSelf->tb_systolic_dma_top__DOT__clk = (1U 
                                                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__clk)));
    }
}

void Vtb_systolic_dma_top___024root___eval_triggers__ico(Vtb_systolic_dma_top___024root* vlSelf);
void Vtb_systolic_dma_top___024root___eval_ico(Vtb_systolic_dma_top___024root* vlSelf);

bool Vtb_systolic_dma_top___024root___eval_phase__ico(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_phase__ico\n"); );
    // Init
    CData/*0:0*/ __VicoExecute;
    // Body
    Vtb_systolic_dma_top___024root___eval_triggers__ico(vlSelf);
    __VicoExecute = vlSelf->__VicoTriggered.any();
    if (__VicoExecute) {
        Vtb_systolic_dma_top___024root___eval_ico(vlSelf);
    }
    return (__VicoExecute);
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___act_sequent__TOP__0(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___act_sequent__TOP__0\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpu_resetn 
        = vlSelf->tb_systolic_dma_top__DOT__rstn;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_valid 
        = vlSelf->tb_systolic_dma_top__DOT__job_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id 
        = vlSelf->tb_systolic_dma_top__DOT__job_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id 
        = vlSelf->tb_systolic_dma_top__DOT__job_device_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m 
        = vlSelf->tb_systolic_dma_top__DOT__job_m;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n 
        = vlSelf->tb_systolic_dma_top__DOT__job_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k 
        = vlSelf->tb_systolic_dma_top__DOT__job_k;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__job_est_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_a_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_b_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_c_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_pulse 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_d)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_rst_n 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__LOCKED) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__rstn));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_valid) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_m;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_n;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_k;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_a_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_b_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_c_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_device_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_start_cycle;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_est_cycles;
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id 
            = vlSelf->tb_systolic_dma_top__DOT__job_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m 
            = vlSelf->tb_systolic_dma_top__DOT__job_m;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n 
            = vlSelf->tb_systolic_dma_top__DOT__job_n;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k 
            = vlSelf->tb_systolic_dma_top__DOT__job_k;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_a_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_b_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_c_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
            = vlSelf->tb_systolic_dma_top__DOT__job_device_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle 
            = vlSelf->tb_systolic_dma_top__DOT__job_start_cycle;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles 
            = vlSelf->tb_systolic_dma_top__DOT__job_est_cycles;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_valid) 
           | (IData)(vlSelf->tb_systolic_dma_top__DOT__job_valid));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_probe 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__sys_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_rst_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_job_id 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_m 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_n 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_k 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_a_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_b_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_c_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_accelerator_id 
        = (3U & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
                 >> 0U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_device_id 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_compute_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_est_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___act_sequent__TOP__2(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___act_sequent__TOP__2\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__descriptor_downstream_ready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_desc_fire 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__0(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__0\n"); );
    // Init
    CData/*3:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 0;
    CData/*2:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 0;
    CData/*7:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending = 0;
    CData/*2:0*/ __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0;
    __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0 = 0;
    VlWide<5>/*157:0*/ __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0;
    VL_ZERO_W(158, __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0);
    CData/*0:0*/ __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0;
    __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0 = 0;
    CData/*1:0*/ __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0;
    __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 = 0;
    QData/*39:0*/ __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0;
    __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0;
    __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 = 0;
    // Body
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy;
    __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 = 0U;
    __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0 = 0U;
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity 
        = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity);
    if ((1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst)))) {
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready))) {
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 
                = (((QData)((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_addr)) 
                    << 0x20U) | (QData)((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_data)));
            __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 = 1U;
            __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0 
                = (3U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin));
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready))) {
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[0U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[0U];
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[1U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[1U];
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[2U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[2U];
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[3U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[3U];
            __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[4U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[4U];
            __Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0 = 1U;
            __Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0 
                = (7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin));
        }
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst) {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_addr = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_opcode = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_addr = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_data = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__err_opcode = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[0U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[1U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[2U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[3U] = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr2 = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2 = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd2 = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr1 = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr1 = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd1 = 0U;
    } else {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending = 1U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_addr 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_base_addr;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending = 0U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next;
        }
        if ((((((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner)) 
                & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid)) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready)) 
              & (1U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data))) 
             & (2U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_opcode = 1U;
        }
        if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner))) {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready))) {
                if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 1U;
                } else if ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 2U;
                }
            }
        } else if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner))) {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 0U;
            }
        } else if ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner))) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_done) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 0U;
            }
        } else {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = 0U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data 
                = vlSelf->tb_systolic_dma_top__DOT__dpti_d;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid = 1U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid = 0U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid = 0U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready))) {
            if ((4U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 0U;
                } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_addr 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_data 
                        = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                            << 0x18U) | (0xffffffU 
                                         & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r));
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid = 1U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
                        = ((0xffffffU & __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r) 
                           | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                              << 0x18U));
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
                        = ((0xff00ffffU & __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r) 
                           | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                              << 0x10U));
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 5U;
                }
            } else if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
                        = ((0xffff00ffU & __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r) 
                           | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                              << 8U));
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 4U;
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
                        = ((0xffffff00U & __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r) 
                           | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data));
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 3U;
                }
            } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 2U;
            } else if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 1U;
            } else {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__err_opcode = 1U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = 0U;
            }
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready) {
                if ((0xfU == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy = 0U;
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx 
                        = (0xfU & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx)));
                }
            }
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[0U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[0U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[1U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[1U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[2U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[2U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[3U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[3U];
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy = 1U;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr2 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr1;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr1;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd2 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd1;
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin_next;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin_next;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr1 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr1 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd1 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending;
    if (__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem__v0;
    }
    if (__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0][0U] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[0U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0][1U] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[1U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0][2U] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[2U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0][3U] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[3U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vdlyvdim0__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0][4U] 
            = __Vdlyvval__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem__v0[4U];
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_valid 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid) 
           & ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner)) 
              | ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner)) 
                 & (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data)))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data 
        = (0xffU & (((0U == (0x1fU & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx), 3U)))
                      ? 0U : (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[
                              (((IData)(7U) + (0x7fU 
                                               & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx), 3U))) 
                               >> 5U)] << ((IData)(0x20U) 
                                           - (0x1fU 
                                              & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx), 3U))))) 
                    | (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r[
                       (3U & (VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx), 3U) 
                              >> 5U))] >> (0x1fU & 
                                           VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx), 3U)))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_d_out 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_valid;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__69(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__69\n"); );
    // Init
    CData/*2:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = 0;
    // Body
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state;
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__start = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__done = 0U;
        if ((4U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
            if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0U;
            } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0U;
            } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid) 
                        & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid = 0U;
                if ((0x800U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count)) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__done = 1U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0U;
                } else {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[0U] = 0U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[1U] = 0U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[2U] = 0U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[3U] = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 3U;
                }
            }
        } else if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
            if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
                if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid) 
                     & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready))) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count 
                        = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count);
                    VL_ASSIGNSEL_WI(128,8,(0x7fU & 
                                           VL_MULS_III(32, (IData)(8U), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index)), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data);
                    if ((0xfU == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index)) {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index = 0U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 4U;
                    } else {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index 
                            = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index);
                    }
                }
            } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid) 
                        & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp 
                    = (((~ ((IData)(0xffU) << (0x1fU 
                                               & VL_MULS_III(32, (IData)(8U), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)))) 
                        & __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp) 
                       | (0xffffffffULL & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                                           << (0x1fU 
                                               & VL_MULS_III(32, (IData)(8U), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)))));
                if ((3U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = 0U;
                    if ((0x800U != (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                                     << 0x18U) | (0xffffffU 
                                                  & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp)))) {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_length = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0U;
                    } else {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count = 0U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index = 0U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__base_addr 
                            = (0x1fffffffU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp);
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__start = 1U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[0U] = 0U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[1U] = 0U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[2U] = 0U;
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[3U] = 0U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 3U;
                    }
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index 
                        = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index);
                }
            }
        } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state))) {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp 
                    = (((~ ((IData)(0xffU) << (0x1fU 
                                               & VL_MULS_III(32, (IData)(8U), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)))) 
                        & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp) 
                       | (0xffffffffULL & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data) 
                                           << (0x1fU 
                                               & VL_MULS_III(32, (IData)(8U), vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)))));
                if ((3U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index)) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 2U;
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index 
                        = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index);
                }
            }
        } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid) 
                    & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready))) {
            if ((2U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_opcode = 1U;
            } else {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp = 0U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp = 0U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = 0U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 1U;
            }
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__base_addr = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[0U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[1U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[2U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[3U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__start = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__done = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_opcode = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_length = 0U;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_base_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__base_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready 
        = (4U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_base_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_base_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_base_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_base_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_base_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_base_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data[3U];
}

extern const VlUnpacked<CData/*1:0*/, 128> Vtb_systolic_dma_top__ConstPool__TABLE_h73ee5be8_0;
extern const VlUnpacked<CData/*0:0*/, 128> Vtb_systolic_dma_top__ConstPool__TABLE_h4abd77d6_0;
extern const VlUnpacked<CData/*0:0*/, 128> Vtb_systolic_dma_top__ConstPool__TABLE_h1c95a1cb_0;
extern const VlUnpacked<CData/*3:0*/, 64> Vtb_systolic_dma_top__ConstPool__TABLE_h7a124f99_0;
extern const VlUnpacked<CData/*0:0*/, 64> Vtb_systolic_dma_top__ConstPool__TABLE_h8c101dee_0;
extern const VlUnpacked<CData/*0:0*/, 64> Vtb_systolic_dma_top__ConstPool__TABLE_h09c976a5_0;
extern const VlUnpacked<CData/*0:0*/, 64> Vtb_systolic_dma_top__ConstPool__TABLE_hd80ff9dd_0;
extern const VlUnpacked<CData/*0:0*/, 64> Vtb_systolic_dma_top__ConstPool__TABLE_h109f5694_0;

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__70(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__70\n"); );
    // Init
    CData/*0:0*/ tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc5d6486b__0;
    tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc5d6486b__0 = 0;
    CData/*0:0*/ tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0;
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 = 0;
    CData/*0:0*/ tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0;
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__Vfuncout = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__k;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__k = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout = 0;
    CData/*6:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v = 0;
    CData/*2:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__Vfuncout = 0;
    IData/*28:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__a;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__a = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__remaining;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__remaining = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__to_page;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__to_page = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__Vfuncout = 0;
    IData/*28:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__a;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__a = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__remaining;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__remaining = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__to_page;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__to_page = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim = 0;
    CData/*5:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g = 0;
    CData/*5:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__Vfuncout = 0;
    IData/*28:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__a;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__a = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__remaining;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__remaining = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__to_page;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__to_page = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__Vfuncout = 0;
    IData/*28:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__a;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__a = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__remaining;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__remaining = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__to_page;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__to_page = 0;
    IData/*31:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim = 0;
    CData/*6:0*/ __Vtableidx1;
    __Vtableidx1 = 0;
    CData/*5:0*/ __Vtableidx2;
    __Vtableidx2 = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid = 0;
    CData/*3:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi = 0;
    CData/*3:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi = 0;
    CData/*3:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running = 0;
    CData/*7:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 0;
    CData/*7:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left = 0;
    CData/*3:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit = 0;
    SData/*15:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left = 0;
    CData/*7:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles = 0;
    CData/*2:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 0;
    CData/*4:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles = 0;
    CData/*0:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy = 0;
    CData/*1:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt = 0;
    IData/*31:0*/ __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written = 0;
    // Body
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__folds_done;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_running;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_c;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_wr;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id_reg;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi;
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left;
    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state;
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_request) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__unnamedblk1__DOT__start_i = 1U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__unnamedblk1__DOT__start_i = 2U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__unnamedblk1__DOT__start_i = 3U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__unnamedblk1__DOT__start_i = 4U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles = 0U;
        } else {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arvalid) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arready)))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles);
            }
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awready)))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles);
            }
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr) 
                 | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written);
            }
            if ((0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles);
            }
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rvalid) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rready)))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles);
            }
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready)))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles);
            }
        }
    } else {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles = 0U;
    }
    if ((1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear)))) {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running = 0U;
    } else {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done 
                = (0xffU & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__folds_done)));
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles);
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running = 1U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_running) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles);
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running = 0U;
            }
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles);
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running = 1U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_running) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles);
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running = 0U;
            }
        }
        if (((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_ready)) 
              & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_running))) 
             & (0U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running = 1U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span = 1U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_running) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span);
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running = 0U;
            }
        }
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        if (VL_UNLIKELY(((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_wr) 
                         & VL_GTS_III(32, 0x10U, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count)))) {
            VL_WRITEF("BWRDBG #%0d beat=%0# dst=%x wsel=%0# waddr=%0# data=%x\n",
                      32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count,
                      16,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat),
                      128,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data.data(),
                      3,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel),
                      5,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr,
                      32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf);
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count 
                = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count);
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count = 0U;
    }
    __Vtableidx1 = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_seen) 
                     << 6U) | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done) 
                                << 5U) | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_armed) 
                                           << 4U) | 
                                          (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active) 
                                            << 3U) 
                                           | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected) 
                                               << 2U) 
                                              | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire) 
                                                  << 1U) 
                                                 | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)))))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_h73ee5be8_0
         [__Vtableidx1])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_seen 
            = Vtb_systolic_dma_top__ConstPool__TABLE_h4abd77d6_0
            [__Vtableidx1];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_h73ee5be8_0
         [__Vtableidx1])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_armed 
            = Vtb_systolic_dma_top__ConstPool__TABLE_h1c95a1cb_0
            [__Vtableidx1];
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles = 0U;
        } else {
            if ((0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles);
            }
            if (((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state)) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid)))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles);
            }
        }
        if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c = 0U;
        } else {
            if (((5U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c 
                    = ((IData)(0xc74b2660U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_c);
            }
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr 
                    = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_wr 
                       + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__active_expected_wr_chk);
            }
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_val_d) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c 
                    = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c 
                       + (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_rd 
                          ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpos));
            }
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__accept) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy = 1U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt = 0U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy) {
            if ((3U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt))) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy = 0U;
            } else {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt 
                    = (3U & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt)));
            }
        }
    } else {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c = 0U;
    }
    if ((1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear)))) {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi = 0U;
    } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) 
                & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi_last)))) {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi 
            = (0xfU & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi)));
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        if (VL_UNLIKELY(((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase) 
                         != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase_prev)))) {
            VL_WRITEF("FSMTRANS t=%0t %0#->%0# fi=%0# | fill=%0b c_done_fold=%0b scan_last=%0b | wb_desc_valid=%0b wb_desc_ready=%0b wb_done=%0b | select8=%0b select4=%0b\n",
                      64,VL_TIME_UNITED_Q(1000),-9,
                      4,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase_prev),
                      4,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase,
                      4,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_ready,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4));
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase_prev 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase;
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase_prev = 0U;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__walmost_full_r 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           & (0x18U <= (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__occupancy)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__walmost_full_r 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           & (0x18U <= (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__occupancy)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_d 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull_r 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull_val));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull_r 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull_val));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_r 
        = (1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
                 | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_val)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_r 
        = (1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
                 | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_val)));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_d 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_i;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_d 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_i;
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_bvalid) 
             & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bresp)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_resp = 1U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__accept) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[0U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[0U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[1U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[1U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[2U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[2U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[3U] 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[3U];
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__in_range 
                = (0x7fU >= (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat));
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__b_fire) 
             & (~ ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bresp)) 
                   | (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bresp)))))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_resp = 1U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_bvalid) 
             & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bresp)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_resp = 1U;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin_next;
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range = 0U;
        } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__accept) 
                    & (0x7fU < (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range = 1U;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_sync 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_meta;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid = 0U;
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_fire) 
             & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__b_fire)))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit 
                = (7U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit) 
                         - (IData)(1U)));
        } else if (((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_fire)) 
                    & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__b_fire))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit 
                = (7U & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit)));
        }
        if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state))) {
            if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state))) {
                if ((4U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit))) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid = 1U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_tag 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__tag;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 0U;
                }
            } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_fire) {
                if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left))) {
                    if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left))) {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 3U;
                    } else {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len 
                            = (0x1fU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__len_next);
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__awlen_r 
                            = (0xffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__len_next 
                                        - (IData)(1U)));
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 1U;
                    }
                } else {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left 
                        = (0x1fU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left) 
                                    - (IData)(1U)));
                }
            }
        } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state))) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_fire) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_next;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_after;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 2U;
            }
        } else if ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid) 
                     & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_ready)) 
                    & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats)))) {
            if ((0U != (0xfU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_align = 1U;
            } else {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__tag = 0x3cU;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len 
                    = (0x1fU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__first_len);
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__awlen_r 
                    = (0xffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__first_len 
                                - (IData)(1U)));
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit = 4U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 1U;
            }
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq2_wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq1_wgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq2_wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq1_wgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq1_rgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq1_rgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid = 0U;
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ar_fire) 
             & (~ ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_fire) 
                   & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rlast))))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit 
                = (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit) 
                           - (IData)(1U)));
        } else if (((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ar_fire)) 
                    & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_fire) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rlast)))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit 
                = (0xfU & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit)));
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_fire) 
             & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx 
                = (0xffffU & ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx)));
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left 
                = (0xffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left) 
                              - (IData)(1U)));
            if ((1U & (~ ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rresp)) 
                          | (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rresp)))))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_resp = 1U;
            }
        }
        if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state))) {
            if ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid) 
                  & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_ready)) 
                 & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats)))) {
                if ((0U != (0xfU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr))) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_align = 1U;
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_tag;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_left 
                        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit = 8U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len 
                        = (0x1fU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__first_len);
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__arlen_r 
                        = (0xffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__first_len 
                                    - (IData)(1U)));
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 1U;
                }
            }
        } else if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state))) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ar_fire) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_next;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_left 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_after;
                if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_after))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 2U;
                } else {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len 
                        = (0x1fU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__len_next);
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__arlen_r 
                        = (0xffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__len_next 
                                    - (IData)(1U)));
                }
            }
        } else if ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state))) {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_fire) 
                 & (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left)))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid = 1U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_tag 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 0U;
            }
        } else {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 0U;
        }
        if ((((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active) 
                & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done)) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last)) 
              & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending))) 
             & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active)))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending = 1U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_addr_reg 
                = (0x1fffffffU & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg));
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_ready))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active = 1U;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_valid) 
             & (0x3cU == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_tag)))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active = 0U;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__done = 0U;
        if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state))) {
            if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state))) {
                if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_bvalid) {
                    if ((1U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left)) {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__done = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 0U;
                    } else {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left 
                            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left 
                               - (IData)(1U));
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr 
                            = (0x1fffffffU & ((IData)(0x100U) 
                                              + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr));
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 1U;
                    }
                }
            } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid) 
                        & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wready))) {
                if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left))) {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 3U;
                } else {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left 
                        = (0xffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left) 
                                    - (IData)(1U)));
                }
            }
        } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state))) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awready) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid = 0U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left = 0xfU;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 2U;
            }
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_start) {
            if ((0U != (0xffU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_base_addr))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_align = 1U;
            } else {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_base_addr;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left = 8U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid = 1U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 1U;
            }
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_sync 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_meta;
        if (((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy) 
               & ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awvalid) 
                    | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wvalid)) 
                   | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awvalid)) 
                  | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wvalid))) 
              | (((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy)) 
                  & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w)) 
                 & ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awvalid) 
                      | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wvalid)) 
                     | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awvalid)) 
                    | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid)))) 
             | (((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy)) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w))) 
                & ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awvalid) 
                     | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wvalid)) 
                    | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awvalid)) 
                   | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid))))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__err_w_owner = 1U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (1U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active) 
                & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done)) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last)) 
              & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending))) 
             & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (2U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (4U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_dst_wr_en) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (8U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_dst_wr_en) 
             & (0U != ((((0x3f800000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[0U]) 
                         | (0x40000000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[1U])) 
                        | (0x40800000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[2U])) 
                       | (0x41000000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data[3U]))))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x10U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wvalid) 
              & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready)) 
             & (0U != ((((0x3f800000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[0U]) 
                         | (0x40000000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[1U])) 
                        | (0x40800000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[2U])) 
                       | (0x41000000U ^ vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[3U]))))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x20U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x40U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x80U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x100U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_valid) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_ready)) 
              & (0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_addr))) 
             & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_data)) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x200U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_valid) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x400U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__core_job_ready) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x800U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_desc_ready) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x1000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x2000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) 
              & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready)) 
             & (2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x4000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x8000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x10000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_done) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x20000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_opcode) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_write32_opcode)) 
              | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_opcode)) 
             | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_length))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x40000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) 
              & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready)) 
             & (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x80000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x100000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x200000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((7U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x400000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x800000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x1000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x2000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__core_job_ready) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x4000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((3U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x8000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_replay) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x10000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start_delayed))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x20000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x40000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
        if ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__state))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky 
                = (0x80000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky);
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_d = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_d = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_resp = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[0U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[1U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[2U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[3U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_resp = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_resp = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__in_range = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_sync = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__tag = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len = 1U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left = 1U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit = 4U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__awlen_r = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_tag = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_align = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq2_wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq2_wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rgray = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit = 8U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_left = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len = 1U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__arlen_r = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_tag = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_align = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_resp = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_addr_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__done = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_align = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_sync = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__err_w_owner = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky = 0U;
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        if (VL_UNLIKELY(((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire) 
                         | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done)))) {
            VL_WRITEF("JOBEDGE_DBG t=%0t job_fire=%0b job_done=%0b job_active=%0b job_busy=%0b job_id_reg=%0# ingress_job_id=%0#\n",
                      64,VL_TIME_UNITED_Q(1000),-9,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy,
                      32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id_reg,
                      32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_id);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire) {
            if (VL_UNLIKELY((0x66U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_id))) {
                VL_WRITEF("JOB2_CBASECAP t=%0t job_fire=%0b ingress_job_id=%0# ingress_c_base=0x%08x c_base_reg_before=0x%08x\n",
                          64,VL_TIME_UNITED_Q(1000),
                          -9,1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire),
                          32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_id,
                          64,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_c_base,
                          64,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg);
            }
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy = 1U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active = 1U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg 
                = (1U > vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id);
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg 
                = ((1U <= vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id) 
                   & (4U > vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id));
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base;
        }
        if (VL_UNLIKELY(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done)) {
            VL_WRITEF("JOBCLOSE_DBG t=%0t job_id=%0# job_done=%0b job_active=%0b job_busy=%0b\n",
                      64,VL_TIME_UNITED_Q(1000),-9,
                      32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id_reg,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done),
                      1,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active,
                      1,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy));
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy = 0U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active = 0U;
        }
    } else {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg = 1U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles_reg = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base_reg = 0ULL;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base_reg = 0ULL;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg = 0ULL;
    }
    __Vtableidx2 = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done) 
                     << 5U) | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done) 
                                << 4U) | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done) 
                                           << 3U) | 
                                          (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done) 
                                            << 2U) 
                                           | (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear) 
                                               << 1U) 
                                              | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n))))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_h7a124f99_0
         [__Vtableidx2])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done_sticky 
            = Vtb_systolic_dma_top__ConstPool__TABLE_h8c101dee_0
            [__Vtableidx2];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_h7a124f99_0
         [__Vtableidx2])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_sticky 
            = Vtb_systolic_dma_top__ConstPool__TABLE_h09c976a5_0
            [__Vtableidx2];
    }
    if ((4U & Vtb_systolic_dma_top__ConstPool__TABLE_h7a124f99_0
         [__Vtableidx2])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fold_done_sticky 
            = Vtb_systolic_dma_top__ConstPool__TABLE_hd80ff9dd_0
            [__Vtableidx2];
    }
    if ((8U & Vtb_systolic_dma_top__ConstPool__TABLE_h7a124f99_0
         [__Vtableidx2])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done_sticky 
            = Vtb_systolic_dma_top__ConstPool__TABLE_h109f5694_0
            [__Vtableidx2];
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__folds_done 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__folds_done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_running 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_running;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_running 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_running;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fill_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_running 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_running;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__t_span;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_c 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_c;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_wr 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__want_wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__chk_c;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_active;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__rb_pending;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id_reg 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_id_reg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_busy;
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__done = 0U;
        if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state))) {
            if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state))) {
                if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_bvalid) {
                    if ((1U == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left)) {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__done = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 0U;
                    } else {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left 
                            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left 
                               - (IData)(1U));
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr 
                            = (0x1fffffffU & ((IData)(0x100U) 
                                              + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr));
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid = 1U;
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 1U;
                    }
                }
            } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wready) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_idx 
                    = ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_idx);
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase 
                    = (0x7fU & ((0x7fU <= (0xffU & 
                                           ((IData)(4U) 
                                            + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase))))
                                 ? ((IData)(5U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase))
                                 : ((IData)(4U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase))));
                if ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left))) {
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid = 0U;
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast = 0U;
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 3U;
                } else {
                    __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left 
                        = (0xffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left) 
                                    - (IData)(1U)));
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast 
                        = (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left));
                }
            }
        } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state))) {
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awready) {
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left = 0xfU;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid = 0U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid = 1U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast = 0U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 2U;
            }
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start) {
            if ((0U != (0xffU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_slab_addr))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_align = 1U;
            } else {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_slab_addr;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left = 8U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_idx = 0U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase = 0U;
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid = 1U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 1U;
            }
        }
    } else {
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_idx = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__done = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_align = 0U;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_rdy_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_aw_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in9 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in5 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_busy_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_r_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in14 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_w_stall_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_busy_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_starve_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_full 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy) 
           & (3U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in1 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_val_d 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) 
           && ((5U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
               & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__walmost_full 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__walmost_full_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__walmost_full 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__walmost_full_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_col16 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_d;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_row8 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_d;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_resp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_resp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__word_sel 
        = (((0U == (0x1fU & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt), 5U)))
             ? 0U : (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[
                     (((IData)(0x1fU) + (0x7fU & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt), 5U))) 
                      >> 5U)] << ((IData)(0x20U) - 
                                  (0x1fU & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt), 5U))))) 
           | (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q[
              (3U & (VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt), 5U) 
                     >> 5U))] >> (0x1fU & VL_SHIFTL_III(7,7,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt), 5U))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_err_resp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_resp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_err_resp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_resp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rempty 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rempty 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wr_err_range 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in19 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_sync;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__mat 
        = (7U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                 >> 4U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__w 
        = ((0x1fcU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                      << 2U)) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__win 
        = (3U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                 >> 5U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_lane 
        = (7U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                 >> 1U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_koff 
        = ((4U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                  << 2U)) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__is_b 
        = (1U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q) 
                 >> 4U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__awlen_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wlast 
        = (1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_after 
        = (0xffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left) 
                      - (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_err_align 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_align;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_next 
        = (0x1fffffffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r 
                          + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len), 4U)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__remaining 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__a 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__to_page 
        = ((IData)(0x100U) - (0xffU & (__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__a 
                                       >> 4U)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim = 0x10U;
    if ((0x10U > __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__remaining)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__remaining;
    }
    if ((__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__to_page 
         < __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__to_page;
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__Vfuncout 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__lim;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__len_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__21__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid 
        = ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state)) 
           & (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rgray;
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x1fU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | (0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g)));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout) 
                  >> 5U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g) 
                            >> 4U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x2fU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0) 
              << 4U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout) 
                  >> 4U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g) 
                            >> 3U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x37U & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0) 
              << 3U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout) 
                  >> 3U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g) 
                            >> 2U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x3bU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0) 
              << 2U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout) 
                  >> 2U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g) 
                            >> 1U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x3dU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0) 
              << 1U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout) 
                  >> 1U) ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__g)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout 
        = ((0x3eU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout)) 
           | (IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT____Vlvbound_h176fdb38__0));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rbin 
        = vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rgray;
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x1fU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | (0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g)));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout) 
                  >> 5U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g) 
                            >> 4U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x2fU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0) 
              << 4U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout) 
                  >> 4U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g) 
                            >> 3U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x37U & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0) 
              << 3U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout) 
                  >> 3U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g) 
                            >> 2U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x3bU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0) 
              << 2U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout) 
                  >> 2U) ^ ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g) 
                            >> 1U)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x3dU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | ((IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0) 
              << 1U));
    tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0 
        = (1U & (((IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout) 
                  >> 1U) ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__g)));
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout 
        = ((0x3eU & (IData)(vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout)) 
           | (IData)(tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT____Vlvbound_h176fdb38__0));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rbin 
        = vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_tag 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_tag;
    if ((1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear)))) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_fi = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_fold = 0U;
    } else {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_fi 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__filled_fi;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_fold = 0U;
        } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid) 
                    & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_fold = 0U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_fold = 1U;
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid) 
           & (0x3bU == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_tag)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_beat 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__arlen_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_araddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_err_resp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_resp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_err_align 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_align;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_next 
        = (0x1fffffffU & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r 
                          + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len), 4U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_after 
        = (0xffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_left) 
                      - (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_tag 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_addr_reg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_align 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_align;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wlast 
        = ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state)) 
           & (0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy 
        = (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in18 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_sync;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in17 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__result_readback_busy 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active) 
           | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_region_base 
        = (0x1fffffffU & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__k 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__Vfuncout 
        = ((0x10U == __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__k)
            ? 0x3f880780U : ((0x20U == __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__k)
                              ? 0x805c1f00U : 0U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__active_expected_wr_chk 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__expected_wr_chk_for_k__8__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_bytes 
        = VL_SHIFTL_III(32,32,32, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg, 6U);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__k_dim 
        = (0x3fU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_bank 
        = (1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_fi));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_busy 
        = (0U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wlast 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[0U] = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[1U] = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[2U] = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[3U] = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase;
    if (VL_LTES_III(32, 0x7fU, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [0U])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[0U] 
            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
               [0U] - (IData)(0x7fU));
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v 
        = (0x7fU & ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [0U]));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e 
        = ((0x40U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
            ? 6U : ((0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                     ? 5U : ((0x10U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                              ? 4U : ((8U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                       ? 3U : ((4U 
                                                & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                ? 2U
                                                : (
                                                   (2U 
                                                    & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                    ? 1U
                                                    : 0U))))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted 
        = ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v) 
           << (0x1fU & ((IData)(0x17U) - (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout 
        = (VL_SHIFTL_III(32,32,32, ((IData)(0x7fU) 
                                    + (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e)), 0x17U) 
           | (0x7fffffU & __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[0U] 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[1U] 
        = ((IData)(1U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase));
    if (VL_LTES_III(32, 0x7fU, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [1U])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[1U] 
            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
               [1U] - (IData)(0x7fU));
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v 
        = (0x7fU & ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [1U]));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e 
        = ((0x40U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
            ? 6U : ((0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                     ? 5U : ((0x10U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                              ? 4U : ((8U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                       ? 3U : ((4U 
                                                & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                ? 2U
                                                : (
                                                   (2U 
                                                    & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                    ? 1U
                                                    : 0U))))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted 
        = ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v) 
           << (0x1fU & ((IData)(0x17U) - (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout 
        = (VL_SHIFTL_III(32,32,32, ((IData)(0x7fU) 
                                    + (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e)), 0x17U) 
           | (0x7fffffU & __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[1U] 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[2U] 
        = ((IData)(2U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase));
    if (VL_LTES_III(32, 0x7fU, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [2U])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[2U] 
            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
               [2U] - (IData)(0x7fU));
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v 
        = (0x7fU & ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [2U]));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e 
        = ((0x40U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
            ? 6U : ((0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                     ? 5U : ((0x10U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                              ? 4U : ((8U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                       ? 3U : ((4U 
                                                & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                ? 2U
                                                : (
                                                   (2U 
                                                    & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                    ? 1U
                                                    : 0U))))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted 
        = ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v) 
           << (0x1fU & ((IData)(0x17U) - (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout 
        = (VL_SHIFTL_III(32,32,32, ((IData)(0x7fU) 
                                    + (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e)), 0x17U) 
           | (0x7fffffU & __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[2U] 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[3U] 
        = ((IData)(3U) + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase));
    if (VL_LTES_III(32, 0x7fU, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [3U])) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[3U] 
            = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
               [3U] - (IData)(0x7fU));
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v 
        = (0x7fU & ((IData)(1U) + vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum
                    [3U]));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e 
        = ((0x40U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
            ? 6U : ((0x20U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                     ? 5U : ((0x10U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                              ? 4U : ((8U & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                       ? 3U : ((4U 
                                                & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                ? 2U
                                                : (
                                                   (2U 
                                                    & (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v))
                                                    ? 1U
                                                    : 0U))))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted 
        = ((IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__v) 
           << (0x1fU & ((IData)(0x17U) - (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e))));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout 
        = (VL_SHIFTL_III(32,32,32, ((IData)(0x7fU) 
                                    + (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__e)), 0x17U) 
           | (0x7fffffU & __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__shifted));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[3U] 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__fp32_small__15__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_err_align 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_align;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__any_err 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_align) 
           | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_resp) 
              | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_align) 
                 | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_resp) 
                    | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range) 
                       | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_align) 
                          | ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__err_w_owner) 
                             | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_resp))))))));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wbin 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wbin_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wbin 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wbin_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_rd 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C
            [vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_i]
            [vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_i];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_meta 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq1_wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq1_wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq1_rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq1_rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_meta 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_replay = 0U;
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_request) {
            if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = 0U;
            }
            if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = 1U;
            }
            if ((4U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = 2U;
            }
            if ((8U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = 3U;
            }
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending = 1U;
        } else if (((3U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
                    & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_replay = 1U;
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending = 0U;
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wbin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wbin = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_rd = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_meta = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq1_wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq1_wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq1_rgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq1_rgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_meta = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_replay = 0U;
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_wb_beats = 0x10U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_words = 0x40U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_bytes = 0x100U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats = 0x10U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid 
            = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_r)));
    } else {
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_wb_beats = 4U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_words = 0x10U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_bytes = 0x40U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats = 4U;
        } else {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_wb_beats = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_words = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_bytes = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats = 0U;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid 
            = (1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_r)) 
                     & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg)));
    }
    if ((1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear)))) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__filled_fi = 0U;
    } else {
        if ((6U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w = 1U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__filled_fi 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi;
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_tag 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending)
            ? 0x3cU : 0x3bU);
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fsm_fold_start = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid = 0U;
        if ((1U & (~ ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase) 
                      >> 3U)))) {
            if ((4U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                    if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_pulse) 
                             | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire))) {
                            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi = 0U;
                            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi = 0U;
                            vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 0U;
                        }
                    } else {
                        if ((1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_started)))) {
                            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid = 1U;
                        }
                        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done) {
                            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last) {
                                vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 7U;
                            } else {
                                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi 
                                    = (0xfU & ((IData)(1U) 
                                               + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi)));
                                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi 
                                    = (0xfU & ((IData)(1U) 
                                               + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi)));
                                vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase 
                                    = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready) 
                                        | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete))
                                        ? 3U : 2U);
                            }
                        }
                    }
                } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last) {
                        vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 6U;
                    } else {
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c 
                            = (0x7fU & ((IData)(1U) 
                                        + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c)));
                    }
                } else if (VL_UNLIKELY(((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected)) 
                                        & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold)))) {
                    VL_WRITEF("CDBG C00=%x C01=%x C10=%x C77=%x\n",
                              32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C
                              [0U][0U],32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C
                              [0U][1U],32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C
                              [1U][0U],32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C
                              [7U][7U]);
                    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c = 0U;
                    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 5U;
                }
            } else if ((2U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                    if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending) 
                         | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected))) {
                        vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 4U;
                    }
                } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready) 
                            | ((vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_written 
                                == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_want) 
                               & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active))))) {
                    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__reads_complete) {
                        vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 3U;
                    }
                }
            } else if ((1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
                if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done) {
                    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi_last) {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi = 0U;
                        vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 2U;
                    } else {
                        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi 
                            = (0xfU & ((IData)(1U) 
                                       + (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi)));
                        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start = 1U;
                    }
                }
            } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__init_calib_complete) 
                        & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv 
                    = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv_next;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi = 0U;
                __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi = 0U;
                vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 2U;
            }
        }
        if (((((((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
                 | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_pipeline_started)) 
                & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_started))) 
               & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active))) 
              & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready))) 
             & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__reads_complete)))) {
            __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid = 1U;
        }
    } else {
        vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv = 1U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fsm_fold_start = 0U;
        __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c = 0U;
    }
    if ((1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)) 
               | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear)))) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__reads_complete = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_started = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_pipeline_started = 0U;
    } else {
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi_last))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__reads_complete = 1U;
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_started = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active = 0U;
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready = 1U;
        } else {
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_ready))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_started = 1U;
            }
            if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid) 
                 & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_ready))) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active = 1U;
            }
            if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready = 0U;
            }
        }
        if ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_pipeline_started = 1U;
        }
    }
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray_next;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray_next;
        if ((6U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_started = 0U;
        } else if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid) 
                    & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_started = 1U;
        }
        if (((3U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
             | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold = 0U;
        } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold = 1U;
        }
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_started = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold = 0U;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in7 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_rdy_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in11 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_aw_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in6 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in8 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_r_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in12 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_w_stall_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in10 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_busy_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in13 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_starve_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_almost_full 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_full;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_full 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_full;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_almost_full 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_full;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpos 
        = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_row8) 
            << 0x10U) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_col16));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__err_resp 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_resp;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wdata 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__word_sel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wfull;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wfull;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wfull;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wfull;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rempty;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rempty;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_koff 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_lane;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_lane 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_koff;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr 
        = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy) 
            & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__in_range)) 
           & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__is_b)));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__is_b) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr 
            = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__in_range));
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wsel 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_koff;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__waddr 
            = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__win) 
                << 3U) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_lane));
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wsel 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_lane;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__waddr 
            = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__win) 
                << 3U) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_koff));
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlen;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wlast 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wlast;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awaddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_beat;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arlen;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__araddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_araddr;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__remaining 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_after;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__a 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_next;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__to_page 
        = ((IData)(0x100U) - (0xffU & (__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__a 
                                       >> 4U)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim = 0x10U;
    if ((0x10U > __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__remaining)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__remaining;
    }
    if ((__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__to_page 
         < __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__to_page;
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__Vfuncout 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__lim;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__len_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__17__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_tag 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_tag;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__err_align 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_align;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awaddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wlast 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wlast;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wlast 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wlast;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awready) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_tag 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_tag;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_is_readback 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_words 
        = VL_SHIFTR_III(32,32,32, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_bytes, 2U);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_beats 
        = VL_SHIFTR_III(32,32,32, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_bytes, 4U);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__k_dim 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__k_dim;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_beats 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid 
        = ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__read_fi;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__busy 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_busy;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wdata[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wdata[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wdata[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wdata[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__fi;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__job_active;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_fi;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr 
        = (0x1fffffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg)
                           ? (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_region_base 
                              + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi), 8U))
                           : ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg)
                               ? (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_region_base 
                                  + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi), 6U))
                               : vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_region_base)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wdata;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__a_wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__bank8 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__k16 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_wr_beat 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arlen;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_araddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__araddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_ready 
        = ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wready));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awaddr 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlen 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awlen;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awsize 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awsize;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awburst 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awburst;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlock 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awlock;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awcache 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awcache;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awprot 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awprot;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awqos 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awqos;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wstrb 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wstrb;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wlast 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wlast;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bready 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_bready;
    } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awaddr 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awaddr;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlen 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlen;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awsize 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awsize;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awburst 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awburst;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlock 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlock;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awcache 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awcache;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awprot 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awprot;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awqos 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awqos;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wstrb 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wstrb;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wlast 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wlast;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bready 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_bready;
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awaddr 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlen 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awlen;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awsize 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awsize;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awburst 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awburst;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlock 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awlock;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awcache 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awcache;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awprot 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awprot;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awqos 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awqos;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wstrb 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wstrb;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wlast 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bready 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_bready;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc1c513c0__0 
        = (1U & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy)) 
                 & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc50c9b32__0 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_beats 
        = (0xffffU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_beats);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_slab_addr 
        = (0x1fffffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base_reg) 
                          + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi), 0xbU)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank 
        = (1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last 
        = (0x40U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_i 
        = (7U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c) 
                 >> 3U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_i 
        = (7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_slab_addr 
        = (0x1fffffffU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base_reg) 
                          + VL_SHIFTL_III(29,29,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi), 0xbU)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in16 
        = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__folds_done) 
            << 8U) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi_last 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi) 
           == (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv) 
                       - (IData)(1U))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi) 
           == (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv) 
                       - (IData)(1U))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wdata 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wdata 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wdata 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wdata 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wsel 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wsel 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wsel 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wsel 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wpos 
        = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__bank8) 
            << 0x10U) | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__k16));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__waddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__waddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__waddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__waddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awaddr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awaddr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awlen 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlen;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awsize 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awsize;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awburst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awburst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awlock 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlock;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awcache 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awcache;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awprot 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awprot;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awqos 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awqos;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wstrb 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wstrb;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wlast 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wlast;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_bready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc1c513c0__0) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc1c513c0__0) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc50c9b32__0) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc50c9b32__0) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_slab_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_0__wr 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_1__wr 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_0__wr 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_1__wr 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__base_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_slab_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active) 
           & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid) 
              & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__remaining 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__a 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__to_page 
        = ((IData)(0x100U) - (0xffU & (__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__a 
                                       >> 4U)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim = 0x10U;
    if ((0x10U > __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__remaining)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__remaining;
    }
    if ((__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__to_page 
         < __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__to_page;
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__Vfuncout 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__lim;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__first_len 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__next_len__20__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi_last 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi) 
           == (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv) 
                       - (IData)(1U))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__desc_valid;
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_addr_reg;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending;
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_beats;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_slab_addr;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid 
        = __Vdly__tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_fire 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awready) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_fire 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_ready 
        = ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_beats 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__remaining 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__a 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__to_page 
        = ((IData)(0x100U) - (0xffU & (__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__a 
                                       >> 4U)));
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim = 0x10U;
    if ((0x10U > __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__remaining)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__remaining;
    }
    if ((__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__to_page 
         < __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim)) {
        __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim 
            = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__to_page;
    }
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__Vfuncout 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__lim;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__first_len 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__next_len__16__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_0__wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_1__wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_0__wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_1__wr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_ready;
    tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc5d6486b__0 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_ready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rd_en 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg) 
           & (IData)(tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc5d6486b__0));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rd_en 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg) 
           & (IData)(tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc5d6486b__0));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rd_en 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rd_en;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin_next 
        = (0x3fU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin) 
                    + ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_r)) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rd_en))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rd_en 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rd_en;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin_next 
        = (0x3fU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin) 
                    + ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_r)) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rd_en))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray_next 
        = (0x3fU & (VL_SHIFTR_III(6,6,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin_next), 1U) 
                    ^ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin_next)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray_next 
        = (0x3fU & (VL_SHIFTR_III(6,6,32, (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin_next), 1U) 
                    ^ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin_next)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_val 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray_next) 
           == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq2_wgray));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_val 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray_next) 
           == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq2_wgray));
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__71(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__71\n"); );
    // Body
    if (VL_UNLIKELY(((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase) 
                     != (IData)(vlSelf->tb_systolic_dma_top__DOT__phase_d)))) {
        VL_WRITEF("  phase %0# -> %0#   words=%0#\n",
                  4,vlSelf->tb_systolic_dma_top__DOT__phase_d,
                  4,(IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase),
                  32,vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_written);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpu_resetn 
        = vlSelf->tb_systolic_dma_top__DOT__rstn;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_valid 
        = vlSelf->tb_systolic_dma_top__DOT__job_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id 
        = vlSelf->tb_systolic_dma_top__DOT__job_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id 
        = vlSelf->tb_systolic_dma_top__DOT__job_device_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m 
        = vlSelf->tb_systolic_dma_top__DOT__job_m;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n 
        = vlSelf->tb_systolic_dma_top__DOT__job_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k 
        = vlSelf->tb_systolic_dma_top__DOT__job_k;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__job_est_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_a_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_b_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base 
        = vlSelf->tb_systolic_dma_top__DOT__job_c_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_rst_n 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__LOCKED) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__rstn));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__sys_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_rst_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_probe 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1;
    vlSelf->tb_systolic_dma_top__DOT__phase_d = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__72(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__72\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_clk_pin 
        = vlSelf->tb_systolic_dma_top__DOT__clk;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_ibuf__DOT__I 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_clk_pin;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_clkout 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_clk;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__clk;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__clk;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_clk;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_clk 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_clk;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__0(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__0\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_valid) 
           | (IData)(vlSelf->tb_systolic_dma_top__DOT__job_valid));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_valid) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_m;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_n;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_k;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_a_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_b_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_c_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_device_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_start_cycle;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_est_cycles;
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id 
            = vlSelf->tb_systolic_dma_top__DOT__job_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m 
            = vlSelf->tb_systolic_dma_top__DOT__job_m;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n 
            = vlSelf->tb_systolic_dma_top__DOT__job_n;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k 
            = vlSelf->tb_systolic_dma_top__DOT__job_k;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_a_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_b_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base 
            = vlSelf->tb_systolic_dma_top__DOT__job_c_base;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
            = vlSelf->tb_systolic_dma_top__DOT__job_device_id;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle 
            = vlSelf->tb_systolic_dma_top__DOT__job_start_cycle;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles 
            = vlSelf->tb_systolic_dma_top__DOT__job_est_cycles;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_job_id 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_m 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_n 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_k 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_a_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_b_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_c_base 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_accelerator_id 
        = (3U & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id 
                 >> 0U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_device_id 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_start_cycle 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_compute_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_est_cycles 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__1(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__1\n"); );
    // Init
    QData/*39:0*/ tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT____VdfgTmp_hd8ac41bd__0;
    tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT____VdfgTmp_hd8ac41bd__0 = 0;
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][3U];
    tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT____VdfgTmp_hd8ac41bd__0 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem
        [(3U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_bin))];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem
        [(7U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin))][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_addr 
        = (0xffU & (IData)((tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT____VdfgTmp_hd8ac41bd__0 
                            >> 0x20U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_data 
        = (IData)(tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT____VdfgTmp_hd8ac41bd__0);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_header_addr 
        = (0x1fffffffU & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[4U]);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data[4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_is_header 
        = (1U & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data[4U] 
                 >> 0x1dU));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_addr 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_addr;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wdata[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wdata[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wdata[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wdata[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wdata[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wdata[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wdata[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wdata[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_is_header)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_valid));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__73(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__73\n"); );
    // Init
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__Vfuncout = 0;
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__bin;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__bin = 0;
    CData/*2:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__Vfuncout = 0;
    CData/*2:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__bin;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__bin = 0;
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid) 
           & ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner)) 
              | ((0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner)) 
                 & (2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data)))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_opcode 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_opcode;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_write32_opcode 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__err_opcode;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity 
        = vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity;
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug = 0U;
    } else {
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray_next;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray_next;
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray 
                = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray_next;
        }
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug 
            = (1U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug);
        if ((1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dpti_rd_n)))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug 
                = (4U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug);
        }
        if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug 
                = (8U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug);
        }
        if (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid) 
             & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready))) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug 
                = (0x10U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug);
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_write32_opcode 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_write32_opcode;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_data 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_data;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dpti_rd_n = (1U 
                                                   & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__fifo_full 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray) 
           == ((6U & ((~ ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr2) 
                          >> 1U)) << 1U)) | (1U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr2))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__fifo_full 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray) 
           == ((0xcU & ((~ ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2) 
                            >> 2U)) << 2U)) | (3U & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__fifo_empty 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray) 
           == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd2));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rd_n 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_rd_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_n 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_rd_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst 
        = ((1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n))) 
           || (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst_meta));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__fifo_full)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__fifo_full)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__fifo_empty)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_wr_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin_next 
        = (7U & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin) 
                 + (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready) 
                     & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid))
                     ? 1U : 0U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready 
        = (1U & (~ ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst) 
                    | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst_meta 
        = (1U & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_ready;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__bin 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin_next;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__Vfuncout 
        = (7U & (VL_SHIFTR_III(3,3,32, (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__bin), 1U) 
                 ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__bin)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__bin_to_gray__13__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next 
        = (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin) 
                   + (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready))
                       ? 1U : 0U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__rst_n 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n) 
           & ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dpti_txe_n)) 
              & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n) 
           & (~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dpti_txe_n)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_rst 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_rst;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_ready;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__bin 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__Vfuncout 
        = (0xfU & (VL_SHIFTR_III(4,4,32, (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__bin), 1U) 
                   ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__bin)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__bin_to_gray__10__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_d_oe 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe;
    vlSelf->tb_systolic_dma_top__DOT__dpti_d = (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe)
                                                  ? 0xffU
                                                  : 0U) 
                                                & (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe)
                                                     ? (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data)
                                                     : 0U) 
                                                   & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe)
                                                       ? 0xffU
                                                       : 0U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready;
    vlSelf->tb_systolic_dma_top__DOT__dpti_wr_n = (1U 
                                                   & (~ 
                                                      ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy) 
                                                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_d 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_d;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_n 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_wr_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_wr_n 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_wr_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_wr_n 
        = vlSelf->tb_systolic_dma_top__DOT__dpti_wr_n;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_d_in 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_d;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__5(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__5\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[0U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [0U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[1U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [1U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[2U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [2U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[3U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [3U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[4U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [4U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[5U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [5U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[6U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [6U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[7U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc
        [7U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[0U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [0U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[1U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [1U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[2U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [2U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[3U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [3U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[4U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [4U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[5U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [5U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[6U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [6U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[7U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out
        [7U][7U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__6(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__6\n"); );
    // Init
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__Vfuncout = 0;
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__bin;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__bin = 0;
    // Body
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[0U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[1U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[2U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[3U] = 0U;
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[4U] 
            = (0x20000000U | vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_addr);
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[0U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[0U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[1U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[1U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[2U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[2U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[3U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data[3U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[4U] = 0U;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid) 
           | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready 
        = ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner))
            ? (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready)
            : ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner))
                ? (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready)
                : ((1U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data))
                    ? (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready)
                    : ((2U != (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data)) 
                       | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready)))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data[4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data[4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin_next 
        = (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin) 
                   + (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid))
                       ? 1U : 0U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__bin 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin_next;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__Vfuncout 
        = (0xfU & (VL_SHIFTR_III(4,4,32, (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__bin), 1U) 
                   ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__bin)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__11__Vfuncout;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__75(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__75\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_opcode 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_opcode;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_length 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_length;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__start;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_mem_opcode 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_opcode;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_mem_length 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_length;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_done 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_done;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_valid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_valid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_start;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__8(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__8\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_pulse 
        = ((~ (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_d)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg));
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_sequent__TOP__76(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_sequent__TOP__76\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_written 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase 
        = vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_8x8_idx 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id_reg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_4x4_idx 
        = (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id_reg 
           - (IData)(1U));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in3 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_written;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_8x8 
        = ((6U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_4x4 
        = ((6U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_window 
        = ((3U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_match 
        = ((7U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase)) 
           & (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c 
              == vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_c));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear 
        = (0U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_8x8;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__start 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_4x4;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_ready 
        = (3U & (- (IData)((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_window))));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__stat_clear 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__stat_clear 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__clear 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__accelerator_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_ready;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__9(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__9\n"); );
    // Init
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__Vfuncout;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__Vfuncout = 0;
    CData/*3:0*/ __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__bin;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__bin = 0;
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_ready 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_valid) 
           & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_is_header) 
              | (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_ready)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid 
        = ((2U == (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state)) 
           & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid));
    if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[0U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[0U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[1U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[1U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[2U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[2U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[3U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata[3U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid;
    } else if (vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[0U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[0U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[1U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[1U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[2U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[2U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[3U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata[3U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid;
    } else {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[0U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[0U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[1U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[1U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[2U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[2U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[3U] 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata[3U];
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wvalid 
            = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid;
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ready 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_ready;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next 
        = (0xfU & ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin) 
                   + (((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_valid) 
                       & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_ready))
                       ? 1U : 0U)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wdata[0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wdata[1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wdata[2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wdata[3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi[3U];
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__bin 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next;
    __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__Vfuncout 
        = (0xfU & (VL_SHIFTR_III(4,4,32, (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__bin), 1U) 
                   ^ (IData)(__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__bin)));
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_next 
        = __Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__bin_to_gray__12__Vfuncout;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wvalid 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wvalid;
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__10(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__10\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[0U][3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out
        [3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__11(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__11\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[1U][3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out
        [3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__12(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__12\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[2U][3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out
        [3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__13(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__13\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][0U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [0U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][1U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [1U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][2U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [2U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][3U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [3U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][4U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [4U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][5U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [5U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][6U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [6U][7U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][0U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][0U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][1U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][1U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][2U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][2U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][3U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][3U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][4U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][4U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][5U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][5U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][6U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][6U];
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[0U][7U][7U] 
        = vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out
        [7U][7U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__16(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__16\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_4x4_selected 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg) 
           & ((2U >= (3U & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_4x4_idx)) 
              && vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_4x4
              [(3U & vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_4x4_idx)]));
}

VL_INLINE_OPT void Vtb_systolic_dma_top___024root___nba_comb__TOP__19(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___nba_comb__TOP__19\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_selected 
        = ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg)
            ? (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_8x8_selected)
            : ((IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg) 
               & (IData)(vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_4x4_selected)));
}

void Vtb_systolic_dma_top___024root___timing_resume(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___timing_resume\n"); );
    // Body
    if ((1ULL & vlSelf->__VactTriggered.word(0U))) {
        vlSelf->__VtrigSched_h2466d03b__0.resume("@(posedge tb_systolic_dma_top.clk)");
    }
    if ((0x100000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        vlSelf->__VdlySched.resume();
    }
}

void Vtb_systolic_dma_top___024root___timing_commit(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___timing_commit\n"); );
    // Body
    if ((! (1ULL & vlSelf->__VactTriggered.word(0U)))) {
        vlSelf->__VtrigSched_h2466d03b__0.commit("@(posedge tb_systolic_dma_top.clk)");
    }
}

void Vtb_systolic_dma_top___024root___eval_triggers__act(Vtb_systolic_dma_top___024root* vlSelf);
void Vtb_systolic_dma_top___024root___eval_act(Vtb_systolic_dma_top___024root* vlSelf);

bool Vtb_systolic_dma_top___024root___eval_phase__act(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_phase__act\n"); );
    // Init
    VlTriggerVec<121> __VpreTriggered;
    CData/*0:0*/ __VactExecute;
    // Body
    Vtb_systolic_dma_top___024root___eval_triggers__act(vlSelf);
    Vtb_systolic_dma_top___024root___timing_commit(vlSelf);
    __VactExecute = vlSelf->__VactTriggered.any();
    if (__VactExecute) {
        __VpreTriggered.andNot(vlSelf->__VactTriggered, vlSelf->__VnbaTriggered);
        vlSelf->__VnbaTriggered.thisOr(vlSelf->__VactTriggered);
        Vtb_systolic_dma_top___024root___timing_resume(vlSelf);
        Vtb_systolic_dma_top___024root___eval_act(vlSelf);
    }
    return (__VactExecute);
}

void Vtb_systolic_dma_top___024root___eval_nba(Vtb_systolic_dma_top___024root* vlSelf);

bool Vtb_systolic_dma_top___024root___eval_phase__nba(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_phase__nba\n"); );
    // Init
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = vlSelf->__VnbaTriggered.any();
    if (__VnbaExecute) {
        Vtb_systolic_dma_top___024root___eval_nba(vlSelf);
        vlSelf->__VnbaTriggered.clear();
    }
    return (__VnbaExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__ico(Vtb_systolic_dma_top___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__nba(Vtb_systolic_dma_top___024root* vlSelf);
#endif  // VL_DEBUG
#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__act(Vtb_systolic_dma_top___024root* vlSelf);
#endif  // VL_DEBUG

void Vtb_systolic_dma_top___024root___eval(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval\n"); );
    // Init
    IData/*31:0*/ __VicoIterCount;
    CData/*0:0*/ __VicoContinue;
    IData/*31:0*/ __VnbaIterCount;
    CData/*0:0*/ __VnbaContinue;
    // Body
    __VicoIterCount = 0U;
    vlSelf->__VicoFirstIteration = 1U;
    __VicoContinue = 1U;
    while (__VicoContinue) {
        if (VL_UNLIKELY((0x64U < __VicoIterCount))) {
#ifdef VL_DEBUG
            Vtb_systolic_dma_top___024root___dump_triggers__ico(vlSelf);
#endif
            VL_FATAL_MT("/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 43, "", "Input combinational region did not converge.");
        }
        __VicoIterCount = ((IData)(1U) + __VicoIterCount);
        __VicoContinue = 0U;
        if (Vtb_systolic_dma_top___024root___eval_phase__ico(vlSelf)) {
            __VicoContinue = 1U;
        }
        vlSelf->__VicoFirstIteration = 0U;
    }
    __VnbaIterCount = 0U;
    __VnbaContinue = 1U;
    while (__VnbaContinue) {
        if (VL_UNLIKELY((0x64U < __VnbaIterCount))) {
#ifdef VL_DEBUG
            Vtb_systolic_dma_top___024root___dump_triggers__nba(vlSelf);
#endif
            VL_FATAL_MT("/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 43, "", "NBA region did not converge.");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        __VnbaContinue = 0U;
        vlSelf->__VactIterCount = 0U;
        vlSelf->__VactContinue = 1U;
        while (vlSelf->__VactContinue) {
            if (VL_UNLIKELY((0x64U < vlSelf->__VactIterCount))) {
#ifdef VL_DEBUG
                Vtb_systolic_dma_top___024root___dump_triggers__act(vlSelf);
#endif
                VL_FATAL_MT("/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 43, "", "Active region did not converge.");
            }
            vlSelf->__VactIterCount = ((IData)(1U) 
                                       + vlSelf->__VactIterCount);
            vlSelf->__VactContinue = 0U;
            if (Vtb_systolic_dma_top___024root___eval_phase__act(vlSelf)) {
                vlSelf->__VactContinue = 1U;
            }
        }
        if (Vtb_systolic_dma_top___024root___eval_phase__nba(vlSelf)) {
            __VnbaContinue = 1U;
        }
    }
}

#ifdef VL_DEBUG
void Vtb_systolic_dma_top___024root___eval_debug_assertions(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_debug_assertions\n"); );
}
#endif  // VL_DEBUG
