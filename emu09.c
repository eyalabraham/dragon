/********************************************************************
 * emu09.c
 *
 *  MC6809E CPU emulation, main module.
 *  Use this module to test and run MC6809E CPU emulation and machine code.
 *  See README.md in the include/test directory
 *
 *  January 8, 2021
 *
 *******************************************************************/

#include    <stdio.h>

#include    "mem.h"
#include    "cpu.h"

/* -----------------------------------------
   Include file for MC6909E test code
----------------------------------------- */
//#include    "test/arith.h"
//#include    "test/logic.h"
//#include    "test/misc.h"
//#include    "test/stack.h"
//#include    "test/addr.h"
//#include    "test/branch.h"
#include    "test/swi.h"

/* -----------------------------------------
   Module functions
----------------------------------------- */
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

    /* Load test code
     */
    printf("Loading code. ");

    while ( code[i] != -1 )
    {
        mem_write(i + LOAD_ADDRESS, code[i]);
        i++;
    }

    printf("Loaded %i bytes.\n", i - 1);

    /* Initialize CPU
     */
    printf("Initializing CPU.\n");
    cpu_init(RUN_ADDRESS);

    printf("Breakpoint set at 0x%04x.\n", (i + LOAD_ADDRESS - 1));
    break_point = (i + LOAD_ADDRESS - 1);

    /* Execution loop
     */
    printf("Press <CR> for single step.\n");
    printf("Starting CPU.\n");

    printf("PC=0x%04x | ", RUN_ADDRESS);

    do
    {
        cpu_run();
        cpu_get_state(&cpu_state);
        print_registers(&cpu_state);

        /* Simple way to manually single-step
         */
        getchar();
    }
    while ( cpu_state.pc != break_point );

    printf("Stopped at breakpoint.\n");

    return 0;
}

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
