/*
 * systolic-cfront -- C to MLIR (func / affine / arith / memref) for the
 * affine subset of C the systolic-array compiler takes.
 *
 *   systolic-cfront --clang CLANG [--clang-major N] [-DNAME[=VAL]] [-IDIR]
 *                   [-std=STD] [--function NAME] [--skip-unsupported]
 *                   [--symbol-suffix S] [--interface FILE.json]
 *                   [--fp-reassociation] FILE.c [-o OUT.mlir]
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
 *                Counting down: for (int i = LB; i > B; --i) -- also >=,
 *                i--, i -= S (S > 0), i = i - S. affine.for only counts up,
 *                so it becomes the ordinal loop affine.for %i = 0 to N (N the
 *                trip count) and every use of i the affine expression
 *                LB - S * %i: the values LB, LB - S, ... in the same order.
 *   subscripts   every array access subscripts all dimensions; each subscript
 *                is an affine expression of the enclosing loop counters and
 *                integer constants (+, -, constant *).
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
  char *type;                  /* memref<?x...> */
  int inner_dims[MAX_RANK];
  /* ENV_SCALAR */
  /* (elem above) */
  /* ENV_IV: the counter's first and last values, unless it runs zero times */
  int has_values;
  long long first, last;
  /* ENV_IV of a loop that counts down: `name` is the ordinal t = 0 .. N-1 of
     the emitted affine.for, and the counter's value is origin - stride * t */
  int descending;
  long long origin, stride;
} env_entry;

typedef struct {
  const char *decl;
  int read, written;
  int has_ranges;
  int rank;
  long long lo[MAX_RANK], hi[MAX_RANK];
} access_info;

typedef struct {
  const char *display_name;
  const source_map *sm;
  strvec lines;
  strvec ssa_names;
  long counter;
  const char *symbol_suffix;
  int fp_reassociation;        /* --fp-reassociation: fastmath<reassoc> */

  env_entry *env;
  size_t env_n, env_cap;
  access_info *accesses;
  size_t acc_n, acc_cap;

  char *interfaces;            /* the JSON objects, comma separated */
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

/* An affine subscript: the text usable inside affine.load brackets and its
   linear form. */
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
    unsupported(node, "subscript uses '%s', which is not an enclosing loop "
                      "counter", referenced_name(node));
  }
  if (strcmp(kind, "UnaryOperator") == 0 && strcmp(op, "-") == 0) {
    linear_form f;
    char *t = affine_index(tr, child(node, 0), &f);
    *form = form_scale(f, -1, node);
    return format("-(%s)", t);
  }
  if (strcmp(kind, "BinaryOperator") == 0 &&
      (strcmp(op, "+") == 0 || strcmp(op, "-") == 0)) {
    linear_form fa, fb;
    char *ta = affine_index(tr, child(node, 0), &fa);
    char *tb = affine_index(tr, child(node, 1), &fb);
    *form = form_add(fa, form_scale(fb, op[0] == '+' ? 1 : -1, node), node);
    return format("(%s %s %s)", ta, op, tb);
  }
  if (strcmp(kind, "BinaryOperator") == 0 && strcmp(op, "*") == 0) {
    for (int first = 1; first >= 0; --first) {
      const json_value *cnode = child(node, first ? 0 : 1);
      const json_value *other = child(node, first ? 1 : 0);
      long long c;
      if (!try_int_constant(cnode, &c))
        continue;
      linear_form f;
      char *t = affine_index(tr, other, &f);
      *form = form_scale(f, c, node);
      return first ? format("(%lld * %s)", c, t) : format("(%s * %lld)", t, c);
    }
    unsupported(node, "subscript multiplies two non-constant terms; not an "
                      "affine expression");
  }
  unsupported(node, "subscript is not an affine expression of the loop "
                    "counters");
}

/* Exact (min, max) of a linear form over the enclosing loops' box; 0 if some
   enclosing loop runs zero times (the access never executes). */
static int form_range(translator *tr, const linear_form *f, long long *lo,
                      long long *hi, const json_value *node) {
  long long l = f->constant, h = f->constant;
  for (size_t i = 0; i < f->n; ++i) {
    env_entry *e = env_find(tr, f->ids[i]);
    if (!e->has_values)
      return 0;
    long long a = checked(f->coef[i], e->first, '*', node);
    long long b = checked(f->coef[i], e->last, '*', node);
    l = checked(l, a < b ? a : b, '+', node);
    h = checked(h, a < b ? b : a, '+', node);
  }
  *lo = l;
  *hi = h;
  return 1;
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

  char *texts[MAX_RANK];
  linear_form forms[MAX_RANK];
  for (size_t d = 0; d < count; ++d)   /* outermost subscript first */
    texts[d] = affine_index(tr, indices[count - 1 - d], &forms[d]);

  access_info *info = access_of(tr, e->decl, e->rank);
  info->read |= read;
  info->written |= written;
  long long lo[MAX_RANK], hi[MAX_RANK];
  int all = 1;
  for (size_t d = 0; d < count; ++d)
    if (!form_range(tr, &forms[d], &lo[d], &hi[d], node))
      all = 0;
  if (all) {
    for (size_t d = 0; d < count; ++d) {
      if (!info->has_ranges || lo[d] < info->lo[d])
        info->lo[d] = lo[d];
      if (!info->has_ranges || hi[d] > info->hi[d])
        info->hi[d] = hi[d];
    }
    info->has_ranges = 1;
  }

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

static char *load(translator *tr, const json_value *node, int indent,
                  const char **elem) {
  char *idx;
  env_entry *e = array_access(tr, node, 1, 0, &idx);
  char *name = fresh(tr);
  emit(tr, indent, "%s = affine.load %s[%s] : %s %s", name, e->name, idx,
       e->type, loc_text(tr, node));
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

/* The value of that expression at counter value `i`, every operation checked
   against the range of the C type clang gives it (the condition is evaluated
   in C, where a signed overflow is undefined behaviour). 0 if one does not
   fit, or a type is not a signed integer type. */
static int counter_eval(const json_value *n, const char *decl, __int128 i,
                        __int128 *out) {
  const json_value *s = strip_parens_and_casts(n, kIntConstantCasts);
  const char *kind = kind_of(s);
  const char *op = opcode_of(s);
  long long v;
  __int128 x, y, r;
  if (is_ref_to(s, decl)) {
    r = i;
  } else if (strcmp(kind, "DeclRefExpr") != 0 && try_int_constant(s, &v)) {
    r = v;
  } else if (strcmp(kind, "UnaryOperator") == 0) {
    if (!counter_eval(child(s, 0), decl, i, &x))
      return 0;
    r = op[0] == '-' ? -x : x;
  } else if (strcmp(kind, "BinaryOperator") == 0) {
    if (!counter_eval(child(s, 0), decl, i, &x) ||
        !counter_eval(child(s, 1), decl, i, &y))
      return 0;
    if (op[0] == '*') {
      __int128 lim = (__int128)1 << 64;
      if (x > lim || x < -lim || y > lim || y < -lim)
        return 0;
      r = x * y;
    } else {
      r = op[0] == '+' ? x + y : x - y;
    }
  } else {
    return 0;
  }
  /* the range of this operation's own C type */
  char *t = strip_qualifiers(qual_type(s));
  unsigned long long max;
  int ok = int_type_max(t, &max) && strncmp(t, "unsigned", 8) != 0 &&
           r <= (__int128)max && r >= -(__int128)max - 1;
  free(t);
  *out = r;
  return ok;
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
    emit(tr, indent, "%s = affine.load %s[%s] : %s %s", old, e->name, idx,
         e->type, loc_text(tr, lhs_node));
    old = convert(tr, old, elem, compute, s, indent);
    const char *r_elem;
    char *r = value(tr, rhs, indent, &r_elem);
    r = convert(tr, r, r_elem, compute, s, indent);
    v = arith(tr, opcode[0], old, r, compute, s, indent);
    v = convert(tr, v, compute, elem, s, indent);
  }
  emit(tr, indent, "affine.store %s, %s[%s] : %s %s", v, e->name, idx,
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
  long long lb = int_constant(child(var, 0));

  const char *cop = opcode_of(cond);
  int has_cond = cond && cond->kind == JSON_OBJECT && cond->count;
  int up = strcmp(cop, "<") == 0 || strcmp(cop, "<=") == 0;
  int down = strcmp(cop, ">") == 0 || strcmp(cop, ">=") == 0;
  long long bound;
  /* An affine condition E OP C (2 * i - 3 > 0): the equivalent comparison of
     the counter with a constant; E itself is checked for overflow below,
     once the values it is evaluated at are known. */
  int affine_cond = 0;
  if (strcmp(kind_of(cond), "BinaryOperator") == 0 && (up || down) &&
      !is_ref_to(child(cond, 0), decl) &&
      affine_condition(cond, decl, &cop, &bound)) {
    affine_cond = 1;
    up = cop[0] == '<';
    down = !up;
  }
  if (!affine_cond) {
    if (strcmp(kind_of(cond), "BinaryOperator") != 0 || !(up || down) ||
        !is_ref_to(child(cond, 0), decl))
      unsupported(has_cond ? cond : s, "the loop condition must be 'counter < "
                                       "bound', 'counter <= bound', 'counter > "
                                       "bound' or 'counter >= bound'");
    bound = int_constant(child(cond, 1));
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

  /* The values the counter takes, first, first + step, ... while the
     condition holds: `trips` of them (exact integer arithmetic). */
  int is_unsigned = strncmp(vtype, "unsigned", 8) == 0;
  __int128 type_min = is_unsigned ? 0 : -(__int128)type_max - 1;
  long long ub = 0;   /* counting up: the exclusive upper bound */
  __int128 trips;
  if (up) {
    ub = bound;
    if (strcmp(cop, "<=") == 0)
      ub = checked(ub, 1, '+', cond);
    trips = ub > lb ? ((__int128)ub - 1 - lb) / step + 1 : 0;
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
    /* The decrement after the last iteration must not leave the type: for
       an unsigned counter and the condition i >= 0 it would wrap around and
       the loop would never end. */
    __int128 exit_value = (__int128)lb - down_step * trips;
    if (exit_value < type_min)
      unsupported(s, "the loop counter could overflow '%s'", vtype);
    if (trips > (__int128)9223372036854775807LL)
      unsupported(s, "the loop runs more than 2^63 - 1 times");
  }

  /* An affine condition is evaluated before every iteration and once more
     when the loop ends: at the counter values lb, lb + step, ..., lb + step *
     trips. Every operation of it is linear in the counter, so its extremes
     are at the first and the last of those values; it must not overflow its
     C type at either. */
  if (affine_cond) {
    __int128 v;
    __int128 last_eval = (__int128)lb + (__int128)step * trips;
    if (!counter_eval(child(cond, 0), decl, lb, &v) ||
        !counter_eval(child(cond, 0), decl, last_eval, &v))
      unsupported(cond, "the loop condition could overflow (it is evaluated "
                        "for the counter values %lld .. %lld)", lb,
                  (long long)last_eval);
  }

  if (writes_to(body, decl))
    unsupported(body, "the loop counter is modified in the loop body");

  char *name = unique(tr, json_string(json_get(var, "name"), "iv"));
  env_entry *e = env_add(tr, decl, ENV_IV);
  e->name = name;
  /* The counter's first and last values; none for a loop that runs zero
     times. */
  if (trips > 0) {
    e->has_values = 1;
    e->first = lb;
    e->last = (long long)((__int128)lb + (__int128)step * (trips - 1));
  }
  if (down) {
    /* affine.for counts up: the ordinal t = 0 .. trips-1, the counter
       lb - |step| * t (see affine_index). */
    e->descending = 1;
    e->origin = lb;
    e->stride = -step;
    emit(tr, indent, "affine.for %s = 0 to %lld {", name, (long long)trips);
  } else if (step == 1)
    emit(tr, indent, "affine.for %s = %lld to %lld {", name, lb, ub);
  else
    emit(tr, indent, "affine.for %s = %lld to %lld step %lld {", name, lb, ub,
         step);
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
      /* Accepted as a parameter, so that a use of it is reported where it is
         used (a symbolic bound, a subscript) -- every use is outside the
         subset. */
      char *name = unique(tr, pname);
      env_entry *e = env_add(tr, json_string(json_get(p, "id"), ""),
                             ENV_INTSCALAR);
      e->name = name;
      return format("%s: i%d %s", name, strstr(t, "long") ? 64 : 32,
                    loc_text(tr, p));
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
  return format("%s: %s%s %s", name, type, noalias ? " {llvm.noalias}" : "",
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
      fprintf(out, ", \"elem\": \"%s\", \"leading_extent\": %lld, "
                   "\"leading_min\": %lld, \"inner_dims\": [",
              e->elem, ranges ? info->hi[0] + 1 : 0,
              ranges ? info->lo[0] : 0);
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
  char *args = xstrdup("");
  for (size_t i = 0; i < nparams; ++i) {
    if (!json_get(params[i], "name"))
      unsupported(params[i], "unnamed parameters are not supported");
    char *a = parameter(tr, params[i]);
    char *joined = format("%s%s%s", args, i ? ", " : "", a);
    free(args);
    args = joined;
  }
  emit(tr, 1, "func.func @%s%s(%s) {", json_string(json_get(fn, "name"), ""),
       tr->symbol_suffix, args);
  statements(tr, json_get(body, "inner"), 2, 1);
  emit(tr, 2, "func.return %s", loc_text(tr, body));
  emit(tr, 1, "} %s", loc_text(tr, fn));
  interface(tr, fn, params, nparams, body);
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
             !try_int_constant(child(cond, 1), &bound)) {
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
  if (depth >= 3 && mac_parts(n, &lhs, &product)) {
    strvec in_lhs = {0}, in_product = {0};
    collect_decl_ids(lhs, &in_lhs);
    collect_decl_ids(product, &in_product);
    size_t used = 0, top = 0;
    int reduction = 0;
    for (size_t d = 0; d < depth; ++d) {
      int uses = 0, reduces = 0;
      for (size_t c = 0; c < stack[d].ids.n; ++c) {
        const char *id = stack[d].ids.items[c];
        int l = strvec_contains(&in_lhs, id), p = strvec_contains(&in_product, id);
        uses |= l || p;
        reduces |= p && !l;
      }
      if (uses && used++ == 0)
        top = d;
      reduction |= reduces;
    }
    if (used >= 3 && reduction) {
      long long executions = 1;
      for (size_t d = 0; d < top; ++d)
        executions = executions < 0 || stack[d].trips < 0
                         ? -1 : executions * stack[d].trips;
      int seen = 0;
      for (size_t s = 0; s < sites->n; ++s)
        if (sites->loops[s] == stack[top].loop)
          seen = 1;
      if (!seen) {
        if (sites->n == sites->cap) {
          sites->cap = sites->cap ? 2 * sites->cap : 8;
          sites->loops = realloc(sites->loops, sites->cap * sizeof *sites->loops);
          sites->executions = realloc(sites->executions,
                                      sites->cap * sizeof *sites->executions);
          if (!sites->loops || !sites->executions) {
            fprintf(stderr, "systolic-cfront: out of memory\n");
            exit(2);
          }
        }
        sites->executions[sites->n] = executions;
        sites->loops[sites->n++] = stack[top].loop;
      }
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
                 "\"end\": %lld, \"from_macro\": %s, \"executions\": %lld}",
            line, col, begin, end, from_macro ? "true" : "false",
            sites.executions[s]);
    *first = 0;
  }
  free(sites.loops);
  free(sites.executions);
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
          "[--gemm-sites FILE] [--fp-reassociation] "
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
      return 2;
    }
    fprintf(out, "{\"sites\": [\n%s\n]}\n", sites_buf ? sites_buf : "");
    fclose(out);
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
