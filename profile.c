/********************************************************************
 * profile.c
 *
 *  MC6809E CPU emulation for runing timing profile tests.
 *  Requires the BCM2835 GPIO C library.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdio.h>
#include    <time.h>

#include    "config.h"
#include    "mem.h"
#include    "cpu.h"
#include    "rpi.h"
#include    "trace.h"
#include    "bcm2835.h"

/* -----------------------------------------
   Include file for MC6909E test code
----------------------------------------- */
#include    "test/profile.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     IO_ADDR             0xf000
#define     RPI_GPIO            RPI_GPIO_P1_24

/* -----------------------------------------
   Module functions
----------------------------------------- */
uint8_t io_handler(uint16_t address, uint8_t data, mem_operation_t op);

/*------------------------------------------------
 * main()
 *
 */
int main(int argc, char *argv[])
{
    int             i;
    uint16_t        break_point = 0xffff;
    cpu_state_t     cpu_state;

    /* Initialize GPIO subsystem
     */
    if ( !bcm2835_init() )
    {
        printf("bcm2835_init failed. Are you running as root?\n");
        return -1;
    }

    bcm2835_gpio_write(RPI_GPIO, LOW);
    bcm2835_gpio_fsel(RPI_GPIO, BCM2835_GPIO_FSEL_OUTP);

    printf("Initialized GPIO\n");

    printf("Loading code... ");
    i = 0;
    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    mem_define_io(IO_ADDR, IO_ADDR, io_handler);

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    //break_point = 0xf099;

    /* Execution loop.
     * Run until breakpoint, and single step from there.
     */
    printf("Starting CPU.\n");

    do
    {
        cpu_run();

        //trace_print_registers(&cpu_state);

        //trace_action();
   }
    while ( cpu_state.pc != break_point );

    printf("Stopped at breakpoint.\n");

    bcm2835_close();
    
    return 0;
}

/* -----------------------------------------
   IO handler call-back functions
----------------------------------------- */

/*------------------------------------------------
 * io_handler()
 *
 *  IO call-back handler that will set or reset a Raspberry Pi
 *  GPIO pin.
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
uint8_t io_handler(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( op == MEM_WRITE )
    {
        bcm2835_gpio_write(RPI_GPIO, data);
    }
    
    return data;
}

