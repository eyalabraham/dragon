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

#define     DB_ERR          0
#define     DB_INFO         1

/********************************************************************
 *  RPi bare meta module API
 */
int      rpi_gpio_init(void);

uint8_t *rpi_fb_init(int h, int v);
uint8_t *rpi_fb_resolution(int h, int v);

uint32_t rpi_system_timer(void);

int      rpi_keyboard_read(void);
void     rpi_keyboard_reset(void);

int      rpi_joystk_comp(void);
int      rpi_rjoystk_button(void);

int      rpi_reset_button(void);

void     rpi_audio_mux_set(int);

void     rpi_write_dac(int);

void     rpi_disable(void);
void     rpi_enable(void);

void     rpi_testpoint_on(void);
void     rpi_testpoint_off(void);

void     rpi_assert_handler(int assert_exp);

#endif  /* __RPI_H__ */
