/********************************************************************
 * rpi.c
 *
 *  Function stubs and definitions for RPi machine-defendant functionality
 *  and implementation for OS or bare-metal option.
 *
 *  January 8, 2021
 *
 *******************************************************************/

#include    <stdio.h>
#include    <unistd.h>
#include    <time.h>
#include    <fcntl.h>
#include    <linux/fb.h>
#include    <linux/kd.h>
#include    <sys/mman.h>
#include    <sys/ioctl.h>

#include    "bcm2835.h"
#include    "rpi.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     AVR_RESET       RPI_GPIO_P1_15
#define     PRI_TEST_POINT  RPI_GPIO_P1_16

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static uint8_t *fb_set_resolution(int fbh, int x_pix, int y_pix);
static int fb_set_tty(const int mode);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static int      fbfd = 0;                        // frame buffer file descriptor

/********************************************************************
 * rpi_fb_init()
 *
 *  Initialize the RPi frame buffer device.
 *
 *  param:  None
 *  return: pointer to frame buffer, or 0 if no error,
 */
uint8_t *rpi_fb_init(int x_pix, int y_pix)
#if (RPI_BARE_METAL)
{
    return 0L;
} /* end of RPI_BARE_METAL */
#else
{
    uint8_t *fbp = 0L;

    // Open the frame buffer device file for reading and writing
    if ( fbfd == 0 )
    {
        fbfd = open("/dev/fb0", O_RDWR);
        if (fbfd == -1)
        {
            printf("%s: Cannot open frame buffer /dev/fb0\n", __FUNCTION__);
            return 0L;
        }
    }

    printf("Frame buffer device is open\n");

    fbp = fb_set_resolution(fbfd, x_pix, y_pix);

    if ( (int)fbp == -1 )
    {
        printf("%s: Failed to mmap()\n", __FUNCTION__);
        return 0L;
    }

    // Select graphics mode
    if ( fb_set_tty(1) )
    {
        printf("%s: Could not set tty0 mode.\n", __FUNCTION__);
        return 0L;
    }

    return fbp;
}
#endif  /* end of not RPI_BARE_METAL */

/********************************************************************
 * rpi_fb_resolution()
 *
 *  Change the RPi frame buffer resolution.
 *  Frame buffer must be alerady initialized with rpi_fb_init()
 *
 *  param:  None
 *  return: Pointer to frame buffer, or 0 if no error,
 */
uint8_t *rpi_fb_resolution(int x_pix, int y_pix)
#if (RPI_BARE_METAL)
{
    return 0L;
} /* end of RPI_BARE_METAL */
#else
{
    uint8_t *fbp = 0L;

    fbp = fb_set_resolution(fbfd, x_pix, y_pix);

    if ( (int)fbp == -1 )
    {
        printf("%s: Failed to mmap()\n", __FUNCTION__);
        return 0L;
    }

    return fbp;
}
#endif  /* end of not RPI_BARE_METAL */

/*------------------------------------------------
 * rpi_system_timer()
 *
 *  Return running system timer time stamp
 *
 *  param:  None
 *  return: System timer value
 */
uint32_t rpi_system_timer(void)
#if (RPI_BARE_METAL)
{
    return 0L;
} /* end of RPI_BARE_METAL */
#else
{
    return (uint32_t) clock();
}
#endif  /* end of not RPI_BARE_METAL */

/*------------------------------------------------
 * rpi_keyboard_init()
 *
 *  Initialize serial interface to AVR (PS2 keyboard controller)
 *
 *  param:  None
 *  return: -1 fail, 0 ok
 */
int rpi_keyboard_init(void)
#if (RPI_BARE_METAL)
{
    return 0L;
} /* end of RPI_BARE_METAL */
#else
{
    if (!bcm2835_init())
    {
      printf("bcm2835_init failed. Are you running as root??\n");
      return -1;
    }

    if (!bcm2835_spi_begin())
    {
      printf("bcm2835_spi_begin failed. Are you running as root??\n");
      return -1;
    }

    /* Initialize GPIO for AVR reset line
     */
    bcm2835_gpio_fsel(AVR_RESET, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(AVR_RESET, HIGH);

    rpi_keyboard_reset();
    sleep(3);

    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_32768);
    //bcm2835_spi_chipSelect(BCM2835_SPI_CS0);
    //bcm2835_spi_setChipSelectPolarity(BCM2835_SPI_CS0, LOW);

    /* Initialize GPIO for RPi test point
     * TODO Find a better place to do this
     */
    bcm2835_gpio_fsel(PRI_TEST_POINT, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(PRI_TEST_POINT, LOW);

    return 0;
}
#endif  /* end of not RPI_BARE_METAL */

/*------------------------------------------------
 * rpi_keyboard_read()
 *
 *  Read serial interface from AVR (PS2 keyboard controller)
 *
 *  param:  None
 *  return: Key code
 */
int rpi_keyboard_read(void)
#if (RPI_BARE_METAL)
{
    return 0L;
} /* end of RPI_BARE_METAL */
#else
{
    return (int)bcm2835_spi_transfer(0);
}
#endif  /* end of not RPI_BARE_METAL */

/*------------------------------------------------
 * rpi_keyboard_reset()
 *
 *  Reset keyboard AVR interface
 *
 *  param:  None
 *  return: None
 */
void rpi_keyboard_reset(void)
#if (RPI_BARE_METAL)
{
} /* end of RPI_BARE_METAL */
#else
{
    bcm2835_gpio_write(AVR_RESET, LOW);
    usleep(10);
    bcm2835_gpio_write(AVR_RESET, HIGH);
}
#endif  /* end of not RPI_BARE_METAL */

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
 * rpi_testpoint_on()
 *
 *  Set test point to logic '1'
 *
 *  param:  None
 *  return: None
 */
void rpi_testpoint_on(void)
{
    bcm2835_gpio_write(PRI_TEST_POINT, HIGH);
}
/*------------------------------------------------
 * rpi_testpoint_on()
 *
 *  Set test point to logic '0'
 *
 *  param:  None
 *  return: None
 */
void rpi_testpoint_off(void)
{
    bcm2835_gpio_write(PRI_TEST_POINT, LOW);
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

/********************************************************************
 * fb_set_resolution()
 *
 *  Set screen resolution and return pointer to screen memory buffer
 *
 *  param:  Frame buffer device handle, horizontal and vertical resolution
 *          in pixels
 *  return: Pointer to memory, 0L on error
 */
static uint8_t *fb_set_resolution(int fbh, int x_pix, int y_pix)
#if (RPI_BARE_METAL)
{
    return -1;
} /* end of RPI_BARE_METAL */
#else
{
    uint8_t    *fbp = 0L;
    int         page_size = 0;
    long int    screen_size = 0;

    struct  fb_var_screeninfo var_info;
    struct  fb_fix_screeninfo fix_info;

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var_info))
    {
        printf("%s: Error reading variable screen info\n", __FUNCTION__);
        return 0L;
    }

    var_info.bits_per_pixel = 8;
    var_info.xres = x_pix;
    var_info.yres = y_pix;
    var_info.xres_virtual = x_pix;
    var_info.yres_virtual = y_pix;
    if ( ioctl(fbfd, FBIOPUT_VSCREENINFO, &var_info) )
    {
        printf("%s: Error setting variable information\n", __FUNCTION__);
    }

    printf("Display info: %dx%d, %d bpp\n",
           var_info.xres, var_info.yres,
           var_info.bits_per_pixel);

    // Get fixed screen information
    if ( ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) )
    {
        printf("%s: Error reading fixed information\n", __FUNCTION__);
        return 0L;
    }

    printf("Device ID: %s\n", fix_info.id);

    // map frame buffer to user memory
    screen_size = var_info.xres * var_info.yres_virtual * var_info.bits_per_pixel / 8;
    page_size = var_info.xres * var_info.yres;

    printf("Screen_size=%ld, page_size=%d\n", screen_size, page_size);

    if ( screen_size > fix_info.smem_len )
    {
        printf("%s: Screen_size over buffer limit\n", __FUNCTION__);
        return 0L;
    }

    fbp = (uint8_t*)mmap(0,
                         screen_size,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fbfd, 0);

    return fbp;
}
#endif  /* end of not RPI_BARE_METAL */

/********************************************************************
 * fb_set_tty()
 *
 *  Set screen to graphics mode.
 *
 *  param:  tty mode text=0, graphics=1
 *  return: 0 no error
 *         -1 on error
 */
static int fb_set_tty(const int mode)
#if (RPI_BARE_METAL)
{
    return -1;
} /* end of RPI_BARE_METAL */
#else
{
    int     console_fd;
    int     result = 0;

    console_fd = open("/dev/tty0", O_RDWR);

    if ( !console_fd )
    {
        printf("%s: could not open console.\n", __FUNCTION__);
        return -1;
    }

    if ( mode )
    {
        if (ioctl( console_fd, KDSETMODE, KD_GRAPHICS))
        {
            printf("%s: could not set console to KD_GRAPHICS mode.\n", __FUNCTION__);
            result = -1;
        }
    }
    else
    {
        if (ioctl( console_fd, KDSETMODE, KD_TEXT))
        {
            printf("%s: could not set console to KD_TEXT mode.\n", __FUNCTION__);
            result = -1;
        }
    }

    close(console_fd);

    return result;
}
#endif  /* end of not RPI_BARE_METAL */
