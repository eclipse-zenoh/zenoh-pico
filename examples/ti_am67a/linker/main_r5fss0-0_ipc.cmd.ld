/*
 * Linker command file for TI AM67A (J722S) main-R5FSS0-0 — FreeRTOS IPC mode.
 *
 * Used when ZENOH_TI_AM67A_IPC=ON.  Unlike the CPSW Ethernet linker
 * (main_r5fss0-0.cmd.ld), this file adds the regions required by the
 * RPMessage driver: resource table, Linux VRING carveout, RTOS VRING SHM.
 *
 * Memory layout
 * ─────────────
 *   DDR_CODE_DATA                0xA2200000  14 MB   code + data + heap + stacks
 *   DDR_IPC_RESOURCE_TABLE_LINUX 0xA2100000   1 KB   .resource_table (remoteproc reads this)
 *   DDR_IPC_TRACE_LINUX          0xA2100400  ~1 MB   .bss.debug_mem_trace_buf
 *   DDR_IPC_VRING_LINUX          0xA2000000   1 MB   Linux VRING carveout (DTS reserved-memory)
 *   DDR_IPC_VRING_RTOS           0xA5000000  28 MB   .bss.ipc_vring_mem (R5F↔R5F, NOLOAD)
 *
 * The DDR_IPC_VRING_LINUX carveout address must match the reserved-memory
 * node in the Linux device tree for j722s-evm main-r5fss0-0 remoteproc.
 *
 * Reference:
 *   MCU+ SDK j722s 11.02.01 examples/drivers/ipc/ipc_rpmsg_echo_linux/
 *       j722s-evm/main-r5fss0-0_freertos/ti-arm-clang/linker.cmd
 */

--stack_size=16384
--heap_size=65536

-e_vectors

__IRQ_STACK_SIZE       = 4096;
__FIQ_STACK_SIZE       = 256;
__SVC_STACK_SIZE       = 4096;   /* FreeRTOS uses SVC mode for task execution */
__ABORT_STACK_SIZE     = 256;
__UNDEFINED_STACK_SIZE = 256;

MEMORY
{
    R5F_VECS  : ORIGIN = 0x00000000, LENGTH = 0x00000040
    R5F_TCMA  : ORIGIN = 0x00000040, LENGTH = 0x00007FC0
    R5F_TCMB0 : ORIGIN = 0x41010000, LENGTH = 0x00004000

    /* Linux VRING carveout — non-cached (MPU CONFIG_MPU_IPC_VRING).
     * Defined in DTS as reserved-memory for main-r5fss0-0 remoteproc.
     * No linker section maps here; the region is reserved by the DTS. */
    DDR_IPC_VRING_LINUX          : ORIGIN = 0xA2000000, LENGTH = 0x00100000

    /* Resource table — remoteproc reads this to discover VRING addresses */
    DDR_IPC_RESOURCE_TABLE_LINUX : ORIGIN = 0xA2100000, LENGTH = 0x00000400

    /* DebugP trace log — Linux can read via debugfs */
    DDR_IPC_TRACE_LINUX          : ORIGIN = 0xA2100400, LENGTH = 0x000FFC00

    /* Application code + data (same base as CPSW mode) */
    DDR_CODE_DATA                : ORIGIN = 0xA2200000, LENGTH = 0x00E00000

    /* RTOS/NoRTOS IPC shared memory — non-cached (MPU CONFIG_MPU_IPC_VRING) */
    DDR_IPC_VRING_RTOS           : ORIGIN = 0xA5000000, LENGTH = 0x01C00000
}

SECTIONS
{
    .vectors: {} palign(8) > R5F_VECS

    GROUP {
        .text.hwi:   palign(8)
        .text.cache: palign(8)
        .text.mpu:   palign(8)
        .text.boot:  palign(8)
        .text:abort: palign(8)
    } > R5F_TCMA

    GROUP {
        .text:   {} palign(8)
        .rodata: {} palign(8)
    } > DDR_CODE_DATA

    GROUP {
        .data: {} palign(8)
    } > DDR_CODE_DATA

    GROUP {
        .bss:    {} palign(8)
        RUN_START(__BSS_START)
        RUN_END(__BSS_END)
        .sysmem: {} palign(8)
        .stack:  {} palign(8)
    } > DDR_CODE_DATA

    GROUP {
        .irqstack: {
            . = . + __IRQ_STACK_SIZE;
        } align(8)
        RUN_START(__IRQ_STACK_START)
        RUN_END(__IRQ_STACK_END)

        .fiqstack: {
            . = . + __FIQ_STACK_SIZE;
        } align(8)
        RUN_START(__FIQ_STACK_START)
        RUN_END(__FIQ_STACK_END)

        .svcstack: {
            . = . + __SVC_STACK_SIZE;
        } align(8)
        RUN_START(__SVC_STACK_START)
        RUN_END(__SVC_STACK_END)

        .abortstack: {
            . = . + __ABORT_STACK_SIZE;
        } align(8)
        RUN_START(__ABORT_STACK_START)
        RUN_END(__ABORT_STACK_END)

        .undefinedstack: {
            . = . + __UNDEFINED_STACK_SIZE;
        } align(8)
        RUN_START(__UNDEFINED_STACK_START)
        RUN_END(__UNDEFINED_STACK_END)
    } > DDR_CODE_DATA

    GROUP {
        .ARM.exidx:  {} palign(8)
        .init_array: {} palign(8)
        .fini_array: {} palign(8)
    } > DDR_CODE_DATA

    /* IPC VRING section — NOLOAD (remoteproc does not initialise this) */
    .bss.ipc_vring_mem (NOLOAD) : {} > DDR_IPC_VRING_RTOS

    /* Resource table — remoteproc reads this to discover VRING addresses */
    GROUP {
        .resource_table: {} palign(1024)
    } > DDR_IPC_RESOURCE_TABLE_LINUX

    /* DebugP trace log — Linux can read via debugfs */
    .bss.debug_mem_trace_buf : {} palign(128) > DDR_IPC_TRACE_LINUX
}
