// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top_systolic_array__N4.h"
#include "Vtb_systolic_dma_top_systolic_pe.h"

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0\n"); );
    // Body
    vlSelf->all_arrived = 1U;
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[0U]
                           [0U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[0U]
                           [1U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[0U]
                           [2U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[0U]
                           [3U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[1U]
                           [0U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[1U]
                           [1U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[1U]
                           [2U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[1U]
                           [3U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[2U]
                           [0U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[2U]
                           [1U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[2U]
                           [2U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[2U]
                           [3U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[3U]
                           [0U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[3U]
                           [1U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[3U]
                           [2U]);
    vlSelf->all_arrived = ((IData)(vlSelf->all_arrived) 
                           & vlSelf->acc_arrived[3U]
                           [3U]);
    vlSelf->clear_arrived = vlSelf->out_state;
    vlSelf->pe_acc_valid[0U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[1U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[2U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[3U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc_valid[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
    vlSelf->pe_acc[0U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[1U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[2U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[3U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->pe_acc[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->a_valid_bus[0U][0U] = vlSelf->a_valid_in
        [0U];
    vlSelf->a_valid_bus[1U][0U] = vlSelf->a_valid_in
        [1U];
    vlSelf->a_valid_bus[2U][0U] = vlSelf->a_valid_in
        [2U];
    vlSelf->a_valid_bus[3U][0U] = vlSelf->a_valid_in
        [3U];
    vlSelf->b_valid_bus[0U][0U] = vlSelf->b_valid_in
        [0U];
    vlSelf->b_valid_bus[0U][1U] = vlSelf->b_valid_in
        [1U];
    vlSelf->b_valid_bus[0U][2U] = vlSelf->b_valid_in
        [2U];
    vlSelf->b_valid_bus[0U][3U] = vlSelf->b_valid_in
        [3U];
    vlSelf->a_bus[0U][0U] = vlSelf->a_in[0U];
    vlSelf->a_bus[1U][0U] = vlSelf->a_in[1U];
    vlSelf->a_bus[2U][0U] = vlSelf->a_in[2U];
    vlSelf->a_bus[3U][0U] = vlSelf->a_in[3U];
    vlSelf->b_bus[0U][0U] = vlSelf->b_in[0U];
    vlSelf->b_bus[0U][1U] = vlSelf->b_in[1U];
    vlSelf->b_bus[0U][2U] = vlSelf->b_in[2U];
    vlSelf->b_bus[0U][3U] = vlSelf->b_in[3U];
    vlSelf->c_out[0U][0U] = vlSelf->pe_acc[0U][0U];
    vlSelf->c_out[0U][1U] = vlSelf->pe_acc[0U][1U];
    vlSelf->c_out[0U][2U] = vlSelf->pe_acc[0U][2U];
    vlSelf->c_out[0U][3U] = vlSelf->pe_acc[0U][3U];
    vlSelf->c_out[1U][0U] = vlSelf->pe_acc[1U][0U];
    vlSelf->c_out[1U][1U] = vlSelf->pe_acc[1U][1U];
    vlSelf->c_out[1U][2U] = vlSelf->pe_acc[1U][2U];
    vlSelf->c_out[1U][3U] = vlSelf->pe_acc[1U][3U];
    vlSelf->c_out[2U][0U] = vlSelf->pe_acc[2U][0U];
    vlSelf->c_out[2U][1U] = vlSelf->pe_acc[2U][1U];
    vlSelf->c_out[2U][2U] = vlSelf->pe_acc[2U][2U];
    vlSelf->c_out[2U][3U] = vlSelf->pe_acc[2U][3U];
    vlSelf->c_out[3U][0U] = vlSelf->pe_acc[3U][0U];
    vlSelf->c_out[3U][1U] = vlSelf->pe_acc[3U][1U];
    vlSelf->c_out[3U][2U] = vlSelf->pe_acc[3U][2U];
    vlSelf->c_out[3U][3U] = vlSelf->pe_acc[3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__1(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__1\n"); );
    // Body
    vlSelf->a_valid_bus[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[1U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[0U][4U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[0U][4U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[2U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[1U][4U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[1U][4U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[3U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[2U][4U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[2U][4U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[4U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[4U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[4U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
    vlSelf->a_valid_bus[3U][4U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->b_valid_bus[4U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_bus[3U][4U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][3U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][3U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][3U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___act_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___act_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0\n"); );
    // Body
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->clk 
        = vlSelf->clk;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->clk 
        = vlSelf->clk;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__0\n"); );
    // Body
    vlSelf->pe_acc[0U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[1U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__1(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__1\n"); );
    // Body
    vlSelf->pe_acc[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__2(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__2\n"); );
    // Body
    vlSelf->pe_acc[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__3(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__3\n"); );
    // Body
    vlSelf->pe_acc[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[0U][4U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[0U][4U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__4(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__4\n"); );
    // Body
    vlSelf->pe_acc[1U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[2U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__5(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__5\n"); );
    // Body
    vlSelf->pe_acc[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__6(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__6\n"); );
    // Body
    vlSelf->pe_acc[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__7(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__7\n"); );
    // Body
    vlSelf->pe_acc[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[1U][4U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[1U][4U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__8(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__8\n"); );
    // Body
    vlSelf->pe_acc[2U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[3U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__9(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__9\n"); );
    // Body
    vlSelf->pe_acc[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__10(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__10\n"); );
    // Body
    vlSelf->pe_acc[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__11(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__11\n"); );
    // Body
    vlSelf->pe_acc[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[2U][4U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[2U][4U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__12(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__12\n"); );
    // Body
    vlSelf->pe_acc[3U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[4U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__13(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__13\n"); );
    // Body
    vlSelf->pe_acc[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[4U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__14(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__14\n"); );
    // Body
    vlSelf->pe_acc[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[4U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__15(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__15\n"); );
    // Body
    vlSelf->pe_acc[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_out;
    vlSelf->b_valid_bus[4U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_out;
    vlSelf->a_valid_bus[3U][4U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_out;
    vlSelf->a_bus[3U][4U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_out;
    vlSelf->b_bus[4U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__17(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__17\n"); );
    // Body
    vlSelf->pe_acc_valid[0U][0U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__18(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__18\n"); );
    // Body
    vlSelf->pe_acc_valid[0U][1U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__19(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__19\n"); );
    // Body
    vlSelf->pe_acc_valid[0U][2U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__20(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__20\n"); );
    // Body
    vlSelf->pe_acc_valid[0U][3U] = vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__21(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__21\n"); );
    // Body
    vlSelf->pe_acc_valid[1U][0U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__22(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__22\n"); );
    // Body
    vlSelf->pe_acc_valid[1U][1U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__23(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__23\n"); );
    // Body
    vlSelf->pe_acc_valid[1U][2U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__24(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__24\n"); );
    // Body
    vlSelf->pe_acc_valid[1U][3U] = vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__25(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__25\n"); );
    // Body
    vlSelf->pe_acc_valid[2U][0U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__26(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__26\n"); );
    // Body
    vlSelf->pe_acc_valid[2U][1U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__27(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__27\n"); );
    // Body
    vlSelf->pe_acc_valid[2U][2U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__28(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__28\n"); );
    // Body
    vlSelf->pe_acc_valid[2U][3U] = vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__29(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__29\n"); );
    // Body
    vlSelf->pe_acc_valid[3U][0U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__30(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__30\n"); );
    // Body
    vlSelf->pe_acc_valid[3U][1U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__31(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__31\n"); );
    // Body
    vlSelf->pe_acc_valid[3U][2U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__32(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__32\n"); );
    // Body
    vlSelf->pe_acc_valid[3U][3U] = vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->acc_valid_out;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__33(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__33\n"); );
    // Body
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->rst 
        = vlSelf->rst;
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->rst 
        = vlSelf->rst;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_comb__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__2(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_comb__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__2\n"); );
    // Body
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_in 
        = vlSelf->a_bus[3U][3U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_in 
        = vlSelf->b_bus[3U][3U];
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_array__N4___nba_comb__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__4(Vtb_systolic_dma_top_systolic_array__N4* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vtb_systolic_dma_top_systolic_array__N4___nba_comb__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__4\n"); );
    // Body
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->a_valid_in 
        = vlSelf->a_valid_bus[3U][3U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][0U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][1U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][2U];
    vlSelf->__PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[0U][3U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][0U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][1U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][2U];
    vlSelf->__PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[1U][3U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][0U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][1U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][2U];
    vlSelf->__PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[2U][3U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][0U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][1U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][2U];
    vlSelf->__PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe->b_valid_in 
        = vlSelf->b_valid_bus[3U][3U];
}
