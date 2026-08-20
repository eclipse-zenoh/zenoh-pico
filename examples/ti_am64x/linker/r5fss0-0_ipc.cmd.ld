/*
 * Linker command file for TI AM64x r5fss0-0 — FreeRTOS IPC mode.
 *
 * Used when ZENOH_TI_AM64X_IPC=ON.  Unlike the CPSW Ethernet linker
 * (r5fss0-0.cmd.ld), this file places the firmware in the DDR region that
 * Linux remoteproc expects (0xA0100000/0xA0101000) and reserves the IPC
 * shared-memory sections required by the RPMessage driver.
 *
 * Memory layout
 * ─────────────
 *   DDR_0       0xA0100000  4 KB    .resource_table  (remoteproc reads this)
 *   DDR_1       0xA0101000  ~15 MB  code + data + heap + stacks
 *   LINUX_IPC   0xA0000000  1 MB    Linux VRING carveout (non-cached, DTS carveout)
 *   RTOS_IPC    0xA5004000  48 KB   .bss.ipc_vring_mem (R5F↔R5F non-cached VRING)
 *   LOG_SHM     0xA5000080  ~16 KB  .bss.log_shared_mem
 *   USER_SHM    0xA5000000  128 B   .bss.user_shared_mem
 *
 * The LINUX_IPC carveout address must match the reserved-memory node in the
 * Linux device tree for am64x-evm r5fss0-0 remoteproc.
 *
 * Reference:
 *   MCU+ SDK examples/drivers/ipc/ipc_rpmsg_echo_linux/
 *       am64x-evm/r5fss0-0_freertos/  (memory_configurator generated linker)
 */

--stack_size=16384
--heap_size=65536

-e_vectors

__IRQ_STACK_SIZE       = 256;
__FIQ_STACK_SIZE       = 256;
__SVC_STACK_SIZE       = 4096;
__ABORT_STACK_SIZE     = 256;
__UNDEFINED_STACK_SIZE = 256;

MEMORY
{
    R5F_VECS  : ORIGIN = 0x00000000, LENGTH = 0x00000040
    R5F_TCMA  : ORIGIN = 0x00000040, LENGTH = 0x00007FC0
    R5F_TCMB0 : ORIGIN = 0x41010000, LENGTH = 0x00008000
    MSRAM     : ORIGIN = 0x70080000, LENGTH = 0x00040000

    /* remoteproc loads the ELF here; Linux DTS carveout must cover this range */
    DDR_0     : ORIGIN = 0xA0100000, LENGTH = 0x00001000   /* resource_table */
    DDR_1     : ORIGIN = 0xA0101000, LENGTH = 0x00EFF000   /* code + data    */

    /* Linux VRING carveout — non-cached (MPU CONFIG_MPU_LINUX_IPC)
     * Defined in DTS as reserved-memory for r5fss0-0 remoteproc.
     * No linker section maps here; the region is reserved by the DTS. */
    LINUX_IPC_SHM_MEM       : ORIGIN = 0xA0000000, LENGTH = 0x00100000

    /* RTOS IPC shared memory — non-cached (MPU CONFIG_MPU_IPC_SHM) */
    USER_SHM_MEM            : ORIGIN = 0xA5000000, LENGTH = 0x00000080
    LOG_SHM_MEM             : ORIGIN = 0xA5000080, LENGTH = 0x00003F80
    RTOS_NORTOS_IPC_SHM_MEM : ORIGIN = 0xA5004000, LENGTH = 0x0000C000
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
    } > DDR_1

    GROUP {
        .data: {} palign(8)
    } > DDR_1

    GROUP {
        .bss:    {} palign(8)
        RUN_START(__BSS_START)
        RUN_END(__BSS_END)
        .sysmem: {} palign(8)
        .stack:  {} palign(8)
    } > DDR_1

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
    } > DDR_1

    GROUP {
        .ARM.exidx:  {} palign(8)
        .init_array: {} palign(8)
        .fini_array: {} palign(8)
    } > DDR_1

    /* IPC VRING sections — must be NOLOAD (remoteproc does not initialise) */
    .bss.user_shared_mem   (NOLOAD) : {} > USER_SHM_MEM
    .bss.log_shared_mem    (NOLOAD) : {} > LOG_SHM_MEM
    .bss.ipc_vring_mem     (NOLOAD) : {} > RTOS_NORTOS_IPC_SHM_MEM

    /* Resource table — remoteproc reads this to discover VRING addresses */
    GROUP {
        .resource_table: {} palign(4096)
    } > DDR_0
}
