#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    char const *start;
    char const *end;
} _z_str_se_t;

typedef struct {
    _z_str_se_t s;
    char const *delimiter;
} _z_splitstr_t;

/**
 * The reverse equivalent of libc's `strstr`.
 *
 * Returns NULL if the needle is not found.
 * If found, the return pointer will point to the end of the last occuring
 * needle within the haystack.
 */
char const *_z_rstrstr(const char *haystack_start, const char *haystack_end, const char *needle);

/**
 * A non-null-terminated haystack equivalent of libc's `strstr`.
 *
 * Returns NULL if the needle is not found.
 * If found, the return pointer will point to the start of the first occurence
 * of the needle within the haystack.
 */
char const *_z_strstr(char const *haystack_start, char const *haystack_end, const char *needle_start);

char const *_z_strstr_skipneedle(char const *haystack_start, char const *haystack_end, const char *needle_start);
char const *_z_bstrstr_skipneedle(_z_str_se_t haystack, _z_str_se_t needle);

_z_str_se_t _z_splitstr_next(_z_splitstr_t *str);
_z_str_se_t _z_splitstr_nextback(_z_splitstr_t *str);

size_t _z_strcnt(char const *haystack_start, const char *harstack_end, const char *needle_start);

size_t _z_str_startswith(const char *s, const char *needle);
