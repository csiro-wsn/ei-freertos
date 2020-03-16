/* Linker script to place sections and symbol values. Should be used together
 * with other linker script that defines memory regions FLASH and RAM.
 * It references following symbols, which must be defined in code:
 *   Reset_Handler : Entry of reset handler
 *
 * It defines following symbols, which code can use without definition:
 *   __exidx_start
 *   __exidx_end
 *   __copy_table_start__
 *   __copy_table_end__
 *   __zero_table_start__
 *   __zero_table_end__
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
 *   __HeapLimit
 *   __StackLimit
 *   __StackTop
 *   __stack
 *   __Vectors_End
 *   __Vectors_Size
 */
ENTRY(Reset_Handler)

SECTIONS
{
  __flash_end__ = ORIGIN(FLASH) + LENGTH(FLASH);

  .text.vectors :
  {
    KEEP(*(.vectors))
    __Vectors_End = .;
    __Vectors_Size = __Vectors_End - __Vectors;
    __end__ = .;

    __buildinfo_start = .;
    KEEP(*(.buildinfo))

    KEEP(*(.gecko_configuration))
    KEEP(*(.xo_configuration)) 
    KEEP(*(.gatt_header))
    KEEP(*(.gatt_data))	
  } > FLASH

  .text.body ALIGN(0x400) :
  {
    *(.text .text.*)  
  } > FLASH
  
  .text.footer :
  {
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

  /* Read only data is aligned to a 1k block */
  .text.rodata ALIGN(0x400) :
  {
    *(.rodata .rodata*)
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

  /* NVM data is not part of the program image, and is therefore added after _etext is calculated 
   *
   * The (DSECT) is very important for reasons unknown to me 
   */
  .nvm3 (DSECT):
  {
    KEEP(*(.nvm3));
  } > NVM3

  /** 
  * Special purpose ram block for noinit variables 
  *  Placed at start of RAM for consistent locations across reprogs
  */
  .reserved_ram (NOLOAD) :
  {
      __reserved_ram_start__ = .;
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

  /* Check if FLASH usage exceeds FLASH size */
  ASSERT( LENGTH(FLASH) >= (__etext + SIZEOF(.data)), "FLASH memory overflowed !")
}
