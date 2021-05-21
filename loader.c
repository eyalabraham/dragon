/********************************************************************
 * loader.C
 *
 *  ROM and CAS file loader module.
 *  Activated as an emulator escape.
 *
 *  May 11, 2021
 *
 *******************************************************************/

#include    <stdint.h>
#include    <ctype.h>

#include    "printf.h"

#include    "cpu.h"
#include    "mem.h"
#include    "rpi.h"
#include    "vdg.h"
#include    "sdfat32.h"

#include    "loader.h"

/* -----------------------------------------
   Module definition
----------------------------------------- */
#define     text_highlight_on(r)    text_highlight(1, r)
#define     text_highlight_off()    text_highlight(0, 0)

#define     FAT32_MAX_DIR_LIST      256

#define     SCAN_CODE_Q             16
#define     SCAN_CODE_ENTR          28
#define     SCAN_CODE_UP            72
#define     SCAN_CODE_DOWN          80

#define     CHAR_UP_ARROW           32
#define     CHAR_DOWN_ARROW         32

#define     TERMINAL_STATUS_ROW     15
#define     TERMINAL_LIST_LENGTH    (TERMINAL_STATUS_ROW-1)

#define     MSG_EXIT                "PRESS <Q> TO EXIT.              "
#define     MSG_STATUS              "PRESS: <UP> <DOWN> <ENTER> <Q>  "
#define     MSG_SD_ERROR            "SD CARD INITIALIZATION FAILED,  " \
                                    "REPLACE OR INSERT A CARD.       "
#define     MSG_FAT32_ERROR         "FAT32 INITIALIZATION FAILED,    " \
                                    "FIX CARD FORMATING.             "
#define     MSG_DIR_READ_ERROR      "DIRECTORY LOADING ERROR.        "
#define     MSG_ROM_READ_ERROR      "ROM IMAGE READ ERROR.           "
#define     MSG_ROM_READ_DONE       "ROM IMAGE LOAD COMPLETED.       "

#define     CODE_BUFFER_SIZE        16384
#define     CARTRIDGE_ROM_BASE      0xc000
#define     CARTRIDGE_ROM_END       0xffef

typedef enum
    {
        FILE_ROM,
        FILE_CAS,
        FILE_PNG,
        FILE_JPG,
        FILE_OTHER,
    } file_type_t;

/* -----------------------------------------
   Module function
----------------------------------------- */
static file_type_t file_get_type(dir_entry_t *directory_entry);

static void        text_write(int row, int col, char *text);
static void        text_highlight(int on_off, int row);
static void        text_dir_output(int list_start, int list_length, dir_entry_t *directory_list);
static void        text_clear(void);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static int      sd_card_initialized = 0;
static uint8_t  text_screen_save[512];
static uint8_t  code_buffer[CODE_BUFFER_SIZE];

/*------------------------------------------------
 * loader()
 *
 *  ROM and CAS file loader function activated as
 *  an emulator escape.
 *
 *  param:  Nothing
 *  return: Nothing
 */
void loader(void)
{
    int             i;
    int             key_pressed;
    int             rom_bytes;

    dir_entry_t     directory_list[FAT32_MAX_DIR_LIST];
    int             list_start, prev_list_start, list_length;
    int             highlighted_line;

    /* Save text screen content and clear it
     */
    for ( i = 0; i < 512; i++ )
    {
        text_screen_save[i] = mem_read(0x400 + i);
        mem_write(0x400 + i, 32);
    }

    /* Initialize SD card
     */
    if ( sd_card_initialized == 0 )
    {
        if ( ( i = rpi_sd_init()) == SD_OK )
        {
            sd_card_initialized = 1;
            printf("SD card initialized.\n");
        }
        else
        {
            printf("SD initialization failed (%d).\n", i);

            text_write(0, 0, MSG_SD_ERROR);
            text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

            for (;;)
            {
                vdg_render();

                key_pressed = rpi_keyboard_read();

                if ( key_pressed == SCAN_CODE_Q )
                {
                    for ( i = 0; i < 512; i++ )
                    {
                        mem_write(0x400 + i, text_screen_save[i]);
                    }

                    return;
                }
            }
        }
    }

    /* Initialize FAT32 file system parameters
     * for file and directory reading and parsing.
     */
    if ( ( i = fat32_init()) == FAT_OK )
    {
        printf("FAT32 initialized.\n");
    }
    else
    {
        sd_card_initialized = 0;

        printf("FAT32 initialization failed (%d).\n", i);

        text_write(0, 0, MSG_FAT32_ERROR);
        text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

        for (;;)
        {
            vdg_render();

            key_pressed = rpi_keyboard_read();

            if ( key_pressed == SCAN_CODE_Q )
            {
                for ( i = 0; i < 512; i++ )
                {
                    mem_write(0x400 + i, text_screen_save[i]);
                }

                return;
            }
        }
    }

    /* Initial directory load
     */
    if ( (list_length = fat32_parse_dir(2, directory_list, FAT32_MAX_DIR_LIST)) == -1 )
    {
        sd_card_initialized = 0;

        text_write(0, 0, MSG_DIR_READ_ERROR);
        text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

        for (;;)
        {
            vdg_render();

            key_pressed = rpi_keyboard_read();

            if ( key_pressed == SCAN_CODE_Q )
            {
                for ( i = 0; i < 512; i++ )
                {
                    mem_write(0x400 + i, text_screen_save[i]);
                }

                return;
            }
        }
    }

    /* Main loop.
     */
    list_start = 0;
    prev_list_start = -1;
    highlighted_line = 0;
    text_dir_output(list_start, list_length, directory_list);
    text_write(TERMINAL_STATUS_ROW, 0, MSG_STATUS);

    for (;;)
    {
        vdg_render();

        key_pressed = rpi_keyboard_read();

        if ( key_pressed == SCAN_CODE_Q )
        {
            break;
        }
        else if ( key_pressed == SCAN_CODE_UP )
        {
            highlighted_line--;
            if ( highlighted_line < 0 )
            {
                highlighted_line = 0;
                if ( list_start > 0 )
                    list_start--;
            }
        }
        else if ( key_pressed == SCAN_CODE_DOWN )
        {
            highlighted_line++;
            if ( highlighted_line == list_length )
            {
                highlighted_line--;
            }
            else if ( highlighted_line > TERMINAL_LIST_LENGTH )
            {
                highlighted_line = TERMINAL_LIST_LENGTH;
                list_start++;
                if ( (list_length - list_start) < TERMINAL_LIST_LENGTH )
                    list_start = list_length - TERMINAL_LIST_LENGTH;
            }
        }
        else if ( key_pressed == SCAN_CODE_ENTR )
        {
            text_clear();

            if ( directory_list[(list_start + highlighted_line)].is_directory )
            {
                /* Read and display the directory
                 */
                if ( (list_length = fat32_parse_dir(directory_list[(list_start + highlighted_line)].cluster_chain_head,
                                                    directory_list, FAT32_MAX_DIR_LIST)) == -1 )
                {
                    sd_card_initialized = 0;

                    text_write(0, 0, MSG_DIR_READ_ERROR);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

                    for (;;)
                    {
                        vdg_render();

                        key_pressed = rpi_keyboard_read();

                        if ( key_pressed == SCAN_CODE_Q )
                        {
                            for ( i = 0; i < 512; i++ )
                            {
                                mem_write(0x400 + i, text_screen_save[i]);
                            }

                            return;
                        }
                    }
                }

                list_start = 0;
                prev_list_start = 0;
                highlighted_line = 0;
                text_dir_output(list_start, list_length, directory_list);
            }
            else
            {
                /* TODO Handle .rom and .cas extensions ignore
                 * all other file types
                 */

                /* Load ROM image into emulator memory
                 * and auto-run it by setting FIRQ
                 */
                rom_bytes = fat32_get_file(directory_list[(list_start + highlighted_line)].cluster_chain_head,
                                           directory_list[(list_start + highlighted_line)].file_size,
                                           code_buffer, CODE_BUFFER_SIZE);

                if ( rom_bytes == -1 )
                {
                    text_write(0, 0, MSG_ROM_READ_ERROR);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

                    for (;;)
                    {
                        vdg_render();

                        key_pressed = rpi_keyboard_read();

                        if ( key_pressed == SCAN_CODE_Q )
                        {
                            for ( i = 0; i < 512; i++ )
                            {
                                mem_write(0x400 + i, text_screen_save[i]);
                            }

                            return;
                        }
                    }
                }
                else
                {
                    mem_load(CARTRIDGE_ROM_BASE, code_buffer, rom_bytes);
                    mem_define_rom(CARTRIDGE_ROM_BASE, (rom_bytes - 1));
                    //cpu_firq(1);

                    text_write(0, 0, MSG_ROM_READ_DONE);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

                    for (;;)
                    {
                        vdg_render();

                        key_pressed = rpi_keyboard_read();

                        if ( key_pressed == SCAN_CODE_Q )
                        {
                            for ( i = 0; i < 512; i++ )
                            {
                                mem_write(0x400 + i, text_screen_save[i]);
                            }

                            return;
                        }
                    }
                }
            }
        }

        if ( list_start != prev_list_start )
        {
            text_clear();
            text_dir_output(list_start, list_length, directory_list);
            prev_list_start = list_start;
        }

        text_highlight_on(highlighted_line);
    }

    /* Restore screen and exit loader.
     */
    for ( i = 0; i < 512; i++ )
    {
        mem_write(0x400 + i, text_screen_save[i]);
    }
}

/*------------------------------------------------
 * file_get_type()
 *
 *  Parse a file name passed in a directory entry record
 *  and return its type.
 *
 *  param:  Pointer to directory entry record
 *  return: File type
 */
static file_type_t file_get_type(dir_entry_t *directory_entry)
{
    return FILE_OTHER;
}

/*------------------------------------------------
 * text_write()
 *
 *  Output text to the text screen buffer
 *  vdg_render() will output. This function allows
 *  the loader to use the regular text display of the
 *  Dragon emulation.
 *  Text longer than a row will be wrapped to next line.
 *  Row and column numbers are 0 based.
 *
 *  param:  Row (0..15) and column (0..31) positions, text to output
 *  return: Nothing
 */
static void text_write(int row, int col, char *text)
{
    int     count;

    count = row * 32 + col;

    while ( *text && (count < 512) )
    {
        mem_write(0x400 + count, toupper((int)(*text)) & 0xbf);
        text++;
        count++;
    }
}

/*------------------------------------------------
 * text_highlight()
 *
 *  Highlight a row on the screen by flipping
 *  its video inverse bit
 *
 *  param:  Text highlighted ('1') and not ('0'), and row number (0..14)
 *  return: Nothing
 */
static void text_highlight(int on_off, int row)
{
    static int  highlighted_row = -1;

    int     i, c;
    int     row_address;

    /* Remove row-highlight if one is highlighted
     */
    if ( on_off == 0 && highlighted_row != -1 )
    {
        row_address = 0x400 + highlighted_row * 32;
        for ( i = 1; i < 32; i++ )
        {
            c = mem_read((row_address + i)) & 0xbf;
            mem_write((row_address + i), c);
        }
        highlighted_row = -1;
    }

    /* Highlight a row
     */
    else if ( on_off == 1 )
    {
        if ( row < 0 || row > 14 )
        {
            return;
        }
        else if ( highlighted_row == row )
        {
            return;
        }
        else if ( highlighted_row == -1 )
        {
            row_address = 0x400 + row * 32;
            for ( i = 1; i < 32; i++ )
            {
                c = mem_read((row_address + i)) | 0x40;
                mem_write((row_address + i), c);
            }
            highlighted_row = row;
        }
        else
        {
            text_highlight(0, 0);
            text_highlight(1, row);
        }
    }
}

/*------------------------------------------------
 * text_dir_output()
 *
 *  Print directory content
 *
 *  param:  Where to start in the list, total list length, pointer to dir info
 *  return: Nothing
 */
static void text_dir_output(int list_start, int list_length, dir_entry_t *directory_list)
{
    int     row;

    for ( row = 0; row <= TERMINAL_LIST_LENGTH; row++)
    {
        if ( (row + list_start) < list_length )
        {
            if ( directory_list[(row + list_start)].is_directory )
                text_write(row, 0, "*");
            directory_list[(row + list_start)].lfn[31] = 0;
            text_write(row, 1, directory_list[(row + list_start)].lfn);
        }
    }
}

/*------------------------------------------------
 * text_clear()
 *
 *  Clear the text output area
 *
 *  param:  Nothing
 *  return: Nothing
 */
static void text_clear(void)
{
    int     i;

    text_highlight_off();
    for ( i = 0; i < TERMINAL_STATUS_ROW; i++)
        text_write(i, 0, "                                ");
}
