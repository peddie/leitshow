/**
 \file timer.h
 \author Michael Aherne (michael.aherne@cosmogia.com)
 \brief Timing Functions
 See the \ref timer_driver "Collaboration Module" for more details.
 \defgroup timer_driver Timer
 \ingroup driver

 \section timer_description Description
 This driver initializes some timers.  Functions are
 provided, using these timers, to measure execution time and schedule callback
 functions.

 \section timer_example Examples
 \subsection ticktock_example Tick Tock Example
  Simple Example:
 \code
       uint32_t t,measure1,measure2;
       t = tick();
       // ... functions to measure ...
       measure1 = tock(t);      // Measures from tick() to here
       // ... more functions ...
       measure2 = tock(t);      // Measures from tick() to here
 \endcode
  Nested Example:
 \code
       uint32_t t1,t2,measure1,measure2,measureTotal;
       t1 = tick();
       // ... functions to measure ...
       measure1 = tock(t1);      // Measures from t1 = tick() to here
       t2 = tick();
       // ... more functions ...
       measure2 = tock(t2);      // Measures from t2 = tick() to here
       measureTotal = tock(t1);  // Measure from t1 = tick() to here
 \endcode
 \subsection aync_example Asynchornous Callback example
 \code
static void callback_function(void) {
  eprintf("In callback function\n");
}

static void async(void){
  eprintf("Do stuff...\n");
  schedule_callback(&callback_function,3000000);   // Schedules callback function for 3 seconds in the future
  eprintf("Do some more stuff...\n");
  _delay_ms(3000);    // Callback will occur some time in here
}
 \endcode

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
*/

/*! \addtogroup timer_driver
  @{
*/

#ifndef __SUPERCONDUCTOR_TIMER_H__
#define __SUPERCONDUCTOR_TIMER_H__

#include <stdint.h>


/// Setup the clocks and interrupts
void timer_setup(void);

/** \brief Begin execution time measurement

    \details This function begins a stopwatch which can be used to measure
            execution time with high precision. See the \ref timer_example
            "Examples Section" for an example.

    \returns A uint32_t which is the starting tick of the stopwatch.  This
             value should be passed to tock() or tock_us().

    \warning Results will be inaccurate if used for measuring times greater than
             the overflow time of the timer.  **Currently this limit is 51
             seconds.**

    \see tock(), tock_us()
*/
uint32_t tick(void);

/** \brief Measure execution time in ticks

    \details This function measures the time between the associated tick()
            function call and itself.  See the \ref timer_example "Examples
            Section" for an example.

    \param[in] ticknumber A number describing the starting time from which to
                          measure.  Use the output of tick() as the input here.

    \returns The number of **ticks** since the associated ticktime.

    \warning Results will be inaccurate if used for measuring times greater than
             the overflow time of the timer.  **Currently this limit is 51
             seconds.**

    \see tick(), tock_us()
*/
uint32_t tock(uint32_t ticknumber);

/** \brief Measure execution time in microseconds

    \details This function measures the time between the associated tick()
            function call and itself.  See the \ref timer_example "Examples
            Section" for an example.

    \param[in] ticknumber A number describing the starting time from which to
                          measure.  Use the output of tick() as the input here.

    \returns The number of **microseconds** since the associated ticktime.

    \warning Results will be inaccurate if used for measuring times greater than
             the overflow time of the timer.  **Currently this limit is 51
             seconds.**

    \see tick(), tock()
*/
uint32_t tock_us(uint32_t ticknumber);

/** \brief Schedule a callback function asynchronously

    \details This function schedules a timer to countdown for the number
            of microseconds given.  At the expiration of the timer,
            the given callback function will be executed.

    \param[in] fn A pointer to a function.  The function must take no arguments,
                  and any returns will be discarded.
    \param[in] uS The number of microseconds to delay.

    \return 0 for success, nonzero for failure.

    \warning **The maximum delay is 25 seconds.** This is a function
              of the microcontroller's clock rate, the prescaler and the size of
              the timer register -- it is determined experimentally. */
int8_t schedule_callback(void * fn, uint32_t uS);


void _delay_us(uint32_t uS);
void _delay_ms(uint32_t ms);


/*! @} */
#endif  /* __SUPERCONDUCTOR_TIMER_H__ */
