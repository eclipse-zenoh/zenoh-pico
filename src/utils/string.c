#include "zenoh-pico/utils/string.h"

#include <stdbool.h>
#include <string.h>

_z_borrowed_str_t _z_bstrnew(const char *start) { return (_z_borrowed_str_t){.start = start, .end = strchr(start, 0)}; }

char const *_z_rstrstr(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    const char *needle_end = strchr(needle_start, 0);
    haystack_start += needle_end - needle_start;
    char const *result = NULL;
    for (; !result && haystack_end >= haystack_start; haystack_end--) {
        char found = 1;
        char const *needle = needle_end - 1;
        char const *haystack = haystack_end - 1;
        for (; needle >= needle_start; needle--, haystack--) {
            if (*needle != *haystack) {
                found = 0;
                break;
            }
        }
        if (found) {
            result = haystack_end;
        }
    }
    return result;
}

char const *_z_bstrstr(_z_borrowed_str_t haystack, _z_borrowed_str_t needle) {
    haystack.end -= needle.end - needle.start;
    char const *result = NULL;
    for (; !result && haystack.start <= haystack.end; haystack.start++) {
        bool found = true;
        char const *n = needle.start;
        char const *h = haystack.start;
        for (; n < needle.end; n++, h++) {
            if (*n != *h) {
                found = false;
                break;
            }
        }
        if (found) {
            result = haystack.start;
        }
    }
    return result;
}
char const *_z_strstr(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    const char *needle_end = strchr(needle_start, 0);
    return _z_bstrstr((_z_borrowed_str_t){.start = haystack_start, .end = haystack_end},
                      (_z_borrowed_str_t){.start = needle_start, .end = needle_end});
}
char const *_z_strstr_skipneedle(char const *haystack_start, char const *haystack_end, const char *needle_start) {
    const char *needle_end = strchr(needle_start, 0);
    return _z_bstrstr_skipneedle((_z_borrowed_str_t){.start = haystack_start, .end = haystack_end},
                                 (_z_borrowed_str_t){.start = needle_start, .end = needle_end});
}

char const *_z_bstrstr_skipneedle(_z_borrowed_str_t haystack, _z_borrowed_str_t needle) {
    char const *result = _z_bstrstr(haystack, needle);
    return result + (result ? (needle.end - needle.start) : 0);
}

_z_borrowed_str_t _z_splitstr_next(_z_splitstr_t *this) {
    _z_borrowed_str_t result = this->s;
    if (this->s.start) {
        result.end = _z_strstr(result.start, result.end, this->delimiter);
        if (!result.end) {
            result.end = this->s.end;
        }
        if (result.end == this->s.end) {
            this->s = (_z_borrowed_str_t){.start = NULL, .end = NULL};
        } else {
            this->s.start = result.end + strlen(this->delimiter);
        }
    }
    return result;
}

_z_borrowed_str_t _z_splitstr_nextback(_z_splitstr_t *this) {
    _z_borrowed_str_t result = this->s;
    if (this->s.start) {
        result.start = _z_rstrstr(result.start, result.end, this->delimiter);
        if (!result.start) {
            result.start = this->s.start;
        }
        if (result.start == this->s.start) {
            this->s = (_z_borrowed_str_t){.start = NULL, .end = NULL};
        } else {
            this->s.end = result.start - strlen(this->delimiter);
        }
    }
    return result;
}

size_t _z_strcnt(char const *haystack_start, const char *harstack_end, const char *needle_start) {
    size_t result = 0;
    for (; haystack_start; haystack_start = _z_strstr_skipneedle(haystack_start, harstack_end, needle_start)) {
        result++;
    }
    return result;
}

size_t _z_str_startswith(const char *s, const char *needle) {
    size_t i = 0;
    for (; needle[i]; i++) {
        if (s[i] != needle[i]) {
            return 0;
        }
    }
    return i;
}