# zenoh-pico Examples — TI AM64x (am64x-evm, r5fss0-0, FreeRTOS)

Peer-mode UDP multicast examples for the TI AM64x EVM using:

| Component | Details |
|-----------|---------|
| SoC       | AM6442 (AM64x) |
| Core      | r5fss0-0 (Cortex-R5F, MAIN domain) |
| RTOS      | FreeRTOS |
| SDK       | TI MCU+ SDK AM64x 12.00.00 |
| Toolchain | TI ARM Clang (tiarmclang) |

The examples support two transport backends selectable at build time:

| Mode | CMake flag | Hardware needed | Linux side |
|---|---|---|---|
| **CPSW Ethernet** (default) | *(none)* | CPSW PHY + cable | standard IP routing / DHCP |
| **IPC / RPMsg** | `-DZENOH_TI_AM64X_IPC=ON` | none (shared DRAM) | `rpmsg_net.ko` kernel module |

---

## Prerequisites

| Tool | Version | Default location |
|------|---------|-----------------|
| TI MCU+ SDK AM64x | 12.00.00 | `/opt/ti/mcu_plus_sdk_am64x_12_00_00_27/` |
| TI ARM Clang CGT  | 3.2.2.LTS | `/opt/ti/ti-cgt-armllvm_3.2.2.LTS/` |
| TI SysConfig      | 1.21.2 – 1.28.0 | `/opt/ti/sysconfig_1.28.0/` |
| CMake             | ≥ 3.20 | — |
| Python 3          | ≥ 3.8  | — |

Install the SDK from [TI E2E / MySecureSoftware](https://www.ti.com/tool/download/MCU-PLUS-SDK-AM64X)
and the CGT from [TI ARM Clang](https://www.ti.com/tool/ARM-CGT-CLANG).

---

## Build Steps

### 1. Generate SysConfig files

If CMake finds `sysconfig_cli.sh` at `/opt/ti/sysconfig_1.28.0/`, it **runs SysConfig automatically during `cmake --build`**. To run manually:

```bash
/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh \
    -s /opt/ti/mcu_plus_sdk_am64x_12_00_00_27/.metadata/product.json \
    -o examples/ti_am64x/syscfg \
    examples/ti_am64x/example.syscfg

python3 examples/ti_am64x/patch_ti_enet_init.py \
    examples/ti_am64x/syscfg/ti_enet_init.c
```

The `patch_ti_enet_init.py` script fixes a SysConfig code-generation bug where
a `const` struct is used as a static initializer (invalid C).

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
    -DZP_PLATFORM=ti_am64x \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release
```

> **Note:** `CGT_TI_ARM_CLANG_PATH` can also be set as an environment variable instead of passing `-D`.

### 3. Build

```bash
cmake --build build --target examples -j$(nproc)
```

Output files: `build/examples/ti_am64x/z_pub.out`, `z_sub.out`, `z_pubsub.out`

---

## Examples

| Binary | Description |
|--------|-------------|
| `z_pub.out` | Publishes `"[N] Hello from TI AM64x R5F!"` on `t3/pub/data` every second |
| `z_sub.out` | Subscribes to `t3/pub/**` and prints each message |
| `z_pubsub.out` | Combined: publishes on `t3/pubsub/tx`, subscribes to `t3/pubsub/rx` |

All examples use **peer mode** over **UDP multicast** (`udp/224.0.0.224:7446`).

---

## IPC / RPMsg Transport Mode

When CPSW Ethernet is unavailable or undesirable (e.g. the Ethernet PHY is used
by another core, or the board has no network cable), zenoh-pico can tunnel all
traffic over the TI IPC RPMessage channel between the R5F and the Linux A53 core.
From zenoh-pico's perspective the transport is unchanged — it still uses lwIP +
UDP; the difference is that Ethernet frames travel through shared DRAM VRINGs
instead of over a PHY.

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
              (0xA0000000, non-cached)
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

CMake runs SysConfig automatically when `-DZENOH_TI_AM64X_IPC=ON` is set. To run manually:

```bash
/opt/ti/sysconfig_1.28.0/sysconfig_cli.sh \
    -s /opt/ti/mcu_plus_sdk_am64x_12_00_00_27/.metadata/product.json \
    -o examples/ti_am64x/syscfg_ipc \
    examples/ti_am64x/example_ipc.syscfg

python3 examples/ti_am64x/patch_ti_ipc_config.py \
    examples/ti_am64x/syscfg_ipc/ti_drivers_config.c
```

The `patch_ti_ipc_config.py` script fixes a SysConfig code-generation issue:
without `memory_configurator`, the generated `ti_drivers_config.c` has empty
subscripts (`gIpcSharedMem[]`) for inter-R5F VRING addresses. The patch replaces
those with `0U` — safe since this firmware only uses Linux IPC, not R5F-to-R5F
VRING communication.

Generated files land in `syscfg_ipc/`. They are excluded by `.gitignore`; CMake
regenerates and patches them automatically on a clean build.

#### 2. CMake configure — IPC mode

```bash
cmake -B build \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain/ti-arm-clang-r5f.cmake \
    -DCGT_TI_ARM_CLANG_PATH=/opt/ti/ti-cgt-armllvm_3.2.2.LTS \
    -DZP_PLATFORM=ti_am64x \
    -DZENOH_TI_AM64X_IPC=ON \
    -DBUILD_EXAMPLES=ON \
    -DCMAKE_BUILD_TYPE=Release
```

#### 3. Build

```bash
cmake --build build --target examples -j$(nproc)
```

Output files: `build/examples/ti_am64x/z_pub.out`, `z_sub.out`, `z_pubsub.out`

#### 4. Linux side — load rpmsg_net kernel module

The `rpmsg_net` kernel module creates a virtual Ethernet device `rpmsg0` backed
by the RPMsg channel. Source: [https://github.com/t3gemstone/rpmsg-net](https://github.com/t3gemstone/rpmsg-net)

```bash
# Clone and build the kernel module (once, on the target Linux system)
git clone https://github.com/t3gemstone/rpmsg-net
cd rpmsg-net
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules

# Load and configure (after R5F firmware has started)
sudo insmod rpmsg_net.ko
sudo ip addr add 192.168.200.2/30 dev rpmsg0
sudo ip link set rpmsg0 up

# Add multicast route if using peer mode
sudo ip route add 224.0.0.0/4 dev rpmsg0
```

When the R5F firmware boots it announces the `"rpmsg-enet"` service; `rpmsg_net.ko`
matches on that name and the `rpmsg0` device becomes active.

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

#### 6. Linux side — run zenoh

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

The VRING shared memory at `0xA0000000` (1 MB) must match the `reserved-memory`
carveout in the Linux device tree for the AM64x r5fss0-0 remoteproc. If your DTS
uses a different address, update `CONFIG_MPU_LINUX_IPC.baseAddr` in
`example_ipc.syscfg` and the `LINUX_IPC_SHM_MEM` region in
`linker/r5fss0-0_ipc.cmd.ld` accordingly.

### MPU configuration requirements

Two MPU regions are critical for correct IPC operation. Both **must** use `attributes = "NonCached"` (tex=1, Normal Non-Cacheable). Using `"Device"` (tex=0) is architecturally incorrect for shared RAM — DCCIMVAC cache operations are UNPREDICTABLE on Device memory, making the trace buffer and VRING invisible to Linux.

| Region name | Base address | Size | Purpose |
|---|---|---|---|
| `CONFIG_MPU_LINUX_IPC` | `0xA0000000` | 1 MB | Linux VRING + resource table + trace buffer |
| `CONFIG_MPU_IPC_SHM`   | `0xA5000000` | 64 KB | R5F VRING / IPC shared memory |

Both are set correctly in `example_ipc.syscfg`. Do not change `attributes` to `"Device"`.

### Debug log (trace0) configuration

The R5F trace buffer (`/sys/kernel/debug/remoteproc/remoteproc0/trace0`) is populated via `DebugP_log()` → `putchar_()` → `DebugP_memLogWriterPutChar()`.

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

### lwIP core lock

The TI SDK builds lwIP with `LWIP_TCPIP_CORE_LOCKING = 1`. Any direct call to lwIP internal functions (e.g. `netif_find()`, `netif_ip4_addr()`) from a non-`tcpip_thread` task **must** hold the core lock:

```c
LOCK_TCPIP_CORE();
struct netif *netif = netif_find(iface);
// ... read netif fields ...
UNLOCK_TCPIP_CORE();
```

The `udp_multicast_lwip.c` `__get_ip_from_iface()` function already wraps `netif_find()` and `netif_ip4_addr()` with the lock (guarded by `#if LWIP_TCPIP_CORE_LOCKING`).

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

**IPC / RPMsg mode (`ZENOH_TI_AM64X_IPC=ON`):**

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

### Memory map

**CPSW Ethernet mode (linker/r5fss0-0.cmd.ld):**

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| R5F_VECS  | `0x00000000` | 64 B | Interrupt vector table |
| R5F_TCMA  | `0x00000040` | 32 KB | (available) |
| R5F_TCMB0 | `0x41010000` | 32 KB | (available) |
| MSRAM     | `0x70080000` | 12 KB | Boot/MPU/cache init code |
| DDR       | `0x80000000` | 14 MB | Application code + data + heap |

**IPC mode (linker/r5fss0-0_ipc.cmd.ld):**

| Region | Address | Size | Usage |
|--------|---------|------|-------|
| R5F_VECS  | `0x00000000` | 64 B | Interrupt vector table |
| R5F_TCMA  | `0x00000040` | 32 KB | (available) |
| R5F_TCMB0 | `0x41010000` | 32 KB | (available) |
| MSRAM     | `0x70080000` | 256 KB | (available) |
| DDR_0     | `0xA0100000` | 4 KB | `.resource_table` (remoteproc reads this) |
| DDR_1     | `0xA0101000` | ~15 MB | Code + data + heap + stacks |
| LINUX_IPC | `0xA0000000` | 1 MB | Linux VRING carveout (non-cached) |
| USER_SHM  | `0xA5000000` | 128 B | `.bss.user_shared_mem` |
| LOG_SHM   | `0xA5000080` | ~16 KB | `.bss.log_shared_mem` |
| RTOS_IPC  | `0xA5004000` | 48 KB | `.bss.ipc_vring_mem` |

Stack size: 16 KB | Heap size: 64 KB (lwIP pbuf pool)

---

## Interoperability

Run a zenoh peer on a Linux host connected to the same network:

```bash
# Subscribe to AM64x publisher (CPSW mode)
zenoh-pico-sub --mode peer --listen udp/224.0.0.224:7446 -k "t3/pub/**"

# Publish to AM64x subscriber (CPSW mode)
zenoh-pico-pub --mode peer --listen udp/224.0.0.224:7446 -k "t3/pub/data" -v "Hello"
```

Or use the [zenoh router](https://github.com/eclipse-zenoh/zenoh) for client/router mode.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| `SysConfig CLI not found` | `sysconfig_cli.sh` not on search path | Pass `-DSYSCONFIG_CLI=...` |
| `SysConfig version mismatch` | SysConfig < 1.21.2 installed | Install SysConfig 1.28.0 |
| `LwipifEnetApp_netifOpen failed` | Enet driver did not start | Check `Drivers_open()` / `Board_driversOpen()` |
| DHCP timeout | Cable/switch issue | Check physical link; try static IP |
| `z_open failed` | Multicast unreachable | Verify router/switch allows `224.0.0.224` multicast |
| Linker error: `*.cmd` not found | `.gitignore` blocking | Extension must be `.cmd.ld`, not `.cmd` |

**IPC / RPMsg mode specific:**

| Symptom | Likely cause | Fix |
|---|---|---|
| `RPMessage_waitForLinuxReady timed out` | Linux remoteproc not started or DT mismatch | Verify `echo start > /sys/class/remoteproc/remoteproc0/state`; check VRING carveout in DTS vs `example_ipc.syscfg` |
| `rpmsg_net.ko` not found after `insmod` | Service name mismatch | R5F announces `"rpmsg-enet"`; check `rpmsg_net.c` service name matches |
| `rpmsg0` device does not appear | Module not loaded, or R5F not booted yet | Boot R5F first, then `insmod rpmsg_net.ko`; check `dmesg` for probe messages |
| `trace0` is empty even though `virtio0: rpmsg host is online` | `putchar(character)` semihosting hang in `putchar_()` | Ensure `example_ipc.syscfg` has `enableCssLog=false` and `enableUartLog=false`; SysConfig must regenerate `ti_dpl_config.c` |
| `trace0` is empty (wrong MPU attributes) | IPC region marked as Device (tex=0) instead of NonCached (tex=1) | Set `attributes = "NonCached"` in `example_ipc.syscfg` for both IPC MPU regions |
| `pbuf_alloc failed (dropped N bytes)` — ARP/zenoh drops | SDK `PBUF_POOL_SIZE=0`; pool pbufs unavailable | `rpmsg_lwip_netif.c` must use `pbuf_alloc(PBUF_RAW, rx_len, PBUF_RAM)` — already fixed |
| `z_open failed` (immediate, before network traffic) | UDP multicast locator missing `#iface=rp0` | Set `MULTICAST_EP "udp/224.0.0.224:7446#iface=rp0"` in `main.c` |
| `Function called without core lock` assertion in `sys_arch.c` | Direct lwIP internal calls without `LOCK_TCPIP_CORE()` | `udp_multicast_lwip.c` wraps `netif_find()` with core lock — already fixed; do not call lwIP internals without the lock |
| `ping 192.168.200.1` → Destination Host Unreachable | ARP dropped due to `pbuf_alloc failed` | Fix pbuf issue above; confirm `rpmsg0` has IP and is up |
| Peer not ready after 15 s | Linux never sent first frame | Check `ip link set rpmsg0 up`; verify `ip addr` on `rpmsg0`; try `ping 192.168.200.1` from Linux |
| Large zenoh messages dropped | MTU mismatch | Confirm `rpmsg_net.ko` was built with 512-byte VRING size (478-byte MTU) |
| Linker: undefined `enet_cpsw` symbols in IPC mode | Old build directory used | Reconfigure with `-DZENOH_TI_AM64X_IPC=ON` in a fresh build dir |
| Build error: `expected expression` in `ti_drivers_config.c` | `patch_ti_ipc_config.py` not run | Run patch manually: `python3 patch_ti_ipc_config.py syscfg_ipc/ti_drivers_config.c` |
