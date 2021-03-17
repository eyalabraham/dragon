/********************************************************************
 * pia.c
 *
 *  Module that implements the MC6847
 *  Video Display Generator (VDG) functionality.
 *
 *  https://en.wikipedia.org/wiki/Motorola_6847
 *  https://www.wikiwand.com/en/Semigraphics
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdint.h>

#include    "cpu.h"
#include    "mem.h"
#include    "vdg.h"
#include    "rpi.h"

#include    "dragon/font.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     VDG_REFRESH_INTERVAL    ((uint32_t)(1000000/50))

#define     SCREEN_WIDTH_PIX        256
#define     SCREEN_HEIGHT_PIX       192

#define     SCREEN_WIDTH_CHAR       32
#define     SCREEN_HEIGHT_CHAR      16

#define     FB_BLACK                0
#define     FB_BLUE                 1
#define     FB_GREEN                2
#define     FB_CYAN                 3
#define     FB_RED                  4
#define     FB_MAGENTA              5
#define     FB_BROWN                6
#define     FB_GRAY                 7
#define     FB_DARK_GRAY            8
#define     FB_LIGHT_BLUE           9
#define     FB_LIGHT_GREEN          10
#define     FB_LIGHT_CYAN           11
#define     FB_LIGHT_RED            12
#define     FB_LIGHT_MAGENT         13
#define     FB_YELLOW               14
#define     FB_WHITE                15

#define     CHAR_SEMI_GRAPHICS      0x80
#define     CHAR_INVERSE            0x40

#define     PIA_COLOR_SET           0x01

typedef enum
{                       // Colors   Res.     Bytes BASIC
    ALPHA_INTERNAL,     // 2 color  32x16    512   Default
    ALPHA_EXTERNAL,     // 4 color  32x16    512
    SEMI_GRAPHICS_4,    // 8 color  64x32    512
    SEMI_GRAPHICS_6,    // 8 color  64x48    512
    SEMI_GRAPHICS_8,    // 8 color  64x64   2048
    SEMI_GRAPHICS_12,   // 8 color  64x96   3072
    SEMI_GRAPHICS_24,   // 8 color  64x192  6144
    GRAPHICS_1C,        // 4 color  64x64   1024
    GRAPHICS_1R,        // 2 color  128x64  1024
    GRAPHICS_2C,        // 4 color  128x64  1536
    GRAPHICS_2R,        // 2 color  128x96  1536   PMODE0
    GRAPHICS_3C,        // 4 color  128x96  3072   PMODE1
    GRAPHICS_3R,        // 2 color  128x192 3072   PMODE2
    GRAPHICS_6C,        // 4 color  128x192 6144   PMODE3
    GRAPHICS_6R,        // 2 color  256x192 6144   PMODE4
    DMA,                // 2 color  256x192 6144
} video_mode_t;

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static void vdg_put_pixel_fb(int color, int x, int y);
static void vdg_draw_char(int c, int col, int row);
static video_mode_t vdg_get_mode(void);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static uint8_t  video_ram_offset;
static int      sam_video_mode;
static uint8_t  pia_video_mode;

static uint8_t *fbp;

/*------------------------------------------------
 * vdg_init()
 *
 *  Initialize the VDG device
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_init(void)
{
    video_ram_offset = 0x02;    // For offset 0x400 text screen
    sam_video_mode = 0;         // Alphanumeric

    fbp = rpi_fb_init(SCREEN_WIDTH_PIX, SCREEN_HEIGHT_PIX);
    if ( fbp == 0L )
    {
        emu_assert(0 && "Frame buffer error vdg_init()");
    }

    /* TODO Clear screen to green?
     */
}

/*------------------------------------------------
 * vdg_render()
 *
 *  Render video display.
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_render(void)
{
    static  uint32_t    last_refresh_time = 0;
    int     col, row, c;

    /* Set the refresh interval
     */
    if ( (rpi_system_timer() - last_refresh_time) < VDG_REFRESH_INTERVAL )
    {
        return;
    }

    last_refresh_time = rpi_system_timer();

    /* Refresh the frame buffer with screen content per
     * VDG/SAM mode settings
     */
    switch ( vdg_get_mode() )
    {
        case ALPHA_INTERNAL:
        case SEMI_GRAPHICS_4:
            for ( col = 0; col < SCREEN_WIDTH_CHAR; col++ )
            {
                for ( row = 0; row < SCREEN_HEIGHT_CHAR; row++ )
                {
                    c = mem_read(col + row * SCREEN_WIDTH_CHAR + (video_ram_offset << 9));
                    vdg_draw_char(c, col, row);
                }
            }
            break;

        case ALPHA_EXTERNAL:
        case SEMI_GRAPHICS_6:
            break;

        case SEMI_GRAPHICS_8:
        case SEMI_GRAPHICS_12:
        case SEMI_GRAPHICS_24:
        case GRAPHICS_1C:
        case GRAPHICS_1R:
        case GRAPHICS_2C:
        case GRAPHICS_2R:
        case GRAPHICS_3C:
        case GRAPHICS_3R:
        case GRAPHICS_6C:
        case GRAPHICS_6R:
        case DMA:
            break;
    }
}

/*------------------------------------------------
 * vdg_field_sync()
 *
 *  Generate an IRQ request at 50Hz rate
 *  emulating video field sync signal rate.
 *
 *  param:  Nothing
 *  return: Nothing
 */
void vdg_field_sync(void)
{
}

/*------------------------------------------------
 * vdg_set_video_offset()
 *
 *  Set the video display start offset in RAM.
 *  Most significant six bits of a 15 bit RAM address.
 *  Value is set by SAM device.
 *
 *  param:  Offset value.
 *  return: Nothing
 */
void vdg_set_video_offset(uint8_t offset)
{
    video_ram_offset = offset;
}

/*------------------------------------------------
 * vdg_set_mode_sam()
 *
 *  Set the video display mode from SAM device.
 *
 *  0   Alpha, S4, S6
 *  1   G1C, G1R
 *  2   G2C
 *  3   G2R
 *  4   G3C
 *  5   G3R
 *  6   G6R, G6C
 *  7   DMA
 *
 *  param:  Mode value.
 *  return: Nothing
 */
void vdg_set_mode_sam(int sam_mode)
{
    sam_video_mode = sam_mode;
}

/*------------------------------------------------
 * vdg_set_mode_pia()
 *
 *  Set the video display mode from PIA device.
 *  Mode bit are as-is for PIA sifted 3 to the right:
 *  Bit 4   O   ScreenMode G / ^A
 *  Bit 3   O   ScreenMode GM2
 *  Bit 2   O   ScreenMode GM1
 *  Bit 1   O   ScreenMode GM0 / ^INT
 *  Bit 0   O   ScreenMode CSS
 *
 *  param:  Mode value.
 *  return: Nothing
 */
void vdg_set_mode_pia(uint8_t pia_mode)
{
    pia_video_mode = pia_mode;
}

/*------------------------------------------------
 * vdg_put_pixel()
 *
 *  Put a pixel in the screen frame buffer
 *  Lowest level function to draw a pixel in the frame buffer.
 *  Assumes an 8-bit per pixel video mode is selected, and
 *  the calling function has calculated the correct (x,y) pixel coordinates.
 *  The Dragon VDG always uses a fixed 256x192 screen resolution.
 *
 * param:  pixel color, x and y coordinates
 * return: none
 *
 */
static void vdg_put_pixel_fb(int color, int x, int y)
{
    int pixel_offset;

    /* Calculate the pixel's byte offset inside the buffer
     * and update the frame buffer.
     */
    pixel_offset = x + y * SCREEN_WIDTH_PIX;
    *((uint8_t*)(fbp + pixel_offset)) = (uint8_t) color;    // The same as 'fbp[pix_offset] = value'
}

/*------------------------------------------------
 * vdg_draw_char()
 *
 * Draw a character in the screen frame buffer.
 * Low level function to draw a character in the frame buffer, and
 * assumes an 8-bit per pixel video mode is selected.
 * Provide VDG character code 0..63, use fonf.h definition for
 * character bitmap.
 * Bit 6, denotes inverse video, and bit 7 denotes Semigraphics
 * character.
 * The 'pia_video_mode' bit.0 to determine color (FB_GREEN or FB_BROWN).
 *
 * NOTE: no range checks are done on c, col, and row values!
 *
 * TODO: semigraphics output
 *
 * param:  c    VDG character code or semigraphics to be printed
 *         col  horizontal text position (0..31)
 *         row  vertical text position (0..15)
 * return: none
 *
 */
void vdg_draw_char(int c, int col, int row)
{
    uint8_t     pix_pos, bit_pattern;
    int         px, py;
    int         char_row, char_col, char_index;
    int         fg_color, bg_color;

    /* Convert text col and row to pixel positions
     * and adjust for non-semigraphics
     */
    px = col * FONT_WIDTH;
    py = row * FONT_HEIGHT;

    /* Determine colors
     */
    bg_color = FB_BLACK;

    if ( pia_video_mode & PIA_COLOR_SET )
        fg_color = FB_BROWN;
    else
        fg_color = FB_GREEN;

    if ( (uint8_t)c & CHAR_INVERSE )
    {
        char_row = fg_color;
        fg_color = bg_color;
        bg_color = char_row;
    }

    /* Output character pixels
     */
    if ( (uint8_t)c & CHAR_SEMI_GRAPHICS )
    {
        // TODO: semigraphics
    }
    else
    {
        char_index = (int)(((uint8_t) c) & ~(CHAR_SEMI_GRAPHICS | CHAR_INVERSE));

        for ( char_row = 0; char_row < FONT_HEIGHT; char_row++, py++ )
        {
            bit_pattern = font_img5x7[char_index][char_row];

            pix_pos = 0x80;

            for ( char_col = 0; char_col < FONT_WIDTH; char_col++ )
            {
                /* Bit is set in Font, print pixel(s) in text color
                 */
                if ( (bit_pattern & pix_pos) )
                {
                    vdg_put_pixel_fb(fg_color, (px + char_col), py);
                }
                /* Bit is cleared in Font
                 */
                else
                {
                    vdg_put_pixel_fb(bg_color, (px + char_col), py);
                }

                /* Move to the next pixel position
                 */
                pix_pos = pix_pos >> 1;
            }
        }
    }
}

/*------------------------------------------------
 * vdg_get_mode()
 *
 * Parse 'sam_video_mode' and 'pia_video_mode' and return video mode type.
 *
 * param:  None
 * return: Video mode
 *
 */
static video_mode_t vdg_get_mode(void)
{
    video_mode_t mode;

    if ( sam_video_mode == 7 )
    {
        mode = DMA;
    }
    else if ( (pia_video_mode & 0x10) == 1 )
    {
        switch ( pia_video_mode & 0x0e  )
        {
            case 0x00:
                mode = GRAPHICS_1C;
                break;
            case 0x02:
                mode = GRAPHICS_1R;
                break;
            case 0x04:
                mode = GRAPHICS_2C;
                break;
            case 0x06:
                mode = GRAPHICS_2R;
                break;
            case 0x08:
                mode = GRAPHICS_3C;
                break;
            case 0x0a:
                mode = GRAPHICS_3R;
                break;
            case 0x0c:
                mode = GRAPHICS_6C;
                break;
            case 0x0e:
                mode = GRAPHICS_6R;
                break;
        }
    }
    else if ( (pia_video_mode & 0x10) == 0 )
    {
        if ( sam_video_mode == 0 &&
             (pia_video_mode & 0x02) == 0 )
        {
            mode = ALPHA_INTERNAL;
            // Character bit.7 selects SEMI_GRAPHICS_4;
        }
        else if ( sam_video_mode == 0 &&
                (pia_video_mode & 0x02) == 1 )
        {
            mode = ALPHA_EXTERNAL;
            // Character bit.7 selects SEMI_GRAPHICS_6;
        }
        else if ( sam_video_mode == 2 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_8;
        }
        else if ( sam_video_mode == 4 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_12;
        }
        else if ( sam_video_mode == 4 &&
                (pia_video_mode & 0x02) == 0 )
        {
            mode = SEMI_GRAPHICS_24;
        }
    }

    return mode;
}
