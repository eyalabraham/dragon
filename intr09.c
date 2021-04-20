/********************************************************************
 * intr09.c
 *
 *  MC6809E CPU emulation for testing hardware interrupts.
 *  Use the Raspberry Pi default serial port "/dev/serial0"
 *  as the monitor connection.
 *
 *  February 6, 2021
 *
 *******************************************************************/

#include    <stdio.h>
#include    <time.h>

#include    "mem.h"
#include    "cpu.h"
#include    "uart.h"
#include    "trace.h"

/* -----------------------------------------
   Include file for MC6909E test code
----------------------------------------- */
#include    "test/irq.h"

/* -----------------------------------------
   Local definitions
----------------------------------------- */
#define     CALLBACK_TRACE      1

#define     IO_ADDR_ACIA_CS     0xf000
#define     IO_ADDR_ACIA_DAT    0xf001
#define     IO_ADDR_FIRQ_ACK    0xf002
#define     IO_ADDR_IRQ_ACK     0xf003

#define     NMI_INTERVAL        1*CLOCKS_PER_SEC
#define     FIRQ_INTERVAL       2*CLOCKS_PER_SEC
#define     IRQ_INTERVAL        4*CLOCKS_PER_SEC

/* Limit emulated ACIA to about 300 BAUD,
 * otherwise RPi serial port is overwhelmed
 * by the CPU emulation and drops characters.
 */
#define     CHAR_TIME           33333

/* -----------------------------------------
   Module functions
----------------------------------------- */
uint8_t io_handler_ACIA6850(uint16_t address, uint8_t data, mem_operation_t op);
uint8_t io_handler_firq_ack(uint16_t address, uint8_t data, mem_operation_t op);
uint8_t io_handler_irq_ack(uint16_t address, uint8_t data, mem_operation_t op);

/*------------------------------------------------
 * main()
 *
 */
int main(int argc, char *argv[])
{
    int             i;
    uint16_t        break_point = 0xffff;
    cpu_state_t     cpu_state;
    clock_t         nmi_time, firq_time, irq_time;

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

    mem_define_io(IO_ADDR_ACIA_CS, IO_ADDR_ACIA_DAT, io_handler_ACIA6850);
    mem_define_io(IO_ADDR_FIRQ_ACK, IO_ADDR_FIRQ_ACK, io_handler_firq_ack);
    mem_define_io(IO_ADDR_IRQ_ACK, IO_ADDR_IRQ_ACK, io_handler_irq_ack);

    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    //break_point = 0xf099;

    /* Execution loop.
     * Run until breakpoint, and single step from there.
     */
    printf("Starting CPU.\n");
    cpu_reset(1);

    nmi_time = firq_time = irq_time = clock();

    do
    {
        cpu_run();

        if ( cpu_get_state(&cpu_state) == CPU_RESET )
            cpu_reset(0);

        //trace_print_registers(&cpu_state);

        //trace_action();

        /* Trigger periodic interrupts.
         */
        if ( (clock() - nmi_time) > NMI_INTERVAL)
        {
            cpu_nmi_trigger();
            nmi_time = clock();
        }

        if ( (clock() - firq_time) > FIRQ_INTERVAL)
        {
            cpu_firq(1);
            firq_time = clock();
        }

        if ( (clock() - irq_time) > IRQ_INTERVAL)
        {
            cpu_irq(1);
            irq_time = clock();
        }
   }
    while ( cpu_state.pc != break_point );

    printf("Stopped at breakpoint.\n");

    uart_close();

    return 0;
}

/* -----------------------------------------
   IO handler call-back functions
----------------------------------------- */

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
    static clock_t  char_delay;

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
            if ( (status & 0x02) == 0 &&
                 (clock() - char_delay) > CHAR_TIME )   // Emulate Tx time
            {
                status |= 0x02;
            }

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
            tx_data = data;             // Data to transmit
            status &= 0xfd;             // ACIA now busy
            uart_send(tx_data);         // Send the byte, clobber if sender did not check readiness
            char_delay = clock();       // Start transmission timer

            #if ( CALLBACK_TRACE )
            printf("ACIA transmit byte 0x%02x\n", tx_data);
            #endif
        }
        else
        {
            response = rx_data;         // Read the data, garbage if reader did not check readiness
            status &= 0xfe;             // Buffer now empty

            #if ( CALLBACK_TRACE )
            printf("ACIA receive byte 0x%02x\n", rx_data);
            #endif
        }
    }

    else
        printf("EXCEPTION: %s access ACIA address 0x%04x\n", ((op == MEM_READ) ? "read" : "write"), address);

    return response;
}

/*------------------------------------------------
 * io_handler_*_ack()
 *
 *  IO call-back handler for a general device that raises IRQ and FIRQ.
 *  The device's interrupt request should be acknowledged by writing to
 *  its IO address.
 *
 *  param:  Call address, data byte for write operation, and operation type
 *  return: Status or data byte
 */
uint8_t io_handler_firq_ack(uint16_t address, uint8_t data, mem_operation_t op)
{
    cpu_firq(0);
    return data;
}

uint8_t io_handler_irq_ack(uint16_t address, uint8_t data, mem_operation_t op)
{
    cpu_irq(0);
    return data;
}

