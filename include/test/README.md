# MC6809E test code

This folder contains header files with MC6809E machine code to use as test code for the CPU emulator ```emu09.c```. Each file is a different test, and only one file should be included at a time to run a test. Each header file contains a static array of integers ('int' type) named 'code', each integer represents a memory byte value. The last integer in the array has a value of '-1' to indicate end of code. The header file contains a definition 'LOAD_ADDRESS' that designates the memory load address, and a definition for 'RUN_ADDRESS' that designates the entry point into executable machine code.

The directory also include two scripts ```lst2h.awk``` and ```lst2h.sh```. Use these scripts with the MC6809E [as9 Assembler](https://github.com/eyalabraham/as9) to write assembly code and conver it to a header file. See ```.asm``` test examples.

Load bytes from 'code' array into memory starting at address 'LOAD_ADDREDD', until encountering '-1' value. Start code execution at address 'RUN_ADDRESS'.

```
#define     LOAD_ADDRESS    0x0000      // Code load address range 0 to 65355
#define     RUN_ADDRESS     0x0000      // Execution entry point

int code[] =
{
    0x00,   // byte #1
    0x00,   // byte #1
    -1,     // end of byte list
};
```

