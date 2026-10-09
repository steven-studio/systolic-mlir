// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtb_systolic_dma_top.h for the primary calling header

#include "Vtb_systolic_dma_top__pch.h"
#include "Vtb_systolic_dma_top__Syms.h"
#include "Vtb_systolic_dma_top_systolic_array__N4.h"

// Parameter definitions for Vtb_systolic_dma_top_systolic_array__N4
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_array__N4::N;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_array__N4::DATA_W;
constexpr IData/*31:0*/ Vtb_systolic_dma_top_systolic_array__N4::ACC_BANKS;


void Vtb_systolic_dma_top_systolic_array__N4___ctor_var_reset(Vtb_systolic_dma_top_systolic_array__N4* vlSelf);

Vtb_systolic_dma_top_systolic_array__N4::Vtb_systolic_dma_top_systolic_array__N4(Vtb_systolic_dma_top__Syms* symsp, const char* v__name)
    : VerilatedModule{v__name}
    , vlSymsp{symsp}
 {
    // Reset structure values
    Vtb_systolic_dma_top_systolic_array__N4___ctor_var_reset(this);
}

void Vtb_systolic_dma_top_systolic_array__N4::__Vconfigure(bool first) {
    if (false && first) {}  // Prevent unused
}

Vtb_systolic_dma_top_systolic_array__N4::~Vtb_systolic_dma_top_systolic_array__N4() {
}
