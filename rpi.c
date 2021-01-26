/********************************************************************
 * rpi.c
 *
 *  Function stubs and definitions for RPi bare-metal option
 *
 *  January 8, 2021
 *
 *******************************************************************/

#include    "rpi.h"

/*------------------------------------------------
 * rpi_disable()
 *
 *  Disable interrupts
 *
 *  param:  None
 *  return: None
 */
void rpi_disable(void)
{
}

/*------------------------------------------------
 * rpi_enable()
 *
 *  Enable interrupts
 *
 *  param:  None
 *  return: None
 */
void rpi_enable(void)
{
}

/*------------------------------------------------
 * rpi_assert_handler()
 *
 *  Assert handler for RPi bare metal mode.
 *  Functionality is same as assert() macro.
 *
 *  param:  Assert expression to evaluate
 *  return: None
 */
void rpi_assert_handler(int assert_exp)
{
    if ( assert_exp )
    {
        // TODO implement assert behavior
    }
}
