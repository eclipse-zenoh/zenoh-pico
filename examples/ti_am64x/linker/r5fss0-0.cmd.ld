/*
 * Linker command file for TI AM64x r5fss0-0 core — FreeRTOS application.
 *
 * Memory map matches TI MCU+ SDK networking examples for am64x-evm r5fss0-0.
 * Adapted for zenoh-pico examples (code+data in DDR, boot code in MSRAM).
 *
 * MCU_PLUS_SDK references:
 *   source/networking/enet/core/examples/lwip/enet_cpsw_tcpserver/
 *       am64x-evm/r5fss0-0_freertos/ti-arm-clang/linker.cmd
 */

/* Main() startup stack (used until vTaskStartScheduler; then per-task stacks take over) */
--stack_size=16384

/* FreeRTOS pvPortMalloc heap — lwIP pbufs and internal FreeRTOS objects come from here.
 * The zenoh-pico pool allocator handles zenoh memory separately from .bss. */
--heap_size=65536

/* Entry point: interrupt vector table at address 0x0 */
-e_vectors

/* Extra tiarmclang linker performance flags */
--zero_init=on
--use_memset=fast
--use_memcpy=fast

/* R5F exception-mode stack sizes */
__IRQ_STACK_SIZE       = 4096;
__FIQ_STACK_SIZE       = 256;
__SVC_STACK_SIZE       = 4096;   /* FreeRTOS uses SVC mode for task execution */
__ABORT_STACK_SIZE     = 256;
__UNDEFINED_STACK_SIZE = 256;

MEMORY
{
    /* Interrupt vector table — MUST start at 0x0 */
    R5F_VECS  : ORIGIN = 0x00000000, LENGTH = 0x00000040

    /* Tightly-coupled memory A — boot/MPU code before DDR is accessible */
    R5F_TCMA  : ORIGIN = 0x00000040, LENGTH = 0x00007FC0

    /* Tightly-coupled memory B0 */
    R5F_TCMB0 : ORIGIN = 0x41010000, LENGTH = 0x00008000

    /* MSRAM — boot code that must run before DDR mapping is set up (11 KB) */
    MSRAM     : ORIGIN = 0x70080000, LENGTH = 0x00002F00

    /* DDR — 14 MB window for this core (code + data + zenoh pool ~96 KB + lwIP heap) */
    DDR       : ORIGIN = 0x80000000, LENGTH = 0x00E00000
}

SECTIONS
{
    /* Vector table — must be at address 0x0 */
    .vectors: {} palign(8) > R5F_VECS

    /* Boot/cache/MPU code must stay below 0x80000000 (before DDR is mapped) */
    GROUP {
        .text.hwi:   palign(8)
        .text.cache: palign(8)
        .text.mpu:   palign(8)
        .text.boot:  palign(8)
        .text:abort: palign(8)
    } > MSRAM

    /* Application code and read-only data */
    GROUP {
        .text:   {} palign(8)
        .rodata: {} palign(8)
    } > DDR

    /* Initialized globals and statics */
    GROUP {
        .data: {} palign(8)
    } > DDR

    /* Uninitialized data, heap, and main() stack */
    GROUP {
        .bss:    {} palign(8)
        RUN_START(__BSS_START)
        RUN_END(__BSS_END)
        .sysmem: {} palign(8)   /* pvPortMalloc heap */
        .stack:  {} palign(8)   /* main() startup stack */
    } > DDR

    /* Exception-mode stacks */
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
    } > DDR

    /* C++ support sections (kept even for C projects for library compatibility) */
    GROUP {
        .ARM.exidx:  {} palign(8)
        .init_array: {} palign(8)
        .fini_array: {} palign(8)
    } > DDR
}
