/*
 * systolic-cc -- compile a C program, running the matrix multiplications the
 * compiler recognizes on the systolic-array FPGA.
 *
 *   systolic-cc [options] FILE.c -o PROGRAM
 *
 * Function by function:
 *
 *  1. systolic-cfront (clang 24 -ast-dump=json) translates every function
 *     inside the supported C subset to MLIR; the others (main, I/O, anything
 *     outside the subset) are reported as notes and stay C.
 *  2. --affine-matmul-to-systolic -- the existing GEMM recognition and exact
 *     Affine Partitioning over the systolic.device fleet -- decides which of
 *     them hold a GEMM it can lower; its remarks say why the others do not.
 *  3. Those functions ("offloaded") go through the existing pipeline:
 *     scheduled systolic.matmul_tile -> --lower-systolic-matmul-tile-to-fpga
 *     -> --systolic-runtime-to-llvm -> mlir-translate -> llc.
 *  4. The rest of the program is compiled by clang 24 as written, except that
 *     each offloaded function's definition is renamed NAME__systolic_reference
 *     (a prototype of NAME takes its place).
 *  5. A generated C shim defines NAME with its original C signature: it
 *     snapshots the arrays NAME writes, passes them to the compiled kernel,
 *     and asks the runtime (runtime/offload/systolic_offload.h) whether the
 *     FPGA ran it: hardware runtime, no failed invocation, exactly the
 *     planned number of invocations. If not, the kernel's writes are undone
 *     and NAME__systolic_reference -- the function as written -- runs on the
 *     CPU instead (CPU fallback; the program goes on). A call whose integer
 *     arguments place an access before the first element an array argument
 *     points to (a negative run-time row offset) is not offloaded at all and
 *     runs as written on the CPU. With --verify (testing) the result is also
 *     compared with the reference (exit 4 on a mismatch).
 *  6. Every GEMM loop nest of the program (systolic-cfront --gemm-sites)
 *     reports how it ran, once per execution, on stdout:
 *       [systolic-cc] FPGA Executed: GEMM #N
 *       [systolic-cc] CPU Executed: GEMM #N
 *     N counts GEMM executions from 1 (runtime/offload/systolic_cc_report.c).
 *     Loops that are not GEMMs report nothing.
 *  7. Everything is linked with a runtime: --runtime=board (default), the
 *     real runtime (DPTI to the programmed board; SYSTOLIC_FPGA_CONFIG names
 *     its bitstream, else the archived K_MAX 16 / 16 one, without the
 *     completion record: a bitstream that sends the record is refused at
 *     the first result and every GEMM runs on the CPU); --runtime=soft,
 *     the software model of the runtime ABI -- NOT the FPGA: its GEMMs
 *     report "CPU Executed". On a bitstream with the completion record
 *     (COMPLETION_HEADER=1) the kernels call the asynchronous runtime: every
 *     tile of a GEMM submitted without waiting for the one before it, one
 *     wait per GEMM (--submission auto, the default; sync keeps one
 *     synchronous call per tile, async insists and is refused elsewhere).
 *
 * The compiler prints only errors, unless --explain (what it offloads and
 * why not).
 *
 * Intermediate files are kept in PROGRAM.systolic/ (--keep-dir overrides).
 *
 * Exit status: 0 compiled; 1 a compilation step failed (its diagnostics on
 * stderr); 2 usage or tool-chain error.
 */

#define _GNU_SOURCE
#include "json.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifndef SYSTOLIC_SOURCE_DIR
#error "SYSTOLIC_SOURCE_DIR (the systolic-mlir source tree) must be defined"
#endif

#define KERNEL_SUFFIX "__systolic_kernel"
#define REFERENCE_SUFFIX "__systolic_reference"

/* ---------------------------------------------------------------- helpers */

static void *xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    fprintf(stderr, "systolic-cc: out of memory\n");
    exit(2);
  }
  return p;
}

static char *format(const char *fmt, ...) {
  va_list ap, aq;
  va_start(ap, fmt);
  va_copy(aq, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *out = xmalloc((size_t)n + 1);
  vsnprintf(out, (size_t)n + 1, fmt, aq);
  va_end(aq);
  return out;
}

typedef struct {
  const char **items;
  size_t n, cap;
} strvec;

static void push(strvec *v, const char *s) {
  if (v->n == v->cap) {
    v->cap = v->cap ? 2 * v->cap : 16;
    v->items = realloc(v->items, v->cap * sizeof *v->items);
    if (!v->items) {
      fprintf(stderr, "systolic-cc: out of memory\n");
      exit(2);
    }
  }
  v->items[v->n++] = s;
}

static void push_all(strvec *v, const strvec *w) {
  for (size_t i = 0; i < w->n; ++i)
    push(v, w->items[i]);
}

/* A growable text buffer. */
typedef struct {
  char *text;
  size_t n, cap;
} buffer;

static void bappend(buffer *b, const char *s, size_t n) {
  if (b->n + n + 1 > b->cap) {
    b->cap = (b->n + n + 1) * 2;
    b->text = realloc(b->text, b->cap);
    if (!b->text) {
      fprintf(stderr, "systolic-cc: out of memory\n");
      exit(2);
    }
  }
  memcpy(b->text + b->n, s, n);
  b->n += n;
  b->text[b->n] = 0;
}

static void bprintf(buffer *b, const char *fmt, ...) {
  va_list ap, aq;
  va_start(ap, fmt);
  va_copy(aq, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *s = xmalloc((size_t)n + 1);
  vsnprintf(s, (size_t)n + 1, fmt, aq);
  va_end(aq);
  bappend(b, s, (size_t)n);
  free(s);
}

/* --explain: the compiler's notes (what it offloads and why not). Without it
   the compiler prints nothing but errors, and the program prints nothing but
   its own output and one "[systolic-cc] ... Executed: GEMM #N" line per GEMM
   it runs (runtime/offload/systolic_cc_report.c). */
static int explain;

static void log_error(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fputs("systolic-cc: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

static void log_note(const char *fmt, ...) {
  if (!explain)
    return;
  va_list ap;
  va_start(ap, fmt);
  fputs("systolic-cc: ", stderr);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
}

static char *read_file(const char *path, size_t *length) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = xmalloc((size_t)(n > 0 ? n : 0) + 1);
  size_t got = fread(buf, 1, (size_t)(n > 0 ? n : 0), f);
  buf[got] = 0;
  fclose(f);
  if (length)
    *length = got;
  return buf;
}

static int write_file(const char *path, const char *text, size_t n) {
  FILE *f = fopen(path, "wb");
  if (!f)
    return 0;
  int ok = fwrite(text, 1, n, f) == n;
  return fclose(f) == 0 && ok;
}

static char *dir_name(const char *path) {
  const char *slash = strrchr(path, '/');
  if (!slash)
    return format(".");
  if (slash == path)
    return format("/");
  return format("%.*s", (int)(slash - path), path);
}

static const char *base_name(const char *path) {
  const char *slash = strrchr(path, '/');
  return slash ? slash + 1 : path;
}

/* ------------------------------------------------------------- the driver */

typedef struct {
  const char *source, *output, *clang, *build_dir, *llvm_build, *runtime;
  const char *fpga_config, *keep_dir, *std, *opt_level;
  const char *submission;      /* auto | sync | async (see fleet_config) */
  const char *cost_model;      /* v1 | v2: the binary search's cost model */
  long k_max_8x8, k_max_4x4;
  int verify;
  int fp_reassociation;         /* --fp-reassociation */
  int no_reduction_batches;     /* --no-reduction-batches */
  int fp_distribute;            /* --fp-distribute */
  double verify_rtol;
  strvec defines, includes;
  const char *self_dir;        /* where this executable is: systolic-cfront */
} options;

typedef struct {
  const options *a;
  FILE *commands;
  char error[3 * PATH_MAX];
} driver;

static char *kpath(driver *d, const char *name) {
  return format("%s/%s", d->a->keep_dir, name);
}

static void quote_into(buffer *b, const char *s) {
  int plain = *s != 0;
  for (const char *c = s; *c; ++c)
    if (!(isalnum((unsigned char)*c) || strchr("_@%+=:,./-", *c)))
      plain = 0;
  if (plain) {
    bappend(b, s, strlen(s));
    return;
  }
  bappend(b, "'", 1);
  for (const char *c = s; *c; ++c) {
    if (*c == '\'')
      bappend(b, "'\\''", 4);
    else
      bappend(b, c, 1);
  }
  bappend(b, "'", 1);
}

typedef struct {
  char *out;
  char *err;
  int status;
} result;

/* Runs cmd (logged to commands.txt), capturing stdout and stderr; with
   env_name set, that variable is set for the child. Returns 0 if it ran and
   exited 0; otherwise writes its stderr, sets d->error and returns -1. */
static int run(driver *d, const strvec *cmd, const char *what,
               const char *env_name, const char *env_value, result *r) {
  buffer line = {0};
  for (size_t i = 0; i < cmd->n; ++i) {
    if (i)
      bappend(&line, " ", 1);
    quote_into(&line, cmd->items[i]);
  }
  fprintf(d->commands, "%s\n", line.text ? line.text : "");
  fflush(d->commands);
  free(line.text);

  char *outp = kpath(d, ".stdout.XXXXXX"), *errp = kpath(d, ".stderr.XXXXXX");
  int ofd = mkstemp(outp), efd = mkstemp(errp);
  if (ofd < 0 || efd < 0) {
    snprintf(d->error, sizeof d->error, "cannot create temporary files in %s",
             d->a->keep_dir);
    return -1;
  }
  pid_t pid = fork();
  if (pid < 0) {
    snprintf(d->error, sizeof d->error, "cannot fork: %s", strerror(errno));
    return -1;
  }
  if (pid == 0) {
    dup2(ofd, 1);
    dup2(efd, 2);
    if (env_name)
      setenv(env_name, env_value, 1);
    char **argv = xmalloc((cmd->n + 1) * sizeof *argv);
    for (size_t i = 0; i < cmd->n; ++i)
      argv[i] = strdup(cmd->items[i]);
    argv[cmd->n] = NULL;
    execvp(argv[0], argv);
    fprintf(stderr, "cannot run %s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }
  int status;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
    ;
  close(ofd);
  close(efd);
  r->out = read_file(outp, NULL);
  r->err = read_file(errp, NULL);
  unlink(outp);
  unlink(errp);
  free(outp);
  free(errp);
  r->status = WIFEXITED(status) ? WEXITSTATUS(status) : 128;
  if (r->status != 0) {
    fputs(r->err ? r->err : "", stderr);
    snprintf(d->error, sizeof d->error, "%s failed (exit %d): %s", what,
             r->status, cmd->items[0]);
    return -1;
  }
  return 0;
}

static strvec argv_of(const char *first, ...) {
  strvec v = {0};
  va_list ap;
  va_start(ap, first);
  for (const char *s = first; s; s = va_arg(ap, const char *))
    push(&v, s);
  va_end(ap);
  return v;
}

/* ----------------------------------------------------- reading the results */

/* func symbol -> number of systolic.matmul_tile ops in its body. */
static long tiles_of(const char *mlir, const char *symbol) {
  long count = -1;
  int current = 0;
  for (const char *line = mlir; line && *line;) {
    const char *end = strchr(line, '\n');
    size_t len = end ? (size_t)(end - line) : strlen(line);
    const char *p = line;
    while (p < line + len && isspace((unsigned char)*p))
      ++p;
    if ((size_t)(line + len - p) > 10 && strncmp(p, "func.func ", 10) == 0) {
      const char *q = p + 10;
      /* an optional visibility word */
      const char *w = q;
      while (w < line + len && (isalnum((unsigned char)*w) || *w == '_'))
        ++w;
      if (w > q && w < line + len && *w == ' ')
        q = w + 1;
      if (q < line + len && *q == '@') {
        const char *n = q + 1, *paren = n;
        while (paren < line + len && *paren != '(' &&
               !isspace((unsigned char)*paren))
          ++paren;
        current = paren < line + len && *paren == '(' &&
                  (size_t)(paren - n) == strlen(symbol) &&
                  strncmp(n, symbol, strlen(symbol)) == 0;
        if (current && count < 0)
          count = 0;
      }
    } else if (current && memmem(line, len, "systolic.matmul_tile", 20)) {
      ++count;
    }
    line = end ? end + 1 : NULL;
  }
  return count < 0 ? 0 : count;
}

/* The pass's own reason for a function's nests: the first "not lowered"
   remark whose line lies in the function's definition. */
static char *remark_reason(const char *remarks, long first, long last) {
  static const char marker[] =
      ": remark: affine GEMM not lowered to systolic.matmul_tile: ";
  for (const char *m = strstr(remarks, marker); m; m = strstr(m + 1, marker)) {
    /* ":LINE:COL" right before the marker */
    const char *p = m;
    const char *col_end = p;
    while (p > remarks && isdigit((unsigned char)p[-1]))
      --p;
    if (p == col_end || p == remarks || p[-1] != ':')
      continue;
    --p;
    const char *line_end = p;
    while (p > remarks && isdigit((unsigned char)p[-1]))
      --p;
    if (p == line_end || p == remarks || p[-1] != ':')
      continue;
    long line = strtol(p, NULL, 10);
    if (line < first || line > last)
      continue;
    const char *why = m + sizeof marker - 1;
    const char *eol = strchr(why, '\n');
    int n = eol ? (int)(eol - why) : (int)strlen(why);
    return format(" (line %ld: %.*s)", line, n, why);
  }
  return format("%s", "");
}

/* ------------------------------------------------------ the C-side pieces */

/* A GEMM loop nest of the program (systolic-cfront --gemm-sites): it gets a
   "GEMM #N" report each time it runs -- `executions` times per call of its
   function (the loops around it; -1 unknown). `lowered` and `regions`: the
   existing pass lowered it to that many systolic.matmul_tile invocations,
   which the loops enclosing the tiles run `repeat` times per call (-1
   unknown) -- from its "lowered as R exact region(s)[; the enclosing loops
   run it N time(s)]" remark at the nest's line:col. */
typedef struct {
  const char *function;
  long line, col;
  long long begin, end;
  int from_macro;
  long long executions;
  int lowered;
  long regions;
  long long repeat;
} gemm_site;

typedef struct {
  const json_value *fn;
  const char *name, *symbol;
  long line, col, end_line;
} function_info;

/* The program with each offloaded function's definition renamed to
   NAME__systolic_reference, a prototype of NAME before it, and #line
   directives so that diagnostics keep pointing at the source. */
typedef struct {
  long long start, end;
  char *text;
} edit;

static int edit_order(const void *x, const void *y) {
  const edit *a = x, *b = y;
  if (a->start != b->start)
    return a->start < b->start ? 1 : -1;
  if (a->end != b->end)
    return a->end < b->end ? 1 : -1;
  return 0;
}

/* The offset just past a statement that ends at `end`: past a ';' that
   follows it (an expression body's range ends before its ';'), skipping
   blanks and comments; else `end`. */
static long long statement_end(const char *data, size_t length,
                               long long end) {
  long long p = end;
  while ((size_t)p < length) {
    if (isspace((unsigned char)data[p])) {
      ++p;
    } else if (data[p] == '/' && (size_t)p + 1 < length && data[p + 1] == '*') {
      const char *close = strstr(data + p + 2, "*/");
      if (!close)
        return end;
      p = close - data + 2;
    } else if (data[p] == '/' && (size_t)p + 1 < length && data[p + 1] == '/') {
      while ((size_t)p < length && data[p] != '\n')
        ++p;
    } else {
      break;
    }
  }
  return (size_t)p < length && data[p] == ';' ? p + 1 : end;
}

static char *rewrite_host(const char *data, size_t length, const char *src,
                          const function_info *offload, size_t n,
                          const gemm_site *sites, size_t nsites) {
  edit *edits = xmalloc((2 * n + 2 * nsites + 1) * sizeof *edits);
  size_t ne = 0;
  /* Every GEMM loop nest reports itself after it has run on the CPU:
     { <nest> __systolic_cc_gemm_executed(0); }. No newline is inserted, so
     line numbers do not move. (In an offloaded function this is the
     NAME__systolic_reference copy: the CPU fallback reports itself.) */
  for (size_t i = 0; i < nsites; ++i) {
    if (sites[i].from_macro || sites[i].begin < 0 || sites[i].end <= 0)
      continue;
    long long after = statement_end(data, length, sites[i].end);
    edits[ne++] = (edit){sites[i].begin, sites[i].begin, format("{ ")};
    edits[ne++] = (edit){after, after,
                         format(" __systolic_cc_gemm_executed(0); }")};
  }
  for (size_t i = 0; i < n; ++i) {
    const json_value *fn = offload[i].fn;
    long long begin = json_int(json_get(fn, "def_begin"), 0);
    long long body = json_int(json_get(fn, "body_begin"), begin);
    long long name_at = json_int(json_get(fn, "name_offset"), 0);
    long long name_len = json_int(json_get(fn, "name_length"), 0);
    long long proto_end = body;
    while (proto_end > begin && isspace((unsigned char)data[proto_end - 1]))
      --proto_end;
    long line = 1;
    for (long long k = 0; k < begin; ++k)
      if (data[k] == '\n')
        ++line;
    long long sol = begin;
    while (sol > 0 && data[sol - 1] != '\n')
      --sol;
    long long col = begin - sol;
    buffer ins = {0};
    bprintf(&ins, "\n%.*s;\n#line %ld \"%s\"\n", (int)(proto_end - begin),
            data + begin, line, src);
    for (long long c = 0; c < col; ++c)
      bappend(&ins, " ", 1);
    edits[ne++] = (edit){begin, begin, ins.text};
    edits[ne++] = (edit){name_at, name_at + name_len,
                         format("%s%s", offload[i].name, REFERENCE_SUFFIX)};
  }
  qsort(edits, ne, sizeof *edits, edit_order);
  /* Apply from the end, so earlier offsets stay valid. */
  char *text = xmalloc(length + 1);
  memcpy(text, data, length);
  text[length] = 0;
  size_t tlen = length;
  for (size_t e = 0; e < ne; ++e) {
    size_t nl = strlen(edits[e].text);
    size_t keep_tail = tlen - (size_t)edits[e].end;
    char *next = xmalloc((size_t)edits[e].start + nl + keep_tail + 1);
    memcpy(next, text, (size_t)edits[e].start);
    memcpy(next + edits[e].start, edits[e].text, nl);
    memcpy(next + edits[e].start + nl, text + edits[e].end, keep_tail);
    tlen = (size_t)edits[e].start + nl + keep_tail;
    next[tlen] = 0;
    free(text);
    text = next;
  }
  char *out = format("void __systolic_cc_gemm_executed(int);\n"
                     "#line 1 \"%s\"\n%s", src, text);
  free(text);
  return out;
}

/* A C declaration of `name` with the type clang printed. */
static char *declarator(const char *ctype, const char *name) {
  if (strstr(ctype, "(*")) {
    const char *close = strchr(ctype, ')');
    return format("%.*s %s%s", (int)(close - ctype), ctype, name, close);
  }
  return format("%s %s", ctype, name);
}

static const char *c_type(const char *elem) {
  return strcmp(elem, "f32") == 0 ? "float" : "double";
}

/* The shortest decimal that reads back as v (what Python's repr gives). */
static char *shortest(double v) {
  for (int p = 1; p <= 17; ++p) {
    char *s = format("%.*g", p, v);
    if (strtod(s, NULL) == v)
      return s;
    free(s);
  }
  return format("%.17g", v);
}

static long long element_count(const json_value *p) {
  long long count = json_int(json_get(p, "leading_extent"), 0);
  const json_value *dims = json_get(p, "inner_dims");
  for (size_t d = 0; d < json_size(dims); ++d)
    count *= json_int(json_at(dims, d), 1);
  return count;
}

static int is_written_memref(const json_value *p) {
  return strcmp(json_string(json_get(p, "kind"), ""), "memref") == 0 &&
         json_bool(json_get(p, "written"), 0);
}

/* Element count and C type of a written array parameter. */
static void array_desc(const json_value *p, long long *count, const char **ct) {
  *count = element_count(p);
  *ct = c_type(json_string(json_get(p, "elem"), "f32"));
}

/* The leading extent of an array parameter as a C expression, evaluable in
   the shim where the parameters are in scope: the constant the front end
   computed, or -- when a subscript uses a parameter (a dynamic offset) --
   the expression of the parameters it wrote (leading_extent_expr). */
static char *leading_extent_expr(const json_value *p) {
  const char *e = json_string(json_get(p, "leading_extent_expr"), NULL);
  if (e)
    return format("((long long)%s)", e);
  return format("%lldLL", json_int(json_get(p, "leading_extent"), 0));
}

/* Element count of a written array parameter, as a C expression. */
static char *count_expr(const json_value *p) {
  char *lead = leading_extent_expr(p);
  const json_value *dims = json_get(p, "inner_dims");
  long long inner = 1;
  for (size_t d = 0; d < json_size(dims); ++d)
    inner *= json_int(json_at(dims, d), 1);
  char *r = format("(%s * %lldLL)", lead, inner);
  free(lead);
  return r;
}

/* The C entry point of each offloaded function NAME.
 *
 *   0. A call whose integer arguments make an access reach a row before the
 *      first element an array argument points to (the run-time minimum row,
 *      leading_min_expr, below 0 -- legal C on a pointer into an array) is
 *      not offloaded: the kernel's views, the snapshot, the undo and the
 *      check below all start at that element, so the call goes straight to
 *      step 5 without a snapshot and runs as written on the CPU. Evaluated
 *      before any local of the shim, which could shadow a parameter.
 *   1. Snapshot every array NAME writes (the exact rows its accesses reach).
 *      Without the snapshot (malloc failed) the FPGA is not tried.
 *   2. Run the compiled kernel; its invocations go through the runtime.
 *   3. The FPGA ran NAME iff the runtime is the hardware one
 *      (is_hardware), none of the kernel's invocations failed (every one
 *      returned 0: operand slab written, descriptor accepted, result tile
 *      read back in full and added into C), and the kernel made exactly the
 *      invocations its plan has. Then each lowered GEMM nest reports "FPGA
 *      Executed", and any other GEMM nest of NAME -- run by the kernel's own
 *      loops on the CPU -- "CPU Executed".
 *   4. Completed by a runtime that is not the hardware (the software model:
 *      the tiles were computed on the CPU): every nest reports "CPU
 *      Executed"; the result is kept.
 *   5. Otherwise (an invocation failed, or the count is off) the kernel's
 *      writes are undone from the snapshot -- C may hold partial tile sums --
 *      and NAME__systolic_reference, the function as written, runs on the
 *      original arguments: the CPU computes NAME exactly once, from C's
 *      original values, and its GEMM nests report "CPU Executed" themselves.
 *
 * With --verify (testing) the result is compared with
 * NAME__systolic_reference run silently on copies of the original arrays.
 *
 * NAME's parameters are in scope in its shim, and the extents and counts are
 * expressions of them: every local and helper of the shim carries the
 * implementation's reserved prefix __systolic_cc_ (arrays by parameter
 * index, never by the user's name), so none shadows a parameter or is
 * shadowed by one, and each array's element count is evaluated once at the
 * top. */
static char *shim_source(const function_info *offload, size_t n, int verify,
                         double rtol, const char *source,
                         const gemm_site *sites, size_t nsites) {
  buffer b = {0};
  bprintf(&b, "/* Generated by systolic-cc: the C entry points of the "
              "functions it offloaded\n * from %s. */\n", base_name(source));
  bprintf(&b, "#include \"systolic_offload.h\"\n#include <math.h>\n"
              "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
              "#include <string.h>\n\n");
  bprintf(&b, "void __systolic_cc_gemm_executed(int on_fpga);\n"
              "extern int __systolic_cc_quiet;\n\n");
  bprintf(&b, "static void __systolic_cc_close_runtime(void) { systolic_offload_close(); }\n"
              "static void __systolic_cc_once(void) {\n  static int done;\n"
              "  if (!done) { done = 1; atexit(__systolic_cc_close_runtime); }\n}\n\n");
  char *how_text = rtol <= 0 ? format("bitwise")
                             : format("relative tolerance %g", rtol);
  for (size_t f = 0; f < n; ++f) {
    const json_value *params = json_get(offload[f].fn, "params");
    size_t np = json_size(params);
    buffer decl = {0}, kargs = {0}, kdecl = {0};
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      const char *pname = json_string(json_get(p, "name"), "");
      const char *kind = json_string(json_get(p, "kind"), "");
      char *dcl = declarator(json_string(json_get(p, "ctype"), ""), pname);
      bprintf(&decl, "%s%s", i ? ", " : "", dcl);
      free(dcl);
      if (strcmp(kind, "memref") == 0) {
        const char *ct = c_type(json_string(json_get(p, "elem"), "f32"));
        const json_value *dims = json_get(p, "inner_dims");
        size_t rank = 1 + json_size(dims);
        long long sizes[16], strides[16];
        sizes[0] = json_int(json_get(p, "leading_extent"), 0);
        for (size_t d = 1; d < rank; ++d)
          sizes[d] = json_int(json_at(dims, d - 1), 1);
        long long acc = 1;
        for (size_t d = rank; d-- > 1;) {
          strides[d] = acc;
          acc *= sizes[d];
        }
        strides[0] = acc;
        for (int twice = 0; twice < 2; ++twice) {
          bprintf(&kargs, "%s(%s *)%s", kargs.n ? ", " : "", ct, pname);
          bprintf(&kdecl, "%s%s *", kdecl.n ? ", " : "", ct);
        }
        bprintf(&kargs, ", 0");
        bprintf(&kdecl, ", int64_t");
        {
          char *lead = leading_extent_expr(p);
          bprintf(&kargs, ", %s", lead);
          bprintf(&kdecl, ", int64_t");
          free(lead);
        }
        for (size_t d = 1; d < rank; ++d) {
          bprintf(&kargs, ", %lld", sizes[d]);
          bprintf(&kdecl, ", int64_t");
        }
        for (size_t d = 0; d < rank; ++d) {
          bprintf(&kargs, ", %lld", strides[d]);
          bprintf(&kdecl, ", int64_t");
        }
      } else if (strcmp(kind, "scalar") == 0) {
        bprintf(&kargs, "%s%s", kargs.n ? ", " : "", pname);
        bprintf(&kdecl, "%s%s", kdecl.n ? ", " : "",
                c_type(json_string(json_get(p, "elem"), "f32")));
      } else {
        /* An integer parameter: i32 / i64 as the front end typed it. */
        const char *ctype = json_string(json_get(p, "ctype"), "int");
        bprintf(&kargs, "%s%s", kargs.n ? ", " : "", pname);
        bprintf(&kdecl, "%s%s", kdecl.n ? ", " : "",
                strstr(ctype, "long") ? "long long" : "int");
      }
    }
    const char *name = offload[f].name, *symbol = offload[f].symbol;
    char *where = format("%s:%ld", base_name(source), offload[f].line);
    const char *decl_text = decl.n ? decl.text : "void";

    /* NAME's GEMM nests, in source order, each reported once per
       execution; and the invocations one call makes: every lowered nest's
       regions, as many times as the loops around its tiles run them. */
    buffer labels = {0};
    long long expected = 0;
    int count_known = 1;
    for (size_t s = 0; s < nsites; ++s) {
      if (strcmp(sites[s].function, name) != 0)
        continue;
      if (sites[s].lowered) {
        if (sites[s].repeat < 0)
          count_known = 0;
        else
          expected += sites[s].regions * sites[s].repeat;
      }
      long long times = sites[s].executions < 0 ? 1 : sites[s].executions;
      if (times == 1)
        bprintf(&labels, "    __systolic_cc_gemm_executed(__systolic_cc_hw && %d);\n",
                sites[s].lowered);
      else
        bprintf(&labels, "    for (long long __systolic_cc_n = 0; __systolic_cc_n < %lldLL; "
                         "++__systolic_cc_n)\n"
                         "      __systolic_cc_gemm_executed(__systolic_cc_hw && %d);\n",
                times, sites[s].lowered);
    }

    /* Step 0: the run-time minimum row of every array (read or written)
       whose subscripts use a parameter. */
    buffer guard = {0};
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      const char *min = json_string(json_get(p, "leading_min_expr"), NULL);
      if (min && strcmp(json_string(json_get(p, "kind"), ""), "memref") == 0)
        bprintf(&guard, "%s(long long)(%s) < 0LL", guard.n ? " || " : "", min);
    }

    bprintf(&b, "void %s(%s);\n", symbol, kdecl.n ? kdecl.text : "void");
    bprintf(&b, "void %s%s(%s);\n", name, REFERENCE_SUFFIX, decl_text);
    bprintf(&b, "void %s(%s) {\n", name, decl_text);
    bprintf(&b, "  __systolic_cc_once();\n");
    if (guard.n)
      bprintf(&b, "  const int __systolic_cc_rows_ok = !(%s);\n", guard.text);
    /* The element count of each written array, from the parameters. */
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      if (!is_written_memref(p))
        continue;
      char *n = count_expr(p);
      bprintf(&b, "  const long long __systolic_cc_count%zu = %s;\n", i, n);
      free(n);
    }
    bprintf(&b, "  int __systolic_cc_snapshots = %s;\n",
            guard.n ? "__systolic_cc_rows_ok" : "1");
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      if (!is_written_memref(p))
        continue;
      const char *ct, *pname = json_string(json_get(p, "name"), "");
      long long count;
      array_desc(p, &count, &ct);
      if (guard.n)
        bprintf(&b, "  %s *__systolic_cc_save%zu = __systolic_cc_snapshots ? "
                    "malloc(__systolic_cc_count%zu * sizeof(%s) + 1) : NULL;\n",
                ct, i, i, ct);
      else
        bprintf(&b, "  %s *__systolic_cc_save%zu = "
                    "malloc(__systolic_cc_count%zu * sizeof(%s) + 1);\n",
                ct, i, i, ct);
      bprintf(&b, "  if (__systolic_cc_save%zu) memcpy(__systolic_cc_save%zu, %s, "
                  "__systolic_cc_count%zu * sizeof(%s)); "
                  "else __systolic_cc_snapshots = 0;\n", i, i, pname, i, ct);
    }
    bprintf(&b, "  int __systolic_cc_completed = 0, __systolic_cc_hw = 0;\n");
    bprintf(&b, "  if (__systolic_cc_snapshots) {\n"
                "    systolic_offload_stats_t __systolic_cc_before, __systolic_cc_after;\n"
                "    systolic_offload_stats(&__systolic_cc_before);\n"
                "    %s(%s);\n"
                "    systolic_offload_stats(&__systolic_cc_after);\n"
                "    unsigned long long __systolic_cc_inv = 0;\n"
                "    for (int __systolic_cc_d = 0; __systolic_cc_d < 4; ++__systolic_cc_d)\n"
                "      __systolic_cc_inv += __systolic_cc_after.per_device[__systolic_cc_d] -\n"
                "                           __systolic_cc_before.per_device[__systolic_cc_d];\n"
                "    __systolic_cc_completed =\n"
                "        __systolic_cc_after.failed == __systolic_cc_before.failed && %s;\n"
                "    __systolic_cc_hw = __systolic_cc_after.is_hardware;\n"
                "  }\n", symbol, kargs.n ? kargs.text : "",
                count_known ? format("__systolic_cc_inv == %lldULL", expected)
                            : format("%s", "1 /* invocation count unknown */"));
    bprintf(&b, "  if (__systolic_cc_completed) {\n%s  } else {\n",
            labels.n ? labels.text : "");
    /* Undo whatever the kernel wrote, then run the function as written. */
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      if (!is_written_memref(p))
        continue;
      const char *ct, *pname = json_string(json_get(p, "name"), "");
      long long count;
      array_desc(p, &count, &ct);
      bprintf(&b, "    if (__systolic_cc_snapshots) memcpy(%s, __systolic_cc_save%zu, "
                  "__systolic_cc_count%zu * sizeof(%s));\n", pname, i, i, ct);
    }
    bprintf(&b, "    %s%s(", name, REFERENCE_SUFFIX);
    for (size_t i = 0; i < np; ++i)
      bprintf(&b, "%s%s", i ? ", " : "",
              json_string(json_get(json_at(params, i), "name"), ""));
    bprintf(&b, ");\n  }\n");

    if (verify) {
      /* The reference on copies of the original arrays, silently. */
      bprintf(&b, "  if (__systolic_cc_snapshots) {\n");
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        if (!is_written_memref(p))
          continue;
        const char *ct;
        long long count;
        array_desc(p, &count, &ct);
        bprintf(&b, "    %s *__systolic_cc_ref%zu = malloc(__systolic_cc_count%zu * "
                    "sizeof(%s) + 1);\n", ct, i, i, ct);
        bprintf(&b, "    memcpy(__systolic_cc_ref%zu, __systolic_cc_save%zu, "
                    "__systolic_cc_count%zu * sizeof(%s));\n", i, i, i, ct);
      }
      bprintf(&b, "    __systolic_cc_quiet = 1;\n    %s%s(", name,
              REFERENCE_SUFFIX);
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        const char *pname = json_string(json_get(p, "name"), "");
        if (is_written_memref(p))
          bprintf(&b, "%s(void *)__systolic_cc_ref%zu", i ? ", " : "", i);
        else
          bprintf(&b, "%s%s", i ? ", " : "", pname);
      }
      bprintf(&b, ");\n    __systolic_cc_quiet = 0;\n"
                  "    unsigned long long __systolic_cc_checked = 0, __systolic_cc_bad = 0;\n");
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        if (!is_written_memref(p))
          continue;
        const char *ct, *pname = json_string(json_get(p, "name"), "");
        long long count;
        array_desc(p, &count, &ct);
        bprintf(&b, "    for (long long __systolic_cc_e = 0; __systolic_cc_e < "
                    "__systolic_cc_count%zu; ++__systolic_cc_e) {\n", i);
        bprintf(&b, "      %s __systolic_cc_got = ((%s *)%s)[__systolic_cc_e],\n"
                    "         __systolic_cc_want = __systolic_cc_ref%zu[__systolic_cc_e];\n",
                ct, ct, pname, i);
        if (rtol > 0) {
          char *r = shortest(rtol);
          bprintf(&b, "      int __systolic_cc_same =\n"
                      "          fabs((double)__systolic_cc_got - __systolic_cc_want) <= %s *\n"
                      "          fmax(fabs((double)__systolic_cc_got), "
                      "fabs((double)__systolic_cc_want));\n", r);
          free(r);
        } else {
          bprintf(&b, "      int __systolic_cc_same = memcmp(&__systolic_cc_got, "
                      "&__systolic_cc_want, sizeof __systolic_cc_got) == 0;\n");
        }
        bprintf(&b, "      ++__systolic_cc_checked;\n"
                    "      if (!__systolic_cc_same && __systolic_cc_bad++ == 0)\n");
        bprintf(&b, "        fprintf(stderr, \"systolic-cc: %s (%s): first "
                    "mismatch %s[%%lld]: result %%.9g, CPU reference "
                    "%%.9g\\n\", __systolic_cc_e, (double)__systolic_cc_got, "
                    "(double)__systolic_cc_want);\n", name, where, pname);
        bprintf(&b, "    }\n    free(__systolic_cc_ref%zu);\n", i);
      }
      bprintf(&b, "    if (__systolic_cc_bad) {\n");
      bprintf(&b, "      fprintf(stderr, \"systolic-cc: %s (%s): NUMERICAL "
                  "MISMATCH: %%llu of %%llu elements differ from the CPU "
                  "reference (%s)\\n\", __systolic_cc_bad, __systolic_cc_checked);\n",
              name, where, how_text);
      bprintf(&b, "      exit(4);\n    }\n");
      bprintf(&b, "    fprintf(stderr, \"systolic-cc: %s (%s): verified: %%llu "
                  "elements equal the CPU reference (%s)\\n\", __systolic_cc_checked);\n",
              name, where, how_text);
      bprintf(&b, "  }\n");
      if (guard.n)
        bprintf(&b, "  else if (!__systolic_cc_rows_ok)\n"
                    "    fprintf(stderr, \"systolic-cc: %s (%s): ran on the CPU: at "
                    "these arguments an access reaches a row before the first "
                    "element of an argument array; nothing to compare\\n\");\n",
                name, where);
    }
    for (size_t i = 0; i < np; ++i) {
      const json_value *p = json_at(params, i);
      if (is_written_memref(p))
        bprintf(&b, "  free(__systolic_cc_save%zu);\n", i);
    }
    bprintf(&b, "}\n\n");
    free(guard.text);
    free(where);
    free(decl.text);
    free(kargs.text);
    free(kdecl.text);
    free(labels.text);
  }
  free(how_text);
  return b.text ? b.text : format("%s", "");
}

/* --------------------------------------------------------------- compile */

/* The fleet the kernels are compiled for: K_MAX_8X8 / K_MAX_4X4, and whether
   the bitstream reports completions (COMPLETION_HEADER=1), which decides the
   runtime the kernels call:
     asynchronous  fpga_gemm_begin, one fpga_tile_submit per tile, one
                   fpga_gemm_wait per GEMM (runtime/fpga_async_runtime.h):
                   every tile submitted without waiting for the one before
                   it, PIPELINE_DEPTH descriptors in flight per array, each
                   completion record matched to its tile. The default for
                   --runtime board on such a bitstream (--submission auto).
     synchronous   one fpga_matmul_tiled_auto_scheduled per tile, each
                   waited for before the next: the software model (unless
                   --submission async, which it implements too), a
                   bitstream without the record, or --submission sync. */
static int fleet_config(driver *d, long *k8, long *k4, int *header,
                        long *depth) {
  const options *a = d->a;
  *header = 0;
  /* PIPELINE_DEPTH of the configuration (the context's job queue, 1 when
     not recorded: the archived bitstream, the software model): the fleet is
     declared pipelined when it is more than 1, as systolic-matmul declares
     it from the same record, so the schedule describes the loaded hardware
     whatever the submission mode is. */
  *depth = 1;
  if (strcmp(a->runtime, "board") != 0) {
    *k8 = a->k_max_8x8;
    *k4 = a->k_max_4x4;
    return 0;
  }
  const char *config = a->fpga_config ? a->fpga_config
                                      : getenv("SYSTOLIC_FPGA_CONFIG");
  if (!config) {
    /* The runtime's own default without SYSTOLIC_FPGA_CONFIG: the archived
       1x8x8 + 3x4x4 bitstream, K_MAX 16 / 16 (runtime/fpga_matmul_tiled.h),
       no completion record, so the synchronous lowering. A bitstream that
       sends the record (the current RTL) is refused at the first result
       (-9), and every GEMM then runs on the CPU; a deeper K is refused by
       the runtime (-10). Name the bitstream's fpga_config.txt to use it. */
    *k8 = 16;
    *k4 = 16;
    return 0;
  }
  char *rt = format("%s/bin/runtime-dispatch-test", a->build_dir);
  strvec cmd = argv_of(rt, "--print-config", NULL);
  result r;
  if (run(d, &cmd, "runtime-dispatch-test --print-config",
          "SYSTOLIC_FPGA_CONFIG", config, &r) != 0) {
    snprintf(d->error, sizeof d->error, "the runtime refuses %s: %s%s",
             config, r.out ? r.out : "", r.err ? r.err : "");
    return -1;
  }
  const char *p8 = strstr(r.out, "K_MAX_8X8="), *p4 = strstr(r.out, "K_MAX_4X4=");
  if (!p8 || !p4) {
    snprintf(d->error, sizeof d->error, "the runtime refuses %s: %s", config,
             r.out);
    return -1;
  }
  *k8 = strtol(p8 + 10, NULL, 10);
  *k4 = strtol(p4 + 10, NULL, 10);
  const char *ph = strstr(r.out, "COMPLETION_HEADER=");
  *header = ph && ph[18] == '1';
  const char *pd = strstr(r.out, "PIPELINE_DEPTH=");
  if (pd) {
    char *end = NULL;
    long v = strtol(pd + 15, &end, 10);
    if (end == pd + 15 || v < 1) {
      snprintf(d->error, sizeof d->error,
               "the runtime reports an unusable PIPELINE_DEPTH for %s: %s",
               config, r.out);
      return -1;
    }
    *depth = v;
  }
  return 0;
}

/* Whether the kernels call the asynchronous runtime (see fleet_config). */
static int choose_async(driver *d, int header) {
  const options *a = d->a;
  const int board = strcmp(a->runtime, "board") == 0;
  if (strcmp(a->submission, "sync") == 0)
    return 0;
  if (strcmp(a->submission, "async") == 0) {
    /* The software model implements the asynchronous API too (a queue run
       at polls and waits), so a program's asynchronous lowering can be
       checked without the board; it is not its default, which keeps the
       model's one-call-one-invocation fault semantics for --runtime soft. */
    if (!board)
      return 1;
    if (!header) {
      snprintf(d->error, sizeof d->error,
               "--submission async needs a bitstream with the completion "
               "record (COMPLETION_HEADER=1 in the configuration); %s has "
               "none",
               a->fpga_config ? a->fpga_config
               : getenv("SYSTOLIC_FPGA_CONFIG") ? getenv("SYSTOLIC_FPGA_CONFIG")
               : "the default configuration");
      return -1;
    }
    return 1;
  }
  return board && header;
}

/* The flags every compile of the user's source gets (the front end's two
   passes and the host compile): the user's -D / -I, then the offload
   runtime's headers (systolic_offload.h), which the program is linked with
   -- from this build's source tree, never from the working directory. */
static void add_flags(strvec *v, const options *a) {
  for (size_t i = 0; i < a->defines.n; ++i)
    push(v, format("-D%s", a->defines.items[i]));
  for (size_t i = 0; i < a->includes.n; ++i)
    push(v, format("-I%s", a->includes.items[i]));
  push(v, format("-I%s/runtime/offload", SYSTOLIC_SOURCE_DIR));
}

/* The front end's flags: the permission to reassociate floating-point sums,
   as fastmath<reassoc> on its arith ops (docs/AFFINE_GEMM_MAPPING.md
   section 6). Never given to the host compile: the program as written runs
   there unchanged. */
static void add_frontend_flags(strvec *v, const options *a) {
  add_flags(v, a);
  if (a->fp_reassociation)
    push(v, "--fp-reassociation");
}

static int compile(driver *d) {
  const options *a = d->a;
  char *opt = format("%s/bin/systolic-opt", a->build_dir);
  char *cfront = format("%s/systolic-cfront", a->self_dir);
  char *offload_rt = format("%s/runtime/offload", SYSTOLIC_SOURCE_DIR);
  long k8, k4, depth;
  int header;
  if (fleet_config(d, &k8, &k4, &header, &depth) != 0)
    return -1;
  const int async = choose_async(d, header);
  if (async < 0)
    return -1;
  char *fleet = format("--systolic-declare-fpga-fleet=k-max-8x8=%ld "
                       "k-max-4x4=%ld pipeline-depth=%ld", k8, k4, depth);
  char *phys = format("--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=%ld "
                      "phys-k-max-4x4=%ld%s", k8, k4,
                      async ? " async=true" : "");

  char src[PATH_MAX];
  if (!realpath(a->source, src)) {
    snprintf(d->error, sizeof d->error, "cannot read %s: %s", a->source,
             strerror(errno));
    return -1;
  }
  size_t src_len;
  char *src_text = read_file(src, &src_len);
  if (!src_text || !write_file(kpath(d, "00-source.c"), src_text, src_len)) {
    snprintf(d->error, sizeof d->error, "cannot copy %s", src);
    return -1;
  }

  /* 1. C -> MLIR for every function in the subset. */
  strvec cmd = argv_of(cfront, "--clang", a->clang, format("-std=%s", a->std),
                       "--skip-unsupported", "--symbol-suffix=" KERNEL_SUFFIX,
                       "--interface", kpath(d, "10-interface.json"),
                       "--gemm-sites", kpath(d, "10-gemm-sites.json"), src, "-o",
                       kpath(d, "10-frontend.mlir"), NULL);
  add_frontend_flags(&cmd, a);
  result r;
  if (run(d, &cmd, "the C front end", NULL, NULL, &r) != 0)
    return -1;
  if (explain)
    fputs(r.err, stderr);   /* its notes: functions outside the subset */

  /* The program's GEMM loop nests. */
  size_t slen;
  char *stext = read_file(kpath(d, "10-gemm-sites.json"), &slen);
  char serr[128];
  json_value *sitedoc = stext ? json_parse(stext, slen, serr) : NULL;
  if (!sitedoc) {
    snprintf(d->error, sizeof d->error, "cannot read the GEMM sites: %s",
             stext ? serr : "missing");
    return -1;
  }
  const json_value *sitelist = json_get(sitedoc, "sites");
  size_t nsites = json_size(sitelist), sites_cap = nsites + 16;
  gemm_site *sites = xmalloc(sites_cap * sizeof *sites);
  for (size_t i = 0; i < nsites; ++i) {
    const json_value *x = json_at(sitelist, i);
    sites[i] = (gemm_site){json_string(json_get(x, "function"), ""),
                           (long)json_int(json_get(x, "line"), 0),
                           (long)json_int(json_get(x, "col"), 0),
                           json_int(json_get(x, "begin"), -1),
                           json_int(json_get(x, "end"), -1),
                           json_bool(json_get(x, "from_macro"), 0),
                           json_int(json_get(x, "executions"), 1), 0, 0, 1};
  }
  size_t ilen;
  char *itext = read_file(kpath(d, "10-interface.json"), &ilen);
  char jerr[128];
  json_value *interface = itext ? json_parse(itext, ilen, jerr) : NULL;
  if (!interface) {
    snprintf(d->error, sizeof d->error, "cannot read the front end's "
             "interface: %s", itext ? jerr : "missing");
    return -1;
  }
  const json_value *fns = json_get(interface, "functions");
  size_t nfns = json_size(fns);

  /* 2. Which of them hold a GEMM the existing pass lowers. */
  function_info *offload = xmalloc((nfns ? nfns : 1) * sizeof *offload);
  size_t noffload = 0;
  strvec not_offloaded = {0};
  if (nfns) {
    strvec c2 = argv_of(opt, fleet,
                        format("--affine-matmul-to-systolic=report=true "
                               "cost-model=%s%s%s",
                               a->cost_model,
                               a->no_reduction_batches
                                   ? " reduction-batches=false"
                                   : "",
                               a->fp_distribute ? " fp-distribute=true" : ""),
                        kpath(d, "10-frontend.mlir"), "-o",
                        kpath(d, "20-systolic.mlir"), NULL);
    if (run(d, &c2, "GEMM recognition / Affine Partitioning", NULL, NULL,
            &r) != 0)
      return -1;
    const char *remarks = r.err;
    write_file(kpath(d, "20-remarks.txt"), remarks, strlen(remarks));
    /* "FILE:L:C: remark: affine GEMM MxNxK lowered as R exact region(s)":
       the nest at L:C runs as R systolic.matmul_tile invocations. */
    static const char lowered_mark[] = ": remark: affine GEMM ";
    for (const char *m = strstr(remarks, lowered_mark); m;
         m = strstr(m + 1, lowered_mark)) {
      const char *eol = strchr(m, '\n');
      const char *as = strstr(m, " lowered as ");
      if (!as || (eol && as > eol))
        continue;
      long regions = strtol(as + 12, NULL, 10);
      long long repeat = 1;
      const char *runs = strstr(as, "; the enclosing loops run it ");
      if (runs && (!eol || runs < eol)) {
        runs += strlen("; the enclosing loops run it ");
        repeat = isdigit((unsigned char)*runs) ? strtoll(runs, NULL, 10) : -1;
      }
      const char *p = m;
      while (p > remarks && isdigit((unsigned char)p[-1]))
        --p;
      if (p == m || p == remarks || p[-1] != ':')
        continue;
      long col = strtol(p, NULL, 10);
      const char *q = p - 1;
      while (q > remarks && isdigit((unsigned char)q[-1]))
        --q;
      if (q == p - 1)
        continue;
      long line = strtol(q, NULL, 10);
      int matched = 0;
      for (size_t i = 0; i < nsites; ++i)
        if (sites[i].line == line && sites[i].col == col) {
          sites[i].lowered = 1;
          sites[i].regions = regions;
          sites[i].repeat = repeat;
          matched = 1;
        }
      if (!matched) {
        /* A lowered nest the site finder did not report: still one GEMM. */
        const char *owner = "";
        for (size_t f = 0; f < nfns; ++f) {
          const json_value *fn = json_at(fns, f);
          if (line >= json_int(json_get(fn, "line"), 0) &&
              line <= json_int(json_get(fn, "end_line"), 0))
            owner = json_string(json_get(fn, "name"), "");
        }
        if (nsites == sites_cap) {
          gemm_site *more = xmalloc(2 * sites_cap * sizeof *sites);
          memcpy(more, sites, nsites * sizeof *sites);
          free(sites);
          sites = more;
          sites_cap *= 2;
        }
        sites[nsites++] =
            (gemm_site){owner, line, col, -1, -1, 1, 1, 1, regions, repeat};
      }
    }
    char *systolic = read_file(kpath(d, "20-systolic.mlir"), NULL);
    for (size_t i = 0; i < nfns; ++i) {
      const json_value *fn = json_at(fns, i);
      function_info info = {
          fn, json_string(json_get(fn, "name"), ""),
          json_string(json_get(fn, "symbol"), ""),
          (long)json_int(json_get(fn, "line"), 0),
          (long)json_int(json_get(fn, "col"), 0),
          (long)json_int(json_get(fn, "end_line"), json_int(json_get(fn, "line"), 0))};
      long n = tiles_of(systolic, info.symbol);
      const json_value *params = json_get(fn, "params");
      int reads_before = 0, uses_double = 0;
      for (size_t p = 0; p < json_size(params); ++p) {
        const json_value *pp = json_at(params, p);
        if (strcmp(json_string(json_get(pp, "kind"), ""), "memref") != 0)
          continue;
        if (json_int(json_get(pp, "leading_min"), 0) < 0)
          reads_before = 1;
        if (strcmp(json_string(json_get(pp, "elem"), ""), "f32") != 0)
          uses_double = 1;
      }
      char *why = NULL;
      if (n == 0) {
        char *reason = remark_reason(remarks, info.line, info.end_line);
        /* A GEMM that was recognized but may not be reassociated is not
           "no GEMM": say what is missing (docs/AFFINE_GEMM_MAPPING.md
           section 6). */
        if (strstr(reason, "floating-point reassociation is not permitted"))
          why = format("its GEMM is offloaded only with permission to "
                       "reassociate floating-point sums (--fp-reassociation)%s",
                       reason);
        else
          why = format("no GEMM the systolic arrays can run was recognized in "
                       "it%s", reason);
      } else if (strcmp(json_string(json_get(fn, "storage_class"), ""),
                        "static") == 0 ||
                 json_bool(json_get(fn, "inline"), 0)) {
        why = format("it is static or inline (offloading replaces an external "
                     "definition)");
      } else if (json_bool(json_get(fn, "from_macro"), 0)) {
        why = format("its definition comes from a macro expansion");
      } else if (reads_before) {
        why = format("it reads an array before its first element");
      } else if (uses_double) {
        why = format("the FPGA datapath is fp32 and it uses double arrays");
      }
      if (why) {
        log_note("%s:%ld:%ld: note: '%s' stays on the host: %s", a->source,
                 info.line, info.col, info.name, why);
        push(&not_offloaded, info.name);
      } else {
        /* Loops around a lowered nest repeat its tiles: what one call of
           the function makes. */
        long long per_call = 0;
        for (size_t s = 0; s < nsites; ++s)
          if (sites[s].lowered && strcmp(sites[s].function, info.name) == 0)
            per_call = per_call < 0 || sites[s].repeat < 0
                           ? -1 : per_call + sites[s].regions * sites[s].repeat;
        char *repeated =
            per_call == n ? format("%s", "")
            : per_call < 0
                ? format("%s", "; the loops around them repeat them an unknown "
                               "number of times")
                : format("; %lld invocation(s) per call (the loops around "
                         "them repeat them)", per_call);
        log_note("%s:%ld:%ld: note: '%s' is offloaded: %ld systolic tile "
                 "invocation(s) on the 1x8x8 + 3x4x4 fleet%s", a->source,
                 info.line, info.col, info.name, n, repeated);
        free(repeated);
        offload[noffload++] = info;
      }
    }
  }

  /* The plan, for the record. */
  {
    buffer plan = {0};
    bprintf(&plan, "{\n \"source\": \"%s\",\n \"runtime\": \"%s\",\n"
                   " \"submission\": \"%s\",\n"
                   " \"k_max_8x8\": %ld,\n \"k_max_4x4\": %ld,\n"
                   " \"pipeline_depth\": %ld,\n"
                   " \"verify\": %s,\n \"offloaded\": [", src, a->runtime,
            async ? "asynchronous" : "synchronous", k8, k4, depth,
            a->verify ? "true" : "false");
    for (size_t i = 0; i < noffload; ++i)
      bprintf(&plan, "%s\"%s\"", i ? ", " : "", offload[i].name);
    bprintf(&plan, "],\n \"translated_not_offloaded\": [");
    /* sorted, as the Python driver wrote them */
    for (size_t x = 0; x < not_offloaded.n; ++x)
      for (size_t y = x + 1; y < not_offloaded.n; ++y)
        if (strcmp(not_offloaded.items[y], not_offloaded.items[x]) < 0) {
          const char *t = not_offloaded.items[x];
          not_offloaded.items[x] = not_offloaded.items[y];
          not_offloaded.items[y] = t;
        }
    for (size_t i = 0; i < not_offloaded.n; ++i)
      bprintf(&plan, "%s\"%s\"", i ? ", " : "", not_offloaded.items[i]);
    bprintf(&plan, "]\n}\n");
    write_file(kpath(d, "25-plan.json"), plan.text, plan.n);
    free(plan.text);
  }

  strvec objects = {0};
  /* 3. The offloaded functions through the FPGA pipeline. */
  if (noffload) {
    strvec c3 = argv_of(cfront, "--clang", a->clang, format("-std=%s", a->std),
                        "--symbol-suffix=" KERNEL_SUFFIX, src, "-o",
                        kpath(d, "30-kernels.mlir"), NULL);
    add_frontend_flags(&c3, a);
    for (size_t i = 0; i < noffload; ++i)
      push(&c3, format("--function=%s", offload[i].name));
    if (run(d, &c3, "the C front end (offloaded functions)", NULL, NULL, &r))
      return -1;
    strvec c4 = argv_of(
        opt, fleet,
        format("--affine-matmul-to-systolic=cost-model=%s%s%s", a->cost_model,
               a->no_reduction_batches ? " reduction-batches=false" : "",
               a->fp_distribute ? " fp-distribute=true" : ""),
                        kpath(d, "30-kernels.mlir"), "-o",
                        kpath(d, "40-systolic.mlir"), NULL);
    if (run(d, &c4, "Affine -> Systolic", NULL, NULL, &r))
      return -1;
    strvec c5 = argv_of(opt, phys, kpath(d, "40-systolic.mlir"), "-o",
                        kpath(d, "50-fpga.mlir"), NULL);
    if (run(d, &c5, "FPGA lowering", NULL, NULL, &r))
      return -1;
    strvec c6 = argv_of(opt, "--systolic-runtime-to-llvm",
                        kpath(d, "50-fpga.mlir"), "-o",
                        kpath(d, "60-llvm.mlir"), NULL);
    if (run(d, &c6, "bufferization / LLVM dialect", NULL, NULL, &r))
      return -1;
    strvec c7 = argv_of(format("%s/bin/mlir-translate", a->llvm_build),
                        "--mlir-to-llvmir", kpath(d, "60-llvm.mlir"), "-o",
                        kpath(d, "70-kernels.ll"), NULL);
    if (run(d, &c7, "mlir-translate", NULL, NULL, &r))
      return -1;
    strvec c8 = argv_of(format("%s/bin/llc", a->llvm_build), "-O0",
                        "-filetype=obj", "-relocation-model=pic",
                        kpath(d, "70-kernels.ll"), "-o",
                        kpath(d, "70-kernels.o"), NULL);
    if (run(d, &c8, "llc", NULL, NULL, &r))
      return -1;
    push(&objects, kpath(d, "70-kernels.o"));
  }

  /* 4. The program itself, with the offloaded definitions renamed. */
  char *host = rewrite_host(src_text, src_len, src, offload, noffload, sites,
                            nsites);
  write_file(kpath(d, "80-host.c"), host, strlen(host));
  strvec c9 = argv_of(a->clang, format("-std=%s", a->std), a->opt_level, NULL);
  add_flags(&c9, a);
  push(&c9, format("-I%s", dir_name(src)));
  push(&c9, "-c");
  push(&c9, kpath(d, "80-host.c"));
  push(&c9, "-o");
  push(&c9, kpath(d, "80-host.o"));
  if (run(d, &c9, "compiling the program", NULL, NULL, &r))
    return -1;
  push(&objects, kpath(d, "80-host.o"));

  /* 5. The call shims. */
  if (noffload) {
    char *shim = shim_source(offload, noffload, a->verify, a->verify_rtol,
                             a->source, sites, nsites);
    write_file(kpath(d, "85-shim.c"), shim, strlen(shim));
    strvec c10 = argv_of(a->clang, "-std=c11", "-O1",
                         format("-I%s", offload_rt), "-c",
                         kpath(d, "85-shim.c"), "-o", kpath(d, "85-shim.o"),
                         NULL);
    if (run(d, &c10, "compiling the call shims", NULL, NULL, &r))
      return -1;
    push(&objects, kpath(d, "85-shim.o"));
  }

  /* 6. Link with a runtime. */
  strvec link = argv_of(a->clang, NULL);
  push_all(&link, &objects);
  push(&link, format("-I%s", offload_rt));
  push(&link, format("%s/systolic_cc_report.c", offload_rt));
  if (strcmp(a->runtime, "soft") == 0) {
    push(&link, format("%s/soft_runtime.c", offload_rt));
    push(&link, format("-DSOFT_DEFAULT_K_MAX_8X8=%ld", k8));
    push(&link, format("-DSOFT_DEFAULT_K_MAX_4X4=%ld", k4));
  } else {
    strvec pc = argv_of("pkg-config", "--libs", "libftdi1", NULL);
    if (run(d, &pc, "pkg-config libftdi1", NULL, NULL, &r))
      return -1;
    push(&link, format("%s/board_stats.c", offload_rt));
    push(&link, format("-I%s/runtime", SYSTOLIC_SOURCE_DIR));
    push(&link, format("%s/runtime/libsystolic_fpga_runtime.a", a->build_dir));
    for (char *tok = strtok(r.out, " \t\n"); tok; tok = strtok(NULL, " \t\n"))
      push(&link, tok);
    push(&link, "-lstdc++");
  }
  if (noffload) {
    char *libdir = format("%s/lib", a->llvm_build);
    push(&link, format("-L%s", libdir));
    push(&link, "-lmlir_c_runner_utils");
    push(&link, format("-Wl,-rpath,%s", libdir));
  }
  push(&link, "-lm");
  push(&link, "-o");
  push(&link, a->output);
  if (run(d, &link, "linking", NULL, NULL, &r))
    return -1;
  log_note("wrote %s (%zu function(s) offloaded, runtime: %s, %s, "
           "scheduled for %s contexts (PIPELINE_DEPTH=%ld); "
           "intermediates in %s)", a->output, noffload,
           strcmp(a->runtime, "soft") == 0 ? "software model, NOT the FPGA"
                                           : "FPGA board",
           async ? "asynchronous submission (fpga_tile_submit per tile, one "
                   "fpga_gemm_wait per GEMM)"
                 : "synchronous submission (one fpga_matmul_tiled_auto_"
                   "scheduled per tile)",
           depth > 1 ? "pipelined" : "serialized", depth, a->keep_dir);
  return 0;
}

/* ------------------------------------------------------------------- main */

static void usage(void) {
  fprintf(stderr,
          "usage: systolic-cc [--clang CLANG] [--build-dir DIR] "
          "[--llvm-build DIR] [--runtime soft|board] [--fpga-config FILE] "
          "[--submission auto|sync|async] [--cost-model v1|v2] "
          "[--k-max-8x8 N] [--k-max-4x4 N] [--verify] [--verify-rtol R] "
          "[--keep-dir DIR] [--explain] [--fp-reassociation] "
          "[--no-reduction-batches] [--fp-distribute] [-DNAME[=VAL]] "
          "[-IDIR] [-std=STD] "
          "[-ON] FILE.c "
          "-o PROGRAM\n");
}

static const char *option_value(const char *arg, const char *name, int *i,
                                int argc, char **argv) {
  size_t n = strlen(name);
  if (strncmp(arg, name, n) != 0)
    return NULL;
  if (arg[n] == '=')
    return arg + n + 1;
  if (arg[n] == 0) {
    if (*i + 1 >= argc) {
      fprintf(stderr, "systolic-cc: %s needs a value\n", name);
      exit(2);
    }
    return argv[++*i];
  }
  return NULL;
}

int main(int argc, char **argv) {
  options a = {0};
  a.clang = getenv("SYSTOLIC_CFRONT_CLANG");
#ifdef SYSTOLIC_DEFAULT_CLANG
  if (!a.clang && SYSTOLIC_DEFAULT_CLANG[0])
    a.clang = SYSTOLIC_DEFAULT_CLANG;   /* the clang 24 of this build */
#endif
  a.build_dir = getenv("SYSTOLIC_BUILD");
  if (!a.build_dir)
    a.build_dir = format("%s/build", SYSTOLIC_SOURCE_DIR);
  a.llvm_build = getenv("LLVM_BUILD");
  if (!a.llvm_build)
    a.llvm_build = format("%s/work/llvm-project/build", getenv("HOME") ? getenv("HOME") : "");
  a.runtime = "board";
  a.submission = "auto";
  a.cost_model = "v2";
  a.k_max_8x8 = 16;
  a.k_max_4x4 = 16;
  a.std = "c11";
  const char *opt = "0";

  for (int i = 1; i < argc; ++i) {
    const char *s = argv[i], *v;
    if (strcmp(s, "-h") == 0 || strcmp(s, "--help") == 0) {
      usage();
      return 0;
    } else if ((v = option_value(s, "--clang", &i, argc, argv))) {
      a.clang = v;
    } else if ((v = option_value(s, "--build-dir", &i, argc, argv))) {
      a.build_dir = v;
    } else if ((v = option_value(s, "--llvm-build", &i, argc, argv))) {
      a.llvm_build = v;
    } else if ((v = option_value(s, "--runtime", &i, argc, argv))) {
      if (strcmp(v, "soft") && strcmp(v, "board")) {
        fprintf(stderr, "systolic-cc: --runtime must be soft or board\n");
        return 2;
      }
      a.runtime = v;
    } else if ((v = option_value(s, "--fpga-config", &i, argc, argv))) {
      a.fpga_config = v;
    } else if ((v = option_value(s, "--submission", &i, argc, argv))) {
      if (strcmp(v, "auto") && strcmp(v, "sync") && strcmp(v, "async")) {
        fprintf(stderr, "systolic-cc: --submission must be auto, sync or async\n");
        return 2;
      }
      a.submission = v;
    } else if ((v = option_value(s, "--cost-model", &i, argc, argv))) {
      if (strcmp(v, "v1") != 0 && strcmp(v, "v2") != 0) {
        fprintf(stderr, "systolic-cc: --cost-model must be v1 or v2\n");
        return 2;
      }
      a.cost_model = v;
    } else if ((v = option_value(s, "--k-max-8x8", &i, argc, argv))) {
      a.k_max_8x8 = strtol(v, NULL, 10);
    } else if ((v = option_value(s, "--k-max-4x4", &i, argc, argv))) {
      a.k_max_4x4 = strtol(v, NULL, 10);
    } else if ((v = option_value(s, "--verify-rtol", &i, argc, argv))) {
      a.verify_rtol = strtod(v, NULL);
    } else if (strcmp(s, "--explain") == 0) {
      explain = 1;
    } else if (strcmp(s, "--verify") == 0) {
      a.verify = 1;
    } else if (strcmp(s, "--fp-reassociation") == 0) {
      a.fp_reassociation = 1;
    } else if (strcmp(s, "--reduction-batches") == 0) {
      /* the default (docs/AFFINE_GEMM_MAPPING.md section 23) */
    } else if (strcmp(s, "--no-reduction-batches") == 0) {
      a.no_reduction_batches = 1;
    } else if (strcmp(s, "--fp-distribute") == 0) {
      a.fp_distribute = 1;
    } else if ((v = option_value(s, "--keep-dir", &i, argc, argv))) {
      a.keep_dir = v;
    } else if (strcmp(s, "-o") == 0) {
      if (i + 1 >= argc) {
        usage();
        return 2;
      }
      a.output = argv[++i];
    } else if (strncmp(s, "-std", 4) == 0) {
      if (s[4] == '=')
        a.std = s + 5;
      else if (s[4] == 0 && i + 1 < argc)
        a.std = argv[++i];
      else {
        usage();
        return 2;
      }
    } else if (strncmp(s, "-D", 2) == 0 || strncmp(s, "-I", 2) == 0) {
      const char *val = s[2] ? s + 2 : (i + 1 < argc ? argv[++i] : NULL);
      if (!val) {
        usage();
        return 2;
      }
      push(s[1] == 'D' ? &a.defines : &a.includes, val);
    } else if (strncmp(s, "-O", 2) == 0) {
      opt = s[2] ? s + 2 : (i + 1 < argc ? argv[++i] : "0");
    } else if (s[0] == '-' && s[1]) {
      fprintf(stderr, "systolic-cc: unknown option %s\n", s);
      usage();
      return 2;
    } else if (!a.source) {
      a.source = s;
    } else {
      usage();
      return 2;
    }
  }
  if (!a.source || !a.output) {
    usage();
    return 2;
  }
  a.opt_level = format("-O%s", opt);
  if (!a.clang) {
    log_error("no clang 24 given (--clang or SYSTOLIC_CFRONT_CLANG)");
    return 2;
  }
  if (!a.keep_dir)
    a.keep_dir = format("%s.systolic", a.output);

  /* systolic-cfront is installed next to this program. */
  char self[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
  if (n <= 0) {
    log_error("cannot locate this executable");
    return 2;
  }
  self[n] = 0;
  a.self_dir = dir_name(self);

  if (mkdir(a.keep_dir, 0777) != 0 && errno != EEXIST) {
    log_error("error: cannot create %s: %s", a.keep_dir, strerror(errno));
    return 1;
  }
  driver d = {&a, NULL, ""};
  char *commands = kpath(&d, "commands.txt");
  d.commands = fopen(commands, "w");
  if (!d.commands) {
    log_error("error: cannot write %s", commands);
    return 1;
  }
  if (compile(&d) != 0) {
    log_error("error: %s", d.error);
    return 1;
  }
  return 0;
}
