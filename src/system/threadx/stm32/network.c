/* Ubiquity robotics
 * ======================================================================
 * Zenoh-pico stm32 threadx
 * Network implementation for serial device running in circular DMA mode.
 * ======================================================================
 */
#if defined(ZENOH_THREADX_STM32)
#include "hal.h"
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

#endif  // ZENOH_THREADX_STM32
