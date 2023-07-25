#include "stdio.h"
#include "zenoh-pico.h"
#include "zenoh-pico/api/primitives.h"

void callback(const z_sample_t* sample, void* context) {
    z_publisher_t pub = z_publisher_loan((z_owned_publisher_t*)context);
    z_publisher_put(pub, sample->payload.start, sample->payload.len, NULL);
}
void drop(void* context) {
    z_owned_publisher_t* pub = (z_owned_publisher_t*)context;
    z_undeclare_publisher(pub);
    // A note on lifetimes:
    //  here, `sub` takes ownership of `pub` and will drop it before returning from its own `drop`,
    //  which makes passing a pointer to the stack safe as long as `sub` is dropped in a scope where `pub` is still
    //  valid.
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    z_owned_config_t config = z_config_default();
    z_owned_session_t session = z_open(z_move(config));
    z_keyexpr_t ping = z_keyexpr_unchecked("test/ping");
    z_keyexpr_t pong = z_keyexpr_unchecked("test/pong");
    z_owned_publisher_t pub = z_declare_publisher(z_session_loan(&session), pong, NULL);
    z_owned_closure_sample_t respond = z_closure(callback, drop, (void*)z_move(pub));
    z_owned_subscriber_t sub = z_declare_subscriber(z_session_loan(&session), ping, z_move(respond), NULL);
    while (getchar() != 'q') {
    }
    z_undeclare_subscriber(z_move(sub));
    z_close(z_move(session));
}