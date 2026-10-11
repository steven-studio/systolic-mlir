// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top___024root.h"

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_static__TOP(Vtb_systolic_dma_top___024root* vlSelf);

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_static(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_static\n"); );
    // Body
    Vtb_systolic_dma_top___024root___eval_static__TOP(vlSelf);
}

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_static__TOP(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_static__TOP\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__n_inv = 1U;
    vlSelf->tb_systolic_dma_top__DOT__clk = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dpti_clkout = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dpti_txe_n = 0U;
    vlSelf->tb_systolic_dma_top__DOT__rstn = 0U;
    vlSelf->tb_systolic_dma_top__DOT__job_valid = 0U;
    vlSelf->tb_systolic_dma_top__DOT__errors = 0U;
    vlSelf->tb_systolic_dma_top__DOT__waited = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hb_ui = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__errors = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__wb_seen = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__wb_region = 0x1000U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_open = 0U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__n_inv_arg = 1U;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg = 0U;
}

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_final(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_final\n"); );
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__stl(Vtb_systolic_dma_top___024root* vlSelf);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vtb_systolic_dma_top___024root___eval_phase__stl(Vtb_systolic_dma_top___024root* vlSelf);

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_settle(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_settle\n"); );
    // Init
    IData/*31:0*/ __VstlIterCount;
    CData/*0:0*/ __VstlContinue;
    // Body
    __VstlIterCount = 0U;
    vlSelf->__VstlFirstIteration = 1U;
    __VstlContinue = 1U;
    while (__VstlContinue) {
        if (VL_UNLIKELY((0x64U < __VstlIterCount))) {
#ifdef VL_DEBUG
            Vtb_systolic_dma_top___024root___dump_triggers__stl(vlSelf);
#endif
            VL_FATAL_MT("/home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/tb/tb_systolic_dma_top.sv", 43, "", "Settle region did not converge.");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
        __VstlContinue = 0U;
        if (Vtb_systolic_dma_top___024root___eval_phase__stl(vlSelf)) {
            __VstlContinue = 1U;
        }
        vlSelf->__VstlFirstIteration = 0U;
    }
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__stl(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VstlTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VstlTriggered.word(0U))) {
        VL_DBG_MSGF("         'stl' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_triggers__stl(Vtb_systolic_dma_top___024root* vlSelf);
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___eval_stl(Vtb_systolic_dma_top___024root* vlSelf);

VL_ATTR_COLD bool Vtb_systolic_dma_top___024root___eval_phase__stl(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___eval_phase__stl\n"); );
    // Init
    CData/*0:0*/ __VstlExecute;
    // Body
    Vtb_systolic_dma_top___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = vlSelf->__VstlTriggered.any();
    if (__VstlExecute) {
        Vtb_systolic_dma_top___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__ico(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___dump_triggers__ico\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VicoTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VicoTriggered.word(0U))) {
        VL_DBG_MSGF("         'ico' region trigger index 0 is active: Internal 'ico' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__act(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VactTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 0 is active: @(posedge tb_systolic_dma_top.clk)\n");
    }
    if ((2ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 1 is active: @(posedge tb_systolic_dma_top.dut.u_ibuf.O or negedge tb_systolic_dma_top.dut.ui_rst_n)\n");
    }
    if ((4ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 2 is active: @(posedge tb_systolic_dma_top.dpti_clkout)\n");
    }
    if ((8ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 3 is active: @(posedge tb_systolic_dma_top.dut.u_ibuf.O)\n");
    }
    if ((0x10ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 4 is active: @(posedge tb_systolic_dma_top.dpti_clkout or negedge tb_systolic_dma_top.dut.u_dpti_command_frontend.__Vcellinp__u_mem_write__rst_n)\n");
    }
    if ((0x20ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 5 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.clk)\n");
    }
    if ((0x40ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 6 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.clk)\n");
    }
    if ((0x80ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 7 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.clk)\n");
    }
    if ((0x100ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 8 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x200ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 9 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x400ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 10 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x800ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 11 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x1000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 12 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x2000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 13 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x4000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 14 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x8000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 15 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x10000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 16 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x20000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 17 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x40000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 18 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x80000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 19 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 20 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x200000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 21 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x400000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 22 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x800000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 23 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 24 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 25 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 26 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 27 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 28 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 29 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 30 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 31 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 32 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 33 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 34 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 35 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 36 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 37 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 38 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 39 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 40 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 41 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 42 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 43 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 44 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 45 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 46 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 47 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 48 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 49 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 50 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 51 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 52 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 53 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 54 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 55 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 56 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 57 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 58 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 59 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 60 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[4].u_pe.clk)\n");
    }
    if ((0x2000000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 61 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[5].u_pe.clk)\n");
    }
    if ((0x4000000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 62 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[6].u_pe.clk)\n");
    }
    if ((0x8000000000000000ULL & vlSelf->__VactTriggered.word(0U))) {
        VL_DBG_MSGF("         'act' region trigger index 63 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[7].u_pe.clk)\n");
    }
    if ((1ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 64 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((2ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 65 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((4ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 66 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((8ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 67 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x10ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 68 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[4].u_pe.clk)\n");
    }
    if ((0x20ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 69 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[5].u_pe.clk)\n");
    }
    if ((0x40ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 70 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[6].u_pe.clk)\n");
    }
    if ((0x80ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 71 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[7].u_pe.clk)\n");
    }
    if ((0x100ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 72 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x200ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 73 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x400ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 74 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x800ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 75 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x1000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 76 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[4].u_pe.clk)\n");
    }
    if ((0x2000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 77 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[5].u_pe.clk)\n");
    }
    if ((0x4000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 78 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[6].u_pe.clk)\n");
    }
    if ((0x8000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 79 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[7].u_pe.clk)\n");
    }
    if ((0x10000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 80 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x20000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 81 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x40000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 82 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x80000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 83 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 84 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[4].u_pe.clk)\n");
    }
    if ((0x200000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 85 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[5].u_pe.clk)\n");
    }
    if ((0x400000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 86 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[6].u_pe.clk)\n");
    }
    if ((0x800000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 87 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[7].u_pe.clk)\n");
    }
    if ((0x1000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 88 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 89 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 90 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 91 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 92 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[4].u_pe.clk)\n");
    }
    if ((0x20000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 93 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[5].u_pe.clk)\n");
    }
    if ((0x40000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 94 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[6].u_pe.clk)\n");
    }
    if ((0x80000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 95 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[7].u_pe.clk)\n");
    }
    if ((0x100000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 96 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 97 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 98 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 99 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 100 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[4].u_pe.clk)\n");
    }
    if ((0x2000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 101 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[5].u_pe.clk)\n");
    }
    if ((0x4000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 102 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[6].u_pe.clk)\n");
    }
    if ((0x8000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 103 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[7].u_pe.clk)\n");
    }
    if ((0x10000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 104 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 105 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 106 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 107 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 108 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[4].u_pe.clk)\n");
    }
    if ((0x200000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 109 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[5].u_pe.clk)\n");
    }
    if ((0x400000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 110 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[6].u_pe.clk)\n");
    }
    if ((0x800000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 111 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[7].u_pe.clk)\n");
    }
    if ((0x1000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 112 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 113 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 114 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 115 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 116 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[4].u_pe.clk)\n");
    }
    if ((0x20000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 117 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[5].u_pe.clk)\n");
    }
    if ((0x40000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 118 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[6].u_pe.clk)\n");
    }
    if ((0x80000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 119 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[7].u_pe.clk)\n");
    }
    if ((0x100000000000000ULL & vlSelf->__VactTriggered.word(1U))) {
        VL_DBG_MSGF("         'act' region trigger index 120 is active: @([true] __VdlySched.awaitingCurrentTime())\n");
    }
}
#endif  // VL_DEBUG

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtb_systolic_dma_top___024root___dump_triggers__nba(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___dump_triggers__nba\n"); );
    // Body
    if ((1U & (~ (IData)(vlSelf->__VnbaTriggered.any())))) {
        VL_DBG_MSGF("         No triggers active\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 0 is active: @(posedge tb_systolic_dma_top.clk)\n");
    }
    if ((2ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 1 is active: @(posedge tb_systolic_dma_top.dut.u_ibuf.O or negedge tb_systolic_dma_top.dut.ui_rst_n)\n");
    }
    if ((4ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 2 is active: @(posedge tb_systolic_dma_top.dpti_clkout)\n");
    }
    if ((8ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 3 is active: @(posedge tb_systolic_dma_top.dut.u_ibuf.O)\n");
    }
    if ((0x10ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 4 is active: @(posedge tb_systolic_dma_top.dpti_clkout or negedge tb_systolic_dma_top.dut.u_dpti_command_frontend.__Vcellinp__u_mem_write__rst_n)\n");
    }
    if ((0x20ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 5 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.clk)\n");
    }
    if ((0x40ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 6 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.clk)\n");
    }
    if ((0x80ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 7 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.clk)\n");
    }
    if ((0x100ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 8 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x200ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 9 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x400ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 10 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x800ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 11 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x1000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 12 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x2000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 13 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x4000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 14 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x8000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 15 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x10000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 16 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x20000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 17 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x40000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 18 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x80000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 19 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 20 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x200000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 21 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x400000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 22 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x800000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 23 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[0].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 24 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 25 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 26 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 27 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 28 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 29 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 30 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 31 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 32 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 33 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 34 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 35 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 36 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 37 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 38 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 39 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[1].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 40 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 41 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 42 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 43 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 44 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 45 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 46 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 47 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 48 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 49 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 50 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 51 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 52 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 53 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 54 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 55 is active: @(posedge tb_systolic_dma_top.dut.ACC_4X4[2].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 56 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 57 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 58 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 59 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 60 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[4].u_pe.clk)\n");
    }
    if ((0x2000000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 61 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[5].u_pe.clk)\n");
    }
    if ((0x4000000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 62 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[6].u_pe.clk)\n");
    }
    if ((0x8000000000000000ULL & vlSelf->__VnbaTriggered.word(0U))) {
        VL_DBG_MSGF("         'nba' region trigger index 63 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[0].COL[7].u_pe.clk)\n");
    }
    if ((1ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 64 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[0].u_pe.clk)\n");
    }
    if ((2ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 65 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[1].u_pe.clk)\n");
    }
    if ((4ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 66 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[2].u_pe.clk)\n");
    }
    if ((8ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 67 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[3].u_pe.clk)\n");
    }
    if ((0x10ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 68 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[4].u_pe.clk)\n");
    }
    if ((0x20ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 69 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[5].u_pe.clk)\n");
    }
    if ((0x40ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 70 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[6].u_pe.clk)\n");
    }
    if ((0x80ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 71 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[1].COL[7].u_pe.clk)\n");
    }
    if ((0x100ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 72 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[0].u_pe.clk)\n");
    }
    if ((0x200ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 73 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[1].u_pe.clk)\n");
    }
    if ((0x400ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 74 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[2].u_pe.clk)\n");
    }
    if ((0x800ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 75 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[3].u_pe.clk)\n");
    }
    if ((0x1000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 76 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[4].u_pe.clk)\n");
    }
    if ((0x2000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 77 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[5].u_pe.clk)\n");
    }
    if ((0x4000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 78 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[6].u_pe.clk)\n");
    }
    if ((0x8000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 79 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[2].COL[7].u_pe.clk)\n");
    }
    if ((0x10000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 80 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[0].u_pe.clk)\n");
    }
    if ((0x20000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 81 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[1].u_pe.clk)\n");
    }
    if ((0x40000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 82 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[2].u_pe.clk)\n");
    }
    if ((0x80000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 83 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[3].u_pe.clk)\n");
    }
    if ((0x100000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 84 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[4].u_pe.clk)\n");
    }
    if ((0x200000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 85 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[5].u_pe.clk)\n");
    }
    if ((0x400000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 86 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[6].u_pe.clk)\n");
    }
    if ((0x800000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 87 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[3].COL[7].u_pe.clk)\n");
    }
    if ((0x1000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 88 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 89 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 90 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 91 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 92 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[4].u_pe.clk)\n");
    }
    if ((0x20000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 93 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[5].u_pe.clk)\n");
    }
    if ((0x40000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 94 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[6].u_pe.clk)\n");
    }
    if ((0x80000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 95 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[4].COL[7].u_pe.clk)\n");
    }
    if ((0x100000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 96 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[0].u_pe.clk)\n");
    }
    if ((0x200000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 97 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[1].u_pe.clk)\n");
    }
    if ((0x400000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 98 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[2].u_pe.clk)\n");
    }
    if ((0x800000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 99 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[3].u_pe.clk)\n");
    }
    if ((0x1000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 100 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[4].u_pe.clk)\n");
    }
    if ((0x2000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 101 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[5].u_pe.clk)\n");
    }
    if ((0x4000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 102 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[6].u_pe.clk)\n");
    }
    if ((0x8000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 103 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[5].COL[7].u_pe.clk)\n");
    }
    if ((0x10000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 104 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[0].u_pe.clk)\n");
    }
    if ((0x20000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 105 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[1].u_pe.clk)\n");
    }
    if ((0x40000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 106 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[2].u_pe.clk)\n");
    }
    if ((0x80000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 107 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[3].u_pe.clk)\n");
    }
    if ((0x100000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 108 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[4].u_pe.clk)\n");
    }
    if ((0x200000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 109 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[5].u_pe.clk)\n");
    }
    if ((0x400000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 110 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[6].u_pe.clk)\n");
    }
    if ((0x800000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 111 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[6].COL[7].u_pe.clk)\n");
    }
    if ((0x1000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 112 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[0].u_pe.clk)\n");
    }
    if ((0x2000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 113 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[1].u_pe.clk)\n");
    }
    if ((0x4000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 114 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[2].u_pe.clk)\n");
    }
    if ((0x8000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 115 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[3].u_pe.clk)\n");
    }
    if ((0x10000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 116 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[4].u_pe.clk)\n");
    }
    if ((0x20000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 117 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[5].u_pe.clk)\n");
    }
    if ((0x40000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 118 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[6].u_pe.clk)\n");
    }
    if ((0x80000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 119 is active: @(posedge tb_systolic_dma_top.dut.ACC_8X8[0].u_acc.ROW[7].COL[7].u_pe.clk)\n");
    }
    if ((0x100000000000000ULL & vlSelf->__VnbaTriggered.word(1U))) {
        VL_DBG_MSGF("         'nba' region trigger index 120 is active: @([true] __VdlySched.awaitingCurrentTime())\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vtb_systolic_dma_top___024root___ctor_var_reset(Vtb_systolic_dma_top___024root* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtb_systolic_dma_top___024root___ctor_var_reset\n"); );
    // Body
    vlSelf->tb_systolic_dma_top__DOT__n_inv = 0;
    vlSelf->tb_systolic_dma_top__DOT__wb_base = 0;
    vlSelf->tb_systolic_dma_top__DOT__f = 0;
    vlSelf->tb_systolic_dma_top__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dpti_clkout = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dpti_txe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dpti_d = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dpti_rd_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dpti_wr_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dpti_oe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__rstn = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__led = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_addr = VL_RAND_RESET_I(15);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_ba = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_cas_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_ras_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_reset_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_we_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_ck_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_ck_p = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_cke = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_odt = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_dm = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_dq = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_dqs_n = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__ddr3_dqs_p = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__phase_d = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__errors = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__waited = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_fill = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_cyc = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_wb = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_span = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_chk = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_words = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__f_eng = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__i = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__c = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__got = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__want = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__unnamedblk2__DOT__si = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_clk_pin = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpu_resetn = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__led = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_addr = VL_RAND_RESET_I(15);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_ba = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_cas_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_ck_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_ck_p = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_cke = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_ras_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_reset_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_we_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_dq = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_dqs_n = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_dqs_p = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_dm = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ddr3_odt = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_d = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rxf_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_txe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clkout = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rd_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_wr_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_oe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_siwun = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clk100_ibuf = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clk200_raw = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clkfb_raw = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clkfb = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clk_sys_100 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__clk_ref_200 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__mmcm_locked_user = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sys_rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_clk_sync_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__mmcm_locked_mig = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__init_calib_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rst_i = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wdata_axi);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__sd_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_protocol = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hs_err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_owns_w = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__araddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__arready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdata_axi);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__phase_prev = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wr_err_range = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_written = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__desc_started = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_done_sticky = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_sticky = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fold_done_sticky = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fsm_fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_done = VL_RAND_RESET_I(4);
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_8x8[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_4x4[__Vi0] = VL_RAND_RESET_I(1);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_pending_id = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_replay = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_request = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_start_delayed = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_schedule_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_cycle_counter = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_active_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_active_compute_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_active_accelerator_id = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_start_window = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_accelerator_ready = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_fold_start_selected = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_active = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_id_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_device_id_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8_reg = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4_reg = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_m_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_k_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_start_cycle_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_est_cycles_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_8x8 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__select_4x4 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_8x8_idx = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_4x4_idx = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_bytes = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_n_beats = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_rx_words = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_a_base_reg = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_b_base_reg = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_c_base_reg = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__core_job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__result_readback_busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_status = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_host_cmd_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_mem_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_write32_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_frontend_err_mem_length = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_fifo_rd_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_cmd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_cmd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rsp_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rsp_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rsp_resp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_awaddr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_awready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_wdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_wstrb = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_oe = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_tx_wr_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst_meta = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_meta = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_phy_debug_sync = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_meta = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity_sync = VL_RAND_RESET_I(32);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_dst_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_tx_byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__axi_dpti_wr_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_araddr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_arvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_arready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dpti_axi_rready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__effective_job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__descriptor_downstream_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ingress_job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scheduler_desc_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__job_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_seen = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_armed = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_desc_started = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_done_sticky = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__err_w_owner = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_words = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_result_bytes = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__selected_wb_beats = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__words_want = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c = VL_RAND_RESET_I(7);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_last = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv_probe = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_probe = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_d = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rerun_pulse = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__n_inv = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_fi = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_bank = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_bank = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__run_clear = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fi_last = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_fi_last = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_fi_last = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__seed_slab_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_slab_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_region_base = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_tile_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_done_fold = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_active = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__read_pipeline_started = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__reads_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__compute_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__filled_fi = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cyc_running = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_done_fold = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__fill_running = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_running = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_span = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__folds_done = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__t_running = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_busy_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_rdy_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_r_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_busy_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_aw_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_w_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_starve_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_beat = VL_RAND_RESET_I(16);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_wr_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__dst_almost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_almost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_cdc_src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_desc_beats = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_pending = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_active = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_addr_reg = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_req_beats = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_is_readback = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_beats = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__eng_done_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rb_dst_wr_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__external_debug_sticky = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__op_dst_wr_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_wr = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_8x8[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            for (int __Vi2 = 0; __Vi2 < 8; ++__Vi2) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_8x8[__Vi0][__Vi1][__Vi2] = VL_RAND_RESET_I(32);
            }
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__k_dim = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__state = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__feed_t = VL_RAND_RESET_I(6);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_in_4x4[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_in_4x4[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_valid_to_8x8[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_valid_to_8x8[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__a_valid_to_4x4[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__b_valid_to_4x4[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_4x4[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 3; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            for (int __Vi2 = 0; __Vi2 < 4; ++__Vi2) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_out_4x4[__Vi0][__Vi1][__Vi2] = VL_RAND_RESET_I(32);
            }
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_selected = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_4x4_selected = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_valid_out_8x8_selected = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            for (int __Vi2 = 0; __Vi2 < 8; ++__Vi2) {
                vlSelf->tb_systolic_dma_top__DOT__dut__DOT__result_bank[__Vi0][__Vi1][__Vi2] = VL_RAND_RESET_I(32);
            }
        }
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__result_bank_valid[__Vi0] = VL_RAND_RESET_I(1);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_result_bank = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__result_bank_toggle = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rr = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cc = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cyc_count = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cyc_latched = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cyc_total = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C_wb[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__C_4x4[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rempty = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rempty = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rd_data);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rd_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb8_rd_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb4_rd_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr8_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_8x8 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__rdr4_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_rd_start_4x4 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_valid = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wb_src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_i = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_i = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_val_d = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_r_d = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__scan_c_d = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_rd = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__chk_c = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_row8 = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_col16 = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__cpos = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_wr = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__want_c = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__active_expected_wr_chk = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__wr_match = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__c_match = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__any_err = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__hb_ui = VL_RAND_RESET_I(26);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__unnamedblk1__DOT__start_i = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__a_wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wdata_buf = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__k16 = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__bank8 = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__wpos = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_dbg_count = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__a_rdata_0[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__a_rdata_1[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_rdata_0[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__b_rdata_1[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_0__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_a_buf_1__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_0__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__OP_V1__DOT__u_b_buf_1__wr = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__0__KET____DOT__u_acc__c_out[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__0__KET____DOT__u_acc__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__0__KET____DOT__u_acc__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__1__KET____DOT__u_acc__c_out[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__1__KET____DOT__u_acc__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__1__KET____DOT__u_acc__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_4X4__BRA__2__KET____DOT__u_acc__c_out[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__2__KET____DOT__u_acc__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_4X4__BRA__2__KET____DOT__u_acc__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellout__ACC_8X8__BRA__0__KET____DOT__u_acc__c_out[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_8X8__BRA__0__KET____DOT__u_acc__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT____Vcellinp__ACC_8X8__BRA__0__KET____DOT__u_acc__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc1c513c0__0 = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_hc50c9b32__0 = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT____VdfgTmp_h145b784e__0 = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_ibuf__DOT__I = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_ibuf__DOT__O = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKIN1 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKFBIN = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKFBOUT = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKFBOUTB = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT0 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT0B = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT1 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT1B = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT2 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT2B = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT3 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT3B = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT4 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT5 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__CLKOUT6 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__LOCKED = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__PWRDWN = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mmcm__DOT__RST = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_fb__DOT__I = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_fb__DOT__O = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_200__DOT__I = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_200__DOT__O = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_100__DOT__I = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_bufg_100__DOT__O = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_accelerator_id = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__desc_compute_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__accelerator_start = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__accelerator_done = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__accelerator_ready = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__schedule_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__cycle_counter = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__active_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__active_compute_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__active_accelerator_id = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_hw_scheduler__DOT__accelerator_done_seen = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__src_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__src_rst = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__src_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_rst = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__dst_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_data);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__mem[__Vi0]);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_bin = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr1 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd1 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd2 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_bin_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__wr_ptr_gray_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__rd_ptr_gray_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__fifo_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_output_cdc__DOT__u_fifo__DOT__fifo_empty = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__rst = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__beat_r);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__byte_idx = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_beat_to_byte__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_txe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_d_out = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_d_oe = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_tx__DOT__dpti_wr_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_clkout = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_d_in = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rxf_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_oe_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_byte_rx__DOT__dpti_rd_active = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__cmd_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_write32_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_mem_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__err_mem_length = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__owner = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__write32_byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__mem_byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__cmd_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__err_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__state = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__addr_r = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_write32__DOT__data_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_data = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__out_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_opcode = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__err_length = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__state = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__addr_tmp = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__length_tmp = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__header_index = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__payload_count = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT__u_mem_write__DOT__byte_index = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__ui_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__ui_rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__err_protocol = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__m_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__writer_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_base_addr = VL_RAND_RESET_I(29);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__dst_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__err_protocol = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(158, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_wr_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(158, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_rd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_pending = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__header_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_is_header = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__fifo_header_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(158, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(158, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_data);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        VL_RAND_RESET_W(158, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__mem[__Vi0]);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr1 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_wr2 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd1 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray_rd2 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_bin_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__wr_ptr_gray_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_bin_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__rd_ptr_gray_next = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__fifo_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_cdc__DOT__u_fifo__DOT__fifo_empty = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__base_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_valid = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__m_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__burst_left = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_mem_write_cdc_engine__DOT__u_writer__DOT__beat_left = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_data = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__mem[__Vi0] = VL_RAND_RESET_Q(40);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_bin = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr1 = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_wr2 = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray_rd1 = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray_rd2 = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_bin_next = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__wr_ptr_gray_next = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_bin_next = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__rd_ptr_gray_next = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__fifo_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_cmd_async_fifo__DOT__fifo_empty = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__cmd_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__rsp_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__rsp_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__rsp_resp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_awaddr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_awready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_wdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_wstrb = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__m_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__aw_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_axi4lite_master__DOT__w_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__aclk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__aresetn = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_awaddr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_awready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_wdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_wstrb = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_araddr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_arvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_arready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_rdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_rresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_rvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__s_axi_rready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__wr_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__wr_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__status = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__aw_pending = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__awaddr_reg = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__w_pending = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_axi4lite_dpti_bridge__DOT__wdata_reg = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__wr_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__wr_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__wr_addr = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__wr_data = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_dpti_descriptor_bridge__DOT__status = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__in_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__consumer_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_device_id = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_m = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_n = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_k = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_start_cycle = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_est_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_a_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_b_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_c_base = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__valid_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_id_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_device_id_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_m_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_n_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_k_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_start_cycle_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_est_cycles_r = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_a_base_r = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_b_base_r = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_job_ingress__DOT__job_c_base_r = VL_RAND_RESET_Q(64);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__base_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__m_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__burst_left = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_idx = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__beat_left = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vbase = VL_RAND_RESET_I(7);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__vsum[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_seed__DOT__unnamedblk1__DOT__j = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__init_calib_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_beats = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__desc_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__done_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_araddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_arready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rid = VL_RAND_RESET_I(2);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__m_axi_rready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_almost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_beat = VL_RAND_RESET_I(16);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__dst_wr_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__busy_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__rdy_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__stat_clear = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_r = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_left = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_left = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ret_idx = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__credit = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__cur_len = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__arlen_r = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__ar_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__r_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__first_len = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__addr_next = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__issue_after = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__len_next = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_eng__DOT__may_issue = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__enable = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__feed_t = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__k_dim = VL_RAND_RESET_I(6);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__a_rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__b_rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__a_raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__b_raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__a_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__b_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__a_valid_c[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__b_valid_c[__Vi0] = VL_RAND_RESET_I(1);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk1__DOT__i = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk2__DOT__i = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk3__DOT__i = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk4__DOT__r = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk4__DOT__unnamedblk5__DOT__gk_a = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk6__DOT__c = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_feeder__DOT__unnamedblk6__DOT__unnamedblk7__DOT__gk_b = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__done = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__C[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__rd = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__bidx = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__started = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__unnamedblk1__DOT__j = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_8x8__DOT__unnamedblk1__DOT__unnamedblk2__DOT__w = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wclk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wrst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__walmost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rclk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rrst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rd_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rd_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__mem[__Vi0]);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wbin_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wgray_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rbin_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rgray_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq1_rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq1_wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rq2_wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__walmost_full_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wfull_val = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__wq2_rbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__occupancy = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__rempty_val = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__done = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 4; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 4; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__C[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__rd = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__bidx = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__started = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__unnamedblk1__DOT__j = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_rdr_4x4__DOT__unnamedblk1__DOT__unnamedblk2__DOT__w = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wclk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wrst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wr_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__walmost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rclk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rrst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rd_en = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rd_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__mem[__Vi0]);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wbin_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wgray_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rbin_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rgray_next = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq1_rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq1_wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rq2_wgray = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__walmost_full_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wfull_val = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__wq2_rbin = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__occupancy = VL_RAND_RESET_I(6);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_r = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__rempty_val = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__init_calib_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_addr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_beats = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__desc_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_valid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__done_tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__m_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_valid = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_ready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__busy_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_stall_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__src_starve_cycles = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_align = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__err_resp = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__stat_clear = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__tag = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_r = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_left = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__cur_len = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_left = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__credit = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__awlen_r = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__aw_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__w_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__b_fire = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__first_len = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__len_next = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__addr_next = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_wb__DOT__issue_after = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_addr = VL_RAND_RESET_I(15);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_ba = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_cas_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_ck_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_ck_p = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_cke = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_ras_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_reset_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_we_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_dq = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_dqs_n = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_dqs_p = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__init_calib_complete = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_dm = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ddr3_odt = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_clk_sync_rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_addn_clk_0 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_addn_clk_1 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_addn_clk_2 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_addn_clk_3 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__ui_addn_clk_4 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__mmcm_locked = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__aresetn = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_sr_req = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_ref_req = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_zq_req = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_sr_active = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_ref_ack = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__app_zq_ack = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awaddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_awready = VL_RAND_RESET_I(1);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wstrb = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_wready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_bid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_bresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_bvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_bready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arid = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_araddr = VL_RAND_RESET_I(29);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arlen = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arsize = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arburst = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arlock = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arcache = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arprot = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arqos = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_arready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rid = VL_RAND_RESET_I(2);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rdata);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rresp = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rlast = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rvalid = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__s_axi_rready = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__sys_clk_i = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__clk_ref_i = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__sys_rst = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 32768; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__errors = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__wb_seen = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__wb_region = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__calib_ctr = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_addr = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_left = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_burst_base = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_burst_len = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__w_open = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__q_addr.atDefault() = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__q_len.atDefault() = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__r_addr = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__r_left = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__unnamedblk1__DOT__i = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_mig_7series_0__DOT__unnamedblk2__DOT__j = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in0 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in1 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in2 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in3 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in4 = VL_RAND_RESET_I(8);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in5 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in6 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in7 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in8 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in9 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in10 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in11 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in12 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in13 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in14 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in15 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in16 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in17 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in18 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_in19 = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out0 = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__probe_out1 = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__n_inv_arg = VL_RAND_RESET_I(4);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__rerun_arg = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__u_vio__DOT__n_inv_plus = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__accelerator_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__accelerator_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_8X8__BRA__0__KET____DOT__u_adapter__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__accelerator_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__accelerator_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__0__KET____DOT__u_adapter__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__accelerator_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__accelerator_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__1__KET____DOT__u_adapter__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__accelerator_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__accelerator_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__fold_start = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__c_done = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__SCHED_4X4__BRA__2__KET____DOT__u_adapter__DOT__state = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__rst_n = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_wr_en = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_wr_beat = VL_RAND_RESET_I(16);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_wr_data);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__dst_almost_full = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__wdata = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__words_written = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__err_range = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__clear = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__busy = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__cnt = VL_RAND_RESET_I(2);
    VL_RAND_RESET_W(128, vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__data_q);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__beat_q = VL_RAND_RESET_I(16);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__in_range = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__accept = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__w = VL_RAND_RESET_I(9);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__mat = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__is_b = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__win = VL_RAND_RESET_I(2);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_lane = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__a_koff = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_koff = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__b_lane = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_wr__DOT__word_sel = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__0__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__0__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__1__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__1__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__2__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__2__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__3__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__3__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__4__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__4__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__5__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__5__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__6__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__6__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__7__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_0__DOT__BANK__BRA__7__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__0__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__0__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__1__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__1__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__2__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__2__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__3__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__3__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__4__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__4__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__5__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__5__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__6__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__6__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__7__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_a_buf_1__DOT__BANK__BRA__7__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__0__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__0__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__1__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__1__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__2__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__2__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__3__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__3__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__4__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__4__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__5__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__5__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__6__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__6__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__7__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_0__DOT__BANK__BRA__7__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wr = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wsel = VL_RAND_RESET_I(3);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__waddr = VL_RAND_RESET_I(5);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__raddr[__Vi0] = VL_RAND_RESET_I(5);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__rdata[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__0__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__0__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__1__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__1__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__2__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__2__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__3__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__3__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__4__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__4__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__5__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__5__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__6__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__6__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 32; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__7__KET____DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__OP_V1__DOT__u_b_buf_1__DOT__BANK__BRA__7__KET____DOT__rdata_q = VL_RAND_RESET_I(32);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__rst = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__a_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__b_in[__Vi0] = VL_RAND_RESET_I(32);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__a_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__b_valid_in[__Vi0] = VL_RAND_RESET_I(1);
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_valid_out = VL_RAND_RESET_I(1);
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__c_out[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 9; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__a_bus[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__b_bus[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 9; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__a_valid_bus[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__b_valid_bus[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc[__Vi0][__Vi1] = VL_RAND_RESET_I(32);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__pe_acc_valid[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    for (int __Vi0 = 0; __Vi0 < 8; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 8; ++__Vi1) {
            vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__acc_arrived[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__clear_arrived = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__all_arrived = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__out_state = VL_RAND_RESET_I(1);
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__unnamedblk1__DOT__rr = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__unnamedblk1__DOT__unnamedblk2__DOT__cc = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__unnamedblk3__DOT__rr = 0;
    vlSelf->tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__unnamedblk3__DOT__unnamedblk4__DOT__cc = 0;
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_8x8__DOT__gray2bin__18__Vfuncout = VL_RAND_RESET_I(6);
    vlSelf->__Vfunc_tb_systolic_dma_top__DOT__dut__DOT__u_wb_fifo_4x4__DOT__gray2bin__19__Vfuncout = VL_RAND_RESET_I(6);
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__phase = VL_RAND_RESET_I(4);
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__dpti_clk_activity = VL_RAND_RESET_I(32);
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank_valid__v0 = 0;
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__result_bank_toggle = VL_RAND_RESET_I(1);
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank_valid__v1 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v0 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v0 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v1 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v1 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v2 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v3 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v4 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v5 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v6 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v7 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v8 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v8 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v9 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v9 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v10 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v10 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v11 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v11 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v12 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v12 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v13 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v13 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v14 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v14 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v15 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v15 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v16 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v17 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v18 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v19 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v20 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v21 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v22 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v23 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v24 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v25 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v26 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v27 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v28 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v29 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v30 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v31 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v32 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v33 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v34 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v35 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v36 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v37 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v38 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v39 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v40 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v41 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v42 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v43 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v44 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v45 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v46 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v47 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v48 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v49 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v50 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v51 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v52 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v53 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v54 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v55 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v56 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v57 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v58 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v59 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v60 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v61 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v62 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v63 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank_valid__v2 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v64 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v64 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v65 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v65 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v66 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v67 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v68 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v68 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v69 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v69 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v70 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v70 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__C__v71 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v71 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v72 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v73 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v74 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v75 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v76 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v77 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v78 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank__v79 = 0;
    vlSelf->__Vdlyvset__tb_systolic_dma_top__DOT__dut__DOT__result_bank_valid__v3 = 0;
    vlSelf->__Vdly__tb_systolic_dma_top__DOT__dut__DOT__state = VL_RAND_RESET_I(3);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__u_ibuf__DOT__O__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ui_rst_n__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dpti_clkout__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__u_dpti_command_frontend__DOT____Vcellinp__u_mem_write__rst_n__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__1__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__2__KET____DOT__u_acc__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__0__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__1__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__2__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__3__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__4__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__5__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__6__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__0__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__1__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__2__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__3__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__4__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__5__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__6__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
    vlSelf->__Vtrigprevexpr___TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_8X8__BRA__0__KET____DOT__u_acc__DOT__ROW__BRA__7__KET____DOT__COL__BRA__7__KET____DOT__u_pe__clk__0 = VL_RAND_RESET_I(1);
}
