# FPGA 與編譯器原始碼重建封存：Shared Writeback R2

本封存保存 **2026-10-26** 對新建置 FPGA 映像的燒錄與實板驗證，以及從目前 Git 原始碼乾淨建置的編譯器、Runtime 和整合測試證據。所有測試均實際執行，不以歷史 Log 取代本次測試；舊封存未覆蓋。

## 驗證結論

| 項目 | 狀態 | 證據範圍 |
|---|---|---|
| FPGA Source Rebuild | VERIFIED | 新專案合成、實作、bitstream 與時序／DPTI 關卡紀錄完整；組態記錄乾淨 Git 來源 |
| Source Rebuild | VERIFIED | 指本次新 FPGA 產物，非舊 bitstream 的二進位精確重現 |
| FPGA Programming | PASS | 指定 tag 燒錄成功，啟動狀態 HIGH，程序結束碼 0 |
| FPGA Board E2E | PASS | 新編譯器／Runtime 四組共 173/173 次案例執行 PASS |
| Board E2E | PASS | Quick、Regression、Sync、Async 均通過 |
| Compiler Source Rebuild | VERIFIED | 全新獨立 Build Directory，CMake 設定與 59/59 建置步驟成功 |
| Compiler Regression Tests | PASS | 新工具執行 lit/FileCheck 172/172，FAIL 0、SKIP 0 |
| Compiler → FPGA End-to-End | PASS | 新 systolic-cc 編譯的同步／非同步 GEMM、仿射映射與批次 GEMM，2,811 元素逐位元一致，非預期 CPU fallback 0 |
| 舊 bitstream 二進位精確重現 | MATCH: NO | 新舊 SHA-256 不同，不標示 Bitstream SHA-256 Reproduction: VERIFIED |

PASS 表示該項測試通過；VERIFIED 表示已有所列來源與執行證據，不代表所有可能工作負載皆已驗證。完整說明見[硬體驗證報告](hardware-report.md)與根目錄 [REPRODUCE.md](../../../../../REPRODUCE.md)。

## 硬體與來源

- Nexys Video / `xc7a200tsbg484-1`，1×8×8 + 3×4×4 FP32 加速器群。
- `VARIANT=v1`、K8=16、K4=16、深度 4、完成紀錄、512 MiB DDR3、1,024 位元組運算元資料區塊。
- Git：`075640b4872d4240a99d89af625d7dd7aa3fc207`，`RTL_TREE_DIRTY=0`。
- 專案標籤 `shared_wb_r2_tclfix_1025`，佈局 `ExtraTimingOpt`，繞線 `Explore`，DPTI 相位 0.0。
- Vivado v2026.1，SW Build 6511674；建置 2026-10-25 啟動，於 `2026-10-26T01:21:49+0800` 完成。
- WNS **+0.020144 ns**、TNS **0.000 ns**、WHS **+0.002302 ns**、THS **0.000 ns**；`TIMING_MET=1`、`DPTI_IO_GATE=PASS`。
- 本次未重跑 Vivado Build；已確認新產物成功，於 2026-10-26 實際燒錄並測試。原專案引用 `~/work/systolic-mlir`；使用者指定的 `~/work/systolic-mlir-heterogeneous` 不存在，來源選擇依 XPR 與一致的 Git HEAD 確認。

## 新編譯器與 Runtime 的實板結果

| 測試組 | 案例 PASS／總數 | CPU 比對元素 | Mismatch | 非預期逾時／Runtime 錯誤 |
|---|---|---|---|---|
| Quick Sync | 26/26 | 1,068 | 0 | 0／0 |
| Regression Sync | 52/52 | 15,875 | 0 | 0／0 |
| 完整 Sync | 43/43 | 7,008 | 0 | 0／0 |
| Regression Async | 52/52 | 15,875 | 0 | 0／0 |
| 合計 | 173/173 | 39,826 | 0 | 0／0 |

測試組之間包含重複案例，合計為執行次數，不是不同案例數。第一輪既有工具亦有相同四組 PASS，獨立保留；主要新編譯器結論使用 `fresh-` 前綴的第二輪結果。

新編譯器 lit：172 PASS、0 FAIL、0 SKIP；主機組態整合：109 PASS、0 FAIL、3 SKIP（未建置共同模擬後端）；新 `systolic-cost` R2 不變量檢查 PASS。C 至 FPGA 四組使用新 Runtime 與實際 libftdi，無非預期 CPU fallback。軟體模擬僅用於建立預期執行標籤，不能算作實板 PASS。

## 新舊映像比較

```text
OLD_SHA256: bd8aba7212a24347eba1dc497204dd6eb5117e8a8ff77094fd94c496a41f3db5
NEW_SHA256: 75f982b4161bbf7351e018d1652439d4d0aeb2275b3590281dc080ec5f5bc1a4
MATCH: NO
```

二進位內容不同，不影響新映像的建置、燒錄及實板 PASS；不宣稱精確重現 10/25 舊映像。

## 封存內容與完整性

| 路徑 | 內容 |
|---|---|
| `systolic_dma_top_1x8x8_3x4x4.bit`、`.ltx` | 本次新映像與配對探針檔 |
| `fpga_config.txt` | 僅將 BITSTREAM 改為相鄰封存檔名，其他欄位保留 |
| `build/` | 完整 Vivado Build、合成與實作紀錄 |
| `systolic_dma_top_timing_summary_routed.rpt`、`systolic_dma_top_utilization_placed.rpt` | 新映像繞線後時序與佈局後資源報告 |
| `dpti_io/`、`dpti_io_summary.txt` | DPTI I/O／時脈報告與 PASS 摘要 |
| `hardware-validation/` | 燒錄 Log、兩輪實板案例輸入／輸出／IR、完整 Log、命令與時間戳、機器可讀結果 |
| `compiler-validation/` | 新建置設定／Log／雜湊、完整 lit 結果、整合檢查、C 編譯與 FPGA 執行紀錄 |
| `provenance/` | Git commit／狀態／樹與原始碼雜湊、XPR、原始組態、建置參數及新舊 SHA 比較 |
| `hardware-report.md` | 本次驗證方法、結果與限制 |
| `SHA256SUMS` | 除清單本身外，所有封存檔案的 SHA-256 |

從此目錄執行：

```bash
sha256sum -c SHA256SUMS
```

新建置工具與 Runtime 完整雜湊位於 [new-build-sha256.json](compiler-validation/new-build-sha256.json)。建置目錄保留於獨立 `/tmp`，未覆蓋原 `build/`；封存不保證暫存建置目錄永久存在。絕對路徑是實驗來源紀錄，並非封存解壓後可直接執行的保證。

全域忽略規則仍可能隱藏尚未追蹤的 `.bit`／`.log` 等檔案；本次未修改忽略規則，未執行 git add／commit／push，仍待使用者審核。

## 驗證限制

內部同時計算、提早重用結果記憶體分組、AXI 仲裁公平性與所有背壓情況未直接在實板量測；有限案例 PASS 不等於通用死結自由或完整 FP32 特殊值驗證。DPTI 不提供映像身分讀取，實板身分以本次明確燒錄與相符雜湊／組態追溯。

初版暫存包裝誤將範例的 CPU 參考計算標籤視為 fallback；原紀錄保留，修正判定後在新目錄重跑並 PASS，沒有更動 Repository 測試程式。同步 Runtime 的 p95 統計欄位出現大於 max 的異常，本次不採用作效能結論；保留原始輸出且未修改 Runtime。
