/* Linker script for Silicon Labs EFR32BG13P devices                      */
/*                                                                     */
/* This file is subject to the license terms as defined in ARM's       */
/* CMSIS END USER LICENSE AGREEMENT.pdf, governing the use of          */
/* Example Code.                                                       */
/*                                                                     */
/* Copyright 2017 Silicon Laboratories, Inc. http://www.silabs.com     */
/*                                                                     */
/* Version 5.4.0 */
/*                                                                     */

/*
 * Linker separates static libraries, activities and application code 
 * to their own sections of FLASH in order to minimise binary diffs
 * from minor code changes.
 *
 * This minimises the number of RPC's required to perform an OTA
 * firmware upgrade, at the cost of increasing the initial binary size.
 *
 * Static libraries are ordered in reverse likelyhood of being modified
 */

_nvm_size = 4 * 2K;

MEMORY
{
  FLASH (rx) : ORIGIN = 0x00000000, LENGTH = 512K - _nvm_size
  NVM3 (r)   : ORIGIN = 512K - _nvm_size, LENGTH = _nvm_size
  RAM (rwx)  : ORIGIN = 0x20000000, LENGTH = 64K
}

INCLUDE "EFR32_common.ld"
