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
#include    <string.h>

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

#define     TERMINAL_STATUS_ROW     15
#define     TERMINAL_LIST_LENGTH    (TERMINAL_STATUS_ROW-1)
#define     TERMINAL_LINE_LENGTH    31

#define     MSG_EXIT                "PRESS <Q> TO EXIT.              "
#define     MSG_STATUS              "PRESS: <UP> <DOWN> <ENTER> <Q>  "
#define     MSG_SD_ERROR            "SD CARD INITIALIZATION FAILED,  " \
                                    "REPLACE OR INSERT A CARD.       "
#define     MSG_FAT32_ERROR         "FAT32 INITIALIZATION FAILED,    " \
                                    "FIX CARD FORMATING.             "
#define     MSG_DIR_READ_ERROR      "DIRECTORY LOADING ERROR.        "
#define     MSG_ROM_READ_ERROR      "ROM IMAGE READ ERROR.           "
#define     MSG_ROM_READ_DONE       "ROM IMAGE LOAD COMPLETED.       "
#define     MSG_CAS_READ_ERROR      "CAS FILE READ ERROR.            "
#define     MSG_CAS_READ_DONE       "CAS FILE LOAD COMPLETED.        "

#define     CODE_BUFFER_SIZE        32768
#define     CARTRIDGE_ROM_BASE      0xc000
#define     CARTRIDGE_ROM_END       0xffef

#define     EXEC_VECTOR_HI          0x9d
#define     EXEC_VECTOR_LO          0x9e

#define     CAS_LEADER_BYTE         0x55
#define     CAS_SYNC_BYTE           0x3c

#define     CAS_BLOCK_NAME          0x00
#define     CAS_BLOCK_DATA          0x01
#define     CAS_BLOCK_EOF           0xff

#define     CAS_FILE_NAME_LENGTH    8
#define     CAS_FILE_TYPE_BASIC     0
#define     CAS_FILE_TYPE_DATA      1
#define     CAS_FILE_TYPE_CODE      2
#define     CAS_FILE_ASCII          0xff
#define     CAS_FILE_BINARY         0x00
#define     CAS_FILE_CONT           0x00
#define     CAS_FILE_GAPS           0xff

#define     CAS_FILES_COUNT         TERMINAL_LIST_LENGTH

typedef struct
    {
        char        name[CAS_FILE_NAME_LENGTH+1];
        int         file_type;      // see CAS_FILE_TYPE_*
        int         exec_addr;
        int         load_addr;
        uint32_t    seek_position;
    } cas_entry_t ;

typedef enum
    {
        IDLE,
        GET_BLOCK_TYPE,
        GET_BLOCK_LENGTH,
        GET_FILENAME,
        GET_FILE_TYPE,
        GET_ASCII_FLAG,
        GET_GAP_FLAG,
        GET_EXEC_ADDR,
        GET_LOAD_ADDR,
        GET_FILE_DATA,
        VERIFY_CHECKSUM,
    } process_state_t;

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
static file_type_t file_get_type(char *directory_entry);

static void        text_write(int row, int col, char *text);
static void        text_highlight(int on_off, int row);
static void        text_dir_output(int list_start, int list_length, dir_entry_t *directory_list);
static void        text_clear(void);
static void        text_cas_list_output(int list_length, cas_entry_t *cas_file_list);

static void        util_wait_quit(void);
static void        util_save_text_screen(void);
static void        util_restore_text_screen(void);
static int         util_parse_cas_file(cas_entry_t *cas_file_list);
static int         util_read_cas_object(uint8_t *code_buffer, int buffer_length);

/* -----------------------------------------
   Module globals
----------------------------------------- */
static int      sd_card_initialized = 0;
static uint8_t  text_screen_save[512];
static uint8_t  code_buffer[CODE_BUFFER_SIZE];
static char    *file_type_str[] = {"BASIC", "DATA", "BINARY"};

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
    cas_entry_t     cas_file_list[CAS_FILES_COUNT];
    int             list_start, prev_list_start, list_length;
    int             highlighted_line;
    int             is_file_list;

    file_type_t     file_type;

    printf("Entering loader()\n");

    util_save_text_screen();

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

            util_wait_quit();
            util_restore_text_screen();
            return;
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

        util_wait_quit();
        util_restore_text_screen();
        return;
    }

    /* Initial directory load
     */
    if ( (list_length = fat32_parse_dir(2, directory_list, FAT32_MAX_DIR_LIST)) == -1 )
    {
        sd_card_initialized = 0;

        text_write(0, 0, MSG_DIR_READ_ERROR);
        text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

        util_wait_quit();
        util_restore_text_screen();
        return;
    }

    /* Main loop.
     */
    list_start = 0;
    prev_list_start = -1;
    highlighted_line = 0;
    is_file_list = 1;
    text_dir_output(list_start, list_length, directory_list);
    text_write(TERMINAL_STATUS_ROW, 0, MSG_STATUS);

    for (;;)
    {
        vdg_render();

        key_pressed = rpi_keyboard_read();

        if ( key_pressed == SCAN_CODE_Q )
        {
            /* Quit the loader
             */
            break;
        }
        else if ( key_pressed == SCAN_CODE_UP )
        {
            /* Highlight one line up
             * scroll the list down or stop if at top of list
             */
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
            /* Highlight one line down
             * scroll the list up or stop if at end of list
             */
            highlighted_line++;
            if ( highlighted_line == list_length )
            {
                highlighted_line--;
            }
            else if ( highlighted_line > TERMINAL_LIST_LENGTH )
            {
                highlighted_line = TERMINAL_LIST_LENGTH;
                list_start++;
                if ( (list_length - list_start) < (TERMINAL_LIST_LENGTH + 1) )
                    list_start = list_length - (TERMINAL_LIST_LENGTH + 1);
            }
        }
        else if ( key_pressed == SCAN_CODE_ENTR )
        {
            text_clear();

            if ( is_file_list == 0 )
            {
                /* Load a selected object from a CAS file
                 */
                fat32_fseek(cas_file_list[highlighted_line].seek_position);

                /* Read CAS object into buffer
                 */
                rom_bytes = util_read_cas_object(code_buffer, CODE_BUFFER_SIZE);
                fat32_fclose();

                /* Load into memory and change ECEC vector
                 */
                if ( rom_bytes == 0 )
                {
                    text_write(0, 0, MSG_CAS_READ_ERROR);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);
                }
                else
                {
                    /* The memory load action will have the effect of overwriting the
                     * Dragon text buffer as some cassette loads used to do as a way to display
                     * an opening screen for a game.
                     * TODO consider copying the text buffer segment also into the
                     *      text buffer save area 'text_screen_save[]'
                     */
                    mem_load(cas_file_list[highlighted_line].load_addr, code_buffer, rom_bytes);

                    mem_write(EXEC_VECTOR_HI, (cas_file_list[highlighted_line].exec_addr >> 8) & 0xff);
                    mem_write(EXEC_VECTOR_LO, cas_file_list[highlighted_line].exec_addr & 0xff);

                    text_write(0, 0, MSG_CAS_READ_DONE);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);
                }

                util_wait_quit();
                break;
            }
            else if ( is_file_list && directory_list[(list_start + highlighted_line)].is_directory )
            {
                /* Read and display the directory
                 */
                if ( (list_length = fat32_parse_dir(directory_list[(list_start + highlighted_line)].cluster_chain_head,
                                                    directory_list, FAT32_MAX_DIR_LIST)) == -1 )
                {
                    sd_card_initialized = 0;

                    text_write(0, 0, MSG_DIR_READ_ERROR);
                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

                    util_wait_quit();
                    break;
                }

                list_start = 0;
                prev_list_start = 0;
                highlighted_line = 0;
                text_dir_output(list_start, list_length, directory_list);
            }
            else
            {
                /* Handle .ROM and .CAS extensions ignore
                 * all other file types
                 */
                file_type = file_get_type(directory_list[(list_start + highlighted_line)].lfn);

                if ( file_type == FILE_ROM )
                {
                    /* Load ROM image into emulator memory
                     * and change EXEC default vector to 0xC000
                     */
                    fat32_fopen(&directory_list[(list_start + highlighted_line)]);
                    rom_bytes = fat32_fread(code_buffer, CODE_BUFFER_SIZE);
                    fat32_fclose();

                    if ( rom_bytes == -1 )
                    {
                        text_write(0, 0, MSG_ROM_READ_ERROR);
                        text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);
                    }
                    else
                    {
                        mem_load(CARTRIDGE_ROM_BASE, code_buffer, rom_bytes);
                        mem_define_rom(CARTRIDGE_ROM_BASE, (rom_bytes - 1));
                        mem_write(EXEC_VECTOR_HI, 0xc0);
                        mem_write(EXEC_VECTOR_LO, 0x00);

                        text_write(0, 0, MSG_ROM_READ_DONE);
                        text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);
                    }

                    util_wait_quit();
                    break;
                }
                else if ( file_type == FILE_CAS )
                {
                    /* Read, parse and load first program in a CAS file.
                     * Adjust default EXEC vector etc
                     * Process will load first program in a CAS file
                     * Parse and present list of programs (objects) if more than one is in a CAS file.
                     */
                    fat32_fopen(&directory_list[(list_start + highlighted_line)]);

                    list_start = 0;
                    prev_list_start = 0;
                    highlighted_line = 0;

                    list_length = util_parse_cas_file(cas_file_list);

                    text_cas_list_output(list_length, cas_file_list);

                    text_write(TERMINAL_STATUS_ROW, 0, MSG_EXIT);

                    is_file_list = 0;   // The list is now a CAS file object list
                }
                else
                {
                    // ** Do nothing
                }
            }
        }

        if ( list_start != prev_list_start )
        {
            text_clear();
            if ( is_file_list )
            {
                text_dir_output(list_start, list_length, directory_list);
            }
            else
            {
                /* Nothing to do here since 'cas_file_list[]' is always
                 * less than or equal in length to screen row count.
                 * Otherwise we would output it here to update list
                 * scrolling.
                 */
            }
            prev_list_start = list_start;
        }

        text_highlight_on(highlighted_line);
    }

    util_restore_text_screen();

    printf("Exiting loader()\n");
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
static file_type_t file_get_type(char *directory_entry)
{
    if ( strstr(directory_entry, ".ROM") || strstr(directory_entry, ".rom") )
    {
        return FILE_ROM;
    }
    else if ( strstr(directory_entry, ".CAS") || strstr(directory_entry, ".cas") )
    {
        return FILE_CAS;
    }

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
    char    text_line[(TERMINAL_LINE_LENGTH + 1)] = {0};

    for ( row = 0; row <= TERMINAL_LIST_LENGTH; row++)
    {
        if ( (row + list_start) < list_length )
        {
            if ( directory_list[(row + list_start)].is_directory )
                text_write(row, 0, "*");
            strncpy(text_line, directory_list[(row + list_start)].lfn, TERMINAL_LINE_LENGTH);
            text_line[(TERMINAL_LINE_LENGTH)] = 0;
            text_write(row, 1, text_line);
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

/*------------------------------------------------
 * text_cas_list_output()
 *
 *  Print the CAS file content to the screen
 *
 *  param:  Number of items, pointer to CAS file contents list
 *  return: Nothing
 */
static void text_cas_list_output(int list_length, cas_entry_t *cas_file_list)
{
    int     row;
    char    text_line[(TERMINAL_LINE_LENGTH + 1)] = {0};

    for ( row = 0; row < list_length; row++)
    {
            strncat(text_line, cas_file_list[row].name, CAS_FILE_NAME_LENGTH);
            //strncat(text_line, " ", 1);
            strcat(text_line, " ");
            strcat(text_line, file_type_str[cas_file_list[row].file_type]);
            text_write(row, 1, text_line);
            text_line[0] = 0;
    }
}

/*------------------------------------------------
 * util_wait_quit()
 *
 *  Block and wait for 'Q' key to be pressed in keyboard.
 *
 *  param:  Nothing
 *  return: Nothing
 */
static void util_wait_quit(void)
{
    int     key_pressed;

    do
    {
        vdg_render();

        key_pressed = rpi_keyboard_read();
    }
    while ( key_pressed != SCAN_CODE_Q );
}

/*------------------------------------------------
 * util_save_text_screen()
 *
 *  Save Dragon text screen buffer.
 *  Uses 'text_screen_save' global buffer.
 *
 *  param:  Nothing
 *  return: Nothing
 */
static void util_save_text_screen(void)
{
    int     i;

    for ( i = 0; i < 512; i++ )
    {
        text_screen_save[i] = mem_read(0x400 + i);
        mem_write(0x400 + i, 32);
    }
}

/*------------------------------------------------
 * util_restore_text_screen()
 *
 *  Restore Dragon text screen buffer.
 *  Uses 'text_screen_save' global buffer.
 *
 *  param:  Nothing
 *  return: Nothing
 */
static void util_restore_text_screen(void)
{
    int     i;

    for ( i = 0; i < 512; i++ )
    {
        mem_write(0x400 + i, text_screen_save[i]);
    }
}

/*------------------------------------------------
 * util_parse_cas_file()
 *
 *  Parse an open CAS file and extract parameters
 *  of its contents. A single CAS file can contain one or more
 *  code or data files. Return a list of up to 14 of them.
 *  Dragon 32 CAS format:
 *    https://archive.worldofdragon.org/index.php?title=Tape%5CDisk_Preservation#CAS_File_Format
 *
 *  param:  Pointer to CAS file content list
 *  return: Count list, 0=error
 */
static int util_parse_cas_file(cas_entry_t *cas_file_list)
{
    uint8_t         byte;
    int             error;
    int             i, j, k;
    uint8_t         check_sum;
    process_state_t state;
    int             block_type;
    int             block_length;
    int             file_type;
    int             exec_addr;
    int             load_addr;
    char            filename[CAS_FILE_NAME_LENGTH+1];

    int             count;

    /* Process the CAS file
     */
    error = 0;
    i = 0;
    j = 0;
    k = 0;
    check_sum = 0;
    state = IDLE;
    block_type = -1;
    block_length = -1;
    file_type = -1;
    exec_addr = -1;
    load_addr = -1;
    filename[0] = 0;

    count = 0;

    printf("Parsing CAS file.\n");

    for (;;)
    {
        if( fat32_fread(&byte, 1) == 0 )
            break;
        i++;

        switch ( state )
        {
            case IDLE:
                if ( byte == CAS_LEADER_BYTE )
                {
                    // ** Do nothing **
                }
                else if ( byte == CAS_SYNC_BYTE )
                {
                    state = GET_BLOCK_TYPE;
                }
                else
                {
                    printf("  Unexpected input at file offset 0x%04x\n", (i - 1));
                    error = 1;
                }
                break;

            case GET_BLOCK_TYPE:
                block_type = (int) byte;
                check_sum += byte;
                state = GET_BLOCK_LENGTH;
                break;

            case GET_BLOCK_LENGTH:
                block_length = (int) byte;
                check_sum += byte;
                if ( block_type == CAS_BLOCK_NAME )
                {
                    j = 0;
                    cas_file_list[count].seek_position = i - 4; // Point to one byte before SYNC byte
                    state = GET_FILENAME;
                }
                else if ( block_type == CAS_BLOCK_DATA )
                {
                    j = 0;
                    k = 0;
                    state = GET_FILE_DATA;
                }
                else if ( block_type == CAS_BLOCK_EOF )
                {
                    state = VERIFY_CHECKSUM;
                }
                else
                {
                    printf("  Bad block type.\n");
                    error = 1;
                }
                break;

            case GET_FILENAME:
                filename[j] = byte;
                check_sum += byte;
                j++;

                if ( j == CAS_FILE_NAME_LENGTH )
                {
                    filename[j] = 0;
                    memcpy(cas_file_list[count].name, filename, (CAS_FILE_NAME_LENGTH+1));
                    printf("  File [%d]: %s 0x%04x", count, filename, cas_file_list[count].seek_position);
                    state = GET_FILE_TYPE;
                }
                break;

            case GET_FILE_TYPE:
                file_type = (int) byte;
                check_sum += byte;
                cas_file_list[count].file_type = file_type;
                state = GET_ASCII_FLAG;
                break;

            case GET_ASCII_FLAG:
                check_sum += byte;
                state = GET_GAP_FLAG;
                break;

            case GET_GAP_FLAG:
                check_sum += byte;
                j = 0;
                state = GET_EXEC_ADDR;
                break;

            case GET_EXEC_ADDR:
                check_sum += byte;
                if ( j == 0 )
                {
                    exec_addr = (int) byte * 256;
                    j++;
                }
                else
                {
                    exec_addr += (int) byte;
                    cas_file_list[count].exec_addr = exec_addr;
                    printf(" 0x%04x", exec_addr);
                    j = 0;
                    state = GET_LOAD_ADDR;
                }
                break;

            case GET_LOAD_ADDR:
                check_sum += byte;
                if ( j == 0 )
                {
                    load_addr = (int) byte * 256;
                    j++;
                }
                else
                {
                    load_addr += (int) byte;
                    cas_file_list[count].load_addr = load_addr;
                    printf(" 0x%04x\n", load_addr);
                    j = 0;
                    state = VERIFY_CHECKSUM;
                }
                break;

            case GET_FILE_DATA:
                check_sum += byte;
                j++;
                if ( j == block_length )
                {
                    k++;
                    state = VERIFY_CHECKSUM;
                }
                break;

            case VERIFY_CHECKSUM:
                if ( check_sum != byte )
                {
                    printf("  Checksum mismatch %d != %d\n", check_sum, byte);
                    error = 1;
                }
                else if ( block_type == CAS_BLOCK_EOF )
                {
                    count++;
                }
                check_sum = 0;
                state = IDLE;
                break;
        }

        if ( error )
        {
            count = 0;
            break;
        }

        if ( count == CAS_FILES_COUNT )
        {
            break;
        }
    }

    printf("  Processed %d bytes, %d object(s).\n", i, count);

    return count;
}

/*------------------------------------------------
 * util_read_cas_object()
 *
 *  Read an open CAS file and extract the object
 *  pointed to by the CAS file entry.
 *  The CAS file read position should already point to where
 *  the object starts with fas32_fseek()
 *  Return the object content into a buffer and return number
 *  of bytes written to it.
 *  Dragon 32 CAS format:
 *    https://archive.worldofdragon.org/index.php?title=Tape%5CDisk_Preservation#CAS_File_Format
 *
 *  param:  Pointer to CAS file content list
 *  return: Byte count loaded into buffer, 0=error
 */
static int util_read_cas_object(uint8_t *code_buffer, int buffer_length)
{
    uint8_t         byte;
    int             error;
    int             i, j;
    uint8_t         check_sum;
    process_state_t state;
    int             block_type;
    int             block_length;
    int             byte_count;

    /* Process the CAS file
     */
    error = 0;
    i = 0;
    j = 0;
    check_sum = 0;
    state = IDLE;
    block_type = -1;
    block_length = -1;
    byte_count = 0;

    printf("Extracting CAS object.\n");

    for (;;)
    {
        if( fat32_fread(&byte, 1) == 0 )
            break;
        i++;

        switch ( state )
        {
            case IDLE:
                if ( byte == CAS_LEADER_BYTE )
                {
                    // ** Do nothing **
                }
                else if ( byte == CAS_SYNC_BYTE )
                {
                    state = GET_BLOCK_TYPE;
                }
                else
                {
                    printf("  Unexpected input (0x%02x) at offset 0x%04x.\n", byte, (i - 1));
                    error = 1;
                }
                break;

            case GET_BLOCK_TYPE:
                block_type = (int) byte;
                check_sum += byte;
                state = GET_BLOCK_LENGTH;
                break;

            case GET_BLOCK_LENGTH:
                block_length = (int) byte;
                check_sum += byte;
                if ( block_type == CAS_BLOCK_NAME )
                {
                    j = 0;
                    state = GET_FILENAME;
                }
                else if ( block_type == CAS_BLOCK_DATA )
                {
                    j = 0;
                    state = GET_FILE_DATA;
                }
                else if ( block_type == CAS_BLOCK_EOF )
                {
                    state = VERIFY_CHECKSUM;
                }
                else
                {
                    printf("  Bad block type.\n");
                    error = 1;
                }
                break;

            case GET_FILENAME:
                check_sum += byte;
                j++;

                if ( j == CAS_FILE_NAME_LENGTH )
                {
                    state = GET_FILE_TYPE;
                }
                break;

            case GET_FILE_TYPE:
                check_sum += byte;
                state = GET_ASCII_FLAG;
                break;

            case GET_ASCII_FLAG:
                check_sum += byte;
                state = GET_GAP_FLAG;
                break;

            case GET_GAP_FLAG:
                check_sum += byte;
                j = 0;
                state = GET_EXEC_ADDR;
                break;

            case GET_EXEC_ADDR:
                check_sum += byte;
                if ( j == 0 )
                {
                    j++;
                }
                else
                {
                    j = 0;
                    state = GET_LOAD_ADDR;
                }
                break;

            case GET_LOAD_ADDR:
                check_sum += byte;
                if ( j == 0 )
                {
                    j++;
                }
                else
                {
                    j = 0;
                    state = VERIFY_CHECKSUM;
                }
                break;

            case GET_FILE_DATA:
                code_buffer[byte_count] = byte;
                byte_count++;
                if ( byte_count == buffer_length )
                {
                    printf("  Insufficient buffer space.\n", check_sum, byte);
                    error = 1;
                }
                else
                {
                    check_sum += byte;
                    j++;
                    if ( j == block_length )
                    {
                        state = VERIFY_CHECKSUM;
                    }
                }
                break;

            case VERIFY_CHECKSUM:
                if ( check_sum != byte )
                {
                    printf("  Checksum mismatch %d != %d\n", check_sum, byte);
                    error = 1;
                }
                check_sum = 0;
                state = IDLE;
                break;
        }

        if ( error )
        {
            byte_count = 0;
            break;
        }

        if ( block_type == CAS_BLOCK_EOF )
        {
            break;
        }
    }

    printf("  Done.\n");

    return byte_count;
}
