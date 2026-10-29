/*
 * mock_ftdi.h -- a libftdi1 stand-in for host-only tests of the DPTI
 * transport (Phase T1, descriptor coalescing). mock_ftdi.cpp defines the
 * ftdi_* functions runtime/dpti_ftdi.cpp calls, records every write call
 * and the byte stream, decodes the stream like the FPGA's command front end
 * (WRITE32 register writes, MEM_WRITE slabs) and, when the responder is on,
 * answers each CONTROL = submit with a completion record and a zero result
 * tile, so both runtimes can run end to end without a board.
 *
 * Injection: a maximum of bytes accepted per write call (partial writes),
 * a call that fails (-1, nothing accepted) and a call that returns 0.
 */
#ifndef SYSTOLIC_MOCK_FTDI_H
#define SYSTOLIC_MOCK_FTDI_H

#ifdef __cplusplus
extern "C" {
#endif

void mock_ftdi_reset(void);                  /* counters, stream, decoder, injection */
void mock_ftdi_set_max_per_call(int bytes);  /* 0: accept every call whole */
void mock_ftdi_fail_at_call(long n);         /* the n-th write call (from 1) fails; 0: never */
void mock_ftdi_zero_at_call(long n);         /* the n-th write call returns 0; 0: never */
void mock_ftdi_fail_at_size(int size, long nth); /* the nth write call asking for exactly `size` bytes fails; 0: never */
void mock_ftdi_set_responder(int on);        /* answer submits with records (default on) */

long mock_ftdi_call_count(void);             /* write calls so far, failed ones included */
int mock_ftdi_call_request(long i);          /* bytes call i (from 0) asked to write */
int mock_ftdi_call_accepted(long i);         /* bytes call i accepted */
long mock_ftdi_stream_size(void);            /* bytes accepted, in order */
const unsigned char *mock_ftdi_stream(void);

long mock_ftdi_submit_count(void);           /* CONTROL = submit seen, in order */
int mock_ftdi_submit(long i, unsigned *job_id, unsigned *device, unsigned *m,
                     unsigned *n, unsigned *k, unsigned *a_lo, unsigned *c_lo);
long mock_ftdi_mem_writes(void);
long mock_ftdi_mem_write_bytes(void);
long mock_ftdi_bad_opcodes(void);
long mock_ftdi_open_count(void);

#ifdef __cplusplus
}
#endif

#endif
