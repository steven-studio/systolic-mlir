#!/usr/bin/env python3
"""
patch_costmodel_calib.py -- 讓 systolic.device 能攜帶自己的校正常數。

    cd ~/work/systolic-mlir
    python3 eval/scalesim/patch_costmodel_calib.py

問題
----
ArrayConfig 有 initiationInterval 與 fixedOverhead 兩個欄位,但四個 pass
(IR/SystolicCostAnalysisPass、Transforms/SystolicCostAnalysisPass、
SystolicScheduleOverlapPass、SystolicSelectDevicePass)都只填 rows / cols /
depth / clockHz / dmaBytesPerCycle,從來沒有賦值給那兩個。它們永遠是 struct
的預設值 1 和 6,而 systolic.device 上也沒有對應的 attribute。

結果是模型無法表達「不同微架構」:整個 module 共用同一組校正值。而那組
預設值來自 HLS cosim。手寫的 fold RTL 實測是 104,差 17 倍 -- 沿用預設值時
select-device 會嚴重高估這顆加速器。

為什麼是 104 而不是 118
-----------------------
矽上量到 total = k_dim + 118 (R = C = 8),但模型的幾何項已經含了
rows + cols - 2:

    perTile = II * (depth + rows + cols - 2) + fixedOverhead
            = 1 * (k_dim + 14) + fixedOverhead

    k_dim=16 -> 實測 134,幾何項 30  =>  fixedOverhead = 104
    k_dim=64 -> 實測 182,幾何項 78  =>  fixedOverhead = 104

兩個獨立的矽上量測點給出同一個值,殘差為零。填 118 會把 R+C-2 重複計算。

104 由三部分組成,沒有一項依賴 R、C 或 K:
  - 樹狀歸約約 74 拍:層數為 log2(ACC_BANKS),每層等待加法器延遲 L
  - FP mul + add 延遲 20 拍:IP 組態常數
  - 陣列輸出 FSM 與 context 切換

所以它對這個微架構是真常數,取決於 (ACC_BANKS, L) 而非陣列形狀。

改動
----
1. SystolicOps.td      systolic.device 新增兩個 optional i64 attribute
2. 四個 pass           讀進 ArrayConfig(缺席時沿用預設值,不影響既有測試)
3. CostModel.h/.cpp    註明這兩個是「每微架構」的校正常數,並記錄兩組實測
4. 新增 lit test       鎖住 fold RTL 的六個點

既有的 cost_analysis.mlir 與 select_device.mlir 不受影響 -- 它們的 device
沒有這兩個 attribute,行為與改動前完全相同。

冪等。每個被改的檔案備份到 <file>.bak_calib。
"""

import sys
import os
import shutil

# ------------------------------------------------------------------ 1. td

TD_OLD = """    OptionalAttr<F64Attr>:$clock_hz,
    OptionalAttr<F64Attr>:$dma_bytes_per_cycle
  );
"""

TD_NEW = """    OptionalAttr<F64Attr>:$clock_hz,
    OptionalAttr<F64Attr>:$dma_bytes_per_cycle,

    // Calibration constants, per microarchitecture rather than per array
    // shape. The geometric term of the cost model is determined by rows,
    // cols and depth; these two describe what the datapath adds on top of
    // it -- pipeline latency of the arithmetic, the final reduction, and
    // whatever the output handshake costs.
    //
    // They belong on the device rather than in the model because two
    // accelerators of identical geometry can differ here by more than an
    // order of magnitude. The HLS pipeline this model was first fitted to
    // has fixed_overhead 6; the hand-written fold RTL measures 104. A
    // single global constant cannot describe both, and using the wrong one
    // makes device selection pick the wrong accelerator for the right
    // reason.
    //
    // Absent means "use the ArrayConfig default", which keeps every
    // existing device declaration behaving exactly as before.
    OptionalAttr<I64Attr>:$initiation_interval,
    OptionalAttr<I64Attr>:$fixed_overhead
  );
"""

# ------------------------------------------------------------------ 2. pass

PASS_OLD = """      if (FloatAttr bw = device.getDmaBytesPerCycleAttr())
        cfg.dmaBytesPerCycle = bw.getValueAsDouble();
"""

PASS_NEW = """      if (FloatAttr bw = device.getDmaBytesPerCycleAttr())
        cfg.dmaBytesPerCycle = bw.getValueAsDouble();
      // Calibration is a property of the microarchitecture, not of the
      // array shape, so it travels on the device op. Leaving the attribute
      // off keeps the ArrayConfig default.
      if (IntegerAttr ii = device.getInitiationIntervalAttr())
        cfg.initiationInterval = ii.getInt();
      if (IntegerAttr fo = device.getFixedOverheadAttr())
        cfg.fixedOverhead = fo.getInt();
"""

PASS_FILES = [
    "lib/Systolic/IR/SystolicCostAnalysisPass.cpp",
    "lib/Systolic/Transforms/SystolicCostAnalysisPass.cpp",
    "lib/Systolic/Transforms/SystolicScheduleOverlapPass.cpp",
    "lib/Systolic/Transforms/SystolicSelectDevicePass.cpp",
]

# ------------------------------------------------------------------ 3. header

H_OLD = """  int64_t initiationInterval = 1; // 校正值：cosim 實測 II
  int64_t fixedOverhead = 6;      // 校正值：init/drain 迴圈 + 介面握手
"""

H_NEW = """  // Calibration constants. These describe the datapath, not the array
  // shape, and must be measured per microarchitecture -- two arrays of
  // identical geometry can differ here by more than an order of magnitude.
  //
  // The defaults below are the HLS pipeline the model was first fitted to
  // (C/RTL cosim, 14 configurations, residual zero). The hand-written fold
  // RTL on xc7a200t measures fixedOverhead = 104 with the same II = 1;
  // see test/Systolic/cost_model_fold_rtl.mlir.
  //
  // A device that carries `initiation_interval` / `fixed_overhead`
  // attributes overrides these.
  int64_t initiationInterval = 1;
  int64_t fixedOverhead = 6;
"""

CPP_OLD = """  // II and fixedOverhead are calibrated by C/RTL co-simulation over 14
  // (rows, cols, depth) configurations: II = 1, fixedOverhead = 6, residual
  // exactly zero at every point (TIME_STEPS 8..70, rows+cols 4..34).
"""

CPP_NEW = """  // II and fixedOverhead are calibration constants, and which values are
  // correct depends on the microarchitecture rather than on the geometry
  // this function computes.
  //
  //   HLS pipeline    II = 1, fixedOverhead =   6
  //     C/RTL cosim over 14 (rows, cols, depth) configurations, residual
  //     exactly zero at every point (TIME_STEPS 8..70, rows+cols 4..34).
  //
  //   fold RTL 8x8    II = 1, fixedOverhead = 104
  //     xc7a200t at 100 MHz. Silicon measurement at k_dim = 16 and 64
  //     gives 134 and 182 cycles; subtracting the geometric term
  //     (k_dim + 8 + 8 - 2) leaves 104 at both points.
  //
  // The two differ by more than 17x on the same formula, which is why the
  // constants live on the device op and not here.
"""

# ------------------------------------------------------------------ 4. test

TEST_PATH = "test/Systolic/cost_model_fold_rtl.mlir"

TEST = r'''// RUN: systolic-opt --systolic-cost-analysis %s | FileCheck %s

// Pins the cost model to the hand-written 8x8 fold RTL, whose calibration
// differs from the HLS pipeline in cost_analysis.mlir by more than 17x.
//
//   cycles_tile = II * (depth + rows + cols - 2) + fixedOverhead
//   II = 1, fixedOverhead = 104, rows = cols = 8
//             => depth + 118
//
// Why 104 and not the 118 the hardware counter reports: the measured
// number is end-to-end, and the geometric term already accounts for
// rows + cols - 2 = 14. Two independent silicon points agree:
//
//   k_dim = 16 -> measured 134, geometric 30 -> 104
//   k_dim = 64 -> measured 182, geometric 78 -> 104
//
// The 104 is the datapath, not the array: roughly 74 cycles of tree
// reduction (log2(ACC_BANKS) levels, each waiting the FP adder latency),
// 20 cycles of multiplier plus adder pipeline, and the output handshake.
// None of those terms depends on rows, cols or depth, which is why a
// single constant is the right shape for them.
//
// `depth` is the K-tile depth the hardware is configured for. On this
// design it is a runtime value (k_dim in the request header), so each
// device below is the same silicon under a different configuration --
// not six different accelerators.
//
// Provenance: k_dim 8/16/24/48/64 measured in xsim via tb_array_fold_kmax
// (drain = 111 at every point, errors = 0); k_dim 16 and 64 additionally
// measured on the board through the hardware cycle counter. k_dim = 32 is
// held out -- it is predicted here and has never been measured.

module {
  systolic.device @fold_k8  rows = 8 cols = 8 depth = 8
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k16 rows = 8 cols = 8 depth = 16
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k32 rows = 8 cols = 8 depth = 32
      dataflow = output_stationary {fixed_overhead = 104 : i64}
  systolic.device @fold_k64 rows = 8 cols = 8 depth = 64
      dataflow = output_stationary {fixed_overhead = 104 : i64}

  // xsim: 126. 1 * (8 + 8 + 8 - 2) + 104 = 126.
  // CHECK-LABEL: func.func @fold_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 126
  func.func @fold_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                    %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Silicon: the hardware cycle counter reported 134.
  // CHECK-LABEL: func.func @fold_16
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 134
  func.func @fold_16(%a: tensor<8x16xf32>, %b: tensor<16x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k16
         {m = 8 : i64, n = 8 : i64, k = 16 : i64}
         : (tensor<8x16xf32>, tensor<16x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Held out: never measured, in simulation or on the board. If the model
  // is right this is 150.
  // CHECK-LABEL: func.func @fold_32
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 150
  func.func @fold_32(%a: tensor<8x32xf32>, %b: tensor<32x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k32
         {m = 8 : i64, n = 8 : i64, k = 32 : i64}
         : (tensor<8x32xf32>, tensor<32x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Silicon: the hardware cycle counter reported 182.
  // CHECK-LABEL: func.func @fold_64
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 182
  func.func @fold_64(%a: tensor<8x64xf32>, %b: tensor<64x8xf32>,
                     %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k64
         {m = 8 : i64, n = 8 : i64, k = 64 : i64}
         : (tensor<8x64xf32>, tensor<64x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Two K tiles on the k=32 configuration: ceil(64/32) = 2 tiles, each
  // 150 -> 300. The fold design does pay the reduction drain once per
  // invocation, so charging fixedOverhead per tile is right for it. A
  // design that pipelined one tile's drain under the next tile's fill
  // would not be described correctly by this model -- that is a stated
  // limitation, not an accident of the constants.
  // CHECK-LABEL: func.func @fold_two_tiles
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 300
  func.func @fold_two_tiles(%a: tensor<8x64xf32>, %b: tensor<64x8xf32>,
                            %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @fold_k32
         {m = 8 : i64, n = 8 : i64, k = 64 : i64}
         : (tensor<8x64xf32>, tensor<64x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }

  // Same geometry, no calibration attribute: falls back to the HLS
  // default of 6 and gives 8 + 14 + 6 = 28. This is the whole point --
  // identical rows, cols and depth, 4.8x apart in cost, and the model can
  // only tell them apart because the constant is on the device.
  systolic.device @hls_k8 rows = 8 cols = 8 depth = 8
      dataflow = weight_stationary

  // CHECK-LABEL: func.func @hls_8
  // CHECK: systolic.matmul_tile
  // CHECK-SAME: est_cycles = 28
  func.func @hls_8(%a: tensor<8x8xf32>, %b: tensor<8x8xf32>,
                   %c: tensor<8x8xf32>) -> tensor<8x8xf32> {
    %0 = systolic.matmul_tile %a, %b, %c on @hls_k8
         {m = 8 : i64, n = 8 : i64, k = 8 : i64}
         : (tensor<8x8xf32>, tensor<8x8xf32>, tensor<8x8xf32>)
           -> tensor<8x8xf32>
    return %0 : tensor<8x8xf32>
  }
}
'''


def main():
    root = os.getcwd()
    if not os.path.isdir(os.path.join(root, "lib", "Systolic")):
        print("ERROR: 請在 systolic-mlir 的根目錄執行")
        return 1

    td = "include/Systolic/SystolicOps.td"
    h = "include/Systolic/CostModel.h"
    cpp = "lib/Systolic/Transforms/CostModel.cpp"

    # 冪等
    src_td = open(td, encoding="utf-8").read()
    if "fixed_overhead" in src_td:
        print("已經套用過(SystolicOps.td 裡已有 fixed_overhead),不做任何事。")
        return 0

    plan = [(td, TD_OLD, TD_NEW), (h, H_OLD, H_NEW), (cpp, CPP_OLD, CPP_NEW)]
    plan += [(f, PASS_OLD, PASS_NEW) for f in PASS_FILES]

    problems = []
    for path, old, _ in plan:
        if not os.path.isfile(path):
            problems.append(f"  找不到 {path}")
            continue
        n = open(path, encoding="utf-8").read().count(old)
        if n != 1:
            problems.append(f"  {path}: 錨點出現 {n} 次(需要剛好 1 次)")

    if problems:
        print("ERROR: 錨點對不上,沒有做任何修改。")
        print("\n".join(problems))
        return 1

    for path, old, new in plan:
        s = open(path, encoding="utf-8").read()
        bak = path + ".bak_calib"
        if not os.path.exists(bak):
            shutil.copy2(path, bak)
        open(path, "w", encoding="utf-8").write(s.replace(old, new, 1))
        print(f"  ok  {path}")

    with open(TEST_PATH, "w", encoding="utf-8") as fh:
        fh.write(TEST)
    print(f"  ok  {TEST_PATH}  (新增)")

    print()
    print("重建與測試:")
    print("  cmake --build build -j$(nproc)")
    print("  cmake --build build --target check-systolic")
    print()
    print("既有的 cost_analysis.mlir 與 select_device.mlir 應該全部照舊通過 --")
    print("它們的 device 沒有新 attribute,行為與改動前完全相同。")
    return 0


if __name__ == "__main__":
    sys.exit(main())
