/*
 * systolic-cfront -- C to MLIR (func / affine / arith / memref) for the
 * affine subset of C the systolic-array compiler takes.
 *
 *   systolic-cfront --clang CLANG [--clang-major N] [-DNAME[=VAL]] [-IDIR]
 *                   [-std=STD] [--function NAME] [--skip-unsupported]
 *                   [--symbol-suffix S] [--interface FILE.json]
 *                   [--fp-reassociation] [--static-extents]
 *                   [--modulo-subscripts] [--gather-subscripts]
 *                   FILE.c [-o OUT.mlir]
 *
 * --static-extents (systolic-cc's Generalized GEMM fallback,
 * docs/GENERALIZED_GEMM_SCOPE.md "The fallback in systolic-cc"): an array
 * parameter written with a leading bound -- float A[64][16], A[M][K],
 * A[restrict 64][16], A[static 64] -- becomes memref<EXTENTxKxf32> instead of
 * memref<?xKxf32>, where EXTENT is the leading extent the interface already
 * gives (the last row the accesses reach, plus one), when every access's
 * range is static and starts at row 0 or later. That is the extent
 * systolic-cc builds the memref descriptor with, so the static type states
 * nothing the call does not already: C does not enforce a parameter's
 * declared bound, so the bound written is not used. A parameter written as
 * a pointer (float (*A)[16], A[][16]), one whose accesses are offset by a
 * parameter, or one that reads before its first row keeps memref<?x...>.
 *
 * --modulo-subscripts: a subscript may also be E % C, E an affine subscript of
 * the loop counters (no parameter), C a positive integer constant, and E >= 0
 * wherever C evaluates it (checked; C's % is the floor modulo there). It
 * becomes the affine expression (E) mod C; its range is computed by running
 * the nest's counters (up to the enumeration limit below). The GEMM lowering
 * finds no GEMM in such a nest; systolic-cc's Generalized GEMM fallback may.
 *
 * --gather-subscripts (with --static-extents; systolic-cc's Generalized GEMM
 * fallback): the leading subscript of an array that is only read may also
 * be T[E], T a one-dimensional integer array parameter (int, unsigned, long,
 * int32_t, ...) read nowhere else and never written, E an affine subscript
 * of the loop counters -- A[dp[i]][k], dp a MoE token index or a BSR column
 * index. It becomes affine.load T[E], arith.index_cast (index_castui when
 * T's element is unsigned), and a memref.load of the array at that row and
 * its other subscripts (each an affine form of the counters, no parameter,
 * no %%). The array's leading extent is then the bound written as an
 * integer literal (float A[32][12], A[restrict 32][12]); without one the
 * function is outside the subset. The interface says which table gathers
 * the array (gather_table): systolic-cc's shim checks, at every call, that
 * the table's entries the nest reads are rows in [0, extent) before it
 * offloads, and runs the function as written otherwise. One level only
 * (T[T[i]] is refused), never in an array that is written (C's stores stay
 * affine), and a table read as anything but such a subscript is refused.
 *
 * --scatter-subscripts (with --static-extents; systolic-cc's Generalized GEMM
 * fallback): a subscript of an array that is written may be any integer
 * expression of the loop counters, signed integer parameters and integer
 * literals with + - * / % & | ^ << >>, unary - and ~, E1 CMP E2 ? E3 : E4,
 * and reads of one-dimensional integer array parameters at such
 * expressions, to any depth: C[dp1[dp2[i]]][j], C[(i * i) % 16][j],
 * C[f(i, j)][g(i, j)]. It is computed in index arithmetic (arith.divsi,
 * remsi, ...: C's int results wherever C defines them) and the array is read
 * and written with memref.load / memref.store. The array's leading extent is
 * then the bound written as an integer literal, as is every table's; without
 * one the function is outside the subset. The interface gives the
 * expressions as C text and the counters they read with their ranges
 * (scatter): systolic-cc's shim evaluates them at every call and runs the
 * function as written unless every one is inside the array. An affine or
 * E % C subscript stays what it was.
 *
 * --fp-reassociation marks every floating-point arith.addf / subf / mulf
 * `fastmath<reassoc>`: the program permits reassociating floating-point sums,
 * as clang's -fassociative-math does. Without it nothing is marked, and the
 * GEMM lowering keeps a floating-point accumulation in the loop's order (on
 * the host; docs/AFFINE_GEMM_MAPPING.md section 6).
 *
 * Clang parses and type-checks the C source (-fsyntax-only -Xclang
 * -ast-dump=json): preprocessing, declarations, implicit conversions and
 * types are clang's. Only the clang executable is run; nothing of it is
 * linked. It must be the clang the project is standardized on -- LLVM 24
 * (--clang-major, default 24): another major version is refused, never used
 * silently, since its AST JSON is a different format. This program
 * translates clang's AST, construct by construct, into MLIR. Nothing in it
 * knows about matrix multiplication; a GEMM is recognized later, from the
 * affine IR, by --affine-matmul-to-systolic.
 *
 * The subset is the one whose meaning is an affine loop nest over arrays,
 * and it is translated exactly; anything outside it is an error with its
 * source location, never an approximation:
 *
 *   functions    void, defined in FILE.c. Parameters: arrays of float/double
 *                (float A[M][K], float A[restrict M][K], which C adjusts to
 *                float (*restrict)[K]) become memref<?xKxf32> -- the leading
 *                extent is not part of a C parameter's type; restrict becomes
 *                {llvm.noalias}. Scalar float/double parameters become
 *                f32/f64 arguments.
 *   statements   compound statements, for, expression statements that assign
 *                an array element (=, +=, -=, *=, /=), a final return;.
 *   for          for (int i = LB; i < UB; ++i) -- also <=, i++, i += S
 *                (S > 0), i = i + S -- with LB, UB, S integer constant
 *                expressions; i declared in the for, never written in the
 *                body. Becomes affine.for LB to UB step S (<=: UB + 1).
 *                LB and UB may also be affine expressions of the enclosing
 *                counters, counting up; the loop becomes affine.for with
 *                affine_map bounds over the enclosing counters. With the
 *                same counter terms (for (int k = i; k < i + 4; ++k)) the
 *                bounds shift together and the domain is a constant-extent
 *                parallelotope (docs/AFFINE_GEMM_MAPPING.md section 19.12);
 *                with different ones (for (int k = i; k < 8; ++k), a
 *                triangle) the loop is translated as written, and the
 *                ranges of the subscripts in its scope (the interface's
 *                extents) and the checks of its loops are computed by
 *                running the nest's counters -- up to 4194304 values, past
 *                which the function is refused (section 19.13); on a
 *                parallelotope they are closed forms. The condition may be
 *                any comparison L OP R (<, <=, >, >=) with L - R = a * i + g
 *                + c affine in the counter i and the enclosing counters g
 *                (a != 0; a = 1 or -1 when g has counter terms): k - i < 4
 *                and i + 4 > k are k < i + 4, 2 * i - 3 > 0 is i > 1 (exact
 *                rounding), 8 > k is k < 8.
 *                An upper bound may also be the min of two such bounds
 *                (docs/AFFINE_GEMM_MAPPING.md section 26.5): i < MIN(P, Q)
 *                with the macro ((a) < (b) ? (a) : (b)) -- the conditional
 *                expression that is exactly min(P, Q), also with <= or with
 *                > / >= and its branches exchanged -- or i < P && i < Q
 *                (< or <= each), P and Q affine in the enclosing counters
 *                and of the counter's type. It becomes the affine.for upper
 *                bound min (P, Q), the values running the nest are computed
 *                at every outer point (as for a triangle), and every node
 *                of P and Q is checked in its C type. A function call
 *                (even one named min), a max, a conditional that is not the
 *                smaller of its operands, a nested one or a conversion is
 *                refused.
 *                Counting down: for (int i = LB; i > B; --i) -- also >=,
 *                i--, i -= S (S > 0), i = i - S. affine.for only counts up,
 *                so it becomes the ordinal loop affine.for %i = 0 to N (N the
 *                trip count) and every use of i the affine expression
 *                LB - S * %i: the values LB, LB - S, ... in the same order.
 *   subscripts   every array access subscripts all dimensions; each subscript
 *                is an affine expression of the enclosing loop counters and
 *                integer constants (+, -, constant *).
 *   integers     C evaluates every node of a loop's initial value, bound and
 *                condition and of a subscript -- each operation and each
 *                implicit conversion -- in its own C type. The translation
 *                computes exact integer values, so a function is translated
 *                only when every such node's exact value lies in its type's
 *                range wherever C evaluates it: no signed overflow, no
 *                unsigned wrap-around, no conversion that changes a value
 *                (unsigned r = -1; k < 8u at k = -5); else it is refused.
 *                The step S is an integer constant, taken by its value: the
 *                counter's values, the last one plus S included, must fit
 *                the counter's type.
 *   arithmetic   + - * / and unary - on float/double, floating literals,
 *                integer constants converted to floating point, and C's own
 *                implicit float <-> double conversions (arith.extf/truncf).
 *
 * Floating-point contraction: every C operator becomes its own MLIR op (no
 * a * b + c is fused), i.e. the semantics of -ffp-contract=off.
 *
 * For a whole program: --skip-unsupported reports a function outside the
 * subset as a note and leaves it out; --symbol-suffix renames the emitted
 * functions; --interface writes, per translated function, its C parameters,
 * the memref shape each becomes (with the exact extent the C type leaves
 * open), which arrays are read and written, and the source ranges of the
 * definition -- what systolic-cc needs to call it from C.
 *
 * Exit status: 0 success, 1 the source is outside the subset (diagnostic on
 * stderr, clang style), 2 usage error or clang failed.
 */

#define _GNU_SOURCE
#include "json.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---------------------------------------------------------------- helpers */

/* This program translates one file and ends, and most of what it allocates
   (the text of the output, names, forms, the captured output of clang) lives
   until then. Nothing released it: main returned, or an error path called
   exit, with all of it allocated. Every block this file allocates is recorded
   here, taken off the record when it is freed or reallocated, and what is
   left is freed when the process ends, whichever way it ends. A pointer that
   was allocated elsewhere (open_memstream's buffer, the JSON reader's) is not
   on the record and is freed as it always was. */
static void **live_blocks;      /* open addressing; NULL empty, LIVE_GONE a hole */
static size_t live_cap, live_used, live_filled;
#define LIVE_GONE ((void *)1)

static size_t live_slot(void *p, size_t cap) {
  return (size_t)(((uintptr_t)p >> 4) * 0x9E3779B97F4A7C15ull) & (cap - 1);
}

static void live_release_all(void) {
  for (size_t i = 0; i < live_cap; ++i)
    if (live_blocks[i] && live_blocks[i] != LIVE_GONE)
      free(live_blocks[i]);
  free(live_blocks);
  live_blocks = NULL;
  live_cap = live_used = live_filled = 0;
}

static void live_add(void *p) {
  if (!p)
    return;
  if (2 * (live_filled + 1) > live_cap) {
    size_t cap = live_cap ? live_cap : 1024;
    while (2 * (live_used + 1) > cap)
      cap *= 2;
    if (4 * (live_used + 1) > cap)
      cap *= 2;
    void **blocks = calloc(cap, sizeof *blocks);
    if (!blocks) {
      fprintf(stderr, "systolic-cfront: out of memory\n");
      exit(2);
    }
    for (size_t i = 0; i < live_cap; ++i)
      if (live_blocks[i] && live_blocks[i] != LIVE_GONE) {
        size_t s = live_slot(live_blocks[i], cap);
        while (blocks[s])
          s = (s + 1) & (cap - 1);
        blocks[s] = live_blocks[i];
      }
    if (!live_blocks)
      atexit(live_release_all);
    free(live_blocks);
    live_blocks = blocks;
    live_cap = cap;
    live_filled = live_used;
  }
  size_t s = live_slot(p, live_cap);
  while (live_blocks[s] && live_blocks[s] != LIVE_GONE)
    s = (s + 1) & (live_cap - 1);
  if (!live_blocks[s])
    ++live_filled;
  live_blocks[s] = p;
  ++live_used;
}

static void live_remove(void *p) {
  if (!p || !live_cap)
    return;
  for (size_t s = live_slot(p, live_cap); live_blocks[s]; s = (s + 1) & (live_cap - 1))
    if (live_blocks[s] == p) {
      live_blocks[s] = LIVE_GONE;
      --live_used;
      return;
    }
}

static void *live_malloc(size_t n) {
  void *p = malloc(n);
  live_add(p);
  return p;
}

static void *live_realloc(void *old, size_t n) {
  live_remove(old);
  void *p = realloc(old, n);
  live_add(p ? p : old);   /* realloc that fails leaves the old block as it is */
  return p;
}

static void live_free(void *p) {
  live_remove(p);
  free(p);
}

/* From here on this file's malloc, realloc and free are the recorded ones. */
#define malloc(n) live_malloc(n)
#define realloc(p, n) live_realloc(p, n)
#define free(p) live_free(p)

static void *xmalloc(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    fprintf(stderr, "systolic-cfront: out of memory\n");
    exit(2);
  }
  return p;
}

static char *xstrdup(const char *s) {
  size_t n = strlen(s);
  char *p = xmalloc(n + 1);
  memcpy(p, s, n + 1);
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

static void strvec_push(strvec *v, const char *s) {
  if (v->n == v->cap) {
    v->cap = v->cap ? 2 * v->cap : 16;
    v->items = realloc(v->items, v->cap * sizeof *v->items);
    if (!v->items) {
      fprintf(stderr, "systolic-cfront: out of memory\n");
      exit(2);
    }
  }
  v->items[v->n++] = s;
}

static int strvec_contains(const strvec *v, const char *s) {
  for (size_t i = 0; i < v->n; ++i)
    if (strcmp(v->items[i], s) == 0)
      return 1;
  return 0;
}

static const char *kind_of(const json_value *n) {
  return json_string(json_get(n, "kind"), "");
}

static const char *opcode_of(const json_value *n) {
  return json_string(json_get(n, "opcode"), "");
}

static const char *qual_type(const json_value *n) {
  return json_string(json_get(json_get(n, "type"), "qualType"), "");
}

static const json_value *child(const json_value *n, size_t i) {
  return json_at(json_get(n, "inner"), i);
}

static size_t child_count(const json_value *n) {
  return json_size(json_get(n, "inner"));
}

/* The id of the declaration a DeclRefExpr names. */
static const char *referenced_id(const json_value *n) {
  return json_string(json_get(json_get(n, "referencedDecl"), "id"), NULL);
}

static const char *referenced_name(const json_value *n) {
  return json_string(json_get(json_get(n, "referencedDecl"), "name"), "?");
}

/* t with every "const" / "volatile" word (and the blanks after it) removed,
   then trimmed. */
static char *strip_qualifiers(const char *t) {
  size_t n = strlen(t);
  char *out = xmalloc(n + 1);
  size_t o = 0;
  for (size_t i = 0; i < n;) {
    int boundary = i == 0 || !(t[i - 1] == '_' || isalnum((unsigned char)t[i - 1]));
    size_t w = 0;
    if (boundary && strncmp(t + i, "const", 5) == 0)
      w = 5;
    else if (boundary && strncmp(t + i, "volatile", 8) == 0)
      w = 8;
    if (w) {
      char after = t[i + w];
      if (!(after == '_' || isalnum((unsigned char)after))) {
        i += w;
        while (t[i] == ' ' || t[i] == '\t' || t[i] == '\n')
          ++i;
        continue;
      }
    }
    out[o++] = t[i++];
  }
  out[o] = 0;
  size_t b = 0;
  while (out[b] == ' ' || out[b] == '\t')
    ++b;
  size_t e = strlen(out);
  while (e > b && (out[e - 1] == ' ' || out[e - 1] == '\t'))
    --e;
  memmove(out, out + b, e - b);
  out[e - b] = 0;
  return out;
}

static const char *float_type(const char *t) {
  if (strcmp(t, "float") == 0)
    return "f32";
  if (strcmp(t, "double") == 0)
    return "f64";
  return NULL;
}

/* The integer types a loop counter can have, with their largest value. */
static const struct {
  const char *name;
  unsigned long long max;
} kIntTypes[] = {
    {"int", 2147483647ULL},
    {"long", 9223372036854775807ULL},
    {"long long", 9223372036854775807ULL},
    {"short", 32767ULL},
    {"unsigned int", 4294967295ULL},
    {"unsigned long", 18446744073709551615ULL},
    {"unsigned long long", 18446744073709551615ULL},
    {"unsigned short", 65535ULL},
    {"unsigned", 4294967295ULL},
    {"char", 127ULL},
    {"signed char", 127ULL},
    {"unsigned char", 255ULL},
};

static int int_type_max(const char *t, unsigned long long *max) {
  for (size_t i = 0; i < sizeof kIntTypes / sizeof kIntTypes[0]; ++i)
    if (strcmp(kIntTypes[i].name, t) == 0) {
      if (max)
        *max = kIntTypes[i].max;
      return 1;
    }
  return 0;
}

/* --------------------------------------------------------------- locations */

typedef struct {
  const char *text;
  size_t length;
  size_t *line_starts;
  size_t lines;
} source_map;

static void source_map_init(source_map *sm, const char *text, size_t length) {
  sm->text = text;
  sm->length = length;
  size_t cap = 64;
  sm->line_starts = xmalloc(cap * sizeof(size_t));
  sm->lines = 0;
  sm->line_starts[sm->lines++] = 0;
  for (size_t i = 0; i < length; ++i)
    if (text[i] == '\n') {
      if (sm->lines == cap) {
        cap *= 2;
        sm->line_starts = realloc(sm->line_starts, cap * sizeof(size_t));
      }
      sm->line_starts[sm->lines++] = i + 1;
    }
}

static void source_map_line_col(const source_map *sm, long long offset,
                                long *line, long *col) {
  size_t lo = 0, hi = sm->lines - 1;
  while (lo < hi) {
    size_t mid = (lo + hi + 1) / 2;
    if ((long long)sm->line_starts[mid] <= offset)
      lo = mid;
    else
      hi = mid - 1;
  }
  *line = (long)lo + 1;
  *col = (long)(offset - (long long)sm->line_starts[lo]) + 1;
}

/* The expansion offset of a node's beginning (macros expand where used), or
   -1. */
static long long node_offset(const json_value *node) {
  const char *keys[] = {"range", "loc"};
  for (int k = 0; k < 2; ++k) {
    const json_value *loc = json_get(node, keys[k]);
    if (!loc)
      continue;
    if (k == 0) {
      loc = json_get(loc, "begin");
      if (!loc)
        continue;
    }
    const json_value *exp = json_get(loc, "expansionLoc");
    if (exp)
      loc = exp;
    const json_value *off = json_get(loc, "offset");
    if (off)
      return json_int(off, -1);
  }
  return -1;
}

static const json_value *strip_parens_and_casts(const json_value *node,
                                                const char *const *casts) {
  for (;;) {
    const char *kind = kind_of(node);
    if (strcmp(kind, "ParenExpr") == 0) {
      node = child(node, 0);
      continue;
    }
    if (strcmp(kind, "ImplicitCastExpr") == 0) {
      const char *ck = json_string(json_get(node, "castKind"), "");
      int match = 0;
      for (const char *const *c = casts; *c; ++c)
        if (strcmp(*c, ck) == 0)
          match = 1;
      if (match) {
        node = child(node, 0);
        continue;
      }
    }
    return node;
  }
}

static const char *const kLValueToRValue[] = {"LValueToRValue", NULL};
static const char *const kIntConstantCasts[] = {"IntegralCast", "NoOp",
                                                "LValueToRValue", NULL};
static const char *const kIndexCasts[] = {"LValueToRValue", "IntegralCast",
                                          "NoOp", NULL};
static const char *const kNoOp[] = {"NoOp", NULL};
static const char *const kDecayNoOp[] = {"ArrayToPointerDecay", "NoOp", NULL};
static const char *const kLoadNoOp[] = {"LValueToRValue", "NoOp", NULL};

/* ------------------------------------------------------- unsupported input */

static jmp_buf unsupported_jump;
static const json_value *unsupported_node;
static char *unsupported_message;

static _Noreturn void unsupported(const json_value *node, const char *fmt,
                                  ...) {
  va_list ap, aq;
  va_start(ap, fmt);
  va_copy(aq, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  unsupported_message = xmalloc((size_t)n + 1);
  vsnprintf(unsupported_message, (size_t)n + 1, fmt, aq);
  va_end(aq);
  unsupported_node = node;
  longjmp(unsupported_jump, 1);
}

/* ------------------------------------------------------------ the translator */

enum { MAX_FORM = 32, MAX_RANK = 8 };

/* A linear form over the enclosing loop counters: sum coef[i] * ids[i] +
   constant. */
typedef struct {
  size_t n;
  const char *ids[MAX_FORM];
  long long coef[MAX_FORM];
  long long constant;
} linear_form;

typedef enum { ENV_MEMREF, ENV_SCALAR, ENV_INTSCALAR, ENV_IV } env_kind;

typedef struct {
  const char *decl;            /* the declaration's id */
  env_kind kind;
  char *name;                  /* the SSA name */
  /* ENV_MEMREF */
  int rank;
  const char *elem;
  char *type;                  /* memref<?x...>; with --static-extents a
                                  placeholder until the function is done */
  char *dynamic_type;          /* --static-extents: memref<?x...> */
  int declared_bound;          /* --static-extents: written with a leading
                                  bound, float A[64][16] / A[M][K] */
  long long declared_extent;   /* --gather-subscripts: that bound when it is
                                  an integer literal, else -1 */
  int index_table;             /* --gather-subscripts: a one-dimensional
                                  integer array, read only as T[E] in a
                                  leading subscript */
  int inner_dims[MAX_RANK];
  /* ENV_SCALAR */
  /* (elem above) */
  /* ENV_INTSCALAR: the index-typed copy usable as an affine symbol, its C
     name (for the interface's extent expressions), and whether the C type
     is signed (only signed parameters may offset a subscript) */
  char *sym_name;
  const char *cname;
  int is_signed;
  int bits;                    /* 32 or 64, as the parameter is passed */
  /* ENV_IV: the counter's first and last values, unless it runs zero times */
  int has_values;
  long long first, last;
  /* ENV_IV of a loop that counts down: `name` is the ordinal t = 0 .. N-1 of
     the emitted affine.for, and the counter's value is origin - stride * t */
  int descending;
  long long origin, stride;
  /* ENV_IV of a loop whose bounds are affine in the enclosing counters: the
     counter is first + step * t plus this shift, a form over the enclosing
     counters with constant 0 (first and last are the values without it). */
  linear_form shift;
  /* ENV_IV of a loop whose bounds are affine in the enclosing counters with
     different counter terms (a triangle, docs/AFFINE_GEMM_MAPPING.md section
     19.13): its values are lb_form, lb_form + step, ... below ub_form
     (exclusive), both evaluated at the enclosing counters' values. first /
     last are unused: while such a loop is in scope every range is computed
     by running the nest's counters (enumerate_points). */
  int nonbox;
  linear_form lb_form, ub_form;
  /* S5.5: a second exclusive upper bound -- the loop runs below
     min(ub_form, ub2_form) (docs/AFFINE_GEMM_MAPPING.md section 26.5). Such
     a loop is always non-box. */
  int has_ub2;
  linear_form ub2_form;
  /* ENV_IV: the signed step of the counter's values, and its value at the
     current point of an enumeration. */
  long long step;
  long long cur;
} env_entry;

typedef struct {
  const char *decl;
  int read, written;
  int has_ranges;
  int rank;
  long long lo[MAX_RANK], hi[MAX_RANK];
  /* Subscripts that use a parameter: the range of the leading subscript is
     a C expression of the parameters (what the caller-side shim needs to
     size the array it snapshots), lo_expr / hi_expr, or NULL while every
     range is static. */
  char *lo_expr, *hi_expr;
  /* --gather-subscripts: the leading subscript is T[E] in some access; the
     table's C name and its declarator's id, and the leading extent (the
     literal bound written). The leading dimension then has no range. */
  int gathered;
  const char *gather_table, *gather_table_decl;
  long long gather_extent;
  /* --scatter-subscripts: written at a general subscript; the interface's
     "scatter" object (the counters and the expressions as C text). */
  char *scatter_json;
} access_info;

typedef struct {
  const char *display_name;
  const source_map *sm;
  strvec lines;
  strvec ssa_names;
  long counter;
  const char *symbol_suffix;
  int fp_reassociation;        /* --fp-reassociation: fastmath<reassoc> */
  int static_extents;          /* --static-extents */
  int modulo_subscripts;       /* --modulo-subscripts */
  int gather_subscripts;       /* --gather-subscripts */
  int scatter_subscripts;      /* --scatter-subscripts */
  /* Set by array_access for a written access at general subscripts, for the
     assignment to emit; cleared there. */
  struct {
    int active;
    int count;
    const json_value *nodes[MAX_RANK];
  } general;
  int in_gather;               /* translating the T[E] of a gather */
  /* Set by array_access for the access it just read when its leading
     subscript is a gather, for load() to emit; cleared by load(). */
  struct {
    int active;
    env_entry *table;
    char *table_index;           /* E, affine text */
    int count;                   /* the array's rank */
    linear_form forms[MAX_RANK]; /* the other subscripts' forms (d >= 1) */
  } gather;

  env_entry *env;
  size_t env_n, env_cap;
  access_info *accesses;
  size_t acc_n, acc_cap;

  char *interfaces;            /* the JSON objects, comma separated */
  /* Set while a subscript is parsed: a signed integer parameter is an
     affine symbol there (an additive run-time offset); in a loop header it
     stays outside the subset. */
  int in_subscript;
} translator;

static void emit(translator *tr, int indent, const char *fmt, ...) {
  va_list ap, aq;
  va_start(ap, fmt);
  va_copy(aq, ap);
  int n = vsnprintf(NULL, 0, fmt, ap);
  va_end(ap);
  char *line = xmalloc((size_t)(2 * indent + n + 1));
  memset(line, ' ', (size_t)(2 * indent));
  vsnprintf(line + 2 * indent, (size_t)n + 1, fmt, aq);
  va_end(aq);
  strvec_push(&tr->lines, line);
}

static char *loc_text(translator *tr, const json_value *node) {
  long long offset = node_offset(node);
  if (offset < 0)
    return format("loc(\"%s\":0:0)", tr->display_name);
  long line, col;
  source_map_line_col(tr->sm, offset, &line, &col);
  return format("loc(\"%s\":%ld:%ld)", tr->display_name, line, col);
}

static char *fresh(translator *tr) {
  for (;;) {
    char *name = format("%%t%ld", tr->counter++);
    if (!strvec_contains(&tr->ssa_names, name)) {
      strvec_push(&tr->ssa_names, name);
      return name;
    }
    free(name);
  }
}

static char *unique(translator *tr, const char *stem_in) {
  char *stem = xstrdup(stem_in);
  for (char *c = stem; *c; ++c)
    if (!(isalnum((unsigned char)*c) || *c == '_'))
      *c = '_';
  if (!stem[0]) {
    free(stem);
    stem = xstrdup("v");
  }
  if (isdigit((unsigned char)stem[0])) {
    char *v = format("v%s", stem);
    free(stem);
    stem = v;
  }
  char *name = format("%%%s", stem);
  for (int n = 1; strvec_contains(&tr->ssa_names, name); ++n) {
    free(name);
    name = format("%%%s_%d", stem, n);
  }
  free(stem);
  strvec_push(&tr->ssa_names, name);
  return name;
}

static env_entry *env_find(translator *tr, const char *decl) {
  if (!decl)
    return NULL;
  for (size_t i = tr->env_n; i-- > 0;)
    if (tr->env[i].decl && strcmp(tr->env[i].decl, decl) == 0)
      return &tr->env[i];
  return NULL;
}

static env_entry *env_add(translator *tr, const char *decl, env_kind kind) {
  if (tr->env_n == tr->env_cap) {
    tr->env_cap = tr->env_cap ? 2 * tr->env_cap : 16;
    tr->env = realloc(tr->env, tr->env_cap * sizeof *tr->env);
  }
  env_entry *e = &tr->env[tr->env_n++];
  memset(e, 0, sizeof *e);
  e->decl = decl;
  e->kind = kind;
  return e;
}

/* ------------------------------------------------------- integer constants */

static long long checked(long long a, long long b, char op,
                         const json_value *node) {
  long long r;
  int overflow = op == '+' ? __builtin_add_overflow(a, b, &r)
                 : op == '-' ? __builtin_sub_overflow(a, b, &r)
                             : __builtin_mul_overflow(a, b, &r);
  if (overflow)
    unsupported(node, "integer constant expression overflows");
  return r;
}

/* Value of an integer constant expression (literals, + - *, unary -,
   parentheses, integer casts). */
static long long int_constant(const json_value *node) {
  node = strip_parens_and_casts(node, kIntConstantCasts);
  const char *kind = kind_of(node);
  if (strcmp(kind, "IntegerLiteral") == 0) {
    const char *v = json_string(json_get(node, "value"), "0");
    errno = 0;
    long long r = strtoll(v, NULL, 10);
    if (errno == ERANGE)
      unsupported(node, "integer constant expression overflows");
    return r;
  }
  if (strcmp(kind, "DeclRefExpr") == 0)
    unsupported(node, "'%s' is not an integer constant (loop bounds and "
                      "steps must be static)", referenced_name(node));
  const char *op = opcode_of(node);
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "-") == 0 || strcmp(op, "+") == 0)) {
    long long v = int_constant(child(node, 0));
    return op[0] == '-' ? checked(0, v, '-', node) : v;
  }
  if (strcmp(kind, "BinaryOperator") == 0 &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0)) {
    long long a = int_constant(child(node, 0));
    long long b = int_constant(child(node, 1));
    return checked(a, b, op[0], node);
  }
  unsupported(node, "expected an integer constant expression (loop bounds "
                    "and steps must be static)");
}

static int try_int_constant(const json_value *node, long long *out) {
  jmp_buf saved;
  memcpy(saved, unsupported_jump, sizeof saved);
  int ok = 0;
  if (setjmp(unsupported_jump) == 0) {
    *out = int_constant(node);
    ok = 1;
  }
  memcpy(unsupported_jump, saved, sizeof saved);
  return ok;
}

static int is_ref_to(const json_value *node, const char *decl) {
  node = strip_parens_and_casts(node, kLValueToRValue);
  const char *id = referenced_id(node);
  return strcmp(kind_of(node), "DeclRefExpr") == 0 && id && decl &&
         strcmp(id, decl) == 0;
}

/* Whether `node` can modify the variable `decl`: assigned, incremented, or
   its address taken. */
static int writes_to(const json_value *node, const char *decl) {
  if (!node || node->kind != JSON_OBJECT)
    return 0;
  const char *kind = kind_of(node);
  const char *op = opcode_of(node);
  size_t n = child_count(node);
  size_t oplen = strlen(op);
  if ((strcmp(kind, "BinaryOperator") == 0 ||
       strcmp(kind, "CompoundAssignOperator") == 0) &&
      oplen && op[oplen - 1] == '=' && strcmp(op, "==") && strcmp(op, "!=") &&
      strcmp(op, "<=") && strcmp(op, ">=") && n &&
      is_ref_to(child(node, 0), decl))
    return 1;
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "++") == 0 || strcmp(op, "--") == 0 || strcmp(op, "&") == 0) &&
      n && is_ref_to(child(node, 0), decl))
    return 1;
  for (size_t i = 0; i < n; ++i)
    if (writes_to(child(node, i), decl))
      return 1;
  return 0;
}

/* ------------------------------------------------------------ linear forms */

static void form_add_term(linear_form *f, const char *id, long long c,
                          const json_value *node) {
  if (c == 0)
    return;
  for (size_t i = 0; i < f->n; ++i)
    if (strcmp(f->ids[i], id) == 0) {
      f->coef[i] = checked(f->coef[i], c, '+', node);
      if (f->coef[i] == 0) {
        f->ids[i] = f->ids[f->n - 1];
        f->coef[i] = f->coef[f->n - 1];
        --f->n;
      }
      return;
    }
  if (f->n == MAX_FORM)
    unsupported(node, "subscript uses too many loop counters");
  f->ids[f->n] = id;
  f->coef[f->n++] = c;
}

static linear_form form_scale(linear_form f, long long c,
                              const json_value *node) {
  linear_form out = {0};
  for (size_t i = 0; i < f.n; ++i)
    form_add_term(&out, f.ids[i], checked(f.coef[i], c, '*', node), node);
  out.constant = checked(f.constant, c, '*', node);
  return out;
}

static linear_form form_add(linear_form a, linear_form b,
                            const json_value *node) {
  for (size_t i = 0; i < b.n; ++i)
    form_add_term(&a, b.ids[i], b.coef[i], node);
  a.constant = checked(a.constant, b.constant, '+', node);
  return a;
}

/* *f := c * *f, out of line (form_scale's by-value forms would sit in every
   frame of affine_index's recursion, one per AST level: about 2 KB a level
   overflowed an 8 MB stack at some 3,800 terms, review of 2026-10-19). */
static __attribute__((noinline)) void form_scale_in_place(
    linear_form *f, long long c, const json_value *node) {
  *f = form_scale(*f, c, node);
}

/* *f := *f + sign * *g, out of line (form_scale_in_place). */
static __attribute__((noinline)) void form_add_in_place(
    linear_form *f, const linear_form *g, long long sign,
    const json_value *node) {
  *f = form_add(*f, form_scale(*g, sign, node), node);
}

/* An affine subscript: the text usable inside affine.load brackets and its
   linear form (built in *form, the right operand's of + and - on the
   heap: the recursion keeps no form in its frames). */
static char *affine_index(translator *tr, const json_value *node,
                          linear_form *form) {
  node = strip_parens_and_casts(node, kIndexCasts);
  const char *kind = kind_of(node);
  const char *op = opcode_of(node);
  if (strcmp(kind, "IntegerLiteral") == 0) {
    long long v = int_constant(node);
    memset(form, 0, sizeof *form);
    form->constant = v;
    return format("%lld", v);
  }
  if (strcmp(kind, "DeclRefExpr") == 0) {
    const char *decl = referenced_id(node);
    env_entry *e = env_find(tr, decl);
    if (e && e->kind == ENV_IV) {
      /* The form is over the counter's value (form_range takes its first and
         last values); the text is what the counter is in the emitted IR. */
      memset(form, 0, sizeof *form);
      form->ids[0] = e->decl;
      form->coef[0] = 1;
      form->n = 1;
      if (e->descending)
        return e->stride == 1 ? format("(%lld - %s)", e->origin, e->name)
                              : format("(%lld - %s * %lld)", e->origin, e->name,
                                       e->stride);
      return xstrdup(e->name);
    }
    if (e && e->kind == ENV_INTSCALAR && tr->in_subscript) {
      /* A signed integer parameter: an affine symbol of the nest, an
         additive run-time offset of the access (C evaluates r0 + i in int,
         where overflow is undefined, so the exact index arithmetic agrees
         with every defined execution). An unsigned one wraps around: not
         affine. */
      if (!e->is_signed)
        unsupported(node, "subscript uses the unsigned parameter '%s'; only "
                          "signed integer parameters can offset a subscript "
                          "(unsigned arithmetic wraps, which is not affine)",
                    referenced_name(node));
      memset(form, 0, sizeof *form);
      form->ids[0] = e->decl;
      form->coef[0] = 1;
      form->n = 1;
      return format("symbol(%s)", e->sym_name);
    }
    unsupported(node, "subscript uses '%s', which is not an enclosing loop "
                      "counter%s", referenced_name(node),
                tr->in_subscript ? " or a signed integer parameter" : "");
  }
  if (strcmp(kind, "UnaryOperator") == 0 && strcmp(op, "-") == 0) {
    char *t = affine_index(tr, child(node, 0), form);
    form_scale_in_place(form, -1, node);
    return format("-(%s)", t);
  }
  if (strcmp(kind, "BinaryOperator") == 0 &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)) {
    linear_form *fb = xmalloc(sizeof *fb);
    char *ta = affine_index(tr, child(node, 0), form);
    char *tb = affine_index(tr, child(node, 1), fb);
    form_add_in_place(form, fb, op[0] == '+' ? 1 : -1, node);
    free(fb);
    return format("(%s %s %s)", ta, op, tb);
  }
  if (strcmp(kind, "BinaryOperator") == 0 && strcmp(op, "*") == 0) {
    for (int first = 1; first >= 0; --first) {
      const json_value *cnode = child(node, first ? 0 : 1);
      const json_value *other = child(node, first ? 1 : 0);
      long long c;
      if (!try_int_constant(cnode, &c))
        continue;
      char *t = affine_index(tr, other, form);
      form_scale_in_place(form, c, node);
      return first ? format("(%lld * %s)", c, t) : format("(%s * %lld)", t, c);
    }
    unsupported(node, "subscript multiplies two non-constant terms; not an "
                      "affine expression");
  }
  unsupported(node, "subscript is not an affine expression of the loop "
                    "counters");
}

/* The linear form of a loop bound over the enclosing counters (affine_index's
   rules: counters, integer constants, + - and constant *), or 0 when the
   expression is not one. */
static int try_bound_form(translator *tr, const json_value *node,
                          linear_form *form) {
  jmp_buf saved;
  memcpy(saved, unsupported_jump, sizeof saved);
  int ok = 0;
  if (setjmp(unsupported_jump) == 0) {
    affine_index(tr, node, form);
    ok = 1;
  }
  memcpy(unsupported_jump, saved, sizeof saved);
  return ok;
}

/* The text of an affine.for bound given as a linear form over the enclosing
   counters: the constant alone, or affine_map<(d0, ...) -> (expr)>(%a, ...)
   over the counters the form reads (a counting-down counter's value is
   origin - stride * ordinal, substituted as affine_index does). */
static char *bound_text(translator *tr, const linear_form *f,
                        const json_value *node) {
  long long constant = f->constant;
  const char *names[MAX_FORM];
  long long coef[MAX_FORM];
  size_t n = 0;
  for (size_t i = 0; i < f->n; ++i) {
    env_entry *e = env_find(tr, f->ids[i]);
    long long c = f->coef[i];
    if (e->descending) {
      constant = checked(constant, checked(c, e->origin, '*', node), '+', node);
      c = checked(0, checked(c, e->stride, '*', node), '-', node);
    }
    if (c == 0)
      continue;
    names[n] = e->name;
    coef[n++] = c;
  }
  if (n == 0)
    return format("%lld", constant);
  char *dims = xstrdup(""), *args = xstrdup("");
  char *expr = format("%lld", constant);
  for (size_t i = 0; i < n; ++i) {
    unsigned long long magnitude =
        coef[i] < 0 ? 0ULL - (unsigned long long)coef[i]
                    : (unsigned long long)coef[i];
    dims = format("%s%sd%zu", dims, i ? ", " : "", i);
    args = format("%s%s%s", args, i ? ", " : "", names[i]);
    expr = format("%s %c d%zu * %llu", expr, coef[i] < 0 ? '-' : '+', i,
                  magnitude);
  }
  return format("affine_map<(%s) -> (%s)>(%s)", dims, expr, args);
}

/* ------------------------------------------------- exact iteration domains */

/* A nest whose loops are not a constant-extent parallelotope -- a loop whose
   two bounds have different counter terms (a triangle) -- has no closed
   form for the ranges of its subscripts: they are computed exactly by
   running the nest's counters, every loop's values evaluated at the outer
   counters' values, up to this many visited values; past it the function is
   refused (docs/AFFINE_GEMM_MAPPING.md section 19.13). */
enum { kEnumerationLimit = 1 << 22 };

static const char kEnumerationRefusal[] =
    "the iteration domain is not a parallelotope and running its loops "
    "takes more than 4194304 counter values: its exact subscript ranges are "
    "not computed";

static int env_has_nonbox(translator *tr) {
  for (size_t i = 0; i < tr->env_n; ++i)
    if (tr->env[i].kind == ENV_IV && tr->env[i].nonbox)
      return 1;
  return 0;
}

/* The value of a form over the enclosing counters at the current point of an
   enumeration (each counter's value in its entry's `cur`), exactly; 0 when
   it does not fit a long long. */
static int form_value(translator *tr, const linear_form *f, long long *out) {
  const __int128 kLimit = (__int128)1 << 120;
  __int128 v = f->constant;
  for (size_t i = 0; i < f->n; ++i) {
    env_entry *e = env_find(tr, f->ids[i]);
    if (e->kind != ENV_IV)
      continue;   /* a parameter's term: not a counter value (its symbolic
                     part is kept apart by form_split_symbols) */
    v += (__int128)f->coef[i] * e->cur;
    if (v > kLimit || v < -kLimit)
      return 0;
  }
  if (v > LLONG_MAX || v < LLONG_MIN)
    return 0;
  *out = (long long)v;
  return 1;
}

typedef void (*point_fn)(translator *tr, void *ctx);

/* Calls fn at every point of the nest of the environment's loop counters
   from entry `from` on (outermost first; each counter's value in its entry's
   `cur`): a loop with constant bounds runs first, first + step, ..., last; a
   shifted one (constant extent) the same plus its shift at the outer values;
   a non-box one from its lower to its upper bound at the outer values.
   *visited counts the values given to counters; past kEnumerationLimit the
   function is refused at `node`. */
static void enumerate_points(translator *tr, size_t from, point_fn fn,
                             void *ctx, size_t *visited,
                             const json_value *node) {
  size_t i = from;
  while (i < tr->env_n && tr->env[i].kind != ENV_IV)
    ++i;
  if (i == tr->env_n) {
    fn(tr, ctx);
    return;
  }
  env_entry *e = &tr->env[i];
  if (!e->has_values)
    return;
  const long long step = e->step;
  __int128 first, count;
  if (e->nonbox) {
    long long lo, hi;
    if (!form_value(tr, &e->lb_form, &lo) || !form_value(tr, &e->ub_form, &hi))
      unsupported(node, "integer constant expression overflows");
    if (e->has_ub2) {
      long long hi2;
      if (!form_value(tr, &e->ub2_form, &hi2))
        unsupported(node, "integer constant expression overflows");
      if (hi2 < hi)
        hi = hi2;
    }
    first = lo;
    count = hi > lo ? ((__int128)hi - 1 - lo) / step + 1 : 0;
  } else {
    long long shift = 0;
    if (e->shift.n && !form_value(tr, &e->shift, &shift))
      unsupported(node, "integer constant expression overflows");
    first = (__int128)e->first + shift;
    count = ((__int128)e->last - e->first) / step + 1;
  }
  for (__int128 t = 0; t < count; ++t) {
    if (++*visited > kEnumerationLimit)
      unsupported(node, "%s", kEnumerationRefusal);
    __int128 v = first + (__int128)step * t;
    if (v > LLONG_MAX || v < LLONG_MIN)
      unsupported(node, "integer constant expression overflows");
    e->cur = (long long)v;
    enumerate_points(tr, i + 1, fn, ctx, visited, node);
  }
}

typedef struct {
  const linear_form *forms;
  size_t n;
  long long *lo, *hi;
  int found;
  const json_value *node;
} range_ctx;

static void range_point(translator *tr, void *p) {
  range_ctx *c = p;
  for (size_t i = 0; i < c->n; ++i) {
    long long v;
    if (!form_value(tr, &c->forms[i], &v))
      unsupported(c->node, "integer constant expression overflows");
    if (!c->found || v < c->lo[i])
      c->lo[i] = v;
    if (!c->found || v > c->hi[i])
      c->hi[i] = v;
  }
  c->found = 1;
}

/* The exact ranges [lo, hi] of n forms over the points of the nest of the
   environment's loops, by running it; 0 when the nest has no point. */
static int forms_range_enumerated(translator *tr, const linear_form *forms,
                                  size_t n, long long *lo, long long *hi,
                                  const json_value *node) {
  range_ctx c = {forms, n, lo, hi, 0, node};
  size_t visited = 0;
  enumerate_points(tr, 0, range_point, &c, &visited, node);
  return c.found;
}

/* The range [lo, hi] of a form's values over the loops' iterations. Each
   counter with affine bounds is x = x' + shift(enclosing counters) with x'
   running first .. last: the shifts are substituted innermost first (a
   shift's own counters are substituted after it), which leaves a form over
   independently ranging values, extreme at their ends -- exact on a
   constant-extent parallelotope, not a hull. With a non-box loop in scope
   (bounds with different counter terms) the nest is run instead
   (forms_range_enumerated): exact on any domain. 0 when the form has no
   value: in closed form, when a loop whose counter it reads (shifts
   substituted) runs zero times -- a form that reads no such counter still
   gets its range although the nest then has no point (array_access gives
   such an access no interface range all the same: nest_has_point); a run of
   the nest returns 0 whenever the nest has no point. */
static int form_range(translator *tr, const linear_form *f, long long *lo,
                      long long *hi, const json_value *node) {
  if (env_has_nonbox(tr))
    return forms_range_enumerated(tr, f, 1, lo, hi, node);
  linear_form g = *f;
  for (size_t i = tr->env_n; i-- > 0;) {
    env_entry *e = &tr->env[i];
    if (e->kind != ENV_IV || e->shift.n == 0)
      continue;
    long long c = 0;
    for (size_t k = 0; k < g.n; ++k)
      if (strcmp(g.ids[k], e->decl) == 0)
        c = g.coef[k];
    if (c != 0)
      g = form_add(g, form_scale(e->shift, c, node), node);
  }
  long long l = g.constant, h = g.constant;
  for (size_t i = 0; i < g.n; ++i) {
    env_entry *e = env_find(tr, g.ids[i]);
    if (!e->has_values)
      return 0;
    long long a = checked(g.coef[i], e->first, '*', node);
    long long b = checked(g.coef[i], e->last, '*', node);
    l = checked(l, a < b ? a : b, '+', node);
    h = checked(h, a < b ? b : a, '+', node);
  }
  *lo = l;
  *hi = h;
  return 1;
}

/* ----------------------------------- integer expressions, node by node */

/* C evaluates every integer node of a loop header -- the counter's initial
   value, the bound, both sides of the condition -- and of a subscript in
   that node's own type, the one clang gives it (an operator's promoted or
   common type; every implicit conversion is a node of its own, of its
   target type): a signed result outside the type is undefined behaviour, an
   unsigned one wraps around, a conversion to a type that cannot represent
   the value changes it. The translation computes with exact integer values,
   so a loop or an access is translated only when every node's exact value
   lies in its type's range at every point where C evaluates it -- C's
   arithmetic is exact arithmetic there, unsigned included. A node is kept
   with its value, a * counter + form (a loop's own counter; the form over
   the enclosing counters, with the constant), and its type's range. */
typedef struct {
  const json_value *node;
  long long a;            /* the coefficient of the loop's own counter */
  linear_form form;       /* the rest: the enclosing counters, the constant */
  __int128 min, max;      /* the range of the node's C type */
  int conversion;         /* an implicit integer conversion to that type */
  int reads;              /* the node reads a loop counter */
  int symbolic;           /* the node reads an integer parameter (a symbol):
                             its exact value is not known here; signed
                             overflow would be undefined behaviour, so it is
                             not range-checked */
} header_node;

typedef struct {
  header_node *items;
  size_t n, cap;
} header_nodes;

enum { HEADER_INIT, HEADER_BOUND, HEADER_COND, HEADER_SUBSCRIPT };

/* The range of the C type of `node`: one of kIntTypes, or a bit-precise
   integer type of at most 64 bits, _BitInt(N) or unsigned _BitInt(N) (C23;
   clang gives a `wb` / `uwb` literal one, and arithmetic on two of them
   stays in it -- no integer promotion applies). 0 for another type. */
static int node_type_range(const json_value *node, __int128 *min,
                           __int128 *max) {
  char *t = strip_qualifiers(qual_type(node));
  unsigned long long m;
  int ok = int_type_max(t, &m);
  if (ok) {
    *max = (__int128)m;
    *min = strncmp(t, "unsigned", 8) == 0 ? 0 : -(__int128)m - 1;
  } else {
    const int is_unsigned = strncmp(t, "unsigned ", 9) == 0;
    const char *b = is_unsigned ? t + 9 : t;
    int width = 0, used = 0;
    if (sscanf(b, "_BitInt(%d)%n", &width, &used) == 1 && used > 0 &&
        b[used] == '\0' && width >= 1 && width <= 64) {
      ok = 1;
      if (is_unsigned) {
        *min = 0;
        *max = ((__int128)1 << width) - 1;
      } else {
        *max = ((__int128)1 << (width - 1)) - 1;
        *min = -*max - 1;
      }
    }
  }
  free(t);
  return ok;
}

/* -(a * counter + f), in place. Out of line, as header_binary and
   header_push are: header_expr and cond_side_form recurse once per AST
   level, and the forms these build by value would otherwise sit in each of
   their frames (about 2.3 KB a level overflowed an 8 MB stack at some 3,600
   terms, review of 2026-10-19). */
static __attribute__((noinline)) void header_negate(long long *a,
                                                    linear_form *f,
                                                    const json_value *n) {
  *f = form_scale(*f, -1, n);
  *a = checked(0, *a, '-', n);
}

/* *a, *f := the value of `n` = left OP right, from the left operand's (a0,
   *f on entry) and the right one's (a1, *f1); 0 when that is not affine. */
static __attribute__((noinline)) int header_binary(const char *op,
                                                   long long a0, long long a1,
                                                   const linear_form *f1,
                                                   long long *a, linear_form *f,
                                                   const json_value *n) {
  const linear_form f0 = *f;
  if (op[0] != '*') {
    const long long sign = op[0] == '+' ? 1 : -1;
    *f = form_add(f0, form_scale(*f1, sign, n), n);
    *a = checked(a0, checked(a1, sign, '*', n), '+', n);
  } else if (a0 == 0 && f0.n == 0) {
    *f = form_scale(*f1, f0.constant, n);
    *a = checked(a1, f0.constant, '*', n);
  } else if (a1 == 0 && f1->n == 0) {
    *f = form_scale(f0, f1->constant, n);
    *a = checked(a0, f1->constant, '*', n);
  } else {
    return 0;
  }
  return 1;
}

/* Appends node `n`, of value a * counter + *f, to *out with its type's
   range; refuses a type whose range is not known. */
static __attribute__((noinline)) void header_push(header_nodes *out,
                                                  const json_value *n,
                                                  long long a,
                                                  const linear_form *f,
                                                  int conversion, int reads) {
  header_node h;
  memset(&h, 0, sizeof h);
  h.node = n;
  h.a = a;
  h.form = *f;
  h.conversion = conversion;
  h.reads = reads;
  if (!node_type_range(n, &h.min, &h.max)) {
    char *t = strip_qualifiers(qual_type(n));
    unsupported(n, "integer arithmetic in the type '%s' is not supported", t);
  }
  if (out->n == out->cap) {
    out->cap = out->cap ? 2 * out->cap : 16;
    out->items = realloc(out->items, out->cap * sizeof *out->items);
    if (!out->items) {
      fprintf(stderr, "systolic-cfront: out of memory\n");
      exit(2);
    }
  }
  out->items[out->n++] = h;
}

/* Walks an integer expression of the loop counters -- literals, references
   to the loop's own counter `decl` (none may appear when it is NULL) and to
   enclosing counters, unary + / -, + -, multiplication by a constant,
   parentheses and implicit integer conversions: the shapes the parsers of a
   loop header and of a subscript accept -- and appends to *out, every
   operand before its operator, the nodes whose value can leave their type:
   the operations, the conversions and the loop's own counter, each with its
   type's range (a literal fits its type, and an enclosing counter's values
   fit its own, so neither is kept). The value of `n`, a * counter + form, in
   *a and *f; in *reads whether it reads a loop counter (its form may still
   be constant: k - i with k = i - 1). 0 when `n` is not of that shape. */
static int header_expr(translator *tr, const json_value *n, const char *decl,
                       header_nodes *out, long long *a, linear_form *f,
                       int *reads) {
  const char *kind = kind_of(n);
  const char *op = opcode_of(n);
  int conversion = 0, keep = 1;
  memset(f, 0, sizeof *f);
  *a = 0;
  *reads = 0;
  if (strcmp(kind, "ParenExpr") == 0)
    return header_expr(tr, child(n, 0), decl, out, a, f, reads);
  if (strcmp(kind, "ImplicitCastExpr") == 0) {
    const char *ck = json_string(json_get(n, "castKind"), "");
    if (strcmp(ck, "LValueToRValue") == 0 || strcmp(ck, "NoOp") == 0)
      return header_expr(tr, child(n, 0), decl, out, a, f, reads);
    if (strcmp(ck, "IntegralCast") != 0 ||
        !header_expr(tr, child(n, 0), decl, out, a, f, reads))
      return 0;
    conversion = 1;
  } else if (strcmp(kind, "IntegerLiteral") == 0) {
    errno = 0;
    f->constant = strtoll(json_string(json_get(n, "value"), "0"), NULL, 10);
    if (errno == ERANGE)
      return 0;
    keep = 0;
  } else if (strcmp(kind, "DeclRefExpr") == 0) {
    const char *id = referenced_id(n);
    env_entry *e = env_find(tr, id);
    if (decl && id && strcmp(id, decl) == 0) {
      *a = 1;
    } else if (e && e->kind == ENV_IV) {
      f->ids[0] = e->decl;
      f->coef[0] = 1;
      f->n = 1;
      keep = 0;
    } else if (e && e->kind == ENV_INTSCALAR && e->is_signed &&
               tr->in_subscript) {
      /* a signed integer parameter in a subscript: an affine symbol (its
         value is not known here; nodes that read it are not range-checked,
         see header_node.symbolic) */
      f->ids[0] = e->decl;
      f->coef[0] = 1;
      f->n = 1;
      keep = 0;
    } else {
      return 0;
    }
    *reads = 1;
  } else if (strcmp(kind, "UnaryOperator") == 0 &&
             (strcmp(op, "-") == 0 || strcmp(op, "+") == 0)) {
    if (!header_expr(tr, child(n, 0), decl, out, a, f, reads))
      return 0;
    if (op[0] == '-')
      header_negate(a, f, n);
  } else if (strcmp(kind, "BinaryOperator") == 0 &&
             (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
              strcmp(op, "*") == 0)) {
    /* the left operand's value in *f, the right one's on the heap */
    linear_form *f1 = xmalloc(sizeof *f1);
    long long a0, a1;
    int r0, r1;
    const int ok = header_expr(tr, child(n, 0), decl, out, &a0, f, &r0) &&
                   header_expr(tr, child(n, 1), decl, out, &a1, f1, &r1) &&
                   header_binary(op, a0, a1, f1, a, f, n);
    free(f1);
    if (!ok)
      return 0;
    *reads = r0 || r1;
  } else {
    return 0;
  }
  if (keep)
    header_push(out, n, *a, f, conversion, *reads);
  return 1;
}

static char *int128_text(__int128 v) {
  char buf[48];
  size_t i = sizeof buf;
  unsigned __int128 u = v < 0 ? -(unsigned __int128)v : (unsigned __int128)v;
  buf[--i] = 0;
  do {
    buf[--i] = (char)('0' + (int)(u % 10));
    u /= 10;
  } while (u);
  if (v < 0)
    buf[--i] = '-';
  return xstrdup(buf + i);
}

/* Refuses the loop for header node h, whose exact value v at some point where
   C evaluates it is outside its type's range. A node of the condition that
   reads a counter is reported at the condition, with the counter values it is
   evaluated for, k_first .. k_exit; one that reads none (a constant, checked
   as written) at the node. */
static _Noreturn void header_refuse(const header_node *h, int role, __int128 v,
                                    const json_value *cond, __int128 k_first,
                                    __int128 k_exit) {
  char *t = strip_qualifiers(qual_type(h->node));
  char *value = int128_text(v);
  if (role == HEADER_COND && h->reads) {
    char *first = int128_text(k_first), *last = int128_text(k_exit);
    if (h->conversion)
      unsupported(cond, "the loop condition converts %s to '%s', which cannot "
                        "represent it (it is evaluated for the counter values "
                        "%s .. %s)", value, t, first, last);
    unsupported(cond, "the loop condition could overflow (it is evaluated for "
                      "the counter values %s .. %s)", first, last);
  }
  const char *what = role == HEADER_INIT    ? "the loop counter's initial value"
                     : role == HEADER_BOUND ? "the loop bound"
                     : role == HEADER_COND  ? "the loop condition"
                                            : "the subscript";
  if (h->conversion)
    unsupported(h->node, "%s converts %s to '%s', which cannot represent it",
                what, value, t);
  unsupported(h->node, "%s %s '%s' (an operation's exact value is %s)", what,
              h->min == 0 ? "wraps around in" : "overflows", t, value);
}

/* *v = a * k + w exactly; 0 when that leaves 128 bits. */
static int header_affine(long long a, __int128 k, long long w, __int128 *v) {
  __int128 p;
  return !__builtin_mul_overflow((__int128)a, k, &p) &&
         !__builtin_add_overflow(p, (__int128)w, v);
}

/* Checks header nodes at the current point of an enumeration (the enclosing
   counters at their `cur` values), the loop's own counter at k_first and at
   k_exit -- each node is affine in it, so its values in between lie between
   those two. */
static void header_check_point(translator *tr, const header_nodes *nodes,
                               int role, __int128 k_first, __int128 k_exit,
                               const json_value *cond) {
  for (size_t i = 0; i < nodes->n; ++i) {
    const header_node *h = &nodes->items[i];
    long long w;
    if (!form_value(tr, &h->form, &w))
      unsupported(h->node, "integer constant expression overflows");
    for (int end = 0; end < (h->a ? 2 : 1); ++end) {
      __int128 v;
      if (!header_affine(h->a, end ? k_exit : k_first, w, &v))
        unsupported(h->node, "integer constant expression overflows");
      if (v < h->min || v > h->max)
        header_refuse(h, role, v, cond, k_first, k_exit);
    }
  }
}

/* Checks the nodes of `nodes` that read no counter at their values as written
   -- C evaluates such a node to one value wherever the code runs, and it is
   checked whether the code runs or not, as the other checks of a constant
   are. For a non-box loop's header that C never evaluates (its enclosing
   nest has no point), whose other nodes are not checked. */
static void header_check_constants(const header_nodes *nodes, int role,
                                   const json_value *cond) {
  for (size_t i = 0; i < nodes->n; ++i) {
    const header_node *h = &nodes->items[i];
    if (h->reads)
      continue;
    const __int128 v = h->form.constant;
    if (v < h->min || v > h->max)
      header_refuse(h, role, v, cond, 0, 0);
  }
}

/* Whether the loops in scope have a point in common, i.e. whether the code
   they enclose runs: every loop runs -- a non-box loop at some point of its
   enclosing nest, any other at every point (its trip count is the same at
   all of them). C evaluates nothing of code that never runs. */
static int nest_has_point(translator *tr) {
  for (size_t i = 0; i < tr->env_n; ++i)
    if (tr->env[i].kind == ENV_IV && !tr->env[i].has_values)
      return 0;
  return 1;
}

/* Checks the nodes of a loop header in closed form, by their exact ranges
   over the points of the enclosing nest (form_range: closed form on a
   parallelotope; one run of the nest, for all of them, when a non-box loop
   is in scope): the initial value's (`init`) and a bound's node at every
   point; a condition's node with the counter at its first value `first` (a
   form over the enclosing counters) and at the value that ends the loop,
   first + `step_trips` (the step times the trip count, the same at every
   point) -- each node is affine in the counter, so its values in between
   lie between those two. A node that reads a counter -- the loop's own one
   included, and also when its counter terms cancel (k - i with k = i - 1) --
   is checked only where the code runs: when an enclosing loop never runs
   (nest_has_point), C evaluates no such node. A node that reads none is
   checked as written, as the other checks of a constant loop are. */
static void header_check_closed(translator *tr, const header_nodes *init,
                                const header_nodes *rest, int rest_role,
                                const linear_form *first, __int128 step_trips,
                                const json_value *cond,
                                const json_value *loop) {
  const header_nodes *lists[2] = {init, rest};
  const int roles[2] = {HEADER_INIT, rest_role};
  const size_t total = 1 + init->n + rest->n;
  linear_form *forms = xmalloc(total * sizeof *forms);
  linear_form *open = xmalloc(total * sizeof *open);
  size_t *where = xmalloc(total * sizeof *where);
  long long *lo = xmalloc(total * sizeof *lo), *hi = xmalloc(total * sizeof *hi);
  long long *olo = xmalloc(total * sizeof *olo);
  long long *ohi = xmalloc(total * sizeof *ohi);
  int *found = xmalloc(total * sizeof *found);
  /* forms[0]: the first value (for the diagnostic); then every node's value,
     a condition's at the first value (at the exit value it is that plus
     a * step_trips) */
  size_t m = 0, nopen = 0;
  forms[m++] = *first;
  for (int l = 0; l < 2; ++l)
    for (size_t i = 0; i < lists[l]->n; ++i) {
      const header_node *h = &lists[l]->items[i];
      forms[m++] = h->a ? form_add(h->form, form_scale(*first, h->a, h->node),
                                   h->node)
                        : h->form;
    }
  for (size_t j = 0; j < m; ++j) {
    found[j] = 1;
    lo[j] = hi[j] = forms[j].constant;
    if (forms[j].n) {
      where[nopen] = j;
      open[nopen++] = forms[j];
    }
  }
  if (nopen && env_has_nonbox(tr)) {
    int any = forms_range_enumerated(tr, open, nopen, olo, ohi, loop);
    for (size_t v = 0; v < nopen; ++v) {
      found[where[v]] = any;
      if (any) {
        lo[where[v]] = olo[v];
        hi[where[v]] = ohi[v];
      }
    }
  } else {
    for (size_t v = 0; v < nopen; ++v)
      found[where[v]] = form_range(tr, &open[v], &lo[where[v]],
                                   &hi[where[v]], loop);
  }
  if (!nest_has_point(tr)) {
    if (forms[0].n)
      found[0] = 0;
    size_t q = 1;
    for (int l = 0; l < 2; ++l)
      for (size_t i = 0; i < lists[l]->n; ++i, ++q)
        if (lists[l]->items[i].reads)
          found[q] = 0;
  }
  const __int128 k_first = found[0] ? lo[0] : first->constant;
  const __int128 k_exit =
      (__int128)(found[0] ? hi[0] : first->constant) + step_trips;
  size_t j = 1;
  for (int l = 0; l < 2; ++l)
    for (size_t i = 0; i < lists[l]->n; ++i, ++j) {
      const header_node *h = &lists[l]->items[i];
      if (!found[j])
        continue;
      __int128 low = lo[j], high = hi[j];
      if (h->a) {
        __int128 d, low_exit, high_exit;
        if (__builtin_mul_overflow((__int128)h->a, step_trips, &d) ||
            __builtin_add_overflow(low, d, &low_exit) ||
            __builtin_add_overflow(high, d, &high_exit))
          unsupported(h->node, "integer constant expression overflows");
        if (low_exit < low)
          low = low_exit;
        if (high_exit > high)
          high = high_exit;
      }
      if (low < h->min)
        header_refuse(h, roles[l], low, cond, k_first, k_exit);
      if (high > h->max)
        header_refuse(h, roles[l], high, cond, k_first, k_exit);
    }
  free(forms);
  free(open);
  free(where);
  free(lo);
  free(hi);
  free(olo);
  free(ohi);
  free(found);
}

/* Whether a form has a term in an integer parameter (an affine symbol). */
static int form_has_symbol(translator *tr, const linear_form *f) {
  for (size_t i = 0; i < f->n; ++i) {
    env_entry *e = env_find(tr, f->ids[i]);
    if (e && e->kind == ENV_INTSCALAR)
      return 1;
  }
  return 0;
}

/* Removes the terms in parameters (symbols) from *f, in place, and returns
   them as a C expression of the parameters (" + (r0) * 2LL"; NULL when
   there are none): the range of the form is then lo + sym .. hi + sym at
   run time, with lo / hi the range of what remains (form_range). */
static char *form_split_symbols(translator *tr, linear_form *f) {
  char *symbolic = NULL;
  size_t n = 0;
  for (size_t i = 0; i < f->n; ++i) {
    env_entry *e = env_find(tr, f->ids[i]);
    if (e && e->kind == ENV_INTSCALAR) {
      char *more = format("%s + (%s) * %lldLL", symbolic ? symbolic : "",
                          e->cname, f->coef[i]);
      free(symbolic);
      symbolic = more;
      continue;
    }
    f->ids[n] = f->ids[i];
    f->coef[n] = f->coef[i];
    ++n;
  }
  f->n = n;
  return symbolic;
}

static access_info *access_of(translator *tr, const char *decl, int rank) {
  for (size_t i = 0; i < tr->acc_n; ++i)
    if (strcmp(tr->accesses[i].decl, decl) == 0)
      return &tr->accesses[i];
  if (tr->acc_n == tr->acc_cap) {
    tr->acc_cap = tr->acc_cap ? 2 * tr->acc_cap : 8;
    tr->accesses = realloc(tr->accesses, tr->acc_cap * sizeof *tr->accesses);
  }
  access_info *a = &tr->accesses[tr->acc_n++];
  memset(a, 0, sizeof *a);
  a->decl = decl;
  a->rank = rank;
  return a;
}

/* --modulo-subscripts: when the subscript `ix` is E % C (parentheses and
   integer conversions around it), C a positive integer constant, the node E
   and C in *modulus; else NULL. Every conversion between `ix` and the % must
   hold the values 0 .. C - 1 (the remainder of a nonnegative E). */
static const json_value *modulo_operand(const json_value *ix,
                                        long long *modulus) {
  const json_value *n = strip_parens_and_casts(ix, kIndexCasts);
  if (strcmp(kind_of(n), "BinaryOperator") != 0 ||
      strcmp(opcode_of(n), "%") != 0)
    return NULL;
  long long c;
  if (!try_int_constant(child(n, 1), &c))
    unsupported(n, "the right operand of %% in a subscript is not an integer "
                   "constant");
  if (c <= 0)
    unsupported(n, "the right operand of %% in a subscript is %lld; only a "
                   "positive constant is supported", c);
  for (const json_value *cur = ix; cur != n; cur = child(cur, 0)) {
    __int128 min, max;
    if (strcmp(kind_of(cur), "ParenExpr") == 0)
      continue;
    if (!node_type_range(cur, &min, &max) || min > 0 || max < c - 1)
      unsupported(cur, "a conversion of the remainder in this subscript "
                       "cannot hold every value 0 .. %lld", c - 1);
  }
  *modulus = c;
  return child(n, 0);
}

typedef struct {
  const linear_form *form;
  long long modulus;
  long long lo, hi;
  int found;
  const json_value *node;
} mod_range_ctx;

static void mod_range_point(translator *tr, void *p) {
  mod_range_ctx *c = p;
  long long v;
  if (!form_value(tr, c->form, &v))
    unsupported(c->node, "integer constant expression overflows");
  if (v < 0)
    unsupported(c->node, "the left operand of %% in this subscript is %lld at "
                         "a point of the nest; C's %% of a negative value is "
                         "not the floor modulo of an affine map", v);
  v %= c->modulus;
  if (!c->found || v < c->lo)
    c->lo = v;
  if (!c->found || v > c->hi)
    c->hi = v;
  c->found = 1;
}

/* The exact range of (form mod modulus) over the points of the nest, by
   running it; refuses a point where the form is negative. 0 when the nest
   has no point. */
static int mod_range_enumerated(translator *tr, const linear_form *form,
                                long long modulus, long long *lo,
                                long long *hi, const json_value *node) {
  mod_range_ctx c = {form, modulus, 0, 0, 0, node};
  size_t visited = 0;
  enumerate_points(tr, 0, mod_range_point, &c, &visited, node);
  *lo = c.lo;
  *hi = c.hi;
  return c.found;
}

/* --scatter-subscripts: whether the subscript is affine, or E % C under
   --modulo-subscripts, without abandoning the function when it is not. */
static int subscript_is_affine(translator *tr, const json_value *ix) {
  jmp_buf saved;
  memcpy(saved, unsupported_jump, sizeof saved);
  int ok = 0;
  if (setjmp(unsupported_jump) == 0) {
    long long mod = 0;
    const json_value *e_node = tr->modulo_subscripts ? modulo_operand(ix, &mod) : NULL;
    linear_form form;
    affine_index(tr, e_node ? e_node : ix, &form);
    ok = 1;
  }
  memcpy(unsupported_jump, saved, sizeof saved);
  return ok;
}

/* --scatter-subscripts: an index table, read at a general subscript. */
static env_entry *general_table(translator *tr, const json_value *node) {
  const json_value *base = strip_parens_and_casts(
      strip_parens_and_casts(child(node, 0), kDecayNoOp), kLoadNoOp);
  env_entry *t = strcmp(kind_of(base), "DeclRefExpr") == 0
                     ? env_find(tr, referenced_id(base)) : NULL;
  if (!t || t->kind != ENV_MEMREF || !t->index_table || t->rank != 1)
    unsupported(node, "a general subscript reads an array that is no "
                      "one-dimensional integer array parameter");
  if (!tr->static_extents || t->declared_extent <= 0)
    unsupported(node, "the integer array '%s' is read at a general subscript, "
                      "and its bound is not written as an integer literal",
                referenced_name(base));
  return t;
}

static const char *general_binary(const char *op) {
  static const char *const ops[][2] = {
      {"+", "addi"}, {"-", "subi"}, {"*", "muli"}, {"/", "divsi"},
      {"%", "remsi"}, {"&", "andi"}, {"|", "ori"}, {"^", "xori"},
      {"<<", "shli"}, {">>", "shrsi"}};
  for (size_t i = 0; i < sizeof ops / sizeof *ops; ++i)
    if (strcmp(op, ops[i][0]) == 0)
      return ops[i][1];
  return NULL;
}

static const char *general_compare(const char *op) {
  static const char *const ops[][2] = {{"<", "slt"}, {"<=", "sle"}, {">", "sgt"},
                                       {">=", "sge"}, {"==", "eq"}, {"!=", "ne"}};
  for (size_t i = 0; i < sizeof ops / sizeof *ops; ++i)
    if (strcmp(op, ops[i][0]) == 0)
      return ops[i][1];
  return NULL;
}

/* --scatter-subscripts: a general subscript as C text for the shim (every
   value as long long, a counter as __systolic_cc_iv_NAME), checking that it
   is inside the subset; the counters it reads are added to `ivs`. */
static char *general_text(translator *tr, const json_value *node, strvec *ivs) {
  node = strip_parens_and_casts(node, kIndexCasts);
  const char *kind = kind_of(node), *op = opcode_of(node);
  char *type = strip_qualifiers(qual_type(node));
  if (strstr(type, "unsigned") && strcmp(kind, "ArraySubscriptExpr") != 0)
    unsupported(node, "a general subscript computes in an unsigned type, "
                      "which wraps");
  if (strcmp(kind, "IntegerLiteral") == 0)
    return format("%lldLL", int_constant(node));
  if (strcmp(kind, "DeclRefExpr") == 0) {
    env_entry *e = env_find(tr, referenced_id(node));
    if (e && e->kind == ENV_IV) {
      if (e->shift.n || e->nonbox)
        unsupported(node, "a general subscript reads a counter whose bounds "
                          "read another");
      /* "decl id|C name": the shim's loop runs the counter by its C name */
      char *entry = format("%s|%s", e->decl, referenced_name(node));
      int seen = 0;
      for (size_t i = 0; i < ivs->n; ++i)
        seen |= strcmp(ivs->items[i], entry) == 0;
      if (!seen)
        strvec_push(ivs, entry);
      return format("__systolic_cc_iv_%s", referenced_name(node));
    }
    if (e && e->kind == ENV_INTSCALAR && e->is_signed)
      return format("((long long)%s)", referenced_name(node));
    unsupported(node, "a general subscript reads '%s', which is no loop "
                      "counter or signed integer parameter",
                referenced_name(node));
  }
  if (strcmp(kind, "ArraySubscriptExpr") == 0) {
    env_entry *t = general_table(tr, node);
    char *ix = general_text(tr, child(node, 1), ivs);
    return format("((long long)%s[%s])", t->cname, ix);
  }
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "-") == 0 || strcmp(op, "~") == 0 || strcmp(op, "+") == 0))
    return format("(%s%s)", op, general_text(tr, child(node, 0), ivs));
  if (strcmp(kind, "BinaryOperator") == 0 && general_binary(op)) {
    char *a = general_text(tr, child(node, 0), ivs);
    char *b = general_text(tr, child(node, 1), ivs);
    return format("(%s %s %s)", a, op, b);
  }
  if (strcmp(kind, "ConditionalOperator") == 0) {
    const json_value *cond = strip_parens_and_casts(child(node, 0), kIndexCasts);
    if (strcmp(kind_of(cond), "BinaryOperator") != 0 ||
        !general_compare(opcode_of(cond)))
      unsupported(cond, "the condition of ?: in a general subscript is no "
                        "comparison of integers");
    char *a = general_text(tr, child(cond, 0), ivs);
    char *b = general_text(tr, child(cond, 1), ivs);
    char *t = general_text(tr, child(node, 1), ivs);
    char *f = general_text(tr, child(node, 2), ivs);
    return format("((%s %s %s) ? %s : %s)", a, opcode_of(cond), b, t, f);
  }
  unsupported(node, "a general subscript uses %s, which is outside the subset",
              op[0] ? op : kind);
}

/* The memref entry of a fully subscripted array element, and its subscripts
   joined by ", ". Records the access (read / written, the exact range of each
   subscript) for the function's interface. */
static env_entry *array_access(translator *tr, const json_value *node,
                               int read, int written, char **subscripts) {
  const json_value *indices[MAX_RANK + 1];
  size_t count = 0;
  const json_value *cur = node;
  for (;;) {
    cur = strip_parens_and_casts(cur, kDecayNoOp);
    if (strcmp(kind_of(cur), "ArraySubscriptExpr") != 0)
      break;
    if (count == MAX_RANK + 1)
      unsupported(node, "too many subscripts");
    indices[count++] = child(cur, 1);
    cur = child(cur, 0);
  }
  const json_value *base = strip_parens_and_casts(cur, kLoadNoOp);
  if (strcmp(kind_of(base), "DeclRefExpr") != 0)
    unsupported(node, "array accesses must subscript a parameter directly "
                      "(no pointer arithmetic)");
  env_entry *e = env_find(tr, referenced_id(base));
  if (!e || e->kind != ENV_MEMREF)
    unsupported(base, "'%s' is not an array parameter", referenced_name(base));
  if ((int)count != e->rank)
    unsupported(node, "'%s' has %d dimensions but is subscripted %zu times",
                referenced_name(base), e->rank, count);
  if (e->index_table && (written || !tr->in_gather))
    unsupported(node, "the integer array '%s' is read only as the leading "
                      "subscript of another array's read (A[%s[i]][k])",
                referenced_name(base), referenced_name(base));
  /* --scatter-subscripts: a written array at a general subscript. */
  if (written && tr->scatter_subscripts && !tr->in_gather) {
    int general = 0;
    tr->in_subscript = 1;
    for (size_t d = 0; d < count && !general; ++d)
      general = !subscript_is_affine(tr, indices[count - 1 - d]);
    /* Only an expression general_text takes is a general subscript; for any
       other the affine path below says what it always said. */
    strvec ivs = {0};
    char *texts[MAX_RANK];
    if (general) {
      jmp_buf saved;
      memcpy(saved, unsupported_jump, sizeof saved);
      if (setjmp(unsupported_jump) == 0) {
        for (size_t d = 0; d < count; ++d)
          texts[d] = general_text(tr, indices[count - 1 - d], &ivs);
      } else {
        general = 0;
      }
      memcpy(unsupported_jump, saved, sizeof saved);
    }
    if (general) {
      if (!tr->static_extents || e->declared_extent <= 0)
        unsupported(node, "'%s' is written at a general subscript, and its "
                          "leading bound is not written as an integer literal "
                          "(float %s[16][...])",
                    referenced_name(base), referenced_name(base));
      if (env_has_nonbox(tr))
        unsupported(node, "a general subscript in a nest whose bounds read "
                          "the enclosing counters");
      tr->in_subscript = 0;
      access_info *info = access_of(tr, e->decl, e->rank);
      if (info->scatter_json)
        unsupported(node, "'%s' is written at general subscripts twice",
                    referenced_name(base));
      info->read |= read;
      info->written = 1;
      info->gathered = 1;
      info->gather_extent = e->declared_extent;
      char *loops = xstrdup("");
      for (size_t i = 0; i < ivs.n; ++i) {
        const char *bar = strchr(ivs.items[i], '|');
        char *decl = format("%.*s", (int)(bar - ivs.items[i]), ivs.items[i]);
        env_entry *iv = env_find(tr, decl);
        char *one = format("%s{\"name\": \"__systolic_cc_iv_%s\", \"runs\": %s, "
                           "\"first\": %lld, \"last\": %lld, \"step\": %lld}",
                           i ? ", " : "", bar + 1,
                           iv->has_values ? "true" : "false", iv->first, iv->last,
                           iv->step);
        char *joined = format("%s%s", loops, one);
        free(loops);
        free(one);
        loops = joined;
      }
      char *subs = xstrdup("");
      for (size_t d = 0; d < count; ++d) {
        char *joined = format("%s%s\"%s\"", subs, d ? ", " : "", texts[d]);
        free(subs);
        subs = joined;
      }
      info->scatter_json = format("\"scatter\": {\"loops\": [%s], \"subscripts\": [%s]}",
                                  loops, subs);
      tr->general.active = 1;
      tr->general.count = (int)count;
      for (size_t d = 0; d < count; ++d)
        tr->general.nodes[d] = indices[count - 1 - d];
      *subscripts = xstrdup("@@general@@");
      return e;
    }
    tr->in_subscript = 0;
  }

  char *texts[MAX_RANK];
  linear_form forms[MAX_RANK];
  /* --modulo-subscripts: a subscript E % C is E's form, its node E, and C */
  long long mods[MAX_RANK] = {0};
  const json_value *operands[MAX_RANK];
  /* --gather-subscripts: the leading subscript is T[E] (gather_table_of) */
  int gathered = 0;
  env_entry *table = NULL;
  char *table_index = NULL;
  tr->in_subscript = 1;   /* a signed integer parameter is a symbol here */
  for (size_t d = 0; d < count; ++d) {   /* outermost subscript first */
    const json_value *ix = indices[count - 1 - d];
    if (d == 0 && tr->gather_subscripts && !tr->in_gather) {
      const json_value *t = strip_parens_and_casts(ix, kIndexCasts);
      const json_value *tbase = NULL;
      if (strcmp(kind_of(t), "ArraySubscriptExpr") == 0)
        tbase = strip_parens_and_casts(
            strip_parens_and_casts(child(t, 0), kDecayNoOp), kLoadNoOp);
      env_entry *te = tbase && strcmp(kind_of(tbase), "DeclRefExpr") == 0
                          ? env_find(tr, referenced_id(tbase)) : NULL;
      if (te && te->kind == ENV_MEMREF && te->index_table) {
        if (written)
          unsupported(ix, "a gathered subscript in an array that is written; "
                          "the stores stay affine");
        if (!tr->static_extents || e->declared_extent <= 0)
          unsupported(node, "'%s' is read at a gathered row, and its leading "
                            "bound is not written as an integer literal "
                            "(float %s[32][...])",
                      referenced_name(base), referenced_name(base));
        if (env_has_nonbox(tr))
          unsupported(ix, "a gathered subscript in a nest whose bounds read "
                          "the enclosing counters");
        /* T[E]: an access of its own, one level (no gather inside). */
        tr->in_subscript = 0;
        tr->in_gather = 1;
        table = array_access(tr, t, 1, 0, &table_index);
        tr->in_gather = 0;
        tr->in_subscript = 1;
        gathered = 1;
        memset(&forms[0], 0, sizeof forms[0]);
        operands[0] = NULL;
        texts[0] = xstrdup("@@gather@@");
        continue;
      }
    }
    const json_value *e_node =
        tr->modulo_subscripts ? modulo_operand(ix, &mods[d]) : NULL;
    operands[d] = e_node ? e_node : ix;
    char *t = affine_index(tr, operands[d], &forms[d]);
    if (e_node) {
      if (form_has_symbol(tr, &forms[d]))
        unsupported(ix, "the left operand of %% in a subscript reads a "
                        "parameter; only loop counters and constants are "
                        "supported there");
      texts[d] = format("((%s) mod %lld)", t, mods[d]);
      free(t);
    } else {
      texts[d] = t;
    }
  }

  /* Every operation and conversion of the subscripts in its C type
     (header_expr): C computes a subscript in its own type, so one that
     wraps around or overflows addresses another element than its exact
     value -- the ranges of those nodes are computed with the subscripts'. */
  header_nodes sub = {0};
  for (size_t d = 0; d < count; ++d) {
    if (gathered && d == 0)
      continue;
    if (gathered && (mods[d] || form_has_symbol(tr, &forms[d])))
      unsupported(operands[d], "a subscript beside a gathered one reads a "
                               "parameter or takes a remainder");
    for (size_t f = 0; gathered && f < forms[d].n; ++f) {
      env_entry *iv = env_find(tr, forms[d].ids[f]);
      if (iv && iv->shift.n)
        unsupported(operands[d], "a subscript beside a gathered one reads a "
                                 "counter whose bounds read another");
    }
    long long a_unused;
    linear_form f_unused;
    int r_unused;
    if (!header_expr(tr, operands[d], NULL, &sub, &a_unused,
                     &f_unused, &r_unused))
      unsupported(operands[d], "subscript is not an affine "
                               "expression of the loop counters");
  }
  tr->in_subscript = 0;
  /* A node that reads a parameter has no exact value here (header_node.
     symbolic): C evaluates it in a signed type, where overflow is undefined,
     so every defined execution agrees with the exact index arithmetic. */
  for (size_t i = 0; i < sub.n; ++i)
    sub.items[i].symbolic = form_has_symbol(tr, &sub.items[i].form);
  /* The subscripts' terms in parameters, apart: their ranges below are
     those of the loop part, offset by these at run time. */
  char *sym[MAX_RANK] = {0};
  for (size_t d = 0; d < count; ++d)
    sym[d] = form_split_symbols(tr, &forms[d]);
  const size_t nforms = count + sub.n;
  linear_form *all_forms = xmalloc(nforms * sizeof *all_forms);
  long long *all_lo = xmalloc(nforms * sizeof *all_lo);
  long long *all_hi = xmalloc(nforms * sizeof *all_hi);
  int *found = xmalloc(nforms * sizeof *found);
  for (size_t d = 0; d < count; ++d)
    all_forms[d] = forms[d];
  for (size_t i = 0; i < sub.n; ++i)
    all_forms[count + i] = sub.items[i].form;
  if (env_has_nonbox(tr)) {
    /* one run of the nest for every subscript of the access */
    int any = forms_range_enumerated(tr, all_forms, nforms, all_lo, all_hi,
                                     node);
    for (size_t j = 0; j < nforms; ++j)
      found[j] = any;
  } else {
    for (size_t j = 0; j < nforms; ++j)
      found[j] = form_range(tr, &all_forms[j], &all_lo[j], &all_hi[j], node);
  }
  /* A subscript E % C: E >= 0 at every point, and the exact range of the
     remainder, both by running the nest. */
  for (size_t d = 0; d < count; ++d)
    if (mods[d])
      found[d] = mod_range_enumerated(tr, &forms[d], mods[d], &all_lo[d],
                                      &all_hi[d], indices[count - 1 - d]);
  /* In an access that never runs C evaluates no node that reads a counter,
     and it touches no element: it gives the interface no range (systolic-cc
     sizes the memref descriptor and its copies of the caller's array by the
     extent, review of 2026-10-19). A node that reads none is checked as
     written, wherever it is (header_check_constants). */
  const int reached = nest_has_point(tr);
  for (size_t i = 0; i < sub.n; ++i) {
    const header_node *h = &sub.items[i];
    if (h->symbolic)
      continue;
    __int128 low = h->form.constant, high = h->form.constant;
    if (h->reads) {
      if (!found[count + i] || !reached)
        continue;
      low = all_lo[count + i];
      high = all_hi[count + i];
    }
    if (low < h->min)
      header_refuse(h, HEADER_SUBSCRIPT, low, NULL, 0, 0);
    if (high > h->max)
      header_refuse(h, HEADER_SUBSCRIPT, high, NULL, 0, 0);
  }
  access_info *info = access_of(tr, e->decl, e->rank);
  info->read |= read;
  info->written |= written;
  if (gathered) {
    /* No row range: the rows are the table's entries, checked by the shim
       at every call against the literal bound, the leading extent. */
    if (info->gathered && strcmp(info->gather_table_decl, table->decl) != 0)
      unsupported(node, "'%s' is gathered by two tables",
                  referenced_name(base));
    info->gathered = 1;
    info->gather_table_decl = table->decl;
    info->gather_table = table->cname;
    info->gather_extent = e->declared_extent;
    tr->gather.active = 1;
    tr->gather.table = table;
    tr->gather.table_index = table_index;
    tr->gather.count = (int)count;
    for (size_t d = 0; d < count; ++d)
      tr->gather.forms[d] = forms[d];
  }
  long long lo[MAX_RANK], hi[MAX_RANK];
  int all = reached && !gathered;
  for (size_t d = 0; d < count; ++d) {
    all = all && found[d];
    if (found[d]) {
      lo[d] = all_lo[d];
      hi[d] = all_hi[d];
    }
  }
  free(all_forms);
  free(all_lo);
  free(all_hi);
  free(found);
  free(sub.items);
  if (all) {
    /* The interface gives the leading dimension's extent, the last row
       reached plus one (interface()). */
    if (count && hi[0] == LLONG_MAX)
      unsupported(node, "the access reaches row %lld of '%s', so the array's "
                        "extent does not fit in 64 bits",
                  hi[0], referenced_name(base));
    /* The leading subscript's range as a C expression of the parameters:
       the extreme over the accesses, each "constant + symbolic terms".
       Built before info->lo / hi take this access in: the fold of the
       earlier static accesses below must not see this access's loop part
       without its symbolic terms (an extreme too large, past the caller's
       array when the terms are negative). */
    if (count && (sym[0] || info->lo_expr)) {
      char *lo_e = format("(%lldLL%s)", lo[0], sym[0] ? sym[0] : "");
      char *hi_e = format("(%lldLL%s)", hi[0], sym[0] ? sym[0] : "");
      if (!info->lo_expr) {
        if (info->has_ranges) {
          /* earlier accesses were static: fold their range in */
          char *l2 = format("((%lldLL) < %s ? (%lldLL) : %s)", info->lo[0], lo_e,
                            info->lo[0], lo_e);
          char *h2 = format("((%lldLL) > %s ? (%lldLL) : %s)", info->hi[0], hi_e,
                            info->hi[0], hi_e);
          free(lo_e); free(hi_e);
          lo_e = l2; hi_e = h2;
        }
        info->lo_expr = lo_e;
        info->hi_expr = hi_e;
      } else {
        char *l2 = format("(%s < %s ? %s : %s)", info->lo_expr, lo_e,
                          info->lo_expr, lo_e);
        char *h2 = format("(%s > %s ? %s : %s)", info->hi_expr, hi_e,
                          info->hi_expr, hi_e);
        free(info->lo_expr); free(info->hi_expr); free(lo_e); free(hi_e);
        info->lo_expr = l2;
        info->hi_expr = h2;
      }
    }
    for (size_t d = 0; d < count; ++d) {
      if (!info->has_ranges || lo[d] < info->lo[d])
        info->lo[d] = lo[d];
      if (!info->has_ranges || hi[d] > info->hi[d])
        info->hi[d] = hi[d];
    }
    info->has_ranges = 1;
  }
  for (size_t d = 0; d < count; ++d)
    free(sym[d]);

  size_t len = 1;
  for (size_t d = 0; d < count; ++d)
    len += strlen(texts[d]) + 2;
  char *joined = xmalloc(len);
  joined[0] = 0;
  for (size_t d = 0; d < count; ++d) {
    if (d)
      strcat(joined, ", ");
    strcat(joined, texts[d]);
  }
  *subscripts = joined;
  return e;
}

/* ------------------------------------------------------------- expressions */

static char *float_constant(double value, const char *elem) {
  if (strcmp(elem, "f32") == 0) {
    float f = (float)value;
    uint32_t bits;
    memcpy(&bits, &f, sizeof bits);
    return format("0x%08X", bits);
  }
  uint64_t bits;
  memcpy(&bits, &value, sizeof bits);
  return format("0x%016llX", (unsigned long long)bits);
}

static char *arith(translator *tr, char opcode, const char *a, const char *b,
                   const char *elem, const json_value *node, int indent) {
  const char *op = opcode == '+' ? "addf" : opcode == '-' ? "subf"
                 : opcode == '*' ? "mulf" : "divf";
  char *name = fresh(tr);
  emit(tr, indent, "%s = arith.%s %s, %s%s : %s %s", name, op, a, b,
       tr->fp_reassociation && opcode != '/' ? " fastmath<reassoc>" : "", elem,
       loc_text(tr, node));
  return name;
}

static char *convert(translator *tr, char *value, const char *src,
                     const char *dst, const json_value *node, int indent) {
  if (strcmp(src, dst) == 0)
    return value;
  const char *op = strcmp(src, "f32") == 0 && strcmp(dst, "f64") == 0
                       ? "extf" : "truncf";
  char *name = fresh(tr);
  emit(tr, indent, "%s = arith.%s %s : %s to %s %s", name, op, value, src, dst,
       loc_text(tr, node));
  return name;
}

/* --scatter-subscripts: a general subscript as index arithmetic at `indent`;
   returns its SSA name (general_text has checked it). */
static char *general_value(translator *tr, const json_value *node, int indent) {
  node = strip_parens_and_casts(node, kIndexCasts);
  const char *kind = kind_of(node), *op = opcode_of(node);
  char *v = NULL;
  if (strcmp(kind, "IntegerLiteral") == 0) {
    v = fresh(tr);
    emit(tr, indent, "%s = arith.constant %lld : index %s", v, int_constant(node),
         loc_text(tr, node));
    return v;
  }
  if (strcmp(kind, "DeclRefExpr") == 0) {
    env_entry *e = env_find(tr, referenced_id(node));
    if (e->kind == ENV_INTSCALAR)
      return xstrdup(e->sym_name);
    if (!e->descending)
      return xstrdup(e->name);
    linear_form f;
    affine_index(tr, node, &f);
    char *b = bound_text(tr, &f, node);
    v = fresh(tr);
    emit(tr, indent, "%s = affine.apply %s %s", v, b, loc_text(tr, node));
    return v;
  }
  if (strcmp(kind, "ArraySubscriptExpr") == 0) {
    env_entry *t = general_table(tr, node);
    char *ix = general_value(tr, child(node, 1), indent);
    access_info *info = access_of(tr, t->decl, 1);
    info->read = 1;
    info->gathered = 1;
    info->gather_extent = t->declared_extent;
    char *raw = fresh(tr);
    emit(tr, indent, "%s = memref.load %s[%s] : %s %s", raw, t->name, ix, t->type,
         loc_text(tr, node));
    v = fresh(tr);
    emit(tr, indent, "%s = arith.index_cast%s %s : %s to index %s", v,
         t->is_signed ? "" : "ui", raw, t->elem, loc_text(tr, node));
    return v;
  }
  if (strcmp(kind, "UnaryOperator") == 0) {
    char *a = general_value(tr, child(node, 0), indent);
    if (strcmp(op, "+") == 0)
      return a;
    char *k = fresh(tr);
    emit(tr, indent, "%s = arith.constant %d : index %s", k,
         strcmp(op, "-") == 0 ? 0 : -1, loc_text(tr, node));
    v = fresh(tr);
    if (strcmp(op, "-") == 0)
      emit(tr, indent, "%s = arith.subi %s, %s : index %s", v, k, a, loc_text(tr, node));
    else
      emit(tr, indent, "%s = arith.xori %s, %s : index %s", v, a, k, loc_text(tr, node));
    return v;
  }
  if (strcmp(kind, "BinaryOperator") == 0) {
    char *a = general_value(tr, child(node, 0), indent);
    char *b = general_value(tr, child(node, 1), indent);
    v = fresh(tr);
    emit(tr, indent, "%s = arith.%s %s, %s : index %s", v, general_binary(op), a, b,
         loc_text(tr, node));
    return v;
  }
  /* ConditionalOperator */
  const json_value *cond = strip_parens_and_casts(child(node, 0), kIndexCasts);
  char *a = general_value(tr, child(cond, 0), indent);
  char *b = general_value(tr, child(cond, 1), indent);
  char *c = fresh(tr);
  emit(tr, indent, "%s = arith.cmpi %s, %s, %s : index %s", c,
       general_compare(opcode_of(cond)), a, b, loc_text(tr, node));
  char *t = general_value(tr, child(node, 1), indent);
  char *f = general_value(tr, child(node, 2), indent);
  v = fresh(tr);
  emit(tr, indent, "%s = arith.select %s, %s, %s : index %s", v, c, t, f,
       loc_text(tr, node));
  return v;
}

static char *load(translator *tr, const json_value *node, int indent,
                  const char **elem) {
  char *idx;
  env_entry *e = array_access(tr, node, 1, 0, &idx);
  if (!tr->gather.active) {
    char *name = fresh(tr);
    emit(tr, indent, "%s = affine.load %s[%s] : %s %s", name, e->name, idx,
         e->type, loc_text(tr, node));
    *elem = e->elem;
    return name;
  }
  /* --gather-subscripts: row = T[E] as an index, the other subscripts as
     index values (a counter as it is, else affine.apply over the counters),
     and a memref.load: a loaded value is no affine dimension. */
  tr->gather.active = 0;
  env_entry *t = tr->gather.table;
  char *raw = fresh(tr);
  emit(tr, indent, "%s = affine.load %s[%s] : %s %s", raw, t->name,
       tr->gather.table_index, t->type, loc_text(tr, node));
  char *row = fresh(tr);
  emit(tr, indent, "%s = arith.index_cast%s %s : %s to index %s", row,
       t->is_signed ? "" : "ui", raw, t->elem, loc_text(tr, node));
  char *list = xstrdup(row);
  for (int d = 1; d < tr->gather.count; ++d) {
    const linear_form *f = &tr->gather.forms[d];
    char *v = NULL;
    if (f->n == 1 && f->coef[0] == 1 && f->constant == 0) {
      env_entry *iv = env_find(tr, f->ids[0]);
      if (iv && iv->kind == ENV_IV && !iv->descending)
        v = xstrdup(iv->name);
    }
    if (!v) {
      char *b = bound_text(tr, f, node);
      v = fresh(tr);
      if (strncmp(b, "affine_map", 10) == 0)
        emit(tr, indent, "%s = affine.apply %s %s", v, b, loc_text(tr, node));
      else
        emit(tr, indent, "%s = arith.constant %s : index %s", v, b,
             loc_text(tr, node));
      free(b);
    }
    char *joined = format("%s, %s", list, v);
    free(list);
    free(v);
    list = joined;
  }
  char *name = fresh(tr);
  emit(tr, indent, "%s = memref.load %s[%s] : %s %s", name, e->name, list,
       e->type, loc_text(tr, node));
  free(list);
  free(tr->gather.table_index);
  tr->gather.table_index = NULL;
  *elem = e->elem;
  return name;
}

/* Emit `node` (an rvalue of floating type); returns its SSA name, its type in
   *elem. */
static char *value(translator *tr, const json_value *node, int indent,
                   const char **elem_out) {
  const char *kind = kind_of(node);
  char *t = strip_qualifiers(qual_type(node));
  if (strcmp(kind, "ParenExpr") == 0)
    return value(tr, child(node, 0), indent, elem_out);
  const char *cast = json_string(json_get(node, "castKind"), "");
  if (strcmp(kind, "ImplicitCastExpr") == 0 &&
      strcmp(cast, "LValueToRValue") == 0) {
    const json_value *target = strip_parens_and_casts(child(node, 0), kNoOp);
    if (strcmp(kind_of(target), "ArraySubscriptExpr") == 0)
      return load(tr, target, indent, elem_out);
    if (strcmp(kind_of(target), "DeclRefExpr") == 0) {
      env_entry *e = env_find(tr, referenced_id(target));
      if (e && e->kind == ENV_SCALAR) {
        *elem_out = e->elem;
        return xstrdup(e->name);
      }
    }
    unsupported(node, "only array elements and scalar float parameters can "
                      "be read");
  }
  const char *elem = float_type(t);
  if (!elem)
    unsupported(node, "expression of type '%s' is not supported in "
                      "arithmetic (float/double only)", qual_type(node));
  *elem_out = elem;
  if (strcmp(kind, "FloatingLiteral") == 0) {
    char *name = fresh(tr);
    double v = strtod(json_string(json_get(node, "value"), "0"), NULL);
    emit(tr, indent, "%s = arith.constant %s : %s %s", name,
         float_constant(v, elem), elem, loc_text(tr, node));
    return name;
  }
  if (strcmp(kind, "ImplicitCastExpr") == 0 &&
      strcmp(cast, "IntegralToFloating") == 0) {
    long long v;
    if (!try_int_constant(child(node, 0), &v))
      unsupported(node, "converting an integer variable to floating point is "
                        "not supported (only integer constants)");
    if (v > (1LL << 24) || v < -(1LL << 24))
      unsupported(node, "integer constant %lld is not exactly representable "
                        "here", v);
    char *name = fresh(tr);
    emit(tr, indent, "%s = arith.constant %s : %s %s", name,
         float_constant((double)v, elem), elem, loc_text(tr, node));
    return name;
  }
  if (strcmp(kind, "ImplicitCastExpr") == 0 &&
      strcmp(cast, "FloatingCast") == 0) {
    const char *src_elem;
    char *src = value(tr, child(node, 0), indent, &src_elem);
    return convert(tr, src, src_elem, elem, node, indent);
  }
  const char *op = opcode_of(node);
  if (strcmp(kind, "UnaryOperator") == 0 && strcmp(op, "-") == 0) {
    const char *src_elem;
    char *src = value(tr, child(node, 0), indent, &src_elem);
    char *name = fresh(tr);
    emit(tr, indent, "%s = arith.negf %s : %s %s", name, src, elem,
         loc_text(tr, node));
    return name;
  }
  if (strcmp(kind, "BinaryOperator") == 0 && strlen(op) == 1 &&
      strchr("+-*/", op[0])) {
    const char *ea, *eb;
    char *a = value(tr, child(node, 0), indent, &ea);
    char *b = value(tr, child(node, 1), indent, &eb);
    return arith(tr, op[0], a, b, elem, node, indent);
  }
  unsupported(node, "expression '%s' is not supported", kind);
}

/* -------------------------------------------------------------- statements */

/* ------------------------------------------------------- loop conditions */

/* The linear form a * counter + b of an integer expression built from the
   loop counter `decl`, integer constants, + - (binary and unary) and
   multiplication by a constant -- the left side of an affine loop condition
   such as `2 * i - 3 > 0`. 0 if the expression is not of that form (the
   magnitudes are kept below 2^100, far beyond any value the checks below let
   through). */
static int counter_linear(const json_value *n, const char *decl, __int128 *a,
                          __int128 *b) {
  const __int128 kLimit = (__int128)1 << 100;
  n = strip_parens_and_casts(n, kIntConstantCasts);
  const char *kind = kind_of(n);
  const char *op = opcode_of(n);
  long long v;
  if (is_ref_to(n, decl)) {
    *a = 1;
    *b = 0;
    return 1;
  }
  if (strcmp(kind, "DeclRefExpr") != 0 && try_int_constant(n, &v)) {
    *a = 0;
    *b = v;
    return 1;
  }
  __int128 a0, b0, a1, b1;
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "-") == 0 || strcmp(op, "+") == 0)) {
    if (!counter_linear(child(n, 0), decl, &a0, &b0))
      return 0;
    *a = op[0] == '-' ? -a0 : a0;
    *b = op[0] == '-' ? -b0 : b0;
    return 1;
  }
  if (strcmp(kind, "BinaryOperator") != 0)
    return 0;
  if (!counter_linear(child(n, 0), decl, &a0, &b0) ||
      !counter_linear(child(n, 1), decl, &a1, &b1))
    return 0;
  if (strcmp(op, "+") == 0 || strcmp(op, "-") == 0) {
    int sign = op[0] == '+' ? 1 : -1;
    *a = a0 + sign * a1;
    *b = b0 + sign * b1;
  } else if (strcmp(op, "*") == 0 && (a0 == 0 || a1 == 0)) {
    /* one factor is a constant */
    __int128 c = a0 == 0 ? b0 : b1, fa = a0 == 0 ? a1 : a0,
             fb = a0 == 0 ? b1 : b0;
    if (c > kLimit || c < -kLimit || fa > kLimit || fa < -kLimit ||
        fb > kLimit || fb < -kLimit)
      return 0;
    *a = c * fa;
    *b = c * fb;
  } else {
    return 0;
  }
  return *a < kLimit && *a > -kLimit && *b < kLimit && *b > -kLimit;
}

static __int128 floor_div(__int128 n, __int128 d) {   /* d > 0 */
  return n >= 0 ? n / d : -((-n + d - 1) / d);
}

/* `E OP C` with E = a * counter + b (a != 0) and C an integer constant, as the
   equivalent `counter OP' BOUND` (exact over the integers): divide by a (the
   comparison flips when a < 0), then round -- i < q is i < ceil(q), i <= q is
   i <= floor(q), i > q is i > floor(q), i >= q is i >= ceil(q). 0 if the
   condition is not of this form or the bound does not fit a long long. */
static int affine_condition(const json_value *cond, const char *decl,
                            const char **op, long long *bound) {
  const char *cop = opcode_of(cond);
  if (strcmp(kind_of(cond), "BinaryOperator") != 0 ||
      !(strcmp(cop, "<") == 0 || strcmp(cop, "<=") == 0 ||
        strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0))
    return 0;
  __int128 a, b;
  long long c;
  if (!counter_linear(child(cond, 0), decl, &a, &b) || a == 0 ||
      !try_int_constant(child(cond, 1), &c))
    return 0;
  __int128 num = (__int128)c - b, den = a;
  char rel = cop[0];            /* '<' or '>' */
  int strict = cop[1] == '\0';
  if (den < 0) {
    num = -num;
    den = -den;
    rel = rel == '<' ? '>' : '<';
  }
  __int128 fl = floor_div(num, den), ce = -floor_div(-num, den);
  __int128 q = (rel == '<') == strict ? ce : fl;
  static const char *const kOps[2][2] = {{"<=", "<"}, {">=", ">"}};
  if (q > (__int128)9223372036854775807LL || q < -(__int128)9223372036854775807LL - 1)
    return 0;
  *op = kOps[rel == '>'][strict];
  *bound = (long long)q;
  return 1;
}

/* `cop` with its sides exchanged: a < b is b > a. */
static const char *flipped_comparison(const char *cop) {
  if (strcmp(cop, "<") == 0)
    return ">";
  if (strcmp(cop, "<=") == 0)
    return ">=";
  if (strcmp(cop, ">") == 0)
    return "<";
  return "<=";
}

/* Typed operand equivalence for a min conditional: the same AST up to
   parentheses, loads, no-op casts and commutation of + or *. No reassociation
   or cancellation: intermediate operations and integer promotions stay in
   their original types. All four operands are separately checked by
   header_expr before translation; min_operand_check excludes side effects. */
static int same_expr(const json_value *a, const json_value *b) {
  a = strip_parens_and_casts(a, kLoadNoOp);
  b = strip_parens_and_casts(b, kLoadNoOp);
  const char *kind = kind_of(a);
  if (strcmp(kind, kind_of(b)) != 0 ||
      strcmp(opcode_of(a), opcode_of(b)) != 0 ||
      strcmp(qual_type(a), qual_type(b)) != 0 ||
      child_count(a) != child_count(b))
    return 0;
  if (strcmp(kind, "IntegerLiteral") == 0)
    return strcmp(json_string(json_get(a, "value"), ""),
                  json_string(json_get(b, "value"), "")) == 0;
  if (strcmp(kind, "DeclRefExpr") == 0) {
    const char *ia = referenced_id(a), *ib = referenced_id(b);
    return ia && ib && strcmp(ia, ib) == 0;
  }
  if (strcmp(kind, "ImplicitCastExpr") == 0 ||
      strcmp(kind, "CStyleCastExpr") == 0) {
    if (strcmp(json_string(json_get(a, "castKind"), ""),
               json_string(json_get(b, "castKind"), "")) != 0)
      return 0;
  } else if (strcmp(kind, "BinaryOperator") != 0 &&
             strcmp(kind, "UnaryOperator") != 0) {
    return 0;
  }
  int direct = 1;
  for (size_t i = 0; i < child_count(a); ++i)
    if (!same_expr(child(a, i), child(b, i))) {
      direct = 0;
      break;
    }
  if (direct)
    return 1;
  return strcmp(kind, "BinaryOperator") == 0 && child_count(a) == 2 &&
         (strcmp(opcode_of(a), "+") == 0 || strcmp(opcode_of(a), "*") == 0) &&
         same_expr(child(a, 0), child(b, 1)) &&
         same_expr(child(a, 1), child(b, 0));
}

/* The first function call in an expression, or NULL. */
static const json_value *call_in(const json_value *n) {
  if (!n || n->kind != JSON_OBJECT)
    return NULL;
  if (strcmp(kind_of(n), "CallExpr") == 0)
    return n;
  for (size_t i = 0; i < child_count(n); ++i) {
    const json_value *c = call_in(child(n, i));
    if (c)
      return c;
  }
  return NULL;
}

/* Refuses a loop bound that calls a function: its body, side effects and
   conversions are not analysed -- also one named min (standard C has none;
   whatever is called is not known to be the minimum). */
static _Noreturn void refuse_call(const json_value *call) {
  const json_value *callee = strip_parens_and_casts(
      child(call, 0), (const char *const[]){"FunctionToPointerDecay", NULL});
  const char *name = referenced_name(callee);
  unsupported(call, "the loop bound calls '%s': a function call is not "
                    "translated (its body, side effects and conversions are "
                    "not analysed); write the minimum as the macro "
                    "((a) < (b) ? (a) : (b)) or as 'i < a && i < b' "
                    "(docs/AFFINE_GEMM_MAPPING.md section 26.5)",
              name && *name ? name : "a function");
}

/* Only expressions whose evaluation can be repeated or exchanged. In
   particular a volatile/atomic read is observable even without a call. The
   affine/header parsers below impose the narrower integer expression subset. */
static void min_purity_check(const json_value *n) {
  if (strstr(qual_type(n), "volatile") || strstr(qual_type(n), "_Atomic"))
    unsupported(n, "a min loop bound must not read volatile or atomic values");
  const char *op = opcode_of(n);
  if (strcmp(kind_of(n), "CompoundAssignOperator") == 0 ||
      (strcmp(kind_of(n), "BinaryOperator") == 0 && strcmp(op, "=") == 0) ||
      (strcmp(kind_of(n), "UnaryOperator") == 0 &&
       (strcmp(op, "++") == 0 || strcmp(op, "--") == 0)))
    unsupported(n, "a min loop bound must not have side effects");
  for (size_t i = 0; i < child_count(n); ++i)
    min_purity_check(child(n, i));
}

/* Normalize an upper comparison without changing its C operands or casts.
   Integral promotions of the counter are deliberately not stripped. */
static int clipped_comparison(const json_value *c, const char *decl,
                              const json_value **bound, int *le) {
  c = strip_parens_and_casts(c, kNoOp);
  if (strcmp(kind_of(c), "BinaryOperator") != 0)
    return 0;
  const char *op = opcode_of(c);
  const json_value *counter = child(c, 0);
  *bound = child(c, 1);
  if (strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) {
    counter = child(c, 1);
    *bound = child(c, 0);
  } else if (strcmp(op, "<") != 0 && strcmp(op, "<=") != 0) {
    return 0;
  }
  counter = strip_parens_and_casts(counter, kLoadNoOp);
  if (strcmp(kind_of(counter), "DeclRefExpr") != 0 ||
      !referenced_id(counter) || strcmp(referenced_id(counter), decl) != 0)
    return 0;
  min_purity_check(counter);
  *le = op[1] == '=';
  return 1;
}

/* An operand of a min bound, P or Q: no call, no conditional (a min of more
   than two operands), its value of the counter's type as written. */
static void min_operand_check(const json_value *x, const char *vtype,
                              const json_value *where) {
  const json_value *call = call_in(x);
  if (call)
    refuse_call(call);
  min_purity_check(x);
  const json_value *v = strip_parens_and_casts(x, kLoadNoOp);
  if (strcmp(kind_of(v), "ConditionalOperator") == 0)
    unsupported(v, "the loop bound is a min of more than two operands (a "
                   "nested conditional): not translated");
  char *t = strip_qualifiers(qual_type(v));
  if (strcmp(t, vtype) != 0)
    unsupported(where, "an operand of the min loop bound has type '%s', not "
                       "the counter's type '%s': the conversion is not "
                       "translated (signed and unsigned are not mixed)",
                t, vtype);
}

/* S5.5 (docs/AFFINE_GEMM_MAPPING.md section 26.5). Whether the condition
   `cond` of a loop with counter `decl`, of type `vtype`, bounds the counter
   above by min(P, Q):
     counter OP C   (OP < or <=) with C the conditional expression that is
                    exactly min(P, Q): a comparison of P and Q selecting the
                    smaller one -- (P) < (Q) ? (P) : (Q), as the usual MIN
                    macro expands, also with <=, or with > / >= and the
                    branches exchanged (typed operand equivalence, same_expr);
     counter OP P && counter OP' Q   (OP, OP' < or <=).
   Each comparison may exchange its sides with the opposite relation.
   On 1: *p, *q are P and Q, and *p_le, *q_le whether the comparison of the
   counter with them is <= (the exclusive bound is then P + 1). The counter,
   P, Q, the conditional and both sides of its comparison must have the
   counter's type as written: no value is converted. 0 when the condition has
   neither shape (the other forms are the caller's); a condition with one of
   them that is not exactly a min is refused here, with the reason: a max, a
   conditional that is not the smaller of its operands, a function call (even
   one named min), a conversion. */
static int min_bound_of(const json_value *cond, const char *decl,
                        const char *vtype, const json_value **p, int *p_le,
                        const json_value **q, int *q_le,
                        const json_value **branch_t, const json_value **branch_f) {
  cond = strip_parens_and_casts(cond, kNoOp);
  if (strcmp(kind_of(cond), "BinaryOperator") != 0)
    return 0;
  const char *cop = opcode_of(cond);
  if (strcmp(cop, "&&") == 0) {
    const json_value *sides[2] = {
        strip_parens_and_casts(child(cond, 0), kNoOp),
        strip_parens_and_casts(child(cond, 1), kNoOp)};
    const json_value *bounds[2];
    int le[2];
    for (int s = 0; s < 2; ++s) {
      const json_value *c = sides[s];
      if (!clipped_comparison(c, decl, &bounds[s], &le[s]))
        unsupported(cond, "a loop condition joined by && must be 'counter < "
                          "a && counter < b' (< or <= each, either side of "
                          "each comparison): the minimum of two upper bounds "
                          "(docs/AFFINE_GEMM_MAPPING.md section 26.5)");
      min_operand_check(bounds[s], vtype, c);
    }
    *p = bounds[0];
    *p_le = le[0];
    *q = bounds[1];
    *q_le = le[1];
    return 1;
  }
  const json_value *bound;
  int le;
  /* Locate a conditional even if the counter was converted: preserve the
     existing diagnostic rather than silently accepting the conversion. */
  const json_value *candidate = child(cond,
      strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0 ? 0 : 1);
  const json_value *rhs = strip_parens_and_casts(candidate, kLoadNoOp);
  const json_value *converted = strip_parens_and_casts(candidate, kIndexCasts);
  if (strcmp(kind_of(rhs), "CallExpr") == 0)
    refuse_call(rhs);
  if (strcmp(kind_of(converted), "ConditionalOperator") != 0)
    return 0;
  if (!clipped_comparison(cond, decl, &bound, &le) || rhs != converted ||
      strcmp(strip_qualifiers(qual_type(rhs)), vtype) != 0)
    unsupported(cond, "the loop condition compares the counter with a "
                      "conditional expression of another type, or not the "
                      "counter itself: the conversion is not translated "
                      "(docs/AFFINE_GEMM_MAPPING.md section 26.5)");
  const json_value *call = call_in(rhs);
  if (call)
    refuse_call(call);
  const json_value *test = strip_parens_and_casts(child(rhs, 0), kNoOp);
  const char *top = opcode_of(test);
  if (strcmp(kind_of(test), "BinaryOperator") != 0 ||
      !(strcmp(top, "<") == 0 || strcmp(top, "<=") == 0 ||
        strcmp(top, ">") == 0 || strcmp(top, ">=") == 0))
    unsupported(rhs, "the loop bound is a conditional expression whose "
                     "condition is not a comparison of its two branches: not "
                     "a min (docs/AFFINE_GEMM_MAPPING.md section 26.5)");
  const json_value *x = child(test, 0), *y = child(test, 1);
  const json_value *t = child(rhs, 1), *f = child(rhs, 2);
  min_operand_check(x, vtype, test);
  min_operand_check(y, vtype, test);
  min_operand_check(t, vtype, rhs);
  min_operand_check(f, vtype, rhs);
  const int less = top[0] == '<';
  /* min: x < y ? x : y (or <=), x > y ? y : x (or >=) */
  const int is_min = less ? same_expr(t, x) && same_expr(f, y)
                          : same_expr(t, y) && same_expr(f, x);
  const int is_max = less ? same_expr(t, y) && same_expr(f, x)
                          : same_expr(t, x) && same_expr(f, y);
  if (is_max)
    unsupported(rhs, "the loop bound is the maximum of its two operands, not "
                     "the minimum: an upper bound max is not translated "
                     "(docs/AFFINE_GEMM_MAPPING.md section 26.5)");
  if (!is_min)
    unsupported(rhs, "the loop bound is a conditional expression whose "
                     "branches are not the two sides of its comparison: not "
                     "a min (docs/AFFINE_GEMM_MAPPING.md section 26.5)");
  *p = x;
  *q = y;
  *p_le = *q_le = le;
  *branch_t = t;
  *branch_f = f;
  return 1;
}

/* The text of the affine.for upper bound min(f1, f2) (S5.5): `min
   affine_map<(d0, ...) -> (e1, e2)>(%a, ...)` over the counters either form
   reads, each expression as bound_text writes one (a counting-down counter's
   value substituted). */
static char *min_bound_text(translator *tr, const linear_form *f1,
                            const linear_form *f2, const json_value *node) {
  const linear_form *forms[2] = {f1, f2};
  const char *names[2 * MAX_FORM];
  long long coef[2][2 * MAX_FORM];
  long long constant[2];
  size_t n = 0;
  memset(coef, 0, sizeof coef);
  for (int k = 0; k < 2; ++k) {
    constant[k] = forms[k]->constant;
    for (size_t i = 0; i < forms[k]->n; ++i) {
      env_entry *e = env_find(tr, forms[k]->ids[i]);
      long long c = forms[k]->coef[i];
      if (e->descending) {
        constant[k] = checked(constant[k], checked(c, e->origin, '*', node),
                              '+', node);
        c = checked(0, checked(c, e->stride, '*', node), '-', node);
      }
      if (c == 0)
        continue;
      size_t d = 0;
      while (d < n && strcmp(names[d], e->name) != 0)
        ++d;
      if (d == n)
        names[n++] = e->name;
      coef[k][d] = checked(coef[k][d], c, '+', node);
    }
  }
  char *dims = xstrdup(""), *args = xstrdup("");
  for (size_t d = 0; d < n; ++d) {
    dims = format("%s%sd%zu", dims, d ? ", " : "", d);
    args = format("%s%s%s", args, d ? ", " : "", names[d]);
  }
  char *exprs[2];
  for (int k = 0; k < 2; ++k) {
    exprs[k] = format("%lld", constant[k]);
    for (size_t d = 0; d < n; ++d) {
      if (coef[k][d] == 0)
        continue;
      unsigned long long magnitude =
          coef[k][d] < 0 ? 0ULL - (unsigned long long)coef[k][d]
                         : (unsigned long long)coef[k][d];
      exprs[k] = format("%s %c d%zu * %llu", exprs[k],
                        coef[k][d] < 0 ? '-' : '+', d, magnitude);
    }
  }
  return format("min affine_map<(%s) -> (%s, %s)>(%s)", dims, exprs[0],
                exprs[1], args);
}

/* One side of a loop condition as a * counter + f, f a linear form over the
   enclosing loop counters: integer constants, references to the counter or
   to enclosing counters, unary + / -, + - and constant *, parentheses and
   integer casts. 0 when it is not one. It recurses once per AST level, so
   it keeps no form in its frame: the left operand is built in *f, the right
   one on the heap, and the arithmetic is header_negate's and
   header_binary's (as header_expr does). */
static int cond_side_form(translator *tr, const json_value *n, const char *decl,
                          linear_form *f, long long *a) {
  n = strip_parens_and_casts(n, kIntConstantCasts);
  const char *kind = kind_of(n);
  const char *op = opcode_of(n);
  long long v;
  memset(f, 0, sizeof *f);
  *a = 0;
  if (is_ref_to(n, decl)) {
    *a = 1;
    return 1;
  }
  if (strcmp(kind, "DeclRefExpr") == 0) {
    env_entry *e = env_find(tr, referenced_id(n));
    if (!e || e->kind != ENV_IV)
      return 0;
    f->ids[0] = e->decl;
    f->coef[0] = 1;
    f->n = 1;
    return 1;
  }
  if (try_int_constant(n, &v)) {
    f->constant = v;
    return 1;
  }
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "-") == 0 || strcmp(op, "+") == 0)) {
    if (!cond_side_form(tr, child(n, 0), decl, f, a))
      return 0;
    if (op[0] == '-')
      header_negate(a, f, n);
    return 1;
  }
  if (strcmp(kind, "BinaryOperator") != 0)
    return 0;
  linear_form *f1 = xmalloc(sizeof *f1);
  long long a0, a1;
  const int ok = cond_side_form(tr, child(n, 0), decl, f, &a0) &&
                 cond_side_form(tr, child(n, 1), decl, f1, &a1) &&
                 (strcmp(op, "+") == 0 || strcmp(op, "-") == 0 ||
                  strcmp(op, "*") == 0) &&
                 header_binary(op, a0, a1, f1, a, f, n);
  free(f1);
  return ok;
}

/* The checks of a non-box loop (bounds that do not shift together, a
   triangle: its values at each outer point are lb(x), lb(x) + step, ...
   below ub(x)) at every point of its enclosing nest (enumerate_points): the
   lower bound and the bound as written fit the counter's type, the last
   value plus the step too, and every node of the header fits its C type
   where C evaluates it -- the initial value's and the bound's at that
   point, the condition's at the first value and at the value that ends the
   loop. `runs`: the loop runs at some outer point. */
typedef struct {
  const json_value *loop, *cond;
  linear_form lb;         /* the initial value over the enclosing counters */
  linear_form bound;      /* the bound, before <='s + 1 */
  int has_bound2;         /* S5.5: the loop runs below min(bound, bound2) */
  linear_form bound2;     /* (both exclusive) */
  int le;                 /* the canonical comparison is <= */
  long long step;
  __int128 type_min, type_max;
  const char *vtype;
  int check_written;      /* the condition is `counter OP bound` as written */
  const header_nodes *init_nodes, *bound_nodes, *cond_nodes;
  int runs;
  int evaluated;          /* C evaluates the header at some point */
} loop_check;

static void loop_check_point(translator *tr, void *p) {
  loop_check *c = p;
  long long lbv, bw;
  c->evaluated = 1;
  if (!form_value(tr, &c->lb, &lbv) || !form_value(tr, &c->bound, &bw))
    unsupported(c->loop, "the loop counter could overflow '%s'", c->vtype);
  if (c->has_bound2) {
    /* The loop runs below the smaller bound; each operand's value is
       checked in its C type by its header nodes (an operand that overflows
       at this point is refused there, with its value). */
    long long bw2;
    if (!form_value(tr, &c->bound2, &bw2))
      unsupported(c->loop, "the loop counter could overflow '%s'", c->vtype);
    if (bw2 < bw)
      bw = bw2;
  }
  const __int128 ubx = (__int128)bw + c->le;
  if (lbv < c->type_min || lbv > c->type_max || lbv < -2147483648LL ||
      (c->check_written && (bw < c->type_min || bw > c->type_max)))
    unsupported(c->loop, "the loop counter could overflow '%s'", c->vtype);
  const __int128 trips = ubx > lbv ? (ubx - 1 - lbv) / c->step + 1 : 0;
  if (trips > 0) {
    c->runs = 1;
    __int128 last = (__int128)lbv + (__int128)c->step * (trips - 1);
    if (last + c->step > c->type_max)
      unsupported(c->loop, "the loop counter could overflow '%s'", c->vtype);
  }
  const __int128 exit_value = (__int128)lbv + (__int128)c->step * trips;
  header_check_point(tr, c->init_nodes, HEADER_INIT, lbv, lbv, c->cond);
  header_check_point(tr, c->bound_nodes, HEADER_BOUND, lbv, lbv, c->cond);
  header_check_point(tr, c->cond_nodes, HEADER_COND, lbv, exit_value, c->cond);
}

static void statements(translator *tr, const json_value *stmts, int indent,
                       int top_level);

static void assignment(translator *tr, const json_value *s, int indent) {
  const char *opcode = opcode_of(s);
  if (strcmp(opcode, "=") && strcmp(opcode, "+=") && strcmp(opcode, "-=") &&
      strcmp(opcode, "*=") && strcmp(opcode, "/="))
    unsupported(s, "expression statement '%s' is not supported (only "
                   "assignments to array elements)",
                opcode[0] ? opcode : kind_of(s));
  const json_value *lhs = child(s, 0), *rhs = child(s, 1);
  const json_value *lhs_node = strip_parens_and_casts(lhs, kNoOp);
  if (strcmp(kind_of(lhs_node), "ArraySubscriptExpr") != 0)
    unsupported(lhs, "only array elements can be assigned");
  char *idx;
  env_entry *e = array_access(tr, lhs_node, strcmp(opcode, "=") != 0, 1, &idx);
  const char *elem = e->elem;
  char *v;
  /* --scatter-subscripts: the indices as index arithmetic, read and written
     with memref.load / memref.store (a computed index is no affine dimension). */
  const char *op_load = "affine.load", *op_store = "affine.store";
  if (tr->general.active) {
    tr->general.active = 0;
    char *list = xstrdup("");
    for (int d = 0; d < tr->general.count; ++d) {
      char *one = general_value(tr, tr->general.nodes[d], indent);
      char *joined = format("%s%s%s", list, d ? ", " : "", one);
      free(list);
      list = joined;
    }
    idx = list;
    op_load = "memref.load";
    op_store = "memref.store";
  }
  if (strcmp(opcode, "=") == 0) {
    const char *v_elem;
    v = value(tr, rhs, indent, &v_elem);
    if (strcmp(v_elem, elem) != 0)
      unsupported(rhs, "assigned value has type %s, element is %s", v_elem,
                  elem);
  } else {
    /* E1 op= E2 is E1 = E1 op E2 with E1 evaluated once, computed in clang's
       computation type and converted back to E1's type. */
    char *ct = strip_qualifiers(json_string(
        json_get(json_get(s, "computeResultType"), "qualType"), ""));
    const char *compute = float_type(ct);
    if (!compute)
      unsupported(s, "compound assignment on a non-floating type");
    char *old = fresh(tr);
    emit(tr, indent, "%s = %s %s[%s] : %s %s", old, op_load, e->name, idx,
         e->type, loc_text(tr, lhs_node));
    old = convert(tr, old, elem, compute, s, indent);
    const char *r_elem;
    char *r = value(tr, rhs, indent, &r_elem);
    r = convert(tr, r, r_elem, compute, s, indent);
    v = arith(tr, opcode[0], old, r, compute, s, indent);
    v = convert(tr, v, compute, elem, s, indent);
  }
  emit(tr, indent, "%s %s, %s[%s] : %s %s", op_store, v, e->name, idx,
       e->type, loc_text(tr, s));
}

static void for_stmt(translator *tr, const json_value *s, int indent) {
  /* ForStmt children: init, condition variable (unused), cond, inc, body */
  const json_value *init = child(s, 0), *cond = child(s, 2),
                   *inc = child(s, 3), *body = child(s, 4);
  if (strcmp(kind_of(init), "DeclStmt") != 0 || child_count(init) != 1)
    unsupported(s, "the for loop must declare exactly one integer counter in "
                   "its initializer");
  const json_value *var = child(init, 0);
  char *vtype = strip_qualifiers(qual_type(var));
  unsigned long long type_max;
  if (strcmp(kind_of(var), "VarDecl") != 0 || !int_type_max(vtype, &type_max) ||
      child_count(var) == 0)
    unsupported(var, "the loop counter must be an initialized integer "
                     "variable");
  const char *decl = json_string(json_get(var, "id"), "");
  /* The initial value: an integer constant, or an affine expression of the
     enclosing loop counters (for (int k = i; ...)): its counter terms are
     the loop's shift, its constant the first value without the shift. */
  long long lb;
  linear_form lb_form = {0};
  int lb_affine = 0;
  if (try_int_constant(child(var, 0), &lb)) {
    lb_form.constant = lb;
  } else {
    if (!try_bound_form(tr, child(var, 0), &lb_form))
      unsupported(child(var, 0), "the loop counter's initial value must be an "
                                 "integer constant or an affine expression of "
                                 "the enclosing loop counters");
    lb_affine = lb_form.n != 0;
    lb = lb_form.constant;
  }

  const char *cop = opcode_of(cond);
  int has_cond = cond && cond->kind == JSON_OBJECT && cond->count;
  int up = strcmp(cop, "<") == 0 || strcmp(cop, "<=") == 0;
  int down = strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0;
  long long bound;
  /* S5.5 (docs/AFFINE_GEMM_MAPPING.md section 26.5): an upper bound
     min(P, Q) -- a point loop whose tile a min clips, i < MIN(ii + 4, 10)
     with the macro ((a) < (b) ? (a) : (b)), or i < ii + 4 && i < 10 -- is
     the affine.for upper bound min (P, Q), exactly; anything that only looks
     like one is refused by min_bound_of. */
  const json_value *min_p = NULL, *min_q = NULL;
  const json_value *min_t = NULL, *min_f = NULL;
  int min_p_le = 0, min_q_le = 0;
  const int has_min = has_cond && min_bound_of(cond, decl, vtype, &min_p,
                                               &min_p_le, &min_q, &min_q_le,
                                               &min_t, &min_f);
  linear_form ub2_form = {0};
  int has_ub2 = 0;
  /* An affine condition E OP C (2 * i - 3 > 0): the equivalent comparison of
     the counter with a constant; both sides are checked below, node by node
     in their C types, once the values they are evaluated at are known. */
  int affine_cond = 0;
  if (!has_min && strcmp(kind_of(cond), "BinaryOperator") == 0 &&
      (up || down) &&
      !is_ref_to(child(cond, 0), decl) &&
      affine_condition(cond, decl, &cop, &bound)) {
    affine_cond = 1;
    up = cop[0] == '<';
    down = !up;
  }
  linear_form ub_form = {0};
  int ub_affine = 0;
  /* Any other comparison L OP R with L - R = a * counter + g + c affine in
     the counter and the enclosing counters (a != 0) is its equivalent
     comparison of the counter itself (docs/AFFINE_GEMM_MAPPING.md section
     19.13): without counter terms in g, exactly as affine_condition rounds
     (8 > k is k < 8); with them, a must be 1 or -1 (k - i < 4 is k < i + 4,
     i + 4 > k likewise). Both sides of such a condition are checked, node by
     node in their C types, where C evaluates them. */
  int cond_outer = 0;
  if (!has_min && !affine_cond &&
      strcmp(kind_of(cond), "BinaryOperator") == 0 && (up || down) &&
      !is_ref_to(child(cond, 0), decl)) {
    linear_form fl, fr;
    long long al, ar;
    if (cond_side_form(tr, child(cond, 0), decl, &fl, &al) &&
        cond_side_form(tr, child(cond, 1), decl, &fr, &ar)) {
      linear_form g = form_add(fl, form_scale(fr, -1, cond), cond);
      const long long a = checked(al, ar, '-', cond);
      if (a != 0 && g.n == 0) {
        /* a * k + c OP 0: k OP' bound, rounded as affine_condition rounds */
        __int128 num = -(__int128)g.constant, den = a;
        char rel = cop[0];
        int strict = cop[1] == '\0';
        if (den < 0) {
          num = -num;
          den = -den;
          rel = rel == '<' ? '>' : '<';
        }
        __int128 fl_q = floor_div(num, den), ce_q = -floor_div(-num, den);
        __int128 q = (rel == '<') == strict ? ce_q : fl_q;
        static const char *const kOps[2][2] = {{"<=", "<"}, {">=", ">"}};
        if (q <= (__int128)9223372036854775807LL &&
            q >= -(__int128)9223372036854775807LL - 1) {
          cop = kOps[rel == '>'][strict];
          bound = (long long)q;
          affine_cond = 1;
          up = cop[0] == '<';
          down = !up;
        }
      } else if (a != 0) {
        if (a != 1 && a != -1)
          unsupported(cond, "the loop condition must compare the counter, "
                            "with coefficient 1 or -1, with an affine "
                            "expression of the enclosing loop counters");
        /* k + g + c OP 0 is k OP -(g + c); -k + g + c OP 0 is k OP' g + c */
        if (a == 1) {
          ub_form = form_scale(g, -1, cond);
        } else {
          ub_form = g;
          cop = flipped_comparison(cop);
        }
        bound = ub_form.constant;
        ub_affine = 1;
        cond_outer = 1;
        up = cop[0] == '<';
        down = !up;
      }
    }
  }
  if (has_min) {
    /* Each operand an integer constant or an affine expression of the
       enclosing counters, as any bound (its nodes are checked below in their
       C types); <= is the exclusive bound + 1. Operands with the same
       counter terms differ by a constant: min(L + a, L + b) = L + min(a, b),
       one bound. */
    linear_form fp = {0}, fq = {0};
    if (!try_bound_form(tr, min_p, &fp) || !try_bound_form(tr, min_q, &fq))
      unsupported(cond, "an operand of the min loop bound must be an integer "
                        "constant or an affine expression of the enclosing "
                        "loop counters");
    fp.constant = checked(fp.constant, min_p_le, '+', cond);
    fq.constant = checked(fq.constant, min_q_le, '+', cond);
    linear_form d = form_add(fp, form_scale(fq, -1, cond), cond);
    cop = "<";
    up = 1;
    down = 0;
    if (d.n == 0) {
      ub_form = d.constant <= 0 ? fp : fq;
    } else {
      ub_form = fp;
      ub2_form = fq;
      has_ub2 = 1;
    }
    bound = ub_form.constant;
    ub_affine = ub_form.n != 0 || has_ub2;
  } else if (!affine_cond && !cond_outer) {
    if (strcmp(kind_of(cond), "BinaryOperator") != 0 || !(up || down) ||
        !is_ref_to(child(cond, 0), decl))
      unsupported(has_cond ? cond : s, "the loop condition must be 'counter < "
                                       "bound', 'counter <= bound', 'counter > "
                                       "bound' or 'counter >= bound'");
    /* The bound: an integer constant, or an affine expression of the
       enclosing loop counters (k < i + 4). */
    if (!try_int_constant(child(cond, 1), &bound)) {
      if (!try_bound_form(tr, child(cond, 1), &ub_form))
        unsupported(child(cond, 1), "the loop bound must be an integer "
                                    "constant or an affine expression of the "
                                    "enclosing loop counters");
      ub_affine = ub_form.n != 0;
      bound = ub_form.constant;
    }
  }

  /* The signed step: ++ / += S / = i + S count up, -- / -= S / = i - S count
     down. */
  int has_step = 0;
  long long step = 0;
  const char *iop = opcode_of(inc);
  if (strcmp(kind_of(inc), "UnaryOperator") == 0 &&
      (strcmp(iop, "++") == 0 || strcmp(iop, "--") == 0) &&
      is_ref_to(child(inc, 0), decl)) {
    step = iop[0] == '+' ? 1 : -1;
    has_step = 1;
  } else if (strcmp(kind_of(inc), "CompoundAssignOperator") == 0 &&
             (strcmp(iop, "+=") == 0 || strcmp(iop, "-=") == 0) &&
             is_ref_to(child(inc, 0), decl)) {
    step = int_constant(child(inc, 1));
    if (iop[0] == '-')
      step = checked(0, step, '-', inc);
    has_step = 1;
  } else if (strcmp(kind_of(inc), "BinaryOperator") == 0 &&
             strcmp(iop, "=") == 0 && is_ref_to(child(inc, 0), decl)) {
    const json_value *rhs = strip_parens_and_casts(child(inc, 1),
                                                   kLValueToRValue);
    const char *rop = opcode_of(rhs);
    if (strcmp(kind_of(rhs), "BinaryOperator") == 0 &&
        (strcmp(rop, "+") == 0 || strcmp(rop, "-") == 0) &&
        is_ref_to(child(rhs, 0), decl)) {
      step = int_constant(child(rhs, 1));
      if (rop[0] == '-')
        step = checked(0, step, '-', inc);
      has_step = 1;
    }
  }
  int has_inc = inc && inc->kind == JSON_OBJECT && inc->count;
  if (!has_step)
    unsupported(has_inc ? inc : s, "the loop increment must be '++i', 'i++', "
                                   "'i += S' or 'i = i + S' (counting up), or "
                                   "'--i', 'i--', 'i -= S' or 'i = i - S' "
                                   "(counting down)");
  /* A zero step never reaches the bound; a step against the condition's
     direction runs until the counter overflows. Neither is a loop affine.for
     can express. */
  if (step == 0)
    unsupported(inc, "the loop step must not be zero");
  if (up && step < 0)
    unsupported(inc, "the loop step must be positive (the condition '%s' "
                     "counts up)", cop);
  if (down && step > 0)
    unsupported(inc, "the loop step must be negative (the condition '%s' "
                     "counts down)", cop);

  /* The header's integer nodes, each in its C type (header_expr): the
     initial value's; the bound's, when the condition is `counter OP bound`
     as written (its left side is the counter itself, so the comparison is
     in the counter's type); else both sides' of the condition, which read
     the counter. */
  const int cond_counter = affine_cond || cond_outer;
  header_nodes init_nodes = {0}, bound_nodes = {0}, cond_nodes = {0};
  {
    long long a_unused;
    linear_form f_unused;
    int r_unused;
    if (!header_expr(tr, child(var, 0), NULL, &init_nodes, &a_unused,
                     &f_unused, &r_unused))
      unsupported(child(var, 0), "the loop counter's initial value must be an "
                                 "integer constant or an affine expression of "
                                 "the enclosing loop counters");
    /* A min bound: the nodes of both operands, which C evaluates at every
       test of the condition (the conditional compares them; && may skip the
       second -- checking it everywhere only refuses more). */
    if (cond_counter ? !header_expr(tr, child(cond, 0), decl, &cond_nodes,
                                    &a_unused, &f_unused, &r_unused) ||
                           !header_expr(tr, child(cond, 1), decl, &cond_nodes,
                                        &a_unused, &f_unused, &r_unused)
        : has_min    ? !header_expr(tr, min_p, NULL, &bound_nodes, &a_unused,
                                    &f_unused, &r_unused) ||
                           !header_expr(tr, min_q, NULL, &bound_nodes,
                                        &a_unused, &f_unused, &r_unused)
                     : !header_expr(tr, child(cond, 1), NULL, &bound_nodes,
                                    &a_unused, &f_unused, &r_unused))
      unsupported(cond, "the loop condition must be 'counter < bound', "
                        "'counter <= bound', 'counter > bound' or 'counter >= "
                        "bound'");
  }

  /* Branches may commute operands, but their own promotions and every
     intermediate node must pass the same range checks as the comparison.
     Checking both branches is conservative when C selects only one. */
  if (min_t) {
    long long a_unused;
    linear_form f_unused;
    int r_unused;
    if (!header_expr(tr, min_t, NULL, &bound_nodes, &a_unused, &f_unused,
                     &r_unused) ||
        !header_expr(tr, min_f, NULL, &bound_nodes, &a_unused, &f_unused,
                     &r_unused))
      unsupported(cond, "the min branches must be affine integer expressions "
                        "of the enclosing loop counters");
  }

  /* The values the counter takes, first, first + step, ... while the
     condition holds: `trips` of them (exact integer arithmetic). */
  int is_unsigned = strncmp(vtype, "unsigned", 8) == 0;
  __int128 type_min = is_unsigned ? 0 : -(__int128)type_max - 1;
  long long ub = 0;   /* counting up: the exclusive upper bound */
  __int128 trips;
  __int128 point_trips = 0;   /* (not non-box) the trip count at each point */
  int nonbox = 0;
  loop_check check = {0};
  check.loop = s;
  check.cond = cond;
  check.lb = lb_form;
  check.bound = ub_form;
  check.bound.constant = bound;
  check.has_bound2 = has_ub2;
  check.bound2 = ub2_form;
  check.le = strcmp(cop, "<=") == 0;
  check.step = step;
  check.type_min = type_min;
  check.type_max = (__int128)type_max;
  check.vtype = vtype;
  check.init_nodes = &init_nodes;
  check.bound_nodes = &bound_nodes;
  check.cond_nodes = &cond_nodes;
  if (lb_affine || ub_affine) {
    /* Bounds affine in the enclosing counters, counting up. With the same
       counter terms the extent UB - LB is a constant and the domain a
       constant-extent parallelotope (the compiler maps it onto a box,
       docs/AFFINE_GEMM_MAPPING.md section 19.12). With different ones (a
       triangle) the loop is translated as written: its values at each outer
       point are LB(x), LB(x) + S, ... below UB(x), and every subscript range
       in its scope is computed by running the nest (section 19.13). */
    if (!up)
      unsupported(s, "a loop whose bounds depend on an enclosing loop counter "
                     "must count up");
    ub = bound;
    if (strcmp(cop, "<=") == 0)
      ub = checked(ub, 1, '+', cond);
    linear_form lb_terms = lb_form, ub_terms = ub_form;
    lb_terms.constant = 0;
    ub_terms.constant = 0;
    linear_form extent = form_add(ub_terms, form_scale(lb_terms, -1, s), s);
    /* a min bound: the extent varies with the outer counters (non-box) */
    nonbox = extent.n != 0 || has_ub2;
  }
  if (nonbox) {
    /* Exactly, at every point of the enclosing nest: the initial value and
       the bound as written fit the counter's type, its last value plus the
       step too, every node of the header fits its C type where C evaluates
       it; the loop runs somewhere. */
    check.check_written = !cond_counter;
    size_t visited = 0;
    enumerate_points(tr, 0, loop_check_point, &check, &visited, s);
    trips = check.runs;
    if (!check.evaluated) {
      header_check_constants(&init_nodes, HEADER_INIT, cond);
      header_check_constants(cond_counter ? &cond_nodes : &bound_nodes,
                             cond_counter ? HEADER_COND : HEADER_BOUND, cond);
    }
  } else if (lb_affine || ub_affine) {
    trips = ub > lb ? ((__int128)ub - 1 - lb) / step + 1 : 0;
    point_trips = trips;
    /* At every value of the enclosing counters the bounds must fit the
       counter's type, the last increment too. An enclosing loop that runs
       zero times means this one never runs. */
    if (trips > 0) {
      long long lb_lo, lb_hi, ub_lo, ub_hi;
      linear_form ub_full = ub_form;
      ub_full.constant = ub;
      if (form_range(tr, &lb_form, &lb_lo, &lb_hi, s) &&
          form_range(tr, &ub_full, &ub_lo, &ub_hi, s)) {
        __int128 last_increment = (__int128)ub_hi - 1 + step;
        if (last_increment > (__int128)type_max || lb_lo < type_min ||
            lb_lo < -2147483648LL)
          unsupported(s, "the loop counter could overflow '%s'", vtype);
      } else {
        trips = 0;
      }
    }
  } else if (up) {
    ub = bound;
    if (strcmp(cop, "<=") == 0)
      ub = checked(ub, 1, '+', cond);
    trips = ub > lb ? ((__int128)ub - 1 - lb) / step + 1 : 0;
    point_trips = trips;
    /* The counter's last increment must not overflow its type (that would
       be undefined behaviour or wrap-around, neither of which affine.for
       models). */
    __int128 last_increment = (__int128)ub - 1 + step;
    if (last_increment > (__int128)type_max || lb < -2147483648LL)
      unsupported(s, "the loop counter could overflow '%s'", vtype);
  } else {
    /* i > B is i >= B + 1 */
    __int128 lowest = strcmp(cop, ">") == 0 ? (__int128)bound + 1 : bound;
    __int128 down_step = -(__int128)step;
    trips = (__int128)lb >= lowest ? ((__int128)lb - lowest) / down_step + 1
                                   : 0;
    point_trips = trips;
    /* The decrement after the last iteration must not leave the type: for
       an unsigned counter and the condition i >= 0 it would wrap around and
       the loop would never end. */
    __int128 exit_value = (__int128)lb - down_step * trips;
    if (exit_value < type_min)
      unsupported(s, "the loop counter could overflow '%s'", vtype);
    if (trips > (__int128)9223372036854775807LL)
      unsupported(s, "the loop runs more than 2^63 - 1 times");
  }

  /* Every node of the header in its C type where C evaluates it (a non-box
     loop's: at every point of the enclosing nest, above), in closed form:
     the initial value and the bound at every point of the enclosing nest,
     the condition before every iteration and once more when the loop ends
     -- at the counter values lb, lb + step, ..., lb + step * trips (trips
     the same at every point), where each of its nodes, affine in the
     counter, is extreme at the first or the last of them. */
  if (!nonbox)
    header_check_closed(tr, &init_nodes,
                        cond_counter ? &cond_nodes : &bound_nodes,
                        cond_counter ? HEADER_COND : HEADER_BOUND, &lb_form,
                        (__int128)step * point_trips, cond, s);

  if (writes_to(body, decl))
    unsupported(body, "the loop counter is modified in the loop body");

  char *name = unique(tr, json_string(json_get(var, "name"), "iv"));
  env_entry *e = env_add(tr, decl, ENV_IV);
  e->name = name;
  /* The counter's first and last values; none for a loop that runs zero
     times. A non-box loop keeps its bounds instead. */
  e->step = step;
  if (nonbox) {
    e->nonbox = 1;
    e->has_values = trips > 0;
    e->lb_form = lb_form;
    e->ub_form = ub_form;
    e->ub_form.constant = ub;
    e->has_ub2 = has_ub2;
    e->ub2_form = ub2_form;
  } else if (trips > 0) {
    e->has_values = 1;
    e->first = lb;
    e->last = (long long)((__int128)lb + (__int128)step * (trips - 1));
  }
  /* The shift of a counter with affine bounds (form_range substitutes it). */
  if (lb_affine) {
    e->shift = lb_form;
    e->shift.constant = 0;
  }
  if (down) {
    /* affine.for counts up: the ordinal t = 0 .. trips-1, the counter
       lb - |step| * t (see affine_index). */
    e->descending = 1;
    e->origin = lb;
    e->stride = -step;
    emit(tr, indent, "affine.for %s = 0 to %lld {", name, (long long)trips);
  } else {
    /* Constant bounds are the numbers; bounds affine in the enclosing
       counters are affine_map<(d0, ...) -> (...)>(%counters). */
    ub_form.constant = ub;
    char *lb_text = bound_text(tr, &lb_form, s);
    char *ub_text = has_ub2 ? min_bound_text(tr, &ub_form, &ub2_form, s)
                            : bound_text(tr, &ub_form, s);
    if (step == 1)
      emit(tr, indent, "affine.for %s = %s to %s {", name, lb_text, ub_text);
    else
      emit(tr, indent, "affine.for %s = %s to %s step %lld {", name, lb_text,
           ub_text, step);
  }
  if (strcmp(kind_of(body), "CompoundStmt") == 0) {
    statements(tr, json_get(body, "inner"), indent + 1, 0);
  } else {
    json_value *items[1];
    memcpy(&items[0], &body, sizeof items[0]);
    json_value one = {.kind = JSON_ARRAY, .count = 1, .items = items};
    statements(tr, &one, indent + 1, 0);
  }
  emit(tr, indent, "} %s", loc_text(tr, s));
  /* The counter goes out of scope. */
  for (size_t i = tr->env_n; i-- > 0;)
    if (tr->env[i].decl && strcmp(tr->env[i].decl, decl) == 0) {
      memmove(&tr->env[i], &tr->env[i + 1],
              (tr->env_n - i - 1) * sizeof *tr->env);
      --tr->env_n;
      break;
    }
}

static void statements(translator *tr, const json_value *stmts, int indent,
                       int top_level) {
  size_t n = json_size(stmts);
  for (size_t index = 0; index < n; ++index) {
    const json_value *s = json_at(stmts, index);
    const char *kind = kind_of(s);
    if (strcmp(kind, "NullStmt") == 0)
      continue;
    if (strcmp(kind, "CompoundStmt") == 0) {
      statements(tr, json_get(s, "inner"), indent, 0);
    } else if (strcmp(kind, "ForStmt") == 0) {
      for_stmt(tr, s, indent);
    } else if (strcmp(kind, "ReturnStmt") == 0) {
      if (!top_level || index != n - 1 || child_count(s))
        unsupported(s, "only a final 'return;' of the function body is "
                       "supported");
    } else if (strcmp(kind, "BinaryOperator") == 0 ||
               strcmp(kind, "CompoundAssignOperator") == 0) {
      assignment(tr, s, indent);
    } else if (strcmp(kind, "DeclStmt") == 0) {
      unsupported(s, "local variables are not supported (only for-loop "
                     "counters)");
    } else {
      unsupported(s, "statement '%s' is not supported", kind);
    }
  }
}

/* --------------------------------------------------------------- functions */

/* --static-extents: whether the parameter's declarator, as written in the
   source, has a leading bound: NAME [ qualifiers BOUND ]. A declaration from
   a macro or another file is taken as having none. */
static int has_declared_bound(translator *tr, const json_value *p,
                              const char *pname) {
  const json_value *loc = json_get(p, "loc");
  if (!loc || json_get(loc, "spellingLoc") || json_get(loc, "expansionLoc") ||
      json_get(loc, "includedFrom"))
    return 0;
  long long off = json_int(json_get(loc, "offset"), -1);
  long long len = json_int(json_get(loc, "tokLen"), 0);
  if (off < 0 || len <= 0 || (size_t)(off + len) > tr->sm->length ||
      (size_t)len != strlen(pname) ||
      strncmp(tr->sm->text + off, pname, (size_t)len) != 0)
    return 0;
  const char *s = tr->sm->text + off + len;
  const char *end = tr->sm->text + tr->sm->length;
  while (s < end && isspace((unsigned char)*s))
    ++s;
  if (s == end || *s != '[')
    return 0;
  ++s;
  static const char *const quals[] = {"restrict", "__restrict", "__restrict__",
                                      "const", "volatile", "static"};
  for (;;) {
    while (s < end && isspace((unsigned char)*s))
      ++s;
    int skipped = 0;
    for (size_t q = 0; q < sizeof quals / sizeof *quals; ++q) {
      size_t n = strlen(quals[q]);
      if ((size_t)(end - s) > n && strncmp(s, quals[q], n) == 0 &&
          !isalnum((unsigned char)s[n]) && s[n] != '_') {
        s += n;
        skipped = 1;
        break;
      }
    }
    if (!skipped)
      break;
  }
  return s < end && *s != ']' && *s != '*';
}

/* --gather-subscripts: the leading bound written in the parameter's
   declarator when it is an integer literal -- float A[32][12],
   A[restrict 32][12], A[static 32][12] -- else -1 (a macro, an expression,
   none). Read from the source text as has_declared_bound reads it. */
static long long declared_literal_bound(translator *tr, const json_value *p,
                                        const char *pname) {
  if (!has_declared_bound(tr, p, pname))
    return -1;
  const json_value *loc = json_get(p, "loc");
  long long off = json_int(json_get(loc, "offset"), -1);
  const char *s = tr->sm->text + off + strlen(pname);
  const char *end = tr->sm->text + tr->sm->length;
  while (s < end && *s != '[')
    ++s;
  ++s;
  static const char *const quals[] = {"restrict", "__restrict", "__restrict__",
                                      "const", "volatile", "static"};
  for (;;) {
    while (s < end && isspace((unsigned char)*s))
      ++s;
    int skipped = 0;
    for (size_t q = 0; q < sizeof quals / sizeof *quals; ++q) {
      size_t n = strlen(quals[q]);
      if ((size_t)(end - s) > n && strncmp(s, quals[q], n) == 0 &&
          !isalnum((unsigned char)s[n]) && s[n] != '_') {
        s += n;
        skipped = 1;
        break;
      }
    }
    if (!skipped)
      break;
  }
  long long v = 0;
  int digits = 0;
  while (s < end && isdigit((unsigned char)*s)) {
    if (v > (LLONG_MAX - 9) / 10)
      return -1;
    v = 10 * v + (*s++ - '0');
    ++digits;
  }
  while (s < end && isspace((unsigned char)*s))
    ++s;
  return digits && v > 0 && s < end && *s == ']' ? v : -1;
}

static char *parameter(translator *tr, const json_value *p) {
  char *t = strip_qualifiers(qual_type(p));
  const char *pname = json_string(json_get(p, "name"), "");
  /* float (*restrict)[16][8]  /  float (*)[16]  /  float *restrict */
  const char *elem = NULL;
  int noalias = 0, rank = 0, dims[MAX_RANK] = {0};
  const char *base = strncmp(t, "float ", 6) == 0    ? "float"
                     : strncmp(t, "double ", 7) == 0 ? "double" : NULL;
  if (base) {
    const char *r = t + strlen(base) + 1;
    int matched = 0;
    if (r[0] == '(' && r[1] == '*') {
      r += 2;
      while (*r == ' ' || *r == '\t')
        ++r;
      int na = 0;
      if (strncmp(r, "__restrict", 10) == 0) {
        r += 10;
        na = 1;
      } else if (strncmp(r, "restrict", 8) == 0) {
        r += 8;
        na = 1;
      }
      if (*r == ')') {
        ++r;
        int d = 0;
        while (*r == '[') {
          const char *q = r + 1;
          if (!isdigit((unsigned char)*q))
            break;
          long v = 0;
          while (isdigit((unsigned char)*q))
            v = 10 * v + (*q++ - '0');
          if (*q != ']' || d == MAX_RANK - 1)
            break;
          dims[d++] = (int)v;
          r = q + 1;
        }
        if (d > 0 && *r == 0) {
          matched = 1;
          noalias = na;
          rank = 1 + d;
        }
      }
    } else if (r[0] == '*') {
      ++r;
      while (*r == ' ' || *r == '\t')
        ++r;
      int na = 0;
      if (strcmp(r, "__restrict") == 0)
        na = 1;
      else if (strcmp(r, "restrict") == 0)
        na = 1;
      if (na || *r == 0) {
        matched = 1;
        noalias = na;
        rank = 1;
      }
    }
    if (matched)
      elem = float_type(base);
  }
  /* --gather-subscripts: a one-dimensional integer array, int *T,
     const int T[restrict 16], read only as the T[E] of a gather. */
  int index_table = 0, table_signed = 0;
  if (!elem && tr->gather_subscripts) {
    static const struct {
      const char *name;
      int bits, is_signed;
    } kTables[] = {{"int", 32, 1},           {"unsigned int", 32, 0},
                   {"int32_t", 32, 1},       {"uint32_t", 32, 0},
                   {"long", 64, 1},          {"long long", 64, 1},
                   {"unsigned long", 64, 0}, {"unsigned long long", 64, 0},
                   {"int64_t", 64, 1},       {"uint64_t", 64, 0}};
    for (size_t k = 0; k < sizeof kTables / sizeof *kTables && !elem; ++k) {
      size_t n = strlen(kTables[k].name);
      if (strncmp(t, kTables[k].name, n) != 0 || t[n] != ' ' || t[n + 1] != '*')
        continue;
      const char *r = t + n + 2;
      while (*r == ' ' || *r == '\t')
        ++r;
      int na = strcmp(r, "__restrict") == 0 || strcmp(r, "restrict") == 0;
      if (!na && *r != 0)
        continue;
      noalias = na;
      rank = 1;
      elem = kTables[k].bits == 32 ? "i32" : "i64";
      index_table = 1;
      table_signed = kTables[k].is_signed;
    }
  }
  if (!elem) {
    const char *ft = float_type(t);
    if (ft) {
      char *name = unique(tr, pname);
      env_entry *e = env_add(tr, json_string(json_get(p, "id"), ""),
                             ENV_SCALAR);
      e->name = name;
      e->elem = ft;
      return format("%s: %s %s", name, ft, loc_text(tr, p));
    }
    if (int_type_max(t, NULL)) {
      /* An integer parameter: an affine symbol of the function's nests
         (usable as an additive subscript term when signed), passed as i32 /
         i64 and cast to index at the function's entry (see function()). A
         use as a loop bound is still outside the subset. */
      char *name = unique(tr, pname);
      env_entry *e = env_add(tr, json_string(json_get(p, "id"), ""),
                             ENV_INTSCALAR);
      e->name = name;
      e->cname = xstrdup(pname);
      e->is_signed = strstr(t, "unsigned") == NULL && strcmp(t, "_Bool") != 0;
      /* Reserved like every other SSA name of the function (name is
         "%stem"): a loop counter or parameter spelled <stem>_idx gets
         another name, whichever comes first. */
      e->sym_name = unique(tr, format("%s_idx", name + 1));
      e->bits = strstr(t, "long") ? 64 : 32;
      return format("%s: i%d %s", name, e->bits, loc_text(tr, p));
    }
    unsupported(p, "parameter '%s' of type '%s' is not a float/double array "
                   "or scalar", pname, qual_type(p));
  }
  char *name = unique(tr, pname);
  char type[256];
  int o = snprintf(type, sizeof type, "memref<?");
  for (int d = 0; d < rank - 1; ++d)
    o += snprintf(type + o, sizeof type - (size_t)o, "x%d", dims[d]);
  snprintf(type + o, sizeof type - (size_t)o, "x%s>", elem);
  env_entry *e = env_add(tr, json_string(json_get(p, "id"), ""), ENV_MEMREF);
  e->name = name;
  e->rank = rank;
  e->elem = elem;
  e->type = xstrdup(type);
  for (int d = 0; d < rank - 1; ++d)
    e->inner_dims[d] = dims[d];
  e->index_table = index_table;
  e->is_signed = table_signed;
  e->cname = xstrdup(pname);
  e->declared_extent = -1;
  if (tr->static_extents) {
    /* The type is known once the accesses are (static_extent_types). */
    e->dynamic_type = e->type;
    e->type = format("@@systolic-cfront-param-%zu@@", tr->env_n - 1);
    e->declared_bound = has_declared_bound(tr, p, pname);
    if (tr->gather_subscripts)
      e->declared_extent = declared_literal_bound(tr, p, pname);
  }
  return format("%s: %s%s %s", name, e->type, noalias ? " {llvm.noalias}" : "",
                loc_text(tr, p));
}

static void json_escape(FILE *out, const char *s) {
  fputc('"', out);
  for (; *s; ++s) {
    unsigned char c = (unsigned char)*s;
    if (c == '"' || c == '\\')
      fprintf(out, "\\%c", c);
    else if (c < 0x20)
      fprintf(out, "\\u%04x", c);
    else
      fputc(c, out);
  }
  fputc('"', out);
}

/* What a caller-side shim needs to call the translated function from C: the
   C parameters, the memref shape each becomes (with the exact extent of the
   dimension the C type leaves open), which arrays are read and written, and
   where the definition is in the source. Appended to tr->interfaces. */
static void interface(translator *tr, const json_value *fn,
                      const json_value *const *params, size_t nparams,
                      const json_value *body) {
  char *buf = NULL;
  size_t size = 0;
  FILE *out = open_memstream(&buf, &size);
  long long offset = node_offset(fn);
  long line = 0, col = 0, end_line = 0, unused;
  if (offset >= 0)
    source_map_line_col(tr->sm, offset, &line, &col);
  const json_value *loc = json_get(fn, "loc");
  const json_value *rng = json_get(fn, "range");
  const json_value *rbegin = json_get(rng, "begin");
  const json_value *rend = json_get(rng, "end");
  const json_value *rend_exp = json_get(rend, "expansionLoc");
  long long end_off = json_int(json_get(rend_exp ? rend_exp : rend, "offset"),
                               offset >= 0 ? offset : 0);
  source_map_line_col(tr->sm, end_off, &end_line, &unused);
  /* A definition written by a macro cannot be rewritten in place. */
  int from_macro = json_get(loc, "spellingLoc") || json_get(rbegin, "spellingLoc") ||
                   json_get(rend, "spellingLoc");
  const char *name = json_string(json_get(fn, "name"), "");

  fprintf(out, "{\"name\": ");
  json_escape(out, name);
  fprintf(out, ", \"symbol\": ");
  char *symbol = format("%s%s", name, tr->symbol_suffix);
  json_escape(out, symbol);
  fprintf(out, ", \"line\": %ld, \"col\": %ld, \"end_line\": %ld", line, col,
          end_line);
  fprintf(out, ", \"storage_class\": ");
  json_escape(out, json_string(json_get(fn, "storageClass"), ""));
  fprintf(out, ", \"inline\": %s, \"from_macro\": %s",
          json_bool(json_get(fn, "inline"), 0) ? "true" : "false",
          from_macro ? "true" : "false");
  fprintf(out, ", \"def_begin\": %lld, \"def_end\": %lld",
          json_int(json_get(rbegin, "offset"), -1),
          json_int(json_get(rend, "offset"), -1) +
              json_int(json_get(rend, "tokLen"), 0));
  fprintf(out, ", \"name_offset\": %lld, \"name_length\": %lld",
          json_int(json_get(loc, "offset"), -1),
          json_int(json_get(loc, "tokLen"), 0));
  fprintf(out, ", \"body_begin\": %lld, \"params\": [", node_offset(body));
  for (size_t i = 0; i < nparams; ++i) {
    const json_value *p = params[i];
    env_entry *e = env_find(tr, json_string(json_get(p, "id"), ""));
    fprintf(out, "%s{\"name\": ", i ? ", " : "");
    json_escape(out, json_string(json_get(p, "name"), ""));
    fprintf(out, ", \"ctype\": ");
    json_escape(out, qual_type(p));
    const char *kind = e->kind == ENV_MEMREF   ? "memref"
                       : e->kind == ENV_SCALAR ? "scalar" : "intscalar";
    fprintf(out, ", \"kind\": \"%s\"", kind);
    if (e->kind == ENV_MEMREF) {
      access_info *info = NULL;
      for (size_t a = 0; a < tr->acc_n; ++a)
        if (strcmp(tr->accesses[a].decl, e->decl) == 0)
          info = &tr->accesses[a];
      /* Rows [0, extent): the open leading dimension is as long as the
         accesses reach (0: never accessed). */
      int ranges = info && info->has_ranges;
      long long extent = ranges ? info->hi[0] + 1 : 0;
      long long minimum = ranges ? info->lo[0] : 0;
      if (info && info->gathered) {
        /* The rows a gather reads are checked by the shim against this. */
        if (extent < info->gather_extent)
          extent = info->gather_extent;
        if (minimum > 0)
          minimum = 0;
      }
      fprintf(out, ", \"elem\": \"%s\", \"leading_extent\": %lld, "
                   "\"leading_min\": %lld",
              e->elem, extent, minimum);
      if (info && info->gathered && info->gather_table) {
        fprintf(out, ", \"gather_table\": ");
        json_escape(out, info->gather_table);
        fprintf(out, ", \"gather_extent\": %lld", info->gather_extent);
      }
      if (info && info->scatter_json)
        fprintf(out, ", %s, \"scatter_extent\": %lld", info->scatter_json,
                info->gather_extent);
      if (e->index_table)
        fprintf(out, ", \"index_table\": true, \"signed\": %s",
                e->is_signed ? "true" : "false");
      /* Subscripts in parameters: the leading extent and minimum as C
         expressions of the parameters, exact at run time. */
      if (ranges && info->hi_expr) {
        fprintf(out, ", \"leading_extent_expr\": ");
        char *ext = format("(%s + 1LL)", info->hi_expr);
        json_escape(out, ext);
        free(ext);
        fprintf(out, ", \"leading_min_expr\": ");
        json_escape(out, info->lo_expr);
      }
      fprintf(out, ", \"inner_dims\": [");
      for (int d = 0; d < e->rank - 1; ++d)
        fprintf(out, "%s%d", d ? ", " : "", e->inner_dims[d]);
      fprintf(out, "], \"read\": %s, \"written\": %s, \"noalias\": %s",
              info && info->read ? "true" : "false",
              info && info->written ? "true" : "false",
              strstr(qual_type(p), "restrict") ? "true" : "false");
    } else if (e->kind == ENV_SCALAR) {
      fprintf(out, ", \"elem\": \"%s\"", e->elem);
    }
    fprintf(out, "}");
  }
  fprintf(out, "]}");
  fclose(out);
  char *joined = tr->interfaces ? format("%s,\n  %s", tr->interfaces, buf)
                                : format("  %s", buf);
  free(tr->interfaces);
  free(buf);
  tr->interfaces = joined;
}

/* Every occurrence of `from` in `s` replaced by `to`, in a new string. */
static char *replace_all(const char *s, const char *from, const char *to) {
  size_t nf = strlen(from), nt = strlen(to), count = 0;
  for (const char *q = strstr(s, from); q; q = strstr(q + nf, from))
    ++count;
  char *out = xmalloc(strlen(s) + count * nt + 1);
  char *w = out;
  for (const char *q; (q = strstr(s, from)); s = q + nf) {
    memcpy(w, s, (size_t)(q - s));
    w += q - s;
    memcpy(w, to, nt);
    w += nt;
  }
  strcpy(w, s);
  return out;
}

/* --static-extents, once the function is translated: each array parameter's
   type in the lines from `first_line` on -- memref<EXTENTx...> for one with
   a declared leading bound whose accesses' ranges are static and start at
   row 0 or later (EXTENT = the interface's leading extent), else the
   memref<?x...> it has without the option. */
static void static_extent_types(translator *tr, size_t first_line) {
  for (size_t i = 0; i < tr->env_n; ++i) {
    env_entry *e = &tr->env[i];
    if (e->kind != ENV_MEMREF || !e->dynamic_type)
      continue;
    access_info *info = NULL;
    for (size_t a = 0; a < tr->acc_n; ++a)
      if (strcmp(tr->accesses[a].decl, e->decl) == 0)
        info = &tr->accesses[a];
    char *type;
    if (info && info->gathered) {
      /* The literal bound written; any affine access must stay inside it. */
      long long ext = info->gather_extent;
      if (!info->has_ranges ||
          (!info->hi_expr && info->lo[0] >= 0 && info->hi[0] < ext)) {
        char buf[256];
        int o = snprintf(buf, sizeof buf, "memref<%lld", ext);
        for (int d = 0; d < e->rank - 1; ++d)
          o += snprintf(buf + o, sizeof buf - (size_t)o, "x%d", e->inner_dims[d]);
        snprintf(buf + o, sizeof buf - (size_t)o, "x%s>", e->elem);
        type = xstrdup(buf);
      } else {
        type = xstrdup(e->dynamic_type);
      }
    } else if (e->declared_bound && info && info->has_ranges && !info->hi_expr &&
        info->lo[0] >= 0 && info->hi[0] < LLONG_MAX) {
      char buf[256];
      int o = snprintf(buf, sizeof buf, "memref<%lld", info->hi[0] + 1);
      for (int d = 0; d < e->rank - 1; ++d)
        o += snprintf(buf + o, sizeof buf - (size_t)o, "x%d", e->inner_dims[d]);
      snprintf(buf + o, sizeof buf - (size_t)o, "x%s>", e->elem);
      type = xstrdup(buf);
    } else {
      type = xstrdup(e->dynamic_type);
    }
    for (size_t l = first_line; l < tr->lines.n; ++l) {
      if (!strstr(tr->lines.items[l], e->type))
        continue;
      char *line = replace_all(tr->lines.items[l], e->type, type);
      free((void *)(uintptr_t)tr->lines.items[l]);
      tr->lines.items[l] = line;
    }
    free(e->type);
    e->type = type;
    free(e->dynamic_type);
    e->dynamic_type = NULL;
  }
}

static void function(translator *tr, const json_value *fn) {
  char *ret = xstrdup(qual_type(fn));
  char *paren = strchr(ret, '(');
  if (paren)
    *paren = 0;
  char *rt = ret;   /* trimmed, qualifiers kept */
  while (*rt == ' ')
    ++rt;
  for (size_t e = strlen(rt); e > 0 && rt[e - 1] == ' '; --e)
    rt[e - 1] = 0;
  if (strcmp(rt, "void") != 0)
    unsupported(fn, "only functions returning void are supported (got '%s')",
                rt);
  const json_value *params[64];
  size_t nparams = 0;
  const json_value *body = NULL;
  size_t nbody = 0;
  for (size_t i = 0; i < child_count(fn); ++i) {
    const json_value *c = child(fn, i);
    if (strcmp(kind_of(c), "ParmVarDecl") == 0) {
      if (nparams == 64)
        unsupported(c, "too many parameters");
      params[nparams++] = c;
    } else if (strcmp(kind_of(c), "CompoundStmt") == 0) {
      body = c;
      ++nbody;
    }
  }
  if (nbody != 1)
    unsupported(fn, "function has no body");

  tr->ssa_names.n = 0;   /* SSA names are scoped to the function */
  tr->counter = 0;
  tr->env_n = 0;
  tr->acc_n = 0;
  /* A function abandoned through unsupported() inside a subscript leaves the
     flag set (array_access clears it on its normal exit only): every function
     starts outside a subscript, where an integer parameter is no symbol. */
  tr->in_subscript = 0;
  tr->in_gather = 0;
  tr->gather.active = 0;
  tr->general.active = 0;
  char *args = xstrdup("");
  for (size_t i = 0; i < nparams; ++i) {
    if (!json_get(params[i], "name"))
      unsupported(params[i], "unnamed parameters are not supported");
    char *a = parameter(tr, params[i]);
    char *joined = format("%s%s%s", args, i ? ", " : "", a);
    free(args);
    args = joined;
  }
  const size_t first_line = tr->lines.n;
  emit(tr, 1, "func.func @%s%s(%s) {", json_string(json_get(fn, "name"), ""),
       tr->symbol_suffix, args);
  /* Every integer parameter as an index at the top of the function: a
     top-level value, hence a valid affine symbol for the nests below. */
  for (size_t i = 0; i < tr->env_n; ++i) {
    env_entry *e = &tr->env[i];
    if (e->kind == ENV_INTSCALAR)
      emit(tr, 2, "%s = arith.index_cast %s : i%d to index %s", e->sym_name,
           e->name, e->bits, loc_text(tr, fn));
  }
  statements(tr, json_get(body, "inner"), 2, 1);
  emit(tr, 2, "func.return %s", loc_text(tr, body));
  emit(tr, 1, "} %s", loc_text(tr, fn));
  interface(tr, fn, params, nparams, body);
  if (tr->static_extents)
    static_extent_types(tr, first_line);
}

/* ------------------------------------------------------------ GEMM sites */

/* The sites of the GEMM-like loop nests of a function, for labelling only:
   which loop nests a program's GEMMs are (they get a "GEMM #N" execution
   report), never what is translated or offloaded -- that is decided by the
   translation above and by --affine-matmul-to-systolic.

   A site is a multiply-accumulate statement -- L += P, L -= P, L = L + P or
   L = L - P (either order), where P is a product with at least two array
   reads among its factors -- whose counters span at least three enclosing
   loops (for, while, do), at any depth and with other loops between them,
   one of which is a reduction (its counter is in P but not in L): the
   outermost of those loops. One loop nest is one site however many loops
   it has; a matrix-vector product (two loops) or an element-wise nest (no
   reduction) is none. Loops between the site's and the statement that the
   statement does not use repeat the accumulation within one execution of
   the site; loops outside the site run it `executions` times per call of
   the function (the product of their constant trip counts, -1 when one is
   not a for loop with constant bounds). */

typedef struct {
  const json_value *loop;
  strvec ids;   /* the counters: the for's declared or assigned variable; a
                   while's / do's condition variables */
  long long trips;   /* its constant trip count, -1 if not known */
} loop_frame;

/* The constant trip count of `for (int i = LB; i < UB (or <=); ++i / i++ /
   i += S / i = i + S)` or of the counting-down form `for (int i = LB; i > B
   (or >=); --i / i-- / i -= S / i = i - S)`, else -1 (as for_stmt counts). */
static long long for_trip_count(const json_value *n) {
  const json_value *init = child(n, 0), *cond = child(n, 2), *inc = child(n, 3);
  long long lb, bound, step;
  const json_value *lb_expr = NULL;
  if (strcmp(kind_of(init), "DeclStmt") == 0 && child_count(init) == 1 &&
      child_count(child(init, 0)) > 0)
    lb_expr = child(child(init, 0), 0);
  else if (strcmp(kind_of(init), "BinaryOperator") == 0 &&
           strcmp(opcode_of(init), "=") == 0)
    lb_expr = child(init, 1);
  if (!lb_expr || !try_int_constant(lb_expr, &lb))
    return -1;
  const char *cop = opcode_of(cond);
  int up = strcmp(cop, "<") == 0 || strcmp(cop, "<=") == 0;
  int down = strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0;
  /* the counter: the declared or assigned variable */
  const char *decl = NULL;
  if (strcmp(kind_of(init), "DeclStmt") == 0 && child_count(init) == 1)
    decl = json_string(json_get(child(init, 0), "id"), NULL);
  else if (strcmp(kind_of(init), "BinaryOperator") == 0)
    decl = referenced_id(strip_parens_and_casts(child(init, 0), kNoOp));
  if (strcmp(kind_of(cond), "BinaryOperator") == 0 && (up || down) && decl &&
      !is_ref_to(child(cond, 0), decl) &&
      affine_condition(cond, decl, &cop, &bound)) {
    up = cop[0] == '<';
    down = !up;
  } else if (strcmp(kind_of(cond), "BinaryOperator") != 0 || !(up || down) ||
             !is_ref_to(child(cond, 0), decl) ||
             !try_int_constant(child(cond, 1), &bound)) {
    /* not `counter OP constant`: a condition such as s + r < 3 reads the
       enclosing counters (its trip count varies; site_executions runs it) */
    return -1;
  }
  const char *iop = opcode_of(inc);
  if (strcmp(kind_of(inc), "UnaryOperator") == 0 &&
      (strcmp(iop, "++") == 0 || strcmp(iop, "--") == 0)) {
    step = iop[0] == '+' ? 1 : -1;
  } else if (strcmp(kind_of(inc), "CompoundAssignOperator") == 0 &&
             (strcmp(iop, "+=") == 0 || strcmp(iop, "-=") == 0)) {
    if (!try_int_constant(child(inc, 1), &step) ||
        __builtin_mul_overflow(step, iop[0] == '-' ? -1 : 1, &step))
      return -1;
  } else if (strcmp(kind_of(inc), "BinaryOperator") == 0 &&
             strcmp(iop, "=") == 0) {
    const json_value *rhs = strip_parens_and_casts(child(inc, 1),
                                                   kLValueToRValue);
    const char *rop = opcode_of(rhs);
    if (strcmp(kind_of(rhs), "BinaryOperator") != 0 ||
        (strcmp(rop, "+") != 0 && strcmp(rop, "-") != 0) ||
        !try_int_constant(child(rhs, 1), &step) ||
        __builtin_mul_overflow(step, rop[0] == '-' ? -1 : 1, &step))
      return -1;
  } else {
    return -1;
  }
  __int128 trips;
  if (up && step > 0) {
    __int128 ub = (__int128)bound + (strcmp(cop, "<=") == 0);
    trips = ub > lb ? (ub - 1 - lb) / step + 1 : 0;
  } else if (down && step < 0) {
    __int128 lowest = (__int128)bound + (strcmp(cop, ">") == 0);
    trips = (__int128)lb >= lowest ? ((__int128)lb - lowest) / -(__int128)step + 1
                                   : 0;
  } else {
    return -1;
  }
  return trips > (__int128)9223372036854775807LL ? -1 : (long long)trips;
}

/* A for loop whose iterations are counted by running it (site_executions):
   its counter (declared or assigned), its initial value and condition --
   integer expressions of the counter and the enclosing loops' counters,
   evaluated at their values --, and its constant step (++, --, += S, -= S,
   = i + S, = i - S). 0 when it is not of that shape. */
typedef struct {
  const json_value *init_expr, *cond;
  const char *counter;
  long long step;
} site_loop;

static int site_loop_of(const json_value *n, site_loop *out) {
  const json_value *init = child(n, 0), *cond = child(n, 2), *inc = child(n, 3);
  memset(out, 0, sizeof *out);
  if (strcmp(kind_of(init), "DeclStmt") == 0 && child_count(init) == 1 &&
      child_count(child(init, 0)) > 0) {
    out->init_expr = child(child(init, 0), 0);
    out->counter = json_string(json_get(child(init, 0), "id"), NULL);
  } else if (strcmp(kind_of(init), "BinaryOperator") == 0 &&
             strcmp(opcode_of(init), "=") == 0) {
    out->init_expr = child(init, 1);
    out->counter = referenced_id(strip_parens_and_casts(child(init, 0), kNoOp));
  }
  const char *cop = opcode_of(cond);
  if (!out->init_expr || !out->counter ||
      strcmp(kind_of(cond), "BinaryOperator") != 0 ||
      !(strcmp(cop, "<") == 0 || strcmp(cop, "<=") == 0 ||
        strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0 ||
        strcmp(cop, "&&") == 0))
    return 0;
  out->cond = cond;
  const char *iop = opcode_of(inc);
  long long step = 0;
  if (strcmp(kind_of(inc), "UnaryOperator") == 0 &&
      (strcmp(iop, "++") == 0 || strcmp(iop, "--") == 0)) {
    step = iop[0] == '+' ? 1 : -1;
  } else if (strcmp(kind_of(inc), "CompoundAssignOperator") == 0 &&
             (strcmp(iop, "+=") == 0 || strcmp(iop, "-=") == 0)) {
    if (!try_int_constant(child(inc, 1), &step) ||
        __builtin_mul_overflow(step, iop[0] == '-' ? -1 : 1, &step))
      return 0;
  } else if (strcmp(kind_of(inc), "BinaryOperator") == 0 &&
             strcmp(iop, "=") == 0) {
    const json_value *rhs = strip_parens_and_casts(child(inc, 1),
                                                   kLValueToRValue);
    const char *rop = opcode_of(rhs);
    if (strcmp(kind_of(rhs), "BinaryOperator") != 0 ||
        (strcmp(rop, "+") != 0 && strcmp(rop, "-") != 0) ||
        !try_int_constant(child(rhs, 1), &step) ||
        __builtin_mul_overflow(step, rop[0] == '-' ? -1 : 1, &step))
      return 0;
  } else {
    return 0;
  }
  if (step == 0)
    return 0;
  out->step = step;
  return 1;
}

static int site_holds(const json_value *n, const char *const *ids,
                      const long long *values, size_t count);

/* The value of an integer expression of the counters ids[0 .. count) at
   their values: literals, integer constant expressions, references to those
   counters, unary + / -, + - *, parentheses and integer casts, and a
   conditional expression of a comparison (the MIN macro, S5.5). 0 when it is
   not one, or a value leaves 2^100. */
static int site_eval(const json_value *n, const char *const *ids,
                     const long long *values, size_t count, __int128 *out) {
  const __int128 kLimit = (__int128)1 << 100;
  n = strip_parens_and_casts(n, kIntConstantCasts);
  const char *kind = kind_of(n);
  const char *op = opcode_of(n);
  long long v;
  if (strcmp(kind, "DeclRefExpr") == 0) {
    const char *id = referenced_id(n);
    for (size_t d = count; d-- > 0;)
      if (id && ids[d] && strcmp(ids[d], id) == 0) {
        *out = values[d];
        return 1;
      }
    return 0;
  }
  if (try_int_constant(n, &v)) {
    *out = v;
    return 1;
  }
  __int128 x, y;
  if (strcmp(kind, "ConditionalOperator") == 0) {
    const int holds = site_holds(child(n, 0), ids, values, count);
    return holds >= 0 &&
           site_eval(child(n, holds ? 1 : 2), ids, values, count, out);
  }
  if (strcmp(kind, "UnaryOperator") == 0 &&
      (strcmp(op, "-") == 0 || strcmp(op, "+") == 0)) {
    if (!site_eval(child(n, 0), ids, values, count, &x))
      return 0;
    *out = op[0] == '-' ? -x : x;
    return 1;
  }
  if (strcmp(kind, "BinaryOperator") != 0 ||
      !(strcmp(op, "+") == 0 || strcmp(op, "-") == 0 || strcmp(op, "*") == 0))
    return 0;
  if (!site_eval(child(n, 0), ids, values, count, &x) ||
      !site_eval(child(n, 1), ids, values, count, &y))
    return 0;
  if (op[0] == '*') {
    const __int128 lim = (__int128)1 << 50;
    if (x > lim || x < -lim || y > lim || y < -lim)
      return 0;
    *out = x * y;
  } else {
    *out = op[0] == '+' ? x + y : x - y;
  }
  return *out < kLimit && *out > -kLimit;
}

/* Whether a comparison of integer expressions of the counters ids[0 ..
   count) holds at their values -- or two joined by && (S5.5: counter < a &&
   counter < b): 1 or 0; -1 when it is not one or not evaluable. */
static int site_holds(const json_value *n, const char *const *ids,
                      const long long *values, size_t count) {
  n = strip_parens_and_casts(n, kIntConstantCasts);
  const char *op = opcode_of(n);
  if (strcmp(kind_of(n), "BinaryOperator") != 0)
    return -1;
  if (strcmp(op, "&&") == 0) {
    const int first = site_holds(child(n, 0), ids, values, count);
    return first <= 0 ? first : site_holds(child(n, 1), ids, values, count);
  }
  __int128 lhs, rhs;
  if (!(strcmp(op, "<") == 0 || strcmp(op, "<=") == 0 ||
        strcmp(op, ">") == 0 || strcmp(op, ">=") == 0) ||
      !site_eval(child(n, 0), ids, values, count, &lhs) ||
      !site_eval(child(n, 1), ids, values, count, &rhs))
    return -1;
  return strcmp(op, "<") == 0    ? lhs < rhs
         : strcmp(op, "<=") == 0 ? lhs <= rhs
         : strcmp(op, ">") == 0  ? lhs > rhs
                                 : lhs >= rhs;
}

typedef struct {
  const site_loop *loops;
  size_t depth;
  const char *ids[257];
  long long values[257];
  size_t visited;
  long long count;
  int ok;
} site_run;

static void site_run_loop(site_run *r, size_t d) {
  if (!r->ok)
    return;
  if (d == r->depth) {
    ++r->count;
    return;
  }
  const site_loop *l = &r->loops[d];
  __int128 v;
  if (!site_eval(l->init_expr, r->ids, r->values, d, &v)) {
    r->ok = 0;
    return;
  }
  r->ids[d] = l->counter;
  for (;;) {
    if (v > LLONG_MAX || v < LLONG_MIN) {
      r->ok = 0;
      return;
    }
    r->values[d] = (long long)v;
    const int holds = site_holds(l->cond, r->ids, r->values, d + 1);
    if (holds < 0) {
      r->ok = 0;
      return;
    }
    if (!holds)
      return;
    if (++r->visited > kEnumerationLimit) {
      r->ok = 0;
      return;
    }
    site_run_loop(r, d + 1);
    if (!r->ok)
      return;
    v += l->step;
  }
}

/* The number of points of the nest of the for loops stack[0 .. depth) --
   how often a site inside them runs per call --, counted by running their
   counters (each loop's initial value and condition evaluated at the outer
   counters' values): exact for bounds affine in the enclosing counters,
   constant extent or not (docs/AFFINE_GEMM_MAPPING.md section 19.13). -1
   when a loop is not a for loop of site_loop_of's shape, an expression is
   not evaluable, or more than kEnumerationLimit values are visited. */
static long long site_executions(const loop_frame *stack, size_t depth) {
  static site_loop loops[257];
  for (size_t d = 0; d < depth; ++d)
    if (strcmp(kind_of(stack[d].loop), "ForStmt") != 0 ||
        !site_loop_of(stack[d].loop, &loops[d]))
      return -1;
  static site_run r;
  memset(&r, 0, sizeof r);
  r.loops = loops;
  r.depth = depth;
  r.ok = 1;
  site_run_loop(&r, 0);
  return r.ok ? r.count : -1;
}

static void collect_decl_ids(const json_value *n, strvec *ids) {
  if (!n || n->kind != JSON_OBJECT)
    return;
  if (strcmp(kind_of(n), "DeclRefExpr") == 0) {
    const char *id = referenced_id(n);
    if (id && !strvec_contains(ids, id))
      strvec_push(ids, id);
  }
  for (size_t i = 0; i < child_count(n); ++i)
    collect_decl_ids(child(n, i), ids);
}

static const json_value *strip_value(const json_value *n) {
  for (;;) {
    const char *k = kind_of(n);
    if (strcmp(k, "ParenExpr") == 0 || strcmp(k, "ImplicitCastExpr") == 0 ||
        strcmp(k, "CStyleCastExpr") == 0)
      n = child(n, 0);
    else
      return n;
  }
}

/* Array reads among the factors of a product. */
static int product_reads(const json_value *n) {
  n = strip_value(n);
  const char *k = kind_of(n);
  if (strcmp(k, "BinaryOperator") == 0 && strcmp(opcode_of(n), "*") == 0)
    return product_reads(child(n, 0)) + product_reads(child(n, 1));
  if (strcmp(k, "ArraySubscriptExpr") == 0)
    return 1;
  if (strcmp(k, "UnaryOperator") == 0 && strcmp(opcode_of(n), "*") == 0)
    return 1;
  return 0;
}

static int is_product_of_reads(const json_value *n) {
  n = strip_value(n);
  return strcmp(kind_of(n), "BinaryOperator") == 0 &&
         strcmp(opcode_of(n), "*") == 0 && product_reads(n) >= 2;
}

/* A multiply-accumulate statement: its target L and its product P. */
static int mac_parts(const json_value *n, const json_value **lhs,
                     const json_value **product) {
  const char *k = kind_of(n), *op = opcode_of(n);
  *lhs = child(n, 0);
  if (strcmp(k, "CompoundAssignOperator") == 0 &&
      (strcmp(op, "+=") == 0 || strcmp(op, "-=") == 0)) {
    *product = child(n, 1);
    return is_product_of_reads(*product);
  }
  if (strcmp(k, "BinaryOperator") == 0 && strcmp(op, "=") == 0) {
    const json_value *r = strip_value(child(n, 1));
    const char *rop = opcode_of(r);
    /* Assignment/last-write and K=1 products are already supported by the
       compiler; discover their original sites instead of inventing them
       after lowering. This is metadata, not recognition or legality. */
    if (is_product_of_reads(r)) {
      *product = r;
      return 1;
    }
    if (strcmp(kind_of(r), "BinaryOperator") == 0 &&
        (strcmp(rop, "+") == 0 || strcmp(rop, "-") == 0)) {
      for (int side = 0; side < 2; ++side)
        if (is_product_of_reads(child(r, side))) {
          *product = child(r, side);
          return 1;
        }
    }
  }
  return 0;
}

typedef struct {
  const json_value **loops;
  long long *executions;
  int *report;
  size_t n, cap;
} site_set;

static void find_sites(const json_value *n, loop_frame *stack, size_t depth,
                       site_set *sites) {
  if (!n || n->kind != JSON_OBJECT)
    return;
  const char *k = kind_of(n);
  int is_loop = strcmp(k, "ForStmt") == 0 || strcmp(k, "WhileStmt") == 0 ||
                strcmp(k, "DoStmt") == 0;
  const json_value *lhs, *product;
  if (depth >= 1 && mac_parts(n, &lhs, &product)) {
    strvec in_lhs = {0}, in_product = {0};
    collect_decl_ids(lhs, &in_lhs);
    collect_decl_ids(product, &in_product);
    size_t used = 0, top = depth - 1;
    int reduction = 0;
    for (size_t d = 0; d < depth; ++d) {
      int uses = 0;
      for (size_t c = 0; c < stack[d].ids.n; ++c) {
        const char *id = stack[d].ids.items[c];
        int l = strvec_contains(&in_lhs, id), p = strvec_contains(&in_product, id);
        uses |= l || p;
        reduction |= p && !l;
      }
      if (uses && used++ == 0)
        top = d;
    }
    /* Discover all product candidates, including degenerate/assignment
       GEMMs. Preserve the existing CPU-only reporting policy; candidates
       are promoted to reported GEMMs when uniquely matched to lowering.
       Identity discovery never grants transformation legality. */
    int seen = 0;
    for (size_t s = 0; s < sites->n; ++s)
      if (sites->loops[s] == stack[top].loop) {
        sites->report[s] |= used >= 3 && reduction;
        seen = 1;
      }
    if (!seen) {
      /* Once per site: every statement of it has the same enclosing
         loops. */
      long long executions = 1;
      for (size_t d = 0; d < top; ++d)
        executions = executions < 0 || stack[d].trips < 0
                         ? -1 : executions * stack[d].trips;
      /* Loops whose bounds read the enclosing counters (constant extent or
         a triangle): their points, counted by running them. */
      if (executions < 0)
        executions = site_executions(stack, top);
      if (sites->n == sites->cap) {
        sites->cap = sites->cap ? 2 * sites->cap : 8;
        sites->loops = realloc(sites->loops, sites->cap * sizeof *sites->loops);
        sites->executions = realloc(sites->executions,
                                    sites->cap * sizeof *sites->executions);
        sites->report = realloc(sites->report,
                                sites->cap * sizeof *sites->report);
        if (!sites->loops || !sites->executions || !sites->report) {
          fprintf(stderr, "systolic-cfront: out of memory\n");
          exit(2);
        }
      }
      sites->executions[sites->n] = executions;
      sites->report[sites->n] = used >= 3 && reduction;
      sites->loops[sites->n++] = stack[top].loop;
    }
  }
  if (is_loop) {
    if (depth == 256)
      return;
    loop_frame *f = &stack[depth];
    memset(f, 0, sizeof *f);
    f->loop = n;
    f->trips = strcmp(k, "ForStmt") == 0 ? for_trip_count(n) : -1;
    if (strcmp(k, "ForStmt") == 0) {
      const json_value *init = child(n, 0);
      if (strcmp(kind_of(init), "DeclStmt") == 0) {
        for (size_t v = 0; v < child_count(init); ++v) {
          const char *id = json_string(json_get(child(init, v), "id"), NULL);
          if (id)
            strvec_push(&f->ids, id);
        }
      } else if (strcmp(kind_of(init), "BinaryOperator") == 0 &&
                 strcmp(opcode_of(init), "=") == 0) {
        const json_value *lhs = strip_value(child(init, 0));
        const char *id = referenced_id(lhs);
        if (strcmp(kind_of(lhs), "DeclRefExpr") == 0 && id)
          strvec_push(&f->ids, id);
      }
    } else {
      /* while (cond) / do ... while (cond): the condition's variables */
      collect_decl_ids(strcmp(k, "WhileStmt") == 0 ? child(n, 0)
                                                   : child(n, 1), &f->ids);
    }
    for (size_t i = 0; i < child_count(n); ++i)
      find_sites(child(n, i), stack, depth + 1, sites);
    return;
  }
  for (size_t i = 0; i < child_count(n); ++i)
    find_sites(child(n, i), stack, depth, sites);
}

/* An anchor may move outward when normalization recognizes a whole band.
   Only a chain of single-statement for bodies can name the same site: never
   infer identity from source proximity or a containing range alone. */
static int site_anchor_chain(const json_value *n, const json_value *site) {
  if (n == site)
    return 1;
  if (strcmp(kind_of(n), "ForStmt") != 0 || child_count(n) != 5)
    return 0;
  n = child(n, 4);
  while (strcmp(kind_of(n), "CompoundStmt") == 0 && child_count(n) == 1)
    n = child(n, 0);
  return strcmp(kind_of(n), "ForStmt") == 0 && site_anchor_chain(n, site);
}

static void write_site_anchors(const json_value *n, const json_value *site,
                               const source_map *sm, FILE *out, int *first) {
  if (site_anchor_chain(n, site)) {
    long long begin = node_offset(n);
    long line = 0, col = 0;
    if (begin >= 0)
      source_map_line_col(sm, begin, &line, &col);
    fprintf(out, "%s{\"line\": %ld, \"col\": %ld, \"begin\": %lld}",
            *first ? "" : ", ", line, col, begin);
    *first = 0;
  }
  for (size_t i = 0; i < child_count(n); ++i)
    write_site_anchors(child(n, i), site, sm, out, first);
}

/* Appends one function's sites, as JSON objects, to *out. */
static void write_sites(const json_value *fn, const source_map *sm,
                        FILE *out, int *first) {
  static loop_frame stack[257];
  site_set sites = {0};
  find_sites(fn, stack, 0, &sites);
  for (size_t s = 0; s < sites.n; ++s) {
    const json_value *loop = sites.loops[s];
    const json_value *rng = json_get(loop, "range");
    const json_value *b = json_get(rng, "begin"), *e = json_get(rng, "end");
    int from_macro = json_get(b, "spellingLoc") || json_get(e, "spellingLoc");
    long long begin = node_offset(loop);
    const json_value *eloc = json_get(e, "expansionLoc") ? json_get(e, "expansionLoc") : e;
    long long end = json_int(json_get(eloc, "offset"), -1) +
                    json_int(json_get(eloc, "tokLen"), 0);
    long line = 0, col = 0;
    if (begin >= 0)
      source_map_line_col(sm, begin, &line, &col);
    fprintf(out, "%s  {\"function\": ", *first ? "" : ",\n");
    json_escape(out, json_string(json_get(fn, "name"), ""));
    fprintf(out, ", \"line\": %ld, \"col\": %ld, \"begin\": %lld, "
                 "\"end\": %lld, \"from_macro\": %s, \"anchors\": [",
            line, col, begin, end, from_macro ? "true" : "false");
    int first_anchor = 1;
    write_site_anchors(fn, loop, sm, out, &first_anchor);
    fprintf(out, "], \"report\": %s, \"executions\": %lld}",
            sites.report[s] ? "true" : "false", sites.executions[s]);
    *first = 0;
  }
  free(sites.loops);
  free(sites.executions);
  free(sites.report);
}

/* ------------------------------------------------------------------- main */

/* Runs argv, its stdout into *out (NUL-terminated, length in *outlen), its
   stderr into *err. Returns the exit status, or -1 if it could not run. */
static int run_capture(const char *const argv[], char **out, size_t *outlen,
                       char **err) {
  char errpath[] = "/tmp/systolic-cfront-XXXXXX";
  int errfd = mkstemp(errpath);
  if (errfd < 0)
    return -1;
  unlink(errpath);
  int pipefd[2];
  if (pipe(pipefd) != 0)
    return -1;
  pid_t pid = fork();
  if (pid < 0)
    return -1;
  if (pid == 0) {
    dup2(pipefd[1], 1);
    dup2(errfd, 2);
    close(pipefd[0]);
    close(pipefd[1]);
    size_t argc = 0;
    while (argv[argc])
      ++argc;
    char **args = xmalloc((argc + 1) * sizeof *args);
    for (size_t a = 0; a < argc; ++a)
      args[a] = xstrdup(argv[a]);
    args[argc] = NULL;
    execvp(args[0], args);
    fprintf(stderr, "cannot run %s: %s\n", argv[0], strerror(errno));
    _exit(127);
  }
  close(pipefd[1]);
  size_t cap = 1 << 16, n = 0;
  char *buf = xmalloc(cap);
  for (;;) {
    if (n + 4096 > cap) {
      cap *= 2;
      buf = realloc(buf, cap);
      if (!buf) {
        fprintf(stderr, "systolic-cfront: out of memory\n");
        exit(2);
      }
    }
    ssize_t r = read(pipefd[0], buf + n, cap - n - 1);
    if (r < 0 && errno == EINTR)
      continue;
    if (r <= 0)
      break;
    n += (size_t)r;
  }
  buf[n] = 0;
  close(pipefd[0]);
  int status;
  while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
    ;
  off_t elen = lseek(errfd, 0, SEEK_END);
  char *ebuf = xmalloc((size_t)(elen > 0 ? elen : 0) + 1);
  if (elen > 0) {
    lseek(errfd, 0, SEEK_SET);
    ssize_t got = read(errfd, ebuf, (size_t)elen);
    ebuf[got > 0 ? got : 0] = 0;
  } else {
    ebuf[0] = 0;
  }
  close(errfd);
  *out = buf;
  if (outlen)
    *outlen = n;
  *err = ebuf;
  if (WIFEXITED(status))
    return WEXITSTATUS(status);
  return 128;
}

static char *read_file(const char *path, size_t *length) {
  FILE *f = fopen(path, "rb");
  if (!f)
    return NULL;
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = xmalloc((size_t)n + 1);
  size_t got = fread(buf, 1, (size_t)n, f);
  buf[got] = 0;
  fclose(f);
  *length = got;
  return buf;
}

static void usage(void) {
  fprintf(stderr,
          "usage: systolic-cfront --clang CLANG [--clang-major N] "
          "[-DNAME[=VAL]] [-IDIR] [-std=STD] [--function NAME] "
          "[--skip-unsupported] [--symbol-suffix S] [--interface FILE] "
          "[--gemm-sites FILE] [--fp-reassociation] [--static-extents] "
          "[--modulo-subscripts] [--gather-subscripts] [--scatter-subscripts] "
          "FILE.c [-o OUT.mlir]\n");
}

/* --opt=value or --opt value. */
static const char *option_value(const char *arg, const char *name, int *i,
                                int argc, char **argv) {
  size_t n = strlen(name);
  if (strncmp(arg, name, n) != 0)
    return NULL;
  if (arg[n] == '=')
    return arg + n + 1;
  if (arg[n] == 0) {
    if (*i + 1 >= argc) {
      fprintf(stderr, "systolic-cfront: %s needs a value\n", name);
      exit(2);
    }
    return argv[++*i];
  }
  return NULL;
}

int main(int argc, char **argv) {
  const char *source = NULL, *output = "-", *clang = getenv("SYSTOLIC_CFRONT_CLANG");
  const char *std = "c11", *symbol_suffix = "", *interface_path = NULL;
  const char *sites_path = NULL;
  long clang_major = 24;
  int skip_unsupported = 0, fp_reassociation = 0;
  int static_extents = 0, modulo_subscripts = 0, gather_subscripts = 0;
  int scatter_subscripts = 0;
  strvec defines = {0}, includes = {0}, only = {0};

  for (int i = 1; i < argc; ++i) {
    const char *a = argv[i], *v;
    if (strcmp(a, "-h") == 0 || strcmp(a, "--help") == 0) {
      usage();
      return 0;
    } else if ((v = option_value(a, "--clang-major", &i, argc, argv))) {
      clang_major = strtol(v, NULL, 10);
    } else if ((v = option_value(a, "--clang", &i, argc, argv))) {
      clang = v;
    } else if ((v = option_value(a, "--function", &i, argc, argv))) {
      strvec_push(&only, v);
    } else if ((v = option_value(a, "--symbol-suffix", &i, argc, argv))) {
      symbol_suffix = v;
    } else if ((v = option_value(a, "--interface", &i, argc, argv))) {
      interface_path = v;
    } else if ((v = option_value(a, "--gemm-sites", &i, argc, argv))) {
      sites_path = v;
    } else if (strcmp(a, "--skip-unsupported") == 0) {
      skip_unsupported = 1;
    } else if (strcmp(a, "--fp-reassociation") == 0) {
      fp_reassociation = 1;
    } else if (strcmp(a, "--static-extents") == 0) {
      static_extents = 1;
    } else if (strcmp(a, "--modulo-subscripts") == 0) {
      modulo_subscripts = 1;
    } else if (strcmp(a, "--gather-subscripts") == 0) {
      gather_subscripts = 1;
    } else if (strcmp(a, "--scatter-subscripts") == 0) {
      scatter_subscripts = 1;
    } else if ((v = option_value(a, "--output", &i, argc, argv))) {
      output = v;
    } else if (strcmp(a, "-o") == 0) {
      if (i + 1 >= argc) {
        usage();
        return 2;
      }
      output = argv[++i];
    } else if (strncmp(a, "-std", 4) == 0) {
      if (a[4] == '=')
        std = a + 5;
      else if (a[4] == 0 && i + 1 < argc)
        std = argv[++i];
      else {
        usage();
        return 2;
      }
    } else if (strncmp(a, "-D", 2) == 0 || strncmp(a, "-I", 2) == 0) {
      const char *val = a[2] ? a + 2 : (i + 1 < argc ? argv[++i] : NULL);
      if (!val) {
        usage();
        return 2;
      }
      strvec_push(a[1] == 'D' ? &defines : &includes, val);
    } else if (a[0] == '-' && a[1]) {
      fprintf(stderr, "systolic-cfront: unknown option %s\n", a);
      usage();
      return 2;
    } else if (!source) {
      source = a;
    } else {
      usage();
      return 2;
    }
  }
  if (!source) {
    usage();
    return 2;
  }
  if (!clang) {
    fprintf(stderr, "systolic-cfront: no clang given (--clang or "
                    "SYSTOLIC_CFRONT_CLANG)\n");
    return 2;
  }

  /* The clang must be the one the project is standardized on. */
  char *vout, *verr;
  const char *vargv[] = {clang, "--version", NULL};
  int vrc = run_capture(vargv, &vout, NULL, &verr);
  char *nl = vout ? strchr(vout, '\n') : NULL;
  if (vrc != 0 || !vout || !vout[0]) {
    fprintf(stderr, "systolic-cfront: cannot run %s --version: %s\n", clang,
            vrc < 0 ? strerror(errno) : verr);
    return 2;
  }
  if (nl)
    *nl = 0;
  const char *version = vout;
  const char *m = strstr(version, "clang version ");
  long major = -1;
  if (m) {
    char *end;
    major = strtol(m + 14, &end, 10);
    if (end == m + 14 || *end != '.')
      major = -1;
  }
  if (major != clang_major) {
    fprintf(stderr, "systolic-cfront: %s is '%s', not clang %ld; refusing to "
                    "use it (--clang-major selects another version "
                    "deliberately)\n", clang, version, clang_major);
    return 2;
  }

  strvec cmd = {0};
  strvec_push(&cmd, clang);
  strvec_push(&cmd, "-x");
  strvec_push(&cmd, "c");
  strvec_push(&cmd, format("-std=%s", std));
  strvec_push(&cmd, "-fsyntax-only");
  strvec_push(&cmd, "-Xclang");
  strvec_push(&cmd, "-ast-dump=json");
  for (size_t i = 0; i < defines.n; ++i)
    strvec_push(&cmd, format("-D%s", defines.items[i]));
  for (size_t i = 0; i < includes.n; ++i)
    strvec_push(&cmd, format("-I%s", includes.items[i]));
  strvec_push(&cmd, source);
  strvec_push(&cmd, NULL);
  char *ast_text, *clang_err;
  size_t ast_len;
  int rc = run_capture(cmd.items, &ast_text, &ast_len, &clang_err);
  if (rc < 0) {
    fprintf(stderr, "systolic-cfront: cannot run %s: %s\n", clang,
            strerror(errno));
    return 2;
  }
  if (rc != 0) {
    fputs(clang_err, stderr);
    fprintf(stderr, "systolic-cfront: clang rejected %s\n", source);
    return 2;
  }
  fputs(clang_err, stderr);   /* clang's warnings */
  char error[128];
  json_value *ast = json_parse(ast_text, ast_len, error);
  if (!ast) {
    fprintf(stderr, "systolic-cfront: cannot read clang's AST: %s\n", error);
    return 2;
  }

  size_t src_len;
  char *src_text = read_file(source, &src_len);
  if (!src_text) {
    fprintf(stderr, "systolic-cfront: cannot read %s\n", source);
    return 2;
  }
  source_map sm;
  source_map_init(&sm, src_text, src_len);
  const char *slash = strrchr(source, '/');
  const char *display = slash ? slash + 1 : source;
  char main_file[PATH_MAX];
  if (!realpath(source, main_file)) {
    fprintf(stderr, "systolic-cfront: cannot resolve %s\n", source);
    return 2;
  }

  /* Top-level declarations of the main file. The dumper names a file only
     when it changes from the previous node's, so track it in order. */
  char current_file[PATH_MAX] = "";
  /* The GEMM sites of every function defined here, translated or not. */
  /* open_memstream's buffer: not on the record of allocations, so closed and
     freed here on every way out of main below. */
  char *sites_buf = NULL;
  size_t sites_len = 0;
  FILE *sites_out = sites_path ? open_memstream(&sites_buf, &sites_len) : NULL;
  int sites_first = 1;
  const json_value *functions[4096];
  size_t nfunctions = 0;
  const json_value *decls = json_get(ast, "inner");
  for (size_t i = 0; i < json_size(decls); ++i) {
    const json_value *decl = json_at(decls, i);
    const json_value *loc = json_get(decl, "loc");
    const json_value *exp = json_get(loc, "expansionLoc");
    if (exp)
      loc = exp;
    const char *file = json_string(json_get(loc, "file"), NULL);
    if (file && !realpath(file, current_file))
      snprintf(current_file, sizeof current_file, "%s", file);
    if (json_bool(json_get(decl, "isImplicit"), 0) ||
        strcmp(current_file, main_file) != 0)
      continue;
    if (strcmp(kind_of(decl), "FunctionDecl") != 0)
      continue;
    int has_body = 0;
    for (size_t c = 0; c < child_count(decl); ++c)
      if (strcmp(kind_of(child(decl, c)), "CompoundStmt") == 0)
        has_body = 1;
    if (!has_body)
      continue;   /* a prototype */
    const char *name = json_string(json_get(decl, "name"), "");
    if (sites_out)
      write_sites(decl, &sm, sites_out, &sites_first);
    if (only.n && !strvec_contains(&only, name))
      continue;
    if (nfunctions == sizeof functions / sizeof functions[0]) {
      fprintf(stderr, "systolic-cfront: too many functions\n");
      if (sites_out) { fclose(sites_out); free(sites_buf); }
      return 2;
    }
    functions[nfunctions++] = decl;
  }

  /* --function names without a definition, sorted. */
  strvec missing = {0};
  for (size_t i = 0; i < only.n; ++i) {
    int found = 0;
    for (size_t f = 0; f < nfunctions; ++f)
      if (strcmp(json_string(json_get(functions[f], "name"), ""),
                 only.items[i]) == 0)
        found = 1;
    if (!found && !strvec_contains(&missing, only.items[i]))
      strvec_push(&missing, only.items[i]);
  }
  if (missing.n) {
    for (size_t a = 0; a < missing.n; ++a)
      for (size_t b = a + 1; b < missing.n; ++b)
        if (strcmp(missing.items[b], missing.items[a]) < 0) {
          const char *t = missing.items[a];
          missing.items[a] = missing.items[b];
          missing.items[b] = t;
        }
    fprintf(stderr, "systolic-cfront: no definition of ");
    for (size_t a = 0; a < missing.n; ++a)
      fprintf(stderr, "%s%s", a ? ", " : "", missing.items[a]);
    fprintf(stderr, " in %s\n", source);
    if (sites_out) { fclose(sites_out); free(sites_buf); }
    return 2;
  }

  /* Static storage: the translation of a function is abandoned with longjmp
     (unsupported()), and objects of automatic storage modified in between
     would be indeterminate afterwards. */
  static translator tr;
  static char *saved_interfaces;
  tr.display_name = display;
  tr.sm = &sm;
  tr.symbol_suffix = symbol_suffix;
  tr.fp_reassociation = fp_reassociation;
  tr.static_extents = static_extents;
  tr.modulo_subscripts = modulo_subscripts;
  tr.gather_subscripts = gather_subscripts;
  tr.scatter_subscripts = scatter_subscripts;
  emit(&tr, 0, "// Generated by systolic-cfront from %s.", display);
  char *defs = xstrdup("");
  for (size_t i = 0; i < defines.n; ++i) {
    char *d = format("%s -D%s", defs, defines.items[i]);
    free(defs);
    defs = d;
  }
  emit(&tr, 0, "// C front end: %s (-std=%s%s).", version, std, defs);
  emit(&tr, 0, "module {");
  for (size_t f = 0; f < nfunctions; ++f) {
    size_t mark = tr.lines.n;
    saved_interfaces = tr.interfaces ? xstrdup(tr.interfaces) : NULL;
    if (setjmp(unsupported_jump) == 0) {
      function(&tr, functions[f]);
      free(saved_interfaces);
      continue;
    }
    long long offset = node_offset(unsupported_node);
    long line = 0, col = 0;
    if (offset >= 0)
      source_map_line_col(&sm, offset, &line, &col);
    if (!skip_unsupported) {
      fprintf(stderr, "%s:%ld:%ld: error: %s\n", source, line, col,
              unsupported_message);
      if (sites_out) { fclose(sites_out); free(sites_buf); }
      return 1;
    }
    /* The function stays C; nothing of it is emitted. */
    tr.lines.n = mark;
    free(tr.interfaces);
    tr.interfaces = saved_interfaces;
    fprintf(stderr, "%s:%ld:%ld: note: '%s' is outside the supported C "
                    "subset and is not translated: %s\n", source, line, col,
            json_string(json_get(functions[f], "name"), ""),
            unsupported_message);
  }
  emit(&tr, 0, "}");

  if (sites_out) {
    fclose(sites_out);
    FILE *out = fopen(sites_path, "w");
    if (!out) {
      fprintf(stderr, "systolic-cfront: cannot write %s\n", sites_path);
      free(sites_buf);   /* open_memstream's: not on the record (above) */
      return 2;
    }
    fprintf(out, "{\"sites\": [\n%s\n]}\n", sites_buf ? sites_buf : "");
    fclose(out);
    free(sites_buf);
  }

  if (interface_path) {
    FILE *out = fopen(interface_path, "w");
    if (!out) {
      fprintf(stderr, "systolic-cfront: cannot write %s\n", interface_path);
      return 2;
    }
    fprintf(out, "{\"source\": ");
    json_escape(out, main_file);
    fprintf(out, ", \"clang\": ");
    json_escape(out, version);
    fprintf(out, ", \"functions\": [\n%s\n]}\n",
            tr.interfaces ? tr.interfaces : "");
    fclose(out);
  }

  FILE *out = strcmp(output, "-") == 0 ? stdout : fopen(output, "w");
  if (!out) {
    fprintf(stderr, "systolic-cfront: cannot write %s\n", output);
    return 2;
  }
  for (size_t i = 0; i < tr.lines.n; ++i) {
    fputs(tr.lines.items[i], out);
    fputc('\n', out);
  }
  if (out != stdout)
    fclose(out);
  return 0;
}
