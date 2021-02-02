/********************************************************************
 * trace.h
 *
 *  Header file for trace functions
 *
 *  January 30, 2021
 *
 *******************************************************************/

#ifndef __TRACE_H__
#define __TRACE_H__

#include    <stdint.h>

#include    "cpu.h"

/********************************************************************
 *  Memory module API
 */

void trace_action(void);
void trace_print_registers(cpu_state_t* state);

#endif  /* __TRACE_H__ */
