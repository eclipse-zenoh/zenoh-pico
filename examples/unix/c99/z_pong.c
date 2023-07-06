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
    z_owned_session_t session = z_open(z_config_move(&config));
    if (!z_session_check(&session)) {
        printf("Unable to open session!\n");
        return -1;
    }

    if (zp_start_read_task(z_session_loan(&session), NULL) < 0 ||
        zp_start_lease_task(z_session_loan(&session), NULL) < 0) {
        printf("Unable to start read and lease tasks");
        return -1;
    }

    z_keyexpr_t pong = z_keyexpr_unchecked("test/pong");
    z_owned_publisher_t pub = z_declare_publisher(z_session_loan(&session), pong, NULL);
    if (!z_publisher_check(&pub)) {
        printf("Unable to declare publisher for key expression!\n");
        return -1;
    }

    z_keyexpr_t ping = z_keyexpr_unchecked("test/ping");
    z_owned_closure_sample_t respond = z_closure_sample(callback, drop, (void*)z_publisher_move(&pub));
    z_owned_subscriber_t sub =
        z_declare_subscriber(z_session_loan(&session), ping, z_closure_sample_move(&respond), NULL);
    if (!z_subscriber_check(&sub)) {
        printf("Unable to declare subscriber for key expression.\n");
        return -1;
    }

    while (getchar() != 'q') {
    }

    z_undeclare_subscriber(z_subscriber_move(&sub));

    zp_stop_read_task(z_session_loan(&session));
    zp_stop_lease_task(z_session_loan(&session));

    z_close(z_session_move(&session));
}