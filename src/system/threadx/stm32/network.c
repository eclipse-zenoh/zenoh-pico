/* Ubiquity robotics
 * ======================================================================
 * Zenoh-pico stm32 threadx
 * Network implementation for serial device running in circular DMA mode.
 * ======================================================================
 */
#if defined(ZENOH_THREADX_STM32)
#include "hal.h"
#include "zenoh-pico/link/transport/socket.h"
#include "zenoh-pico/system/platform.h"
#include "zenoh-pico/utils/pointers.h"

#if Z_FEATURE_LINK_TCP == 1
#error "Z_FEATURE_LINK_TCP is not supported"
#endif

#if Z_FEATURE_LINK_BLUETOOTH == 1
#error "Z_FEATURE_LINK_BLUETOOTH is not supported"
#endif

#if Z_FEATURE_RAWETH_TRANSPORT == 1
#error "Z_FEATURE_RAWETH_TRANSPORT is not supported"
#endif

void _z_socket_close(_z_sys_net_socket_t *sock) { _ZP_UNUSED(sock); }

z_result_t _z_socket_set_blocking(const _z_sys_net_socket_t *sock, bool blocking) {
    _ZP_UNUSED(sock);
    _ZP_UNUSED(blocking);
    return _Z_RES_OK;
}

z_result_t _z_socket_wait_readable(_z_socket_wait_iter_t *iter, uint32_t timeout_ms) {
    _ZP_UNUSED(iter);
    _ZP_UNUSED(timeout_ms);
    return _Z_RES_OK;
}

#endif  // ZENOH_THREADX_STM32
