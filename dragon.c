/********************************************************************
 * dragon.c
 *
 *  DRAGON DATA computer emulator, main module.
 *  With MC6809E CPU emulation.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdio.h>

#include    "config.h"
#include    "mem.h"
#include    "cpu.h"
#include    "rpi.h"
#include    "trace.h"

#include    "sam.h"
#include    "vdg.h"
#include    "pia.h"

/* -----------------------------------------
   Dragon 32 ROM image
----------------------------------------- */
#include    "dragon/dragon.h"

/* -----------------------------------------
   Module definition
----------------------------------------- */
#define     DRAGON_ROM_START        0x8000
#define     DRAGON_ROM_END          0xfeff

/* -----------------------------------------
   Module functions
----------------------------------------- */

/*------------------------------------------------
 * main()
 *
 */
int main(int argc, char *argv[])
{
    int             i;
    uint16_t        break_point = 0xffff;
    cpu_state_t     cpu_state;

    printf("Loading code... ");
    i = 0;
    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    mem_define_rom(DRAGON_ROM_START, DRAGON_ROM_END);

    sam_init();
    pia_init();
    vdg_init();

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    //break_point = 0xbbe5;

    /* Execution loop.
     * Run until breakpoint, and single step from there.
     */
    printf("Starting CPU.\n");
    cpu_reset(1);

    do
    {
        cpu_run();

        if ( cpu_get_state(&cpu_state) == CPU_RESET )
            cpu_reset(0);

        vdg_render();

        vdg_field_sync();
    }
    while ( cpu_state.pc != break_point );

    printf("Stopped at breakpoint.\n");

    while ( 1 )
    {
        cpu_get_state(&cpu_state);
        trace_print_registers(&cpu_state);

        trace_action();

        cpu_run();
    }

    return 0;
}
