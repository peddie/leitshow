/**
 \file pwm.h
 \brief Pulse Width Modulation
 See the \ref pwm_driver "Collaboration Module" for more details.
 \defgroup pwm_driver PWM
 \ingroup driver

 \section pwm_description Description
 Detailed description...

 \section pwm_example Example Code
 \code
       // tbd...
 \endcode
*/

#ifndef __PWM_H__
#define __PWM_H__

/*! \addtogroup pwm_driver
  @{
*/

#include <stdint.h>


/// Setup the pwm pins
void pwm_setup(void);

/** pwm_set_duty()
    \brief Set the duty cycle of a particular timer

    \param[in] timer TIMx, defined by the libopencm library
    \param[in] channel The timer channel, an enum defined by libopencm3
    \param[in] dp The duty cycle percentage. From 0.0 to 1.0.
*/
void pwm_set_duty(uint32_t timer, uint8_t channel, float dp);


/*! @} */
#endif  /* __PWM_H__ */
