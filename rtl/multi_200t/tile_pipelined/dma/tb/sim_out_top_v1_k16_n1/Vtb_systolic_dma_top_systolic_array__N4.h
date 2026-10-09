// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vtb_systolic_dma_top.h for the primary calling header

#ifndef VERILATED_VTB_SYSTOLIC_DMA_TOP_SYSTOLIC_ARRAY__N4_H_
#define VERILATED_VTB_SYSTOLIC_DMA_TOP_SYSTOLIC_ARRAY__N4_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"
class Vtb_systolic_dma_top_systolic_pe;


class Vtb_systolic_dma_top__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vtb_systolic_dma_top_systolic_array__N4 final : public VerilatedModule {
  public:
    // CELLS
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__0__KET____DOT__COL__BRA__0__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__0__KET____DOT__COL__BRA__1__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__0__KET____DOT__COL__BRA__2__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__0__KET____DOT__COL__BRA__3__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__1__KET____DOT__COL__BRA__0__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__1__KET____DOT__COL__BRA__1__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__1__KET____DOT__COL__BRA__2__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__1__KET____DOT__COL__BRA__3__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__2__KET____DOT__COL__BRA__0__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__2__KET____DOT__COL__BRA__1__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__2__KET____DOT__COL__BRA__2__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__2__KET____DOT__COL__BRA__3__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__3__KET____DOT__COL__BRA__0__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__3__KET____DOT__COL__BRA__1__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__3__KET____DOT__COL__BRA__2__KET____DOT__u_pe;
    Vtb_systolic_dma_top_systolic_pe* __PVT__ROW__BRA__3__KET____DOT__COL__BRA__3__KET____DOT__u_pe;

    // DESIGN SPECIFIC STATE
    VL_IN8(clk,0,0);
    VL_IN8(rst,0,0);
    VL_OUT8(c_valid_out,0,0);
    CData/*0:0*/ clear_arrived;
    CData/*0:0*/ all_arrived;
    CData/*0:0*/ out_state;
    IData/*31:0*/ unnamedblk1__DOT__rr;
    IData/*31:0*/ unnamedblk1__DOT__unnamedblk2__DOT__cc;
    IData/*31:0*/ unnamedblk3__DOT__rr;
    IData/*31:0*/ unnamedblk3__DOT__unnamedblk4__DOT__cc;
    VL_IN(a_in[4],31,0);
    VL_IN(b_in[4],31,0);
    VL_IN8(a_valid_in[4],0,0);
    VL_IN8(b_valid_in[4],0,0);
    VL_OUT(c_out[4][4],31,0);
    VlUnpacked<VlUnpacked<IData/*31:0*/, 5>, 4> a_bus;
    VlUnpacked<VlUnpacked<IData/*31:0*/, 4>, 5> b_bus;
    VlUnpacked<VlUnpacked<CData/*0:0*/, 5>, 4> a_valid_bus;
    VlUnpacked<VlUnpacked<CData/*0:0*/, 4>, 5> b_valid_bus;
    VlUnpacked<VlUnpacked<IData/*31:0*/, 4>, 4> pe_acc;
    VlUnpacked<VlUnpacked<CData/*0:0*/, 4>, 4> pe_acc_valid;
    VlUnpacked<VlUnpacked<CData/*0:0*/, 4>, 4> acc_arrived;

    // INTERNAL VARIABLES
    Vtb_systolic_dma_top__Syms* const vlSymsp;

    // PARAMETERS
    static constexpr IData/*31:0*/ N = 4U;
    static constexpr IData/*31:0*/ DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ ACC_BANKS = 0x00000010U;

    // CONSTRUCTORS
    Vtb_systolic_dma_top_systolic_array__N4(Vtb_systolic_dma_top__Syms* symsp, const char* v__name);
    ~Vtb_systolic_dma_top_systolic_array__N4();
    VL_UNCOPYABLE(Vtb_systolic_dma_top_systolic_array__N4);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
