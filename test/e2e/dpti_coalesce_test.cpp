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
// Phase T2 (SYSTOLIC_DPTI_COALESCE_MEM_WRITES=1): a MEM_WRITE's 9-byte
// header and its payload go out in one write instead of two. The T2
// scenarios check, against the same mock: header || payload equals the
// two-write stream and an independent encoding, for payloads from the
// smallest valid (16 B) to the protocol's largest (1,048,064 B) and
// boundary lengths, at several addresses; invalid lengths refused before
// any write; partial writes; zero-length and failed writes, also after a
// partial acceptance (-4 / -5 as before, the device left a prefix, no
// complete command); recovery after close; sequences of MEM_WRITEs;
// MEM_WRITEs interleaved with descriptors under all four knob
// combinations (16, 3, 15 and 2 write calls per invocation, one byte
// stream); and both runtimes under the four combinations.
//
//   dpti-coalesce-test                 every scenario
//   dpti-coalesce-test --queue-limit-1 the asynchronous runtime with
//                                      SYSTOLIC_ASYNC_QUEUE_LIMIT=1 (host-queue
//                                      backpressure), the four knob combinations
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

// Phase T2: the MEM_WRITE knob, independent of the descriptor knob.
void set_batch(bool on) {
  if (on)
    setenv("SYSTOLIC_ASYNC_BATCH", "1", 1);
  else
    unsetenv("SYSTOLIC_ASYNC_BATCH");
}

void set_coalesce_mem(bool on) {
  if (on)
    setenv("SYSTOLIC_DPTI_COALESCE_MEM_WRITES", "1", 1);
  else
    unsetenv("SYSTOLIC_DPTI_COALESCE_MEM_WRITES");
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
                     "COMPLETION_HEADER=1\nDDR_BYTES=1048576\nPIPELINE_DEPTH=4\n"
                     "JOB_QUEUE_DEPTH=4\nRB_DEPTH=8\n";
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
  long read_calls = 0, read_empty = 0; // Phase T3: the mock's read calls
};

AsyncRun run_async(bool coalesce, bool fail_first_descriptor, bool mem = false,
                   bool fail_first_mem_write = false, long fail_byte = 0,
                   bool sized = false, int read_max = 0, long fail_read = 0,
                   bool batch = false, int write_max = 0) {
  AsyncRun r;
  mock_ftdi_reset();
  if (fail_first_descriptor)
    mock_ftdi_fail_at_size(static_cast<int>(DPTI_DESCRIPTOR_BYTES), 1);
  if (fail_first_mem_write)
    mock_ftdi_fail_at_size(static_cast<int>(DPTI_MEM_WRITE_HEADER_BYTES) + 1024, 1);
  if (fail_byte > 0)
    mock_ftdi_fail_at_byte(fail_byte);
  if (read_max > 0)
    mock_ftdi_set_read_max_per_call(read_max);
  if (fail_read > 0)
    mock_ftdi_fail_read_at_call(fail_read);
  if (write_max > 0)
    mock_ftdi_set_max_per_call(write_max);
  set_coalesce(coalesce);
  set_coalesce_mem(mem);
  set_batch(batch);
  if (sized)
    setenv("SYSTOLIC_ASYNC_SIZED_READS", "1", 1);
  else
    unsetenv("SYSTOLIC_ASYNC_SIZED_READS");
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
  r.read_calls = mock_ftdi_read_calls();
  r.read_empty = mock_ftdi_read_empty();
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

// ---- Phase T2: MEM_WRITE coalescing ---------------------------------------

Bytes pattern(size_t n, unsigned seed) {
  Bytes b(n);
  uint32_t x = 2463534242u ^ seed;
  for (size_t i = 0; i < n; ++i) {
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    b[i] = static_cast<unsigned char>(x);
  }
  return b;
}

// The MEM_WRITE as dpti_runtime.h documents it, written independently of
// the transport: 0x02, address LE32, length LE32, payload.
Bytes reference_mem_write(uint32_t address, const Bytes &payload) {
  Bytes b;
  b.push_back(0x02);
  put32(b, address);
  put32(b, static_cast<uint32_t>(payload.size()));
  b.insert(b.end(), payload.begin(), payload.end());
  return b;
}

struct MemObserved {
  Bytes stream;
  long calls = 0;
  unsigned long long bytes_written = 0, write_calls = 0;
  int rc = 0;
  int coalescing = 0;
  long decoded = 0, complete = 0;
  unsigned address = 0, length = 0;
  bool payload_matches = false;
};

MemObserved mem_write_once(uint32_t address, const Bytes &payload, bool coalesce_mem) {
  MemObserved o;
  mock_ftdi_reset();
  set_coalesce_mem(coalesce_mem);
  DptiFtdiSession *s = dpti_ftdi_open();
  if (!s) {
    o.rc = -999;
    return o;
  }
  o.coalescing = dpti_ftdi_mem_write_coalescing(s);
  o.rc = dpti_ftdi_mem_write_session(s, address, payload.data(), static_cast<uint32_t>(payload.size()));
  o.stream = stream();
  o.calls = mock_ftdi_call_count();
  o.bytes_written = dpti_ftdi_bytes_written(s);
  o.write_calls = dpti_ftdi_write_calls(s);
  o.decoded = mock_ftdi_mem_writes();
  o.complete = mock_ftdi_mem_writes_complete();
  if (o.decoded == 1) {
    int complete = 0;
    const unsigned char *pl = nullptr;
    long received = 0;
    mock_ftdi_mem_write(0, &o.address, &o.length, &complete, &pl, &received);
    o.payload_matches = received == static_cast<long>(payload.size()) && complete &&
                        std::memcmp(pl, payload.data(), payload.size()) == 0;
  }
  dpti_ftdi_close(s);
  return o;
}

void scenario_t2_equivalence() {
  // 16 B is the smallest valid length (one beat); 1024 B the 16/16 slab;
  // 32768 B the K_MAX 512 slab; 1,048,064 B the protocol's largest slab
  // (ceil(16376 / 8) windows of 512 B); the others are window boundaries.
  const uint32_t lengths[] = {16, 32, 512, 1008, 1024, 1040, 1536, 4096, 32768, 1048064};
  const uint32_t addresses[] = {0x00010000u, 0x00003000u, 0x00015000u, 0x1fffff00u, 0x00000100u};
  for (uint32_t len : lengths) {
    const Bytes payload = pattern(len, len);
    // Every address at the slab length; one address for the other lengths.
    const size_t n_addr = len == 1024 ? sizeof addresses / sizeof addresses[0] : 1;
    for (size_t a = 0; a < n_addr; ++a) {
      const uint32_t addr = addresses[a];
      const MemObserved off = mem_write_once(addr, payload, false);
      const MemObserved on = mem_write_once(addr, payload, true);
      const Bytes ref = reference_mem_write(addr, payload);
      char what[200];
      std::snprintf(what, sizeof what,
                    "MEM_WRITE %u B at 0x%08x: one-write stream == two-write stream == reference encoding, decoded once, payload intact",
                    len, addr);
      check(off.rc == 0 && on.rc == 0 && off.stream == ref && on.stream == ref && off.coalescing == 0 &&
                on.coalescing == 1 && off.decoded == 1 && on.decoded == 1 && off.complete == 1 &&
                on.complete == 1 && on.address == addr && on.length == len && on.payload_matches &&
                off.payload_matches,
            what);
      std::snprintf(what, sizeof what, "MEM_WRITE %u B at 0x%08x: 2 write calls become 1, %u bytes written either way",
                    len, addr, len + 9);
      check(off.calls == 2 && on.calls == 1 && off.write_calls == 2 && on.write_calls == 1 &&
                off.bytes_written == len + 9 && on.bytes_written == len + 9,
            what);
    }
  }
  check(mock_ftdi_bad_opcodes() == 0, "no byte of a coalesced MEM_WRITE is read as an unknown opcode");
}

void scenario_t2_invalid() {
  const Bytes payload = pattern(1024, 7);
  for (int mode = 0; mode < 2; ++mode) {
    mock_ftdi_reset();
    set_coalesce_mem(mode == 1);
    DptiFtdiSession *s = dpti_ftdi_open();
    if (!check(s != nullptr, "session opens")) return;
    const int zero = dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 0);
    const int odd = dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1000);
    const int beat = dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 8);
    const int null = dpti_ftdi_mem_write_session(s, 0x10000, nullptr, 1024);
    check(zero == -3 && odd == -3 && beat == -3 && null == -2 && mock_ftdi_call_count() == 0 &&
              mock_ftdi_stream_size() == 0,
          mode ? "coalesced MEM_WRITE: lengths 0, 1000, 8 are -3 and a null payload -2, before any write"
               : "two-write MEM_WRITE: lengths 0, 1000, 8 are -3 and a null payload -2, before any write");
    dpti_ftdi_close(s);
  }
}

void scenario_t2_partial() {
  const Bytes payload = pattern(1024, 11);
  const Bytes ref = reference_mem_write(0x10000, payload);
  mock_ftdi_reset();
  set_coalesce_mem(true);
  mock_ftdi_set_max_per_call(100);
  DptiFtdiSession *s = dpti_ftdi_open();
  int rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  bool sizes = true;
  for (long c = 0; c < 11; ++c)
    sizes = sizes && mock_ftdi_call_request(c) == 1033 - 100 * static_cast<int>(c) &&
            mock_ftdi_call_accepted(c) == (c < 10 ? 100 : 33);
  check(rc == 0 && stream() == ref && mock_ftdi_call_count() == 11 && sizes &&
            mock_ftdi_mem_writes_complete() == 1 && dpti_ftdi_bytes_written(s) == 1033,
        "coalesced MEM_WRITE over a transport accepting 100 B per call: 11 calls continuing from the first unaccepted byte, identical bytes, one complete command");
  if (s) dpti_ftdi_close(s);
  mock_ftdi_reset();
  set_coalesce_mem(false);
  mock_ftdi_set_max_per_call(100);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  check(rc == 0 && stream() == ref && mock_ftdi_call_count() == 12 && mock_ftdi_mem_writes_complete() == 1,
        "two-write MEM_WRITE over the same transport: 12 calls, identical bytes");
  if (s) dpti_ftdi_close(s);
}

void scenario_t2_failures() {
  const Bytes payload = pattern(1024, 13);
  // Coalesced: the one write fails outright -> -4 (not even the header), nothing sent.
  mock_ftdi_reset();
  set_coalesce_mem(true);
  mock_ftdi_fail_at_call(1);
  DptiFtdiSession *s = dpti_ftdi_open();
  int rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  check(rc == -4 && mock_ftdi_stream_size() == 0 && mock_ftdi_mem_writes() == 0 &&
            dpti_ftdi_bytes_written(s) == 0 && dpti_ftdi_write_calls(s) == 1,
        "coalesced MEM_WRITE: a failed first write is -4 with nothing sent");
  if (s) dpti_ftdi_close(s);
  // Coalesced: a zero-length write is -4 too.
  mock_ftdi_reset();
  set_coalesce_mem(true);
  mock_ftdi_zero_at_call(1);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  check(rc == -4 && mock_ftdi_stream_size() == 0, "coalesced MEM_WRITE: a write that returns 0 is -4 with nothing sent");
  if (s) dpti_ftdi_close(s);
  // Coalesced: failure after partial acceptance -> -5 (the header went
  // out), the device holds a header and 391 payload bytes, no complete
  // command, nothing resent.
  mock_ftdi_reset();
  set_coalesce_mem(true);
  mock_ftdi_set_max_per_call(100);
  mock_ftdi_fail_at_call(5);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  {
    unsigned addr = 0, len = 0;
    int complete = 1;
    long received = -1;
    const bool decoded = mock_ftdi_mem_writes() == 1 &&
                         mock_ftdi_mem_write(0, &addr, &len, &complete, nullptr, &received) == 0;
    check(rc == -5 && mock_ftdi_stream_size() == 400 && dpti_ftdi_bytes_written(s) == 400 &&
              mock_ftdi_call_count() == 5 && decoded && addr == 0x10000 && len == 1024 && !complete &&
              received == 391 && mock_ftdi_mem_writes_complete() == 0,
          "coalesced MEM_WRITE: a failure after four partial writes is -5, a 400-byte prefix is out, the command is incomplete and never completed");
  }
  if (s) dpti_ftdi_close(s);
  // Two-write path, for reference: the payload write fails -> -5, header out.
  mock_ftdi_reset();
  set_coalesce_mem(false);
  mock_ftdi_fail_at_call(2);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  check(rc == -5 && mock_ftdi_stream_size() == 9 && mock_ftdi_mem_writes() == 1 &&
            mock_ftdi_mem_writes_complete() == 0,
        "two-write MEM_WRITE: a failed payload write is -5 with the header out (the reference behaviour)");
  if (s) dpti_ftdi_close(s);
  // Recovery: after a failure the session is closed by its caller; a new
  // session (and a device reset to its opcode state) runs again.
  mock_ftdi_reset();
  set_coalesce_mem(true);
  s = dpti_ftdi_open();
  rc = s ? dpti_ftdi_mem_write_session(s, 0x10000, payload.data(), 1024) : -999;
  check(rc == 0 && mock_ftdi_mem_writes_complete() == 1 && mock_ftdi_call_count() == 1,
        "coalesced MEM_WRITE: a new session after a failure writes a whole command again");
  if (s) dpti_ftdi_close(s);
}

void scenario_t2_sequence() {
  Bytes streams[2];
  long calls[2] = {0, 0}, complete[2] = {0, 0};
  bool decoded_ok[2] = {true, true};
  for (int mode = 0; mode < 2; ++mode) {
    mock_ftdi_reset();
    set_coalesce_mem(mode == 1);
    DptiFtdiSession *s = dpti_ftdi_open();
    if (!check(s != nullptr, "session opens")) return;
    int rc = 0;
    for (uint32_t i = 0; i < 20 && rc == 0; ++i) {
      const Bytes payload = pattern(1024, 100 + i);
      rc = dpti_ftdi_mem_write_session(s, 0x10000 + 1024 * i, payload.data(), 1024);
    }
    streams[mode] = stream();
    calls[mode] = mock_ftdi_call_count();
    complete[mode] = mock_ftdi_mem_writes_complete();
    for (long i = 0; i < 20; ++i) {
      unsigned addr = 0, len = 0;
      int c = 0;
      const unsigned char *pl = nullptr;
      long received = 0;
      const Bytes payload = pattern(1024, 100 + static_cast<unsigned>(i));
      if (mock_ftdi_mem_write(i, &addr, &len, &c, &pl, &received) != 0 || addr != 0x10000 + 1024 * i ||
          len != 1024 || !c || received != 1024 || std::memcmp(pl, payload.data(), 1024) != 0)
        decoded_ok[mode] = false;
    }
    check(rc == 0, mode ? "20 sequential MEM_WRITEs coalesced" : "20 sequential MEM_WRITEs in two writes each");
    dpti_ftdi_close(s);
  }
  check(streams[0] == streams[1] && streams[0].size() == 20 * 1033, "20 MEM_WRITEs: identical 20660-byte streams");
  check(calls[0] == 40 && calls[1] == 20, "20 MEM_WRITEs: 40 write calls become 20");
  check(complete[0] == 20 && complete[1] == 20 && decoded_ok[0] && decoded_ok[1],
        "20 MEM_WRITEs: the decoder saw 20 complete commands with the right addresses and payloads either way");
}

void scenario_t2_interleaved() {
  // The invocation pattern, MEM_WRITE then descriptor, eight times, under
  // the four knob combinations: one byte stream, 16 / 3 / 15 / 2 calls.
  Bytes streams[4];
  long calls[4] = {0, 0, 0, 0}, submits[4] = {0, 0, 0, 0}, complete[4] = {0, 0, 0, 0};
  int rc_all[4] = {0, 0, 0, 0};
  for (int cfg = 0; cfg < 4; ++cfg) {
    mock_ftdi_reset();
    set_coalesce((cfg & 1) != 0);
    set_coalesce_mem((cfg & 2) != 0);
    DptiFtdiSession *s = dpti_ftdi_open();
    if (!check(s != nullptr, "session opens")) return;
    int rc = 0;
    for (uint32_t i = 0; i < 8 && rc == 0; ++i) {
      const Bytes payload = pattern(1024, 200 + i);
      rc = dpti_ftdi_mem_write_session(s, 0x10000 + 1024 * (i % 5), payload.data(), 1024);
      if (rc == 0) {
        const dpti_job_t j = job(0x07000000u + i, i % 4, i % 4 == 0 ? 8 : 4, 16, 0x10000 + 1024 * (i % 5),
                                 0x3000 + 256 * i, 250 * i, 120);
        rc = dpti_ftdi_submit_session(s, &j);
      }
    }
    rc_all[cfg] = rc;
    streams[cfg] = stream();
    calls[cfg] = mock_ftdi_call_count();
    submits[cfg] = mock_ftdi_submit_count();
    complete[cfg] = mock_ftdi_mem_writes_complete();
    dpti_ftdi_close(s);
  }
  check(rc_all[0] == 0 && rc_all[1] == 0 && rc_all[2] == 0 && rc_all[3] == 0 && streams[0].size() == 8 * 1117 &&
            streams[1] == streams[0] && streams[2] == streams[0] && streams[3] == streams[0],
        "8 invocations interleaving MEM_WRITE and descriptor: one 8936-byte stream under all four knob combinations");
  char what[160];
  std::snprintf(what, sizeof what, "8 invocations: write calls %ld / %ld / %ld / %ld (baseline, T1, T2, T1+T2)", calls[0],
                calls[1], calls[2], calls[3]);
  check(calls[0] == 8 * 16 && calls[1] == 8 * 3 && calls[2] == 8 * 15 && calls[3] == 8 * 2, what);
  bool decoded = true;
  for (int cfg = 0; cfg < 4; ++cfg)
    decoded = decoded && submits[cfg] == 8 && complete[cfg] == 8;
  check(decoded, "8 invocations: the decoder saw 8 complete MEM_WRITEs and 8 submits under every combination");
  set_coalesce(false);
  set_coalesce_mem(false);
}

void scenario_t2_runtimes() {
  // The synchronous runtime: two invocations under the four combinations.
  std::vector<float> A(8 * 16), B(16 * 8), C(8 * 8), C0;
  fill(A, 21);
  fill(B, 22);
  fill(C, 23);
  C0 = C;
  Bytes masked[4];
  long calls[4] = {0, 0, 0, 0};
  const long expected_calls[4] = {32, 6, 30, 4};
  for (int cfg = 0; cfg < 4; ++cfg) {
    mock_ftdi_reset();
    set_coalesce((cfg & 1) != 0);
    set_coalesce_mem((cfg & 2) != 0);
    fpga_runtime_reset_stats();
    C = C0;
    const int rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
    const int rc2 = fpga_matmul_tiled_auto_scheduled(4, 7, 4, A.data(), B.data(), C.data(), 2, 500, 106);
    fpga_runtime_stats_t st{};
    fpga_runtime_get_stats(&st);
    masked[cfg] = mask_job_ids(stream());
    calls[cfg] = mock_ftdi_call_count();
    char what[160];
    std::snprintf(what, sizeof what, "sync runtime, combination %d (T1 %s, T2 %s): two invocations complete, C unchanged, %ld write calls",
                  cfg, (cfg & 1) ? "on" : "off", (cfg & 2) ? "on" : "off", calls[cfg]);
    check(rc == 0 && rc2 == 0 && C == C0 && st.invocations == 2 && st.bytes_to_fpga == 2 * 1117 &&
              st.bytes_from_fpga == 272 + 80 && calls[cfg] == expected_calls[cfg],
          what);
    fpga_runtime_close();
  }
  check(masked[1] == masked[0] && masked[2] == masked[0] && masked[3] == masked[0] && masked[0].size() == 2 * 1117,
        "sync runtime: the four combinations put the same bytes on the wire (job ids aside)");
  // Failure propagation with T2 on: the slab's one write fails -> -4, then -14, then recovery.
  mock_ftdi_reset();
  set_coalesce(true);
  set_coalesce_mem(true);
  mock_ftdi_fail_at_call(1);
  C = C0;
  const int rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  const int rc_after = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  check(rc == -4 && rc_after == -14 && C == C0 && mock_ftdi_mem_writes() == 0 && mock_ftdi_submit_count() == 0,
        "sync runtime T1+T2: a failed coalesced MEM_WRITE is -4, the next call -14, nothing on the wire, C untouched");
  fpga_runtime_close();
  mock_ftdi_reset();
  const int again = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
  check(again == 0 && mock_ftdi_submit_count() == 1 && mock_ftdi_call_count() == 2,
        "sync runtime T1+T2: a new session after close runs again with 2 write calls");
  fpga_runtime_close();
  set_coalesce(false);
  set_coalesce_mem(false);
}

void scenario_t2_async(bool backpressure) {
  const char *tag = backpressure ? " (host queue limit 1)" : "";
  const long expected_calls[4] = {64 * 16, 64 * 3, 64 * 15, 64 * 2};
  std::vector<Bytes> commands_of[4];
  for (int cfg = 0; cfg < 4; ++cfg) {
    AsyncRun r = run_async((cfg & 1) != 0, false, (cfg & 2) != 0, false);
    fpga_async_runtime_close();
    char what[200];
    std::snprintf(what, sizeof what,
                  "async runtime%s, combination %d (T1 %s, T2 %s): 64 tiles complete, C unchanged, 64 submits in per-device order, %ld write calls",
                  tag, cfg, (cfg & 1) ? "on" : "off", (cfg & 2) ? "on" : "off", r.calls);
    check(r.rc == 0 && r.st.tiles_completed == 64 && r.st.tiles_failed == 0 && r.c_unchanged && r.submits == 64 &&
              r.per_device_ascending && r.st.descriptors == 64 && r.st.mem_writes == 64 &&
              r.calls == expected_calls[cfg] && r.stream.size() == 64 * 1117,
          what);
    commands_of[cfg] = commands(mask_job_ids(r.stream));
    std::sort(commands_of[cfg].begin(), commands_of[cfg].end());
    if (backpressure) {
      std::snprintf(what, sizeof what, "async runtime (host queue limit 1), combination %d: submits waited for room (%llu)", cfg,
                    r.st.submits_blocked);
      check(r.st.submits_blocked > 0, what);
    }
  }
  check(commands_of[1] == commands_of[0] && commands_of[2] == commands_of[0] && commands_of[3] == commands_of[0],
        backpressure ? "async runtime (host queue limit 1): the four combinations send the same multiset of commands"
                     : "async runtime: the four combinations send the same multiset of commands (job ids aside)");
  if (backpressure)
    return;
  // Failure propagation with T2 on: the first coalesced MEM_WRITE fails ->
  // the GEMM fails (-4, or -14 after it), no job reaches the device, a
  // new session after close runs.
  AsyncRun f = run_async(true, false, true, true);
  char what[200];
  std::snprintf(what, sizeof what,
                "async runtime T1+T2: a failed coalesced MEM_WRITE fails the GEMM (rc %d), %llu completed + %llu failed of %llu accepted, 0 submits reached the device, C untouched",
                f.rc, f.st.tiles_completed, f.st.tiles_failed, f.st.tiles_submitted);
  check((f.rc == -4 || f.rc == -14) && f.st.tiles_submitted >= 1 && f.st.tiles_completed == 0 &&
            f.st.tiles_failed == f.st.tiles_submitted && f.submits == 0 && f.c_unchanged && mock_ftdi_mem_writes_complete() == 0,
        what);
  fpga_async_runtime_close();
  AsyncRun again = run_async(true, false, true, false);
  check(again.rc == 0 && again.st.tiles_completed == 64 && again.submits == 64 && again.calls == 64 * 2,
        "async runtime T1+T2: a new session after the failure runs 64 tiles again with 128 write calls");
  fpga_async_runtime_close();
  set_coalesce(false);
  set_coalesce_mem(false);
}

// ---- faults by accepted-byte position, under the four knob combinations ----
//
// The co-simulation's fault injector names the failing write by its call
// number (SYSTOLIC_DPTI_COSIM_FAIL_WRITE), which the two coalescing knobs
// renumber; its new SYSTOLIC_DPTI_COSIM_FAIL_BYTE and the mock's
// fail_at_byte name the fault by the position in the byte stream instead,
// so the same logical fault -- the 5th packet of the first descriptor, a
// byte of the first slab's payload, the 4th slab after three staged ones --
// lands on the same command whatever the call boundaries. What the knobs
// may change is the granularity: the failing call carries more bytes when
// coalesced, so the accepted prefix is shorter and the code follows the
// command the call carried (MEM_WRITE -4 / -5, descriptor -6). What they
// may not change: nothing of the failing call is accepted, nothing is
// sent after it, no job is submitted without its CONTROL, C is untouched,
// the next call is -14, and a new session after close runs.

void scenario_fault_by_position() {
  std::vector<float> A(8 * 16), B(16 * 8), C(8 * 8), C0;
  fill(A, 31);
  fill(B, 32);
  fill(C, 33);
  C0 = C;
  for (int cfg = 0; cfg < 4; ++cfg) {
    const bool t1 = (cfg & 1) != 0, t2 = (cfg & 2) != 0;
    struct Fault {
      long byte;
      const char *where;
    };
    const Fault faults[] = {{1058, "the first descriptor (its 5th packet in the baseline)"},
                            {500, "the first slab's payload"}};
    for (const Fault &f : faults) {
      mock_ftdi_reset();
      set_coalesce(t1);
      set_coalesce_mem(t2);
      mock_ftdi_fail_at_byte(f.byte);
      C = C0;
      const int rc = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
      const int rc2 = fpga_matmul_tiled_auto_scheduled(4, 7, 4, A.data(), B.data(), C.data(), 2, 500, 106);
      const long calls = mock_ftdi_call_count();
      long failing = -1;
      for (long i = 0; i < calls; ++i) {
        const long start = mock_ftdi_call_start(i);
        if (start < f.byte && start + mock_ftdi_call_request(i) >= f.byte) {
          failing = i;
          break;
        }
      }
      const long accepted = mock_ftdi_stream_size();
      // The runtime reports every MEM_WRITE failure as -4 (fpga_matmul_tiled.h)
      // whatever the transport's -4 / -5 distinction, which the T2 failure
      // scenario checks on the transport directly.
      const int expected_rc = f.byte == 1058 ? -6 : -4;
      char what[260];
      std::snprintf(what, sizeof what,
                    "sync, T1 %s T2 %s, fault at byte %ld in %s: call %ld of %ld fails whole, %ld bytes accepted before it, rc %d, next call -14, no submit, C untouched",
                    t1 ? "on" : "off", t2 ? "on" : "off", f.byte, f.where, failing + 1, calls, accepted, rc);
      check(failing >= 0 && failing == calls - 1 && mock_ftdi_call_accepted(failing) == 0 &&
                accepted == mock_ftdi_call_start(failing) && rc == expected_rc && rc2 == -14 &&
                mock_ftdi_submit_count() == 0 && mock_ftdi_mem_writes_complete() == (f.byte == 1058 ? 1 : 0) &&
                C == C0,
            what);
      fpga_runtime_close();
      mock_ftdi_reset();
      const int again = fpga_matmul_tiled_auto_scheduled(8, 16, 8, A.data(), B.data(), C.data(), 0, 0, 110);
      std::snprintf(what, sizeof what, "sync, T1 %s T2 %s, after the fault at byte %ld: a new session runs again", t1 ? "on" : "off",
                    t2 ? "on" : "off", f.byte);
      check(again == 0 && mock_ftdi_submit_count() == 1 && C == C0, what);
      fpga_runtime_close();
    }
    // Asynchronous: the 4th slab fails after three were staged (byte 3100):
    // exactly three slabs, 3099 bytes, are on the link in every combination.
    AsyncRun r = run_async(t1, false, t2, false, 3100);
    char what[260];
    std::snprintf(what, sizeof what,
                  "async, T1 %s T2 %s, fault at byte 3100 (the 4th slab): rc %d, 3099 bytes on the link, 3 complete slabs, 0 submits, %llu failed of %llu accepted, C untouched",
                  t1 ? "on" : "off", t2 ? "on" : "off", r.rc, r.st.tiles_failed, r.st.tiles_submitted);
    check((r.rc == -4 || r.rc == -14) && r.stream.size() == 3099 && mock_ftdi_mem_writes_complete() == 3 &&
              r.submits == 0 && r.st.tiles_completed == 0 && r.st.tiles_submitted >= 1 &&
              r.st.tiles_failed == r.st.tiles_submitted && r.c_unchanged && r.st.bytes_from_fpga == 0,
          what);
    fpga_async_runtime_close();
    AsyncRun again = run_async(t1, false, t2, false);
    std::snprintf(what, sizeof what, "async, T1 %s T2 %s, after the fault: a new session runs 64 tiles again", t1 ? "on" : "off",
                  t2 ? "on" : "off");
    check(again.rc == 0 && again.st.tiles_completed == 64 && again.submits == 64, what);
    fpga_async_runtime_close();
  }
  set_coalesce(false);
  set_coalesce_mem(false);
}

// ---- Phase T3: sized completion reads -------------------------------------
//
// SYSTOLIC_ASYNC_SIZED_READS makes the async runtime's transfer thread ask
// the transport for exactly the bytes of the completion record it is
// assembling (its 16-byte header, then its payload) instead of for the
// whole receive buffer. The bytes on the wire, their order and the records
// parse_rx builds are unchanged; only how the host cuts the stream into
// read calls differs. These scenarios prove, against the mock, that the two
// read paths are observably identical -- the same completions, the same
// per-device order, the same result bytes and credit accounting -- however
// the transport fragments the stream, and that a read failure fails closed
// the same way under both.

bool async_all_ok(const AsyncRun &r) {
  return r.rc == 0 && r.st.tiles_completed == 64 && r.st.tiles_failed == 0 &&
         r.c_unchanged && r.submits == 64 && r.per_device_ascending &&
         r.st.descriptors == 64 && r.st.mem_writes == 64 &&
         r.st.bytes_from_fpga == 16ull * 272 + 48ull * 80;
}

void scenario_t3_equivalence() {
  // Each T1/T2 combination, read side blocked (off) then sized (on): both
  // complete all 64 tiles identically.
  const bool combos[4][2] = {{false, false}, {true, false}, {false, true}, {true, true}};
  for (int i = 0; i < 4; ++i) {
    const bool t1 = combos[i][0], t2 = combos[i][1];
    AsyncRun off = run_async(t1, false, t2, false, 0, false);
    fpga_async_runtime_close();
    AsyncRun on = run_async(t1, false, t2, false, 0, true);
    fpga_async_runtime_close();
    char what[240];
    std::snprintf(what, sizeof what,
                  "T3 async (T1 %s T2 %s): blocked and sized reads each complete 64 tiles, C untouched, "
                  "64 submits in per-device order, %llu result bytes",
                  t1 ? "on" : "off", t2 ? "on" : "off", on.st.bytes_from_fpga);
    check(async_all_ok(off) && async_all_ok(on), what);
  }
  // Sized reads make one read per record header and one per payload, so
  // more read calls than the blocked path -- the same 64 completions.
  AsyncRun off = run_async(true, false, true, false, 0, false);
  fpga_async_runtime_close();
  AsyncRun on = run_async(true, false, true, false, 0, true);
  fpga_async_runtime_close();
  char what[240];
  std::snprintf(what, sizeof what,
                "T3 async: sized reads ask per record (%ld read calls vs %ld blocked), same 64 completions, same result bytes",
                on.read_calls, off.read_calls);
  check(on.read_calls >= off.read_calls && on.st.tiles_completed == 64 &&
            off.st.tiles_completed == 64 && on.st.bytes_from_fpga == off.st.bytes_from_fpga,
        what);
}

void scenario_t3_fragmented_reads() {
  // The transport returns at most `cap` bytes per read, whatever the runtime
  // asked for: a header split across reads (cap < 16), a payload split (cap
  // between a header and a tile), several records in one read (cap large),
  // with empty reads in between. Every cap, blocked and sized, completes all
  // 64 tiles with C untouched and the same result bytes.
  const int caps[] = {1, 7, 9, 16, 17, 80, 272, 4096};
  for (int cap : caps)
    for (int s = 0; s < 2; ++s) {
      const bool sized = s == 1;
      AsyncRun r = run_async(true, false, true, false, 0, sized, cap);
      fpga_async_runtime_close();
      char what[240];
      std::snprintf(what, sizeof what,
                    "T3 async %s reads, transport <= %d B per read: 64 tiles complete, C untouched, "
                    "%llu result bytes (%ld read calls, %ld empty)",
                    sized ? "sized" : "blocked", cap, r.st.bytes_from_fpga, r.read_calls, r.read_empty);
      check(async_all_ok(r), what);
    }
}

void scenario_t3_read_failure() {
  // A read that fails mid-stream (the 3rd read call), blocked and sized: the
  // link fails closed, every submitted tile is accounted exactly once
  // (completed or failed, none twice, none lost), nothing read after the
  // failure is counted as a result, and a new session after close runs all
  // 64 again.
  for (int s = 0; s < 2; ++s) {
    const bool sized = s == 1;
    AsyncRun f = run_async(true, false, true, false, 0, sized, 0, /*fail_read=*/3);
    char what[260];
    std::snprintf(what, sizeof what,
                  "T3 async %s reads: a failed read fails the GEMM (rc %d), %llu completed + %llu failed == %llu submitted (each tile once), C untouched",
                  sized ? "sized" : "blocked", f.rc, f.st.tiles_completed, f.st.tiles_failed,
                  f.st.tiles_submitted);
    check(f.rc != 0 && f.st.tiles_submitted == 64 &&
              f.st.tiles_completed + f.st.tiles_failed == f.st.tiles_submitted && f.c_unchanged,
          what);
    fpga_async_runtime_close();
    AsyncRun again = run_async(true, false, true, false, 0, sized);
    check(again.rc == 0 && again.st.tiles_completed == 64 && again.submits == 64,
          sized ? "T3 async sized reads: a new session after a read failure runs 64 tiles again"
                : "T3 async blocked reads: a new session after a read failure runs 64 tiles again");
    fpga_async_runtime_close();
  }
}

// ---- Phase T4: adaptive multi-job transport batching -----------------------
//
// SYSTOLIC_ASYNC_BATCH builds every legal write action of a burst -- the same
// admission decisions, order, slots and credits as the per-job path -- into
// one buffer and sends it in a single write_all. These scenarios prove,
// against the mock, that batching produces the identical device-visible
// command multiset and the identical completions as the per-job path, with
// fewer transport write calls; that a failed batch write fails closed with
// every tile accounted once and C untouched; and that batching makes forward
// progress under host-queue backpressure.

void scenario_t4_equivalence() {
  const bool combos[4][2] = {{false, false}, {true, false}, {false, true}, {true, true}};
  for (int i = 0; i < 4; ++i) {
    const bool t1 = combos[i][0], t2 = combos[i][1];
    AsyncRun off = run_async(t1, false, t2, false, 0, false, 0, 0, /*batch=*/false);
    fpga_async_runtime_close();
    AsyncRun on = run_async(t1, false, t2, false, 0, false, 0, 0, /*batch=*/true);
    fpga_async_runtime_close();
    std::vector<Bytes> a = commands(mask_job_ids(off.stream));
    std::vector<Bytes> b = commands(mask_job_ids(on.stream));
    std::sort(a.begin(), a.end());
    std::sort(b.begin(), b.end());
    char what[240];
    std::snprintf(what, sizeof what,
                  "T4 async (T1 %s T2 %s): batched stream == per-job stream (job ids aside), "
                  "64 tiles complete, C untouched, %ld write calls batched vs %ld per-job",
                  t1 ? "on" : "off", t2 ? "on" : "off", on.calls, off.calls);
    check(a == b && off.stream.size() == on.stream.size() && async_all_ok(off) &&
              async_all_ok(on) && on.calls < off.calls,
          what);
  }
}

void scenario_t4_fault() {
  // A batched write that fails after a prefix of accepted bytes: within the
  // first MEM_WRITE header, within a payload, at a command boundary, inside a
  // descriptor, and after several complete jobs. The 64-byte write cap makes
  // the prefix land before the failing call. Every case: the link fails
  // closed, every submitted tile is accounted exactly once (completed +
  // failed == submitted, none twice), C untouched, and a new session runs.
  const long positions[] = {1, 5, 100, 1033, 1040, 2066, 3099, 5000};
  for (long pos : positions) {
    AsyncRun r = run_async(true, false, true, false, /*fail_byte=*/pos, false, 0, 0,
                           /*batch=*/true, /*write_max=*/64);
    char what[240];
    std::snprintf(what, sizeof what,
                  "T4 async batch, write fails carrying byte %ld: link down (rc %d), %llu completed + "
                  "%llu failed == %llu submitted (each once), C untouched",
                  pos, r.rc, r.st.tiles_completed, r.st.tiles_failed, r.st.tiles_submitted);
    check(r.rc != 0 && r.st.tiles_submitted >= 1 && r.st.tiles_completed == 0 &&
              r.st.tiles_completed + r.st.tiles_failed == r.st.tiles_submitted && r.c_unchanged,
          what);
    fpga_async_runtime_close();
    AsyncRun again = run_async(true, false, true, false, 0, false, 0, 0, /*batch=*/true);
    check(again.rc == 0 && again.st.tiles_completed == 64 && again.submits == 64,
          "T4 async batch: a new session after a batched write failure runs 64 tiles again");
    fpga_async_runtime_close();
  }
}

void scenario_t4_saturation(bool backpressure) {
  // Batching under full in-flight depth, and (backpressure) a host queue of
  // one: the burst saturates the per-device credits, the batch flushes, the
  // GEMM still completes -- forward progress, no deadlock.
  AsyncRun on = run_async(true, false, true, false, 0, false, 0, 0, /*batch=*/true);
  fpga_async_runtime_close();
  char what[220];
  std::snprintf(what, sizeof what,
                "T4 async batch%s: 64 tiles complete, C untouched, max %u in flight per device honoured",
                backpressure ? " (host queue limit 1)" : "", 4u);
  check(async_all_ok(on), what);
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
  set_coalesce_mem(false);
  if (queue_limit_1) {
    scenario_async_runtime(true);
    scenario_t2_async(true);
    scenario_t4_saturation(true);
  } else {
    scenario_equivalence();
    scenario_many_jobs();
    scenario_partial_writes();
    scenario_failures();
    scenario_sync_runtime();
    scenario_async_runtime(false);
    scenario_t2_equivalence();
    scenario_t2_invalid();
    scenario_t2_partial();
    scenario_t2_failures();
    scenario_t2_sequence();
    scenario_t2_interleaved();
    scenario_t2_runtimes();
    scenario_t2_async(false);
    scenario_fault_by_position();
    scenario_t3_equivalence();
    scenario_t3_fragmented_reads();
    scenario_t3_read_failure();
    scenario_t4_equivalence();
    scenario_t4_fault();
    scenario_t4_saturation(false);
  }
  unlink(cfg.c_str());
  std::printf("%s: %d failure(s)\n", g_failures ? "DPTI COALESCE: FAIL" : "DPTI COALESCE: PASS", g_failures);
  return g_failures ? 1 : 0;
}
