/* Linker script for Nordic Semiconductor nRF devices
 *
 * Version: Sourcery G++ 4.5-1
 * Support: https://support.codesourcery.com/GNUToolchain/
 *
 * Copyright (c) 2007, 2008, 2009, 2010 CodeSourcery, Inc.
 *
 * The authors hereby grant permission to use, copy, modify, distribute,
 * and license this software and its documentation for any purpose, provided
 * that existing copyright notices are retained in all copies and that this
 * notice is included verbatim in any distributions.  No written agreement,
 * license, or royalty fee is required for any of the authorized uses.
 * Modifications to this software may be copyrighted by their authors
 * and need not follow the licensing terms described here, provided that
 * the new terms are clearly indicated on the first page of each file where
 * they apply.
 */
OUTPUT_FORMAT ("elf32-littlearm", "elf32-bigarm", "elf32-littlearm")

/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __etext
 *   __data_start__
 *   __preinit_array_start
 *   __preinit_array_end
 *   __init_array_start
 *   __init_array_end
 *   __fini_array_start
 *   __fini_array_end
 *   __data_end__
 *   __bss_start__
 *   __bss_end__
 *   __end__
 *   end
 *   __HeapBase
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 */
ENTRY(Reset_Handler)

SECTIONS
{
    __flash_end__ = ORIGIN(FLASH) + LENGTH(FLASH);

    /* softdevice binary is added to this section in the startup assembly file */
    .softdevice_flash : 
    {
        KEEP(*(.softdevice*))
    } > SOFTDEVICE_FLASH

    .text.vectors :
    {
        KEEP(*(.isr_vector))
        __buildinfo_start = .;
        KEEP(*(.buildinfo))
    } > FLASH

    /* Library .text, ordered by likelihood of changing with application updates */
    .text.libraries :
    {
        *libMBEDTLS.a:*(.text .text.*)
        *libCMSIS.a:*(.text .text.*)
        *libCORE_FREERTOS.a:*(.text .text.*)
        *libNRF_SDK.a:*(.text .text.*)
        *libgcc.a:*(.text .text.*)
        *libg.a:*(.text .text.*)
        *libCORE_ARCH.a:*(.text .text.*)
        *libCORE_CSIRO.a:*(.text .text.*)
    } > FLASH

    .text :
    {
        *(.text*)

        KEEP(*(.init))
        KEEP(*(.fini))

        /* .ctors */
        *crtbegin.o(.ctors)
        *crtbegin?.o(.ctors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .ctors)
        *(SORT(.ctors.*))
        *(.ctors)

        /* .dtors */
        *crtbegin.o(.dtors)
        *crtbegin?.o(.dtors)
        *(EXCLUDE_FILE(*crtend?.o *crtend.o) .dtors)
        *(SORT(.dtors.*))
        *(.dtors)

        KEEP(*(.eh_frame*))
    } > FLASH

    /* Read only library data is aligned to a 4k block */
    .text.rodata_lib ALIGN(0x1000):
    {
        *libMBEDTLS.a:*(.rodata .rodata.*)
        *libCMSIS.a:*(.rodata .rodata.*)
        *libCORE_FREERTOS.a:*(.rodata .rodata.*)
        *libNRF_SDK.a:*(.rodata .rodata.*)
        *libgcc.a:*(.rodata .rodata.*)
        *libg.a:*(.rodata .rodata.*)
        *libCORE_ARCH.a:*(.rodata .rodata.*)
        *libCORE_CSIRO.a:*(.rodata .rodata.*)
    } > FLASH

    /* Remaining read-only data is aligned to 1k block */
    .text.rodata ALIGN(0x400):
    {
        *(.rodata*)
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    __exidx_start = .;
    .ARM.exidx :
    {
        *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    } > FLASH
    __exidx_end = .;

    __etext = .;

    /* Dummy section for NVM pages */
    .nvm (NOLOAD) :
    {
        KEEP(*(.nvm))
    } > NVM

    /* Empty section so that softdevice RAM shows up in our section table */
    .softdevice_ram (NOLOAD) :
    {
        . = . + LENGTH(SOFTDEVICE_RAM);
    } > SOFTDEVICE_RAM
    
    __softdevice_ram_end__ = ORIGIN(RAM);

    /** 
     * Special purpose ram block for errata mitigation and noinit variables 
     *  Placed at start of RAM as errata needs to reserve an entire RAM block,
     *  and the final ram block is 192kB, while the others are 8kB.
     */
    .reserved_ram (NOLOAD) :
    {
        . = ALIGN(0x2000);
        __reserved_ram_start__ = .;
        *(.errata*)
        . = ALIGN(0x2000);
        *(.noinit*)
        __reserved_ram_end__ = .;
    } > RAM

    .data : AT (__etext)
    {
        __data_start__ = .;
        *(vtable)
        *(.data*)

        /* RAM Functions */
        . = ALIGN(4);
        *(.ram_function)

        . = ALIGN(4);
        /* preinit data */
        PROVIDE_HIDDEN (__preinit_array_start = .);
        KEEP(*(.preinit_array))
        PROVIDE_HIDDEN (__preinit_array_end = .);

        . = ALIGN(4);
        /* init data */
        PROVIDE_HIDDEN (__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array))
        PROVIDE_HIDDEN (__init_array_end = .);


        . = ALIGN(4);
        /* finit data */
        PROVIDE_HIDDEN (__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array))
        PROVIDE_HIDDEN (__fini_array_end = .);

        /* .fs_data ? */
        PROVIDE(__start_fs_data = .);
        KEEP(*(.fs_data))
        PROVIDE(__stop_fs_data = .);

        KEEP(*(.jcr*))
        . = ALIGN(4);
        /* All data end */
        __data_end__ = .;

    } > RAM

    .bss :
    {
        . = ALIGN(4);
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
    
    . = ALIGN(16);
    __ram_end__ = .;

    /* For validating flash contents, store the end of the program image 
    * The program image consists of both the application code (.text) and constants (.data)
    */
    __rom_end__ = __etext + SIZEOF(.data);

    /* At this point we want:
    *        A stack of defined size at the end of RAM
    *        A heap that fills the space between the end of .bss and the start of the stack
    * 
    * The maximally sized heap will be the space used for the FreeRTOS heap via heap_1.c
    * We will reserve space for the stack via the .stack_dummy section
    * No space will be reserved for the heap, the amount of unused ram reported is therefore the heap size
    */

    /* This section contains no symbols, only used for size calculations */
    .stack_dummy (COPY):
    {
        KEEP(*(.stack*))
    } > RAM
    
    /* Assign locations for stack and heap */
    __StackTop = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    __HeapBase = __ram_end__;
    __HeapLimit = __StackLimit - 8; /* Heap finishes 8 bytes before stack starts */
    __HeapSize = __HeapLimit - __HeapBase;

    PROVIDE(__stack = __StackTop);

    /* Check if data + heap + stack exceeds RAM limit */
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
    
    /* Check if text sections + data exceeds FLASH limit */
    DataInitFlashUsed = __bss_start__ - __data_start__;
    CodeFlashUsed = __etext - ORIGIN(FLASH);
    TotalFlashUsed = CodeFlashUsed + DataInitFlashUsed;
    ASSERT(TotalFlashUsed <= LENGTH(FLASH), "region FLASH overflowed with .data and user data")

}
