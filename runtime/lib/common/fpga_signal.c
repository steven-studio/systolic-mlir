#include "fpga_signal.h"

#include <signal.h>
#include <string.h>
#include <unistd.h>

// volatile sig_atomic_t is the only type the C standard lets a handler and
// normal code share. Anything wider is not guaranteed to be read or written
// in one indivisible step.
static volatile sig_atomic_t g_stop = 0;

// How many stops have been asked for. The second one gives up on being
// graceful: if the first Ctrl+C did not get us out -- a wedged read waiting
// out its 5 second VTIME, say -- the user should not have to wait again.
static volatile sig_atomic_t g_count = 0;

static void on_stop(int sig) {
    g_stop = 1;
    if (++g_count >= 2) {
        // Restore the default disposition and re-raise, so the process dies
        // with the right exit status. Only async-signal-safe calls here:
        // no printf, no malloc, no exit().
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

void fpga_install_signal_handler(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_stop;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // no SA_RESTART -- see the header
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

int fpga_stop_requested(void) { return g_stop != 0; }

void fpga_clear_stop_request(void) {
    g_stop = 0;
    g_count = 0;
}