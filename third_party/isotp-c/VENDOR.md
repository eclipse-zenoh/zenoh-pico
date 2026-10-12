# isotp-c -- vendored

| | |
| --- | --- |
| Vendored from | https://github.com/jerry73204/isotp-c |
| Commit | `8aa1b2a` |
| Fork of | https://github.com/SimonCahill/isotp-c @ `abb9e552df0e7ca0148c146124795341d57124fe` |
| Licence | MIT (`LICENSE`, verbatim) |
| Vendored for | the ISO-TP unicast link |

A portable ISO 15765-2 implementation for the platforms that have no ISO-TP of
their own. Linux has it in the kernel (`CAN_ISOTP`) and Zephyr in
`subsys/canbus/isotp`; both are used in preference. This library is for
everything else -- FreeRTOS, ThreadX, NuttX, bare metal.

## Why a fork, and what is in it

The copy here comes from a fork carrying **one commit** on top of `abb9e55`,
which adds an `ISO_TP_NO_FORMATTED_ERRORS` build flag. That is the whole
difference; `git diff abb9e55..8aa1b2a` shows 18 added lines and nothing else.

`snprintf` is the library's only libc dependency beyond `memcpy`/`memset`, and
it exists solely for two formatted diagnostics -- a bad size in
`isotp_send_with_id`, a bad TX_DL in `isotp_set_tx_dl` -- each writing into a
128-byte stack buffer. That makes `snprintf` a hard requirement of the whole
library on exactly the targets it exists for. The flag omits both; the errors
are still reported through `isotp_user_debug()` and the return codes are
unchanged. It is **off by default**, so this tree builds identically to
upstream unless a port asks otherwise.

Measured on Cortex-M4, `-Os -DNDEBUG -ffreestanding`: `.text` 2235 -> 2091
bytes, largest stack frame 160 -> 32 bytes, and the object then needs nothing
from libc but `memcpy` and `memset`.

The fork is a staging area, not a destination -- the change belongs upstream,
and this table should point back at `SimonCahill/isotp-c` once it lands there.

## Why this one

The survey is recorded here so nobody re-litigates it. The two
near misses are **licence** rejections, not technical ones:

* `devcoons/iso15765-canbus` -- AGPL-3.0
* `altelch/iso-tp` -- GPL-3.0

Neither can enter this tree. `SimonCahill/isotp-c` is MIT, is a maintained fork
of the widely used `lishen2/isotp-c`, and **uses no allocator at all** -- `grep
-c 'malloc\|calloc\|free('` over `isotp.c` returns 0. The caller supplies both
buffers to `isotp_init_link`, which is what makes it usable on an MCU with no
heap.

## Files

Only the library itself is vendored: `isotp.c`, `isotp.h`, `isotp_config.h`,
`isotp_defines.h`, `isotp_user.h`, `LICENSE`. Upstream's build system, tests,
Doxygen config and submodules are not -- zenoh-pico builds the one translation
unit directly.

## Proven on an MCU

Building `isotp.c` for a bare-metal Cortex-M4 and checking the object for an
allocator is how this was confirmed. What the object actually needs from outside:

```
U __assert_func
U isotp_user_debug
U isotp_user_get_us
U isotp_user_send_can
U memcpy
U memset
U snprintf
```

Three of those are the hooks below. `memcpy`/`memset` are unavoidable. The two
worth knowing about are **`__assert_func`** and **`snprintf`**: the library
asserts on caller misuse and formats its debug strings, so a build that wants
neither should compile it with `-DNDEBUG` and an `isotp_user_debug` that
discards its arguments. Neither is on a hot path.

## What the integrator must supply

Three hooks, declared in `isotp_user.h`:

| Hook | Purpose |
| --- | --- |
| `isotp_user_send_can` | put one CAN frame on the bus |
| `isotp_user_get_us` | a microsecond clock, for `STmin` and the N_* timers |
| `isotp_user_debug` | diagnostics |

The library is **poll-driven**: `isotp_on_can_message` feeds it received frames
and `isotp_poll` runs its timers. There is no thread and no I/O inside it.

## N_As is the integrator's problem

`isotp_user_send_can` returns as soon as the frame is handed to the driver, not
when the bus has acknowledged it. ISO 15765-2's `N_As` is the *transmit
confirmation* time, so on a platform whose send is asynchronous the library
cannot measure it and neither can this hook. No
portable ISO-TP library supplies this. The port must either block in the hook
until transmit-confirm or add its own timer; see the `unix` port in
`src/system/unix/network.c`, which documents which of the two it does and why.

## Updating

Bump the commit above, re-copy the six files, and re-run the link tests
against both backends. The kernel's `CAN_ISOTP` is the oracle for this
library on Linux, which is the whole reason the `unix` platform implements
the link twice.
