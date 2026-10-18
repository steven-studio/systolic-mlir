/*
 * json.h -- a small JSON reader for systolic-cfront and systolic-cc.
 *
 * Reads a whole document into a tree (clang's -ast-dump=json of a C file,
 * tens of MB with system headers, or the front end's interface file). Values
 * live in one arena and are never freed individually: the tools read a
 * document, use it, and exit.
 */

#ifndef SYSTOLIC_JSON_H
#define SYSTOLIC_JSON_H

#include <stddef.h>

typedef enum { JSON_NULL, JSON_BOOL, JSON_NUMBER, JSON_STRING, JSON_ARRAY,
               JSON_OBJECT } json_kind;

typedef struct json_value json_value;
struct json_value {
  json_kind kind;
  int boolean;
  const char *text;            /* JSON_STRING: the decoded string (UTF-8);
                                  JSON_NUMBER: the number as written */
  size_t length;               /* JSON_STRING: bytes in text */
  size_t count;                /* JSON_ARRAY / JSON_OBJECT: entries */
  json_value **items;          /* array elements / object values */
  const char **keys;           /* JSON_OBJECT: keys */
};

/* Parses text[0 .. length). NULL on a syntax error, with the reason in
   error (at least 128 bytes). */
json_value *json_parse(const char *text, size_t length, char *error);

/* The member `key` of an object, or NULL (also when v is not an object). */
const json_value *json_get(const json_value *v, const char *key);

/* Convenience accessors; each returns `fallback` when v is NULL or of
   another kind. */
const char *json_string(const json_value *v, const char *fallback);
long long json_int(const json_value *v, long long fallback);
int json_bool(const json_value *v, int fallback);
size_t json_size(const json_value *v);   /* entries of an array, else 0 */
const json_value *json_at(const json_value *v, size_t i);

#endif
