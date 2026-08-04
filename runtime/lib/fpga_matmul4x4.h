#ifndef FPGA_MATMUL4X4_H
#define FPGA_MATMUL4X4_H

// 開啟並設定 UART 連線,回傳 file descriptor,失敗回傳 -1
int fpga_uart_open(const char *port);

// 關閉連線
void fpga_uart_close(int fd);

// 執行 C = A @ B + C_init (全部都是 4x4 float32,row-major)
// 回傳 0 成功,非 0 表示逾時或協定錯誤
int fpga_matmul4x4(int fd, const float A[16], const float B[16],
                    const float C_init[16], float C_out[16]);

#endif
