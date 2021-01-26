/********************************************************************
 * mem.c
 *
 *  Memory module interface
 *
 *  July 2, 2020
 *
 *******************************************************************/

#include    <string.h>

#include    "config.h"
#include    "mem.h"
#include    "rpi.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */

#define     IO_HANDLERS             256         // IO device handler callbacks

#define     MEM_FLAG_ROM            0x80000000  // Memory location is read-only
#define     MEM_FLAG_IO             0x40000000  // Memory location is a registered memory mapped IO device
#define     MEM_MASK_CALLBACK       0x00ff0000  // Index to list of 256 IO device handler callbacks
#define     MEM_MASK_DATA           0x000000ff  // Mask data bits
#define     MEM_MASK_FLAGS          (MEM_FLAG_ROM | MEM_FLAG_IO)

/* -----------------------------------------
   Module static functions
----------------------------------------- */
static void do_nothing_io_handler(int addr, int data);

/* -----------------------------------------
   Module globals
----------------------------------------- */

/* b07..b00  8 least significant bits are memory location data
 * b23..b16  8 bits holding index to a list of 256 callbacks for memory-mapped IO device handlers  
 * b31..b24  8 most significant bit are flags
 */
static uint32_t    memory[MEMORY];
static void        (*io_handlers[IO_HANDLERS])(int, int);

/*------------------------------------------------
 * mem_init()
 *
 *  Initialize the memory module
 *
 *  param:  Nothing
 *  return: Nothing
 */
void mem_init(void)
{
    int i;

    memset(memory, 0, sizeof(memory));
    for (i = 0; i < IO_HANDLERS; i++)
        io_handlers[i] = do_nothing_io_handler;
}

/*------------------------------------------------
 * mem_read()
 *
 *  Read memory address
 *
 *  param:  Memory address
 *  return: Memory content at address
 *          '-1' if address range error
 */
int mem_read(int address)
{
    if ( address < 0 || address > (MEMORY-1) )
        return MEM_ADD_RANGE;

    return (int)(memory[address] & MEM_MASK_DATA);
}

/*------------------------------------------------
 * mem_write()
 *
 *  Write to memory address
 *
 *  param:  Memory address and data to write
 *  return: ' 0' - write ok,
 *          '-1' - memory location is out of range
 *          '-2' - memory location is ROM
 */
int mem_write(int address, int data)
{
    register uint32_t   memory_cell;
    register int        callback_index;

    if ( address < 0 || address > (MEMORY-1) )
        return MEM_ADD_RANGE;

    if ( memory[address] & MEM_FLAG_ROM )
        return MEM_ROM;

    memory_cell = ((memory[address] & ~MEM_MASK_DATA) | ((uint32_t)data & MEM_MASK_DATA));
    memory[address] = memory_cell;

    if ( memory_cell & MEM_FLAG_IO )
    {
        callback_index = (int)((data & MEM_MASK_CALLBACK) >> 16);
        io_handlers[callback_index](address, (int)(memory[address] & MEM_MASK_DATA));
    }

    return MEM_OK;
}

/*------------------------------------------------
 * mem_define_rom()
 *
 *  Define address range as ROM
 *
 *  param:  Memory address range start and end
 *  return: ' 0' - write ok,
 *          '-1' - memory location is out of range
 */
int  mem_define_rom(int addr_start, int addr_end)
{
    int i;

    if ( addr_start < 0 || addr_start > (MEMORY-1) ||
         addr_end < 0   || addr_end > (MEMORY-1)   ||
         addr_start > addr_end )
        return MEM_ADD_RANGE;

    disable();

    for (i = addr_start; i < addr_end; i++)
        memory[i] |= MEM_FLAG_ROM;

    enable();

    return MEM_OK;
}

/*------------------------------------------------
 * mem_define_io()
 *
 *  Define IO device address range and callback handler
 *
 *  param:  Memory address range start and end, IO handler callback for the range
 *  return: ' 0' - write ok,
 *          '-1' - memory location is out of range
 */
int  mem_define_io(int addr_start, int addr_end, void (*io_handler)(int, int))
{
    int i;

    if ( addr_start < 0 || addr_start > (MEMORY-1) ||
         addr_end < 0   || addr_end > (MEMORY-1)   ||
         addr_start > addr_end )
        return MEM_ADD_RANGE;

    disable();

    for (i = addr_start; i < addr_end; i++)
    {
        /* TODO Hook IO callback */
        memory[i] |= MEM_FLAG_IO;
    }

    enable();

    return MEM_OK;
}

/*------------------------------------------------
 * mem_load()
 *
 *  Load a memory range from a data buffer.
 *  Function clears IO, ROM, and callback index fields.
 *
 *  param:  Memory address start, source data buffer and
 *          number of data elements to load
 *  return: ' 0' - write ok,
 *          '-1' - memory location is out of range
 */
int mem_load(int addr_start, int *buffer, int length)
{
    int i;

    if ( addr_start < 0 || addr_start > (MEMORY-1) ||
         (addr_start + length) > MEMORY )
        return MEM_ADD_RANGE;

    disable();

    for (i = 0; i < length; i++)
    {
        memory[(i+addr_start)] = (uint32_t)buffer[i] & MEM_MASK_DATA;
    }

    enable();

    return MEM_OK;
}

/*------------------------------------------------
 * do_nothing_io_handler()
 *
 *  A default do-nothing IO handler
 *
 *  param:  Nothing
 *  return: Nothing
 */
static void do_nothing_io_handler(int addr, int data)
{
    /* do nothing */
    /* TODO generate an exception? */
}

