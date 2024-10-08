/********************************************************************
 * rpi.c
 *
 *  Functions and definitions for RPi machine-dependent functionality.
 *  This is the Linux implementation (*not* bare-metal option.)
 *  Some conditional compilation is included for RPi models with
 *  different GPIO pins.
 *
 *  January 8, 2021
 *
 *******************************************************************/

#include    <stdio.h>
#include    <time.h>
#include    <assert.h>
#include    <fcntl.h>
#include    <linux/fb.h>
#include    <linux/kd.h>
#include    <sys/mman.h>
#include    <sys/ioctl.h>

#include    <unistd.h>
#include    <string.h>

#include    "bcm2835.h"
#include    "printf.h"
#include    "rpi.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
// AVR and keyboard
#define     AVR_RESET           RPI_V2_GPIO_P1_11
#define     PRI_TEST_POINT      RPI_V2_GPIO_P1_07

// Miscellaneous IO
#define     EMULATOR_RESET      RPI_V2_GPIO_P1_29

// Audio multiplexer and DAC/ADC
#define     AUDIO_MUX0          RPI_V2_GPIO_P1_03
#define     AUDIO_MUX1          RPI_V2_GPIO_P1_05
#define     AUDIO_MUX_MASK      ((1 << AUDIO_MUX0) | (1 << AUDIO_MUX1))

#define     DAC_BIT0            RPI_V2_GPIO_P1_15
#define     DAC_BIT1            RPI_V2_GPIO_P1_16
#define     DAC_BIT2            RPI_V2_GPIO_P1_18
#define     DAC_BIT3            RPI_V2_GPIO_P1_22
#if (RPI_MODEL_ZERO==1)
#define     DAC_BIT4            RPI_V2_GPIO_P1_37
#else
#define     DAC_BIT4            RPI_V2_GPIO_P1_12
#endif
#define     DAC_BIT5            RPI_V2_GPIO_P1_13

#define     JOYSTK_COMP         RPI_V2_GPIO_P1_26   // Joystick
#define     JOYSTK_BUTTON       RPI_V2_GPIO_P1_24   // Joystick button

#define     DAC_BIT_MASK        ((1 << DAC_BIT0) | (1 << DAC_BIT1) | (1 << DAC_BIT2) | \
                                 (1 << DAC_BIT3) | (1 << DAC_BIT4) | (1 << DAC_BIT5))

// SD card
#define     SPI_FILL_BYTE           0xff

#define     SD_CMD0                 0
#define     SD_CMD1                 1
#define     SD_CMD8                 8
#define     SD_CMD9                 9
#define     SD_CMD10                10
#define     SD_CMD12                12
#define     SD_CMD16                16
#define     SD_CMD17                17
#define     SD_CMD18                18
#define     SD_CMD23                23
#define     SD_CMD24                24
#define     SD_CMD25                25
#define     SD_CMD55                55
#define     SD_CMD58                58
#define     SD_CMD59                59
#define     SD_ACMD41               41

#define     SD_GO_IDLE_STATE        SD_CMD0
#define     SD_SEND_OP_COND         SD_CMD1
#define     SD_SEND_IF_COND         SD_CMD8
#define     SD_SEND_CSD             SD_CMD9
#define     SD_SEND_CID             SD_CMD10
#define     SD_STOP_TRANSMISSION    SD_CMD12
#define     SD_SET_BLOCKLEN         SD_CMD16
#define     SD_READ_SINGLE_BLOCK    SD_CMD17
#define     SD_READ_MULTIPLE_BLOCK  SD_CMD18
#define     SD_SET_BLOCK_COUNT      SD_CMD23
#define     SD_WRITE_BLOCK          SD_CMD24
#define     SD_WRITE_MULTIPLE_BLOCK SD_CMD25
#define     SD_APP_CMD              SD_CMD55
#define     SD_READ_OCR             SD_CMD58
#define     SD_NO_CRC               SD_CMD59
#define     SD_APP_SEND_OP_COND     SD_ACMD41

#define     SD_NCR                  10          // Command response time: 0 to 8 bytes for SDC, 1 to 8 bytes for MMC
#define     SD_TOKEN_START_BLOCK    0xfe        // For CMD17/18/24

#define     SD_R1_READY             0b00000000
#define     SD_R1_IDLE              0b00000001
#define     SD_R1_ERASE_RESET       0b00000010
#define     SD_R1_ILLIGAL_CMD       0b00000100
#define     SD_R1_CRC_ERROR         0b00001000
#define     SD_R1_ERASE_ERROR       0b00010000
#define     SD_R1_ADDRESS_ERROR     0b00100000
#define     SD_R1_PARAM_ERROR       0b01000000
#define     SD_FAILURE              0xff

#define     SD_BLOCK_SIZE           512         // Bytes
#define     SD_TIME_OUT             500000      // 500mSec
#define     SD_SPI_BIT_RATE         100000

#define     SD_SPI_CE2              RPI_V2_GPIO_P1_36

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static uint8_t   sd_send_cmd(int cmd, uint32_t arg);
static int       sd_wait_read_token(uint8_t token);
static int       sd_wait_ready(void);
static uint8_t   sd_get_crc7(uint8_t *message, int length);
static uint16_t  sd_get_crc16(const uint8_t *buf, int len );

static uint8_t  *fb_set_resolution(int fbh, int x_pix, int y_pix);
static int       fb_set_tty(const int mode);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static int      fbfd = 0;                        // frame buffer file descriptor

/*------------------------------------------------
 * rpi_gpio_init()
 *
 *  Initialize RPi GPIO functions:
 *  - Serial interface to AVR (PS2 keyboard controller)
 *  - Reset output GPIO to AVR
 *  - Output timing test point
 *  - Output GPIO bits to 6-bit DAC
 *
 *  param:  None
 *  return: -1 fail, 0 ok
 */
int rpi_gpio_init(void)
{
    if (!bcm2835_init())
    {
      printf("rpi_gpio_init(): bcm2835_init failed. Are you running as root??\n");
      return -1;
    }

    if (!bcm2835_spi_begin())
    {
      printf("rpi_gpio_init(): bcm2835_spi_begin failed. Are you running as root??\n");
      bcm2835_close();
      return -1;
    }

    /* Initialize GPIO for AVR reset line
     */
    bcm2835_gpio_fsel(AVR_RESET, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(AVR_RESET, HIGH);

    rpi_keyboard_reset();
    sleep(3);

    /* Initialize GPIO for RPi test point
     */
    bcm2835_gpio_fsel(PRI_TEST_POINT, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_write(PRI_TEST_POINT, LOW);

    /* Initialize 6-bit DAC, joystick comparator,
     * audio multiplexer control, and emulator reset GPIO lines
     */
    bcm2835_gpio_fsel(DAC_BIT0, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(DAC_BIT1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(DAC_BIT2, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(DAC_BIT3, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(DAC_BIT4, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(DAC_BIT5, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr_multi((1 << DAC_BIT0) | (1 << DAC_BIT1) | (1 << DAC_BIT2) |
                           (1 << DAC_BIT3) | (1 << DAC_BIT4) | (1 << DAC_BIT5));

    bcm2835_gpio_fsel(JOYSTK_COMP, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOYSTK_COMP, BCM2835_GPIO_PUD_OFF);

    bcm2835_gpio_fsel(JOYSTK_BUTTON, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(JOYSTK_BUTTON, BCM2835_GPIO_PUD_UP);

    bcm2835_gpio_fsel(AUDIO_MUX0, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(AUDIO_MUX1, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_clr_multi((1 << AUDIO_MUX0) | (1 << AUDIO_MUX1));

#if (RPI_MODEL_ZERO==1)
    bcm2835_gpio_fsel(EMULATOR_RESET, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_set_pud(EMULATOR_RESET, BCM2835_GPIO_PUD_UP);
#endif

    /* Initialize SPI0 for AVR keyboard interface
     */
    bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
    bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
    bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);

    return 0;
}

/********************************************************************
 * rpi_fb_init()
 *
 *  Initialize the RPi frame buffer device.
 *
 *  param:  None
 *  return: Pointer to frame buffer, or 0 if error,
 */
uint8_t *rpi_fb_init(int x_pix, int y_pix)
{
    uint8_t *fbp = 0;

    // Open the frame buffer device file for reading and writing
    if ( fbfd == 0 )
    {
        fbfd = open("/dev/fb0", O_RDWR);
        if (fbfd == -1)
        {
            printf("rpi_fb_init(): Cannot open frame buffer /dev/fb0\n");
            return 0;
        }
    }

    printf("Frame buffer device is open\n");

    if ( (int)(fbp = fb_set_resolution(fbfd, x_pix, y_pix)) == 0 )
    {
        return 0;
    }

    // Select graphics mode
    if ( fb_set_tty(1) )
    {
        printf("rpi_fb_init(): Could not set tty0 mode.\n");
        return 0;
    }

    return fbp;
}

/********************************************************************
 * rpi_fb_resolution()
 *
 *  Change the RPi frame buffer resolution.
 *  Frame buffer must be already initialized with rpi_fb_init()
 *
 *  param:  None
 *  return: Pointer to frame buffer, or 0 if error,
 */
uint8_t *rpi_fb_resolution(int x_pix, int y_pix)
{
    uint8_t *fbp = 0;

    if ( (int)(fbp = fb_set_resolution(fbfd, x_pix, y_pix)) == 0 )
    {
        return 0;
    }

    return fbp;
}

/*------------------------------------------------
 * rpi_system_timer()
 *
 *  Return running system timer time stamp
 *
 *  param:  None
 *  return: System timer value
 */
uint32_t rpi_system_timer(void)
{
    return (uint32_t) clock();
}

/*------------------------------------------------
 * rpi_keyboard_read()
 *
 *  Read serial interface from AVR (PS2 keyboard controller)
 *
 *  param:  None
 *  return: Key code
 */
int rpi_keyboard_read(void)
{
    return (int)bcm2835_spi_transfer(0);
}

/*------------------------------------------------
 * rpi_keyboard_reset()
 *
 *  Reset keyboard AVR interface
 *
 *  param:  None
 *  return: None
 */
void rpi_keyboard_reset(void)
{
    bcm2835_gpio_write(AVR_RESET, LOW);
    usleep(10);
    bcm2835_gpio_write(AVR_RESET, HIGH);
}

/*------------------------------------------------
 * rpi_joystk_comp()
 *
 *  Read joystick comparator GPIO input pin and return its value.
 *
 *  param:  None
 *  return: GPIO joystick comparator input level
 */
int rpi_joystk_comp(void)
{
    /* The delay is needed to allow the DAC and comparator
     * to stabilize the output, and propagate it through the
     * 5v/3.3v level-shifter that is bandwidth-limited.
     * The Dragon code is limited by a ~13uSec between writing
     * to DAC and reading comparator input:
     *
     *      STB     PIA1DA          ; send value to D/A converter
     *      TST     PIA0DA          ; read result value, comparator output in bit 7
     *
     * A 20uSec delay seems to stabilize the joystick ADC readings.
     *
     */
    bcm2835_delayMicroseconds(20);

    return (int) bcm2835_gpio_lev(JOYSTK_COMP);
}

/*------------------------------------------------
 * rpi_rjoystk_button()
 *
 *  Read right joystick button GPIO input pin and return its value.
 *
 *  param:  None
 *  return: GPIO joystick button input level
 */
int rpi_rjoystk_button(void)
{
    return (int) bcm2835_gpio_lev(JOYSTK_BUTTON);
}

/*------------------------------------------------
 * rpi_reset_button()
 *
 *  Emulator reset button GPIO input pin and return its value.
 *
 *  param:  None
 *  return: GPIO reset button input level
 */
int rpi_reset_button(void)
{
#if (RPI_MODEL_ZERO==1)
    return (int) bcm2835_gpio_lev(EMULATOR_RESET);
#else
    return 1;
#endif
}

/*------------------------------------------------
 * rpi_audio_mux_set()
 *
 *  Set GPIO to select analog multiplexer output.
 *
 *  param:  Multiplexer select bit field: b.1=PIA1-CB2, b.0=PIA0-CA2
 *  return: None
 */
void rpi_audio_mux_set(int select)
{
    static int previous_select = 0;

    if ( select != previous_select )
    {
        bcm2835_gpio_write_mask((uint32_t)select << AUDIO_MUX0, (uint32_t) AUDIO_MUX_MASK);
        bcm2835_delayMicroseconds(20);  // TODO check if this is needed to reduce value noise
        previous_select = select;
    }
}

/*------------------------------------------------
 * rpi_write_dac()
 *
 *  Write 6-bit value to DAC
 *
 *  param:  DAC value 0x00 to 0x3f
 *  return: None
 */
void rpi_write_dac(int dac_value)
{
    uint32_t    dac_bit_values = 0;

    /* Arrange GPIO bit pin outputs
     */
    dac_bit_values = dac_value << DAC_BIT0;

#if (RPI_MODEL_ZERO==0)
    if ( dac_bit_values & 0x04000000 )
        dac_bit_values |= (1 << DAC_BIT4);
#endif

    /* Set the first 32 GPIO output pins specified in the 'mask' to the value given by 'value'
     *  value: values required for each bit masked in by mask.
     *  mask: of pins to affect
     */
    bcm2835_gpio_write_mask(dac_bit_values, (uint32_t) DAC_BIT_MASK);
}

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
 * rpi_testpoint_off()
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

/********************************************************************
 * rpi_halt()
 *
 *  Output message and halt
 *
 *  param:  Message
 *  return: None
 */
void rpi_halt(void)
{
    printf("HALT\n");
    assert(0);
}

/*------------------------------------------------
 * _putchar()
 *
 *  Low level character output/stream for printf()
 *
 *  param:  character
 *  return: none
 */
void _putchar(char character)
{
    putchar(character);
}

/* -------------------------------------------------------------
 * rpi_sd_init()
 *
 *  Initialize SD card and reader
 *
 *  Param:  None
 *  Return: Driver error
 */
#if (RPI_MODEL_ZERO==0)
sd_error_t rpi_sd_init(void)
{
    return SD_GPIO_FAIL;
}
#else
sd_error_t rpi_sd_init(void)
{
    uint8_t     mosi_buffer[10] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
    uint8_t     sd_response;
    uint32_t    start_time;

    if (!bcm2835_aux_spi_begin())
    {
      printf("rpi_sd_init() bcm2835_aux_spi_begin failed. Are you running as root??\n");
      return SD_GPIO_FAIL;
    }

    /* Initialize SPI1 for SD card
     * Resource: http://elm-chan.org/docs/mmc/i/sdinit.png
     */
    bcm2835_gpio_fsel(SD_SPI_CE2, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_set(SD_SPI_CE2);                           // CS to 'High'

    //BCM2835_AUX_SPI_CLOCK_MIN
    bcm2835_aux_spi_setClockDivider(bcm2835_aux_spi_CalcClockDivider(SD_SPI_BIT_RATE));

    bcm2835_delayMicroseconds(2000);                        // Power on delay
    bcm2835_aux_spi_transfern((char *) mosi_buffer, 10);    // Dummy clocks, CS=DI=High
    bcm2835_gpio_fsel(SD_SPI_CE2, BCM2835_GPIO_FSEL_ALT4);  // Back to normal CS

    if ( sd_wait_ready() == 0 )                             // Check MISO is high (card DO=1)
    {
        printf("rpi_sd_init(): Time out waiting for SD ready state.\n");
        return SD_FAIL;
    }

    sd_response = sd_send_cmd(SD_GO_IDLE_STATE, 0);
    if ( sd_response != SD_R1_IDLE )
    {
      printf("rpi_sd_init(): SD card failed SD_GO_IDLE_STATE.\n");
      return SD_FAIL;
    }

    start_time = (uint32_t) clock();

    do
        {
            sd_response = sd_send_cmd(SD_APP_CMD, 0);
            if ( sd_response == SD_FAILURE )
            {
              printf("rpi_sd_init(): SD card failed SD_APP_CMD.\n");
              return SD_FAIL;
            }

            sd_response = sd_send_cmd(SD_APP_SEND_OP_COND, 0);
            if ( sd_response == SD_FAILURE )
            {
              printf("rpi_sd_init(): SD card failed SD_APP_SEND_OP_COND.\n");
              return SD_FAIL;
            }
        }
    while ( sd_response != SD_R1_READY && ((uint32_t)clock() - start_time) < SD_TIME_OUT );

    if ( sd_response != SD_R1_READY )
    {
      printf("rpi_sd_init(): SD card failed SD_APP_SEND_OP_COND.\n");
      return SD_TIMEOUT;
    }

    sd_response = sd_send_cmd(SD_SET_BLOCKLEN, SD_BLOCK_SIZE);
    if ( sd_response != SD_R1_READY )
    {
      printf("rpi_sd_init(): SD card failed SD_SET_BLOCKLEN.\n");
      return SD_FAIL;
    }

    return SD_OK;
}
#endif

/* -------------------------------------------------------------
 * rpi_sd_read_block()
 *
 *  Read a block (sector) from the SD card
 *
 *  Param:  LBA number, buffer address, and its length
 *  Return: Driver error
 */
#if (RPI_MODEL_ZERO==0)
sd_error_t rpi_sd_read_block(uint32_t lba, uint8_t *buffer, uint32_t length)
{
    return SD_GPIO_FAIL;
}
#else
sd_error_t rpi_sd_read_block(uint32_t lba, uint8_t *buffer, uint32_t length)
{
    int     i;
    uint8_t sd_response;
    uint8_t crc_high, crc_low;
    uint8_t input_buffer[SD_BLOCK_SIZE+2];  // Data block plus two-byte CRC

    if ( length < SD_BLOCK_SIZE )
    {
        return SD_READ_FAIL;
    }

    /* Send read command to SD card
     */
    bcm2835_delayMicroseconds(500);

    sd_response = sd_send_cmd(SD_READ_SINGLE_BLOCK, lba * SD_BLOCK_SIZE);   // *** SDC uses BYTE addressing ***
    if ( sd_response != SD_R1_READY )
    {
        printf("rpi_sd_read_block(): sd_send_cmd() failed %d.\n", sd_response);
        return SD_FAIL;
    }

    /* Wait for start of data token (0xFE)
     */
    if ( sd_wait_read_token(SD_TOKEN_START_BLOCK) == 0 )
    {
        printf("rpi_sd_read_block(): sd_wait_read_token() failed.\n");
        return SD_TIMEOUT;
    }

    /* Read a data block (one SD sector)
     */
    memset(input_buffer, SPI_FILL_BYTE, (SD_BLOCK_SIZE+2));

    for ( i = 0; i < (SD_BLOCK_SIZE+2); i++ )
    {
        input_buffer[i] = bcm2835_aux_spi_transfer(SPI_FILL_BYTE);
    }

    /* Check CRC
     */
    crc_high = input_buffer[SD_BLOCK_SIZE];
    crc_low = input_buffer[SD_BLOCK_SIZE+1];

    if ( sd_get_crc16(input_buffer, SD_BLOCK_SIZE) != ((crc_high << 8) + crc_low) )
    {
        printf("rpi_sd_read_block(): sd_get_crc16() failed.\n");
        return SD_BAD_CRC;
    }

    memcpy(buffer, input_buffer, SD_BLOCK_SIZE);

    return SD_OK;
}
#endif

/* -------------------------------------------------------------
 * sd_send_cmd()
 *
 *  Send a command to the SD card
 *
 *  Param:  Command code and argument
 *  Return: Failure=SD_FAILURE, or R1 SD status
 */
uint8_t sd_send_cmd(int cmd, uint32_t arg)
{
    int     i;
    uint8_t response;
    uint8_t mosi_buffer[6];
    uint8_t crc;

    /* check if DO is high
     */
    if ( sd_wait_ready() == 0 )
    {
        printf("sd_send_cmd(): Time out waiting for SD ready state.\n");
        return SD_FAILURE;
    }

    /* Prepare the command packet and
     * always provide a correct CRC
     */
    mosi_buffer[0] = 0x40 | ((uint8_t) cmd);
    mosi_buffer[1] = (uint8_t)(arg >> 24);
    mosi_buffer[2] = (uint8_t)(arg >> 16);
    mosi_buffer[3] = (uint8_t)(arg >> 8);
    mosi_buffer[4] = (uint8_t)(arg >> 0);

    crc = sd_get_crc7(mosi_buffer, 5);
    mosi_buffer[5] = (crc << 1) | 0b00000001;

    /* Send the command command packet via SPI.
     */
    for ( i = 0; i < 6; i++ )
    {
        bcm2835_aux_spi_transfer(mosi_buffer[i]);
    }

    /* Send out '1's on MOSI until a response is received from the SD
     */
    i = 0;
    do
    {
        response = bcm2835_aux_spi_transfer(SPI_FILL_BYTE);
        i++;
    }
    while ( response == SPI_FILL_BYTE && i < SD_NCR );

    return response;
}

/* -------------------------------------------------------------
 * sd_wait_read_token()
 *
 *  Wait for a read-token from the SD card
 *
 *  Param:  Token to wait for
 *  Return: ok=1, time-out=0
 */
static int sd_wait_read_token(uint8_t token)
{
    uint32_t    start_time;
    uint8_t     have_token;

    start_time = (uint32_t) clock();
    have_token = 0;

    do
    {
        if ( bcm2835_aux_spi_transfer(SPI_FILL_BYTE) == token )
        {
            have_token = 1;
            break;
        }
    }
    while ( ((uint32_t)clock() - start_time) < SD_TIME_OUT );

    return have_token;
}

/* -------------------------------------------------------------
 * sd_wait_ready()
 *
 *  Wait for a ready state (DO=1) from the SD card
 *
 *  Param:  None
 *  Return: read=1, time-out=0
 */
static int sd_wait_ready(void)
{
    uint32_t    start_time;

    start_time = (uint32_t) clock();

    do
    {
        if ( SPI_FILL_BYTE == bcm2835_aux_spi_transfer(SPI_FILL_BYTE) )
        {
            return 1;
        }
    }
    while ( ((uint32_t)clock() - start_time) < SD_TIME_OUT );

    return 0;
}

/* -------------------------------------------------------------
 * sd_get_crc7()
 *
 *  Calculate CRC7 on a message.
 *
 *  Param:  Pointer to message buffer, length of message
 *  Return: CRC7 byte
 */
static uint8_t sd_get_crc7(uint8_t *message, int length)
{
    static uint8_t crc_lookup_table[256] =
        {
            0x00, 0x09, 0x12, 0x1b, 0x24, 0x2d, 0x36, 0x3f, 0x48, 0x41, 0x5a, 0x53, 0x6c, 0x65, 0x7e, 0x77,
            0x19, 0x10, 0x0b, 0x02, 0x3d, 0x34, 0x2f, 0x26, 0x51, 0x58, 0x43, 0x4a, 0x75, 0x7c, 0x67, 0x6e,
            0x32, 0x3b, 0x20, 0x29, 0x16, 0x1f, 0x04, 0x0d, 0x7a, 0x73, 0x68, 0x61, 0x5e, 0x57, 0x4c, 0x45,
            0x2b, 0x22, 0x39, 0x30, 0x0f, 0x06, 0x1d, 0x14, 0x63, 0x6a, 0x71, 0x78, 0x47, 0x4e, 0x55, 0x5c,
            0x64, 0x6d, 0x76, 0x7f, 0x40, 0x49, 0x52, 0x5b, 0x2c, 0x25, 0x3e, 0x37, 0x08, 0x01, 0x1a, 0x13,
            0x7d, 0x74, 0x6f, 0x66, 0x59, 0x50, 0x4b, 0x42, 0x35, 0x3c, 0x27, 0x2e, 0x11, 0x18, 0x03, 0x0a,
            0x56, 0x5f, 0x44, 0x4d, 0x72, 0x7b, 0x60, 0x69, 0x1e, 0x17, 0x0c, 0x05, 0x3a, 0x33, 0x28, 0x21,
            0x4f, 0x46, 0x5d, 0x54, 0x6b, 0x62, 0x79, 0x70, 0x07, 0x0e, 0x15, 0x1c, 0x23, 0x2a, 0x31, 0x38,
            0x41, 0x48, 0x53, 0x5a, 0x65, 0x6c, 0x77, 0x7e, 0x09, 0x00, 0x1b, 0x12, 0x2d, 0x24, 0x3f, 0x36,
            0x58, 0x51, 0x4a, 0x43, 0x7c, 0x75, 0x6e, 0x67, 0x10, 0x19, 0x02, 0x0b, 0x34, 0x3d, 0x26, 0x2f,
            0x73, 0x7a, 0x61, 0x68, 0x57, 0x5e, 0x45, 0x4c, 0x3b, 0x32, 0x29, 0x20, 0x1f, 0x16, 0x0d, 0x04,
            0x6a, 0x63, 0x78, 0x71, 0x4e, 0x47, 0x5c, 0x55, 0x22, 0x2b, 0x30, 0x39, 0x06, 0x0f, 0x14, 0x1d,
            0x25, 0x2c, 0x37, 0x3e, 0x01, 0x08, 0x13, 0x1a, 0x6d, 0x64, 0x7f, 0x76, 0x49, 0x40, 0x5b, 0x52,
            0x3c, 0x35, 0x2e, 0x27, 0x18, 0x11, 0x0a, 0x03, 0x74, 0x7d, 0x66, 0x6f, 0x50, 0x59, 0x42, 0x4b,
            0x17, 0x1e, 0x05, 0x0c, 0x33, 0x3a, 0x21, 0x28, 0x5f, 0x56, 0x4d, 0x44, 0x7b, 0x72, 0x69, 0x60,
            0x0e, 0x07, 0x1c, 0x15, 0x2a, 0x23, 0x38, 0x31, 0x46, 0x4f, 0x54, 0x5d, 0x62, 0x6b, 0x70, 0x79
        };

    int     i;
    uint8_t crc = 0;

    for ( i = 0; i < length; i++ )
        crc = crc_lookup_table[(crc << 1) ^ message[i]];

    return crc;
}

/* -------------------------------------------------------------
 * sd_get_crc16()
 *
 *  Calculate CRC16 on a message.
 *
 *  Param:  Pointer to message buffer, length of message
 *  Return: CRC16 word
 */
static uint16_t sd_get_crc16(const uint8_t *buf, int len )
{
    uint16_t crc = 0;
    int i;

    while( len-- )
    {
        crc ^= *(uint8_t *)buf++ << 8;
        for( i = 0; i < 8; i++ )
        {
            if( crc & 0x8000 )
                crc = (crc << 1) ^ 0x1021;
            else
                crc = crc << 1;
        }
    }
    return crc;
}

/********************************************************************
 * fb_set_resolution()
 *
 *  Set screen resolution and return pointer to screen memory buffer.
 *  Function sets frame buffer to 8-bits per-pixel.
 *
 *  param:  Frame buffer device handle, horizontal and vertical resolution
 *          in pixels
 *  return: Pointer to memory, 0L on error
 */
static uint8_t *fb_set_resolution(int fbh, int x_pix, int y_pix)
{
    uint8_t    *fbp = 0;
    int         page_size = 0;
    long int    screen_size = 0;

    struct  fb_var_screeninfo var_info;
    struct  fb_fix_screeninfo fix_info;

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &var_info))
    {
        printf("fb_set_resolution(): Error reading variable screen info.\n");
        return 0L;
    }

    var_info.bits_per_pixel = 8;
    var_info.xres = x_pix;
    var_info.yres = y_pix;
    var_info.xres_virtual = x_pix;
    var_info.yres_virtual = y_pix;
    if ( ioctl(fbfd, FBIOPUT_VSCREENINFO, &var_info) )
    {
        printf("fb_set_resolution(): Error setting variable information.\n");
    }

    printf("Display info: %dx%d, %d bpp\n",
           var_info.xres, var_info.yres,
           var_info.bits_per_pixel);

    // Get fixed screen information
    if ( ioctl(fbfd, FBIOGET_FSCREENINFO, &fix_info) )
    {
        printf("fb_set_resolution(): Error reading fixed information.\n");
        return 0L;
    }

    printf("Device ID: %s\n", fix_info.id);

    // map frame buffer to user memory
    screen_size = var_info.xres * var_info.yres_virtual * var_info.bits_per_pixel / 8;
    page_size = var_info.xres * var_info.yres;

    printf("Screen_size=%ld, page_size=%d\n", screen_size, page_size);

    if ( screen_size > fix_info.smem_len )
    {
        printf("fb_set_resolution(): screen_size over buffer limit.\n");
        return 0L;
    }

    fbp = (uint8_t*)mmap(0,
                         screen_size,
                         PROT_READ | PROT_WRITE,
                         MAP_SHARED,
                         fbfd, 0);

    if ( (int)fbp == -1 )
    {
        printf("fb_set_resolution(): Failed to mmap()\n");
        return 0;
    }

    return fbp;
}

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
{
    int     console_fd;
    int     result = 0;

    console_fd = open("/dev/tty0", O_RDWR);

    if ( !console_fd )
    {
        printf("fb_set_tty(): Could not open console.\n");
        return -1;
    }

    if ( mode )
    {
        if (ioctl( console_fd, KDSETMODE, KD_GRAPHICS))
        {
            printf("fb_set_tty(): Could not set console to KD_GRAPHICS mode.\n");
            result = -1;
        }
    }
    else
    {
        if (ioctl( console_fd, KDSETMODE, KD_TEXT))
        {
            printf("fb_set_tty(): Could not set console to KD_TEXT mode.\n");
            result = -1;
        }
    }

    close(console_fd);

    return result;
}
