#ifndef FPGA_SIGNAL_H
#define FPGA_SIGNAL_H

// Cooperative Ctrl+C handling for the UART drivers.
//
// A process has exactly one SIGINT disposition, and the flag it sets has to
// be one object, so this lives in one translation unit rather than being
// duplicated per driver.
//
// The point is NOT to stop as fast as possible. The UART protocol is
// fixed-length and unframed -- 48 bytes out, 16 back, no header, no length
// field, no checksum. Dying midway through a transaction leaves the board
// still transmitting into the kernel's tty buffer, and the next process to
// open the port reads that leftover as the head of its own first response.
// Every transaction after that is shifted by the same offset and the results
// are silently wrong, which looks exactly like a hardware fault.
//
// So: finish the packet in flight, then stop at the next tile boundary.
//
// Usage:
//   main()                     -> fpga_install_signal_handler()
//   per-tile loop              -> if (fpga_stop_requested()) return -5;
//   read_full/write_full       -> already retry on EINTR; that retry is what
//                                 finishes the in-flight packet, and it only
//                                 does anything once a handler is installed
//                                 (with no handler, SIGINT just kills the
//                                 process and the loop never sees EINTR).

#ifdef __cplusplus
extern "C" {
#endif

// Installs handlers for SIGINT and SIGTERM. Safe to call more than once.
// Deliberately does not set SA_RESTART: blocking reads and writes must come
// back with EINTR so their retry loops can complete the current packet.
void fpga_install_signal_handler(void);

// Non-zero once a stop has been requested. Poll this between transactions.
int fpga_stop_requested(void);

// Clears the flag. For a long-running host process that wants to keep going
// after an aborted run; the one-shot drivers do not need it.
void fpga_clear_stop_request(void);

#ifdef __cplusplus
}
#endif

#endif  // FPGA_SIGNAL_H
