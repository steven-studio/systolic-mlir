// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top_systolic_pe.h"

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___ico_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0\n"); );
    // Body
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__rst = vlSelf->rst;
    vlSelf->u_fp_add_accum__DOT__rst = vlSelf->rst;
    vlSelf->u_fp_add_reduce__DOT__rst = vlSelf->rst;
    vlSelf->u_fp_mul__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set0_copy0__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set0_copy1__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set1_copy0__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set1_copy1__DOT__clk = vlSelf->clk;
    vlSelf->u_fp_add_accum__DOT__clk = vlSelf->clk;
    vlSelf->u_fp_add_reduce__DOT__clk = vlSelf->clk;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        vlSelf->product_valid = 1U;
        vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
        vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
        vlSelf->set0_c1_raddr = 0U;
        vlSelf->set1_c1_raddr = 0U;
        vlSelf->set0_c0_raddr = 0U;
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->product_valid = 0U;
        vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
        vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
        vlSelf->set0_c1_raddr = 0U;
        vlSelf->set1_c1_raddr = 0U;
        vlSelf->set0_c0_raddr = 0U;
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        vlSelf->accum_add_valid = 1U;
        vlSelf->set0_we = 0U;
        vlSelf->set1_we = 0U;
        vlSelf->set0_waddr = 0U;
        vlSelf->set1_waddr = 0U;
        vlSelf->set0_wdata = 0U;
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->accum_add_valid = 0U;
        vlSelf->set0_we = 0U;
        vlSelf->set1_we = 0U;
        vlSelf->set0_waddr = 0U;
        vlSelf->set1_waddr = 0U;
        vlSelf->set0_wdata = 0U;
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___act_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___act_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0\n"); );
    // Body
    vlSelf->u_fp_mul__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set0_copy0__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set0_copy1__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set1_copy0__DOT__clk = vlSelf->clk;
    vlSelf->u_acc_set1_copy1__DOT__clk = vlSelf->clk;
    vlSelf->u_fp_add_accum__DOT__clk = vlSelf->clk;
    vlSelf->u_fp_add_reduce__DOT__clk = vlSelf->clk;
}

extern const VlUnpacked<CData/*1:0*/, 32> Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0;
extern const VlUnpacked<CData/*0:0*/, 32> Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0;
extern const VlUnpacked<CData/*0:0*/, 32> Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0;

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0\n"); );
    // Init
    CData/*4:0*/ __Vtableidx1;
    __Vtableidx1 = 0;
    CData/*7:0*/ __Vdly__mul_busy;
    __Vdly__mul_busy = 0;
    CData/*7:0*/ __Vdly__accum_add_busy;
    __Vdly__accum_add_busy = 0;
    CData/*7:0*/ __Vdly__reduce_add_busy;
    __Vdly__reduce_add_busy = 0;
    CData/*0:0*/ __Vdly__acc_set;
    __Vdly__acc_set = 0;
    CData/*3:0*/ __Vdly__product_bank;
    __Vdly__product_bank = 0;
    CData/*3:0*/ __Vdly__accum_wb_bank;
    __Vdly__accum_wb_bank = 0;
    CData/*1:0*/ __Vdly__red_state;
    __Vdly__red_state = 0;
    CData/*3:0*/ __Vdly__reduce_stride;
    __Vdly__reduce_stride = 0;
    CData/*3:0*/ __Vdly__reduce_i;
    __Vdly__reduce_i = 0;
    CData/*3:0*/ __Vdly__reduce_wb_i;
    __Vdly__reduce_wb_i = 0;
    CData/*3:0*/ __Vdly__reduce_todo;
    __Vdly__reduce_todo = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v0;
    __Vdlyvset__acc_bank_valid__v0 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v32;
    __Vdlyvdim0__acc_bank_valid__v32 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v32;
    __Vdlyvdim1__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v32;
    __Vdlyvset__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v33;
    __Vdlyvdim0__acc_bank_valid__v33 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v33;
    __Vdlyvdim1__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v33;
    __Vdlyvset__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v34;
    __Vdlyvdim0__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v34;
    __Vdlyvset__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v35;
    __Vdlyvdim0__acc_bank_valid__v35 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v36;
    __Vdlyvdim0__acc_bank_valid__v36 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v37;
    __Vdlyvdim0__acc_bank_valid__v37 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v38;
    __Vdlyvdim0__acc_bank_valid__v38 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v39;
    __Vdlyvdim0__acc_bank_valid__v39 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v40;
    __Vdlyvdim0__acc_bank_valid__v40 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v41;
    __Vdlyvdim0__acc_bank_valid__v41 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v42;
    __Vdlyvdim0__acc_bank_valid__v42 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v43;
    __Vdlyvdim0__acc_bank_valid__v43 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v44;
    __Vdlyvdim0__acc_bank_valid__v44 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v45;
    __Vdlyvdim0__acc_bank_valid__v45 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v46;
    __Vdlyvdim0__acc_bank_valid__v46 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v47;
    __Vdlyvdim0__acc_bank_valid__v47 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v48;
    __Vdlyvdim0__acc_bank_valid__v48 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v49;
    __Vdlyvdim0__acc_bank_valid__v49 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    // Body
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0U;
    __Vdly__mul_busy = vlSelf->mul_busy;
    __Vdly__product_bank = vlSelf->product_bank;
    __Vdly__accum_wb_bank = vlSelf->accum_wb_bank;
    __Vdly__accum_add_busy = vlSelf->accum_add_busy;
    __Vdly__reduce_add_busy = vlSelf->reduce_add_busy;
    __Vdly__reduce_stride = vlSelf->reduce_stride;
    __Vdly__reduce_wb_i = vlSelf->reduce_wb_i;
    __Vdly__acc_set = vlSelf->acc_set;
    __Vdly__reduce_i = vlSelf->reduce_i;
    __Vdly__reduce_todo = vlSelf->reduce_todo;
    __Vdly__red_state = vlSelf->red_state;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__acc_bank_valid__v0 = 0U;
    __Vdlyvset__acc_bank_valid__v32 = 0U;
    __Vdlyvset__acc_bank_valid__v33 = 0U;
    __Vdlyvset__acc_bank_valid__v34 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0U;
    if (vlSelf->rst) {
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__s = 1U;
        vlSelf->unnamedblk1__DOT__s = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xcU;
        __Vdly__mul_busy = 0U;
        __Vdly__accum_add_busy = 0U;
        __Vdly__reduce_add_busy = 0U;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__acc_bank_valid__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 1U;
        vlSelf->accum_old_valid = 0U;
        vlSelf->reduce_a_old_valid = 0U;
        vlSelf->reduce_b_old_valid = 0U;
        vlSelf->acc_out = 0U;
        vlSelf->a_reg = 0U;
        vlSelf->b_reg = 0U;
        vlSelf->accum_product_d = 0U;
        __Vdly__red_state = 0U;
        vlSelf->red_set = 0U;
        __Vdly__reduce_stride = 0U;
        __Vdly__reduce_i = 0U;
        __Vdly__reduce_wb_i = 0U;
        __Vdly__reduce_todo = 0U;
        vlSelf->final_reduce_result = 0U;
    } else {
        if (((IData)(vlSelf->pipe_pair_valid) & (~ (IData)(vlSelf->u_fp_mul__DOT__valid_out)))) {
            __Vdly__mul_busy = (0xffU & ((IData)(1U) 
                                         + (IData)(vlSelf->mul_busy)));
        } else if (((~ (IData)(vlSelf->pipe_pair_valid)) 
                    & (IData)(vlSelf->u_fp_mul__DOT__valid_out))) {
            __Vdly__mul_busy = (0xffU & ((IData)(vlSelf->mul_busy) 
                                         - (IData)(1U)));
        }
        if (((IData)(vlSelf->accum_read_valid) & (~ (IData)(vlSelf->u_fp_add_accum__DOT__valid_out)))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(1U) 
                                               + (IData)(vlSelf->accum_add_busy)));
        } else if (((~ (IData)(vlSelf->accum_read_valid)) 
                    & (IData)(vlSelf->u_fp_add_accum__DOT__valid_out))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(vlSelf->accum_add_busy) 
                                               - (IData)(1U)));
        }
        if (((IData)(vlSelf->reduce_read_valid) & (~ (IData)(vlSelf->reduce_add_valid)))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(1U) 
                                                + (IData)(vlSelf->reduce_add_busy)));
        } else if (((~ (IData)(vlSelf->reduce_read_valid)) 
                    & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(vlSelf->reduce_add_busy) 
                                                - (IData)(1U)));
        }
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = vlSelf->pipe_pair_valid;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 1U;
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = VL_MULS_III(32, vlSelf->a_reg, vlSelf->b_reg);
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 
            = (vlSelf->reduce_a_value + vlSelf->reduce_b_value);
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 
            = (vlSelf->accum_old_value + vlSelf->accum_product_d);
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 1U;
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdlyvset__acc_bank_valid__v32 = 1U;
            __Vdlyvdim1__acc_bank_valid__v32 = vlSelf->accum_wb_bank;
            __Vdlyvdim0__acc_bank_valid__v32 = vlSelf->acc_set;
        }
        if (vlSelf->reduce_add_valid) {
            __Vdlyvset__acc_bank_valid__v33 = 1U;
            __Vdlyvdim1__acc_bank_valid__v33 = vlSelf->reduce_wb_i;
            __Vdlyvdim0__acc_bank_valid__v33 = vlSelf->red_set;
        }
        if ((2U == (IData)(vlSelf->red_state))) {
            __Vdlyvset__acc_bank_valid__v34 = 1U;
            __Vdlyvdim0__acc_bank_valid__v34 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v35 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v36 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v37 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v38 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v39 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v40 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v41 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v42 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v43 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v44 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v45 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v46 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v47 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v48 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v49 = vlSelf->red_set;
        }
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 
            = vlSelf->accum_read_valid;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 
            = vlSelf->reduce_read_valid;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 1U;
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            vlSelf->accum_old_valid = ((IData)(vlSelf->acc_set)
                                        ? vlSelf->acc_bank_valid
                                       [1U][vlSelf->product_bank]
                                        : vlSelf->acc_bank_valid
                                       [0U][vlSelf->product_bank]);
            vlSelf->accum_product_d = vlSelf->u_fp_mul__DOT__result;
        }
        if (vlSelf->reduce_issue) {
            if (vlSelf->red_set) {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [1U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [1U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            } else {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [0U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [0U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            }
        }
        if (vlSelf->acc_commit_q) {
            vlSelf->acc_out = vlSelf->final_reduce_result;
        }
        if (vlSelf->a_valid_in) {
            vlSelf->a_reg = vlSelf->a_in;
        }
        if (vlSelf->b_valid_in) {
            vlSelf->b_reg = vlSelf->b_in;
        }
        if (((1U == (IData)(vlSelf->red_state)) & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_wb_i = (0xfU & ((IData)(1U) 
                                           + (IData)(vlSelf->reduce_wb_i)));
        }
        if ((0U == (IData)(vlSelf->red_state))) {
            if (vlSelf->acc_handoff) {
                vlSelf->red_set = vlSelf->acc_set;
                __Vdly__reduce_stride = 8U;
                __Vdly__reduce_todo = 8U;
                __Vdly__reduce_i = 0U;
                __Vdly__reduce_wb_i = 0U;
                __Vdly__red_state = 1U;
            }
        } else if ((1U == (IData)(vlSelf->red_state))) {
            if ((0U != (IData)(vlSelf->reduce_todo))) {
                __Vdly__reduce_todo = (0xfU & ((IData)(vlSelf->reduce_todo) 
                                               - (IData)(1U)));
                __Vdly__reduce_i = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->reduce_i)));
            } else if (((0U == (IData)(vlSelf->reduce_add_busy)) 
                        & (~ (IData)(vlSelf->reduce_read_valid)))) {
                if ((1U == (IData)(vlSelf->reduce_stride))) {
                    __Vdly__red_state = 2U;
                } else {
                    __Vdly__reduce_stride = (0xfU & 
                                             VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_todo = (0xfU & VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_i = 0U;
                    __Vdly__reduce_wb_i = 0U;
                }
            }
        } else {
            __Vdly__red_state = 0U;
        }
        if (((IData)(vlSelf->reduce_add_valid) & (1U 
                                                  == (IData)(vlSelf->reduce_stride)))) {
            vlSelf->final_reduce_result = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    if ((1U & (~ (IData)(vlSelf->rst)))) {
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xcU;
        if ((2U == (IData)(vlSelf->red_state))) {
            vlSelf->unnamedblk3__DOT__i = 1U;
            vlSelf->unnamedblk3__DOT__i = 2U;
            vlSelf->unnamedblk3__DOT__i = 3U;
            vlSelf->unnamedblk3__DOT__i = 4U;
            vlSelf->unnamedblk3__DOT__i = 5U;
            vlSelf->unnamedblk3__DOT__i = 6U;
            vlSelf->unnamedblk3__DOT__i = 7U;
            vlSelf->unnamedblk3__DOT__i = 8U;
            vlSelf->unnamedblk3__DOT__i = 9U;
            vlSelf->unnamedblk3__DOT__i = 0xaU;
            vlSelf->unnamedblk3__DOT__i = 0xbU;
            vlSelf->unnamedblk3__DOT__i = 0xcU;
            vlSelf->unnamedblk3__DOT__i = 0xdU;
            vlSelf->unnamedblk3__DOT__i = 0xeU;
            vlSelf->unnamedblk3__DOT__i = 0xfU;
            vlSelf->unnamedblk3__DOT__i = 0x10U;
        }
    }
    if (vlSelf->set1_we) {
        __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_waddr;
        __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_waddr;
    }
    if (vlSelf->set0_we) {
        __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_waddr;
        __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_waddr;
    }
    if (((IData)(vlSelf->rst) | (IData)(vlSelf->acc_handoff))) {
        __Vdly__product_bank = 0U;
        __Vdly__accum_wb_bank = 0U;
    } else {
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            __Vdly__product_bank = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->product_bank)));
        }
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdly__accum_wb_bank = (0xfU & ((IData)(1U) 
                                             + (IData)(vlSelf->accum_wb_bank)));
        }
    }
    __Vtableidx1 = (((IData)(vlSelf->pipe_pair_valid) 
                     << 4U) | (((IData)(vlSelf->acc_set) 
                                << 3U) | (((IData)(vlSelf->acc_handoff) 
                                           << 2U) | 
                                          (((IData)(vlSelf->acc_state) 
                                            << 1U) 
                                           | (IData)(vlSelf->rst)))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx1])) {
        vlSelf->acc_state = Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0
            [__Vtableidx1];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx1])) {
        __Vdly__acc_set = Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0
            [__Vtableidx1];
    }
    vlSelf->u_acc_set1_copy1__DOT__rdata = vlSelf->u_acc_set1_copy1__DOT__mem
        [vlSelf->set1_c1_raddr];
    vlSelf->u_acc_set0_copy1__DOT__rdata = vlSelf->u_acc_set0_copy1__DOT__mem
        [vlSelf->set0_c1_raddr];
    vlSelf->u_acc_set1_copy0__DOT__rdata = vlSelf->u_acc_set1_copy0__DOT__mem
        [vlSelf->set1_c0_raddr];
    vlSelf->u_acc_set0_copy0__DOT__rdata = vlSelf->u_acc_set0_copy0__DOT__mem
        [vlSelf->set0_c0_raddr];
    vlSelf->acc_valid_out = ((1U & (~ (IData)(vlSelf->rst))) 
                             && (IData)(vlSelf->acc_commit_q));
    vlSelf->b_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->b_valid_in));
    vlSelf->a_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->a_valid_in));
    vlSelf->mul_busy = __Vdly__mul_busy;
    vlSelf->accum_add_busy = __Vdly__accum_add_busy;
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v0) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v9) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v10) {
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v11) {
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v12) {
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v13) {
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v14) {
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v15) {
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v16) {
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v17) {
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v0) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v9) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v10) {
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v11) {
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v12) {
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v13) {
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v14) {
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v15) {
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v16) {
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v17) {
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    }
    vlSelf->accum_wb_bank = __Vdly__accum_wb_bank;
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_acc_set1_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set1_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    }
    vlSelf->product_bank = __Vdly__product_bank;
    if (__Vdlyvset__acc_bank_valid__v0) {
        vlSelf->acc_bank_valid[0U][0U] = 0U;
        vlSelf->acc_bank_valid[0U][1U] = 0U;
        vlSelf->acc_bank_valid[0U][2U] = 0U;
        vlSelf->acc_bank_valid[0U][3U] = 0U;
        vlSelf->acc_bank_valid[0U][4U] = 0U;
        vlSelf->acc_bank_valid[0U][5U] = 0U;
        vlSelf->acc_bank_valid[0U][6U] = 0U;
        vlSelf->acc_bank_valid[0U][7U] = 0U;
        vlSelf->acc_bank_valid[0U][8U] = 0U;
        vlSelf->acc_bank_valid[0U][9U] = 0U;
        vlSelf->acc_bank_valid[0U][0xaU] = 0U;
        vlSelf->acc_bank_valid[0U][0xbU] = 0U;
        vlSelf->acc_bank_valid[0U][0xcU] = 0U;
        vlSelf->acc_bank_valid[0U][0xdU] = 0U;
        vlSelf->acc_bank_valid[0U][0xeU] = 0U;
        vlSelf->acc_bank_valid[0U][0xfU] = 0U;
        vlSelf->acc_bank_valid[1U][0U] = 0U;
        vlSelf->acc_bank_valid[1U][1U] = 0U;
        vlSelf->acc_bank_valid[1U][2U] = 0U;
        vlSelf->acc_bank_valid[1U][3U] = 0U;
        vlSelf->acc_bank_valid[1U][4U] = 0U;
        vlSelf->acc_bank_valid[1U][5U] = 0U;
        vlSelf->acc_bank_valid[1U][6U] = 0U;
        vlSelf->acc_bank_valid[1U][7U] = 0U;
        vlSelf->acc_bank_valid[1U][8U] = 0U;
        vlSelf->acc_bank_valid[1U][9U] = 0U;
        vlSelf->acc_bank_valid[1U][0xaU] = 0U;
        vlSelf->acc_bank_valid[1U][0xbU] = 0U;
        vlSelf->acc_bank_valid[1U][0xcU] = 0U;
        vlSelf->acc_bank_valid[1U][0xdU] = 0U;
        vlSelf->acc_bank_valid[1U][0xeU] = 0U;
        vlSelf->acc_bank_valid[1U][0xfU] = 0U;
    }
    if (__Vdlyvset__acc_bank_valid__v32) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v32][__Vdlyvdim1__acc_bank_valid__v32] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v33) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v33][__Vdlyvdim1__acc_bank_valid__v33] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v34) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v34][0U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v35][1U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v36][2U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v37][3U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v38][4U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v39][5U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v40][6U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v41][7U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v42][8U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v43][9U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v44][0xaU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v45][0xbU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v46][0xcU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v47][0xdU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v48][0xeU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v49][0xfU] = 0U;
    }
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->accum_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                && (IData)(vlSelf->u_fp_mul__DOT__valid_out));
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->acc_commit_q = ((~ (IData)(vlSelf->rst)) 
                            & (IData)(vlSelf->final_reduction_complete));
    vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->accum_add_valid = vlSelf->u_fp_add_accum__DOT__valid_out;
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->reduce_add_busy = __Vdly__reduce_add_busy;
    vlSelf->reduce_wb_i = __Vdly__reduce_wb_i;
    vlSelf->reduce_i = __Vdly__reduce_i;
    vlSelf->reduce_todo = __Vdly__reduce_todo;
    vlSelf->red_state = __Vdly__red_state;
    vlSelf->acc_set = __Vdly__acc_set;
    vlSelf->reduce_stride = __Vdly__reduce_stride;
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->product_valid = vlSelf->u_fp_mul__DOT__valid_out;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->reduce_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                 && (IData)(vlSelf->reduce_issue));
    vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    vlSelf->set0_we = 0U;
    vlSelf->set1_we = 0U;
    vlSelf->set0_waddr = 0U;
    vlSelf->set1_waddr = 0U;
    vlSelf->set0_wdata = 0U;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->set0_c1_raddr = 0U;
    vlSelf->set1_c1_raddr = 0U;
    vlSelf->set0_c0_raddr = 0U;
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__1(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe__1\n"); );
    // Body
    vlSelf->u_fp_mul__DOT__rst = vlSelf->rst;
    vlSelf->u_fp_add_accum__DOT__rst = vlSelf->rst;
    vlSelf->u_fp_add_reduce__DOT__rst = vlSelf->rst;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe__0\n"); );
    // Init
    CData/*4:0*/ __Vtableidx2;
    __Vtableidx2 = 0;
    CData/*7:0*/ __Vdly__mul_busy;
    __Vdly__mul_busy = 0;
    CData/*7:0*/ __Vdly__accum_add_busy;
    __Vdly__accum_add_busy = 0;
    CData/*7:0*/ __Vdly__reduce_add_busy;
    __Vdly__reduce_add_busy = 0;
    CData/*0:0*/ __Vdly__acc_set;
    __Vdly__acc_set = 0;
    CData/*3:0*/ __Vdly__product_bank;
    __Vdly__product_bank = 0;
    CData/*3:0*/ __Vdly__accum_wb_bank;
    __Vdly__accum_wb_bank = 0;
    CData/*1:0*/ __Vdly__red_state;
    __Vdly__red_state = 0;
    CData/*3:0*/ __Vdly__reduce_stride;
    __Vdly__reduce_stride = 0;
    CData/*3:0*/ __Vdly__reduce_i;
    __Vdly__reduce_i = 0;
    CData/*3:0*/ __Vdly__reduce_wb_i;
    __Vdly__reduce_wb_i = 0;
    CData/*3:0*/ __Vdly__reduce_todo;
    __Vdly__reduce_todo = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v0;
    __Vdlyvset__acc_bank_valid__v0 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v32;
    __Vdlyvdim0__acc_bank_valid__v32 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v32;
    __Vdlyvdim1__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v32;
    __Vdlyvset__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v33;
    __Vdlyvdim0__acc_bank_valid__v33 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v33;
    __Vdlyvdim1__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v33;
    __Vdlyvset__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v34;
    __Vdlyvdim0__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v34;
    __Vdlyvset__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v35;
    __Vdlyvdim0__acc_bank_valid__v35 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v36;
    __Vdlyvdim0__acc_bank_valid__v36 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v37;
    __Vdlyvdim0__acc_bank_valid__v37 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v38;
    __Vdlyvdim0__acc_bank_valid__v38 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v39;
    __Vdlyvdim0__acc_bank_valid__v39 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v40;
    __Vdlyvdim0__acc_bank_valid__v40 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v41;
    __Vdlyvdim0__acc_bank_valid__v41 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v42;
    __Vdlyvdim0__acc_bank_valid__v42 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v43;
    __Vdlyvdim0__acc_bank_valid__v43 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v44;
    __Vdlyvdim0__acc_bank_valid__v44 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v45;
    __Vdlyvdim0__acc_bank_valid__v45 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v46;
    __Vdlyvdim0__acc_bank_valid__v46 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v47;
    __Vdlyvdim0__acc_bank_valid__v47 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v48;
    __Vdlyvdim0__acc_bank_valid__v48 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v49;
    __Vdlyvdim0__acc_bank_valid__v49 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    // Body
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0U;
    __Vdly__mul_busy = vlSelf->mul_busy;
    __Vdly__product_bank = vlSelf->product_bank;
    __Vdly__accum_wb_bank = vlSelf->accum_wb_bank;
    __Vdly__accum_add_busy = vlSelf->accum_add_busy;
    __Vdly__reduce_add_busy = vlSelf->reduce_add_busy;
    __Vdly__reduce_stride = vlSelf->reduce_stride;
    __Vdly__reduce_wb_i = vlSelf->reduce_wb_i;
    __Vdly__acc_set = vlSelf->acc_set;
    __Vdly__reduce_i = vlSelf->reduce_i;
    __Vdly__reduce_todo = vlSelf->reduce_todo;
    __Vdly__red_state = vlSelf->red_state;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__acc_bank_valid__v0 = 0U;
    __Vdlyvset__acc_bank_valid__v32 = 0U;
    __Vdlyvset__acc_bank_valid__v33 = 0U;
    __Vdlyvset__acc_bank_valid__v34 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0U;
    if (vlSelf->rst) {
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__s = 1U;
        vlSelf->unnamedblk1__DOT__s = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xcU;
        __Vdly__mul_busy = 0U;
        __Vdly__accum_add_busy = 0U;
        __Vdly__reduce_add_busy = 0U;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__acc_bank_valid__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 1U;
        vlSelf->accum_old_valid = 0U;
        vlSelf->reduce_a_old_valid = 0U;
        vlSelf->reduce_b_old_valid = 0U;
        vlSelf->acc_out = 0U;
        vlSelf->a_reg = 0U;
        vlSelf->b_reg = 0U;
        vlSelf->accum_product_d = 0U;
        __Vdly__red_state = 0U;
        vlSelf->red_set = 0U;
        __Vdly__reduce_stride = 0U;
        __Vdly__reduce_i = 0U;
        __Vdly__reduce_wb_i = 0U;
        __Vdly__reduce_todo = 0U;
        vlSelf->final_reduce_result = 0U;
    } else {
        if (((IData)(vlSelf->pipe_pair_valid) & (~ (IData)(vlSelf->u_fp_mul__DOT__valid_out)))) {
            __Vdly__mul_busy = (0xffU & ((IData)(1U) 
                                         + (IData)(vlSelf->mul_busy)));
        } else if (((~ (IData)(vlSelf->pipe_pair_valid)) 
                    & (IData)(vlSelf->u_fp_mul__DOT__valid_out))) {
            __Vdly__mul_busy = (0xffU & ((IData)(vlSelf->mul_busy) 
                                         - (IData)(1U)));
        }
        if (((IData)(vlSelf->accum_read_valid) & (~ (IData)(vlSelf->u_fp_add_accum__DOT__valid_out)))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(1U) 
                                               + (IData)(vlSelf->accum_add_busy)));
        } else if (((~ (IData)(vlSelf->accum_read_valid)) 
                    & (IData)(vlSelf->u_fp_add_accum__DOT__valid_out))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(vlSelf->accum_add_busy) 
                                               - (IData)(1U)));
        }
        if (((IData)(vlSelf->reduce_read_valid) & (~ (IData)(vlSelf->reduce_add_valid)))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(1U) 
                                                + (IData)(vlSelf->reduce_add_busy)));
        } else if (((~ (IData)(vlSelf->reduce_read_valid)) 
                    & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(vlSelf->reduce_add_busy) 
                                                - (IData)(1U)));
        }
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = vlSelf->pipe_pair_valid;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 1U;
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = VL_MULS_III(32, vlSelf->a_reg, vlSelf->b_reg);
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 
            = (vlSelf->reduce_a_value + vlSelf->reduce_b_value);
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 
            = (vlSelf->accum_old_value + vlSelf->accum_product_d);
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 1U;
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdlyvset__acc_bank_valid__v32 = 1U;
            __Vdlyvdim1__acc_bank_valid__v32 = vlSelf->accum_wb_bank;
            __Vdlyvdim0__acc_bank_valid__v32 = vlSelf->acc_set;
        }
        if (vlSelf->reduce_add_valid) {
            __Vdlyvset__acc_bank_valid__v33 = 1U;
            __Vdlyvdim1__acc_bank_valid__v33 = vlSelf->reduce_wb_i;
            __Vdlyvdim0__acc_bank_valid__v33 = vlSelf->red_set;
        }
        if ((2U == (IData)(vlSelf->red_state))) {
            __Vdlyvset__acc_bank_valid__v34 = 1U;
            __Vdlyvdim0__acc_bank_valid__v34 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v35 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v36 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v37 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v38 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v39 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v40 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v41 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v42 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v43 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v44 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v45 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v46 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v47 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v48 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v49 = vlSelf->red_set;
        }
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 
            = vlSelf->accum_read_valid;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 
            = vlSelf->reduce_read_valid;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 1U;
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            vlSelf->accum_old_valid = ((IData)(vlSelf->acc_set)
                                        ? vlSelf->acc_bank_valid
                                       [1U][vlSelf->product_bank]
                                        : vlSelf->acc_bank_valid
                                       [0U][vlSelf->product_bank]);
            vlSelf->accum_product_d = vlSelf->u_fp_mul__DOT__result;
        }
        if (vlSelf->reduce_issue) {
            if (vlSelf->red_set) {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [1U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [1U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            } else {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [0U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [0U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            }
        }
        if (vlSelf->acc_commit_q) {
            vlSelf->acc_out = vlSelf->final_reduce_result;
        }
        if (vlSelf->a_valid_in) {
            vlSelf->a_reg = vlSelf->a_in;
        }
        if (vlSelf->b_valid_in) {
            vlSelf->b_reg = vlSelf->b_in;
        }
        if (((1U == (IData)(vlSelf->red_state)) & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_wb_i = (0xfU & ((IData)(1U) 
                                           + (IData)(vlSelf->reduce_wb_i)));
        }
        if ((0U == (IData)(vlSelf->red_state))) {
            if (vlSelf->acc_handoff) {
                vlSelf->red_set = vlSelf->acc_set;
                __Vdly__reduce_stride = 8U;
                __Vdly__reduce_todo = 8U;
                __Vdly__reduce_i = 0U;
                __Vdly__reduce_wb_i = 0U;
                __Vdly__red_state = 1U;
            }
        } else if ((1U == (IData)(vlSelf->red_state))) {
            if ((0U != (IData)(vlSelf->reduce_todo))) {
                __Vdly__reduce_todo = (0xfU & ((IData)(vlSelf->reduce_todo) 
                                               - (IData)(1U)));
                __Vdly__reduce_i = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->reduce_i)));
            } else if (((0U == (IData)(vlSelf->reduce_add_busy)) 
                        & (~ (IData)(vlSelf->reduce_read_valid)))) {
                if ((1U == (IData)(vlSelf->reduce_stride))) {
                    __Vdly__red_state = 2U;
                } else {
                    __Vdly__reduce_stride = (0xfU & 
                                             VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_todo = (0xfU & VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_i = 0U;
                    __Vdly__reduce_wb_i = 0U;
                }
            }
        } else {
            __Vdly__red_state = 0U;
        }
        if (((IData)(vlSelf->reduce_add_valid) & (1U 
                                                  == (IData)(vlSelf->reduce_stride)))) {
            vlSelf->final_reduce_result = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    if ((1U & (~ (IData)(vlSelf->rst)))) {
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xcU;
        if ((2U == (IData)(vlSelf->red_state))) {
            vlSelf->unnamedblk3__DOT__i = 1U;
            vlSelf->unnamedblk3__DOT__i = 2U;
            vlSelf->unnamedblk3__DOT__i = 3U;
            vlSelf->unnamedblk3__DOT__i = 4U;
            vlSelf->unnamedblk3__DOT__i = 5U;
            vlSelf->unnamedblk3__DOT__i = 6U;
            vlSelf->unnamedblk3__DOT__i = 7U;
            vlSelf->unnamedblk3__DOT__i = 8U;
            vlSelf->unnamedblk3__DOT__i = 9U;
            vlSelf->unnamedblk3__DOT__i = 0xaU;
            vlSelf->unnamedblk3__DOT__i = 0xbU;
            vlSelf->unnamedblk3__DOT__i = 0xcU;
            vlSelf->unnamedblk3__DOT__i = 0xdU;
            vlSelf->unnamedblk3__DOT__i = 0xeU;
            vlSelf->unnamedblk3__DOT__i = 0xfU;
            vlSelf->unnamedblk3__DOT__i = 0x10U;
        }
    }
    if (vlSelf->set1_we) {
        __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_waddr;
        __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_waddr;
    }
    if (vlSelf->set0_we) {
        __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_waddr;
        __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_waddr;
    }
    if (((IData)(vlSelf->rst) | (IData)(vlSelf->acc_handoff))) {
        __Vdly__product_bank = 0U;
        __Vdly__accum_wb_bank = 0U;
    } else {
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            __Vdly__product_bank = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->product_bank)));
        }
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdly__accum_wb_bank = (0xfU & ((IData)(1U) 
                                             + (IData)(vlSelf->accum_wb_bank)));
        }
    }
    __Vtableidx2 = (((IData)(vlSelf->pipe_pair_valid) 
                     << 4U) | (((IData)(vlSelf->acc_set) 
                                << 3U) | (((IData)(vlSelf->acc_handoff) 
                                           << 2U) | 
                                          (((IData)(vlSelf->acc_state) 
                                            << 1U) 
                                           | (IData)(vlSelf->rst)))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx2])) {
        vlSelf->acc_state = Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0
            [__Vtableidx2];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx2])) {
        __Vdly__acc_set = Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0
            [__Vtableidx2];
    }
    vlSelf->u_acc_set1_copy1__DOT__rdata = vlSelf->u_acc_set1_copy1__DOT__mem
        [vlSelf->set1_c1_raddr];
    vlSelf->u_acc_set0_copy1__DOT__rdata = vlSelf->u_acc_set0_copy1__DOT__mem
        [vlSelf->set0_c1_raddr];
    vlSelf->u_acc_set1_copy0__DOT__rdata = vlSelf->u_acc_set1_copy0__DOT__mem
        [vlSelf->set1_c0_raddr];
    vlSelf->u_acc_set0_copy0__DOT__rdata = vlSelf->u_acc_set0_copy0__DOT__mem
        [vlSelf->set0_c0_raddr];
    vlSelf->acc_valid_out = ((1U & (~ (IData)(vlSelf->rst))) 
                             && (IData)(vlSelf->acc_commit_q));
    vlSelf->b_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->b_valid_in));
    vlSelf->a_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->a_valid_in));
    vlSelf->mul_busy = __Vdly__mul_busy;
    vlSelf->accum_add_busy = __Vdly__accum_add_busy;
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v0) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v9) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v10) {
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v11) {
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v12) {
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v13) {
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v14) {
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v15) {
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v16) {
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v17) {
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v0) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v9) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v10) {
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v11) {
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v12) {
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v13) {
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v14) {
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v15) {
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v16) {
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v17) {
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    }
    vlSelf->accum_wb_bank = __Vdly__accum_wb_bank;
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_acc_set1_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set1_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    }
    vlSelf->product_bank = __Vdly__product_bank;
    if (__Vdlyvset__acc_bank_valid__v0) {
        vlSelf->acc_bank_valid[0U][0U] = 0U;
        vlSelf->acc_bank_valid[0U][1U] = 0U;
        vlSelf->acc_bank_valid[0U][2U] = 0U;
        vlSelf->acc_bank_valid[0U][3U] = 0U;
        vlSelf->acc_bank_valid[0U][4U] = 0U;
        vlSelf->acc_bank_valid[0U][5U] = 0U;
        vlSelf->acc_bank_valid[0U][6U] = 0U;
        vlSelf->acc_bank_valid[0U][7U] = 0U;
        vlSelf->acc_bank_valid[0U][8U] = 0U;
        vlSelf->acc_bank_valid[0U][9U] = 0U;
        vlSelf->acc_bank_valid[0U][0xaU] = 0U;
        vlSelf->acc_bank_valid[0U][0xbU] = 0U;
        vlSelf->acc_bank_valid[0U][0xcU] = 0U;
        vlSelf->acc_bank_valid[0U][0xdU] = 0U;
        vlSelf->acc_bank_valid[0U][0xeU] = 0U;
        vlSelf->acc_bank_valid[0U][0xfU] = 0U;
        vlSelf->acc_bank_valid[1U][0U] = 0U;
        vlSelf->acc_bank_valid[1U][1U] = 0U;
        vlSelf->acc_bank_valid[1U][2U] = 0U;
        vlSelf->acc_bank_valid[1U][3U] = 0U;
        vlSelf->acc_bank_valid[1U][4U] = 0U;
        vlSelf->acc_bank_valid[1U][5U] = 0U;
        vlSelf->acc_bank_valid[1U][6U] = 0U;
        vlSelf->acc_bank_valid[1U][7U] = 0U;
        vlSelf->acc_bank_valid[1U][8U] = 0U;
        vlSelf->acc_bank_valid[1U][9U] = 0U;
        vlSelf->acc_bank_valid[1U][0xaU] = 0U;
        vlSelf->acc_bank_valid[1U][0xbU] = 0U;
        vlSelf->acc_bank_valid[1U][0xcU] = 0U;
        vlSelf->acc_bank_valid[1U][0xdU] = 0U;
        vlSelf->acc_bank_valid[1U][0xeU] = 0U;
        vlSelf->acc_bank_valid[1U][0xfU] = 0U;
    }
    if (__Vdlyvset__acc_bank_valid__v32) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v32][__Vdlyvdim1__acc_bank_valid__v32] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v33) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v33][__Vdlyvdim1__acc_bank_valid__v33] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v34) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v34][0U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v35][1U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v36][2U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v37][3U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v38][4U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v39][5U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v40][6U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v41][7U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v42][8U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v43][9U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v44][0xaU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v45][0xbU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v46][0xcU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v47][0xdU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v48][0xeU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v49][0xfU] = 0U;
    }
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->accum_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                && (IData)(vlSelf->u_fp_mul__DOT__valid_out));
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->acc_commit_q = ((~ (IData)(vlSelf->rst)) 
                            & (IData)(vlSelf->final_reduction_complete));
    vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->accum_add_valid = vlSelf->u_fp_add_accum__DOT__valid_out;
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->reduce_add_busy = __Vdly__reduce_add_busy;
    vlSelf->reduce_wb_i = __Vdly__reduce_wb_i;
    vlSelf->reduce_i = __Vdly__reduce_i;
    vlSelf->reduce_todo = __Vdly__reduce_todo;
    vlSelf->red_state = __Vdly__red_state;
    vlSelf->acc_set = __Vdly__acc_set;
    vlSelf->reduce_stride = __Vdly__reduce_stride;
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->product_valid = vlSelf->u_fp_mul__DOT__valid_out;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->reduce_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                 && (IData)(vlSelf->reduce_issue));
    vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    vlSelf->set0_we = 0U;
    vlSelf->set1_we = 0U;
    vlSelf->set0_waddr = 0U;
    vlSelf->set1_waddr = 0U;
    vlSelf->set0_wdata = 0U;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->set0_c1_raddr = 0U;
    vlSelf->set1_c1_raddr = 0U;
    vlSelf->set0_c0_raddr = 0U;
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe__0\n"); );
    // Init
    CData/*4:0*/ __Vtableidx3;
    __Vtableidx3 = 0;
    CData/*7:0*/ __Vdly__mul_busy;
    __Vdly__mul_busy = 0;
    CData/*7:0*/ __Vdly__accum_add_busy;
    __Vdly__accum_add_busy = 0;
    CData/*7:0*/ __Vdly__reduce_add_busy;
    __Vdly__reduce_add_busy = 0;
    CData/*0:0*/ __Vdly__acc_set;
    __Vdly__acc_set = 0;
    CData/*3:0*/ __Vdly__product_bank;
    __Vdly__product_bank = 0;
    CData/*3:0*/ __Vdly__accum_wb_bank;
    __Vdly__accum_wb_bank = 0;
    CData/*1:0*/ __Vdly__red_state;
    __Vdly__red_state = 0;
    CData/*3:0*/ __Vdly__reduce_stride;
    __Vdly__reduce_stride = 0;
    CData/*3:0*/ __Vdly__reduce_i;
    __Vdly__reduce_i = 0;
    CData/*3:0*/ __Vdly__reduce_wb_i;
    __Vdly__reduce_wb_i = 0;
    CData/*3:0*/ __Vdly__reduce_todo;
    __Vdly__reduce_todo = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v0;
    __Vdlyvset__acc_bank_valid__v0 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v32;
    __Vdlyvdim0__acc_bank_valid__v32 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v32;
    __Vdlyvdim1__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v32;
    __Vdlyvset__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v33;
    __Vdlyvdim0__acc_bank_valid__v33 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v33;
    __Vdlyvdim1__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v33;
    __Vdlyvset__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v34;
    __Vdlyvdim0__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v34;
    __Vdlyvset__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v35;
    __Vdlyvdim0__acc_bank_valid__v35 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v36;
    __Vdlyvdim0__acc_bank_valid__v36 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v37;
    __Vdlyvdim0__acc_bank_valid__v37 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v38;
    __Vdlyvdim0__acc_bank_valid__v38 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v39;
    __Vdlyvdim0__acc_bank_valid__v39 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v40;
    __Vdlyvdim0__acc_bank_valid__v40 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v41;
    __Vdlyvdim0__acc_bank_valid__v41 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v42;
    __Vdlyvdim0__acc_bank_valid__v42 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v43;
    __Vdlyvdim0__acc_bank_valid__v43 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v44;
    __Vdlyvdim0__acc_bank_valid__v44 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v45;
    __Vdlyvdim0__acc_bank_valid__v45 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v46;
    __Vdlyvdim0__acc_bank_valid__v46 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v47;
    __Vdlyvdim0__acc_bank_valid__v47 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v48;
    __Vdlyvdim0__acc_bank_valid__v48 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v49;
    __Vdlyvdim0__acc_bank_valid__v49 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    // Body
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0U;
    __Vdly__mul_busy = vlSelf->mul_busy;
    __Vdly__product_bank = vlSelf->product_bank;
    __Vdly__accum_wb_bank = vlSelf->accum_wb_bank;
    __Vdly__accum_add_busy = vlSelf->accum_add_busy;
    __Vdly__reduce_add_busy = vlSelf->reduce_add_busy;
    __Vdly__reduce_stride = vlSelf->reduce_stride;
    __Vdly__reduce_wb_i = vlSelf->reduce_wb_i;
    __Vdly__acc_set = vlSelf->acc_set;
    __Vdly__reduce_i = vlSelf->reduce_i;
    __Vdly__reduce_todo = vlSelf->reduce_todo;
    __Vdly__red_state = vlSelf->red_state;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__acc_bank_valid__v0 = 0U;
    __Vdlyvset__acc_bank_valid__v32 = 0U;
    __Vdlyvset__acc_bank_valid__v33 = 0U;
    __Vdlyvset__acc_bank_valid__v34 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0U;
    if (vlSelf->rst) {
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__s = 1U;
        vlSelf->unnamedblk1__DOT__s = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xcU;
        __Vdly__mul_busy = 0U;
        __Vdly__accum_add_busy = 0U;
        __Vdly__reduce_add_busy = 0U;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__acc_bank_valid__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 1U;
        vlSelf->accum_old_valid = 0U;
        vlSelf->reduce_a_old_valid = 0U;
        vlSelf->reduce_b_old_valid = 0U;
        vlSelf->acc_out = 0U;
        vlSelf->a_reg = 0U;
        vlSelf->b_reg = 0U;
        vlSelf->accum_product_d = 0U;
        __Vdly__red_state = 0U;
        vlSelf->red_set = 0U;
        __Vdly__reduce_stride = 0U;
        __Vdly__reduce_i = 0U;
        __Vdly__reduce_wb_i = 0U;
        __Vdly__reduce_todo = 0U;
        vlSelf->final_reduce_result = 0U;
    } else {
        if (((IData)(vlSelf->pipe_pair_valid) & (~ (IData)(vlSelf->u_fp_mul__DOT__valid_out)))) {
            __Vdly__mul_busy = (0xffU & ((IData)(1U) 
                                         + (IData)(vlSelf->mul_busy)));
        } else if (((~ (IData)(vlSelf->pipe_pair_valid)) 
                    & (IData)(vlSelf->u_fp_mul__DOT__valid_out))) {
            __Vdly__mul_busy = (0xffU & ((IData)(vlSelf->mul_busy) 
                                         - (IData)(1U)));
        }
        if (((IData)(vlSelf->accum_read_valid) & (~ (IData)(vlSelf->u_fp_add_accum__DOT__valid_out)))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(1U) 
                                               + (IData)(vlSelf->accum_add_busy)));
        } else if (((~ (IData)(vlSelf->accum_read_valid)) 
                    & (IData)(vlSelf->u_fp_add_accum__DOT__valid_out))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(vlSelf->accum_add_busy) 
                                               - (IData)(1U)));
        }
        if (((IData)(vlSelf->reduce_read_valid) & (~ (IData)(vlSelf->reduce_add_valid)))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(1U) 
                                                + (IData)(vlSelf->reduce_add_busy)));
        } else if (((~ (IData)(vlSelf->reduce_read_valid)) 
                    & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(vlSelf->reduce_add_busy) 
                                                - (IData)(1U)));
        }
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = vlSelf->pipe_pair_valid;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 1U;
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = VL_MULS_III(32, vlSelf->a_reg, vlSelf->b_reg);
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 
            = (vlSelf->reduce_a_value + vlSelf->reduce_b_value);
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 
            = (vlSelf->accum_old_value + vlSelf->accum_product_d);
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 1U;
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdlyvset__acc_bank_valid__v32 = 1U;
            __Vdlyvdim1__acc_bank_valid__v32 = vlSelf->accum_wb_bank;
            __Vdlyvdim0__acc_bank_valid__v32 = vlSelf->acc_set;
        }
        if (vlSelf->reduce_add_valid) {
            __Vdlyvset__acc_bank_valid__v33 = 1U;
            __Vdlyvdim1__acc_bank_valid__v33 = vlSelf->reduce_wb_i;
            __Vdlyvdim0__acc_bank_valid__v33 = vlSelf->red_set;
        }
        if ((2U == (IData)(vlSelf->red_state))) {
            __Vdlyvset__acc_bank_valid__v34 = 1U;
            __Vdlyvdim0__acc_bank_valid__v34 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v35 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v36 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v37 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v38 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v39 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v40 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v41 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v42 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v43 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v44 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v45 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v46 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v47 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v48 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v49 = vlSelf->red_set;
        }
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 
            = vlSelf->accum_read_valid;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 
            = vlSelf->reduce_read_valid;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 1U;
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            vlSelf->accum_old_valid = ((IData)(vlSelf->acc_set)
                                        ? vlSelf->acc_bank_valid
                                       [1U][vlSelf->product_bank]
                                        : vlSelf->acc_bank_valid
                                       [0U][vlSelf->product_bank]);
            vlSelf->accum_product_d = vlSelf->u_fp_mul__DOT__result;
        }
        if (vlSelf->reduce_issue) {
            if (vlSelf->red_set) {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [1U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [1U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            } else {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [0U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [0U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            }
        }
        if (vlSelf->acc_commit_q) {
            vlSelf->acc_out = vlSelf->final_reduce_result;
        }
        if (vlSelf->a_valid_in) {
            vlSelf->a_reg = vlSelf->a_in;
        }
        if (vlSelf->b_valid_in) {
            vlSelf->b_reg = vlSelf->b_in;
        }
        if (((1U == (IData)(vlSelf->red_state)) & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_wb_i = (0xfU & ((IData)(1U) 
                                           + (IData)(vlSelf->reduce_wb_i)));
        }
        if ((0U == (IData)(vlSelf->red_state))) {
            if (vlSelf->acc_handoff) {
                vlSelf->red_set = vlSelf->acc_set;
                __Vdly__reduce_stride = 8U;
                __Vdly__reduce_todo = 8U;
                __Vdly__reduce_i = 0U;
                __Vdly__reduce_wb_i = 0U;
                __Vdly__red_state = 1U;
            }
        } else if ((1U == (IData)(vlSelf->red_state))) {
            if ((0U != (IData)(vlSelf->reduce_todo))) {
                __Vdly__reduce_todo = (0xfU & ((IData)(vlSelf->reduce_todo) 
                                               - (IData)(1U)));
                __Vdly__reduce_i = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->reduce_i)));
            } else if (((0U == (IData)(vlSelf->reduce_add_busy)) 
                        & (~ (IData)(vlSelf->reduce_read_valid)))) {
                if ((1U == (IData)(vlSelf->reduce_stride))) {
                    __Vdly__red_state = 2U;
                } else {
                    __Vdly__reduce_stride = (0xfU & 
                                             VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_todo = (0xfU & VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_i = 0U;
                    __Vdly__reduce_wb_i = 0U;
                }
            }
        } else {
            __Vdly__red_state = 0U;
        }
        if (((IData)(vlSelf->reduce_add_valid) & (1U 
                                                  == (IData)(vlSelf->reduce_stride)))) {
            vlSelf->final_reduce_result = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    if ((1U & (~ (IData)(vlSelf->rst)))) {
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xcU;
        if ((2U == (IData)(vlSelf->red_state))) {
            vlSelf->unnamedblk3__DOT__i = 1U;
            vlSelf->unnamedblk3__DOT__i = 2U;
            vlSelf->unnamedblk3__DOT__i = 3U;
            vlSelf->unnamedblk3__DOT__i = 4U;
            vlSelf->unnamedblk3__DOT__i = 5U;
            vlSelf->unnamedblk3__DOT__i = 6U;
            vlSelf->unnamedblk3__DOT__i = 7U;
            vlSelf->unnamedblk3__DOT__i = 8U;
            vlSelf->unnamedblk3__DOT__i = 9U;
            vlSelf->unnamedblk3__DOT__i = 0xaU;
            vlSelf->unnamedblk3__DOT__i = 0xbU;
            vlSelf->unnamedblk3__DOT__i = 0xcU;
            vlSelf->unnamedblk3__DOT__i = 0xdU;
            vlSelf->unnamedblk3__DOT__i = 0xeU;
            vlSelf->unnamedblk3__DOT__i = 0xfU;
            vlSelf->unnamedblk3__DOT__i = 0x10U;
        }
    }
    if (vlSelf->set1_we) {
        __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_waddr;
        __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_waddr;
    }
    if (vlSelf->set0_we) {
        __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_waddr;
        __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_waddr;
    }
    if (((IData)(vlSelf->rst) | (IData)(vlSelf->acc_handoff))) {
        __Vdly__product_bank = 0U;
        __Vdly__accum_wb_bank = 0U;
    } else {
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            __Vdly__product_bank = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->product_bank)));
        }
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdly__accum_wb_bank = (0xfU & ((IData)(1U) 
                                             + (IData)(vlSelf->accum_wb_bank)));
        }
    }
    __Vtableidx3 = (((IData)(vlSelf->pipe_pair_valid) 
                     << 4U) | (((IData)(vlSelf->acc_set) 
                                << 3U) | (((IData)(vlSelf->acc_handoff) 
                                           << 2U) | 
                                          (((IData)(vlSelf->acc_state) 
                                            << 1U) 
                                           | (IData)(vlSelf->rst)))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx3])) {
        vlSelf->acc_state = Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0
            [__Vtableidx3];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx3])) {
        __Vdly__acc_set = Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0
            [__Vtableidx3];
    }
    vlSelf->u_acc_set1_copy1__DOT__rdata = vlSelf->u_acc_set1_copy1__DOT__mem
        [vlSelf->set1_c1_raddr];
    vlSelf->u_acc_set0_copy1__DOT__rdata = vlSelf->u_acc_set0_copy1__DOT__mem
        [vlSelf->set0_c1_raddr];
    vlSelf->u_acc_set1_copy0__DOT__rdata = vlSelf->u_acc_set1_copy0__DOT__mem
        [vlSelf->set1_c0_raddr];
    vlSelf->u_acc_set0_copy0__DOT__rdata = vlSelf->u_acc_set0_copy0__DOT__mem
        [vlSelf->set0_c0_raddr];
    vlSelf->acc_valid_out = ((1U & (~ (IData)(vlSelf->rst))) 
                             && (IData)(vlSelf->acc_commit_q));
    vlSelf->b_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->b_valid_in));
    vlSelf->a_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->a_valid_in));
    vlSelf->mul_busy = __Vdly__mul_busy;
    vlSelf->accum_add_busy = __Vdly__accum_add_busy;
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v0) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v9) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v10) {
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v11) {
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v12) {
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v13) {
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v14) {
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v15) {
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v16) {
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v17) {
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v0) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v9) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v10) {
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v11) {
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v12) {
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v13) {
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v14) {
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v15) {
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v16) {
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v17) {
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    }
    vlSelf->accum_wb_bank = __Vdly__accum_wb_bank;
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_acc_set1_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set1_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    }
    vlSelf->product_bank = __Vdly__product_bank;
    if (__Vdlyvset__acc_bank_valid__v0) {
        vlSelf->acc_bank_valid[0U][0U] = 0U;
        vlSelf->acc_bank_valid[0U][1U] = 0U;
        vlSelf->acc_bank_valid[0U][2U] = 0U;
        vlSelf->acc_bank_valid[0U][3U] = 0U;
        vlSelf->acc_bank_valid[0U][4U] = 0U;
        vlSelf->acc_bank_valid[0U][5U] = 0U;
        vlSelf->acc_bank_valid[0U][6U] = 0U;
        vlSelf->acc_bank_valid[0U][7U] = 0U;
        vlSelf->acc_bank_valid[0U][8U] = 0U;
        vlSelf->acc_bank_valid[0U][9U] = 0U;
        vlSelf->acc_bank_valid[0U][0xaU] = 0U;
        vlSelf->acc_bank_valid[0U][0xbU] = 0U;
        vlSelf->acc_bank_valid[0U][0xcU] = 0U;
        vlSelf->acc_bank_valid[0U][0xdU] = 0U;
        vlSelf->acc_bank_valid[0U][0xeU] = 0U;
        vlSelf->acc_bank_valid[0U][0xfU] = 0U;
        vlSelf->acc_bank_valid[1U][0U] = 0U;
        vlSelf->acc_bank_valid[1U][1U] = 0U;
        vlSelf->acc_bank_valid[1U][2U] = 0U;
        vlSelf->acc_bank_valid[1U][3U] = 0U;
        vlSelf->acc_bank_valid[1U][4U] = 0U;
        vlSelf->acc_bank_valid[1U][5U] = 0U;
        vlSelf->acc_bank_valid[1U][6U] = 0U;
        vlSelf->acc_bank_valid[1U][7U] = 0U;
        vlSelf->acc_bank_valid[1U][8U] = 0U;
        vlSelf->acc_bank_valid[1U][9U] = 0U;
        vlSelf->acc_bank_valid[1U][0xaU] = 0U;
        vlSelf->acc_bank_valid[1U][0xbU] = 0U;
        vlSelf->acc_bank_valid[1U][0xcU] = 0U;
        vlSelf->acc_bank_valid[1U][0xdU] = 0U;
        vlSelf->acc_bank_valid[1U][0xeU] = 0U;
        vlSelf->acc_bank_valid[1U][0xfU] = 0U;
    }
    if (__Vdlyvset__acc_bank_valid__v32) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v32][__Vdlyvdim1__acc_bank_valid__v32] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v33) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v33][__Vdlyvdim1__acc_bank_valid__v33] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v34) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v34][0U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v35][1U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v36][2U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v37][3U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v38][4U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v39][5U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v40][6U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v41][7U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v42][8U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v43][9U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v44][0xaU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v45][0xbU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v46][0xcU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v47][0xdU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v48][0xeU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v49][0xfU] = 0U;
    }
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->accum_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                && (IData)(vlSelf->u_fp_mul__DOT__valid_out));
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->acc_commit_q = ((~ (IData)(vlSelf->rst)) 
                            & (IData)(vlSelf->final_reduction_complete));
    vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->accum_add_valid = vlSelf->u_fp_add_accum__DOT__valid_out;
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->reduce_add_busy = __Vdly__reduce_add_busy;
    vlSelf->reduce_wb_i = __Vdly__reduce_wb_i;
    vlSelf->reduce_i = __Vdly__reduce_i;
    vlSelf->reduce_todo = __Vdly__reduce_todo;
    vlSelf->red_state = __Vdly__red_state;
    vlSelf->acc_set = __Vdly__acc_set;
    vlSelf->reduce_stride = __Vdly__reduce_stride;
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->product_valid = vlSelf->u_fp_mul__DOT__valid_out;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->reduce_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                 && (IData)(vlSelf->reduce_issue));
    vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    vlSelf->set0_we = 0U;
    vlSelf->set1_we = 0U;
    vlSelf->set0_waddr = 0U;
    vlSelf->set1_waddr = 0U;
    vlSelf->set0_wdata = 0U;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->set0_c1_raddr = 0U;
    vlSelf->set1_c1_raddr = 0U;
    vlSelf->set0_c0_raddr = 0U;
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe__0\n"); );
    // Init
    CData/*4:0*/ __Vtableidx4;
    __Vtableidx4 = 0;
    CData/*7:0*/ __Vdly__mul_busy;
    __Vdly__mul_busy = 0;
    CData/*7:0*/ __Vdly__accum_add_busy;
    __Vdly__accum_add_busy = 0;
    CData/*7:0*/ __Vdly__reduce_add_busy;
    __Vdly__reduce_add_busy = 0;
    CData/*0:0*/ __Vdly__acc_set;
    __Vdly__acc_set = 0;
    CData/*3:0*/ __Vdly__product_bank;
    __Vdly__product_bank = 0;
    CData/*3:0*/ __Vdly__accum_wb_bank;
    __Vdly__accum_wb_bank = 0;
    CData/*1:0*/ __Vdly__red_state;
    __Vdly__red_state = 0;
    CData/*3:0*/ __Vdly__reduce_stride;
    __Vdly__reduce_stride = 0;
    CData/*3:0*/ __Vdly__reduce_i;
    __Vdly__reduce_i = 0;
    CData/*3:0*/ __Vdly__reduce_wb_i;
    __Vdly__reduce_wb_i = 0;
    CData/*3:0*/ __Vdly__reduce_todo;
    __Vdly__reduce_todo = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v0;
    __Vdlyvset__acc_bank_valid__v0 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v32;
    __Vdlyvdim0__acc_bank_valid__v32 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v32;
    __Vdlyvdim1__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v32;
    __Vdlyvset__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v33;
    __Vdlyvdim0__acc_bank_valid__v33 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v33;
    __Vdlyvdim1__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v33;
    __Vdlyvset__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v34;
    __Vdlyvdim0__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v34;
    __Vdlyvset__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v35;
    __Vdlyvdim0__acc_bank_valid__v35 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v36;
    __Vdlyvdim0__acc_bank_valid__v36 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v37;
    __Vdlyvdim0__acc_bank_valid__v37 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v38;
    __Vdlyvdim0__acc_bank_valid__v38 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v39;
    __Vdlyvdim0__acc_bank_valid__v39 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v40;
    __Vdlyvdim0__acc_bank_valid__v40 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v41;
    __Vdlyvdim0__acc_bank_valid__v41 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v42;
    __Vdlyvdim0__acc_bank_valid__v42 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v43;
    __Vdlyvdim0__acc_bank_valid__v43 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v44;
    __Vdlyvdim0__acc_bank_valid__v44 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v45;
    __Vdlyvdim0__acc_bank_valid__v45 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v46;
    __Vdlyvdim0__acc_bank_valid__v46 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v47;
    __Vdlyvdim0__acc_bank_valid__v47 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v48;
    __Vdlyvdim0__acc_bank_valid__v48 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v49;
    __Vdlyvdim0__acc_bank_valid__v49 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    // Body
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0U;
    __Vdly__mul_busy = vlSelf->mul_busy;
    __Vdly__product_bank = vlSelf->product_bank;
    __Vdly__accum_wb_bank = vlSelf->accum_wb_bank;
    __Vdly__accum_add_busy = vlSelf->accum_add_busy;
    __Vdly__reduce_add_busy = vlSelf->reduce_add_busy;
    __Vdly__reduce_stride = vlSelf->reduce_stride;
    __Vdly__reduce_wb_i = vlSelf->reduce_wb_i;
    __Vdly__acc_set = vlSelf->acc_set;
    __Vdly__reduce_i = vlSelf->reduce_i;
    __Vdly__reduce_todo = vlSelf->reduce_todo;
    __Vdly__red_state = vlSelf->red_state;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__acc_bank_valid__v0 = 0U;
    __Vdlyvset__acc_bank_valid__v32 = 0U;
    __Vdlyvset__acc_bank_valid__v33 = 0U;
    __Vdlyvset__acc_bank_valid__v34 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0U;
    if (vlSelf->rst) {
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__s = 1U;
        vlSelf->unnamedblk1__DOT__s = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xcU;
        __Vdly__mul_busy = 0U;
        __Vdly__accum_add_busy = 0U;
        __Vdly__reduce_add_busy = 0U;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__acc_bank_valid__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 1U;
        vlSelf->accum_old_valid = 0U;
        vlSelf->reduce_a_old_valid = 0U;
        vlSelf->reduce_b_old_valid = 0U;
        vlSelf->acc_out = 0U;
        vlSelf->a_reg = 0U;
        vlSelf->b_reg = 0U;
        vlSelf->accum_product_d = 0U;
        __Vdly__red_state = 0U;
        vlSelf->red_set = 0U;
        __Vdly__reduce_stride = 0U;
        __Vdly__reduce_i = 0U;
        __Vdly__reduce_wb_i = 0U;
        __Vdly__reduce_todo = 0U;
        vlSelf->final_reduce_result = 0U;
    } else {
        if (((IData)(vlSelf->pipe_pair_valid) & (~ (IData)(vlSelf->u_fp_mul__DOT__valid_out)))) {
            __Vdly__mul_busy = (0xffU & ((IData)(1U) 
                                         + (IData)(vlSelf->mul_busy)));
        } else if (((~ (IData)(vlSelf->pipe_pair_valid)) 
                    & (IData)(vlSelf->u_fp_mul__DOT__valid_out))) {
            __Vdly__mul_busy = (0xffU & ((IData)(vlSelf->mul_busy) 
                                         - (IData)(1U)));
        }
        if (((IData)(vlSelf->accum_read_valid) & (~ (IData)(vlSelf->u_fp_add_accum__DOT__valid_out)))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(1U) 
                                               + (IData)(vlSelf->accum_add_busy)));
        } else if (((~ (IData)(vlSelf->accum_read_valid)) 
                    & (IData)(vlSelf->u_fp_add_accum__DOT__valid_out))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(vlSelf->accum_add_busy) 
                                               - (IData)(1U)));
        }
        if (((IData)(vlSelf->reduce_read_valid) & (~ (IData)(vlSelf->reduce_add_valid)))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(1U) 
                                                + (IData)(vlSelf->reduce_add_busy)));
        } else if (((~ (IData)(vlSelf->reduce_read_valid)) 
                    & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(vlSelf->reduce_add_busy) 
                                                - (IData)(1U)));
        }
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = vlSelf->pipe_pair_valid;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 1U;
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = VL_MULS_III(32, vlSelf->a_reg, vlSelf->b_reg);
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 
            = (vlSelf->reduce_a_value + vlSelf->reduce_b_value);
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 
            = (vlSelf->accum_old_value + vlSelf->accum_product_d);
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 1U;
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdlyvset__acc_bank_valid__v32 = 1U;
            __Vdlyvdim1__acc_bank_valid__v32 = vlSelf->accum_wb_bank;
            __Vdlyvdim0__acc_bank_valid__v32 = vlSelf->acc_set;
        }
        if (vlSelf->reduce_add_valid) {
            __Vdlyvset__acc_bank_valid__v33 = 1U;
            __Vdlyvdim1__acc_bank_valid__v33 = vlSelf->reduce_wb_i;
            __Vdlyvdim0__acc_bank_valid__v33 = vlSelf->red_set;
        }
        if ((2U == (IData)(vlSelf->red_state))) {
            __Vdlyvset__acc_bank_valid__v34 = 1U;
            __Vdlyvdim0__acc_bank_valid__v34 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v35 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v36 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v37 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v38 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v39 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v40 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v41 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v42 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v43 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v44 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v45 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v46 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v47 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v48 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v49 = vlSelf->red_set;
        }
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 
            = vlSelf->accum_read_valid;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 
            = vlSelf->reduce_read_valid;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 1U;
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            vlSelf->accum_old_valid = ((IData)(vlSelf->acc_set)
                                        ? vlSelf->acc_bank_valid
                                       [1U][vlSelf->product_bank]
                                        : vlSelf->acc_bank_valid
                                       [0U][vlSelf->product_bank]);
            vlSelf->accum_product_d = vlSelf->u_fp_mul__DOT__result;
        }
        if (vlSelf->reduce_issue) {
            if (vlSelf->red_set) {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [1U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [1U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            } else {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [0U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [0U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            }
        }
        if (vlSelf->acc_commit_q) {
            vlSelf->acc_out = vlSelf->final_reduce_result;
        }
        if (vlSelf->a_valid_in) {
            vlSelf->a_reg = vlSelf->a_in;
        }
        if (vlSelf->b_valid_in) {
            vlSelf->b_reg = vlSelf->b_in;
        }
        if (((1U == (IData)(vlSelf->red_state)) & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_wb_i = (0xfU & ((IData)(1U) 
                                           + (IData)(vlSelf->reduce_wb_i)));
        }
        if ((0U == (IData)(vlSelf->red_state))) {
            if (vlSelf->acc_handoff) {
                vlSelf->red_set = vlSelf->acc_set;
                __Vdly__reduce_stride = 8U;
                __Vdly__reduce_todo = 8U;
                __Vdly__reduce_i = 0U;
                __Vdly__reduce_wb_i = 0U;
                __Vdly__red_state = 1U;
            }
        } else if ((1U == (IData)(vlSelf->red_state))) {
            if ((0U != (IData)(vlSelf->reduce_todo))) {
                __Vdly__reduce_todo = (0xfU & ((IData)(vlSelf->reduce_todo) 
                                               - (IData)(1U)));
                __Vdly__reduce_i = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->reduce_i)));
            } else if (((0U == (IData)(vlSelf->reduce_add_busy)) 
                        & (~ (IData)(vlSelf->reduce_read_valid)))) {
                if ((1U == (IData)(vlSelf->reduce_stride))) {
                    __Vdly__red_state = 2U;
                } else {
                    __Vdly__reduce_stride = (0xfU & 
                                             VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_todo = (0xfU & VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_i = 0U;
                    __Vdly__reduce_wb_i = 0U;
                }
            }
        } else {
            __Vdly__red_state = 0U;
        }
        if (((IData)(vlSelf->reduce_add_valid) & (1U 
                                                  == (IData)(vlSelf->reduce_stride)))) {
            vlSelf->final_reduce_result = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    if ((1U & (~ (IData)(vlSelf->rst)))) {
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xcU;
        if ((2U == (IData)(vlSelf->red_state))) {
            vlSelf->unnamedblk3__DOT__i = 1U;
            vlSelf->unnamedblk3__DOT__i = 2U;
            vlSelf->unnamedblk3__DOT__i = 3U;
            vlSelf->unnamedblk3__DOT__i = 4U;
            vlSelf->unnamedblk3__DOT__i = 5U;
            vlSelf->unnamedblk3__DOT__i = 6U;
            vlSelf->unnamedblk3__DOT__i = 7U;
            vlSelf->unnamedblk3__DOT__i = 8U;
            vlSelf->unnamedblk3__DOT__i = 9U;
            vlSelf->unnamedblk3__DOT__i = 0xaU;
            vlSelf->unnamedblk3__DOT__i = 0xbU;
            vlSelf->unnamedblk3__DOT__i = 0xcU;
            vlSelf->unnamedblk3__DOT__i = 0xdU;
            vlSelf->unnamedblk3__DOT__i = 0xeU;
            vlSelf->unnamedblk3__DOT__i = 0xfU;
            vlSelf->unnamedblk3__DOT__i = 0x10U;
        }
    }
    if (vlSelf->set1_we) {
        __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_waddr;
        __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_waddr;
    }
    if (vlSelf->set0_we) {
        __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_waddr;
        __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_waddr;
    }
    if (((IData)(vlSelf->rst) | (IData)(vlSelf->acc_handoff))) {
        __Vdly__product_bank = 0U;
        __Vdly__accum_wb_bank = 0U;
    } else {
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            __Vdly__product_bank = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->product_bank)));
        }
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdly__accum_wb_bank = (0xfU & ((IData)(1U) 
                                             + (IData)(vlSelf->accum_wb_bank)));
        }
    }
    __Vtableidx4 = (((IData)(vlSelf->pipe_pair_valid) 
                     << 4U) | (((IData)(vlSelf->acc_set) 
                                << 3U) | (((IData)(vlSelf->acc_handoff) 
                                           << 2U) | 
                                          (((IData)(vlSelf->acc_state) 
                                            << 1U) 
                                           | (IData)(vlSelf->rst)))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx4])) {
        vlSelf->acc_state = Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0
            [__Vtableidx4];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx4])) {
        __Vdly__acc_set = Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0
            [__Vtableidx4];
    }
    vlSelf->u_acc_set1_copy1__DOT__rdata = vlSelf->u_acc_set1_copy1__DOT__mem
        [vlSelf->set1_c1_raddr];
    vlSelf->u_acc_set0_copy1__DOT__rdata = vlSelf->u_acc_set0_copy1__DOT__mem
        [vlSelf->set0_c1_raddr];
    vlSelf->u_acc_set1_copy0__DOT__rdata = vlSelf->u_acc_set1_copy0__DOT__mem
        [vlSelf->set1_c0_raddr];
    vlSelf->u_acc_set0_copy0__DOT__rdata = vlSelf->u_acc_set0_copy0__DOT__mem
        [vlSelf->set0_c0_raddr];
    vlSelf->acc_valid_out = ((1U & (~ (IData)(vlSelf->rst))) 
                             && (IData)(vlSelf->acc_commit_q));
    vlSelf->b_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->b_valid_in));
    vlSelf->a_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->a_valid_in));
    vlSelf->mul_busy = __Vdly__mul_busy;
    vlSelf->accum_add_busy = __Vdly__accum_add_busy;
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v0) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v9) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v10) {
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v11) {
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v12) {
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v13) {
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v14) {
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v15) {
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v16) {
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v17) {
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v0) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v9) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v10) {
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v11) {
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v12) {
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v13) {
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v14) {
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v15) {
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v16) {
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v17) {
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    }
    vlSelf->accum_wb_bank = __Vdly__accum_wb_bank;
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_acc_set1_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set1_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    }
    vlSelf->product_bank = __Vdly__product_bank;
    if (__Vdlyvset__acc_bank_valid__v0) {
        vlSelf->acc_bank_valid[0U][0U] = 0U;
        vlSelf->acc_bank_valid[0U][1U] = 0U;
        vlSelf->acc_bank_valid[0U][2U] = 0U;
        vlSelf->acc_bank_valid[0U][3U] = 0U;
        vlSelf->acc_bank_valid[0U][4U] = 0U;
        vlSelf->acc_bank_valid[0U][5U] = 0U;
        vlSelf->acc_bank_valid[0U][6U] = 0U;
        vlSelf->acc_bank_valid[0U][7U] = 0U;
        vlSelf->acc_bank_valid[0U][8U] = 0U;
        vlSelf->acc_bank_valid[0U][9U] = 0U;
        vlSelf->acc_bank_valid[0U][0xaU] = 0U;
        vlSelf->acc_bank_valid[0U][0xbU] = 0U;
        vlSelf->acc_bank_valid[0U][0xcU] = 0U;
        vlSelf->acc_bank_valid[0U][0xdU] = 0U;
        vlSelf->acc_bank_valid[0U][0xeU] = 0U;
        vlSelf->acc_bank_valid[0U][0xfU] = 0U;
        vlSelf->acc_bank_valid[1U][0U] = 0U;
        vlSelf->acc_bank_valid[1U][1U] = 0U;
        vlSelf->acc_bank_valid[1U][2U] = 0U;
        vlSelf->acc_bank_valid[1U][3U] = 0U;
        vlSelf->acc_bank_valid[1U][4U] = 0U;
        vlSelf->acc_bank_valid[1U][5U] = 0U;
        vlSelf->acc_bank_valid[1U][6U] = 0U;
        vlSelf->acc_bank_valid[1U][7U] = 0U;
        vlSelf->acc_bank_valid[1U][8U] = 0U;
        vlSelf->acc_bank_valid[1U][9U] = 0U;
        vlSelf->acc_bank_valid[1U][0xaU] = 0U;
        vlSelf->acc_bank_valid[1U][0xbU] = 0U;
        vlSelf->acc_bank_valid[1U][0xcU] = 0U;
        vlSelf->acc_bank_valid[1U][0xdU] = 0U;
        vlSelf->acc_bank_valid[1U][0xeU] = 0U;
        vlSelf->acc_bank_valid[1U][0xfU] = 0U;
    }
    if (__Vdlyvset__acc_bank_valid__v32) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v32][__Vdlyvdim1__acc_bank_valid__v32] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v33) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v33][__Vdlyvdim1__acc_bank_valid__v33] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v34) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v34][0U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v35][1U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v36][2U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v37][3U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v38][4U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v39][5U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v40][6U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v41][7U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v42][8U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v43][9U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v44][0xaU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v45][0xbU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v46][0xcU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v47][0xdU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v48][0xeU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v49][0xfU] = 0U;
    }
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->accum_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                && (IData)(vlSelf->u_fp_mul__DOT__valid_out));
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->acc_commit_q = ((~ (IData)(vlSelf->rst)) 
                            & (IData)(vlSelf->final_reduction_complete));
    vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->accum_add_valid = vlSelf->u_fp_add_accum__DOT__valid_out;
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->reduce_add_busy = __Vdly__reduce_add_busy;
    vlSelf->reduce_wb_i = __Vdly__reduce_wb_i;
    vlSelf->reduce_i = __Vdly__reduce_i;
    vlSelf->reduce_todo = __Vdly__reduce_todo;
    vlSelf->red_state = __Vdly__red_state;
    vlSelf->acc_set = __Vdly__acc_set;
    vlSelf->reduce_stride = __Vdly__reduce_stride;
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->product_valid = vlSelf->u_fp_mul__DOT__valid_out;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->reduce_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                 && (IData)(vlSelf->reduce_issue));
    vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    vlSelf->set0_we = 0U;
    vlSelf->set1_we = 0U;
    vlSelf->set0_waddr = 0U;
    vlSelf->set1_waddr = 0U;
    vlSelf->set0_wdata = 0U;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->set0_c1_raddr = 0U;
    vlSelf->set1_c1_raddr = 0U;
    vlSelf->set0_c0_raddr = 0U;
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
}

VL_INLINE_OPT void Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___nba_sequent__TOP__tb_systolic_dma_top__DOT__dut__DOT__ACC_4X4__BRA__0__KET____DOT__u_acc__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe__0\n"); );
    // Init
    CData/*4:0*/ __Vtableidx5;
    __Vtableidx5 = 0;
    CData/*7:0*/ __Vdly__mul_busy;
    __Vdly__mul_busy = 0;
    CData/*7:0*/ __Vdly__accum_add_busy;
    __Vdly__accum_add_busy = 0;
    CData/*7:0*/ __Vdly__reduce_add_busy;
    __Vdly__reduce_add_busy = 0;
    CData/*0:0*/ __Vdly__acc_set;
    __Vdly__acc_set = 0;
    CData/*3:0*/ __Vdly__product_bank;
    __Vdly__product_bank = 0;
    CData/*3:0*/ __Vdly__accum_wb_bank;
    __Vdly__accum_wb_bank = 0;
    CData/*1:0*/ __Vdly__red_state;
    __Vdly__red_state = 0;
    CData/*3:0*/ __Vdly__reduce_stride;
    __Vdly__reduce_stride = 0;
    CData/*3:0*/ __Vdly__reduce_i;
    __Vdly__reduce_i = 0;
    CData/*3:0*/ __Vdly__reduce_wb_i;
    __Vdly__reduce_wb_i = 0;
    CData/*3:0*/ __Vdly__reduce_todo;
    __Vdly__reduce_todo = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v0;
    __Vdlyvset__acc_bank_valid__v0 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v32;
    __Vdlyvdim0__acc_bank_valid__v32 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v32;
    __Vdlyvdim1__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v32;
    __Vdlyvset__acc_bank_valid__v32 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v33;
    __Vdlyvdim0__acc_bank_valid__v33 = 0;
    CData/*3:0*/ __Vdlyvdim1__acc_bank_valid__v33;
    __Vdlyvdim1__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v33;
    __Vdlyvset__acc_bank_valid__v33 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v34;
    __Vdlyvdim0__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvset__acc_bank_valid__v34;
    __Vdlyvset__acc_bank_valid__v34 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v35;
    __Vdlyvdim0__acc_bank_valid__v35 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v36;
    __Vdlyvdim0__acc_bank_valid__v36 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v37;
    __Vdlyvdim0__acc_bank_valid__v37 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v38;
    __Vdlyvdim0__acc_bank_valid__v38 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v39;
    __Vdlyvdim0__acc_bank_valid__v39 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v40;
    __Vdlyvdim0__acc_bank_valid__v40 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v41;
    __Vdlyvdim0__acc_bank_valid__v41 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v42;
    __Vdlyvdim0__acc_bank_valid__v42 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v43;
    __Vdlyvdim0__acc_bank_valid__v43 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v44;
    __Vdlyvdim0__acc_bank_valid__v44 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v45;
    __Vdlyvdim0__acc_bank_valid__v45 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v46;
    __Vdlyvdim0__acc_bank_valid__v46 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v47;
    __Vdlyvdim0__acc_bank_valid__v47 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v48;
    __Vdlyvdim0__acc_bank_valid__v48 = 0;
    CData/*0:0*/ __Vdlyvdim0__acc_bank_valid__v49;
    __Vdlyvdim0__acc_bank_valid__v49 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v9;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v10;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v11;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_mul__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0;
    CData/*3:0*/ __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0;
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0;
    IData/*31:0*/ __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    CData/*0:0*/ __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0;
    // Body
    __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 0U;
    __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 0U;
    __Vdly__mul_busy = vlSelf->mul_busy;
    __Vdly__product_bank = vlSelf->product_bank;
    __Vdly__accum_wb_bank = vlSelf->accum_wb_bank;
    __Vdly__accum_add_busy = vlSelf->accum_add_busy;
    __Vdly__reduce_add_busy = vlSelf->reduce_add_busy;
    __Vdly__reduce_stride = vlSelf->reduce_stride;
    __Vdly__reduce_wb_i = vlSelf->reduce_wb_i;
    __Vdly__acc_set = vlSelf->acc_set;
    __Vdly__reduce_i = vlSelf->reduce_i;
    __Vdly__reduce_todo = vlSelf->reduce_todo;
    __Vdly__red_state = vlSelf->red_state;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 0U;
    __Vdlyvset__acc_bank_valid__v0 = 0U;
    __Vdlyvset__acc_bank_valid__v32 = 0U;
    __Vdlyvset__acc_bank_valid__v33 = 0U;
    __Vdlyvset__acc_bank_valid__v34 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 0U;
    __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 0U;
    if (vlSelf->rst) {
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 1U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xdU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xeU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0xfU;
        vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0x10U;
        vlSelf->unnamedblk1__DOT__s = 1U;
        vlSelf->unnamedblk1__DOT__s = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 1U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0xcU;
        __Vdly__mul_busy = 0U;
        __Vdly__accum_add_busy = 0U;
        __Vdly__reduce_add_busy = 0U;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0 = 1U;
        __Vdlyvset__acc_bank_valid__v0 = 1U;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0 = 1U;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0 = 1U;
        vlSelf->accum_old_valid = 0U;
        vlSelf->reduce_a_old_valid = 0U;
        vlSelf->reduce_b_old_valid = 0U;
        vlSelf->acc_out = 0U;
        vlSelf->a_reg = 0U;
        vlSelf->b_reg = 0U;
        vlSelf->accum_product_d = 0U;
        __Vdly__red_state = 0U;
        vlSelf->red_set = 0U;
        __Vdly__reduce_stride = 0U;
        __Vdly__reduce_i = 0U;
        __Vdly__reduce_wb_i = 0U;
        __Vdly__reduce_todo = 0U;
        vlSelf->final_reduce_result = 0U;
    } else {
        if (((IData)(vlSelf->pipe_pair_valid) & (~ (IData)(vlSelf->u_fp_mul__DOT__valid_out)))) {
            __Vdly__mul_busy = (0xffU & ((IData)(1U) 
                                         + (IData)(vlSelf->mul_busy)));
        } else if (((~ (IData)(vlSelf->pipe_pair_valid)) 
                    & (IData)(vlSelf->u_fp_mul__DOT__valid_out))) {
            __Vdly__mul_busy = (0xffU & ((IData)(vlSelf->mul_busy) 
                                         - (IData)(1U)));
        }
        if (((IData)(vlSelf->accum_read_valid) & (~ (IData)(vlSelf->u_fp_add_accum__DOT__valid_out)))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(1U) 
                                               + (IData)(vlSelf->accum_add_busy)));
        } else if (((~ (IData)(vlSelf->accum_read_valid)) 
                    & (IData)(vlSelf->u_fp_add_accum__DOT__valid_out))) {
            __Vdly__accum_add_busy = (0xffU & ((IData)(vlSelf->accum_add_busy) 
                                               - (IData)(1U)));
        }
        if (((IData)(vlSelf->reduce_read_valid) & (~ (IData)(vlSelf->reduce_add_valid)))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(1U) 
                                                + (IData)(vlSelf->reduce_add_busy)));
        } else if (((~ (IData)(vlSelf->reduce_read_valid)) 
                    & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_add_busy = (0xffU & ((IData)(vlSelf->reduce_add_busy) 
                                                - (IData)(1U)));
        }
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v9 = vlSelf->pipe_pair_valid;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 
            = vlSelf->u_fp_mul__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__v_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        __Vdlyvset__u_fp_mul__DOT__v_pipe__v17 = 1U;
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v9 = VL_MULS_III(32, vlSelf->a_reg, vlSelf->b_reg);
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v9 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v10 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v10 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v11 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v11 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v12 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v13 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v14 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v15 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v16 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 
            = vlSelf->u_fp_mul__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_mul__DOT__d_pipe__v17 = vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0;
        __Vdlyvset__u_fp_mul__DOT__d_pipe__v17 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12 
            = (vlSelf->reduce_a_value + vlSelf->reduce_b_value);
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12 
            = (vlSelf->accum_old_value + vlSelf->accum_product_d);
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 
            = vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        __Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23 = 1U;
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdlyvset__acc_bank_valid__v32 = 1U;
            __Vdlyvdim1__acc_bank_valid__v32 = vlSelf->accum_wb_bank;
            __Vdlyvdim0__acc_bank_valid__v32 = vlSelf->acc_set;
        }
        if (vlSelf->reduce_add_valid) {
            __Vdlyvset__acc_bank_valid__v33 = 1U;
            __Vdlyvdim1__acc_bank_valid__v33 = vlSelf->reduce_wb_i;
            __Vdlyvdim0__acc_bank_valid__v33 = vlSelf->red_set;
        }
        if ((2U == (IData)(vlSelf->red_state))) {
            __Vdlyvset__acc_bank_valid__v34 = 1U;
            __Vdlyvdim0__acc_bank_valid__v34 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v35 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v36 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v37 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v38 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v39 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v40 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v41 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v42 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v43 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v44 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v45 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v46 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v47 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v48 = vlSelf->red_set;
            __Vdlyvdim0__acc_bank_valid__v49 = vlSelf->red_set;
        }
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12 
            = vlSelf->accum_read_valid;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23 = 1U;
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12 
            = vlSelf->reduce_read_valid;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[1U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[2U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[3U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[4U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[5U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[6U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[7U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[8U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[9U];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22 = 1U;
        vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 
            = vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU];
        __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23 
            = vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        __Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23 = 1U;
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            vlSelf->accum_old_valid = ((IData)(vlSelf->acc_set)
                                        ? vlSelf->acc_bank_valid
                                       [1U][vlSelf->product_bank]
                                        : vlSelf->acc_bank_valid
                                       [0U][vlSelf->product_bank]);
            vlSelf->accum_product_d = vlSelf->u_fp_mul__DOT__result;
        }
        if (vlSelf->reduce_issue) {
            if (vlSelf->red_set) {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [1U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [1U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            } else {
                vlSelf->reduce_a_old_valid = vlSelf->acc_bank_valid
                    [0U][vlSelf->reduce_i];
                vlSelf->reduce_b_old_valid = vlSelf->acc_bank_valid
                    [0U][(0xfU & ((IData)(vlSelf->reduce_i) 
                                  + (IData)(vlSelf->reduce_stride)))];
            }
        }
        if (vlSelf->acc_commit_q) {
            vlSelf->acc_out = vlSelf->final_reduce_result;
        }
        if (vlSelf->a_valid_in) {
            vlSelf->a_reg = vlSelf->a_in;
        }
        if (vlSelf->b_valid_in) {
            vlSelf->b_reg = vlSelf->b_in;
        }
        if (((1U == (IData)(vlSelf->red_state)) & (IData)(vlSelf->reduce_add_valid))) {
            __Vdly__reduce_wb_i = (0xfU & ((IData)(1U) 
                                           + (IData)(vlSelf->reduce_wb_i)));
        }
        if ((0U == (IData)(vlSelf->red_state))) {
            if (vlSelf->acc_handoff) {
                vlSelf->red_set = vlSelf->acc_set;
                __Vdly__reduce_stride = 8U;
                __Vdly__reduce_todo = 8U;
                __Vdly__reduce_i = 0U;
                __Vdly__reduce_wb_i = 0U;
                __Vdly__red_state = 1U;
            }
        } else if ((1U == (IData)(vlSelf->red_state))) {
            if ((0U != (IData)(vlSelf->reduce_todo))) {
                __Vdly__reduce_todo = (0xfU & ((IData)(vlSelf->reduce_todo) 
                                               - (IData)(1U)));
                __Vdly__reduce_i = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->reduce_i)));
            } else if (((0U == (IData)(vlSelf->reduce_add_busy)) 
                        & (~ (IData)(vlSelf->reduce_read_valid)))) {
                if ((1U == (IData)(vlSelf->reduce_stride))) {
                    __Vdly__red_state = 2U;
                } else {
                    __Vdly__reduce_stride = (0xfU & 
                                             VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_todo = (0xfU & VL_SHIFTR_III(4,4,32, (IData)(vlSelf->reduce_stride), 1U));
                    __Vdly__reduce_i = 0U;
                    __Vdly__reduce_wb_i = 0U;
                }
            }
        } else {
            __Vdly__red_state = 0U;
        }
        if (((IData)(vlSelf->reduce_add_valid) & (1U 
                                                  == (IData)(vlSelf->reduce_stride)))) {
            vlSelf->final_reduce_result = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    if ((1U & (~ (IData)(vlSelf->rst)))) {
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0xcU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 2U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 3U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 4U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 5U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 6U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 7U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 8U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 9U;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xaU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xbU;
        vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0xcU;
        if ((2U == (IData)(vlSelf->red_state))) {
            vlSelf->unnamedblk3__DOT__i = 1U;
            vlSelf->unnamedblk3__DOT__i = 2U;
            vlSelf->unnamedblk3__DOT__i = 3U;
            vlSelf->unnamedblk3__DOT__i = 4U;
            vlSelf->unnamedblk3__DOT__i = 5U;
            vlSelf->unnamedblk3__DOT__i = 6U;
            vlSelf->unnamedblk3__DOT__i = 7U;
            vlSelf->unnamedblk3__DOT__i = 8U;
            vlSelf->unnamedblk3__DOT__i = 9U;
            vlSelf->unnamedblk3__DOT__i = 0xaU;
            vlSelf->unnamedblk3__DOT__i = 0xbU;
            vlSelf->unnamedblk3__DOT__i = 0xcU;
            vlSelf->unnamedblk3__DOT__i = 0xdU;
            vlSelf->unnamedblk3__DOT__i = 0xeU;
            vlSelf->unnamedblk3__DOT__i = 0xfU;
            vlSelf->unnamedblk3__DOT__i = 0x10U;
        }
    }
    if (vlSelf->set1_we) {
        __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0 
            = vlSelf->set1_waddr;
        __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_wdata;
        __Vdlyvset__u_acc_set1_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0 
            = vlSelf->set1_waddr;
    }
    if (vlSelf->set0_we) {
        __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy1__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0 
            = vlSelf->set0_waddr;
        __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_wdata;
        __Vdlyvset__u_acc_set0_copy0__DOT__mem__v0 = 1U;
        __Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0 
            = vlSelf->set0_waddr;
    }
    if (((IData)(vlSelf->rst) | (IData)(vlSelf->acc_handoff))) {
        __Vdly__product_bank = 0U;
        __Vdly__accum_wb_bank = 0U;
    } else {
        if (vlSelf->u_fp_mul__DOT__valid_out) {
            __Vdly__product_bank = (0xfU & ((IData)(1U) 
                                            + (IData)(vlSelf->product_bank)));
        }
        if (vlSelf->u_fp_add_accum__DOT__valid_out) {
            __Vdly__accum_wb_bank = (0xfU & ((IData)(1U) 
                                             + (IData)(vlSelf->accum_wb_bank)));
        }
    }
    __Vtableidx5 = (((IData)(vlSelf->pipe_pair_valid) 
                     << 4U) | (((IData)(vlSelf->acc_set) 
                                << 3U) | (((IData)(vlSelf->acc_handoff) 
                                           << 2U) | 
                                          (((IData)(vlSelf->acc_state) 
                                            << 1U) 
                                           | (IData)(vlSelf->rst)))));
    if ((1U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx5])) {
        vlSelf->acc_state = Vtb_systolic_dma_top__ConstPool__TABLE_h7c4b191f_0
            [__Vtableidx5];
    }
    if ((2U & Vtb_systolic_dma_top__ConstPool__TABLE_hbdfccf66_0
         [__Vtableidx5])) {
        __Vdly__acc_set = Vtb_systolic_dma_top__ConstPool__TABLE_h254f92e7_0
            [__Vtableidx5];
    }
    vlSelf->u_acc_set1_copy1__DOT__rdata = vlSelf->u_acc_set1_copy1__DOT__mem
        [vlSelf->set1_c1_raddr];
    vlSelf->u_acc_set0_copy1__DOT__rdata = vlSelf->u_acc_set0_copy1__DOT__mem
        [vlSelf->set0_c1_raddr];
    vlSelf->u_acc_set1_copy0__DOT__rdata = vlSelf->u_acc_set1_copy0__DOT__mem
        [vlSelf->set1_c0_raddr];
    vlSelf->u_acc_set0_copy0__DOT__rdata = vlSelf->u_acc_set0_copy0__DOT__mem
        [vlSelf->set0_c0_raddr];
    vlSelf->acc_valid_out = ((1U & (~ (IData)(vlSelf->rst))) 
                             && (IData)(vlSelf->acc_commit_q));
    vlSelf->b_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->b_valid_in));
    vlSelf->a_valid_reg = ((1U & (~ (IData)(vlSelf->rst))) 
                           && (IData)(vlSelf->a_valid_in));
    vlSelf->mul_busy = __Vdly__mul_busy;
    vlSelf->accum_add_busy = __Vdly__accum_add_busy;
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v0) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v9) {
        vlSelf->u_fp_mul__DOT__v_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v10) {
        vlSelf->u_fp_mul__DOT__v_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v11) {
        vlSelf->u_fp_mul__DOT__v_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v12) {
        vlSelf->u_fp_mul__DOT__v_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v13) {
        vlSelf->u_fp_mul__DOT__v_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v14) {
        vlSelf->u_fp_mul__DOT__v_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v15) {
        vlSelf->u_fp_mul__DOT__v_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v16) {
        vlSelf->u_fp_mul__DOT__v_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__v_pipe__v17) {
        vlSelf->u_fp_mul__DOT__v_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v0) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = 0U;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v9) {
        vlSelf->u_fp_mul__DOT__d_pipe[0U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v9;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v10) {
        vlSelf->u_fp_mul__DOT__d_pipe[1U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v10;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v11) {
        vlSelf->u_fp_mul__DOT__d_pipe[2U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v11;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v12) {
        vlSelf->u_fp_mul__DOT__d_pipe[3U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v13) {
        vlSelf->u_fp_mul__DOT__d_pipe[4U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v14) {
        vlSelf->u_fp_mul__DOT__d_pipe[5U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v15) {
        vlSelf->u_fp_mul__DOT__d_pipe[6U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v16) {
        vlSelf->u_fp_mul__DOT__d_pipe[7U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_mul__DOT__d_pipe__v17) {
        vlSelf->u_fp_mul__DOT__d_pipe[8U] = __Vdlyvval__u_fp_mul__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__d_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__d_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__d_pipe__v23;
    }
    vlSelf->accum_wb_bank = __Vdly__accum_wb_bank;
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xaU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_accum__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[0xbU] = __Vdlyvval__u_fp_add_accum__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] = 0U;
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] = 0U;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v12) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v12;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v13) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[1U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v13;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v14) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[2U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v14;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v15) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[3U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v15;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v16) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[4U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v16;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v17) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[5U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v17;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v18) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[6U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v18;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v19) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[7U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v19;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v20) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[8U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v20;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v21) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[9U] = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v21;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v22) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xaU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v22;
    }
    if (__Vdlyvset__u_fp_add_reduce__DOT__v_pipe__v23) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[0xbU] 
            = __Vdlyvval__u_fp_add_reduce__DOT__v_pipe__v23;
    }
    if (__Vdlyvset__u_acc_set1_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vdlyvdim0__u_acc_set1_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy1__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vdlyvdim0__u_acc_set0_copy1__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy1__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set1_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vdlyvdim0__u_acc_set1_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set1_copy0__DOT__mem__v0;
    }
    if (__Vdlyvset__u_acc_set0_copy0__DOT__mem__v0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vdlyvdim0__u_acc_set0_copy0__DOT__mem__v0] 
            = __Vdlyvval__u_acc_set0_copy0__DOT__mem__v0;
    }
    vlSelf->product_bank = __Vdly__product_bank;
    if (__Vdlyvset__acc_bank_valid__v0) {
        vlSelf->acc_bank_valid[0U][0U] = 0U;
        vlSelf->acc_bank_valid[0U][1U] = 0U;
        vlSelf->acc_bank_valid[0U][2U] = 0U;
        vlSelf->acc_bank_valid[0U][3U] = 0U;
        vlSelf->acc_bank_valid[0U][4U] = 0U;
        vlSelf->acc_bank_valid[0U][5U] = 0U;
        vlSelf->acc_bank_valid[0U][6U] = 0U;
        vlSelf->acc_bank_valid[0U][7U] = 0U;
        vlSelf->acc_bank_valid[0U][8U] = 0U;
        vlSelf->acc_bank_valid[0U][9U] = 0U;
        vlSelf->acc_bank_valid[0U][0xaU] = 0U;
        vlSelf->acc_bank_valid[0U][0xbU] = 0U;
        vlSelf->acc_bank_valid[0U][0xcU] = 0U;
        vlSelf->acc_bank_valid[0U][0xdU] = 0U;
        vlSelf->acc_bank_valid[0U][0xeU] = 0U;
        vlSelf->acc_bank_valid[0U][0xfU] = 0U;
        vlSelf->acc_bank_valid[1U][0U] = 0U;
        vlSelf->acc_bank_valid[1U][1U] = 0U;
        vlSelf->acc_bank_valid[1U][2U] = 0U;
        vlSelf->acc_bank_valid[1U][3U] = 0U;
        vlSelf->acc_bank_valid[1U][4U] = 0U;
        vlSelf->acc_bank_valid[1U][5U] = 0U;
        vlSelf->acc_bank_valid[1U][6U] = 0U;
        vlSelf->acc_bank_valid[1U][7U] = 0U;
        vlSelf->acc_bank_valid[1U][8U] = 0U;
        vlSelf->acc_bank_valid[1U][9U] = 0U;
        vlSelf->acc_bank_valid[1U][0xaU] = 0U;
        vlSelf->acc_bank_valid[1U][0xbU] = 0U;
        vlSelf->acc_bank_valid[1U][0xcU] = 0U;
        vlSelf->acc_bank_valid[1U][0xdU] = 0U;
        vlSelf->acc_bank_valid[1U][0xeU] = 0U;
        vlSelf->acc_bank_valid[1U][0xfU] = 0U;
    }
    if (__Vdlyvset__acc_bank_valid__v32) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v32][__Vdlyvdim1__acc_bank_valid__v32] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v33) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v33][__Vdlyvdim1__acc_bank_valid__v33] = 1U;
    }
    if (__Vdlyvset__acc_bank_valid__v34) {
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v34][0U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v35][1U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v36][2U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v37][3U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v38][4U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v39][5U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v40][6U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v41][7U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v42][8U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v43][9U] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v44][0xaU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v45][0xbU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v46][0xcU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v47][0xdU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v48][0xeU] = 0U;
        vlSelf->acc_bank_valid[__Vdlyvdim0__acc_bank_valid__v49][0xfU] = 0U;
    }
    vlSelf->u_fp_add_accum__DOT__result = vlSelf->u_fp_add_accum__DOT__d_pipe
        [0xbU];
    vlSelf->u_fp_add_accum__DOT__valid_out = vlSelf->u_fp_add_accum__DOT__v_pipe
        [0xbU];
    vlSelf->accum_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                && (IData)(vlSelf->u_fp_mul__DOT__valid_out));
    vlSelf->set1_c1_rdata = vlSelf->u_acc_set1_copy1__DOT__rdata;
    vlSelf->set0_c1_rdata = vlSelf->u_acc_set0_copy1__DOT__rdata;
    vlSelf->set1_c0_rdata = vlSelf->u_acc_set1_copy0__DOT__rdata;
    vlSelf->set0_c0_rdata = vlSelf->u_acc_set0_copy0__DOT__rdata;
    vlSelf->b_valid_out = vlSelf->b_valid_reg;
    vlSelf->pipe_pair_valid = ((IData)(vlSelf->a_valid_reg) 
                               & (IData)(vlSelf->b_valid_reg));
    vlSelf->a_valid_out = vlSelf->a_valid_reg;
    vlSelf->acc_commit_q = ((~ (IData)(vlSelf->rst)) 
                            & (IData)(vlSelf->final_reduction_complete));
    vlSelf->accum_add_result = vlSelf->u_fp_add_accum__DOT__result;
    vlSelf->u_fp_mul__DOT__result = vlSelf->u_fp_mul__DOT__d_pipe
        [8U];
    vlSelf->accum_add_valid = vlSelf->u_fp_add_accum__DOT__valid_out;
    vlSelf->u_fp_mul__DOT__valid_out = vlSelf->u_fp_mul__DOT__v_pipe
        [8U];
    vlSelf->u_fp_mul__DOT__valid_in = vlSelf->pipe_pair_valid;
    vlSelf->reduce_add_busy = __Vdly__reduce_add_busy;
    vlSelf->reduce_wb_i = __Vdly__reduce_wb_i;
    vlSelf->reduce_i = __Vdly__reduce_i;
    vlSelf->reduce_todo = __Vdly__reduce_todo;
    vlSelf->red_state = __Vdly__red_state;
    vlSelf->acc_set = __Vdly__acc_set;
    vlSelf->reduce_stride = __Vdly__reduce_stride;
    vlSelf->u_fp_add_reduce__DOT__result = vlSelf->u_fp_add_reduce__DOT__d_pipe
        [0xbU];
    vlSelf->reduce_add_valid = vlSelf->u_fp_add_reduce__DOT__v_pipe
        [0xbU];
    vlSelf->u_fp_mul__DOT__a = vlSelf->a_reg;
    vlSelf->a_out = vlSelf->a_reg;
    vlSelf->u_fp_mul__DOT__b = vlSelf->b_reg;
    vlSelf->b_out = vlSelf->b_reg;
    vlSelf->product = vlSelf->u_fp_mul__DOT__result;
    vlSelf->u_fp_add_accum__DOT__b = vlSelf->accum_product_d;
    vlSelf->u_fp_add_accum__DOT__valid_in = vlSelf->accum_read_valid;
    vlSelf->product_valid = vlSelf->u_fp_mul__DOT__valid_out;
    vlSelf->acc_handoff = ((IData)(vlSelf->acc_state) 
                           & ((~ (IData)(vlSelf->pipe_pair_valid)) 
                              & ((~ vlSelf->u_fp_mul__DOT__v_pipe
                                  [8U]) & ((~ (IData)(vlSelf->accum_read_valid)) 
                                           & ((0U == (IData)(vlSelf->mul_busy)) 
                                              & ((0U 
                                                  == (IData)(vlSelf->accum_add_busy)) 
                                                 & (0U 
                                                    == (IData)(vlSelf->red_state))))))));
    vlSelf->accum_ram_data = ((IData)(vlSelf->acc_set)
                               ? vlSelf->u_acc_set1_copy0__DOT__rdata
                               : vlSelf->u_acc_set0_copy0__DOT__rdata);
    if (vlSelf->red_set) {
        vlSelf->reduce_a_data = vlSelf->set1_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set1_c1_rdata;
    } else {
        vlSelf->reduce_a_data = vlSelf->set0_c0_rdata;
        vlSelf->reduce_b_data = vlSelf->set0_c1_rdata;
    }
    vlSelf->reduce_read_valid = ((1U & (~ (IData)(vlSelf->rst))) 
                                 && (IData)(vlSelf->reduce_issue));
    vlSelf->reduce_add_result = vlSelf->u_fp_add_reduce__DOT__result;
    vlSelf->u_fp_add_reduce__DOT__valid_out = vlSelf->reduce_add_valid;
    vlSelf->set0_we = 0U;
    vlSelf->set1_we = 0U;
    vlSelf->set0_waddr = 0U;
    vlSelf->set1_waddr = 0U;
    vlSelf->set0_wdata = 0U;
    if (vlSelf->u_fp_add_accum__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->accum_wb_bank;
            vlSelf->set0_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
        vlSelf->set1_wdata = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->accum_wb_bank;
            vlSelf->set1_wdata = vlSelf->u_fp_add_accum__DOT__result;
        }
    } else {
        vlSelf->set1_wdata = 0U;
    }
    if (vlSelf->reduce_add_valid) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_we = 1U;
            vlSelf->set0_waddr = vlSelf->reduce_wb_i;
            vlSelf->set0_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_we = 1U;
            vlSelf->set1_waddr = vlSelf->reduce_wb_i;
            vlSelf->set1_wdata = vlSelf->u_fp_add_reduce__DOT__result;
        }
    }
    vlSelf->accum_old_value = ((IData)(vlSelf->accum_old_valid)
                                ? vlSelf->accum_ram_data
                                : 0U);
    vlSelf->reduce_a_value = ((IData)(vlSelf->reduce_a_old_valid)
                               ? vlSelf->reduce_a_data
                               : 0U);
    vlSelf->reduce_b_value = ((IData)(vlSelf->reduce_b_old_valid)
                               ? vlSelf->reduce_b_data
                               : 0U);
    vlSelf->reduce_issue = ((1U == (IData)(vlSelf->red_state)) 
                            & (0U != (IData)(vlSelf->reduce_todo)));
    vlSelf->u_acc_set0_copy0__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set0_copy1__DOT__we = vlSelf->set0_we;
    vlSelf->u_acc_set1_copy0__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set1_copy1__DOT__we = vlSelf->set1_we;
    vlSelf->u_acc_set0_copy0__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set0_copy1__DOT__waddr = vlSelf->set0_waddr;
    vlSelf->u_acc_set1_copy0__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set1_copy1__DOT__waddr = vlSelf->set1_waddr;
    vlSelf->u_acc_set0_copy0__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set0_copy1__DOT__wdata = vlSelf->set0_wdata;
    vlSelf->u_acc_set1_copy0__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_acc_set1_copy1__DOT__wdata = vlSelf->set1_wdata;
    vlSelf->u_fp_add_accum__DOT__a = vlSelf->accum_old_value;
    vlSelf->u_fp_add_reduce__DOT__a = vlSelf->reduce_a_value;
    vlSelf->u_fp_add_reduce__DOT__b = vlSelf->reduce_b_value;
    vlSelf->u_fp_add_reduce__DOT__valid_in = vlSelf->reduce_read_valid;
    vlSelf->final_reduction_complete = ((1U == (IData)(vlSelf->red_state)) 
                                        & ((0U == (IData)(vlSelf->reduce_todo)) 
                                           & ((0U == (IData)(vlSelf->reduce_add_busy)) 
                                              & ((~ (IData)(vlSelf->reduce_read_valid)) 
                                                 & (1U 
                                                    == (IData)(vlSelf->reduce_stride))))));
    vlSelf->set0_c1_raddr = 0U;
    vlSelf->set1_c1_raddr = 0U;
    vlSelf->set0_c0_raddr = 0U;
    if (vlSelf->u_fp_mul__DOT__valid_out) {
        if ((1U & (~ (IData)(vlSelf->acc_set)))) {
            vlSelf->set0_c0_raddr = vlSelf->product_bank;
        }
        vlSelf->set1_c0_raddr = 0U;
        if (vlSelf->acc_set) {
            vlSelf->set1_c0_raddr = vlSelf->product_bank;
        }
    } else {
        vlSelf->set1_c0_raddr = 0U;
    }
    if (vlSelf->reduce_issue) {
        if ((1U & (~ (IData)(vlSelf->red_set)))) {
            vlSelf->set0_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set0_c0_raddr = vlSelf->reduce_i;
        }
        if (vlSelf->red_set) {
            vlSelf->set1_c1_raddr = (0xfU & ((IData)(vlSelf->reduce_i) 
                                             + (IData)(vlSelf->reduce_stride)));
            vlSelf->set1_c0_raddr = vlSelf->reduce_i;
        }
    }
    vlSelf->u_acc_set0_copy1__DOT__raddr = vlSelf->set0_c1_raddr;
    vlSelf->u_acc_set1_copy1__DOT__raddr = vlSelf->set1_c1_raddr;
    vlSelf->u_acc_set0_copy0__DOT__raddr = vlSelf->set0_c0_raddr;
    vlSelf->u_acc_set1_copy0__DOT__raddr = vlSelf->set1_c0_raddr;
}
