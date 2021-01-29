/********************************************************************
 * mon09.c
 *
 *  MC6809E CPU emulation running SBUG monitor, main module.
 *  Use the Raspberry Pi default serial port "/dev/serial0"
 *  as the monitor connection.
 *  See README.md in the include/mon09 directory
 *
 *  January 29, 2021
 *
 *******************************************************************/

#include    <stdio.h>

#include    "config.h"
#include    "mem.h"
#include    "cpu.h"
#include    "rpi.h"
#include    "uart.h"

/* -----------------------------------------
   Include file for MC6909E test code
----------------------------------------- */
#include    "mon09/sbug.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     CALLBACK_TRACE      1

#define     SBUG_ROM_START      LOAD_ADDRESS
#define     SBUG_ROM_END        0xffff

#define     IO_ADDR_IC11        0xfff0
#define     IO_ADDR_ACIA_CS     0xe004
#define     IO_ADDR_ACIA_DAT    0xe005

/* -----------------------------------------
   Module functions
----------------------------------------- */
uint8_t io_handler_IC11(uint16_t address, uint8_t data, mem_operation_t op);
uint8_t io_handler_ACIA6850(uint16_t address, uint8_t data, mem_operation_t op);

void print_registers(cpu_state_t* state);
void print_decorated_cc(uint8_t cc);

/*------------------------------------------------
 * main()
 *
 */
int main(int argc, char *argv[])
{
    int             i;
    uint16_t        break_point = 0xffff;
    cpu_state_t     cpu_state;

    if ( uart_init() != 0 )
        return -1;

    printf("Loading code... ");
    i = 0;
    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }
    printf("Loaded %i bytes.\n", i - 1);

    mem_define_rom(SBUG_ROM_START, SBUG_ROM_END);

    mem_define_io(IO_ADDR_IC11, IO_ADDR_IC11, io_handler_IC11);
    mem_define_io(IO_ADDR_ACIA_CS, IO_ADDR_ACIA_DAT, io_handler_ACIA6850);

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    //break_point = 0xf83f;

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
    }
    while ( cpu_state.pc != break_point );

    printf("Stopped at breakpoint.\n");

    while ( 1 )
    {
        getchar();

        cpu_run();

        cpu_get_state(&cpu_state);
        print_registers(&cpu_state);
    }

    uart_close();

    return 0;
}

/* -----------------------------------------
   IO handler call-back functions
----------------------------------------- */

/*------------------------------------------------
 * io_handler_IC11()
 *
 *  IO call-back handler for the 74S189 IC memory page decoder
 *  on the SWTPC computer main board.
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Content of the register
 */
uint8_t io_handler_IC11(uint16_t address, uint8_t data, mem_operation_t op)
{
    if ( address != IO_ADDR_IC11 )
        printf("EXCEPTION: %s access IC11\n", ((op == MEM_READ) ? "read" : "write"));
    else
        printf("IC11 %s register access (0x%02x)\n", ((op == MEM_READ) ? "read" : "write"), data);

    return data;
}

/*------------------------------------------------
 * io_handler_ACIA6850()
 *
 *  IO call-back handler for the ACIA serial device 6850.
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
uint8_t io_handler_ACIA6850(uint16_t address, uint8_t data, mem_operation_t op)
{
    static uint8_t  control = 0;
    static uint8_t  status = 0;
    static uint8_t  tx_data = 0;
    static uint8_t  rx_data = 0;

    int             serial_input;
    uint8_t         response = 0;

    if ( address == IO_ADDR_ACIA_CS )
    {
        /* Command register
         * bit  function
         * 0-1  Baud rate (0=CLK/64, 1=CLK/16, 2=CLK/1, 3=RESET)
         * 2-4  Mode (0..7 = 7e2,7o2,7e1,7o1,8n2,8n1,8e1,8o1) (data/stop bits, parity)
         * 5-6  Transmit Interrupt/RTS/Break control (0..3)
         *      0 = Output /RTS=low,  and disable Tx Interrupt
         *      1 = Output /RTS=low,  and enable Tx Interrupt
         *      2 = Output /RTS=high, and disable Tx Interrupt
         *      3 = Output /RTS=low,  and disable Tx Interrupt, and send a Break
         * 7    Receive Interrupt (1=Enable on buffer full/buffer overrun)
         */
        if ( op == MEM_WRITE )
        {
            control = data; // Only save and ignore, the host serial link will have its own configuration

            #if ( CALLBACK_TRACE )
            printf("ACIA control register write 0x%02x\n", control);
            #endif
        }

        /* Status register
         * bit  function
         * 0    Receive Data  (0=No data, 1=Data can be read)
         * 1    Transmit Data (0=Busy, 1=Ready/Empty, Data can be written)
         * 2    /DCD level
         * 3    /CTS level
         * 4    Receive Framing Error (1=Error)
         * 5    Receive Overrun Error (1=Error)
         * 6    Receive Parity Error  (1=Error)
         * 7    Interrupt Flag (see Control Bits 5-7) (IRQ pin is not connected)
         */
        else
        {
            status |= 0x02;                             // Always ok to transmit
            if ( (serial_input = uart_recv()) != -1 )   // Check if character received
            {
                rx_data = (uint8_t) serial_input;
                status |= 0x01;                         // Set character ready
            }
            response = status;

            #if ( CALLBACK_TRACE )
            // Commented to reduce extra output
            //printf("ACIA status register read 0x%02x\n", status);
            #endif
        }
    }

    else if ( address == IO_ADDR_ACIA_DAT )
    {
        if ( op == MEM_WRITE )
        {
            tx_data = data;
            uart_send(tx_data);

            #if ( CALLBACK_TRACE )
            printf("ACIA transmit byte 0x%02x\n", tx_data);
            #endif
        }
        else
        {
            response = rx_data;
            status &= 0xfe;

            #if ( CALLBACK_TRACE )
            printf("ACIA receive byte 0x%02x\n", rx_data);
            #endif
        }
    }

    else
        printf("EXCEPTION: %s access ACIA address 0x%04x\n", ((op == MEM_READ) ? "read" : "write"), address);

    return response;
}

/* -----------------------------------------
   Miscellaneous utility functions
----------------------------------------- */

/*------------------------------------------------
 * print_registers()
 *
 *  Print CPU state from CPU state structure.
 *
 *  param:  Pointer to CPU state structure
 *  return: None
 */
void print_registers(cpu_state_t* state)
{
    int     i, bytes;

    bytes = state->last_opcode_bytes;

    /* Print opcode mnemonic at PC
     */
    printf("(%s) ", cpu_get_menmonic(state->last_pc));

    /* Print opcode and operand bytes
     */
    for ( i = 0; i < bytes; i++ )
    {
        printf("%02x ", mem_read(state->last_pc + i));
    }

    /* Print register content resulting from execution
     */
    printf("\n            a=0x%02x b=0x%02x dp=0x%02x ", state->a, state->b, state->dp);
    print_decorated_cc(state->cc);
    printf("\n");
    printf("            x=0x%04x y=0x%04x u=0x%04x s=0x%04x\n", state->x, state->y, state->u, state->s);

    /* Stopped at next PC to execute
     */
    printf("PC=0x%04x | ", state->pc);
}

/*------------------------------------------------
 * print_decorated_cc()
 *
 *  Print CPU flags as upper (set) / lower (clear) case characters.
 *
 *  param:  CC flags value packed byte
 *  return: None
 */
void print_decorated_cc(uint8_t cc)
{
    static  char cc_flag_set[] = {'C', 'V', 'Z', 'N', 'I', 'H', 'F', 'E'};
    static  char cc_flag_clr[] = {'c', 'v', 'z', 'n', 'i', 'h', 'f', 'e'};

    int     i;

    for ( i = 7; i > -1; i--)
    {
        if ( (1 << i) & cc )
            printf("%c", cc_flag_set[i]);
        else
            printf("%c", cc_flag_clr[i]);
    }
}
