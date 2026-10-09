// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top_systolic_pe.h"

VL_ATTR_COLD void Vtb_systolic_dma_top_systolic_pe___ctor_var_reset(Vtb_systolic_dma_top_systolic_pe* vlSelf) {
    if (false && vlSelf) {}  // Prevent unused
    Vtb_systolic_dma_top__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vtb_systolic_dma_top_systolic_pe___ctor_var_reset\n"); );
    // Body
    vlSelf->clk = VL_RAND_RESET_I(1);
    vlSelf->rst = VL_RAND_RESET_I(1);
    vlSelf->a_valid_in = VL_RAND_RESET_I(1);
    vlSelf->b_valid_in = VL_RAND_RESET_I(1);
    vlSelf->a_in = VL_RAND_RESET_I(32);
    vlSelf->b_in = VL_RAND_RESET_I(32);
    vlSelf->a_valid_out = VL_RAND_RESET_I(1);
    vlSelf->b_valid_out = VL_RAND_RESET_I(1);
    vlSelf->a_out = VL_RAND_RESET_I(32);
    vlSelf->b_out = VL_RAND_RESET_I(32);
    vlSelf->acc_valid_out = VL_RAND_RESET_I(1);
    vlSelf->acc_out = VL_RAND_RESET_I(32);
    vlSelf->a_reg = VL_RAND_RESET_I(32);
    vlSelf->b_reg = VL_RAND_RESET_I(32);
    vlSelf->a_valid_reg = VL_RAND_RESET_I(1);
    vlSelf->b_valid_reg = VL_RAND_RESET_I(1);
    vlSelf->pipe_pair_valid = VL_RAND_RESET_I(1);
    vlSelf->product = VL_RAND_RESET_I(32);
    vlSelf->product_valid = VL_RAND_RESET_I(1);
    vlSelf->set0_c0_raddr = VL_RAND_RESET_I(4);
    vlSelf->set0_c1_raddr = VL_RAND_RESET_I(4);
    vlSelf->set1_c0_raddr = VL_RAND_RESET_I(4);
    vlSelf->set1_c1_raddr = VL_RAND_RESET_I(4);
    vlSelf->set0_c0_rdata = VL_RAND_RESET_I(32);
    vlSelf->set0_c1_rdata = VL_RAND_RESET_I(32);
    vlSelf->set1_c0_rdata = VL_RAND_RESET_I(32);
    vlSelf->set1_c1_rdata = VL_RAND_RESET_I(32);
    vlSelf->set0_we = VL_RAND_RESET_I(1);
    vlSelf->set1_we = VL_RAND_RESET_I(1);
    vlSelf->set0_waddr = VL_RAND_RESET_I(4);
    vlSelf->set1_waddr = VL_RAND_RESET_I(4);
    vlSelf->set0_wdata = VL_RAND_RESET_I(32);
    vlSelf->set1_wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        for (int __Vi1 = 0; __Vi1 < 16; ++__Vi1) {
            vlSelf->acc_bank_valid[__Vi0][__Vi1] = VL_RAND_RESET_I(1);
        }
    }
    vlSelf->acc_set = VL_RAND_RESET_I(1);
    vlSelf->red_set = VL_RAND_RESET_I(1);
    vlSelf->product_bank = VL_RAND_RESET_I(4);
    vlSelf->accum_wb_bank = VL_RAND_RESET_I(4);
    vlSelf->mul_busy = VL_RAND_RESET_I(8);
    vlSelf->accum_add_busy = VL_RAND_RESET_I(8);
    vlSelf->reduce_add_busy = VL_RAND_RESET_I(8);
    vlSelf->accum_add_result = VL_RAND_RESET_I(32);
    vlSelf->accum_add_valid = VL_RAND_RESET_I(1);
    vlSelf->accum_ram_data = VL_RAND_RESET_I(32);
    vlSelf->accum_product_d = VL_RAND_RESET_I(32);
    vlSelf->accum_read_valid = VL_RAND_RESET_I(1);
    vlSelf->accum_old_valid = VL_RAND_RESET_I(1);
    vlSelf->accum_old_value = VL_RAND_RESET_I(32);
    vlSelf->reduce_stride = VL_RAND_RESET_I(4);
    vlSelf->reduce_i = VL_RAND_RESET_I(4);
    vlSelf->reduce_wb_i = VL_RAND_RESET_I(4);
    vlSelf->reduce_todo = VL_RAND_RESET_I(4);
    vlSelf->red_state = VL_RAND_RESET_I(2);
    vlSelf->reduce_issue = VL_RAND_RESET_I(1);
    vlSelf->reduce_add_result = VL_RAND_RESET_I(32);
    vlSelf->reduce_add_valid = VL_RAND_RESET_I(1);
    vlSelf->final_reduce_result = VL_RAND_RESET_I(32);
    vlSelf->reduce_a_data = VL_RAND_RESET_I(32);
    vlSelf->reduce_b_data = VL_RAND_RESET_I(32);
    vlSelf->reduce_read_valid = VL_RAND_RESET_I(1);
    vlSelf->reduce_a_old_valid = VL_RAND_RESET_I(1);
    vlSelf->reduce_b_old_valid = VL_RAND_RESET_I(1);
    vlSelf->reduce_a_value = VL_RAND_RESET_I(32);
    vlSelf->reduce_b_value = VL_RAND_RESET_I(32);
    vlSelf->acc_state = VL_RAND_RESET_I(1);
    vlSelf->acc_handoff = VL_RAND_RESET_I(1);
    vlSelf->acc_commit_q = VL_RAND_RESET_I(1);
    vlSelf->final_reduction_complete = VL_RAND_RESET_I(1);
    vlSelf->unnamedblk1__DOT__s = 0;
    vlSelf->unnamedblk1__DOT__unnamedblk2__DOT__i = 0;
    vlSelf->unnamedblk3__DOT__i = 0;
    vlSelf->u_fp_mul__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_fp_mul__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->u_fp_mul__DOT__valid_in = VL_RAND_RESET_I(1);
    vlSelf->u_fp_mul__DOT__a = VL_RAND_RESET_I(32);
    vlSelf->u_fp_mul__DOT__b = VL_RAND_RESET_I(32);
    vlSelf->u_fp_mul__DOT__valid_out = VL_RAND_RESET_I(1);
    vlSelf->u_fp_mul__DOT__result = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->u_fp_mul__DOT__v_pipe[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 9; ++__Vi0) {
        vlSelf->u_fp_mul__DOT__d_pipe[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_fp_mul__DOT__unnamedblk1__DOT__i = 0;
    vlSelf->u_fp_mul__DOT__unnamedblk2__DOT__i = 0;
    vlSelf->u_fp_mul__DOT____Vlvbound_hdd5b0385__0 = VL_RAND_RESET_I(1);
    vlSelf->u_fp_mul__DOT____Vlvbound_h03793961__0 = VL_RAND_RESET_I(32);
    vlSelf->u_acc_set0_copy0__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set0_copy0__DOT__raddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set0_copy0__DOT__rdata = VL_RAND_RESET_I(32);
    vlSelf->u_acc_set0_copy0__DOT__we = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set0_copy0__DOT__waddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set0_copy0__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->u_acc_set0_copy0__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_acc_set0_copy1__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set0_copy1__DOT__raddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set0_copy1__DOT__rdata = VL_RAND_RESET_I(32);
    vlSelf->u_acc_set0_copy1__DOT__we = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set0_copy1__DOT__waddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set0_copy1__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->u_acc_set0_copy1__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_acc_set1_copy0__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set1_copy0__DOT__raddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set1_copy0__DOT__rdata = VL_RAND_RESET_I(32);
    vlSelf->u_acc_set1_copy0__DOT__we = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set1_copy0__DOT__waddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set1_copy0__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->u_acc_set1_copy0__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_acc_set1_copy1__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set1_copy1__DOT__raddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set1_copy1__DOT__rdata = VL_RAND_RESET_I(32);
    vlSelf->u_acc_set1_copy1__DOT__we = VL_RAND_RESET_I(1);
    vlSelf->u_acc_set1_copy1__DOT__waddr = VL_RAND_RESET_I(4);
    vlSelf->u_acc_set1_copy1__DOT__wdata = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 16; ++__Vi0) {
        vlSelf->u_acc_set1_copy1__DOT__mem[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_fp_add_accum__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_accum__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_accum__DOT__valid_in = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_accum__DOT__a = VL_RAND_RESET_I(32);
    vlSelf->u_fp_add_accum__DOT__b = VL_RAND_RESET_I(32);
    vlSelf->u_fp_add_accum__DOT__valid_out = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_accum__DOT__result = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 12; ++__Vi0) {
        vlSelf->u_fp_add_accum__DOT__v_pipe[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 12; ++__Vi0) {
        vlSelf->u_fp_add_accum__DOT__d_pipe[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_fp_add_accum__DOT__unnamedblk1__DOT__i = 0;
    vlSelf->u_fp_add_accum__DOT__unnamedblk2__DOT__i = 0;
    vlSelf->u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0 = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_accum__DOT____Vlvbound_h2d273a63__0 = VL_RAND_RESET_I(32);
    vlSelf->u_fp_add_reduce__DOT__clk = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_reduce__DOT__rst = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_reduce__DOT__valid_in = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_reduce__DOT__a = VL_RAND_RESET_I(32);
    vlSelf->u_fp_add_reduce__DOT__b = VL_RAND_RESET_I(32);
    vlSelf->u_fp_add_reduce__DOT__valid_out = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_reduce__DOT__result = VL_RAND_RESET_I(32);
    for (int __Vi0 = 0; __Vi0 < 12; ++__Vi0) {
        vlSelf->u_fp_add_reduce__DOT__v_pipe[__Vi0] = VL_RAND_RESET_I(1);
    }
    for (int __Vi0 = 0; __Vi0 < 12; ++__Vi0) {
        vlSelf->u_fp_add_reduce__DOT__d_pipe[__Vi0] = VL_RAND_RESET_I(32);
    }
    vlSelf->u_fp_add_reduce__DOT__unnamedblk1__DOT__i = 0;
    vlSelf->u_fp_add_reduce__DOT__unnamedblk2__DOT__i = 0;
    vlSelf->u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0 = VL_RAND_RESET_I(1);
    vlSelf->u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0 = VL_RAND_RESET_I(32);
}
