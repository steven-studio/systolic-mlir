/*
 * json.c -- see json.h.
 */

#include "json.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ arena */

typedef struct arena_block {
  struct arena_block *next;
  size_t used, size;
  char data[];
} arena_block;

static arena_block *arena;

static void *arena_alloc(size_t n) {
  n = (n + 15) & ~(size_t)15;
  if (!arena || arena->used + n > arena->size) {
    size_t size = n > (1u << 20) ? n : (1u << 20);
    arena_block *b = malloc(sizeof *b + size);
    if (!b) {
      fprintf(stderr, "json: out of memory\n");
      exit(2);
    }
    b->next = arena;
    b->used = 0;
    b->size = size;
    arena = b;
  }
  void *p = arena->data + arena->used;
  arena->used += n;
  return p;
}

/* ------------------------------------------------------------------ parser */

typedef struct {
  const char *begin, *p, *end;
  char *error;
} parser;

static int fail(parser *ps, const char *what) {
  if (!ps->error[0])
    snprintf(ps->error, 128, "%s at byte %ld", what, (long)(ps->p - ps->begin));
  return 0;
}

static void skip_ws(parser *ps) {
  while (ps->p < ps->end &&
         (*ps->p == ' ' || *ps->p == '\t' || *ps->p == '\n' || *ps->p == '\r'))
    ++ps->p;
}

static json_value *new_value(json_kind kind) {
  json_value *v = arena_alloc(sizeof *v);
  memset(v, 0, sizeof *v);
  v->kind = kind;
  return v;
}

static int hex4(const char *s, unsigned *out) {
  unsigned v = 0;
  for (int i = 0; i < 4; ++i) {
    char c = s[i];
    v <<= 4;
    if (c >= '0' && c <= '9')
      v |= (unsigned)(c - '0');
    else if (c >= 'a' && c <= 'f')
      v |= (unsigned)(c - 'a' + 10);
    else if (c >= 'A' && c <= 'F')
      v |= (unsigned)(c - 'A' + 10);
    else
      return 0;
  }
  *out = v;
  return 1;
}

static size_t put_utf8(char *out, unsigned cp) {
  if (cp < 0x80) {
    out[0] = (char)cp;
    return 1;
  }
  if (cp < 0x800) {
    out[0] = (char)(0xC0 | (cp >> 6));
    out[1] = (char)(0x80 | (cp & 0x3F));
    return 2;
  }
  if (cp < 0x10000) {
    out[0] = (char)(0xE0 | (cp >> 12));
    out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[2] = (char)(0x80 | (cp & 0x3F));
    return 3;
  }
  out[0] = (char)(0xF0 | (cp >> 18));
  out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
  out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
  out[3] = (char)(0x80 | (cp & 0x3F));
  return 4;
}

/* A string token; ps->p at the opening quote. */
static const char *parse_string(parser *ps, size_t *length) {
  const char *start = ++ps->p;
  int plain = 1;
  while (ps->p < ps->end && *ps->p != '"') {
    if (*ps->p == '\\') {
      plain = 0;
      ++ps->p;
    }
    ++ps->p;
  }
  if (ps->p >= ps->end) {
    fail(ps, "unterminated string");
    return NULL;
  }
  size_t raw = (size_t)(ps->p - start);
  char *out = arena_alloc(raw + 1);
  if (plain) {
    memcpy(out, start, raw);
    out[raw] = 0;
    *length = raw;
    ++ps->p;
    return out;
  }
  size_t n = 0;
  for (const char *s = start; s < ps->p; ++s) {
    if (*s != '\\') {
      out[n++] = *s;
      continue;
    }
    ++s;
    switch (*s) {
    case '"': out[n++] = '"'; break;
    case '\\': out[n++] = '\\'; break;
    case '/': out[n++] = '/'; break;
    case 'b': out[n++] = '\b'; break;
    case 'f': out[n++] = '\f'; break;
    case 'n': out[n++] = '\n'; break;
    case 'r': out[n++] = '\r'; break;
    case 't': out[n++] = '\t'; break;
    case 'u': {
      unsigned cp;
      if (s + 4 >= ps->p || !hex4(s + 1, &cp)) {
        fail(ps, "bad \\u escape");
        return NULL;
      }
      s += 4;
      if (cp >= 0xD800 && cp < 0xDC00 && s + 6 < ps->p && s[1] == '\\' &&
          s[2] == 'u') {
        unsigned lo;
        if (hex4(s + 3, &lo) && lo >= 0xDC00 && lo < 0xE000) {
          cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00);
          s += 6;
        }
      }
      /* \u escapes never grow past their own six bytes. */
      n += put_utf8(out + n, cp);
      break;
    }
    default:
      fail(ps, "bad escape");
      return NULL;
    }
  }
  out[n] = 0;
  *length = n;
  ++ps->p;
  return out;
}

/* Growable arrays of pointers, copied into the arena when complete. */
typedef struct {
  void **items;
  size_t n, cap;
} ptr_vec;

static void vec_push(ptr_vec *v, void *p) {
  if (v->n == v->cap) {
    v->cap = v->cap ? 2 * v->cap : 8;
    v->items = realloc(v->items, v->cap * sizeof *v->items);
    if (!v->items) {
      fprintf(stderr, "json: out of memory\n");
      exit(2);
    }
  }
  v->items[v->n++] = p;
}

static void **vec_finish(ptr_vec *v) {
  void **out = arena_alloc((v->n ? v->n : 1) * sizeof *out);
  if (v->n)
    memcpy(out, v->items, v->n * sizeof *out);
  free(v->items);
  return out;
}

static json_value *parse_value(parser *ps, int depth);

static json_value *parse_array(parser *ps, int depth) {
  ++ps->p;
  ptr_vec items = {0};
  skip_ws(ps);
  if (ps->p < ps->end && *ps->p == ']') {
    ++ps->p;
  } else {
    for (;;) {
      json_value *item = parse_value(ps, depth + 1);
      if (!item) {
        free(items.items);
        return NULL;
      }
      vec_push(&items, item);
      skip_ws(ps);
      if (ps->p < ps->end && *ps->p == ',') {
        ++ps->p;
        continue;
      }
      if (ps->p < ps->end && *ps->p == ']') {
        ++ps->p;
        break;
      }
      free(items.items);
      fail(ps, "expected ',' or ']'");
      return NULL;
    }
  }
  json_value *v = new_value(JSON_ARRAY);
  v->count = items.n;
  v->items = (json_value **)vec_finish(&items);
  return v;
}

static json_value *parse_object(parser *ps, int depth) {
  ++ps->p;
  ptr_vec keys = {0}, vals = {0};
  skip_ws(ps);
  if (ps->p < ps->end && *ps->p == '}') {
    ++ps->p;
  } else {
    for (;;) {
      skip_ws(ps);
      size_t len;
      const char *key = (ps->p < ps->end && *ps->p == '"')
                            ? parse_string(ps, &len) : NULL;
      if (!key)
        goto bad;
      skip_ws(ps);
      if (ps->p >= ps->end || *ps->p != ':') {
        fail(ps, "expected ':'");
        goto bad;
      }
      ++ps->p;
      json_value *val = parse_value(ps, depth + 1);
      if (!val)
        goto bad;
      void *key_slot;
      memcpy(&key_slot, &key, sizeof key_slot);
      vec_push(&keys, key_slot);
      vec_push(&vals, val);
      skip_ws(ps);
      if (ps->p < ps->end && *ps->p == ',') {
        ++ps->p;
        continue;
      }
      if (ps->p < ps->end && *ps->p == '}') {
        ++ps->p;
        break;
      }
      fail(ps, "expected ',' or '}'");
      goto bad;
    }
  }
  json_value *v = new_value(JSON_OBJECT);
  v->count = vals.n;
  v->keys = (const char **)vec_finish(&keys);
  v->items = (json_value **)vec_finish(&vals);
  return v;
bad:
  free(keys.items);
  free(vals.items);
  if (!ps->error[0])
    fail(ps, "bad object");
  return NULL;
}

static json_value *parse_value(parser *ps, int depth) {
  if (depth > 10000) {
    fail(ps, "nesting too deep");
    return NULL;
  }
  skip_ws(ps);
  if (ps->p >= ps->end) {
    fail(ps, "unexpected end");
    return NULL;
  }
  char c = *ps->p;
  if (c == '{')
    return parse_object(ps, depth);
  if (c == '[')
    return parse_array(ps, depth);
  if (c == '"') {
    json_value *v = new_value(JSON_STRING);
    v->text = parse_string(ps, &v->length);
    return v->text ? v : NULL;
  }
  if (c == 't' && ps->end - ps->p >= 4 && !memcmp(ps->p, "true", 4)) {
    ps->p += 4;
    json_value *v = new_value(JSON_BOOL);
    v->boolean = 1;
    return v;
  }
  if (c == 'f' && ps->end - ps->p >= 5 && !memcmp(ps->p, "false", 5)) {
    ps->p += 5;
    return new_value(JSON_BOOL);
  }
  if (c == 'n' && ps->end - ps->p >= 4 && !memcmp(ps->p, "null", 4)) {
    ps->p += 4;
    return new_value(JSON_NULL);
  }
  if (c == '-' || (c >= '0' && c <= '9')) {
    const char *start = ps->p++;
    while (ps->p < ps->end &&
           strchr("0123456789+-.eE", *ps->p) != NULL)
      ++ps->p;
    size_t n = (size_t)(ps->p - start);
    char *text = arena_alloc(n + 1);
    memcpy(text, start, n);
    text[n] = 0;
    json_value *v = new_value(JSON_NUMBER);
    v->text = text;
    v->length = n;
    return v;
  }
  fail(ps, "unexpected character");
  return NULL;
}

json_value *json_parse(const char *text, size_t length, char *error) {
  parser ps = {text, text, text + length, error};
  error[0] = 0;
  json_value *v = parse_value(&ps, 0);
  if (!v)
    return NULL;
  skip_ws(&ps);
  if (ps.p != ps.end) {
    fail(&ps, "trailing data");
    return NULL;
  }
  return v;
}

/* --------------------------------------------------------------- accessors */

const json_value *json_get(const json_value *v, const char *key) {
  if (!v || v->kind != JSON_OBJECT)
    return NULL;
  for (size_t i = 0; i < v->count; ++i)
    if (strcmp(v->keys[i], key) == 0)
      return v->items[i];
  return NULL;
}

const char *json_string(const json_value *v, const char *fallback) {
  return v && v->kind == JSON_STRING ? v->text : fallback;
}

long long json_int(const json_value *v, long long fallback) {
  if (!v || v->kind != JSON_NUMBER)
    return fallback;
  return strtoll(v->text, NULL, 10);
}

int json_bool(const json_value *v, int fallback) {
  return v && v->kind == JSON_BOOL ? v->boolean : fallback;
}

size_t json_size(const json_value *v) {
  return v && v->kind == JSON_ARRAY ? v->count : 0;
}

const json_value *json_at(const json_value *v, size_t i) {
  return v && v->kind == JSON_ARRAY && i < v->count ? v->items[i] : NULL;
}
