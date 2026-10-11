/*
 * libftdi_cosim.c -- the libftdi1 calls runtime/dpti_ftdi.cpp makes, served
 * by the RTL co-simulation instead of a USB device.  BENCH ONLY.
 *
 * Loaded with LD_PRELOAD in front of the real libftdi1, so the program under
 * test -- systolic-matmul and the runtime linked into it -- is the same
 * binary that drives the board; only the far end of ftdi_write_data() and
 * ftdi_read_data() changes, from the FT2232H to tb_dpti_cosim over the UNIX
 * socket named by SYSTOLIC_DPTI_COSIM_SOCKET.
 *
 * Everything below the byte stream is the simulated board's: the opcode
 * framing, the AXI4-Lite descriptor registers, the DDR3 staging and the
 * result read-back are decoded and executed by the RTL.
 */

#include <ftdi.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

static int g_sock = -1;

/* SYSTOLIC_DPTI_COSIM_CAPTURE=<file>: append every byte written to the link,
 * so two runtimes can be compared byte for byte. */
static FILE *g_capture = NULL;

/* SYSTOLIC_DPTI_COSIM_FAIL_WRITE=<n>: the n-th ftdi_write_data() of a
 * session (from 1) fails as a USB write error would, sending nothing -- to
 * check that the runtime reports a broken link instead of carrying on. */
static long g_fail_write_at = 0;
static long g_writes = 0;

static int fail(struct ftdi_context *ftdi, int code, const char *msg) {
  if (ftdi)
    ftdi->error_str = msg;
  return code;
}

struct ftdi_context *ftdi_new(void) {
  struct ftdi_context *ftdi = calloc(1, sizeof *ftdi);
  if (ftdi)
    ftdi->error_str = "";
  return ftdi;
}

void ftdi_free(struct ftdi_context *ftdi) { free(ftdi); }

int ftdi_set_interface(struct ftdi_context *ftdi,
                       enum ftdi_interface interface) {
  (void)ftdi;
  (void)interface;
  return 0;
}

int ftdi_usb_open(struct ftdi_context *ftdi, int vendor, int product) {
  (void)vendor;
  (void)product;

  const char *path = getenv("SYSTOLIC_DPTI_COSIM_SOCKET");
  if (!path || !*path)
    return fail(ftdi, -3, "libftdi_cosim: SYSTOLIC_DPTI_COSIM_SOCKET is not set");

  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd < 0)
    return fail(ftdi, -3, "libftdi_cosim: socket() failed");

  struct sockaddr_un addr;
  memset(&addr, 0, sizeof addr);
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);

  /* The simulator may still be compiling or coming out of reset. */
  for (int attempt = 0; attempt < 600; ++attempt) {
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) == 0) {
      g_sock = fd;
      const char *cap = getenv("SYSTOLIC_DPTI_COSIM_CAPTURE");
      if (cap && *cap)
        g_capture = fopen(cap, "ab");
      const char *fail_at = getenv("SYSTOLIC_DPTI_COSIM_FAIL_WRITE");
      g_fail_write_at = (fail_at && *fail_at) ? strtol(fail_at, NULL, 10) : 0;
      g_writes = 0;
      fprintf(stderr, "[libftdi_cosim] connected to RTL co-simulation at %s\n",
              path);
      return 0;
    }
    usleep(100 * 1000);
  }

  close(fd);
  return fail(ftdi, -3, "libftdi_cosim: no co-simulation listening on the socket");
}

int ftdi_usb_close(struct ftdi_context *ftdi) {
  (void)ftdi;
  if (g_sock >= 0) {
    close(g_sock);
    g_sock = -1;
  }
  if (g_capture) {
    fclose(g_capture);
    g_capture = NULL;
  }
  return 0;
}

int ftdi_get_latency_timer(struct ftdi_context *ftdi, unsigned char *latency) {
  (void)ftdi;
  if (latency)
    *latency = 16;
  return 0;
}

int ftdi_set_bitmode(struct ftdi_context *ftdi, unsigned char bitmask,
                     unsigned char mode) {
  (void)ftdi;
  (void)bitmask;
  (void)mode;
  return 0;
}

int ftdi_usb_purge_buffers(struct ftdi_context *ftdi) {
  (void)ftdi;
  return 0;
}

int ftdi_write_data(struct ftdi_context *ftdi, const unsigned char *buf,
                    int size) {
  if (g_sock < 0)
    return fail(ftdi, -666, "libftdi_cosim: not open");

  if (++g_writes == g_fail_write_at) {
    fprintf(stderr, "[libftdi_cosim] injected failure of write #%ld (%d B)\n",
            g_writes, size);
    return fail(ftdi, -1, "libftdi_cosim: injected write failure");
  }

  int off = 0;
  while (off < size) {
    ssize_t n = send(g_sock, buf + off, (size_t)(size - off), MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR)
        continue;
      return fail(ftdi, -1, "libftdi_cosim: send() failed");
    }
    off += (int)n;
  }
  if (g_capture)
    fwrite(buf, 1, (size_t)size, g_capture);
  return size;
}

int ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size) {
  if (g_sock < 0)
    return fail(ftdi, -666, "libftdi_cosim: not open");

  ssize_t n = recv(g_sock, buf, (size_t)size, MSG_DONTWAIT);
  if (n > 0)
    return (int)n;
  if (n == 0)
    return fail(ftdi, -1, "libftdi_cosim: co-simulation closed the link");
  if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
    return 0;
  return fail(ftdi, -1, "libftdi_cosim: recv() failed");
}

const char *ftdi_get_error_string(struct ftdi_context *ftdi) {
  return (ftdi && ftdi->error_str) ? ftdi->error_str : "";
}
