#include "zenoh-pico/utils/string.h"

#include <stdbool.h>
#include <string.h>

#include "zenoh-pico/utils/pointers.h"

_z_str_t _z_bstrnew(const char *start) { return (_z_str_t){.start = start, .end = strchr(start, 0)}; }

char const *_z_rstrstr(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    char const *hs = haystack_start;
    char const *he = haystack_end;

    const char *needle_end = strchr(needle_start, 0);
    hs = _z_cptr_char_offset(hs, _z_ptr_char_diff(needle_end, needle_start));
    char const *result = NULL;
    while ((result == false) && (he >= hs)) {
        char found = 1;
        char const *needle = _z_cptr_char_offset(needle_end, -1);
        char const *haystack = _z_cptr_char_offset(he, -1);
        while (needle >= needle_start) {
            if (needle[0] != haystack[0]) {
                found = 0;
                break;
            }

            needle = _z_cptr_char_offset(needle, -1);
            haystack = _z_cptr_char_offset(haystack, -1);
        }
        if (found == true) {
            result = he;
        }
        he = _z_cptr_char_offset(he, -1);
    }
    return result;
}

char const *_z_bstrstr(_z_str_t haystack, _z_str_t needle) {
    haystack.end = _z_cptr_char_offset(haystack.end, -_z_ptr_char_diff(needle.end, needle.start));
    char const *result = NULL;
    for (; (result == false) && (haystack.start <= haystack.end);
         haystack.start = _z_cptr_char_offset(haystack.start, 1)) {
        _Bool found = true;
        char const *n = needle.start;
        char const *h = haystack.start;
        while (_z_ptr_char_diff(needle.end, n) > 0) {
            if (n[0] != h[0]) {
                found = false;
                break;
            }

            n = _z_cptr_char_offset(n, 1);
            h = _z_cptr_char_offset(h, 1);
        }
        if (found == true) {
            result = haystack.start;
        }
    }
    return result;
}
char const *_z_strstr(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    const char *needle_end = strchr(needle_start, 0);
    return _z_bstrstr((_z_str_t){.start = haystack_start, .end = haystack_end},
                      (_z_str_t){.start = needle_start, .end = needle_end});
}
char const *_z_strstr_skipneedle(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    const char *needle_end = strchr(needle_start, 0);
    return _z_bstrstr_skipneedle((_z_str_t){.start = haystack_start, .end = haystack_end},
                                 (_z_str_t){.start = needle_start, .end = needle_end});
}

char const *_z_bstrstr_skipneedle(_z_str_t haystack, _z_str_t needle) {
    char const *result = _z_bstrstr(haystack, needle);
    if (result != NULL) {
        result = _z_cptr_char_offset(result, _z_ptr_char_diff(needle.end, needle.start));
    }
    return result;
}

_z_str_t _z_splitstr_next(_z_splitstr_t *str) {
    _z_str_t result = str->s;
    if (str->s.start != NULL) {
        result.end = _z_strstr(result.start, result.end, str->delimiter);
        if (result.end == NULL) {
            result.end = str->s.end;
        }
        if (result.end == str->s.end) {
            str->s = (_z_str_t){.start = NULL, .end = NULL};
        } else {
            str->s.start = _z_cptr_char_offset(result.end, strlen(str->delimiter));
        }
    }
    return result;
}

_z_str_t _z_splitstr_nextback(_z_splitstr_t *str) {
    _z_str_t result = str->s;
    if (str->s.start != NULL) {
        result.start = _z_rstrstr(result.start, result.end, str->delimiter);
        if (result.start == NULL) {
            result.start = str->s.start;
        }
        if (result.start == str->s.start) {
            str->s = (_z_str_t){.start = NULL, .end = NULL};
        } else {
            str->s.end = _z_cptr_char_offset(result.start, -strlen(str->delimiter));
        }
    }
    return result;
}

size_t _z_strcnt(char const *haystack_start, const char *harstack_end, const char *needle_start) {
    char const *hs = haystack_start;

    size_t result = 0;
    while (hs != NULL) {
        result = result + (size_t)1;
        hs = _z_strstr_skipneedle(hs, harstack_end, needle_start);
    }
    return result;
}

size_t _z_str_startswith(const char *s, const char *needle) {
    size_t i = 0;
    for (; needle[i] != '\0'; i++) {
        if (s[i] != needle[i]) {
            i = 0;
            break;
        }
    }
    return i;
}