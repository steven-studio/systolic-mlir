/*
 * mock_ftdi.cpp -- see mock_ftdi.h. TEST ONLY: linked instead of libftdi1.
 */
#include "mock_ftdi.h"

#include <ftdi.h>

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

struct Submit {
  unsigned job_id, device, m, n, k, a_lo, c_lo;
};

struct Call {
  int requested;
  int accepted;
  long start;   // accepted bytes before this call
};

struct MemWrite {
  uint32_t address, length;
  std::vector<unsigned char> payload;
  bool complete;
};

struct Mock {
  bool open = false;
  long opens = 0;
  std::vector<Call> calls;
  std::vector<unsigned char> stream;
  int max_per_call = 0;
  long fail_at = 0, zero_at = 0;
  int fail_size = 0;
  long fail_size_nth = 0, size_hits = 0;
  long fail_byte = 0;
  int read_max = 0;            // Phase T3: cap bytes per ftdi_read_data (0: no cap)
  long fail_read_at = 0;       // Phase T3: the n-th read call fails (0: never)
  long read_calls = 0, read_empty = 0;
  bool responder = true;
  std::string error;

  // The decoder: the FPGA's command front end, byte by byte.
  int state = 0;             // 0 opcode, 1 WRITE32 body, 2 MEM_WRITE header, 3 MEM_WRITE payload
  unsigned char body[8];
  unsigned body_n = 0;
  uint32_t payload_left = 0;
  uint32_t regs[16] = {};
  std::vector<Submit> submits;
  uint32_t seq = 0;
  long mem_writes = 0, mem_write_bytes = 0, bad_opcodes = 0;
  std::vector<MemWrite> mem;   // Phase T2: every decoded MEM_WRITE
  std::deque<unsigned char> rx;
  unsigned char latency = 2;
};

Mock g;

void push_le32(uint32_t v) {
  for (int i = 0; i < 4; ++i)
    g.rx.push_back(static_cast<unsigned char>(v >> (8 * i)));
}

void submit() {
  Submit s;
  s.job_id = g.regs[0x04 / 4];
  s.device = g.regs[0x08 / 4];
  s.m = g.regs[0x0c / 4];
  s.n = g.regs[0x10 / 4];
  s.k = g.regs[0x14 / 4];
  s.a_lo = g.regs[0x20 / 4];
  s.c_lo = g.regs[0x30 / 4];
  g.submits.push_back(s);
  if (!g.responder)
    return;
  const unsigned P = s.device == 0 ? 8u : s.device <= 3 ? 4u : 0u;
  const unsigned status = P == 0 ? 2u : (s.k < 1 || s.k > 16) ? 1u : 0u;
  const unsigned payload = status ? 0u : P * P * 4u;
  push_le32(0x59534A43u);
  push_le32(s.job_id);
  g.rx.push_back(static_cast<unsigned char>(s.device));
  g.rx.push_back(static_cast<unsigned char>(status));
  g.rx.push_back(static_cast<unsigned char>(payload));
  g.rx.push_back(static_cast<unsigned char>(payload >> 8));
  push_le32(g.seq++);
  for (unsigned i = 0; i < payload; ++i)
    g.rx.push_back(0);
}

void feed(unsigned char b) {
  switch (g.state) {
  case 0:
    if (b == 0x01) { g.state = 1; g.body_n = 0; }
    else if (b == 0x02) { g.state = 2; g.body_n = 0; }
    else ++g.bad_opcodes;
    return;
  case 1:
    g.body[g.body_n++] = b;
    if (g.body_n == 5) {
      const unsigned addr = g.body[0];
      const uint32_t value = static_cast<uint32_t>(g.body[1]) |
                             (static_cast<uint32_t>(g.body[2]) << 8) |
                             (static_cast<uint32_t>(g.body[3]) << 16) |
                             (static_cast<uint32_t>(g.body[4]) << 24);
      if (addr < 0x40 && (addr & 3) == 0)
        g.regs[addr / 4] = value;
      if (addr == 0x00 && (value & 1u))
        submit();
      g.state = 0;
    }
    return;
  case 2:
    g.body[g.body_n++] = b;
    if (g.body_n == 8) {
      g.payload_left = static_cast<uint32_t>(g.body[4]) |
                       (static_cast<uint32_t>(g.body[5]) << 8) |
                       (static_cast<uint32_t>(g.body[6]) << 16) |
                       (static_cast<uint32_t>(g.body[7]) << 24);
      MemWrite mw;
      mw.address = static_cast<uint32_t>(g.body[0]) | (static_cast<uint32_t>(g.body[1]) << 8) |
                   (static_cast<uint32_t>(g.body[2]) << 16) | (static_cast<uint32_t>(g.body[3]) << 24);
      mw.length = g.payload_left;
      mw.complete = g.payload_left == 0;
      g.mem.push_back(std::move(mw));
      ++g.mem_writes;
      g.state = g.payload_left ? 3 : 0;
    }
    return;
  case 3:
    ++g.mem_write_bytes;
    g.mem.back().payload.push_back(b);
    if (--g.payload_left == 0) {
      g.mem.back().complete = true;
      g.state = 0;
    }
    return;
  }
}

} // namespace

extern "C" {

void mock_ftdi_reset(void) {
  const long opens = g.opens;
  g = Mock();
  g.opens = opens;
}
void mock_ftdi_set_max_per_call(int bytes) { g.max_per_call = bytes; }
void mock_ftdi_fail_at_call(long n) { g.fail_at = n; }
void mock_ftdi_zero_at_call(long n) { g.zero_at = n; }
void mock_ftdi_fail_at_size(int size, long nth) {
  g.fail_size = size;
  g.fail_size_nth = nth;
  g.size_hits = 0;
}
void mock_ftdi_fail_at_byte(long n) { g.fail_byte = n; }
void mock_ftdi_set_read_max_per_call(int bytes) { g.read_max = bytes; }
void mock_ftdi_fail_read_at_call(long n) { g.fail_read_at = n; }
long mock_ftdi_read_calls(void) { return g.read_calls; }
long mock_ftdi_read_empty(void) { return g.read_empty; }
long mock_ftdi_call_start(long i) {
  return (i >= 0 && i < static_cast<long>(g.calls.size())) ? g.calls[static_cast<size_t>(i)].start : -1;
}
void mock_ftdi_set_responder(int on) { g.responder = on != 0; }
long mock_ftdi_call_count(void) { return static_cast<long>(g.calls.size()); }
int mock_ftdi_call_request(long i) {
  return (i >= 0 && i < static_cast<long>(g.calls.size())) ? g.calls[static_cast<size_t>(i)].requested : -1;
}
int mock_ftdi_call_accepted(long i) {
  return (i >= 0 && i < static_cast<long>(g.calls.size())) ? g.calls[static_cast<size_t>(i)].accepted : -1;
}
long mock_ftdi_stream_size(void) { return static_cast<long>(g.stream.size()); }
const unsigned char *mock_ftdi_stream(void) { return g.stream.data(); }
long mock_ftdi_submit_count(void) { return static_cast<long>(g.submits.size()); }
int mock_ftdi_submit(long i, unsigned *job_id, unsigned *device, unsigned *m,
                     unsigned *n, unsigned *k, unsigned *a_lo, unsigned *c_lo) {
  if (i < 0 || i >= static_cast<long>(g.submits.size()))
    return -1;
  const Submit &s = g.submits[static_cast<size_t>(i)];
  if (job_id) *job_id = s.job_id;
  if (device) *device = s.device;
  if (m) *m = s.m;
  if (n) *n = s.n;
  if (k) *k = s.k;
  if (a_lo) *a_lo = s.a_lo;
  if (c_lo) *c_lo = s.c_lo;
  return 0;
}
long mock_ftdi_mem_writes(void) { return g.mem_writes; }
long mock_ftdi_mem_writes_complete(void) {
  long n = 0;
  for (const MemWrite &m : g.mem)
    n += m.complete ? 1 : 0;
  return n;
}
int mock_ftdi_mem_write(long i, unsigned *address, unsigned *length, int *complete,
                        const unsigned char **payload, long *received) {
  if (i < 0 || i >= static_cast<long>(g.mem.size()))
    return -1;
  const MemWrite &m = g.mem[static_cast<size_t>(i)];
  if (address) *address = m.address;
  if (length) *length = m.length;
  if (complete) *complete = m.complete ? 1 : 0;
  if (payload) *payload = m.payload.data();
  if (received) *received = static_cast<long>(m.payload.size());
  return 0;
}
long mock_ftdi_mem_write_bytes(void) { return g.mem_write_bytes; }
long mock_ftdi_bad_opcodes(void) { return g.bad_opcodes; }
long mock_ftdi_open_count(void) { return g.opens; }

/* ---- the libftdi1 calls runtime/dpti_ftdi.cpp makes --------------------- */

struct ftdi_context *ftdi_new(void) {
  struct ftdi_context *ftdi = static_cast<struct ftdi_context *>(std::calloc(1, sizeof *ftdi));
  if (ftdi)
    ftdi->error_str = "";
  return ftdi;
}
void ftdi_free(struct ftdi_context *ftdi) { std::free(ftdi); }
int ftdi_set_interface(struct ftdi_context *, enum ftdi_interface) { return 0; }
int ftdi_usb_open(struct ftdi_context *, int, int) {
  g.open = true;
  ++g.opens;
  return 0;
}
int ftdi_usb_close(struct ftdi_context *) {
  g.open = false;
  return 0;
}
int ftdi_get_latency_timer(struct ftdi_context *, unsigned char *latency) {
  if (latency)
    *latency = g.latency;
  return 0;
}
int ftdi_set_latency_timer(struct ftdi_context *, unsigned char latency) {
  g.latency = latency;
  return 0;
}
int ftdi_read_data_set_chunksize(struct ftdi_context *, unsigned int) { return 0; }
int ftdi_set_bitmode(struct ftdi_context *, unsigned char, unsigned char) { return 0; }
int ftdi_usb_purge_buffers(struct ftdi_context *) {
  g.rx.clear();
  return 0;
}
int ftdi_write_data(struct ftdi_context *ftdi, const unsigned char *buf, int size) {
  if (!g.open) {
    g.error = "mock: not open";
    if (ftdi) ftdi->error_str = g.error.c_str();
    return -666;
  }
  const long call = static_cast<long>(g.calls.size()) + 1;
  const long start = static_cast<long>(g.stream.size());
  if (call == g.fail_at) {
    g.calls.push_back({size, 0, start});
    g.error = "mock: injected write failure";
    if (ftdi) ftdi->error_str = g.error.c_str();
    return -1;
  }
  if (g.fail_size > 0 && size == g.fail_size && ++g.size_hits == g.fail_size_nth) {
    g.calls.push_back({size, 0, start});
    g.error = "mock: injected write failure (by size)";
    if (ftdi) ftdi->error_str = g.error.c_str();
    return -1;
  }
  if (g.fail_byte > 0 && start < g.fail_byte && start + size >= g.fail_byte) {
    g.calls.push_back({size, 0, start});
    g.error = "mock: injected write failure (by byte position)";
    if (ftdi) ftdi->error_str = g.error.c_str();
    return -1;
  }
  if (call == g.zero_at) {
    g.calls.push_back({size, 0, start});
    return 0;
  }
  int n = size;
  if (g.max_per_call > 0 && n > g.max_per_call)
    n = g.max_per_call;
  for (int i = 0; i < n; ++i) {
    g.stream.push_back(buf[i]);
    feed(buf[i]);
  }
  g.calls.push_back({size, n, start});
  return n;
}
int ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size) {
  if (!g.open)
    return -666;
  ++g.read_calls;
  if (g.fail_read_at > 0 && g.read_calls == g.fail_read_at) {
    g.error = "mock: injected read failure";
    if (ftdi) ftdi->error_str = g.error.c_str();
    return -1;
  }
  int cap = size;
  if (g.read_max > 0 && cap > g.read_max)
    cap = g.read_max;
  int n = 0;
  while (n < cap && !g.rx.empty()) {
    buf[n++] = g.rx.front();
    g.rx.pop_front();
  }
  if (n == 0)
    ++g.read_empty;
  return n;
}
const char *ftdi_get_error_string(struct ftdi_context *ftdi) {
  return (ftdi && ftdi->error_str) ? ftdi->error_str : "";
}

} // extern "C"
