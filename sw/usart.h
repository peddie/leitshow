/**
 \file usart.h
 \brief USART Driver
 \defgroup usart USART
 \ingroup driver

 \section filename_description Description
 Detailed description...

 \section filename_example Example
 Detailed usage examples:
 \code
       a = {...};      // Explanation
       foo(&my_bar);   // More explanation
       magic = puff(&the_dragon); // *Poof*
 \endcode
*/
#ifndef __SUPERCONDUCTOR_USART_H__
#define __SUPERCONDUCTOR_USART_H__

/*! \addtogroup usart
  @{
*/

/* Set up a USART peripheral (but not the clocks). */
// TODO: convert int to int16 or int32
void usart_init(int usart, int irq, int baudrate, int over8);
void usart_uninit(int usart, int irq);

/* Echo-server ISR for a given USART. */
uint8_t usart_respond_isr(int usart);

/*! @} */
#endif  /* __SUPERCONDUCTOR_USART_H__ */
