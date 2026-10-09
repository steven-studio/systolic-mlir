// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vtb_systolic_dma_top.h for the primary calling header

#ifndef VERILATED_VTB_SYSTOLIC_DMA_TOP_SYSTOLIC_PE_H_
#define VERILATED_VTB_SYSTOLIC_DMA_TOP_SYSTOLIC_PE_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"


class Vtb_systolic_dma_top__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vtb_systolic_dma_top_systolic_pe final : public VerilatedModule {
  public:

    // DESIGN SPECIFIC STATE
    // Anonymous structures to workaround compiler member-count bugs
    struct {
        VL_IN8(clk,0,0);
        VL_IN8(rst,0,0);
        VL_IN8(a_valid_in,0,0);
        VL_IN8(b_valid_in,0,0);
        VL_OUT8(a_valid_out,0,0);
        VL_OUT8(b_valid_out,0,0);
        VL_OUT8(acc_valid_out,0,0);
        CData/*0:0*/ a_valid_reg;
        CData/*0:0*/ b_valid_reg;
        CData/*0:0*/ pipe_pair_valid;
        CData/*0:0*/ product_valid;
        CData/*3:0*/ set0_c0_raddr;
        CData/*3:0*/ set0_c1_raddr;
        CData/*3:0*/ set1_c0_raddr;
        CData/*3:0*/ set1_c1_raddr;
        CData/*0:0*/ set0_we;
        CData/*0:0*/ set1_we;
        CData/*3:0*/ set0_waddr;
        CData/*3:0*/ set1_waddr;
        CData/*0:0*/ acc_set;
        CData/*0:0*/ red_set;
        CData/*3:0*/ product_bank;
        CData/*3:0*/ accum_wb_bank;
        CData/*7:0*/ mul_busy;
        CData/*7:0*/ accum_add_busy;
        CData/*7:0*/ reduce_add_busy;
        CData/*0:0*/ accum_add_valid;
        CData/*0:0*/ accum_read_valid;
        CData/*0:0*/ accum_old_valid;
        CData/*3:0*/ reduce_stride;
        CData/*3:0*/ reduce_i;
        CData/*3:0*/ reduce_wb_i;
        CData/*3:0*/ reduce_todo;
        CData/*1:0*/ red_state;
        CData/*0:0*/ reduce_issue;
        CData/*0:0*/ reduce_add_valid;
        CData/*0:0*/ reduce_read_valid;
        CData/*0:0*/ reduce_a_old_valid;
        CData/*0:0*/ reduce_b_old_valid;
        CData/*0:0*/ acc_state;
        CData/*0:0*/ acc_handoff;
        CData/*0:0*/ acc_commit_q;
        CData/*0:0*/ final_reduction_complete;
        CData/*0:0*/ u_fp_mul__DOT__clk;
        CData/*0:0*/ u_fp_mul__DOT__rst;
        CData/*0:0*/ u_fp_mul__DOT__valid_in;
        CData/*0:0*/ u_fp_mul__DOT__valid_out;
        CData/*0:0*/ u_fp_mul__DOT____Vlvbound_hdd5b0385__0;
        CData/*0:0*/ u_acc_set0_copy0__DOT__clk;
        CData/*3:0*/ u_acc_set0_copy0__DOT__raddr;
        CData/*0:0*/ u_acc_set0_copy0__DOT__we;
        CData/*3:0*/ u_acc_set0_copy0__DOT__waddr;
        CData/*0:0*/ u_acc_set0_copy1__DOT__clk;
        CData/*3:0*/ u_acc_set0_copy1__DOT__raddr;
        CData/*0:0*/ u_acc_set0_copy1__DOT__we;
        CData/*3:0*/ u_acc_set0_copy1__DOT__waddr;
        CData/*0:0*/ u_acc_set1_copy0__DOT__clk;
        CData/*3:0*/ u_acc_set1_copy0__DOT__raddr;
        CData/*0:0*/ u_acc_set1_copy0__DOT__we;
        CData/*3:0*/ u_acc_set1_copy0__DOT__waddr;
        CData/*0:0*/ u_acc_set1_copy1__DOT__clk;
        CData/*3:0*/ u_acc_set1_copy1__DOT__raddr;
        CData/*0:0*/ u_acc_set1_copy1__DOT__we;
        CData/*3:0*/ u_acc_set1_copy1__DOT__waddr;
    };
    struct {
        CData/*0:0*/ u_fp_add_accum__DOT__clk;
        CData/*0:0*/ u_fp_add_accum__DOT__rst;
        CData/*0:0*/ u_fp_add_accum__DOT__valid_in;
        CData/*0:0*/ u_fp_add_accum__DOT__valid_out;
        CData/*0:0*/ u_fp_add_accum__DOT____Vlvbound_h4fe5a259__0;
        CData/*0:0*/ u_fp_add_reduce__DOT__clk;
        CData/*0:0*/ u_fp_add_reduce__DOT__rst;
        CData/*0:0*/ u_fp_add_reduce__DOT__valid_in;
        CData/*0:0*/ u_fp_add_reduce__DOT__valid_out;
        CData/*0:0*/ u_fp_add_reduce__DOT____Vlvbound_h4fe5a259__0;
        VL_IN(a_in,31,0);
        VL_IN(b_in,31,0);
        VL_OUT(a_out,31,0);
        VL_OUT(b_out,31,0);
        VL_OUT(acc_out,31,0);
        IData/*31:0*/ a_reg;
        IData/*31:0*/ b_reg;
        IData/*31:0*/ product;
        IData/*31:0*/ set0_c0_rdata;
        IData/*31:0*/ set0_c1_rdata;
        IData/*31:0*/ set1_c0_rdata;
        IData/*31:0*/ set1_c1_rdata;
        IData/*31:0*/ set0_wdata;
        IData/*31:0*/ set1_wdata;
        IData/*31:0*/ accum_add_result;
        IData/*31:0*/ accum_ram_data;
        IData/*31:0*/ accum_product_d;
        IData/*31:0*/ accum_old_value;
        IData/*31:0*/ reduce_add_result;
        IData/*31:0*/ final_reduce_result;
        IData/*31:0*/ reduce_a_data;
        IData/*31:0*/ reduce_b_data;
        IData/*31:0*/ reduce_a_value;
        IData/*31:0*/ reduce_b_value;
        IData/*31:0*/ unnamedblk1__DOT__s;
        IData/*31:0*/ unnamedblk1__DOT__unnamedblk2__DOT__i;
        IData/*31:0*/ unnamedblk3__DOT__i;
        IData/*31:0*/ u_fp_mul__DOT__a;
        IData/*31:0*/ u_fp_mul__DOT__b;
        IData/*31:0*/ u_fp_mul__DOT__result;
        IData/*31:0*/ u_fp_mul__DOT__unnamedblk1__DOT__i;
        IData/*31:0*/ u_fp_mul__DOT__unnamedblk2__DOT__i;
        IData/*31:0*/ u_fp_mul__DOT____Vlvbound_h03793961__0;
        IData/*31:0*/ u_acc_set0_copy0__DOT__rdata;
        IData/*31:0*/ u_acc_set0_copy0__DOT__wdata;
        IData/*31:0*/ u_acc_set0_copy1__DOT__rdata;
        IData/*31:0*/ u_acc_set0_copy1__DOT__wdata;
        IData/*31:0*/ u_acc_set1_copy0__DOT__rdata;
        IData/*31:0*/ u_acc_set1_copy0__DOT__wdata;
        IData/*31:0*/ u_acc_set1_copy1__DOT__rdata;
        IData/*31:0*/ u_acc_set1_copy1__DOT__wdata;
        IData/*31:0*/ u_fp_add_accum__DOT__a;
        IData/*31:0*/ u_fp_add_accum__DOT__b;
        IData/*31:0*/ u_fp_add_accum__DOT__result;
        IData/*31:0*/ u_fp_add_accum__DOT__unnamedblk1__DOT__i;
        IData/*31:0*/ u_fp_add_accum__DOT__unnamedblk2__DOT__i;
        IData/*31:0*/ u_fp_add_accum__DOT____Vlvbound_h2d273a63__0;
        IData/*31:0*/ u_fp_add_reduce__DOT__a;
        IData/*31:0*/ u_fp_add_reduce__DOT__b;
        IData/*31:0*/ u_fp_add_reduce__DOT__result;
        IData/*31:0*/ u_fp_add_reduce__DOT__unnamedblk1__DOT__i;
        IData/*31:0*/ u_fp_add_reduce__DOT__unnamedblk2__DOT__i;
        IData/*31:0*/ u_fp_add_reduce__DOT____Vlvbound_h2d273a63__0;
        VlUnpacked<VlUnpacked<CData/*0:0*/, 16>, 2> acc_bank_valid;
    };
    struct {
        VlUnpacked<CData/*0:0*/, 9> u_fp_mul__DOT__v_pipe;
        VlUnpacked<IData/*31:0*/, 9> u_fp_mul__DOT__d_pipe;
        VlUnpacked<IData/*31:0*/, 16> u_acc_set0_copy0__DOT__mem;
        VlUnpacked<IData/*31:0*/, 16> u_acc_set0_copy1__DOT__mem;
        VlUnpacked<IData/*31:0*/, 16> u_acc_set1_copy0__DOT__mem;
        VlUnpacked<IData/*31:0*/, 16> u_acc_set1_copy1__DOT__mem;
        VlUnpacked<CData/*0:0*/, 12> u_fp_add_accum__DOT__v_pipe;
        VlUnpacked<IData/*31:0*/, 12> u_fp_add_accum__DOT__d_pipe;
        VlUnpacked<CData/*0:0*/, 12> u_fp_add_reduce__DOT__v_pipe;
        VlUnpacked<IData/*31:0*/, 12> u_fp_add_reduce__DOT__d_pipe;
    };

    // INTERNAL VARIABLES
    Vtb_systolic_dma_top__Syms* const vlSymsp;

    // PARAMETERS
    static constexpr IData/*31:0*/ DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ ACC_BANKS = 0x00000010U;
    static constexpr IData/*31:0*/ MUL_LATENCY = 9U;
    static constexpr IData/*31:0*/ ADD_LATENCY = 0x0000000cU;
    static constexpr IData/*31:0*/ ACC_SEL_W = 4U;
    static constexpr IData/*31:0*/ u_fp_mul__DOT__LAT = 9U;
    static constexpr IData/*31:0*/ u_acc_set0_copy0__DOT__DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ u_acc_set0_copy0__DOT__DEPTH = 0x00000010U;
    static constexpr IData/*31:0*/ u_acc_set0_copy0__DOT__ADDR_W = 4U;
    static constexpr IData/*31:0*/ u_acc_set0_copy1__DOT__DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ u_acc_set0_copy1__DOT__DEPTH = 0x00000010U;
    static constexpr IData/*31:0*/ u_acc_set0_copy1__DOT__ADDR_W = 4U;
    static constexpr IData/*31:0*/ u_acc_set1_copy0__DOT__DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ u_acc_set1_copy0__DOT__DEPTH = 0x00000010U;
    static constexpr IData/*31:0*/ u_acc_set1_copy0__DOT__ADDR_W = 4U;
    static constexpr IData/*31:0*/ u_acc_set1_copy1__DOT__DATA_W = 0x00000020U;
    static constexpr IData/*31:0*/ u_acc_set1_copy1__DOT__DEPTH = 0x00000010U;
    static constexpr IData/*31:0*/ u_acc_set1_copy1__DOT__ADDR_W = 4U;
    static constexpr IData/*31:0*/ u_fp_add_accum__DOT__LAT = 0x0000000cU;
    static constexpr IData/*31:0*/ u_fp_add_reduce__DOT__LAT = 0x0000000cU;

    // CONSTRUCTORS
    Vtb_systolic_dma_top_systolic_pe(Vtb_systolic_dma_top__Syms* symsp, const char* v__name);
    ~Vtb_systolic_dma_top_systolic_pe();
    VL_UNCOPYABLE(Vtb_systolic_dma_top_systolic_pe);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
