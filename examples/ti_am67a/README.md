# zenoh-pico — T3 Gemstone O1 / TI AM67A (J722S) Cortex-R5F Examples

This directory contains three FreeRTOS + lwIP examples that run zenoh-pico on the **main-R5FSS0-0** Cortex-R5F core of the **T3 Gemstone O1** development board ([t3gemstone.org](https://t3gemstone.org)), which is based on the TI AM67A (J722S) SoC.

## Requirements

| Tool / Library | Version |
|---|---|
| MCU+ SDK (J722S) | 11.02.01 |
| TI ARM Clang | 3.2.2.LTS |
| TI SysConfig | **≥ 1.26.2** (tested: 1.28.0) |
| CMake | ≥ 3.20 |

Default install paths:
- SDK: `/opt/ti/mcu_plus_sdk_j722s_11_02_01_05/`
- SysConfig: `/opt/ti/sysconfig_1.28.0/`

## Examples

| Target | Description | Key expression |
|---|---|---|
| `z_pub` | Publisher — sends a message every second | `t3/pub/data` |
| `z_sub` | Subscriber — prints received samples via `DebugP_log` | `t3/pub/**` |
| `z_pubsub` | Both combined — two separate FreeRTOS tasks | `t3/pubsub/tx` / `t3/pubsub/rx` |

All examples use **peer mode + UDP multicast** (`udp/224.0.0.224:7446#iface=rp0` in IPC mode, `#iface=en0` or similar for CPSW mode).

The examples support two transport backends selectable at build time:

| Mode | CMake flag | Hardware needed | Linux side |
|---|---|---|---|
| **CPSW Ethernet** (default) | *(none)* | CPSW PHY + cable | standard IP routing |
| **IPC / RPMsg** | `-DZENOH_TI_AM67A_IPC=ON` | none (shared DRAM) | `rpmsg_net.ko` kernel module |

---

## Build Steps

### 1. Generate SysConfig files

If CMake finds `sysconfig_cli.sh` at `/opt/ti/sysconfig_1.28.0/`, it **runs SysConfig automatically during `cmake --build`**. To run manually:

```bash
/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh \
    -s /opt/ti/mcu_plus_sdk_j722s_11_02_01_05/.metadata/product.json \
    -o examples/ti_am67a/syscfg \
    examples/ti_am67a/example.syscfg
```

Generated files land in `syscfg/`. They are excluded by `.gitignore`; CMake regenerates them on a clean build.

> **Note:** If `sysconfig_cli.sh` is in a different location, tell CMake:
> ```bash
> cmake ... -DSYSCONFIG_CLI=/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh
> ```

### 2. CMake configure

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
    -DCGT_TI_ARM_CLANG_PATH=/opt/ti/ti-cgt-armllvm_3.2.2.LTS \
    -DZP_PLATFORM=ti_am67a \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release
```

> **Note:** `CGT_TI_ARM_CLANG_PATH` can also be set as an environment variable instead of passing `-D`.

### 3. Build

```bash
cmake --build build --target examples -j$(nproc)
```

Output files: `build/examples/ti_am67a/z_pub.out`, `z_sub.out`, `z_pubsub.out`

---

## IPC / RPMsg Transport Mode

When CPSW Ethernet is unavailable or undesirable (e.g. Ethernet PHY used by another core), zenoh-pico can tunnel all traffic over the TI IPC RPMessage channel between the R5F and the Linux A53 core. From zenoh-pico's perspective the transport is unchanged — it still uses lwIP + UDP; the difference is that Ethernet frames travel through shared DRAM VRINGs instead of over a PHY.

```
R5F (FreeRTOS)                          Linux A53
──────────────                          ─────────
zenoh-pico                              zenoh router / peer
   │ UDP/IP                                  │ UDP/IP
lwIP netif (rp0)                        rpmsg0 (virtual Ethernet)
   │ Ethernet frames                         │
rpmsg_lwip_netif.c                      rpmsg_net.ko
   │ RPMessage_send/recv                     │ rpmsg_endpoint
   └──────── VRING shared DRAM ──────────────┘
              (0xA2000000, non-cached)
```

**IP addressing** (static, compile-time defaults):

| Side | Interface | Address |
|---|---|---|
| R5F | lwIP `rp0` | `192.168.200.1/30` |
| Linux | `rpmsg0` | `192.168.200.2/30` |

Override the R5F address at compile time:

```cmake
target_compile_definitions(z_pub PRIVATE
    RPMSG_NETIF_IP="192.168.100.1"
    RPMSG_NETIF_MASK="255.255.255.252"
    RPMSG_NETIF_GW="192.168.100.2")
```

**MTU**: The default VirtIO VRING buffer is 512 bytes -> RPMsg payload max = 496 bytes -> IP MTU = **478 bytes** (496 − 14 ETH_HLEN − 4 ETH_FCS). zenoh-pico's `Z_FEATURE_FRAGMENTATION=1` (already enabled) handles messages larger than the MTU transparently.

### Build — IPC mode

#### 1. Generate SysConfig files (IPC variant)

```bash
/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh \
    -s /opt/ti/mcu_plus_sdk_j722s_11_02_01_05/.metadata/product.json \
    -o examples/ti_am67a/syscfg_ipc \
    examples/ti_am67a/example_ipc.syscfg
```

Generated files land in `syscfg_ipc/`. CMake regenerates them automatically if `sysconfig_cli.sh` is found.

#### 2. CMake configure — IPC mode

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
    -DCGT_TI_ARM_CLANG_PATH=/opt/ti/ti-cgt-armllvm_3.2.2.LTS \
    -DZP_PLATFORM=ti_am67a \
    -DZENOH_TI_AM67A_IPC=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release
```

#### 3. Build

```bash
cmake --build build --target examples -j$(nproc)
```

#### 4. Linux side — load rpmsg_net kernel module

The `rpmsg_net` kernel module creates a virtual Ethernet device `rpmsg0` backed by the RPMsg channel. Source: [https://github.com/t3gemstone/rpmsg-net](https://github.com/t3gemstone/rpmsg-net)

```bash
# Clone and build the kernel module (once, on the target Linux system)
git clone https://github.com/t3gemstone/rpmsg-net
cd rpmsg-net
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load and configure
sudo insmod rpmsg_net.ko
sudo ip addr add 192.168.200.2/30 dev rpmsg0
sudo ip link set rpmsg0 up

# Add multicast route if using peer mode
sudo ip route add 224.0.0.0/4 dev rpmsg0
```

When the R5F firmware boots it announces the `"rpmsg-enet"` service; `rpmsg_net.ko` matches on that name and the `rpmsg0` device becomes active.

#### 5. Build zenoh-pico CLI examples for Linux A53

The `z_sub`, `z_pub`, etc. binaries come from zenoh-pico's own `examples/unix/c11/` sources. Cross-compile them from the development host:

```bash
# From the zenoh-pico repo root (development host, requires aarch64-linux-gnu-gcc):
cmake -B build_linux_aarch64 \
    -DCMAKE_SYSTEM_NAME=Linux \
    -DCMAKE_SYSTEM_PROCESSOR=aarch64 \
    -DCMAKE_C_COMPILER=aarch64-linux-gnu-gcc \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_EXAMPLES=ON \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_EXE_LINKER_FLAGS="-static" \
    -DZP_PLATFORM=linux

cmake --build build_linux_aarch64 --target examples -j$(nproc)
```

#### 6. Run zenoh on Linux

Ensure `rpmsg0` is configured (step 4 above), then:

```bash
# Option A — zenoh-pico CLI peer (no router needed):
z_sub -m peer -l "udp/224.0.0.224:7446#iface=rpmsg0" -k "t3/pubsub/tx"
z_pub -m peer -l "udp/224.0.0.224:7446#iface=rpmsg0" -k "t3/pubsub/rx" -v "hello from linux"

# Option B — zenohd router (default config scans 224.0.0.224:7446):
zenohd
```

> **Common mistake**: `zenohd -l udp/192.168.200.2:7447` fails with `EADDRNOTAVAIL` (os error 99)
> if `rpmsg0` is not yet configured. Use `zenohd` without `-l`, or `zenohd -l udp/0.0.0.0:7447`.

### Device tree / VRING carveout

The VRING shared memory at `0xA2000000` must match the `reserved-memory` carveout in the Linux device tree for the J722S (`main-r5fss-dma-memory-region@a2000000`). If your DTS uses a different address, update `CONFIG_MPU_LINUX_IPC.baseAddr` in `example_ipc.syscfg` accordingly.

### MPU configuration requirements

Two MPU regions are critical for correct IPC operation. Both **must** use `attributes = "NonCached"` (tex=1, Normal Non-Cacheable). Using `"Device"` (tex=0) is architecturally incorrect for shared RAM — DCCIMVAC cache operations are UNPREDICTABLE on Device memory, making the trace buffer and VRING invisible to Linux.

| Region name | Base address | Size | Purpose |
|---|---|---|---|
| `CONFIG_MPU_LINUX_IPC` | `0xA2000000` | 2 MB | Linux VRING + resource table + trace buffer |
| `CONFIG_MPU_RTOS_IPC_VRING` | `0xA5000000` | 16 MB | R5F↔R5F VRING (NOLOAD section) |

Both are set correctly in `example_ipc.syscfg`. Do not change `attributes` to `"Device"`.

### Debug log (trace0) configuration

The R5F trace buffer (`/sys/kernel/debug/remoteproc/remoteproc3/trace0`) is populated via `DebugP_log()` → `putchar_()` → `DebugP_memLogWriterPutChar()`.

**Critical**: if `enableCssLog = true` (SysConfig default) or `enableUartLog = true`, SysConfig generates a `putchar_()` that calls the C stdlib `putchar(character)` **before** writing to the trace buffer. On bare-metal without a JTAG debugger, `putchar()` triggers ARM semihosting (`BKPT #0xAB`) and **hangs the firmware silently** — trace0 stays empty even though VirtIO is online.

`example_ipc.syscfg` already sets:
```js
debug_log.enableCssLog       = false;
debug_log.enableUartLog      = false;
debug_log.enableSharedMemLog = false;
```

This produces the minimal `putchar_()`:
```c
void putchar_(char character) {
    DebugP_memLogWriterPutChar(character);
}
```

### lwIP pbuf allocation

The SDK's `lwipopts.h` sets `PBUF_POOL_SIZE = 0` because the CPSW driver uses custom DMA pbufs. The RPMsg netif uses software-copy receive, so it must allocate from the heap instead:

```c
// rpmsg_lwip_netif.c — use PBUF_RAM, not PBUF_POOL
struct pbuf *p = pbuf_alloc(PBUF_RAW, rx_len, PBUF_RAM);
```

Custom memory pools are available (25×128 B, 18×1568 B, 4×4096 B) and handle all normal frame sizes (42-byte ARP, 329-byte zenoh SCOUT, etc.).

### lwIP core lock

The TI SDK builds lwIP with `LWIP_TCPIP_CORE_LOCKING = 1`. Any direct call to lwIP internal functions (e.g. `netif_find()`, `netif_ip4_addr()`) from a non-`tcpip_thread` task **must** hold the core lock:

```c
LOCK_TCPIP_CORE();
struct netif *netif = netif_find(iface);
// ... read netif fields ...
UNLOCK_TCPIP_CORE();
```

`LOCK_TCPIP_CORE()` uses a FreeRTOS recursive mutex, so it is safe to call while socket API functions (which also acquire the lock internally) are nested within the same call chain.

The `udp_multicast_lwip.c` `__get_ip_from_iface()` function already wraps `netif_find()` and `netif_ip4_addr()` with the lock.

---

## Architecture Notes

### FreeRTOS task flow

**CPSW Ethernet mode (default):**

```
main()
  └── xTaskCreateStatic(freertos_main)
        └── vTaskStartScheduler()
              └── freertos_main():
                    Drivers_open()
                    Board_driversOpen()
                    zenoh_net_init()   <- CPSW + lwIP + DHCP
                    z_open(session)
                    zp_start_read_task / zp_start_lease_task
                    └── application task(s)
```

**IPC / RPMsg mode (`ZENOH_TI_AM67A_IPC=ON`):**

```
main()
  └── xTaskCreateStatic(freertos_main)
        └── vTaskStartScheduler()
              └── freertos_main():
                    Drivers_open()
                    Board_driversOpen()
                    zenoh_net_init()   <- RPMsg netif + static IP
                      ├── tcpip_init -> rpmsg_lwip_netif_init()
                      │     ├── RPMessage_waitForLinuxReady()
                      │     ├── RPMessage_construct()
                      │     ├── RPMessage_announce("rpmsg-enet")
                      │     └── xTaskCreateStatic(rpmsg_rx)  <- recv loop
                      └── rpmsg_lwip_netif_wait_peer()  <- waits for first frame
                    z_open(session)
                    zp_start_read_task / zp_start_lease_task
                    └── application task(s)
```

### Network init (`common/zenoh_net_init.c`)

`zenoh_net_init()`:

1. Calls `tcpip_init(_tcpip_init_done_cb)` — starts the lwIP tcpip_thread
2. Inside the callback (runs in tcpip_thread context):
   - `LwipifEnetApp_getHandle()` — obtain CPSW Enet driver handle
   - `LwipifEnetApp_netifOpen()` — open lwIP netif
   - `dhcp_start()` — start DHCP
   - `LwipifEnetApp_startSchedule()` — start Enet Rx/Tx polling task
3. Calling task waits for DHCP IP assignment on a binary semaphore (timeout: 15 s)

**To use a static IP**, define before including `zenoh_net_init.h`:
```c
#define ZENOH_NET_STATIC_IP   "192.168.1.100"
#define ZENOH_NET_STATIC_MASK "255.255.255.0"
#define ZENOH_NET_STATIC_GW   "192.168.1.1"
```

### Network init — IPC mode (`common/zenoh_net_init_rpmsg.c`)

`zenoh_net_init()` in IPC mode:

1. Calls `tcpip_init(_tcpip_init_done_cb)` — starts the lwIP tcpip_thread
2. Inside the callback:
   - `netif_add()` with `rpmsg_lwip_netif_init` — registers the RPMsg netif with static IP `192.168.200.1/30`
   - `netif_set_default()` / `netif_set_up()` / `netif_set_link_up()`
3. Calling task waits on the init semaphore
4. `rpmsg_lwip_netif_wait_peer()` — blocks until Linux sends the first frame (timeout: 15 s)

Inside `rpmsg_lwip_netif_init()`:

1. `RPMessage_waitForLinuxReady()` — waits for Linux remoteproc to finish booting (timeout: 10 s)
2. `RPMessage_construct()` — opens local endpoint `RPMSG_NETIF_ENDPT` (default: 14)
3. `RPMessage_announce(CSL_CORE_ID_A53SS0_0, RPMSG_NETIF_ENDPT, "rpmsg-enet")` — advertises the service; `rpmsg_net.ko` matches on `"rpmsg-enet"` and probes the device
4. Spawns `rpmsg_rx` FreeRTOS task (calls `RPMessage_recv` in a loop; injects frames into lwIP via `netif->input()`)
5. Sets `netif->mtu = 478`, `netif->output = etharp_output`, `netif->linkoutput = _netif_linkoutput`

### Memory map (linker file)

| Region | Address | Size | Usage |
|---|---|---|---|
| R5F_VECS | 0x00000000 | 64 B | Exception vectors |
| R5F_TCMA | 0x00000040 | 32 KB | TCM-A (fast code) |
| R5F_TCMB0 | 0x41010000 | 16 KB | TCM-B0 (fast data) |
| DDR_IPC_RSC_TABLE | 0xA2100000 | 1 KB | VirtIO resource table (Linux reads at boot) |
| DDR_IPC_TRACE_LINUX | 0xA2100400 | 4 KB | `gDebugMemLog` → Linux `trace0` debugfs |
| DDR_CODE_DATA | 0xA2200000 | 14 MB | Code + data + stack + heap |
| RTOS_VRING (NOLOAD) | 0xA5000000 | 16 MB | R5F↔R5F VRING (IPC mode only) |

Stack size: 16 KB | Heap size: 64 KB

> **Note:** Regions `0xA2000000–0xA21FFFFF` and `0xA5000000–0xA4FFFFFF` must be marked **NonCached** (tex=1) in the MPU. Linux maps these same physical addresses via the DTS `reserved-memory` carveout `main-r5fss-dma-memory-region@a2000000`.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `SysConfig CLI not found` | `sysconfig_cli.sh` not on search path | Pass `-DSYSCONFIG_CLI=...` |
| `SysConfig version mismatch` | SysConfig < 1.26.2 installed | Install SysConfig 1.28.0 |
| `LwipifEnetApp_netifOpen failed` | Enet driver did not start | Check `Drivers_open()` / `Board_driversOpen()` |
| DHCP timeout | Cable/switch issue | Check physical link; try static IP |
| `z_open failed` | Multicast unreachable | Verify router/switch allows `224.0.0.224` multicast |
| Linker error: `*.cmd` not found | `.gitignore` blocking | Extension must be `.cmd.ld`, not `.cmd` |

**IPC / RPMsg mode specific:**

| Symptom | Likely cause | Fix |
|---|---|---|
| `RPMessage_waitForLinuxReady timed out` | Linux remoteproc not started or DT mismatch | Verify remoteproc firmware loading; check VRING carveout address in DTS vs `example_ipc.syscfg` |
| `rpmsg_net.ko` not found after `insmod` | Service name mismatch | R5F announces `"rpmsg-enet"`; check `rpmsg_net.c` `RPM_ENET_CH_NAME` matches |
| `rpmsg0` device does not appear | Kernel module not loaded, or R5F firmware not booted yet | Boot R5F first, then `insmod rpmsg_net.ko`; check `dmesg` for `rpmsg_net` probe messages |
| `trace0` is empty even though `virtio0: rpmsg host is online` | `putchar(character)` semihosting hang in `putchar_()` | Ensure `example_ipc.syscfg` has `enableCssLog=false` and `enableUartLog=false`; SysConfig must regenerate `ti_dpl_config.c` |
| `trace0` is empty (wrong MPU attributes) | IPC region marked as Device (tex=0) instead of NonCached (tex=1) | Set `attributes = "NonCached"` in `example_ipc.syscfg` for both IPC MPU regions |
| `pbuf_alloc failed (dropped N bytes)` — ARP/zenoh drops | SDK `PBUF_POOL_SIZE=0`; pool pbufs unavailable | `rpmsg_lwip_netif.c` must use `pbuf_alloc(PBUF_RAW, rx_len, PBUF_RAM)` — already fixed |
| `z_open failed` (immediate, before network traffic) | UDP multicast locator missing `#iface=rp0` | Set `MULTICAST_EP "udp/224.0.0.224:7446#iface=rp0"` in `main.c` |
| `Function called without core lock` assertion in `sys_arch.c` | Direct lwIP internal calls without `LOCK_TCPIP_CORE()` | `udp_multicast_lwip.c` wraps `netif_find()` with core lock — already fixed; do not call lwIP internals without the lock |
| `ping 192.168.200.1` → Destination Host Unreachable | ARP dropped due to `pbuf_alloc failed` | Fix pbuf issue above; confirm `rpmsg0` has IP and is up |
| Peer not ready after 15 s | Linux never sent first frame | Check `ip link set rpmsg0 up`; verify `ip addr` on `rpmsg0`; try `ping 192.168.200.1` from Linux |
| Large zenoh messages dropped | MTU mismatch | Confirm `rpmsg_net.ko` was built from `tools/rpmsg_net/` (fixed 512-byte VRING size, 478-byte MTU) |
| Linker: undefined `enet_cpsw` symbols in IPC mode | Old build directory used | Reconfigure with `-DZENOH_TI_AM67A_IPC=ON` in a fresh build dir |
