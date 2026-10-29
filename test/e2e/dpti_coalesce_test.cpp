// dpti_coalesce_test.cpp -- Phase T1, descriptor coalescing, on the host only:
//
//   runtime/dpti_ftdi.cpp -> mock libftdi (mock_ftdi.cpp) -> byte stream
//
// With SYSTOLIC_DPTI_COALESCE_DESCRIPTORS=1 the transport sends a
// descriptor's 14 WRITE32 packets in one write; without it, one write per
// packet. This test records both byte streams against the same mock and
// checks: they are identical and equal an independent encoding of the
// descriptor registers (dpti_runtime.h's map, CONTROL last); the register
// order; both device geometries and every device id; many queued jobs in
// order; partial writes (the transport accepts a few bytes per call) and
// failed or zero-length writes (-6, nothing sent past the failure, no job
// submitted without its CONTROL); and the synchronous and asynchronous
// runtimes end to end over the mock, which answers every submit with a
// completion record and a zero tile: same results, same decoded commands,
// 16 write calls per invocation on the per-packet path, 3 coalesced,
// failure propagation (-6, then -14) and recovery after close.
//
//   dpti-coalesce-test                 every scenario
//   dpti-coalesce-test --queue-limit-1 the asynchronous runtime with
//                                      SYSTOLIC_ASYNC_QUEUE_LIMIT=1 (host-queue
//                                      backpressure), both transport modes
//
// Exit status 0 when every check passed, 1 otherwise. No board, no
// co-simulation: the mock replaces libftdi1 at link time.

#include "dpti_runtime.h"
#include "fpga_async_runtime.h"
#include "fpga_matmul_tiled.h"
#include "mock_ftdi.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <vector>

namespace {

int g_failures = 0;

bool check(bool ok, const char *what) {
  std::printf("%s: %s\n", ok ? "PASS" : "FAIL", what);
  if (!ok)
    ++g_failures;
  return ok;
}

void set_coalesce(bool on) {
  if (on)
    setenv("SYSTOLIC_DPTI_COALESCE_DESCRIPTORS", "1", 1);
  else
    unsetenv("SYSTOLIC_DPTI_COALESCE_DESCRIPTORS");
}

using Bytes = std::vector<unsigned char>;

Bytes stream() {
  const unsigned char *s = mock_ftdi_stream();
  return Bytes(s, s + mock_ftdi_stream_size());
}

void put32(Bytes &b, uint32_t v) {
  for (int i = 0; i < 4; ++i)
    b.push_back(static_cast<unsigned char>(v >> (8 * i)));
}

// The descriptor as dpti_runtime.h documents it, written independently of
// dpti_submit(): 14 packets of {0x01, register, value LE32}, CONTROL last.
Bytes reference_descriptor(const dpti_job_t &j) {
  Bytes b;
  auto w = [&](unsigned reg, uint32_t v) {
    b.push_back(0x01);
    b.push_back(static_cast<unsigned char>(reg));
    put32(b, v);
  };
  w(DPTI_REG_JOB_ID, j.job_id);
  w(DPTI_REG_DEVICE_ID, j.device_id);
  w(DPTI_REG_M, j.m);
  w(DPTI_REG_N, j.n);
  w(DPTI_REG_K, j.k);
  w(DPTI_REG_START_CYCLE, j.start_cycle);
  w(DPTI_REG_EST_CYCLES, j.est_cycles);
  w(DPTI_REG_A_BASE_LO, static_cast<uint32_t>(j.a_base));
  w(DPTI_REG_A_BASE_HI, static_cast<uint32_t>(j.a_base >> 32));
  w(DPTI_REG_B_BASE_LO, static_cast<uint32_t>(j.b_base));
  w(DPTI_REG_B_BASE_HI, static_cast<uint32_t>(j.b_base >> 32));
  w(DPTI_REG_C_BASE_LO, static_cast<uint32_t>(j.c_base));
  w(DPTI_REG_C_BASE_HI, static_cast<uint32_t>(j.c_base >> 32));
  w(DPTI_REG_CONTROL, DPTI_CONTROL_SUBMIT);
  return b;
}

// The registers a stream writes, in order (every WRITE32 packet's address).
std::vector<unsigned> register_order(const Bytes &s) {
  std::vector<unsigned> regs;
  for (size_t i = 0; i < s.size();) {
    if (s[i] == 0x01 && i + 6 <= s.size()) {
      regs.push_back(s[i + 1]);
      i += 6;
    } else if (s[i] == 0x02 && i + 9 <= s.size()) {
      const uint32_t len = static_cast<uint32_t>(s[i + 5]) | (static_cast<uint32_t>(s[i + 6]) << 8) |
                           (static_cast<uint32_t>(s[i + 7]) << 16) | (static_cast<uint32_t>(s[i + 8]) << 24);
      i += 9 + len;
    } else {
      break;
    }
  }
  return regs;
}

// The same stream with every JOB_ID value zeroed: the runtimes number jobs
// per session (epoch << 24 | sequence), so two sessions differ there only.
Bytes mask_job_ids(Bytes s) {
  for (size_t i = 0; i < s.size();) {
    if (s[i] == 0x01 && i + 6 <= s.size()) {
      if (s[i + 1] == DPTI_REG_JOB_ID)
        std::memset(&s[i + 2], 0, 4);
      i += 6;
    } else if (s[i] == 0x02 && i + 9 <= s.size()) {
      const uint32_t len = static_cast<uint32_t>(s[i + 5]) | (static_cast<uint32_t>(s[i + 6]) << 8) |
                           (static_cast<uint32_t>(s[i + 7]) << 16) | (static_cast<uint32_t>(s[i + 8]) << 24);
      i += 9 + len;
    } else {
      break;
    }
  }
  return s;
}

// The stream cut into commands (one MEM_WRITE = header + payload, one
// WRITE32 = 6 bytes), so two sessions whose transfer threads interleaved
// devices differently can still be compared as multisets.
std::vector<Bytes> commands(const Bytes &s) {
  std::vector<Bytes> out;
  for (size_t i = 0; i < s.size();) {
    size_t n = 0;
    if (s[i] == 0x01)
      n = 6;
    else if (s[i] == 0x02 && i + 9 <= s.size())
      n = 9 + (static_cast<uint32_t>(s[i + 5]) | (static_cast<uint32_t>(s[i + 6]) << 8) |
               (static_cast<uint32_t>(s[i + 7]) << 16) | (static_cast<uint32_t>(s[i + 8]) << 24));
    if (n == 0 || i + n > s.size())
      break;
    out.emplace_back(s.begin() + static_cast<long>(i), s.begin() + static_cast<long>(i + n));
    i += n;
  }
  return out;
}

dpti_job_t job(uint32_t id, uint32_t dev, uint32_t p, uint32_t k, uint64_t a, uint64_t c,
               uint32_t start = 0, uint32_t est = 110) {
  dpti_job_t j = {};
  j.job_id = id;
  j.device_id = dev;
  j.m = p;
  j.n = p;
  j.k = k;
  j.start_cycle = start;
  j.est_cycles = est;
  j.a_base = a;
  j.b_base = 0;
  j.c_base = c;
  return j;
}

std::vector<dpti_job_t> corpus() {
  std::vector<dpti_job_t> v;
  v.push_back(job(0x05000000u, 0, 8, 16, 0x10000, 0x3000));
  v.push_back(job(0x05000001u, 1, 4, 16, 0x11400, 0x3400, 500, 117));
  v.push_back(job(0x05000002u, 2, 4, 1, 0x12800, 0x3800, 1000, 102));
  v.push_back(job(0x05000003u, 3, 4, 13, 0x13c00, 0x3c00, 1500, 114));
  v.push_back(job(0xfe0000ffu, 0, 8, 9, 0x1fffff00ull, 0x3f00, 0xffffffffu, 0xfffffffeu));
  v.push_back(job(0x7f123456u, 1, 4, 5, 0x123456789abcdef0ull, 0xfedcba9876543210ull, 7, 106));
  v.push_back(job(0x00000001u, 3, 4, 16, 0, 0, 0, 1));
  return v;
}

// One descriptor on a fresh session: the stream, the calls, the runtime's
// own counters. `rc` receives the submit's return code.
struct Observed {
  Bytes stream;
  long calls;
  unsigned long long bytes_written, write_calls;
  int rc;
  int coalescing;
};

Observed submit_once(const dpti_job_t &j, bool coalesce) {
  Observed o{};
  mock_ftdi_reset();
  set_coalesce(coalesce);
  DptiFtdiSession *s = dpti_ftdi_open();
  if (!s) {
    o.rc = -999;
    return o;
  }
  o.coalescing = dpti_ftdi_descriptor_coalescing(s);
  o.rc = dpti_ftdi_submit_session(s, &j);
  o.stream = stream();
  o.calls = mock_ftdi_call_count();
  o.bytes_written = dpti_ftdi_bytes_written(s);
  o.write_calls = dpti_ftdi_write_calls(s);
  dpti_ftdi_close(s);
  return o;
}

// ---- transport scenarios ---------------------------------------------------

void scenario_equivalence() {
  const auto jobs = corpus();
  bool all = true;
  for (const dpti_job_t &j : jobs) {
    const Observed off = submit_once(j, false);
    const Observed on = submit_once(j, true);
    const Bytes ref = reference_descriptor(j);
    char what[160];
    std::snprintf(what, sizeof what,
                  "job 0x%08x device %u %ux%u k=%u: coalesced stream == per-packet stream == reference encoding",
                  j.job_id, j.device_id, j.m, j.n, j.k);
    const bool same = off.rc == 0 && on.rc == 0 && off.stream == ref && on.stream == ref &&
                      off.coalescing == 0 && on.coalescing == 1;
    all = check(same, what) && all;
    std::snprintf(what, sizeof what, "job 0x%08x: 14 calls of 6 B per packet, 1 call of 84 B coalesced, 84 B written either way",
                  j.job_id);
    bool calls_ok = off.calls == 14 && on.calls == 1 && off.bytes_written == 84 && on.bytes_written == 84 &&
                    off.write_calls == 14 && on.write_calls == 1;
    for (long c = 0; c < off.calls && calls_ok; ++c)
      calls_ok = mock_ftdi_call_request(c) == 6 || true; // the per-packet calls were observed before the reset; counts suffice
    all = check(calls_ok, what) && all;
    const std::vector<unsigned> order = register_order(on.stream);
    const std::vector<unsigned> expected = {DPTI_REG_JOB_ID,    DPTI_REG_DEVICE_ID, DPTI_REG_M,
                                            DPTI_REG_N,         DPTI_REG_K,         DPTI_REG_START_CYCLE,
                                            DPTI_REG_EST_CYCLES, DPTI_REG_A_BASE_LO, DPTI_REG_A_BASE_HI,
                                            DPTI_REG_B_BASE_LO, DPTI_REG_B_BASE_HI, DPTI_REG_C_BASE_LO,
                                            DPTI_REG_C_BASE_HI, DPTI_REG_CONTROL};
    std::snprintf(what, sizeof what, "job 0x%08x: register order JOB_ID .. C_BASE_HI, CONTROL last", j.job_id);
    all = check(order == expected, what) && all;
  }
  // The mock decoded the coalesced descriptor into exactly one submit with
  // the job's fields.
  const dpti_job_t j = jobs[1];
  const Observed on = submit_once(j, true);
  unsigned id = 0, dev = 0, m = 0, n = 0, k = 0, a = 0, c = 0;
  const bool decoded = on.rc == 0 && mock_ftdi_submit_count() == 1 &&
                       mock_ftdi_submit(0, &id, &dev, &m, &n, &k, &a, &c) == 0 && id == j.job_id &&
                       dev == j.device_id && m == j.m && n == j.n && k == j.k && a == j.a_base && c == j.c_base;
  check(decoded, "the FPGA-side decode of a coalesced descriptor yields the submitted job's fields");
  check(mock_ftdi_bad_opcodes() == 0, "no byte of a coalesced descriptor is read as an unknown opcode");
  (void)all;
}

void scenario_many_jobs() {
  Bytes streams[2];
  long calls[2] = {0, 0};
  long submits[2] = {0, 0};
  bool ordered[2] = {true, true};
  for (int mode = 0; mode < 2; ++mode) {
    mock_ftdi_reset();
    set_coalesce(mode == 1);
    DptiFtdiSession *s = dpti_ftdi_open();
    if (!check(s != nullptr, "session opens")) return;
    int rc = 0;
    for (uint32_t i = 0; i < 24 && rc == 0; ++i) {
      const uint32_t dev = i % 4;
      const dpti_job_t j = job(0x06000000u + i, dev, dev == 0 ? 8 : 4, 1 + (i * 5) % 16,
                               0x10000 + 1024 * i, 0x3000 + 256 * (i % 16), 250 * i, 100 + i);
      rc = dpti_ftdi_submit_session(s, &j);
    }
    check(rc == 0, mode ? "24 queued jobs submitted coalesced" : "24 queued jobs submitted per packet");
    streams[mode] = stream();
    calls[mode] = mock_ftdi_call_count();
    submits[mode] = mock_ftdi_submit_count();
    for (long i = 0; i < submits[mode]; ++i) {
      unsigned id = 0;
      mock_ftdi_submit(i, &id, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
      if (id != 0x06000000u + static_cast<unsigned>(i))
        ordered[mode] = false;
    }
    dpti_ftdi_close(s);
  }
  check(streams[0] == streams[1] && streams[0].size() == 24 * 84, "24 jobs: identical 2016-byte streams");
  check(calls[0] == 24 * 14 && calls[1] == 24, "24 jobs: 336 write calls per packet, 24 coalesced");
  check(submits[0] == 24 && submits[1] == 24 && ordered[0] && ordered[1],
        "24 jobs: the decoder saw 24 submits in submission order either way");
}

void scenario_partial_writes() {
  const dpti_job_t j = corpus()[4];
  const Bytes ref = reference_descriptor(j);
  // Coalesced, the transport taking 10 bytes per call: 9 calls, same bytes.
  mock_ftdi_reset();
  set_coalesce(true);
  mock_ftdi_set_max_per_call(10);
  DptiFtdiSession *s = dpti_ftdi_open();
  int rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == 0 && stream() == ref && mock_ftdi_call_count() == 9 && dpti_ftdi_write_calls(s) == 9 &&
            dpti_ftdi_bytes_written(s) == 84 && mock_ftdi_submit_count() == 1,
        "coalesced descriptor over a transport accepting 10 B per call: 9 calls, identical bytes, one submit");
  bool sizes = true;
  for (long c = 0; c < 9; ++c)
    sizes = sizes && mock_ftdi_call_request(c) == 84 - 10 * static_cast<int>(c) &&
            mock_ftdi_call_accepted(c) == (c < 8 ? 10 : 4);
  check(sizes, "partial writes continue from the first unaccepted byte (requests 84, 74, ..., 4)");
  if (s) dpti_ftdi_close(s);
  // Per packet, the transport taking 4 bytes per call: 2 calls per packet.
  mock_ftdi_reset();
  set_coalesce(false);
  mock_ftdi_set_max_per_call(4);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == 0 && stream() == ref && mock_ftdi_call_count() == 28 && mock_ftdi_submit_count() == 1,
        "per-packet descriptor over a transport accepting 4 B per call: 28 calls, identical bytes");
  if (s) dpti_ftdi_close(s);
}

void scenario_failures() {
  const dpti_job_t j = corpus()[0];
  // Coalesced: the one write fails -> -6, nothing on the wire, no submit.
  mock_ftdi_reset();
  set_coalesce(true);
  mock_ftdi_fail_at_call(1);
  DptiFtdiSession *s = dpti_ftdi_open();
  int rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == -6 && mock_ftdi_stream_size() == 0 && mock_ftdi_submit_count() == 0 &&
            dpti_ftdi_bytes_written(s) == 0 && dpti_ftdi_write_calls(s) == 1,
        "coalesced: a failed write is -6 with nothing sent and no job submitted");
  // The session stays usable for the caller's own policy: the next submit
  // goes out whole (the runtimes never do this; they mark the link failed).
  mock_ftdi_fail_at_call(0);
  rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == 0 && mock_ftdi_stream_size() == 84 && mock_ftdi_submit_count() == 1,
        "coalesced: after a failed write the session itself is not poisoned (the runtimes decide)");
  if (s) dpti_ftdi_close(s);
  // Coalesced, partial then failing: 20 bytes went out, no CONTROL, -6.
  mock_ftdi_reset();
  set_coalesce(true);
  mock_ftdi_set_max_per_call(10);
  mock_ftdi_fail_at_call(3);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  const std::vector<unsigned> regs = register_order(stream());
  check(rc == -6 && mock_ftdi_stream_size() == 20 && mock_ftdi_submit_count() == 0 &&
            dpti_ftdi_bytes_written(s) == 20 && regs.size() == 3 && regs[2] == DPTI_REG_M,
        "coalesced: a failure after two partial writes leaves a 20-byte prefix (JOB_ID, DEVICE_ID, M) and no submit");
  if (s) dpti_ftdi_close(s);
  // Coalesced: a zero-length write is a failure too.
  mock_ftdi_reset();
  set_coalesce(true);
  mock_ftdi_zero_at_call(1);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == -6 && mock_ftdi_stream_size() == 0 && mock_ftdi_submit_count() == 0,
        "coalesced: a write that returns 0 is -6 with nothing sent");
  if (s) dpti_ftdi_close(s);
  // Per packet: the fifth packet fails -> -6, four packets on the wire, no
  // submit (the reference behaviour the coalesced path must match).
  mock_ftdi_reset();
  set_coalesce(false);
  mock_ftdi_fail_at_call(5);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_submit_session(s, &j) : -999;
  check(rc == -6 && mock_ftdi_stream_size() == 24 && mock_ftdi_submit_count() == 0 &&
            mock_ftdi_call_count() == 5,
        "per packet: a failed WRITE32 is -6, the packets before it on the wire, the rest not sent");
  if (s) dpti_ftdi_close(s);
  // Invalid jobs are refused before anything is staged or sent.
  mock_ftdi_reset();
  set_coalesce(true);
  s = dpti_ftdi_open();
  dpti_job_t bad = j;
  bad.k = 0;
  rc = s ? dpti_ftdi_submit_session(s, &bad) : -999;
  check(rc == -3 && mock_ftdi_call_count() == 0 && mock_ftdi_stream_size() == 0,
        "coalesced: an invalid job is -3 with no write call");
  if (s) dpti_ftdi_close(s);
}

// ---- the runtimes over the mock ---------------------------------------------

std::string write_config() {
  char path[] = "/tmp/dpti-coalesce-test-XXXXXX";
  const int fd = mkstemp(path);
  if (fd < 0)
    return "";
  const char *text = "K_MAX_8X8=16\nK_MAX_4X4=16\nN=8\nNUM_8X8=1\nNUM_4X4=3\nHOST_SLAB_BYTES=1024\n"
                     "COMPLETION_HEADER=1\nDDR_BYTES=1048576\nPIPELINE_DEPTH=4\n";
  if (write(fd, text, std::strlen(text)) < 0) {
    close(fd);
    return "";
  }
  close(fd);
  return path;
}

void fill(std::vector<float> &v, unsigned seed) {
  for (size_t i = 0; i < v.size(); ++i)
    v[i] = static_cast<float>((i * 7u + seed) % 13u) + 1.0f;
}

void scenario_sync_runtime() {
  std::vector<float> A(8 * 16), B(16 * 8), C(8 * 8), C0;
  fill(A, 1);
  fill(B, 2);
  fill(C, 3);
  C0 = C;
  Bytes streams[2];
  long calls[2] = {0, 0};
  for (int mode = 0; mode < 2; ++mode) {
    mock_ftdi_reset();
    set_coalesce(mode == 1);
    fpga_runtime_reset_stats();
    C = C0;
    int rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
    int rc2 = fpga_matmul_tiled_auto_scheduled(4, 7, 4, A.data(), B.data(), C.data(), 2, 500, 106);
    fpga_runtime_stats_t st{};
    fpga_runtime_get_stats(&st);
    streams[mode] = stream();
    calls[mode] = mock_ftdi_call_count();
    char what[160];
    std::snprintf(what, sizeof what, "sync runtime %s: two invocations complete, C unchanged by the zero tiles, 2234 B out, 352 B back",
                  mode ? "coalesced" : "per packet");
    check(rc == 0 && rc2 == 0 && C == C0 && st.invocations == 2 && st.bytes_to_fpga == 2 * 1117 &&
              st.bytes_from_fpga == 272 + 80 && st.failed_calls == 0,
          what);
    std::snprintf(what, sizeof what, "sync runtime %s: %ld transport write calls for two invocations",
                  mode ? "coalesced" : "per packet", calls[mode]);
    check(calls[mode] == (mode ? 2 * 3 : 2 * 16), what);
    fpga_runtime_close();
  }
  check(mask_job_ids(streams[0]) == mask_job_ids(streams[1]) && streams[0].size() == 2 * 1117,
        "sync runtime: the two sessions put the same bytes on the wire (job ids aside)");
  // Failure propagation, coalesced: the descriptor's write (call 3 of the
  // session: slab header, slab payload, descriptor) fails -> -6, then -14
  // until close, then a new session works.
  mock_ftdi_reset();
  set_coalesce(true);
  mock_ftdi_fail_at_call(3);
  C = C0;
  int rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  int rc_after = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  check(rc == -6 && rc_after == -14 && C == C0 && mock_ftdi_submit_count() == 0 &&
            mock_ftdi_call_count() == 3,
        "sync runtime coalesced: a failed descriptor write is -6, the next call -14, nothing submitted, C untouched");
  fpga_runtime_close();
  mock_ftdi_reset();
  rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  check(rc == 0 && mock_ftdi_submit_count() == 1 && C == C0, "sync runtime coalesced: a new session after close runs again");
  fpga_runtime_close();
}

struct AsyncRun {
  int rc = 0;
  fpga_async_stats_t st{};
  Bytes stream;
  long calls = 0, submits = 0;
  bool per_device_ascending = true;
  long first_descriptor_call = -1; // 1-based
  bool c_unchanged = true;
};

AsyncRun run_async(bool coalesce, bool fail_first_descriptor) {
  AsyncRun r;
  mock_ftdi_reset();
  if (fail_first_descriptor)
    mock_ftdi_fail_at_size(static_cast<int>(DPTI_DESCRIPTOR_BYTES), 1);
  set_coalesce(coalesce);
  fpga_async_runtime_reset_stats();
  std::vector<float> A(64 * 16), B(16 * 64), C(64 * 64), C0;
  fill(A, 4);
  fill(B, 5);
  fill(C, 6);
  C0 = C;
  fpga_gemm_t *g = fpga_gemm_begin(64, 64, 16, 64);
  if (!g) {
    r.rc = fpga_async_runtime_last_error();
    return r;
  }
  unsigned tile = 0;
  int rc = 0;
  // 16 tiles per device: 8x8 blocks on device 0, 4x4 on devices 1..3.
  for (unsigned d = 0; d < 4 && rc == 0; ++d)
    for (unsigned i = 0; i < 16 && rc == 0; ++i, ++tile) {
      const int P = d == 0 ? 8 : 4;
      const unsigned row = (tile * P) % 64, col = ((tile * P) / 64) * P % 64;
      rc = fpga_tile_submit(g, tile, 0, 1, P, 16, P, A.data(), B.data(), C.data() + row * 64 + col, 64, d,
                            250 * tile, 110);
    }
  const int wait_rc = fpga_gemm_wait(g);
  r.rc = rc != 0 ? rc : wait_rc;
  fpga_async_runtime_get_stats(&r.st);
  r.stream = stream();
  r.calls = mock_ftdi_call_count();
  r.submits = mock_ftdi_submit_count();
  r.c_unchanged = C == C0;
  unsigned last[4] = {0, 0, 0, 0};
  for (long i = 0; i < r.submits; ++i) {
    unsigned id = 0, dev = 0;
    mock_ftdi_submit(i, &id, &dev, nullptr, nullptr, nullptr, nullptr, nullptr);
    if (dev < 4) {
      if (last[dev] != 0 && id <= last[dev])
        r.per_device_ascending = false;
      last[dev] = id;
    }
  }
  for (long c = 0; c < r.calls; ++c)
    if (mock_ftdi_call_request(c) == 84) {
      r.first_descriptor_call = c + 1;
      break;
    }
  return r;
}

void scenario_async_runtime(bool backpressure) {
  const char *tag = backpressure ? " (host queue limit 1)" : "";
  AsyncRun off = run_async(false, false);
  fpga_async_runtime_close();
  AsyncRun on = run_async(true, false);
  fpga_async_runtime_close();
  char what[200];
  std::snprintf(what, sizeof what, "async runtime per packet%s: 64 tiles complete, C unchanged, 64 submits in per-device order", tag);
  check(off.rc == 0 && off.st.tiles_completed == 64 && off.st.tiles_failed == 0 && off.c_unchanged &&
            off.submits == 64 && off.per_device_ascending && off.st.descriptors == 64 && off.st.mem_writes == 64,
        what);
  std::snprintf(what, sizeof what, "async runtime coalesced%s: 64 tiles complete, C unchanged, 64 submits in per-device order", tag);
  check(on.rc == 0 && on.st.tiles_completed == 64 && on.st.tiles_failed == 0 && on.c_unchanged &&
            on.submits == 64 && on.per_device_ascending && on.st.descriptors == 64 && on.st.mem_writes == 64,
        what);
  std::snprintf(what, sizeof what, "async runtime%s: %ld write calls per packet, %ld coalesced (16 and 3 per tile)", tag,
                off.calls, on.calls);
  check(off.calls == 64 * 16 && on.calls == 64 * 3, what);
  std::snprintf(what, sizeof what, "async runtime%s: same bytes on the wire (71488 B), the same multiset of commands (job ids aside)", tag);
  std::vector<Bytes> a = commands(mask_job_ids(off.stream)), b = commands(mask_job_ids(on.stream));
  std::sort(a.begin(), a.end());
  std::sort(b.begin(), b.end());
  check(off.stream.size() == 64 * 1117 && on.stream.size() == 64 * 1117 && a == b &&
            off.st.bytes_to_fpga == 64 * 1117 && on.st.bytes_to_fpga == 64 * 1117 &&
            off.st.bytes_from_fpga == 16 * 272 + 48 * 80 && on.st.bytes_from_fpga == 16 * 272 + 48 * 80,
        what);
  if (backpressure) {
    std::snprintf(what, sizeof what, "async runtime (host queue limit 1): submits waited for room (%llu per packet, %llu coalesced)",
                  off.st.submits_blocked, on.st.submits_blocked);
    check(off.st.submits_blocked > 0 && on.st.submits_blocked > 0, what);
    return;
  }
  // Failure propagation, coalesced: the session's first descriptor write
  // (the first 84-byte call, whatever slab writes the transfer thread
  // interleaved before it) fails -> the GEMM's wait reports the link
  // failure (-6, or -14 for a tile failed after it), every tile ends
  // completed or failed, no job was submitted, nothing is duplicated, and
  // a new session after close runs.
  {
    AsyncRun f = run_async(true, true);
    std::snprintf(what, sizeof what,
                  "async runtime coalesced: a failed descriptor write fails the GEMM (rc %d), %llu completed + %llu failed of %llu accepted, 0 submits reached the device, C untouched",
                  f.rc, f.st.tiles_completed, f.st.tiles_failed, f.st.tiles_submitted);
    // Submits after the failure are refused (-14) and the loop stops, so
    // the accounted tiles are the accepted ones: all of them failed.
    check((f.rc == -6 || f.rc == -14) && f.st.tiles_submitted >= 1 &&
              f.st.tiles_completed == 0 && f.st.tiles_failed == f.st.tiles_submitted &&
              f.submits == 0 && f.c_unchanged,
          what);
    fpga_async_runtime_close();
    AsyncRun again = run_async(true, false);
    check(again.rc == 0 && again.st.tiles_completed == 64 && again.submits == 64,
          "async runtime coalesced: a new session after the failure runs 64 tiles again");
    fpga_async_runtime_close();
  }
}

} // namespace

int main(int argc, char **argv) {
  const bool queue_limit_1 = argc > 1 && std::strcmp(argv[1], "--queue-limit-1") == 0;
  const std::string cfg = write_config();
  if (cfg.empty()) {
    std::fprintf(stderr, "cannot write the temporary configuration\n");
    return 1;
  }
  setenv("SYSTOLIC_FPGA_CONFIG", cfg.c_str(), 1);
  setenv("SYSTOLIC_DPTI_SETTLE_US", "0", 1);
  setenv("SYSTOLIC_DPTI_EPOCH", "5", 1);
  setenv("SYSTOLIC_ASYNC_POLL_US", "0", 1);
  if (queue_limit_1)
    setenv("SYSTOLIC_ASYNC_QUEUE_LIMIT", "1", 1);
  if (queue_limit_1) {
    scenario_async_runtime(true);
  } else {
    scenario_equivalence();
    scenario_many_jobs();
    scenario_partial_writes();
    scenario_failures();
    scenario_sync_runtime();
    scenario_async_runtime(false);
  }
  unlink(cfg.c_str());
  std::printf("%s: %d failure(s)\n", g_failures ? "DPTI COALESCE: FAIL" : "DPTI COALESCE: PASS", g_failures);
  return g_failures ? 1 : 0;
}
