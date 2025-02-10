#ifndef JANSSON_PRIVATE_H
#define JANSSON_PRIVATE_H

#include "hashtable.h"
#include "jansson.h"
// #include "jansson_private_config.h"
#include "strbuffer.h"
#include <stddef.h>

#define container_of(ptr_, type_, member_)                                               \
    ((type_ *)((char *)ptr_ - offsetof(type_, member_)))

/* On some platforms, max() may already be defined */
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

typedef struct {
    json_t json;
    hashtable_t hashtable;
} json_object_t;

typedef struct {
    json_t json;
    size_t size;
    size_t entries;
    json_t **table;
} json_array_t;

typedef struct {
    json_t json;
    char *value;
    size_t length;
} json_string_t;

typedef struct {
    json_t json;
    double value;
} json_real_t;

typedef struct {
    json_t json;
    long long value;
} json_integer_t;

#define json_to_object(json_)  container_of(json_, json_object_t, json)
#define json_to_array(json_)   container_of(json_, json_array_t, json)
#define json_to_string(json_)  container_of(json_, json_string_t, json)
#define json_to_real(json_)    container_of(json_, json_real_t, json)
#define json_to_integer(json_) container_of(json_, json_integer_t, json)

/* Create a string by taking ownership of an existing buffer */
json_t *jsonp_stringn_nocheck_own(const char *value, size_t len);

/* Error message formatting */
void jsonp_error_init(json_error_t *error, const char *source);
void jsonp_error_set_source(json_error_t *error, const char *source);
void jsonp_error_set(json_error_t *error, int line, int column, size_t position,
                     enum json_error_code code, const char *msg, ...);
void jsonp_error_vset(json_error_t *error, int line, int column, size_t position,
                      enum json_error_code code, const char *msg, va_list ap);

/* Locale independent string<->double conversions */
int jsonp_strtod(strbuffer_t *strbuffer, double *out);
int jsonp_dtostr(char *buffer, size_t size, double value, int prec);

/* Wrappers for custom memory functions */
void *jsonp_malloc(size_t size) JANSSON_ATTRS((warn_unused_result));
void jsonp_free(void *ptr);
char *jsonp_strndup(const char *str, size_t length) JANSSON_ATTRS((warn_unused_result));
char *jsonp_strdup(const char *str) JANSSON_ATTRS((warn_unused_result));
char *jsonp_strndup(const char *str, size_t len) JANSSON_ATTRS((warn_unused_result));

/* Circular reference check*/
/* Space for "0x", double the sizeof a pointer for the hex and a terminator. */
#define LOOP_KEY_LEN (2 + (sizeof(json_t *) * 2) + 1)
int jsonp_loop_check(hashtable_t *parents, const json_t *json, char *key, size_t key_size,
                     size_t *key_len_out);

#endif
