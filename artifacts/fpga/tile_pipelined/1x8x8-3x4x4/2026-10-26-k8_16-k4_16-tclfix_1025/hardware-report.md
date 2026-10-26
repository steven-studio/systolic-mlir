# 新建置 Shared Writeback R2 與編譯器實板驗證報告

## 1. 身分與驗證結論

日期：2026-10-26（Asia/Taipei）。Git commit：`075640b4872d4240a99d89af625d7dd7aa3fc207`。

```text
FPGA Source Rebuild: VERIFIED
Source Rebuild: VERIFIED
FPGA Programming: PASS
FPGA Board E2E: PASS
Board E2E: PASS
Compiler Source Rebuild: VERIFIED
Compiler Regression Tests: PASS
Compiler → FPGA End-to-End: PASS
OLD_SHA256: bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5
NEW_SHA256: 75f982b4161bbf7351e018d1652439d4d0aeb2275b3590281dc080ec5f5bc1a4
MATCH: NO
```

狀態表示本次指定範圍已取得證據，不是所有輸入或內部硬體性質的全面保證。新舊 SHA 不同，**不標示** Bitstream SHA-256 Reproduction: VERIFIED。

## 2. FPGA 原始碼建置證據

- 原始專案：`$HOME/work/vivado/tile_pipelined/systolic_dma_v1_k8_16_k4_16_1x8x8_3x4x4_shared_wb_r2_tclfix_1025/`。
- XPR 原始碼引用實際 `~/work/systolic-mlir`，而非不存在的 `~/work/systolic-mlir-heterogeneous`。Git HEAD 與 fpga_config 的 `RTL_GIT_REV` 相同；`RTL_TREE_DIRTY=0`。
- 開始驗證時已追蹤來源無修改，只有既存未追蹤 `build_recovery/`。原始碼雜湊與 Git 樹保存在 `provenance/`；本次僅新增封存、修改 REPRODUCE.md，未修改程式碼。
- 參數：`build v1 16 16 1 1 3`；tag `shared_wb_r2_tclfix_1025`；`ExtraTimingOpt`／`Explore`；DPTI 相位 0.0；Nexys Video、1×8×8 + 3×4×4、K8/K4=16/16。
- Vivado v2026.1（64 位元），SW Build 6511674、IP Build 6504888、SharedData Build 6501428。
- 建置從 2026-10-25 20:44:12 執行至 2026-10-26 01:21:49；`build/vivado-build.log` 記錄本 tag、產物位置、最終時序與 DPTI 檢查。
- `build/synthesis.log` 與 `build/implementation.log` 對應此專案；實作紀錄顯示 `write_bitstream completed successfully`、0 Errors、0 Critical Warnings（另有 5 Warnings，完整保留）。
- 組態精確 WNS/TNS/WHS/THS：**+0.020144 / 0.000 / +0.002302 / 0.000 ns**；報告顯示精度為 0.020/0.000/0.002/0.000，全部使用者時序限制滿足。
- `dpti_io_summary.txt` 顯示 FT2232H I/O 與時脈關卡 PASS、無未約束 FT2232H 連接埠、建立／保持餘裕為正、I/O 暫存器位於預期 ILOGIC/OLOGIC。WHS 含 0.150 ns 保守保持時間量，結論受 XDC 假設限制。

本次驗證沒有重新啟動 synthesis／implementation／bitstream generation。Source Rebuild VERIFIED 來自此已完成的新原始碼建置紀錄及其實際產物後續燒錄與測試，不是預檢 PASS。

## 3. 實際燒錄與組態

使用同 tag 執行：

```bash
SYSTOLIC_PROJ_TAG=shared_wb_r2_tclfix_1025 \
  vivado -mode batch -nojournal -log programming.log \
  -source /home/steven-studio/work/systolic-mlir/rtl/multi_200t/tile_pipelined/dma/dma_top_build.tcl \
  -tclargs program v1 16 16 1 1 3
```

執行目錄為獨立 `/tmp/systolic-rebuild-validation-20261026-vrZKJ5`，沒有覆蓋根目錄 Vivado Log。`hardware-validation/programming.log` 與 `programming-console.log` 顯示 `program_hw_devices` 成功、`End of startup status: HIGH`、成功燒錄訊息，程序結束碼 0，2026-10-26 08:21:09 結束。

Runtime 使用原 impl_1 的 `fpga_config.txt`：K8/K4=16/16、NUM_8X8=1、NUM_4X4=3、slab 1024、COMPLETION_HEADER=1、PIPELINE_DEPTH=4、DDR_BYTES=536870912。測試前以 Runtime `--print-config` 核對；封存組態的主機接受結果另存 `compiler-validation/archived-config-parse.log`。

本封存組態僅調整 `BITSTREAM` 為相鄰 `.bit`，原組態保留於 `provenance/fpga_config.original.txt`。DPTI 無映像身分讀回協定；不能把組態剖析 PASS 說成 FPGA 自行回報身分。本次以成功燒錄的檔案、相符 SHA 與後續實板測試建立追溯。

## 4. 第一輪既有工具實板測試

在使用者追加乾淨編譯器重建要求前，已實際執行並完成四組：

| 目錄 | PASS／總數 | CPU 比對元素 | Mismatch | 非預期逾時 | 非預期 Runtime 錯誤 |
|---|---|---|---|---|---|
| `hardware-validation/quick-sync/` | 26/26 | 1,068 | 0 | 0 | 0 |
| `hardware-validation/regression-sync/` | 52/52 | 15,875 | 0 | 0 | 0 |
| `hardware-validation/full-sync/` | 43/43 | 7,008 | 0 | 0 | 0 |
| `hardware-validation/regression-async/` | 52/52 | 15,875 | 0 | 0 | 0 |

完整命令與日期見 `hardware-validation/executions.json`；完整主控台 Log 與每案輸入／輸出／IR 均保留，舊執行檔雜湊在 `provenance/test-binaries.json`。此輪不作為 Compiler Source Rebuild 的證據。

## 5. 乾淨編譯器建置與迴歸測試

新的 Build Directory：`/tmp/systolic-rebuild-validation-20261026-vrZKJ5/compiler-build`。依正式 `scripts/reproducibility/build.sh`，指定 LLVM/MLIR 24 與 clang 24，以 `--jobs 4` 完成全新 CMake Configuration 與 59/59 Compilation，既有 `build/` 未覆蓋。確切命令見 `compiler-validation/commands.txt`。

工具鏈：LLVM/MLIR 與 clang 24.0.0git，來源版本 `7615da10b75c57286a4032eac8bb43c1b8d2ba46`。缺少的 `count`／`not` 從相同 LLVM 24 原始碼另外建置至暫存目錄，不使用 LLVM 18 輔助工具，不更動原始碼。新專案工具與 Runtime 的 SHA-256 見 `compiler-validation/new-build-sha256.json`。

| 測試範圍 | 總數／PASS | FAIL | SKIP | 證據 |
|---|---|---|---|---|
| CFront | 14/14 | 0 | 0 | `compiler-lit-results.json`、`compiler-lit.log` |
| Systolic（Dialect、仿射映射、分塊／裝置／排程／成本） | 114/114 | 0 | 0 | 同上 |
| SystolicCC（編譯器與軟體 Runtime） | 31/31 | 0 | 0 | 同上 |
| gemm_recognition | 12/12 | 0 | 0 | 同上 |
| reproducibility | 1/1 | 0 | 0 | 同上 |
| 完整 lit/FileCheck | 172/172 | 0 | 0 | 66.81 秒，逐命令完整紀錄 |
| 主機組態整合 | 112 項：109 PASS | 0 | 3 | `fresh-integration-config.log` |
| R2 不變量 | PASS | 0 | 0 | `fresh-r2-invariants.log` |

三項 SKIP 都因新目錄沒有共同模擬 `libftdi_cosim.so`，沒有以舊模擬工具補結果。完整 lit 由新工具路徑及新 lit.site.cfg 指定；Log 中實際命令引用舊 `build/bin` 的數量為 0。新 `systolic-cost` 不變量檢查涵蓋 R2 資源、循環輪詢、回應錯誤與輸出排空；本次未重新執行完整多 K RTL 差異比對掃描。

## 6. 第二輪新編譯器／新 Runtime 的實板 E2E

`compiler-validation/fresh-executions.json` 保留各命令、乾淨環境、Git commit、起訖時間與結束碼；每組指定 `--build-dir` 為新目錄、`--backend board` 與原新映像組態。

| 目錄 | PASS／總數 | CPU 比對元素 | Mismatch | 非預期逾時 | 非預期 Runtime 錯誤 |
|---|---|---|---|---|---|
| `hardware-validation/fresh-quick-sync/` | 26/26 | 1,068 | 0 | 0 | 0 |
| `hardware-validation/fresh-regression-sync/` | 52/52 | 15,875 | 0 | 0 | 0 |
| `hardware-validation/fresh-full-sync/` | 43/43 | 7,008 | 0 | 0 | 0 |
| `hardware-validation/fresh-regression-async/` | 52/52 | 15,875 | 0 | 0 | 0 |
| 合計（獨立執行次數，含重複案例） | 173/173 | 39,826 | 0 | 0 | 0 |

每案結果與 Runtime 回傳碼均為成功，組態拒絕及 Runtime 層均 PASS。非同步 Runtime 各適用模式通過：P 模式 `wait_for(1 ms)` 預期 -20，以及 V／H 刻意非法參數／不完整 K 歸約的失敗紀錄，皆屬成功的負向測試，不是非預期故障。F 模式傳輸失敗注入僅供共同模擬，實板為 NOT APPLICABLE，未計為 PASS。

## 7. 新 systolic-cc 的 C → FPGA 整合

四組都使用新 `systolic-cc`、`--runtime=board --fp-reassociation --explain --verify`，指定新 Build Directory、clang／LLVM 路徑、實際新映像組態，以及同步或非同步提交。所有編譯產物、IR、連結命令、libftdi 相依性與執行追蹤位於 `compiler-validation/c-board/`。

| 目錄 | 工作負載 | FPGA 函式執行 | 計畫內 CPU 參考 | 非預期 fallback | 逐位元比對元素 | 結果 |
|---|---|---|---|---|---|---|
| `gemm_example-sync` | 24×20×40 GEMM，同步 | 1 | 1 | 0 | 480 | PASS |
| `gemm_example-async` | 相同 GEMM，非同步 | 1 | 1 | 0 | 480 | PASS |
| `affine_mapping-async` | 五種仿射映射核心 | 5 | 0 | 0 | 619 | PASS |
| `affine_batched-async` | 九種批次核心 | 9 | 0 | 0 | 1,232 | PASS |
| 合計 | — | 16 | 2 | 0 | 2,811 | PASS |

此處 FPGA 次數為函式呼叫，不是 tile invocation；批次函式內可能包含多個 GEMM 群組。`ir/commands.txt` 顯示降階呼叫新工具，最終連結新 `compiler-build/runtime/libsystolic_fpga_runtime.a`。`execution.log` 保留 Runtime 工作、DPTI MEM_WRITE／WRITE32、完成與結果讀取、FPGA Executed 標籤及 `--verify` CPU 比對。

為區分刻意在主機執行的 CPU 參考與 fallback，另外用軟體 Runtime 的模擬硬體標籤產生預期序列；正式實板的序列須完全一致。軟體參考 Log 不算實板證據，正式執行未設定 `SOFT_RUNTIME_MOCK_HARDWARE`、`LD_PRELOAD` 或模擬通訊端，使用系統 libftdi1。

第一輪暫存包裝誤將 `gemm_example.c` 的 `CPU Executed: GEMM #2` 參考函式判為 fallback。該程式本身已回傳 0、GEMM #1 在 FPGA 執行且 480 元素一致；中止的是包裝斷言。原始資料保留於 `compiler-validation/initial-wrapper-check/`，未覆蓋；僅修正本次新增的暫存包裝判定，在新目錄重新執行四組並全部通過，未改動 Repository 測試。

## 8. 封存方式與限制

`.bit`、LTX、建置紀錄與報告皆從指定新專案複製；原檔與舊封存不改動。新封存的組態只改相對 BITSTREAM 路徑，原始組態另存。`SHA256SUMS` 涵蓋全部封存檔案（清單本身除外），可用 `sha256sum -c SHA256SUMS` 驗證。

- 兩輪 E2E 共 346 次案例執行，包含重複案例；總 Mismatch 0。新編譯器主要 E2E 為第二輪 173 次，另有四個 C 整合執行。
- 新舊 SHA 不同，不宣稱 bitstream 位元組精確重現。
- 無直接實板波形證明同時計算、提早結果分組重用、所有 AXI 背壓／錯誤與仲裁公平性；數值 PASS 不等於通用死結自由。
- 同步 Runtime 的 `p95_us` 大於 `max_us`，統計欄位不一致；完整保留但不採用作效能結論，未修改 Runtime。
- lit PASS 與模型不變量 PASS 不替代完整 RTL 掃描或實板 makespan 校準；共同模擬相關三個 SKIP 明確保留。
- 未修改 RTL、Compiler、Runtime、Cost Model、Vivado Tcl 或 Repository 測試程式；未執行 git add、commit、push、reset 或 clean。
