/********************************************************************
 * pia.c
 *
 *  Module that implements the MC6821 PIA functionality.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdint.h>

#include    "rpi.h"
#include    "mem.h"
#include    "vdg.h"
#include    "pia.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     PIA0_PA     0xff00
#define     PIA0_PB     0xff02
#define     PIA1_PA     0xff20
#define     PIA1_PB     0xff22

#define     KBD_ROWS    7

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static uint8_t io_handler_pia0_pb(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t io_handler_pia1_pb(uint16_t address, uint8_t data, mem_operation_t op);
static uint8_t get_keyboard_column_scan(uint8_t data);

/* -----------------------------------------
   Module globals
----------------------------------------- */
/*
    Dragon keyboard map

          LSB              $FF02                    MSB
        | PB0   PB1   PB2   PB3   PB4   PB5   PB6   PB7 | <- column
    ----|-----------------------------------------------|-----------
    PA0 |   0     1     2     3     4     5     6     7 |   LSB
    PA1 |   8     9     :     ;     ,     -     .     / |
    PA2 |   @     A     B     C     D     E     F     G |
    PA3 |   H     I     J     K     L     M     N     O | $FF00
    PA4 |   P     Q     R     S     T     U     V     W |
    PA5 |   X     Y     Z    Up  Down  Left Right Space |
    PA6 | ENT   CLR   BRK   N/C   N/C   N/C   N/C  SHFT |
    PA7 | Comparator input                              |   MSB
*/
uint8_t scan_code_table[85][2] = {
        // Column     Row
        { 0xff,       255 }, // #0
        { 0xff,       255 },
        { 0b11111101,   0 }, //      1
        { 0b11111011,   0 }, //      2
        { 0b11110111,   0 }, //      3
        { 0b11101111,   0 }, //      4
        { 0b11011111,   0 }, //      5
        { 0b10111111,   0 }, //      6
        { 0b01111111,   0 }, //      7
        { 0b11111110,   1 }, //      8
        { 0b11111101,   1 }, // #10  9
        { 0b11111110,   0 }, //      0
        { 0b11011111,   1 }, //      -
        { 0b11111011,   1 }, //      :
        { 0b11111101,   6 }, //      CLEAR
        { 0xff,       255 },
        { 0b11111101,   4 }, //      Q
        { 0b01111111,   4 }, //      W
        { 0b11011111,   2 }, //      E
        { 0b11111011,   4 }, //      R
        { 0b11101111,   4 }, // #20  T
        { 0b11111101,   5 }, //      Y
        { 0b11011111,   4 }, //      U
        { 0b11111101,   3 }, //      I
        { 0b01111111,   3 }, //      O
        { 0b11111110,   4 }, //      P
        { 0b11111110,   2 }, //      @
        { 0xff,       255 },
        { 0b11111110,   6 }, //      Enter
        { 0xff,       255 },
        { 0b11111101,   2 }, // #30  A
        { 0b11110111,   4 }, //      S
        { 0b11101111,   2 }, //      D
        { 0b10111111,   2 }, //      F
        { 0b01111111,   2 }, //      G
        { 0b11111110,   3 }, //      H
        { 0b11111011,   3 }, //      J
        { 0b11110111,   3 }, //      K
        { 0b11101111,   3 }, //      L
        { 0b11110111,   1 }, //      ;
        { 0xff,       255 }, // #40
        { 0xff,       255 },
        { 0b01111111,   6 }, //      Shift key
        { 0xff,       255 },
        { 0b11111011,   5 }, //      Z
        { 0b11111110,   5 }, //      X
        { 0b11110111,   2 }, //      C
        { 0b10111111,   4 }, //      V
        { 0b11111011,   2 }, //      B
        { 0b10111111,   3 }, //      N
        { 0b11011111,   3 }, // #50  M
        { 0b11101111,   1 }, //      ,
        { 0b10111111,   1 }, //      .
        { 0b01111111,   1 }, //      /
        { 0xff,       255 },
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b01111111,   5 }, //      Space bar
        { 0xff,       255 },
        { 0xff,       255 }, //      F1
        { 0xff,       255 }, // #60  F2
        { 0xff,       255 }, //      F3
        { 0xff,       255 }, //      F4
        { 0xff,       255 }, //      F5
        { 0xff,       255 }, //      F6
        { 0xff,       255 }, //      F7
        { 0xff,       255 }, //      F8
        { 0xff,       255 }, //      F9
        { 0xff,       255 }, //      F10
        { 0xff,       255 },
        { 0xff,       255 }, // #70
        { 0xff,       255 },
        { 0b11110111,   5 }, //      Up arrow
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b11011111,   5 }, //      Left arrow
        { 0xff,       255 },
        { 0b10111111,   5 }, //      Right arrow
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b11101111,   5 }, // #80  Down arrow
        { 0xff,       255 },
        { 0xff,       255 },
        { 0xff,       255 },
        { 0b11111011,   6 }, // #84  Break
};

uint8_t keyboard_rows[KBD_ROWS] = {
        255,    // row PIA0_PA0
        255,    // row PIA0_PA1
        255,    // row PIA0_PA2
        255,    // row PIA0_PA3
        255,    // row PIA0_PA4
        255,    // row PIA0_PA5
        255,    // row PIA0_PA6
};

/*------------------------------------------------
 * pia_init()
 *
 *  Initialize the PIA device
 *
 *  param:  Nothing
 *  return: Nothing
 */
void pia_init(void)
{
    /* Initialize keyboard interface to the PS2 keyboard and AVR
     * through the serial link (SPI)
     */
    if ( rpi_keyboard_init() == -1 )
    {
        emu_assert(0 && "Keyboard initialization error pia_init()");
    }

    /* Link IO call-backs
     */
    mem_write(PIA0_PA, 0xff);
    mem_define_io(PIA0_PB, PIA0_PB, io_handler_pia0_pb);

    mem_define_io(PIA1_PB, PIA1_PB, io_handler_pia1_pb);
}

/*------------------------------------------------
 * io_handler_pia0_pb()
 *
 *  IO call-back handler 0xFF02 PIA0-B Data
 *  Bit 0..7 Output to keyboard columns
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia0_pb(uint16_t address, uint8_t data, mem_operation_t op)
{
    uint8_t scan_code = 0;
    uint8_t row_switch_bits;
    int     row_index;

    /* Activate the call back to read keyboard scan code from
     * external AVR interface only when testing the keyboard columns
     * for a key press.
     */
    if ( op == MEM_WRITE )
    {
        /* When writing 0 to the port, the code is checking if any
         * key is pressed. This is done once, so a good opportunity
         * to read the keyboard scan code.
         */
        if ( data == 0 )
        {
            scan_code = (uint8_t) rpi_keyboard_read();

            if ( (scan_code & 0x7f) >= 59 && (scan_code & 0x7f) <= 68 )
            {
                // TODO handle special function keys as emulator escapes.
            }
            else if ( scan_code != 0 )
            {
                /* Sanity check
                 */
                if ( (row_index = scan_code_table[(scan_code & 0x7f)][1]) == 255 )
                    emu_assert(0 && "Illegal scan code io_handler_pia0_pb()");

                /* Generate row bit patterns emulating row key closures
                 * and match to 'make' or 'break' codes (bit.7 or scan code)
                 */
                row_switch_bits = scan_code_table[(scan_code & 0x7f)][0];

                if ( scan_code & 0x80 )
                {
                    keyboard_rows[row_index] |= ~row_switch_bits;
                }
                else
                {
                    keyboard_rows[row_index] &= row_switch_bits;
                }
            }
        }

        /* DEBUG
         */
        //printf("data 0x%02x get_keyboard_column_scan() 0x%02x\n", data, get_keyboard_column_scan(data));

        /* Store the appropriate row bit value
         * for PIA0_PA bit pattern
         */
        mem_write(PIA0_PA, (int) get_keyboard_column_scan(data));
    }

    return data;
}

/*------------------------------------------------
 * io_handler_pia1_pb()
 *
 *  IO call-back handler 0xFF22 Dir PIA1-B Data
 *  Bit 7   O   Screen Mode G/^A
 *  Bit 6   O   Screen Mode GM2
 *  Bit 5   O   Screen Mode GM1
 *  Bit 4   O   Screen Mode GM0 / INT
 *  Bit 3   O   Screen Mode CSS
 *  Bit 2   I   Ram Size (1=16k 0=32/64k), not implemented
 *  Bit 1   I   TODO Single bit sound
 *  Bit 0   I   Rs232 In / Printer Busy, not implemented
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
static uint8_t io_handler_pia1_pb(uint16_t address, uint8_t data, mem_operation_t op)
{
    vdg_set_mode_pia(((data >> 3) & 0x1f));

    return data;
}

/*------------------------------------------------
 * get_keyboard_column_scan()
 *
 *  Using the Row scan bit pattern and the key closure
 *  matrix in '', generate the column scan bit pattern
 *
 *  param:  Row scan bit pattern
 *  return: Column scan bit pattern
 */
static uint8_t get_keyboard_column_scan(uint8_t row_scan)
{
    uint8_t result = 0;
    uint8_t bit_position = 0x01;
    uint8_t test;
    int     row;

    for ( row = 0; row < KBD_ROWS; row++ )
    {
        test = (~row_scan) & keyboard_rows[row];

        if ( test == (uint8_t)(~row_scan) )
        {
            result |= bit_position;
        }

        bit_position = bit_position << 1;
    }

    return result;
}
