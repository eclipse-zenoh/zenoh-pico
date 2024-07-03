#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "zenoh-pico/api/primitives.h"
#include "zenoh-pico/api/types.h"

#undef NDEBUG
#include <assert.h>

void test_null_encoding(void) {
    z_owned_encoding_t e;
    z_encoding_null(&e);
    assert(!z_encoding_check(&e));
    z_encoding_drop(z_encoding_move(&e));
}

void test_encoding_without_id(void) {
    z_owned_encoding_t e1;
    z_encoding_from_str(&e1, "my_encoding");
    assert(z_encoding_check(&e1));
    z_owned_string_t s;
    z_encoding_to_string(z_encoding_loan(&e1), &s);
    assert(strncmp("zenoh/bytes;my_encoding", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) == 0);
    z_encoding_drop(z_encoding_move(&e1));
    z_string_drop(z_string_move(&s));

    z_owned_encoding_t e2;
    z_encoding_from_substr(&e2, "my_encoding", 4);
    assert(z_encoding_check(&e2));

    z_encoding_to_string(z_encoding_loan(&e2), &s);
    assert(strncmp("zenoh/bytes;my_e", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) == 0);
    z_encoding_drop(z_encoding_move(&e2));
    z_string_drop(z_string_move(&s));
}

void test_encoding_with_id(void) {
    z_owned_encoding_t e1;
    z_encoding_from_str(&e1, "zenoh/string;utf8");
    assert(z_encoding_check(&e1));
    z_owned_string_t s;
    z_encoding_to_string(z_encoding_loan(&e1), &s);
    assert(strncmp("zenoh/string;utf8", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) == 0);
    z_encoding_drop(z_encoding_move(&e1));
    z_string_drop(z_string_move(&s));

    z_owned_encoding_t e2;
    z_encoding_from_substr(&e2, "zenoh/string;utf8", 15);
    assert(z_encoding_check(&e2));

    z_encoding_to_string(z_encoding_loan(&e2), &s);
    assert(strncmp("zenoh/string;utf8", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) == 0);
    z_encoding_drop(z_encoding_move(&e2));
    z_string_drop(z_string_move(&s));

    z_owned_encoding_t e3;
    z_encoding_from_str(&e3, "custom_id;custom_schema");
    assert(z_encoding_check(&e3));

    z_encoding_to_string(z_encoding_loan(&e3), &s);
    assert(strncmp("zenoh/bytes;custom_id;custom_schema", z_string_data(z_string_loan(&s)),
                   z_string_len(z_string_loan(&s))) == 0);
    z_encoding_drop(z_encoding_move(&e3));
    z_string_drop(z_string_move(&s));

    z_owned_encoding_t e4;
    z_encoding_from_substr(&e4, "custom_id;custom_schema", 16);
    assert(z_encoding_check(&e2));

    z_encoding_to_string(z_encoding_loan(&e4), &s);
    assert(strncmp("zenoh/bytes;custom_id;custom", z_string_data(z_string_loan(&s)), z_string_len(z_string_loan(&s))) ==
           0);
    z_encoding_drop(z_encoding_move(&e4));
    z_string_drop(z_string_move(&s));
}

int main(void) {
    test_null_encoding();
    test_encoding_without_id();
    test_encoding_with_id();
}
