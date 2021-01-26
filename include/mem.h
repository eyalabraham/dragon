/********************************************************************
 * mem.h
 *
 *  Header file that defines the memory module interface
 *
 *  July 2, 2020
 *
 *******************************************************************/

#ifndef __MEM_H__
#define __MEM_H__

#include    <stdint.h>

#define     MEMORY                  65536       // 64K Byte

#define     MEM_OK                  0           // Operation ok
#define     MEM_ADD_RANGE          -1           // Address out of range
#define     MEM_ROM                -2           // Location is ROM

/********************************************************************
 *  Memory module API
 */

void mem_init(void);

int  mem_read(int address);
int  mem_write(int address, int data);
int  mem_define_rom(int addr_start, int addr_end);
int  mem_define_io(int addr_start, int addr_end, void (*io_handler)(int, int));
int  mem_load(int addr_start, int *buffer, int length);

#endif  /* __MEM_H__ */
