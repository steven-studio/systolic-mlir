#include "dpti_runtime.h"

#include <ftdi.h>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <thread>
#include <vector>

namespace {

struct FtdiTransportState {
    struct ftdi_context *ftdi = nullptr;

    // Every byte that crossed the FT2232H, either way.
    unsigned long long bytes_written = 0;
    unsigned long long bytes_read = 0;

    // Every ftdi_write_data call, failed ones included (Phase T1: the
    // quantity descriptor coalescing reduces).
    unsigned long long write_calls = 0;

    // Set by the first WRITE32 of a descriptor that did not go out. The
    // write32 callback cannot return an error to dpti_submit(), so the
    // rest of that descriptor -- CONTROL = submit above all -- is skipped
    // and dpti_ftdi_submit_session() reports the failure.
    bool write32_failed = false;

    // Phase T1: descriptor coalescing (dpti_runtime.h). `coalesce` is
    // SYSTOLIC_DPTI_COALESCE_DESCRIPTORS=1 at open. While `staging` is set,
    // the write32 callback appends its 6-byte packet to desc_buf instead of
    // writing it, and dpti_ftdi_submit_session() writes the whole buffer
    // once, so the bytes on the wire are the per-packet path's, in order.
    bool coalesce = false;
    bool staging = false;
    bool staging_overflow = false;
    unsigned desc_len = 0;
    unsigned char desc_buf[DPTI_DESCRIPTOR_BYTES];

    // Phase T2: MEM_WRITE coalescing (dpti_runtime.h). `coalesce_mem` is
    // SYSTOLIC_DPTI_COALESCE_MEM_WRITES=1 at open; mem_buf holds header ||
    // payload for the one write, grown to the largest MEM_WRITE of the
    // session and reused (the bitstream fixes the slab length, so it grows
    // once).
    bool coalesce_mem = false;
    std::vector<unsigned char> mem_buf;

    // Phase T3 instrumentation (dpti_runtime.h): the transport's reads.
    unsigned long long read_calls = 0;
    unsigned long long read_empty = 0;
    unsigned long long read_usb = 0;
    unsigned long long read_ns = 0;
};

/*
 * Per-packet logging (every WRITE32, every MEM_WRITE, session open/close)
 * is off unless SYSTOLIC_DPTI_TRACE is set; failures are always reported.
 */
static bool dpti_trace()
{
    static int enabled = -1;

    if (enabled < 0) {
        const char *v = std::getenv("SYSTOLIC_DPTI_TRACE");
        enabled = (v && *v && std::strcmp(v, "0") != 0) ? 1 : 0;
    }

    return enabled == 1;
}

/*
 * One ftdi_read_data(), counted and timed (Phase T3 instrumentation).
 * libftdi serves the call whole from the bytes it already holds when it can
 * (readbuffer_remaining >= size) and then makes no USB transfer; otherwise
 * it issues bulk transfers until size bytes are in or one comes back with
 * no data -- on the FT2232H, the latency timer expiring with nothing
 * pending. Returns what ftdi_read_data() returns.
 */
static int transport_read(
    FtdiTransportState *state,
    unsigned char *buf,
    int size)
{
    struct ftdi_context *ftdi = state->ftdi;
    const bool buffered =
        ftdi->readbuffer_remaining >= static_cast<unsigned>(size);
    const auto t0 = std::chrono::steady_clock::now();
    const int rc = ftdi_read_data(ftdi, buf, size);
    const auto t1 = std::chrono::steady_clock::now();
    const unsigned long long ns = static_cast<unsigned long long>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());

    state->read_calls++;
    if (!buffered)
        state->read_usb++;
    if (rc == 0)
        state->read_empty++;
    state->read_ns += ns;

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] read %d of %d byte(s) in %llu us%s\n",
            rc,
            size,
            ns / 1000,
            buffered ? " (from libftdi's buffer)" : "");
    return rc;
}

static int write_all(
    FtdiTransportState *state,
    const unsigned char *buf,
    int len)
{
    struct ftdi_context *ftdi = state->ftdi;
    int off = 0;

    while (off < len) {
        state->write_calls++;
        int rc = ftdi_write_data(
            ftdi,
            const_cast<unsigned char *>(buf + off),
            len - off);

        if (rc < 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] write failed: "
                "requested=%d transferred=%d remaining=%d rc=%d "
                "error=%s\n",
                len,
                off,
                len - off,
                rc,
                ftdi_get_error_string(ftdi));
            return -1;
        }

        if (rc == 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] write returned 0: "
                "requested=%d transferred=%d remaining=%d\n",
                len,
                off,
                len - off);
            return -1;
        }

        off += rc;
        state->bytes_written += static_cast<unsigned long long>(rc);
    }

    return 0;
}

static void ftdi_write32(
    void *ctx,
    uint32_t addr,
    uint32_t data)
{
    auto *state =
        static_cast<FtdiTransportState *>(ctx);

    if (state->write32_failed)
        return;

    unsigned char packet[6];

    packet[0] = 0x01; // WRITE32
    packet[1] = static_cast<unsigned char>(addr);

    packet[2] = static_cast<unsigned char>(data);
    packet[3] = static_cast<unsigned char>(data >> 8);
    packet[4] = static_cast<unsigned char>(data >> 16);
    packet[5] = static_cast<unsigned char>(data >> 24);

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] WRITE32 addr=0x%02x data=0x%08x%s\n",
            addr,
            data,
            state->staging ? " (staged)" : "");

    if (state->staging) {
        // Phase T1: the packet joins the descriptor batch, in emission
        // order; nothing is written here. A batch longer than a descriptor
        // cannot be produced by dpti_submit(), so an overflow is refused
        // rather than truncated.
        if (state->desc_len + sizeof(packet) > sizeof(state->desc_buf)) {
            state->staging_overflow = true;
            return;
        }
        std::memcpy(state->desc_buf + state->desc_len, packet, sizeof(packet));
        state->desc_len += static_cast<unsigned>(sizeof(packet));
        return;
    }

    if (write_all(
            state,
            packet,
            sizeof(packet)) != 0) {
        state->write32_failed = true;
        std::fprintf(
            stderr,
            "[DPTI FTDI] WRITE32 addr=0x%02x transport failure; "
            "the rest of the descriptor is not sent\n",
            addr);
    }
}

} // namespace

struct DptiFtdiSession {
    FtdiTransportState transport;
};

extern "C" DptiFtdiSession *dpti_ftdi_open(void)
{
    auto *session = new DptiFtdiSession();

    session->transport.ftdi = ftdi_new();

    if (!session->transport.ftdi) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] ftdi_new failed\n");

        delete session;
        return nullptr;
    }

    struct ftdi_context *ftdi =
        session->transport.ftdi;

    /*
     * FT2232 has two interfaces.
     *
     * The selected DPTI interface is interface B.
     */
    if (ftdi_set_interface(
            ftdi,
            INTERFACE_A) < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] ftdi_set_interface failed: %s\n",
            ftdi_get_error_string(ftdi));

        ftdi_free(ftdi);
        delete session;
        return nullptr;
    }

    /*
     * Open the already identified FT2232 device.
     */
    if (ftdi_usb_open(
            ftdi,
            0x0403,
            0x6010) < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] ftdi_usb_open failed: %s\n",
            ftdi_get_error_string(ftdi));

        ftdi_free(ftdi);
        delete session;
        return nullptr;
    }

    /*
     * Diagnostic: inspect FTDI latency timer before SYNC FIFO mode.
     */
    unsigned char latency_ms = 0;

    if (ftdi_get_latency_timer(ftdi, &latency_ms) < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] failed to read latency timer: %s\n",
            ftdi_get_error_string(ftdi));
    } else if (dpti_trace()) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] initial latency timer: %u ms\n",
            static_cast<unsigned>(latency_ms));
    }

    /*
     * Enter synchronous FIFO mode once for the entire descriptor
     * submission session.
     */
    if (ftdi_set_bitmode(
            ftdi,
            0xFF,
            BITMODE_SYNCFF) < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] failed to enter SYNC FIFO mode: %s\n",
            ftdi_get_error_string(ftdi));

        ftdi_usb_close(ftdi);
        ftdi_free(ftdi);
        delete session;
        return nullptr;
    }

    /*
     * SYSTOLIC_DPTI_LATENCY_MS: when set to 1..255, the FT2232H latency
     * timer is written to that value; unset, the device keeps whatever it
     * had (16 ms after power-up, which is also the value the board runs of
     * 2026-10-09..13 used -- the baseline measurements of docs/ASYNC.md
     * vary it). The timer is the time a partial USB packet (a 64- or
     * 256-byte result tile is one) waits in the FT2232H before it is sent
     * to the host.
     */
    unsigned latency_set = 0;
    if (const char *v = std::getenv("SYSTOLIC_DPTI_LATENCY_MS")) {
        char *end = nullptr;
        const long t = std::strtol(v, &end, 10);
        if (!*v || (end && *end) || t < 1 || t > 255) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] SYSTOLIC_DPTI_LATENCY_MS='%s' ignored: "
                "expected 1..255\n",
                v);
        } else if (ftdi_set_latency_timer(ftdi, static_cast<unsigned char>(t)) < 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] ftdi_set_latency_timer(%ld) failed: %s\n",
                t,
                ftdi_get_error_string(ftdi));
        } else {
            latency_set = static_cast<unsigned>(t);
        }
    }
    if (latency_set || dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] latency timer: %u ms (%s)\n",
            latency_set ? latency_set : static_cast<unsigned>(latency_ms),
            latency_set ? "SYSTOLIC_DPTI_LATENCY_MS" : "device default");

    /*
     * SYSTOLIC_DPTI_READ_CHUNK: libftdi's read chunk size in bytes (its
     * default is 4096). Only set when asked, so the default behaviour is
     * the board-verified one.
     */
    if (const char *v = std::getenv("SYSTOLIC_DPTI_READ_CHUNK")) {
        const long c = std::strtol(v, nullptr, 10);
        if (c <= 0 || ftdi_read_data_set_chunksize(ftdi, static_cast<unsigned>(c)) < 0)
            std::fprintf(
                stderr,
                "[DPTI FTDI] SYSTOLIC_DPTI_READ_CHUNK='%s' not applied\n",
                v);
        else if (dpti_trace())
            std::fprintf(stderr, "[DPTI FTDI] read chunk size: %ld bytes\n", c);
    }

    /*
     * SYSTOLIC_DPTI_COALESCE_DESCRIPTORS (Phase T1, dpti_runtime.h): "1"
     * sends each descriptor's 14 WRITE32 packets in one transport write.
     * Read once per session, so a process can compare both paths.
     */
    if (const char *v = std::getenv("SYSTOLIC_DPTI_COALESCE_DESCRIPTORS"))
        session->transport.coalesce = (*v && std::strcmp(v, "0") != 0);
    if (session->transport.coalesce || dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] descriptor coalescing: %s\n",
            session->transport.coalesce
                ? "on (SYSTOLIC_DPTI_COALESCE_DESCRIPTORS): 14 WRITE32 per "
                  "descriptor in one write"
                : "off: one write per WRITE32");

    /*
     * SYSTOLIC_DPTI_COALESCE_MEM_WRITES (Phase T2, dpti_runtime.h): "1"
     * sends each MEM_WRITE's header and payload in one transport write.
     */
    if (const char *v = std::getenv("SYSTOLIC_DPTI_COALESCE_MEM_WRITES"))
        session->transport.coalesce_mem = (*v && std::strcmp(v, "0") != 0);
    if (session->transport.coalesce_mem || dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] MEM_WRITE coalescing: %s\n",
            session->transport.coalesce_mem
                ? "on (SYSTOLIC_DPTI_COALESCE_MEM_WRITES): header and payload "
                  "in one write"
                : "off: header, then payload");

    /*
     * Purge once at session start.  Do not purge between scheduled
     * jobs because they belong to one compiler-generated timeline.
     */
    if (ftdi_usb_purge_buffers(ftdi) < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] purge failed: %s\n",
            ftdi_get_error_string(ftdi));

        ftdi_set_bitmode(
            ftdi,
            0x00,
            BITMODE_RESET);

        ftdi_usb_close(ftdi);
        ftdi_free(ftdi);
        delete session;
        return nullptr;
    }

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] persistent session opened: FT2232 0403:6010\n");

    return session;
}

extern "C" int dpti_ftdi_submit_session(
    DptiFtdiSession *session,
    const dpti_job_t *job)
{
    if (!session || !session->transport.ftdi)
        return -1;

    if (!job)
        return -2;

    const int validate_rc =
        dpti_job_validate(job);

    if (validate_rc != 0)
        return -3;

    FtdiTransportState &state = session->transport;
    state.write32_failed = false;

    if (!state.coalesce) {
        const int rc =
            dpti_submit(
                job,
                ftdi_write32,
                &state);

        if (rc != 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] dpti_submit failed rc=%d\n",
                rc);
            return rc;
        }

        if (state.write32_failed) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] job %u not submitted: a descriptor WRITE32 "
                "failed\n",
                job->job_id);
            return -6;
        }

        return 0;
    }

    /*
     * Phase T1: stage the 14 packets dpti_submit() emits, in its order,
     * then write them as one buffer. Until the write, nothing has left the
     * host, so a staging failure sends nothing at all.
     */
    state.staging = true;
    state.staging_overflow = false;
    state.desc_len = 0;

    const int rc =
        dpti_submit(
            job,
            ftdi_write32,
            &state);

    state.staging = false;

    if (rc != 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] dpti_submit failed rc=%d (nothing sent)\n",
            rc);
        return rc;
    }

    if (state.staging_overflow || state.desc_len != DPTI_DESCRIPTOR_BYTES) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] job %u not submitted: the descriptor batch holds "
            "%u bytes, not %u (nothing sent)\n",
            job->job_id,
            state.desc_len,
            static_cast<unsigned>(DPTI_DESCRIPTOR_BYTES));
        state.write32_failed = true;
        return -6;
    }

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] descriptor batch job=%u: %u WRITE32, %u bytes, "
            "one write\n",
            job->job_id,
            static_cast<unsigned>(DPTI_DESCRIPTOR_WRITES),
            state.desc_len);

    if (write_all(
            &state,
            state.desc_buf,
            static_cast<int>(state.desc_len)) != 0) {
        // Not retried: a prefix of the batch may already be in the device
        // (write_all counts what went out in bytes_written), and resending
        // could duplicate register writes. CONTROL is the batch's last
        // packet, so no prefix submits a job.
        state.write32_failed = true;
        std::fprintf(
            stderr,
            "[DPTI FTDI] job %u not submitted: the coalesced descriptor "
            "write failed\n",
            job->job_id);
        return -6;
    }

    return 0;
}

extern "C" int dpti_ftdi_descriptor_coalescing(
    const DptiFtdiSession *session)
{
    return (session && session->transport.coalesce) ? 1 : 0;
}

extern "C" unsigned long long dpti_ftdi_write_calls(
    const DptiFtdiSession *session)
{
    return session ? session->transport.write_calls : 0;
}

extern "C" int dpti_ftdi_mem_write_session(
    DptiFtdiSession *session,
    uint32_t address,
    const void *data,
    uint32_t length)
{
    if (!session || !session->transport.ftdi)
        return -1;

    if (!data)
        return -2;

    /*
     * The RTL decoder accepts exactly one operand slab per MEM_WRITE, of the
     * length its build fixed (dpti_mem_write_decoder PAYLOAD_BYTES: the
     * deepest geometry's slab, 1024 bytes at K_MAX_8X8 = K_MAX_4X4 = 16).
     * Which length that is belongs to the caller, who knows the bitstream
     * (fpga_runtime_config_t.slab_bytes); here the payload only has to be
     * whole 16-byte beats, the decoder's unit.
     */
    if (length == 0 || length % 16 != 0)
        return -3;

    unsigned char header[9];

    header[0] = 0x02; // MEM_WRITE

    header[1] = static_cast<unsigned char>(address);
    header[2] = static_cast<unsigned char>(address >> 8);
    header[3] = static_cast<unsigned char>(address >> 16);
    header[4] = static_cast<unsigned char>(address >> 24);

    header[5] = static_cast<unsigned char>(length);
    header[6] = static_cast<unsigned char>(length >> 8);
    header[7] = static_cast<unsigned char>(length >> 16);
    header[8] = static_cast<unsigned char>(length >> 24);

    FtdiTransportState &state = session->transport;

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] MEM_WRITE addr=0x%08x bytes=%u%s\n",
            address,
            length,
            state.coalesce_mem ? " (header and payload in one write)" : "");

    if (!state.coalesce_mem) {
        if (write_all(
                &state,
                header,
                sizeof(header)) != 0)
            return -4;

        if (write_all(
                &state,
                static_cast<const unsigned char *>(data),
                static_cast<int>(length)) != 0)
            return -5;

        return 0;
    }

    /*
     * Phase T2: header || payload as one buffer, one write. The same bytes
     * in the same order as the two writes above; write_all continues a
     * short write from the first unaccepted byte and stops at a failure,
     * so a failure leaves the device a prefix exactly as before. The
     * return code tells, as before, whether the header went out whole
     * (then -5) or not (-4), from the bytes the transport accepted.
     */
    const size_t total = sizeof(header) + static_cast<size_t>(length);
    if (state.mem_buf.size() < total)
        state.mem_buf.resize(total);
    std::memcpy(state.mem_buf.data(), header, sizeof(header));
    std::memcpy(state.mem_buf.data() + sizeof(header), data, length);

    const unsigned long long before = state.bytes_written;
    if (write_all(
            &state,
            state.mem_buf.data(),
            static_cast<int>(total)) != 0) {
        const unsigned long long accepted = state.bytes_written - before;
        return accepted < sizeof(header) ? -4 : -5;
    }

    return 0;
}

extern "C" int dpti_ftdi_mem_write_coalescing(
    const DptiFtdiSession *session)
{
    return (session && session->transport.coalesce_mem) ? 1 : 0;
}

extern "C" int dpti_ftdi_read_exact_session(
    DptiFtdiSession *session,
    void *data,
    uint32_t length,
    uint32_t timeout_ms)
{
    if (!session || !session->transport.ftdi)
        return -1;

    if (!data || length == 0)
        return -2;

    auto *out =
        static_cast<unsigned char *>(data);

    uint32_t off = 0;

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(timeout_ms);

    while (off < length) {
        const int rc =
            transport_read(
                &session->transport,
                out + off,
                static_cast<int>(length - off));

        if (rc < 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] read failed: %s\n",
                ftdi_get_error_string(
                    session->transport.ftdi));

            return -3;
        }

        if (rc > 0) {
            off += static_cast<uint32_t>(rc);
            session->transport.bytes_read +=
                static_cast<unsigned long long>(rc);
            continue;
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] read timeout: got %u / %u bytes\n",
                off,
                length);

            // Diagnostic only: wait up to 10 more seconds
            // for the remaining bytes before returning failure.
            const auto tail_deadline =
                std::chrono::steady_clock::now() +
                std::chrono::seconds(10);

            while (off < length &&
                   std::chrono::steady_clock::now() < tail_deadline) {
                const int tail_rc = transport_read(
                    &session->transport,
                    out + off,
                    static_cast<int>(length - off));

                if (tail_rc < 0) {
                    std::fprintf(stderr,
                        "[DPTI TAIL] read error: %s\n",
                        ftdi_get_error_string(
                            session->transport.ftdi));
                    break;
                }

                if (tail_rc > 0) {
                    off += static_cast<uint32_t>(tail_rc);
                    session->transport.bytes_read +=
                        static_cast<unsigned long long>(tail_rc);
                    std::fprintf(stderr,
                        "[DPTI TAIL] recovered: %u / %u bytes\n",
                        off, length);
                } else {
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(1));
                }
            }

            std::fprintf(stderr,
                "[DPTI TAIL] final: %u / %u bytes\n",
                off, length);

            return -4;
        }

        std::this_thread::sleep_for(
            std::chrono::milliseconds(1));
    }

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] read complete: %u / %u bytes\n",
            off,
            length);

    return 0;
}

extern "C" int dpti_ftdi_read_some_session(
    DptiFtdiSession *session,
    void *data,
    uint32_t max_length)
{
    if (!session || !session->transport.ftdi)
        return -1;

    if (!data || max_length == 0)
        return -2;

    const int rc =
        transport_read(
            &session->transport,
            static_cast<unsigned char *>(data),
            static_cast<int>(max_length));

    if (rc < 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] read failed: %s\n",
            ftdi_get_error_string(
                session->transport.ftdi));
        return -3;
    }

    session->transport.bytes_read +=
        static_cast<unsigned long long>(rc);

    return rc;
}

extern "C" void dpti_ftdi_read_stats(
    const DptiFtdiSession *session,
    dpti_ftdi_read_stats_t *out)
{
    if (!out)
        return;
    *out = dpti_ftdi_read_stats_t{};
    if (!session)
        return;
    out->calls = session->transport.read_calls;
    out->empty = session->transport.read_empty;
    out->usb   = session->transport.read_usb;
    out->ns    = session->transport.read_ns;
}

extern "C" void dpti_ftdi_close(
    DptiFtdiSession *session)
{
    if (!session)
        return;

    struct ftdi_context *ftdi =
        session->transport.ftdi;

    if (ftdi) {
        ftdi_set_bitmode(
            ftdi,
            0x00,
            BITMODE_RESET);

        ftdi_usb_close(ftdi);
        ftdi_free(ftdi);

        session->transport.ftdi = nullptr;
    }

    if (dpti_trace())
        std::fprintf(
            stderr,
            "[DPTI FTDI] persistent session closed\n");

    delete session;
}

extern "C" unsigned long long dpti_ftdi_bytes_written(
    const DptiFtdiSession *session)
{
    return session ? session->transport.bytes_written : 0;
}

extern "C" unsigned long long dpti_ftdi_bytes_read(
    const DptiFtdiSession *session)
{
    return session ? session->transport.bytes_read : 0;
}
