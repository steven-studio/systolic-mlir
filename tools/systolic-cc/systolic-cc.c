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
 *     passes the arrays to the compiled kernel, then asks the runtime whether
 *     any FPGA invocation failed (runtime/offload/systolic_offload.h) -- a
 *     failure is reported as such and the program exits 3 instead of
 *     continuing with an incomplete result. With --verify it also runs
 *     NAME__systolic_reference on copies of the arrays NAME writes and
 *     compares: a mismatch is reported as a numerical error (exit 4).
 *  6. Everything is linked with a runtime: --runtime=soft (default), the
 *     software model of the runtime ABI, NOT the FPGA; --runtime=board, the
 *     real runtime (DPTI to the programmed board).
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

static void log_note(const char *fmt, ...) {
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
  long k_max_8x8, k_max_4x4;
  int verify;
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

static char *rewrite_host(const char *data, size_t length, const char *src,
                          const function_info *offload, size_t n) {
  edit *edits = xmalloc(2 * n * sizeof *edits);
  size_t ne = 0;
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
  char *out = format("#line 1 \"%s\"\n%s", src, text);
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

static char *shim_source(const function_info *offload, size_t n, int verify,
                         double rtol, const char *source) {
  buffer b = {0};
  bprintf(&b, "/* Generated by systolic-cc: the C entry points of the "
              "functions it offloaded\n * from %s. */\n", base_name(source));
  bprintf(&b, "#include \"systolic_offload.h\"\n#include <math.h>\n"
              "#include <stdint.h>\n#include <stdio.h>\n#include <stdlib.h>\n"
              "#include <string.h>\n\n");
  bprintf(&b, "static void close_runtime(void) { systolic_offload_close(); }\n"
              "static void once(void) {\n  static int done;\n"
              "  if (!done) { done = 1; atexit(close_runtime); }\n}\n\n");
  const char *how = NULL;
  char *how_text = rtol <= 0 ? format("bitwise")
                             : format("relative tolerance %g", rtol);
  how = how_text;
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
        for (size_t d = 0; d < rank; ++d) {
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
        bprintf(&kargs, "%s%s", kargs.n ? ", " : "", pname);
        bprintf(&kdecl, "%sint", kdecl.n ? ", " : "");
      }
    }
    const char *name = offload[f].name, *symbol = offload[f].symbol;
    char *where = format("%s:%ld", base_name(source), offload[f].line);
    const char *decl_text = decl.n ? decl.text : "void";
    bprintf(&b, "void %s(%s);\n", symbol, kdecl.n ? kdecl.text : "void");
    if (verify)
      bprintf(&b, "void %s%s(%s);\n", name, REFERENCE_SUFFIX, decl_text);
    bprintf(&b, "void %s(%s) {\n", name, decl_text);
    bprintf(&b, "  systolic_offload_stats_t before, after;\n  once();\n"
                "  systolic_offload_stats(&before);\n");
    if (verify) {
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        if (!is_written_memref(p))
          continue;
        const char *ct = c_type(json_string(json_get(p, "elem"), "f32"));
        const char *pname = json_string(json_get(p, "name"), "");
        long long count = element_count(p);
        bprintf(&b, "  %s *ref_%s = malloc(%lld * sizeof(%s) + 1);\n", ct,
                pname, count, ct);
        bprintf(&b, "  memcpy(ref_%s, %s, %lld * sizeof(%s));\n", pname, pname,
                count, ct);
      }
      bprintf(&b, "  %s%s(", name, REFERENCE_SUFFIX);
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        const char *pname = json_string(json_get(p, "name"), "");
        if (is_written_memref(p))
          bprintf(&b, "%s(void *)ref_%s", i ? ", " : "", pname);
        else
          bprintf(&b, "%s%s", i ? ", " : "", pname);
      }
      bprintf(&b, ");\n");
    }
    bprintf(&b, "  %s(%s);\n", symbol, kargs.n ? kargs.text : "");
    bprintf(&b, "  systolic_offload_stats(&after);\n"
                "  unsigned long long inv = 0;\n"
                "  for (int d = 0; d < 4; ++d)\n"
                "    inv += after.per_device[d] - before.per_device[d];\n"
                "  if (after.failed != before.failed) {\n");
    bprintf(&b, "    fprintf(stderr, \"systolic-cc: %s (%s): RUNTIME FAILURE: "
                "%%llu of the FPGA invocations failed (first rc %%d) on %%s; "
                "the result is incomplete -- a runtime / transport failure, "
                "not a numerical result\\n\", after.failed - before.failed, "
                "after.first_error, after.backend);\n", name, where);
    bprintf(&b, "    exit(3);\n  }\n");
    if (verify) {
      bprintf(&b, "  unsigned long long checked = 0, bad = 0;\n");
      for (size_t i = 0; i < np; ++i) {
        const json_value *p = json_at(params, i);
        if (!is_written_memref(p))
          continue;
        const char *ct = c_type(json_string(json_get(p, "elem"), "f32"));
        const char *pname = json_string(json_get(p, "name"), "");
        bprintf(&b, "  for (long e = 0; e < %lld; ++e) {\n", element_count(p));
        bprintf(&b, "    %s got = ((%s *)%s)[e], want = ref_%s[e];\n", ct, ct,
                pname, pname);
        if (rtol > 0) {
          char *r = shortest(rtol);
          bprintf(&b, "    int same = fabs((double)got - want) <= %s * "
                      "fmax(fabs((double)got), fabs((double)want));\n", r);
          free(r);
        } else {
          bprintf(&b, "    int same = memcmp(&got, &want, sizeof got) == 0;\n");
        }
        bprintf(&b, "    ++checked;\n    if (!same && bad++ == 0)\n");
        bprintf(&b, "      fprintf(stderr, \"systolic-cc: %s (%s): first "
                    "mismatch %s[%%ld]: FPGA path %%.9g, CPU reference "
                    "%%.9g\\n\", e, (double)got, (double)want);\n", name,
                where, pname);
        bprintf(&b, "  }\n  free(ref_%s);\n", pname);
      }
      bprintf(&b, "  if (bad) {\n");
      bprintf(&b, "    fprintf(stderr, \"systolic-cc: %s (%s): NUMERICAL "
                  "MISMATCH: %%llu of %%llu elements differ from the CPU "
                  "reference (%s; %%llu invocations on %%s)\\n\", bad, "
                  "checked, inv, after.backend);\n", name, where, how);
      bprintf(&b, "    exit(4);\n  }\n");
      bprintf(&b, "  fprintf(stderr, \"systolic-cc: %s (%s): verified: %%llu "
                  "elements equal the CPU reference (%s); %%llu invocations "
                  "(8x8 %%llu, 4x4 %%llu/%%llu/%%llu) on %%s\\n\", checked, "
                  "inv, after.per_device[0] - before.per_device[0], "
                  "after.per_device[1] - before.per_device[1], "
                  "after.per_device[2] - before.per_device[2], "
                  "after.per_device[3] - before.per_device[3], "
                  "after.backend);\n", name, where, how);
    }
    bprintf(&b, "}\n\n");
    free(where);
    free(decl.text);
    free(kargs.text);
    free(kdecl.text);
  }
  free(how_text);
  /* Python joined the lines and added one newline: the text ends "}\n\n"
     for every function, "\n\n" without any. */
  return b.text ? b.text : format("%s", "");
}

/* --------------------------------------------------------------- compile */

static int fleet_k_max(driver *d, long *k8, long *k4) {
  const options *a = d->a;
  if (strcmp(a->runtime, "board") != 0) {
    *k8 = a->k_max_8x8;
    *k4 = a->k_max_4x4;
    return 0;
  }
  const char *config = a->fpga_config ? a->fpga_config
                                      : getenv("SYSTOLIC_FPGA_CONFIG");
  if (!config) {
    snprintf(d->error, sizeof d->error,
             "--runtime=board needs --fpga-config (or SYSTOLIC_FPGA_CONFIG): "
             "the fpga_config.txt of the bitstream on the board");
    return -1;
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
  return 0;
}

static void add_flags(strvec *v, const options *a) {
  for (size_t i = 0; i < a->defines.n; ++i)
    push(v, format("-D%s", a->defines.items[i]));
  for (size_t i = 0; i < a->includes.n; ++i)
    push(v, format("-I%s", a->includes.items[i]));
}

static int compile(driver *d) {
  const options *a = d->a;
  char *opt = format("%s/bin/systolic-opt", a->build_dir);
  char *cfront = format("%s/systolic-cfront", a->self_dir);
  char *offload_rt = format("%s/runtime/offload", SYSTOLIC_SOURCE_DIR);
  long k8, k4;
  if (fleet_k_max(d, &k8, &k4) != 0)
    return -1;
  char *fleet = format("--systolic-declare-fpga-fleet=k-max-8x8=%ld "
                       "k-max-4x4=%ld", k8, k4);
  char *phys = format("--lower-systolic-matmul-tile-to-fpga=phys-k-max-8x8=%ld "
                      "phys-k-max-4x4=%ld", k8, k4);

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
                       "--interface", kpath(d, "10-interface.json"), src, "-o",
                       kpath(d, "10-frontend.mlir"), NULL);
  add_flags(&cmd, a);
  result r;
  if (run(d, &cmd, "the C front end", NULL, NULL, &r) != 0)
    return -1;
  fputs(r.err, stderr);
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
    strvec c2 = argv_of(opt, fleet, "--affine-matmul-to-systolic=report=true",
                        kpath(d, "10-frontend.mlir"), "-o",
                        kpath(d, "20-systolic.mlir"), NULL);
    if (run(d, &c2, "GEMM recognition / Affine Partitioning", NULL, NULL,
            &r) != 0)
      return -1;
    const char *remarks = r.err;
    write_file(kpath(d, "20-remarks.txt"), remarks, strlen(remarks));
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
        log_note("%s:%ld:%ld: note: '%s' is offloaded: %ld systolic tile "
                 "invocation(s) on the 1x8x8 + 3x4x4 fleet", a->source,
                 info.line, info.col, info.name, n);
        offload[noffload++] = info;
      }
    }
  }

  /* The plan, for the record. */
  {
    buffer plan = {0};
    bprintf(&plan, "{\n \"source\": \"%s\",\n \"runtime\": \"%s\",\n"
                   " \"k_max_8x8\": %ld,\n \"k_max_4x4\": %ld,\n"
                   " \"verify\": %s,\n \"offloaded\": [", src, a->runtime, k8,
            k4, a->verify ? "true" : "false");
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
    add_flags(&c3, a);
    for (size_t i = 0; i < noffload; ++i)
      push(&c3, format("--function=%s", offload[i].name));
    if (run(d, &c3, "the C front end (offloaded functions)", NULL, NULL, &r))
      return -1;
    strvec c4 = argv_of(opt, fleet, "--affine-matmul-to-systolic",
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
  char *host = rewrite_host(src_text, src_len, src, offload, noffload);
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
                             a->source);
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
  log_note("wrote %s (%zu function(s) offloaded, runtime: %s; intermediates "
           "in %s)", a->output, noffload,
           strcmp(a->runtime, "soft") == 0 ? "software model, NOT the FPGA"
                                           : "FPGA board", a->keep_dir);
  return 0;
}

/* ------------------------------------------------------------------- main */

static void usage(void) {
  fprintf(stderr,
          "usage: systolic-cc [--clang CLANG] [--build-dir DIR] "
          "[--llvm-build DIR] [--runtime soft|board] [--fpga-config FILE] "
          "[--k-max-8x8 N] [--k-max-4x4 N] [--verify] [--verify-rtol R] "
          "[--keep-dir DIR] [-DNAME[=VAL]] [-IDIR] [-std=STD] [-ON] FILE.c "
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
  a.build_dir = getenv("SYSTOLIC_BUILD");
  if (!a.build_dir)
    a.build_dir = format("%s/build", SYSTOLIC_SOURCE_DIR);
  a.llvm_build = getenv("LLVM_BUILD");
  if (!a.llvm_build)
    a.llvm_build = format("%s/work/llvm-project/build", getenv("HOME") ? getenv("HOME") : "");
  a.runtime = "soft";
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
    } else if ((v = option_value(s, "--k-max-8x8", &i, argc, argv))) {
      a.k_max_8x8 = strtol(v, NULL, 10);
    } else if ((v = option_value(s, "--k-max-4x4", &i, argc, argv))) {
      a.k_max_4x4 = strtol(v, NULL, 10);
    } else if ((v = option_value(s, "--verify-rtol", &i, argc, argv))) {
      a.verify_rtol = strtod(v, NULL);
    } else if (strcmp(s, "--verify") == 0) {
      a.verify = 1;
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
    log_note("no clang 24 given (--clang or SYSTOLIC_CFRONT_CLANG)");
    return 2;
  }
  if (!a.keep_dir)
    a.keep_dir = format("%s.systolic", a.output);

  /* systolic-cfront is installed next to this program. */
  char self[PATH_MAX];
  ssize_t n = readlink("/proc/self/exe", self, sizeof self - 1);
  if (n <= 0) {
    log_note("cannot locate this executable");
    return 2;
  }
  self[n] = 0;
  a.self_dir = dir_name(self);

  if (mkdir(a.keep_dir, 0777) != 0 && errno != EEXIST) {
    log_note("error: cannot create %s: %s", a.keep_dir, strerror(errno));
    return 1;
  }
  driver d = {&a, NULL, ""};
  char *commands = kpath(&d, "commands.txt");
  d.commands = fopen(commands, "w");
  if (!d.commands) {
    log_note("error: cannot write %s", commands);
    return 1;
  }
  if (compile(&d) != 0) {
    log_note("error: %s", d.error);
    return 1;
  }
  return 0;
}
