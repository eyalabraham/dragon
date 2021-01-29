# Dragon 32 computer emulator

This project implements the software and the hardware needed to emulate a [Dragon 32 computer](https://en.wikipedia.org/wiki/Dragon_32/64). The goal is to run the emulation on a bare-metal Raspberry Pi (RPi) platform, but development would be compiled to run under Linux. This was my first computer when home/personal computers started to emerge in the mid 80s, and it is also one of the simplest to emulate. Other emulators exist, [including XROR](http://www.6809.org.uk/xroar/), but I decided to build my own as an exercise in RPi bare-metal programming.

## Resources

- [The DRAGON Archives](https://worldofdragon.org/index.php?title=Main_Page) resources and software.
- [DRAGON Data archives](http://www.dragondata.co.uk/index.html) hardware schematics.
- [Inside the Dragon](http://www.dragondata.co.uk/Publications/InsideTheDragon.pdf) by Duncan Smeed and Ian Sommerville.

## Design

ASCII art depiction of the system for the RPi bare metal implementation:

```
  +-------------------------------------+         |
  | Monitor (via Auxiliary UART)        |         |
  |  Logger                             |         |
  |  Monitor CLI                        |         |
  +-------------------------------------+         |
     |          |                |                |
  +-----+  +----------+  +--------------+  Software emulation
  | CPU |--|   MEM    |--|      IO      |         |
  +-----+  +----------+  |              |         |
                |        |              |         |
           +----------+  |       |      |         |
           | Graphics |  | SAM   | 6821 |         |
           | xlate    |--| VGD   |      |         |
           +----------+  +-------+------+         |
                |                    |          ------
           +----------+          +------+         |
           | RPi      |          | RPi  |         |
           | frame    |          | GPIO |      RPi HW
           | buff     |          |      |         |
           +----------+          +------+         |
                 |                   |          ------
           +----------+      +---------------+    |
           | VGA      |      | Devices:      |    |
           | Monitor  |      |  PS2 Keyboard |  External HW
           +----------+      |  Joystick     |    |
                             |  Audio DAC    |    |
                             +---------------+    |
```

## Implementation

This repository contains all the intermediate implementation steps, and tags them for easy retrieval. Each step builds on the functionality of its predecessors and maintains backward compatibility. The latest release is listed first:

- (next up) Extended BASIC as used in the Tandy Coco 2 modified for the SBC with all I/O via serial on an emulation of [Grant's 6-chip 6809 computer](http://searle.x10host.com/6809/Simple6809.html)
- Tag 0.2 [SBUG-E 6809 Monitor](https://deramp.com/swtpc.com/MP_09/SBUG_Index.htm) program running in emulator.
- [Tag 0.1](https://github.com/eyalabraham/dragon/releases/tag/v0.1) Stand alone emulation driver ```emu09.c``` with CPU assembly op-code tests for ```mem.c``` and ```cpu.c``` modules. Probably around 80% confidence in accuracy of CPU emulation code, will add tests and bug fixes in later releases.

### Stand alone CPU vs Dragon Computer emulation

The ```mem.c``` and ```cpu.c``` modules are all that are required for a basic MC6809E CPU emulator. The ```emu09.c``` module implements such an emulator, which I used for testing. The implementation of function ```cpu_run()``` in module ```cpu.c``` provides a timing-accurate execution of MC6809E code in both single step and continuous execution. Each time the function is called another CPU command is emulated, with CPU state available for inspection when single-stepping.

The method of repeatedly calling ```cpu_run()``` allow interruption for single stepping, break-point detection, and insertion of external events such as CPU interrupts.

To implement the full Dragon computer emulation, the ```cpu_run()``` function is called from an endless loop and IO device call-backs are implemented to carry out IO device activities.

### MC6809E CPU module

The Dragon computers where based on Motorola's [MC6806E](https://en.wikipedia.org/wiki/Motorola_6809) CPUs. The 6809 is an 8-bit microprocessor with some neat 16-bit features. The CPU module emulates the full set of CPU opcodes.

- The CPU module ```cpu.c``` interfaces with the memory module for reading opcode or data and writing to memory locations or to memory-mapped IO devices through the ```mem.c``` module
- Includes an API for external controls:
  - External interrupt emulation
  - CPU halt command
  - CPU Reset
  - CPU state and registers

### Memory module

The Dragon computer supported a maximum of 64K Bytes of memory. The memory map was managed by the SAM chip and divided into RAM, ROM, expansion ROM (cartridge), and memory mapped IO address spaces. In its basic state the memory module emulates 64K Bytes of RAM that can be accessed with the ```mem_read()``` and ```mem_write()``` API calls. Memory address ranges can be configured with special attributes that change their behavior into ROM or memory mapped IO addresses:

- ```mem_define_rom()``` will define a memory address range as read-only after which a ```mem_write()``` call would trigger a debug exception.
- ```mem_define_io()``` will define a memory address range as a memory mapped IO device and will register an IO device handler that will be called when a read or write calls are directed to addresses in the defined range.
- ```mem_load()``` will load a memory range with data copied from an input buffer.
- ```mem_init()``` will initialize memory.
  
#### Memory module data structures

```
#define     MEM_FLAG_ROM                0x80000000  // Memory location is read-only
#define     MEM_FLAG_IO                 0x40000000  // Memory location is a registered memory-mapped IO device
#define     MEM_MASK_CALLBACK           0x00ff0000  // Index to list of 256 IO device handler callbacks
#define     MEM_MASK_DATA               0x000000ff  // Mask data bits
#define     MEM_MASK_FLAGS              (MEM_FLAG_ROM | MEM_FLAG_IO)

/* b07..b00  8 least significant bits are memory location data
 * b23..b16  8 bits holding index to a list of 256 callbacks for memory-mapped IO device handlers  
 * b31..b24  8 most significant bit are flags
 */
uint32_t    memory[0xffff];
````

#### Memory-mapped IO device handlers 

When the CPU emulation module reads a memory location is uses the ```mem_read()``` call that returns the contents of the memory address passed with the call. For a memory write using ```mem_write()``` call the following logic is applied:

1. Check if address is in range 0x0000 to 0xffff. If not flag exception and return with no action
2. Check memory location against MEM_FLAG_ROM flag. If memory location is ROM flag exception and return with no action
3. Write data to memory location.
4. Check memory location against MEM_FLAG_IO flag. If flag is set, extract index of callback with MEM_MASK_CALLBACK and call the callback.
5. Return.

### IO emulation

The MC6809E CPU in the Dragon computer uses memory mapped IO devices. During initialization the emulation registers device call-back functions that implement the IO devices' functionality. The call-backs are registered against memory address ranges associated with the device using the ```mem_define_io()``` call. When writes are issued to memory locations registered to IO devices, the call backs are called after the write operation.

#### SN74LS783/MC6883 Synchronous Address Multiplexer (SAM)

tbd

#### MC6847 Video Display Generator (VDG)

The VDG is Motorola's [MC6847](https://en.wikipedia.org/wiki/Motorola_6847) video chip. Since the VDG's video memory is part of the 64K Bytes of the CPU's memory map, then writes to that region are reflected into the RPi's video frame buffer by the IO handler of the VDG. The handler will adapt the writes to the RPi frame buffer based on the VDG/SAM modes for text or graphics.

#### 6821 parallel IO (PIA)

The Dragon computer's IO was provided by two MC6821 Peripheral Interface Adapters (PIAs).

#### Keyboard

tbd

### System

- Clock interrupts for program execution pacing
- Clock interrupt for at line rate
- 

### Emulation extras

- Serial console for monitoring execution state
- Settable logging to serial console
- Built in exception generation, through which, modules can log exceptions. For example: writing to a memory location that is defines as ROM.

## TODO
- Evaluate exit code from mem_* functions
- Complete CPU assembly op-code funcitonality test
- Implement Monitor application in emulator
- Implement BASIC SBC applciaiton in emulator
- BASIC and/or MONITOR in bare-metal RPi
- 
