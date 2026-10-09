#include "dpti_runtime.h"

#include <ftdi.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <chrono>
#include <thread>

namespace {

struct FtdiTransportState {
    struct ftdi_context *ftdi = nullptr;
};

static int write_all(
    struct ftdi_context *ftdi,
    const unsigned char *buf,
    int len)
{
    int off = 0;

    while (off < len) {
        int rc = ftdi_write_data(
            ftdi,
            const_cast<unsigned char *>(buf + off),
            len - off);

        if (rc < 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] write failed: %s\n",
                ftdi_get_error_string(ftdi));
            return -1;
        }

        if (rc == 0) {
            std::fprintf(
                stderr,
                "[DPTI FTDI] write returned 0\n");
            return -1;
        }

        off += rc;
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

    unsigned char packet[6];

    packet[0] = 0x01; // WRITE32
    packet[1] = static_cast<unsigned char>(addr);

    packet[2] = static_cast<unsigned char>(data);
    packet[3] = static_cast<unsigned char>(data >> 8);
    packet[4] = static_cast<unsigned char>(data >> 16);
    packet[5] = static_cast<unsigned char>(data >> 24);

    std::fprintf(
        stderr,
        "[DPTI FTDI] WRITE32 addr=0x%02x data=0x%08x\n",
        addr,
        data);

    if (write_all(
            state->ftdi,
            packet,
            sizeof(packet)) != 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] WRITE32 transport failure\n");
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
    } else {
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

    const int rc =
        dpti_submit(
            job,
            ftdi_write32,
            &session->transport);

    if (rc != 0) {
        std::fprintf(
            stderr,
            "[DPTI FTDI] dpti_submit failed rc=%d\n",
            rc);
    }

    return rc;
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
     * The current RTL decoder deliberately accepts exactly one
     * 1024-byte operand slab per MEM_WRITE command.
     */
    if (length != 1024)
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

    std::fprintf(
        stderr,
        "[DPTI FTDI] MEM_WRITE addr=0x%08x bytes=%u\n",
        address,
        length);

    if (write_all(
            session->transport.ftdi,
            header,
            sizeof(header)) != 0)
        return -4;

    if (write_all(
            session->transport.ftdi,
            static_cast<const unsigned char *>(data),
            static_cast<int>(length)) != 0)
        return -5;

    return 0;
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
            ftdi_read_data(
                session->transport.ftdi,
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
                const int tail_rc = ftdi_read_data(
                    session->transport.ftdi,
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

    std::fprintf(
        stderr,
        "[DPTI FTDI] read complete: %u / %u bytes\n",
        off,
        length);

    return 0;
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

    std::fprintf(
        stderr,
        "[DPTI FTDI] persistent session closed\n");

    delete session;
}

extern "C" int dpti_ftdi_submit(
    const dpti_job_t *job)
{
    if (!job)
        return -1;

    if (dpti_job_validate(job) != 0)
        return -2;

    DptiFtdiSession *session =
        dpti_ftdi_open();

    if (!session)
        return -3;

    const int rc =
        dpti_ftdi_submit_session(
            session,
            job);

    dpti_ftdi_close(session);

    return rc;
}
