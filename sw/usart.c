/*
Copyright 2013 Cosmogia
*/
#include <libopencm3/stm32/f4/gpio.h>
#include <libopencm3/stm32/f4/usart.h>
#include <libopencm3/stm32/f4/nvic.h>

#include "./usart.h"

void
usart_init(int usart, int irq, int baudrate, int over8) {
  /* Setup USART parameters. */
  nvic_disable_irq(irq);
  usart_disable_rx_interrupt(usart);
  usart_disable_tx_interrupt(usart);
  usart_disable(usart);
  USART_CR1(usart) |= over8;  /* This doubles the listed baudrate. */
  usart_set_baudrate(usart, baudrate);
  usart_set_databits(usart, 8);
  usart_set_stopbits(usart, USART_STOPBITS_1);
  usart_set_mode(usart, USART_MODE_TX_RX);
  usart_set_parity(usart, USART_PARITY_NONE);
  usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);
  /* Finally enable the USART. */
  usart_enable(usart);
  usart_enable_rx_interrupt(usart);
  usart_enable_tx_interrupt(usart);
  nvic_enable_irq(irq);
}

void
usart_uninit(int usart, int irq) {
  /* Disable the IRQ. */
  nvic_disable_irq(irq);

  /* Disable the USART. */
  usart_disable(usart);

  /* Disable usart Receive interrupt. */
  USART_CR1(usart) &= ~USART_CR1_RXNEIE;
}

inline uint8_t
usart_respond_isr(int usart) {
  static uint8_t data = 'A';
  /* Check if we were called because of RXNE. */
  if (((USART_CR1(usart) & USART_CR1_RXNEIE) != 0) &&
      ((USART_SR(usart) & USART_SR_RXNE) != 0)) {
    /* Retrieve the data from the peripheral. */
    data = usart_recv(usart);
    /* Enable transmit interrupt so it sends back the data. */
    USART_CR1(usart) |= USART_CR1_TXEIE;
  }
  /* Check if we were called because of TXE. */
  if (((USART_CR1(usart) & USART_CR1_TXEIE) != 0) &&
      ((USART_SR(usart) & USART_SR_TXE) != 0)) {
    /* Put data into the transmit register. */
    usart_send(usart, data);
    /* Disable the TXE interrupt as we don't need it anymore. */
    USART_CR1(usart) &= ~USART_CR1_TXEIE;
  }
  return data;
}
