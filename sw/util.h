/**
 \file util.h
 \brief General Utilities
 \defgroup util General Utilities
 \ingroup driver

 \copyright
 COSMOGIA PROPRIETARY
 (c) Cosmogia 2013
 All Rights Reserved.
 NOTICE:  All information contained herein is, and remains
 the property of Cosmogia Incorporated and its suppliers,
 if any.  The intellectual and technical concepts contained
 herein are proprietary to Cosmogia Incorporated
 and its suppliers and may be covered by U.S. and Foreign Patents,
 patents in process, and are protected by trade secret or copyright law.
 Dissemination of this information or reproduction of this material
 is strictly forbidden unless prior written permission is obtained
 from Cosmogia Incorporated.

 -------------------------------------------------------------------------*/

#ifndef __SUPERCONDUCTOR_UTIL_H__
#define __SUPERCONDUCTOR_UTIL_H__

#include <stdint.h>


// If you're looking for _delay_us() and _delay_ms() -- they've moved to timer.c
// to be more accurate.  The macros would give different results if a variable
// or a constant were passed in.

// Bit helper macros
#define CHECK_BIT(var,pos)  ((var) &  (1<<(pos)))
#define SET_BIT(var,pos)    ((var) |= (1<<(pos)))
#define CLEAR_BIT(var,pos)  ((var) &= (1<<(pos)))
#define TOGGLE_BIT(var,pos) ((var) ^= (1<<(pos)))
#define CHECK_BITMASK(var,mask) ((var) & (mask))
#define SET_BITMASK(var,mask) ((var) |= (mask))
#define CLEAR_BITMASK(var,mask) ((var) &= ~(mask))
#define TOGGLE_BITMASK(var,mask) ((var) ^= (mask))

// Helper macros
#define LOCSTRING "%s:%d -> %s(): "
#define LOCATION __FILE__, __LINE__, __func__
#ifdef DEBUG
#define eprintf(str, ...) _printf(LOCSTRING str, LOCATION, ## __VA_ARGS__)
#else
#define eprintf(str, ...)
#endif  /* DEBUG */

#ifdef DEBUG
#define dprintf(str, ...) _printf(str, ## __VA_ARGS__)
#else
#define dprintf(str, ...)
#endif  /* DEBUG */

#define check_reply_space(len, amount)          \
    if ((len) < (amount))                       \
      return NACK_REPLY_TOO_LONG;               \

#define check_length(len, amount)               \
  if ((len) < (amount))                         \
    return NACK_PAYLOAD_SHORT;                  \

/*! \addtogroup util
  @{
*/
int _printf(const char *format, ...);

void reboot(void);

/// Display reset reason to the screen
void display_reset_reason(void);
void clear_reset_reason(void);
uint32_t get_reset_reason(void);

/// Display bits of a register to the screen
void printbits(uint32_t n, uint8_t number_of_bits);
void print32bits(uint32_t n);                   ///< Display 32bit registers
void print16bits(uint16_t n);                   ///< Display 16bit registers
void print8bits(uint8_t n);                     ///< Display 8bit registers

/// Display ascii art representing a number
void print_ascii_bar(uint32_t n, uint32_t min, uint32_t max);

/// Print the Build 6 banner
void print_banner(void);

/// Prints a block of data in hex
void hexprint(const void * data, uint8_t length);

// Calculates a crc32 from an array of 32-bit numbers, using hardware.
// This just uses the libopencm3 functions crc_reset and crc_calculate_block.
uint32_t calc_crc(uint32_t* ptr, int len);

/*! @} */
#endif  /* __SUPERCONDUCTOR_UTIL_H__ */
