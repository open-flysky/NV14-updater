/*
 * This file is part of the Black Magic Debug project.
 *
 * Copyright (C) 2011  Black Sphere Technologies Ltd.
 * Written by Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __STM32MEM_H
#define __STM32MEM_H

#include <libusb-1.0/libusb.h>
#include "dfu.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define STM32_FLASH_OFFSET 0x08000000
#define STM32_FIRST_SECTOR_SIZE 0x4000
extern int fwUpdateProgress;

typedef enum {
  mem_st_sector0 = 0,
  mem_st_sector1,
  mem_st_sector2,
  mem_st_sector3,
  mem_st_sector4,
  mem_st_sector5,
  mem_st_sector6,
  mem_st_sector7,
  mem_st_sector8,
  mem_st_sector9,
  mem_st_sector10,
  mem_st_sector11,
  mem_st_system,
  mem_st_otp_area,
  mem_st_option_bytes,
  mem_st_all,
} stm32_mem_sectors;

static const uint32_t stm32_sector_addresses[] = {
  0x08000000,   /* sector  0,  16 kb */
  0x08004000,   /* sector  1,  16 kb */
  0x08008000,   /* sector  2,  16 kb */
  0x0800C000,   /* sector  3,  16 kb */
  0x08010000,   /* sector  4,  64 kb */
  0x08020000,   /* sector  5, 128 kb */
  0x08040000,   /* sector  6, 128 kb */
  0x08060000,   /* sector  7, 128 kb */
  0x08080000,   /* sector  8, 128 kb */
  0x080A0000,   /* sector  9, 128 kb */
  0x080C0000,   /* sector 10, 128 kb */
  0x080E0000,   /* sector 11, 128 kb */
  0x08100000,   /* sector 12,  16 kb */
  0x08104000,   /* sector 13,  16 kb */
  0x08108000,   /* sector 14,  16 kb */
  0x0810C000,   /* sector 15,  16 kb */
  0x08110000,   /* sector 16,  64 kb */
  0x08120000,   /* sector 17, 128 kb */
  0x08140000,   /* sector 18, 128 kb */
  0x08160000,   /* sector 19, 128 kb */
  0x08180000,   /* sector 20, 128 kb */
  0x081A0000,   /* sector 21, 128 kb */
  0x081C0000,   /* sector 22, 128 kb */
  0x081E0000,    /* sector 23, 128 kb */
  0x08200000,   /* sector 24,  16 kb */
};

enum return_codes_enum {
    SUCCESS = 0,
    UNSPECIFIED_ERROR,                  /* general error */
    ARGUMENT_ERROR,                     /* invalid command for target etc. */
    DEVICE_ACCESS_ERROR,                /* security bit etc. */
    BUFFER_INIT_ERROR,                  /* hex files problems etc. */
    FLASH_READ_ERROR,
    FLASH_WRITE_ERROR,
    VALIDATION_ERROR_IN_REGION,
    VALIDATION_ERROR_OUTSIDE_REGION };


#define STM32_MEM_UNIT_NAMES "Sector 0", "Sector 1", "Sector 2", "Sector 3", \
  "Sector 4", "Sector 5", "Sector 6", "Sector 7", "Sector 8", "Sector 9", \
  "Sector 10", "Sector 11", "System Memory", "OTP Area", "Option Bytes", "all"

#define STM32_READ_PROT_ERROR   -10


DFUUTILS_API int32_t stm32_erase_flash( dfu_device_t *device, dfu_bool quiet );
  /*  mass erase flash
   *  device  - the usb_dev_handle to communicate with
   *  returns status DFU_STATUS_OK if ok, anything else on error
   */

DFUUTILS_API int32_t stm32_page_erase( dfu_device_t *device, uint32_t address,
    dfu_bool quiet );
  /* erase a page of memory (provide the page address) */

DFUUTILS_API int32_t stm32_start_app( dfu_device_t *device, dfu_bool quiet,  uint32_t address);
  /* Reset the registers to default reset values and start application
   */
DFUUTILS_API int32_t stm32_read_block( dfu_device_t *device, size_t xfer_len, uint8_t *buffer );
DFUUTILS_API int32_t stm32_read_flash( dfu_device_t *device,
              intel_buffer_in_t *buin,
              uint8_t mem_segment,
              const dfu_bool quiet);
  /* read the flash from buin->info.data_start to data_end and place
   * in buin.data. mem_segment is the segment of memory from the
   * stm32_memory_unit_enum.
   */

DFUUTILS_API int32_t stm32_set_address_ptr( dfu_device_t *device, uint32_t address );
DFUUTILS_API int32_t stm32_write_flash( dfu_device_t *device, intel_buffer_out_t *bout,
    const dfu_bool eeprom, const dfu_bool force, const dfu_bool hide_progress );
  /* Flash data from the buffer to the main program memory on the device.
   * buffer contains the data to flash where buffer[0] is aligned with memory
   * address zero (which could be inside the bootloader and unavailable).
   * buffer[start / end] correspond to the start / end of available memory
   * outside the bootloader.
   * flash_page_size is the size of flash pages - used for alignment
   * eeprom bool tells if you want to flash to eeprom or flash memory
   * hide_progress bool sets whether to display progress
   */
DFUUTILS_API int32_t stm32_write_block( dfu_device_t *device, size_t xfer_len, uint8_t *buffer );
DFUUTILS_API int32_t stm32_get_commands( dfu_device_t *device );
  /* @brief get the commands list, should be length 4
   * @param device pointer
   * @retrn 0 on success
   */

DFUUTILS_API int32_t stm32_get_configuration( dfu_device_t *device );
  /* @brief get the configuration structure
   * @param device pointer
   * @retrn 0 on success, negative for error
   */

DFUUTILS_API int32_t stm32_read_unprotect( dfu_device_t *device, dfu_bool quiet );
  /* @brief unprotect the device (triggers a mass erase)
   * @param device pointer
   * @retrn 0 on success
   */


#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif

