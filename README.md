
> This repository is deprecated. Refer to the refactored Dragon 32 emulator in new repository https://github.com/eyalabraham/dragon32-emu

# Dragon 32 computer emulator

[This project](https://sites.google.com/site/eyalabraham/dragon-32-computer) implements the software and the hardware needed to emulate a [Dragon 32 computer](https://en.wikipedia.org/wiki/Dragon_32/64). The end goal is to run the emulation on a bare-metal Raspberry Pi (RPi) platform with some external peripherals. Development would be done on a Raspberry Pi Linux distribution. The Dragon was my first computer when home/personal computers started to emerge in the mid 80s, and it is also one of the simplest to emulate. Other emulators exist, [including XROR](http://www.6809.org.uk/xroar/), but I decided to build my own as another exercise in RPi bare-metal programming.

## Resources

- [The DRAGON Archives](https://worldofdragon.org/index.php?title=Main_Page) resources and software.
- [DRAGON Data archives](http://www.dragondata.co.uk/index.html) hardware schematics.
- [Inside the Dragon](http://www.dragondata.co.uk/Publications/InsideTheDragon.pdf) by Duncan Smeed and Ian Sommerville.
- Computers based on 6809 [CoCo Coding](https://sites.google.com/a/aaronwolfe.com/cococoding/home)
- [RPi BCM2835 GPIOs](https://elinux.org/RPi_BCM2835_GPIOs)
- [C library for Broadcom BCM 2835](https://www.airspayce.com/mikem/bcm2835/)

## Design

The CPU module is ```cpu.c```, the memory module is ```mem.c```. The IO module is implemented as call-back functions hooked through the memory module to selected memory addresses, and emulate the response of a memory mapper IO device. The IO module call-backs implement the various IO device in the emulated computer. This design is flexible enough to allow the definition of any computer configuration, memory and IO, built around an MC6809 CPU. The examples and development steps show an [SWTPC computer](https://en.wikipedia.org/wiki/SWTPC) and [Grant's 6-chip 6809 computer](http://searle.x10host.com/6809/Simple6809.html) as two instances I used along the way.  
  
The emulation will not all be done through software. In order to save time some IO devices will be implemented in hardware: keyboard interface (possibly PS2 through the Pi's SPI), audio DAC and an analog comparator for a successive approximation ADC that will reproduce sound and support an **original** (circa 1984) Dragon Computer joystick.  

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
           | Graphics |  | SAM   | PIA  |         |
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
                             |  SD card      |    |
                             +---------------+    |
```

## Schematics

Schematics of external emulator hardware (KiCAD drawing)[https://github.com/eyalabraham/schematics/tree/master/dragon].

## Implementation

This repository contains all the intermediate implementation steps and tags them for easy retrieval. Each step builds on the functionality of its predecessors and maintains backward compatibility. The latest release is listed first:

- Release tag 1.0 Complete emulator for Raspberry Pi under RaspiOS "Buster"
- [Release tag 0.11](https://github.com/eyalabraham/dragon/releases/tag/v0.11) CAS file loader/manager from SD card
- [Release tag 0.10](https://github.com/eyalabraham/dragon/releases/tag/v0.10) ROM image file loader/manager from SD card. Emulator reset button, refactoring, preparation for bare-metal, and bug fixes.
- [Release tag 0.9](https://github.com/eyalabraham/dragon/releases/tag/v0.9) Joystick support and a change to RPi Zero.
- [Release tag 0.8](https://github.com/eyalabraham/dragon/releases/tag/v0.8) Dragon 32 computer emulation with sound.
- [Release tag 0.7](https://github.com/eyalabraham/dragon/releases/tag/v0.7) Dragon 32 computer emulation with video and semi-graphics modes.
- [Release tag 0.6](https://github.com/eyalabraham/dragon/releases/tag/v0.6) Minimal Dragon 32 computer emulation with text mode and keyboard.
- [Release tag 0.5](https://github.com/eyalabraham/dragon/releases/tag/v0.5) Timing profiles of CPU emulation running under Linux on Raspberry Pi .
- [Release tag 0.4](https://github.com/eyalabraham/dragon/releases/tag/v0.4) Completed emulation and testing of SWI/SW2/SWI3 software interrupts and IRQ/FIRQ/NMI hardware interrupts.
- [Release tag 0.3](https://github.com/eyalabraham/dragon/releases/tag/v0.3) Extended BASIC as used in the Tandy Coco 2 modified for the SBC with all I/O via serial on an emulation of [Grant's 6-chip 6809 computer](http://searle.x10host.com/6809/Simple6809.html).
- [Release tag 0.2](https://github.com/eyalabraham/dragon/releases/tag/v0.2) [SBUG-E 6809 Monitor](https://deramp.com/swtpc.com/MP_09/SBUG_Index.htm) program running in an emulated [SWTPC computer](https://en.wikipedia.org/wiki/SWTPC).
- [Release tag 0.1](https://github.com/eyalabraham/dragon/releases/tag/v0.1) Stand alone emulation driver ```emu09.c``` with CPU assembly op-code tests for ```mem.c``` and ```cpu.c``` modules. Probably around 80% confidence in accuracy of CPU emulation code, will add tests and bug fixes in later releases.

### Stand alone CPU vs Dragon Computer emulation

The ```mem.c``` and ```cpu.c``` modules are all that are required for a basic MC6809E CPU emulator. The ```emu09.c``` module implements such an emulator, which I used for testing. The implementation of function ```cpu_run()``` in module ```cpu.c``` provides a timing-accurate execution of MC6809E code in both single step and continuous execution. Each time the function is called another CPU command is emulated, with CPU state available for inspection when single-stepping.

The method of repeatedly calling ```cpu_run()``` allow interruption for single stepping, break-point detection, and insertion of external events such as CPU interrupts.

To implement the full Dragon computer emulation, the ```cpu_run()``` function is called from an endless loop and IO device call-backs are implemented to carry out IO device activities.

### Timing profile

One of the goals is to achieve CPU cycle timing that is as close as possible to the 0.89MHz of MC6809E that was used in the Dragon Computer. A crude timing profile using ```profile.asm``` test code and ```profile.c``` emulation module yielded the following results under Raspberrypi Linux

| Measurement       | Setup                    |Total [uSec] | Command [uSec] | Cycles | Cycle time [uSec] |
|:------------------|:-------------------------|:-----------:|:--------------:|:------:|:-----------------:|
| Baseline          | Empty loop overhead      | 2.8         |                |        |                   | 
| NOP op-code       | 10x NOP op-codes         | 10          | 0.72           | 2      | 0.36              |
| ORA <immed>       | 10x ORA <Immed>          | 9.6         | 0.68           | 2      | 0.34              |
| ABX op-code       | 10x ABX op-codes         | 9.8         | 0.7            | 3      | 0.23              |
| STA <addr_ext>    | 10x STA <add_ext>        | 14.8        | 1.2            | 5      | 0.24              |
| LBRA <long>       | 10x LBRA <long>          | 12.8        | 1.0            | 5      | 0.20              |
| PSHS/PULS a,b,x,y | 5x PSHS / 5xPULS a,b,x,y | 27.5        | 2.47           | 11     | 0.22              |

The tests were run on a Raspberry Pi model B, single core (ARM1176JZF-S) 700MHz Broadcom BCM2835 with 512MB RAM, running a generic Raspberrypi Linux distribution.  
The calculations show that the emulated CPU runs at an average rate of 3,754,978[Hz]

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
typedef enum
{
    MEM_TYPE_RAM,
    MEM_TYPE_ROM,
    MEM_TYPE_IO,
} memory_flag_t;

typedef struct
{
    uint8_t data_byte;
    memory_flag_t memory_type;
    io_handler_callback io_handler;
} memory_t;
```

When the CPU emulation module reads a memory location is uses the ```mem_read()``` call that returns the contents of the memory address passed with the call. For a memory write using ```mem_write()``` call the following logic is applied:

1. Check if address is in range 0x0000 to 0xffff. If not flag exception and return with no action
2. Check memory location against MEM_FLAG_ROM flag. If memory location is ROM return with no action
3. Write data to memory location.

For both ```mem_read()``` and ```mem_write()``` check memory location against MEM_FLAG_IO flag. If flag is set, invoke the callback with the accessed address, the data (if a write operation) and a read/write flag. This will give the IO callback the context it needs to emulate the IO behind the memory address.

### IO emulation

The MC6809E CPU in the Dragon computer uses memory mapped IO devices. During initialization the emulation registers device callback functions that implement the IO devices' functionality. The callbacks are registered against memory address ranges associated with the device using the ```mem_define_io()``` call. The callbacks are invoked when reads or writes are issued to memory locations registered to IO devices. The ```dragon.c```, ```mon09.c``` and ```basic09.c``` computer emulation modules use IO callbacks to emulate the SAM, VDG, MC6821 PIA and MC6850 ACIA etc.  
  
The code in the call-backs redirect the IO request to the appropriate Raspberry Pi GPIO pin and function. The Dragon 32 emulation uses the following external GPIO:

| Function                    | RPi model B | RPi Zero / ZeroW |
|-----------------------------|-------------|------------------|
| Analog multiplexer select-0 | GPIO-02     | GPIO-02          |
| Analog multiplexer select-1 | GPIO-03     | GPIO-03          |
| RPi timing test point       | GPIO-04     | GPIO-04          |
| Emulator reset              | na          | GPIO-05          |
| Analog comparator input     | GPIO-07     | GPIO-07          |
| Right joystick "fire"       | GPIO-08     | GPIO-08          |
| AVR ATtiny85 keyboard MISO  | GPIO-09     | GPIO-09          |
| AVR ATtiny85 keyboard MOSI  | GPIO-10     | GPIO-10          |
| AVR ATtiny85 keyboard SCLK  | GPIO-11     | GPIO-11          |
| Serial TxD (future)         | GPIO-14     | GPIO-14          |
| Serial RxD (future)         | GPIO-15     | GPIO-15          |
| SD card CE                  | na          | GPIO-16          |
| AVR ATtiny85 reset          | GPIO-17     | GPIO-17          |
| SD card MISO                | na          | GPIO-19          |
| SD card MOSI                | na          | GPIO-20          |
| SD card SCLK                | na          | GPIO-21          |
| DAC bit.0                   | GPIO-22     | GPIO-22          |
| DAC bit.1                   | GPIO-23     | GPIO-23          |
| DAC bit.2                   | GPIO-24     | GPIO-24          |
| DAC bit.3                   | GPIO-25     | GPIO-25          |
| DAC bit.4                   | GPIO-18     | GPIO-26 (1)      |
| DAC bit.5                   | GPIO-27     | GPIO-27          |

Notes:
- (1) Changed on the Zero/ZeroW in order to form a contiguous bit order in the GPIO register.

#### SN74LS783/MC6883 Synchronous Address Multiplexer (SAM)

The [SAM chip](https://cdn.hackaday.io/files/1685367210644224/datasheet-MC6883_SAM.pdf) in the Dragon computer is responsible for IO address decoding, dynamic RAM memory refresh, and video memory address generation for the Video Display Generator (VDG) chip. Of these three functions, only the last one requires implementation. Since the SAM chip does not generate video, the address scanning is implemented in the VDG module by the ```vdg_render()``` function. The SAM device emulation only transfers offset address and video mode settings to the VDG module.

#### MC6847 Video Display Generator (VDG)

The VDG is Motorola's [MC6847](https://en.wikipedia.org/wiki/Motorola_6847) video chip. Since the VDG's video memory is part of the 64K Bytes of the CPU's memory map, then writes to that region are reflected into the RPi's video frame buffer by the IO handler of the VDG. The handler will adapt the writes to the RPi frame buffer based on the VDG/SAM modes for text or graphics. The Dragon computer video display emulation is implemented in the VDG module by the ```vdg_render()``` function, by accessing the Raspberry Pi Frame Buffer.

#### ```vdg_render()``` performance improvements

Changed rendering routine to take advantage of 32-bit architecture for RPi frame-buffer writes. Only the text screen modes benefit from this change and reduce the render time by 25%. Graphics screens had no change or showed worse timing.

| Function                      | Frame buffer pixels | Baseline  | 32-bit write | 
|-------------------------------|---------------------|-----------|--------------|
| Text                          | 49,152              | 4.00 mSec | 2.75 mSec    |
| Graphics 128x96  BW (PMODE 0) | 12,288              | 0.80 mSec | worse        |
| Graphics 128x96  C  (PMODE 1) | 12,288              | 1.30 mSec | 1.40 mSec    |
| Graphics 192x128 BW (PMODE 2) | 49,152              | 2.20 mSec | worse        |
| Graphics 192x128 C  (PMODE 3) | 49,152              | 3.15 mSec | 3.60 mSec    |
| Graphics 256x192 BW (PMODE 4) | 49,152              | 3.30 mSec | worse        |

### Emulator main loop performance improvements

The main loop of the emulator is responsible for five tasks: execute CPU machine code from program memory, check state of reset button, check state of F1 function key for emulation escape, render video memory to RPi frame buffer, and generate VSYNC IRQ at 50Hz.  

Originally, the emulation produced distorted audio that I attributed to the average 3mSec pause at a 50Hz rate of the frame buffer updates done by vdg_render(). After measuring the emulation loop I discovered that the loop takes ~12uSec to complete, but the cpu_run() function completes at an average of 1uSec. This means that the emulator is running at a slow 300KHz equivalent CPU, and since audio is directly produced by CPU writes to the DAC this produced distorted audio. Most of the 12uSec interval is spent by vdg_render() and pia_vsync_irq() that were timed at about 4.8uSec each due to calls to clock() to retrieve system clock tick count.  

The change removed all calls to clock() and the 50Hz execution trigger of vdg_render() and pia_vsync_irq() is brute-forced by a simple counting cycles. Since each cpu_run() completes at an average time of 1uSec, adding the extra functions now extends the emulation loop to about 1.5uSec. The MC6809E CPU executes machine code utilizing an average of 3 to 4 clock cycles, so the emulation is now equivalent to about 3x faster than a real CPU, and a slow down empty loop is added to waste time and extend the loop up to about 4uSec total.  

Video is refreshed approximately every 20mSec (50Hz). The average 3mSec pause in CPU execution every 20mSec adds a 50Hz undertone that is largely filtered by the high-pass at the audio DAC output.  

The new emulator loop includes CPU execution (line 95), dead-time padding (line 99), test reset button state (lines 101 to 120), emulation escape (lines 122 to 124), vdg_render() and pia_vsync_irq() timing to match 50Hz (lines 126 to 131).  

```
 91    for (;;)
 92    {
 93        rpi_testpoint_on();
 94
 95        cpu_run();
 96
 97        rpi_testpoint_off();
 98
 99        for ( i = 0; i < CPU_TIME_WASTE; i++);
100
101        switch ( get_reset_state(LONG_RESET_DELAY) )
102        {
103            case 0:
104                cpu_reset(0);
105                break;
106
107            case 2:
108                /* Cold start flag set to value that is not 0x55
109                 */
110                mem_write(0x71, 0);
111                printf("Force cold restart.\n");
112                /* no break */
113
114            case 1:
115                cpu_reset(1);
116                break;
117
118            default:
119                printf("main(): unknown reset state.\n");
120        }
121
122        emulator_escape_code = pia_function_key();
123        if ( emulator_escape_code == ESCAPE_LOADER )
124            loader();
125
126        vdg_render_cycles++;
127        if ( vdg_render_cycles == VDG_RENDER_CYCLES )
128        {
129            vdg_render();
130            pia_vsync_irq();
131            vdg_render_cycles = 0;
132        }
133    }
```

#### 6821 parallel IO (PIA)

The Dragon computer's IO was provided by two MC6821 Peripheral Interface Adapters (PIAs).

##### Keyboard

The keyboard interface uses an ATtiny85 AVR coded with a PS2 to SPI interface. It implements a PS2 keyboard interface and an SPI serial interface. The AVR connects with the Raspberry Pi's SPI. The code configures the keyboard, accepts scan codes, converts the AT scan codes to ASCII make/break codes for the [Dragon 32 emulation](https://github.com/eyalabraham/dragon) running on the Raspberry Pi.
The AVR buffers the key codes in a small FIFO buffer, and the emulation periodically reads the buffer through the SPI interface.

```
 +-----+               +-----+            +-------+
 |     |               |     |            |       |
 |     +----[ MOSI>----+     |            |       |
 |     |               |     |            |       |
 |     +----< MISO]----+     +--< Data >--+ Level |
 | RPi |               | AVR |            | shift +---> PS2 keyboard
 |     +----[ SCL >----+     +--< CLK ]---+       |
 |     |               |     |            |       |
 |     +----[ RST >----+     |            |       |
 |     |               |     |            |       |
 +-----+               +-----+            +-------+
```

##### Audio

The Dragon's sound system is built around a simple 6-bit Digital to Analog (DAC). The DAC is a common [resistor ladder](https://en.wikipedia.org/wiki/Resistor_ladder) that is driven by a 6-bit MC4050 buffer. The Dragon emulator uses a similar setup with an 8-bit TTL buffer (74244) and similar resistor values. The GPIO pins of a Raspberry Pi B do not provide a contiguous set of output bits, so bit position translation is implemented.

```
 DAC    RPi B        (RPi Zero)
                                           +-------+
 bit.0  GPIO22 P1.15 (GPIO22 P1.15 )  >----+       +-[330K]-+
 bit.1  GPIO23 P1.16 (GPIO23 P1.16 )  >----+       +-[150K]-+
 bit.2  GPIO24 P1.18 (GPIO24 P1.18 )  >----+ 74244 +-[ 82K]-+
 bit.3  GPIO25 P1.22 (GPIO25 P1.22 )  >----+       +-[ 39K]-+
 bit.4  GPIO18 P1.12 (GPIO26 P1.37 )  >----+       +-[ 20K]-+
 bit.5  GPIO27 P1.13 (GPIO27 P1.13 )  >----+       +-[ 10K]-+---[Analog out>
                                           +-------+
```

In the Dragon computer the audio multiplexer is controlled by PIA0-CA2 and CB2, with PA1-CB2 controlling the audio source inhibit line. The CD4052 user in this emulator is different from the 4529 device used in the original computer and some changes in the emulation call-back are implemented to account for the difference. The changes reduce the number of supported joysticks to one with only the right joystick, and only two audio sources: DAC, and one open source for future use.

##### Joystick

The external hardware provides connectivity for the right joystick. The emulation software supports only one joystick. The external hardware is built with an analog multiplexer (CD4052) that routes the joystick output voltages to a comparator. The comparator works in conjunction with the DAC and the Dragon software to convert the analog joystick position to a number range between 0 and 63. The analog multiplexer is controlled by GPIO pins that represent PIA0-CA2 and PIA1-CB2 control lines, using low order select bit and the inhibit line instead of the high order select bit.

##### Field Sync IRQ

In the Dragon computer, the system generates an IRQ interrupt at the frame synchronization (FS) rate of 50 or 60Hz. The FS signal is routed through PIA0-CB1 (control register B-side) and generates an IRQ signal. Resetting the interrupt request by reading data register PIA0 B-side.

### Software loader

The software loader/manager interfaces with an SD card that holds Dragon 32 ROM cartridge images and CAS files. The loader/manager can be escaped into from the emulation using the F1 key. Within the loader one can brows ROM and CAS files to load and run on the Dragon 32 emulator.

This functionality is available only on RPi Zero/W and uses an SD card interface connected to the auxiliary SPI interface (SPI1).

ROM code files are loaded as-is into the Dragon's ROM cartridge memory address space. No auto start is provided, but the BASIC EXEC vector is modified to point to 0xC000, so a simple EXEC from the BASIC prompt will start the ROM code.

CAS files are digital images of old-style tape content and not memory images. More on [CAS file formats here](https://retrocomputing.stackexchange.com/questions/150/what-format-is-used-for-coco-cassette-tapes/153#153), and [Dragon 32 CAS format here](https://archive.worldofdragon.org/index.php?title=Tape%5CDisk_Preservation#CAS_File_Format). A cassette file can be mounted by the loader (like loading a cassette into a tape player), and then use the BASIC CLOAD or CLOADM commands to do the reading.

### TODOs

#### System

- Cartridge interrupt for cartridge code auto start

#### Emulation extras

- Serial console for monitoring execution state
- Settable logging to serial console
  - Exception generation, example: writing to a memory location that is defines as ROM.
- Dragon sound sources: single-bit

### Known problems

- Joystick drift: check reference voltage. Reduce to TTL output level from 74LS244
- Improve keyboard scan call-back.

## Files

- **dragon.c** main module for Dragon Computer emulation.
- Emulation
  - **cpu.c** 6809E emulation.
  - **mem.c** memory emulation module.
  - **sam.c** SAM emulation call-back functions.
  - **vdg.c** VDG emulation.
  - **pia.c** PIA emulation call-back functions.
  - **rpi.c** Raspberry Pi hardware specific functions.
- Emulated computers
  - **basic09.c**  emulation of [Grant's 6-chip 6809 computer](http://searle.x10host.com/6809/Simple6809.html).
  - **emu09.c** general module for loading and executing 6809E machine language test code.
  - **intr09.c** general module for loading and executing 6809E interrupt tests.
  - **mon09.c** emulation of [SBUG-E 6809 Monitor](https://deramp.com/swtpc.com/MP_09/SBUG_Index.htm) program.
  - **profile.c** general module for loading and executing 6809E timing profile tests.
- Utilities and drivers
  - **trace.c** CPU trace utility functions.
  - **uart.c** RPi UART utility module.
  - **spi.c** SPI test program.
  - **i2c.c** I2C test program.
  - **loader.c** ROM and CAS file loader/manager.
  - **sdfat32.c** SD card reader for FAT32 file system.
- RPi bare-metal code modules
  - **printf.c** printf() replacement for bare-metal.
- Miscellaneous
  - **README.md** this file.
  - **LICENSE.md** license.
  - **Makefile** make file.
  - **include/** include files.
  - **scripts/** helper scripts.

