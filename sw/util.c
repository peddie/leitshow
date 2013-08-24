/*
Copyright 2013 Cosmogia
*/

#include <stdio.h>
#include <errno.h>
#include <libopencm3/stm32/f4/rcc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/f4/usart.h>
#include <libopencm3/stm32/f4/crc.h>

#include <stdarg.h>   // for printf
#include <string.h>   // for printf
#include <assert.h>

#include "./usart.h"
#include "./esp_io.h"
#include "./messages/command.h"

#include "../apps/debug.h"

#include "./util.h"

inline void
reboot(void) {
  scb_reset_system();
}

// Like printf, except it adds ESP framing and Message Header
int _printf(const char *format, ...) {
  char dbgbuf[MESSAGE_MAX_PAYLOAD + MESSAGE_HEADER_SIZE];
  va_list arg;
  int done;

  // Create a header for the packet we're going to send
  message_header hdr;
  hdr.hwid = 0;
  hdr.seqnum = 0;
  hdr.type = CMD_DEBUG;

  // Copy the header into a buffer
  memcpy(dbgbuf, &hdr, MESSAGE_HEADER_SIZE);
  dbgbuf[MESSAGE_HEADER_SIZE] = ASCII;  // DEBUG command ASCII
  #define HEADER_AND_COMMAND_SIZE   MESSAGE_HEADER_SIZE + 1

  // Add the message to the buffer
  va_start(arg, format);
  done = vsprintf(&dbgbuf[HEADER_AND_COMMAND_SIZE], format, arg);
  va_end(arg);

  // Ship it out the door
  esp_packet_write(DEBUG_FD, strlen(&dbgbuf[HEADER_AND_COMMAND_SIZE]) +
                   HEADER_AND_COMMAND_SIZE, (const uint8_t *)dbgbuf);

  #undef HEADER_AND_COMMAND_SIZE
  return done;
}

void clear_reset_reason(void) {
  RCC_CSR |= RCC_CSR_RMVF;
}

uint32_t get_reset_reason(void) {
  return RCC_CSR;
}

#ifdef DEBUG
/* Print n as a binary number */
#ifdef DEBUG
void printbits(uint32_t n, uint8_t bits) {
  unsigned int step;
  // Loop over bits, printing as we go
  step = 4;

  // A buffer for collecting the 0s and 1s before printing as a group
  char output[MESSAGE_MAX_PAYLOAD];

  int j = 0;
  for (int i = bits - 1; i >= 0; i--) {
    if (CHECK_BIT(n, i)) {
      // dprintf("1");
      output[j]='1';
    } else {
      // dprintf("0");
      output[j]='0';
    }
    j++;
    if (i % step == 0) {
      // dprintf(" ");
      output[j]=' ';
      j++;
    }
    assert(j < MESSAGE_MAX_PAYLOAD - 2);
  }
  output[j++]='\n';
  output[j++]='\0';
  dprintf("%s", output);
}

void print32bits(uint32_t n) {
  printbits(n, 32);
}
void print16bits(uint16_t n) {
  printbits(n, 16);
}
void print8bits(uint8_t n)  {
  printbits(n, 8);
}
#endif  /* DEBUG */

// Display thte reset reason
void display_reset_reason(void) {
  dprintf("Reset Flag:");
#ifdef DEBUG
  printbits(RCC_CSR, 32);
#endif  /* DEBUG */
  dprintf("\n");
  dprintf("Reset due to: \n");
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_LPWRRSTF)) {
    dprintf("---> RCC_CSR_LPWRRSTF -- Low Power Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_WWDGRSTF)) {
    dprintf("---> RCC_CSR_WWDGRSTF -- Windowed Watchdog Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_IWDGRSTF)) {
    dprintf("---> RCC_CSR_IWDGRSTF -- Independent Watchdog Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_SFTRSTF)) {
    dprintf("---> RCC_CSR_SFTRSTF -- Software Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_PORRSTF)) {
    dprintf("---> RCC_CSR_PORRSTF -- POR/PDR Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_PINRSTF)) {
    dprintf("---> RCC_CSR_PINRSTF -- NRST PIN Reset Flag\n");
  }
  if (CHECK_BITMASK(RCC_CSR, RCC_CSR_BORRSTF)) {
    dprintf("---> RCC_CSR_BORRSTF -- Brown Out Reset Flag\n");
  }
}



void print_ascii_bar(uint32_t n, uint32_t min, uint32_t max) {
  dprintf("|");
  uint8_t mx = 40;
  uint8_t val;
  float scale;
  scale = (float)n / (float)(max - min);
  // dprintf("%f ",scale);
  val = (uint8_t)(scale * mx);
  for (uint8_t i = 0; i < mx; i++) {
    if (i < val)
      {dprintf("-");}
    else
      if (i == val)
        {dprintf("+");}
      else
        if (i > val)
          {dprintf(" ");}
  }
  dprintf("|");
}

void print_banner() {
  dprintf("\r\n\r\n");
  dprintf(".____/\\/\\/\\______________/\\\\__/\\________/\\______/\\/\\/\\/\\/\\___\r\n");
  dprintf(".___/\\____/\\__/\\____/\\______/\\____/\\/\\/\\______________/\\____\r\n");
  dprintf(".__/\\/\\/\\____/\\____/\\__/\\__/\\__/\\____/\\____________/\\_______\r\n");
  dprintf("._/\\____/\\__/\\____/\\__/\\__/\\__/\\____/\\__________/\\__________\r\n");
  dprintf("./\\/\\/\\______/\\/\\/\\__/\\__/\\____/\\/\\/\\________/\\_____________\r\n");
  dprintf("\r\n\r\n");
}






void hexprint(const void * data, uint8_t length) {
  const uint8_t * ptr = data;
  char temp[256];
  if (ptr == NULL) return;
  for (uint8_t i = 0; i < length; i++) {
    snprintf(&temp[i*5], sizeof(temp), "0x%02x ", ptr[i]);
  }
  dprintf("%s\n", temp);
}

#else
void print_banner() {}
void display_reset_reason() {}
#endif  /* DEBUG */

uint32_t
calc_crc(uint32_t* ptr, int len) {
  crc_reset();
  uint32_t crc = crc_calculate_block(ptr, len);
  return crc;
}
