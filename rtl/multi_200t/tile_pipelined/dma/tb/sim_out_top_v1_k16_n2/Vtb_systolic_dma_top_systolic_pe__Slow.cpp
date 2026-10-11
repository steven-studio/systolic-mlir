// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top__Syms.h"
#include "Vtb_systolic_dma_top_systolic_pe.h"

// Parameter definitions for Vtb_systolic_dma_top_systolic_pe
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::ACC_BANKS;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::MUL_LATENCY;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::ADD_LATENCY;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::ACC_SEL_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_fp_mul__DOT__LAT;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy0__DOT__DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy0__DOT__DEPTH;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy0__DOT__ADDR_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy1__DOT__DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy1__DOT__DEPTH;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set0_copy1__DOT__ADDR_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy0__DOT__DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy0__DOT__DEPTH;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy0__DOT__ADDR_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy1__DOT__DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy1__DOT__DEPTH;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_acc_set1_copy1__DOT__ADDR_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_fp_add_accum__DOT__LAT;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_pe::u_fp_add_reduce__DOT__LAT;


void Vtb_systolic_dma_top_systolic_pe___ctor_var_reset(Vtb_systolic_dma_top_systolic_pe* vlSelf);

Vtb_systolic_dma_top_systolic_pe::Vtb_systolic_dma_top_systolic_pe(Vtb_systolic_dma_top__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vtb_systolic_dma_top_systolic_pe___ctor_var_reset(this);
}

void Vtb_systolic_dma_top_systolic_pe::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vtb_systolic_dma_top_systolic_pe::~Vtb_systolic_dma_top_systolic_pe() {
}
