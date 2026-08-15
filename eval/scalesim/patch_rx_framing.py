#!/usr/bin/env python3
"""
patch_rx_framing.py -- 用顯式的 START / END 標記重寫 RX framing。

    python3 patch_rx_framing.py hls/multi_200t/fold_pipelined/rtl/systolic_uart_fold_top.sv

問題
----
一筆交易是固定長度的連續 byte，沒有邊界標記，也沒有回報錯誤的通道。
主機送錯長度一次，rx_count / byte_pos / hdr_done 就停在交易中間，之後
每一筆請求都被切成兩半跨在兩筆交易上，而且錯誤會一直延續到手動 reset。

不用「byte 間隔」當邊界的理由
-----------------------------
1028 bytes 在 115200 baud 下要傳 89 ms。這段期間主機只要被排程器踢掉、
觸發一次 GC、或 USB host controller 忙一下，burst 中間就會出現間隔。
用間隔當邊界的話，硬體會把一筆「合法」的請求攔腰砍斷 —— 從「送錯才錯」
變成「送對也可能錯，看運氣」。把正確性押在 OS 排程行為上是不能接受的。

作法
----
    FRAME_START(4) | HDR(4) | PAYLOAD(RX_BYTES) | FRAME_END(4)

  RX_HUNT   每個進來的 byte 推進一個 32-bit 移位暫存器，比對 FRAME_START。
            這是滑動視窗，卡在任何位置都能自己找回邊界，不需要回捲。

  RX_BODY   照原本的邏輯收 header 與 payload，照常寫進 A_buf / B_buf。

  RX_TAIL   收 4 個 byte 比對 FRAME_END。
            對了才 matrices_ready <= 1；不對就整筆丟掉，回到 RX_HUNT。

payload 是任意的 float32，任何 byte 值都可能出現，所以 START 有機會誤
觸發。但那筆的 END 必定對不上，於是被拒絕、回去繼續 hunt —— 最多一個
frame 長度就收斂，因此不需要 byte-stuffing（SLIP / COBS）。
隨機資料撞上某個特定 32-bit 值的機率是 2^-32，一個 frame 內約 2.4e-7。

payload 採「樂觀寫入」：邊收邊寫 A_buf / B_buf，但只有 END 驗過才啟動
計算。誤觸發寫進去的垃圾在被讀之前就會被下一個真 frame 蓋掉，所以不需要
1036 bytes 的緩衝區。

實際的自癒行為（板子 K_MAX=16、主機誤送 K_MAX=64）
--------------------------------------------------
  主機送 4108 bytes，硬體的 frame 是 1036 bytes。
  HUNT 在 byte 0 找到 START，BODY 吃掉 1028，TAIL 檢查 byte 1032..1035
  —— 那是 payload 資料，對不上 END，整筆丟棄回到 HUNT。
  剩下的 3072 bytes 裡沒有 START，全部被 HUNT 忽略。
  下一筆正確的請求在它自己的 byte 0 被找到 -> PASS。

  一個 frame 內復原，不需要按 BTNC。

host 端要同步修改
-----------------
    FRAME_START = 0xA55AC33C
    FRAME_END   = 0x5AA53CC3
    req = struct.pack('<I', FRAME_START) + req + struct.pack('<I', FRAME_END)

冪等。原檔備份到 <file>.bak_framing。
"""

import sys
import os
import shutil

DECL_ANCHOR = "    logic matrices_ready;\n"

DECL_NEW = '''    logic matrices_ready;


    /*
     * ============================================================
     * RX framing
     * ============================================================
     *
     *   FRAME_START(4) | HDR(4) | PAYLOAD(RX_BYTES) | FRAME_END(4)
     *
     * The transaction used to be a bare fixed-length burst. With no
     * delimiter there is no way to tell where one request ends and
     * the next begins, so a host that sent the wrong number of bytes
     * left the byte counters pointing into the middle of a frame and
     * every later request was split across two of them. That state
     * survived until the board was reset by hand.
     *
     * The obvious cheap alternative -- treat a gap between bytes as
     * a boundary -- is not sound. One transaction takes 89 ms on the
     * wire, and any host-side stall inside that window would split a
     * VALID request. That trades "wrong length fails" for "correct
     * length sometimes fails", which is worse: it makes correctness
     * depend on the host's scheduler.
     *
     * sync_sr is a sliding window over the last four bytes, ordered
     * to match the little-endian word convention used everywhere
     * else on this interface. Matching on a sliding window is what
     * makes HUNT self-synchronising: whatever offset the receiver is
     * stuck at, it walks forward one byte at a time until the marker
     * lines up. No rewind and no buffer are needed.
     *
     * A false START inside float payload is possible -- any byte
     * value can occur -- but the END check then fails and the frame
     * is discarded, so the receiver converges within one frame. The
     * odds are 2^-32 per offset, about 2.4e-7 per frame, which is
     * why byte stuffing (SLIP/COBS) is not worth its cost here.
     * ============================================================
     */
    localparam logic [31:0] FRAME_START = 32'hA55A_C33C;
    localparam logic [31:0] FRAME_END   = 32'h5AA5_3CC3;

    typedef enum logic [1:0] {
        RX_HUNT,
        RX_BODY,
        RX_TAIL
    } rx_state_t;

    rx_state_t rx_state;

    logic [31:0] sync_sr;
    logic [1:0]  tail_cnt;

    /*
     * Oldest of the four bytes ends up in bits [7:0], matching
     * rx_word above, so a marker constant reads the same way a
     * header word does.
     */
    wire [31:0] sync_next = {rx_byte, sync_sr[31:8]};
'''


OLD_BLOCK = """    always_ff @(posedge clk) begin
        if (rst) begin

            rx_count       <= '0;
            byte_pos       <= 2'd0;
            word_buf       <= 32'd0;
            matrices_ready <= 1'b0;
            hdr_done       <= 1'b0;
            k_dim          <= FEED_W'(K_MAX);

        end
        else begin

            matrices_ready <= 1'b0;

            if (rx_valid) begin

                case (byte_pos)

                    2'd0:
                        word_buf[7:0] <= rx_byte;

                    2'd1:
                        word_buf[15:8] <= rx_byte;

                    2'd2:
                        word_buf[23:16] <= rx_byte;

                    2'd3: begin

                        word_buf[31:24] <= rx_byte;

                        /*
                         * The first complete word of a transaction
                         * is the header, not operand data.
                         */
                        if (!hdr_done) begin

                            k_dim <= hdr_valid ? FEED_W'(hdr_k)
                                               : FEED_W'(K_MAX);

                        end

                        /*
                         * Absolute k is {window, col} for A and
                         * {window, row} for B -- each k window is
                         * exactly 8 deep, so the concatenation is
                         * window*8 + offset with no adder.
                         */
                        else if (rx_is_b) begin

                            B_buf[{rx_win, rx_row}]
                                 [rx_col]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end
                        else begin

                            A_buf[rx_row]
                                 [{rx_win, rx_col}]
                                <= {
                                    rx_byte,
                                    word_buf[23:0]
                                };

                        end

                    end

                endcase


                if (byte_pos == 2'd3)
                    byte_pos <= 2'd0;
                else
                    byte_pos <= byte_pos + 1'b1;


                /*
                 * Header bytes do not advance the payload counter,
                 * so rx_count still addresses operand storage
                 * exactly as before: the write at byte_pos == 3
                 * sees rx_count = 4w+3 for payload word w.
                 *
                 * hdr_done is cleared at end of transaction, which
                 * rearms the header for the next request -- every
                 * invocation therefore carries its own k_dim.
                 */
                if (!hdr_done) begin

                    if (byte_pos == 2'd3)
                        hdr_done <= 1'b1;

                end
                else if (rx_count == RX_CNT_W'(RX_BYTES - 1)) begin

                    rx_count       <= '0;
                    hdr_done       <= 1'b0;
                    matrices_ready <= 1'b1;

                end
                else begin

                    rx_count <= rx_count + 1'b1;

                end

            end

        end
    end
"""


NEW_BLOCK = """    always_ff @(posedge clk) begin
        if (rst) begin

            rx_state       <= RX_HUNT;
            sync_sr        <= 32'd0;
            tail_cnt       <= 2'd0;

            rx_count       <= '0;
            byte_pos       <= 2'd0;
            word_buf       <= 32'd0;
            matrices_ready <= 1'b0;
            hdr_done       <= 1'b0;
            k_dim          <= FEED_W'(K_MAX);

        end
        else begin

            matrices_ready <= 1'b0;

            if (rx_valid) begin

                /*
                 * The sliding window advances on every byte, in
                 * every state. HUNT needs it to find a marker at an
                 * arbitrary offset; TAIL needs it to read the four
                 * bytes it is checking.
                 */
                sync_sr <= sync_next;

                case (rx_state)

                /*
                 * ----------------------------------------------------
                 * Walk forward one byte at a time until the start
                 * marker lines up. Everything before it is discarded,
                 * which is what recovers from a truncated or oversized
                 * previous transfer.
                 * ----------------------------------------------------
                 */
                RX_HUNT: begin

                    if (sync_next == FRAME_START) begin

                        rx_state <= RX_BODY;

                        rx_count <= '0;
                        byte_pos <= 2'd0;
                        hdr_done <= 1'b0;

                    end

                end


                /*
                 * ----------------------------------------------------
                 * Header word followed by the operand payload. This is
                 * the original receive path, unchanged: the framing
                 * states around it decide whether its results are
                 * allowed to start a computation.
                 * ----------------------------------------------------
                 */
                RX_BODY: begin

                    case (byte_pos)

                        2'd0:
                            word_buf[7:0] <= rx_byte;

                        2'd1:
                            word_buf[15:8] <= rx_byte;

                        2'd2:
                            word_buf[23:16] <= rx_byte;

                        2'd3: begin

                            word_buf[31:24] <= rx_byte;

                            /*
                             * The first complete word of a frame is
                             * the header, not operand data.
                             */
                            if (!hdr_done) begin

                                k_dim <= hdr_valid ? FEED_W'(hdr_k)
                                                   : FEED_W'(K_MAX);

                            end

                            /*
                             * Absolute k is {window, col} for A and
                             * {window, row} for B -- each k window is
                             * exactly 8 deep, so the concatenation is
                             * window*8 + offset with no adder.
                             *
                             * Payload is written optimistically,
                             * before the end marker has been seen. A
                             * frame that turns out to be spurious
                             * leaves stale operands behind, but they
                             * are overwritten by the next accepted
                             * frame before anything reads them --
                             * which is why no 1036-byte holding
                             * buffer is needed.
                             */
                            else if (rx_is_b) begin

                                B_buf[{rx_win, rx_row}]
                                     [rx_col]
                                    <= {
                                        rx_byte,
                                        word_buf[23:0]
                                    };

                            end
                            else begin

                                A_buf[rx_row]
                                     [{rx_win, rx_col}]
                                    <= {
                                        rx_byte,
                                        word_buf[23:0]
                                    };

                            end

                        end

                    endcase


                    if (byte_pos == 2'd3)
                        byte_pos <= 2'd0;
                    else
                        byte_pos <= byte_pos + 1'b1;


                    /*
                     * Header bytes do not advance the payload
                     * counter, so rx_count still addresses operand
                     * storage exactly as before: the write at
                     * byte_pos == 3 sees rx_count = 4w+3 for
                     * payload word w.
                     */
                    if (!hdr_done) begin

                        if (byte_pos == 2'd3)
                            hdr_done <= 1'b1;

                    end
                    else if (rx_count == RX_CNT_W'(RX_BYTES - 1)) begin

                        rx_count <= '0;
                        hdr_done <= 1'b0;
                        tail_cnt <= 2'd0;

                        /*
                         * The payload is complete but not yet
                         * trusted. matrices_ready is asserted in
                         * RX_TAIL and only if the end marker
                         * matches.
                         */
                        rx_state <= RX_TAIL;

                    end
                    else begin

                        rx_count <= rx_count + 1'b1;

                    end

                end


                /*
                 * ----------------------------------------------------
                 * Four bytes of end marker. A match is the only thing
                 * that starts a computation; a mismatch means the
                 * start marker was spurious or the frame was mangled,
                 * so the whole frame is dropped and the receiver goes
                 * back to hunting.
                 * ----------------------------------------------------
                 */
                RX_TAIL: begin

                    if (tail_cnt == 2'd3) begin

                        if (sync_next == FRAME_END)
                            matrices_ready <= 1'b1;

                        rx_state <= RX_HUNT;

                    end
                    else begin

                        tail_cnt <= tail_cnt + 1'b1;

                    end

                end


                default: begin

                    rx_state <= RX_HUNT;

                end

                endcase

            end

        end
    end
"""


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2

    path = sys.argv[1]
    if not os.path.isfile(path):
        print(f"ERROR: 找不到 {path}")
        return 1

    src = open(path, encoding="utf-8").read()

    if "FRAME_START" in src:
        print("已經套用過(檔案裡已有 FRAME_START),不做任何事。")
        return 0

    problems = []
    for name, anchor in (("宣告插入點 (logic matrices_ready;)", DECL_ANCHOR),
                         ("RX always_ff 區塊", OLD_BLOCK)):
        n = src.count(anchor)
        if n != 1:
            problems.append(f"  {name}: 出現 {n} 次(需要剛好 1 次)")

    if problems:
        print("ERROR: 錨點對不上,沒有做任何修改。")
        print("\n".join(problems))
        print()
        print("把這段貼給我,我照實際版本重寫:")
        print(f"  sed -n '/logic matrices_ready;/,/^    end$/p' {path}")
        return 1

    src = src.replace(DECL_ANCHOR, DECL_NEW, 1)
    src = src.replace(OLD_BLOCK, NEW_BLOCK, 1)

    bak = path + ".bak_framing"
    if not os.path.exists(bak):
        shutil.copy2(path, bak)

    open(path, "w", encoding="utf-8").write(src)

    print(f"已修改 {path}")
    print(f"備份    {bak}")
    print()
    print("  ok  FRAME_START / FRAME_END 常數")
    print("  ok  rx_state_t {RX_HUNT, RX_BODY, RX_TAIL} + sync_sr 滑動視窗")
    print("  ok  RX always_ff 改為三狀態 framing")
    print("  ok  matrices_ready 改由 END 標記驗證後才觸發")
    print()
    print("host 端必須同步修改 test_uart_kmax.py:")
    print("    FRAME_START = 0xA55AC33C")
    print("    FRAME_END   = 0x5AA53CC3")
    print("    req = struct.pack('<I', FRAME_START) + req \\")
    print("        + struct.pack('<I', FRAME_END)")
    print()
    print("驗收 -- 故意把它弄壞,不按 BTNC:")
    print("    python3 test_uart_kmax.py --kmax 64    # 送錯長度,預期 MISMATCH")
    print("    python3 test_uart_kmax.py --kmax 16    # 應該直接 PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
