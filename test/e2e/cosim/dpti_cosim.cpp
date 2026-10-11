// dpti_cosim.cpp -- DPI-C side of tb_dpti_cosim.  BENCH ONLY.
//
// Two jobs:
//
//   1. The host end of the FT2232H. Bytes the host writes (libftdi_cosim.so,
//      over a UNIX socket) queue up as the FT2232H's receive FIFO; bytes the
//      FPGA transmits are sent back. One host connection is one session, like
//      one ftdi_usb_open() on the board; between sessions the simulation
//      waits in accept(), so simulated time does not run while no host is
//      attached.
//
//      When the host closes, the bytes it already wrote are still delivered
//      to the RTL as it reads them (for at most kDrainCycles FIFO clocks),
//      as the board, ~1000x faster than this simulation, would have taken
//      them before the host got to its close. Each session logs the bytes
//      received from the host, the bytes the RTL consumed (RD# strobes) and
//      the bytes the RTL sent (WR# strobes).
//
//   2. The arithmetic of the Floating-Point Operator IP for fp_ieee.sv:
//      binary32 multiply and add, round to nearest even, denormals flushed to
//      signed zero on input and output. C float arithmetic on x86-64/AArch64
//      is IEEE binary32 with RNE; each operation is a separate call, so no
//      fused multiply-add can form.

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

int g_listen = -1;
int g_conn = -1;
std::string g_path;
int g_max_sessions = 0;
int g_sessions = 0;
bool g_done = false;

std::deque<uint8_t> g_rx;    // host -> FPGA, the FT2232H receive FIFO
std::vector<uint8_t> g_tx;   // FPGA -> host, not yet sent

unsigned long long g_session_rx = 0;        // received from the host
unsigned long long g_session_consumed = 0;  // taken by the RTL (RD#)
unsigned long long g_session_tx = 0;        // sent by the RTL (WR#)

// The host has gone; g_rx is still being delivered to the RTL.
bool g_draining = false;
const char *g_drain_why = "";
long g_drain_cycles = 0;
constexpr long kDrainCycles = 1L << 20;  // ~17 ms of the 60 MHz FIFO clock

void finish_session(const char *why) {
  if (g_conn >= 0) {
    // Hand back whatever the FPGA produced before the host went away.
    if (!g_tx.empty())
      (void)send(g_conn, g_tx.data(), g_tx.size(), MSG_NOSIGNAL);
    close(g_conn);
    g_conn = -1;
  }
  std::fprintf(stdout,
               "cosim: session %d closed (%s): host->fpga %llu B received, "
               "%llu B consumed by the RTL, fpga->host %llu B\n",
               g_sessions, why, g_session_rx, g_session_consumed,
               g_session_tx);
  std::fflush(stdout);

  // A new session starts from purged FIFOs, as ftdi_usb_purge_buffers()
  // leaves them on the board.
  g_rx.clear();
  g_tx.clear();
  g_draining = false;

  if (g_max_sessions > 0 && g_sessions >= g_max_sessions)
    g_done = true;
}

// The host closed (or vanished): stop talking to it, keep feeding the RTL
// what it had written.
void host_gone(const char *why) {
  if (g_conn >= 0) {
    close(g_conn);
    g_conn = -1;
  }
  g_tx.clear();
  g_draining = true;
  g_drain_why = why;
  g_drain_cycles = 0;
}

void accept_session() {
  for (;;) {
    int fd = accept(g_listen, nullptr, nullptr);
    if (fd >= 0) {
      fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
      g_conn = fd;
      g_sessions++;
      g_session_rx = 0;
      g_session_consumed = 0;
      g_session_tx = 0;
      std::fprintf(stdout, "cosim: session %d opened\n", g_sessions);
      std::fflush(stdout);
      return;
    }
    if (errno != EINTR) {
      std::perror("cosim: accept");
      std::exit(1);
    }
  }
}

uint32_t ftz(uint32_t bits) {
  if ((bits & 0x7f800000u) == 0)
    return bits & 0x80000000u;
  return bits;
}

float as_float(uint32_t bits) {
  float f;
  std::memcpy(&f, &bits, sizeof f);
  return f;
}

uint32_t as_bits(float f) {
  uint32_t bits;
  std::memcpy(&bits, &f, sizeof bits);
  return bits;
}

} // namespace

extern "C" {

int cosim_init(const char *path, int max_sessions) {
  g_path = path ? path : "";
  g_max_sessions = max_sessions;

  g_listen = socket(AF_UNIX, SOCK_STREAM, 0);
  if (g_listen < 0) {
    std::perror("cosim: socket");
    return -1;
  }

  sockaddr_un addr{};
  addr.sun_family = AF_UNIX;
  if (g_path.size() >= sizeof(addr.sun_path)) {
    std::fprintf(stderr, "cosim: socket path too long: %s\n", g_path.c_str());
    return -1;
  }
  std::strcpy(addr.sun_path, g_path.c_str());
  unlink(g_path.c_str());

  if (bind(g_listen, reinterpret_cast<sockaddr *>(&addr), sizeof addr) != 0 ||
      listen(g_listen, 1) != 0) {
    std::perror("cosim: bind/listen");
    return -1;
  }
  return 0;
}

// Called once per FT2232H clock. Returns 1 when the bench should finish.
int cosim_service() {
  if (g_done)
    return 1;

  if (g_draining) {
    if (g_rx.empty() || ++g_drain_cycles > kDrainCycles)
      finish_session(g_drain_why);
    return g_done ? 1 : 0;
  }

  if (g_conn < 0)
    accept_session();

  while (!g_tx.empty()) {
    ssize_t n = send(g_conn, g_tx.data(), g_tx.size(),
                     MSG_NOSIGNAL | MSG_DONTWAIT);
    if (n > 0) {
      g_tx.erase(g_tx.begin(), g_tx.begin() + n);
      continue;
    }
    if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
      break;
    host_gone("host gone while sending");
    return 0;
  }

  uint8_t buf[4096];
  ssize_t n = recv(g_conn, buf, sizeof buf, MSG_DONTWAIT);
  if (n > 0) {
    g_rx.insert(g_rx.end(), buf, buf + n);
    g_session_rx += static_cast<unsigned long long>(n);
  } else if (n == 0) {
    host_gone("host closed");
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    host_gone("receive error");
  }

  return 0;
}

int cosim_rx_count() { return static_cast<int>(g_rx.size()); }

int cosim_rx_peek() { return g_rx.empty() ? 0 : g_rx.front(); }

void cosim_rx_pop() {
  if (!g_rx.empty()) {
    g_rx.pop_front();
    g_session_consumed++;
  }
}

void cosim_tx_push(int b) {
  // While draining the host is gone: the byte is counted as sent by the
  // RTL but has nowhere to go.
  if (!g_draining)
    g_tx.push_back(static_cast<uint8_t>(b));
  g_session_tx++;
}

unsigned int cosim_f32_mul(unsigned int a, unsigned int b) {
  return ftz(as_bits(as_float(ftz(a)) * as_float(ftz(b))));
}

unsigned int cosim_f32_add(unsigned int a, unsigned int b) {
  return ftz(as_bits(as_float(ftz(a)) + as_float(ftz(b))));
}

} // extern "C"
