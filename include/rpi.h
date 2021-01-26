/********************************************************************
 * rpi.h
 *
 *  Header file that includes function stubs and definitions
 *  for RPi bare-metal option
 *
 *  January 8, 2021
 *
 *******************************************************************/

#ifndef __RPI_H__
#define __RPI_H__

#include    <stdint.h>
#include    <assert.h>

#include    "config.h"

/* Define emulator stubs
 */
#if (RPI_BARE_METAL)
    #define     disable()           rpi_disable()
    #define     enable()            rpi_enable()
    #define     emu_assert(exp)     rpi_assert_handler(exp)
#else
    #define     disable()
    #define     enable()
    #define     emu_assert(exp)     assert(exp)
#endif

/********************************************************************
 *  RPi bare meta module API
 */
void    rpi_disable(void);
void    rpi_enable(void);
void    rpi_assert_handler(int assert_exp);

#endif  /* __RPI_H__ */
