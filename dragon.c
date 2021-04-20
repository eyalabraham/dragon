/********************************************************************
 * dragon.c
 *
 *  DRAGON DATA computer emulator, main module.
 *  With MC6809E CPU emulation.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    "printf.h"

#include    "mem.h"
#include    "cpu.h"
#include    "rpi.h"

#include    "sam.h"
#include    "vdg.h"
#include    "pia.h"

/* -----------------------------------------
   Dragon 32 ROM image
----------------------------------------- */
#include    "dragon/dragon.h"
#include    "dragon/doodle-bug.h"

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
#if (RPI_BARE_METAL==1)
    void kernel(uint32_t r0, uint32_t machid, uint32_t atags)
#else
    int main(int argc, char *argv[])
#endif
{
    int             i;

    /* ROM code load
     */
    printf("Loading code... ");
    i = 0;
    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    /* Cartridge ROM load as demo for a game
     * TODO remove when loader/manager from SD card is available.
     */
    printf("Loading cartridge... ");
    i = 0;
    while ( cartridge[i] != -1 )
    {
        mem_write(i + CART_LOAD_ADDRESS, cartridge[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    mem_define_rom(DRAGON_ROM_START, DRAGON_ROM_END);

    /* Emulation initialization
     */
    if ( rpi_gpio_init() == -1 )
        rpi_halt("Failed to initialize GPIO rpi_gpio_init()");;

    sam_init();
    pia_init();
    vdg_init();

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    /* CPU endless execution loop.
     */
    printf("Starting CPU.\n");
    cpu_reset(1);

    for (;;)
    {
        cpu_run();

        if ( rpi_reset_button() == 0 )  // Active low reset
            cpu_reset(1);
        else
            cpu_reset(0);

        vdg_render();

        pia_vsync_irq();
    }

#if (RPI_BARE_METAL==0)
    return 0;
#endif
}
