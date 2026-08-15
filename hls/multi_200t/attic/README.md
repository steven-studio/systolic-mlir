# attic — 移出主線,但不刪除

| 檔案 | 為什麼在這 |
|---|---|
| `hls_k*.log` | Vitis HLS 各 K 的執行 log。結論已進 `../sweep_results.csv` |
| `recover_originals.sh` | 一次性的檔案復原腳本,已執行完畢,無人引用 |
| `timing_fix_new.tcl` | fo=2048 控制網的時序修補嘗試,無人引用 |
| `run_cosim_new.tcl` | cosim 流程,已被 `../run_hls.tcl` 取代 |
| `design_tb.cpp` / `testbench.cpp` | HLS 階段的 testbench。`testbench.cpp` 檔頭還寫著 `hls/gemm_4x4/`,是複本 |
| `test_dual_8x8.py` | 雙 8x8 的煙霧測試。對應的 bitstream 已無法重建 |
| `hls_8x8_cosim.cfg` | cosim 專用設定,與 `../hls_8x8.cfg` 只差幾行 |

沒有一個被主線引用（搬移前 grep 確認過）。要用的話原地就能跑。
