# SBUG-E 6809 ROM Monitor

[Source code](https://deramp.com/swtpc.com/MP_09/SBUG_Index.htm) is part ot the SWTPC site curated by Michael Holley. The source is included in this directory, unmodified except for three padding bytes added for convenient file conversion at address $fefd.  
The source code can be assembled as-is with the [as9 assembler](https://github.com/eyalabraham/as9) with ```as9 sbug.asm -l s19``` to produce a listing file and the .s19 image.  
  
This directory includes produces the loadable code for the emulator to run the SBUG monitor as-is in the MC6809C emulator as another compatibility test.

Module ```mon09.c``` contains the IO environment emulation for the SWTPC computer to execute the SBUG monitor program. All monitor commands are functional except for the **Disk Bootstrap - "D"**, **Punch MIKBUG Tape - "P [addr] - [addr]"** and **Load MIKBUG Format Tape - "L"**.

## Memory map for SBUG

The ```mon09.c ``` module implements the following memory mapped IO devices that are part of the SWTPC computer:

### Memory mapping register

The IC11 RAM re-addressing registers is located at address $FFF0. This device listed as a 75S189 TTL chip that acts as a memory paging device. The emulated computer only traps and displayes read and write access to this address.

### ACIA serial communication device registers

The emulated computer traps and services read and write access to this device. The IO requests for serial communication data bytes is redirected by the call-back function to and from the Raspberry Pi's default serial port. A serial console should be connected to the Raspberry Pi.
  
**MC6850 Control Register (Write) $E004**

 0-1  Baudrate (0=CLK/64, 1=CLK/16, 2=CLK/1, 3=RESET)  
 2-4  Mode (0..7 = 7e2,7o2,7e1,7o1,8n2,8n1,8e1,8o1) (data/stop bits, parity)  
 5-6  Transmit Interrupt/RTS/Break control (0..3)  
       0 = Output /RTS=low,  and disable Tx Interrupt  
       1 = Output /RTS=low,  and enable Tx Interrupt  
       2 = Output /RTS=high, and disable Tx Interrupt  
       3 = Output /RTS=low,  and disable Tx Interrupt, and send a Break  
 7    Receive Interrupt (1=Enable on buffer full/buffer overrun)  

**MC6850 Status Register (Read) $E004**

 0    Receive Data  (0=No data, 1=Data can be read)  
 1    Transmit Data (0=Busy, 1=Ready/Empty, Data can be written)  
 2    /DCD level  
 3    /CTS level  
 4    Receive Framing Error (1=Error)  
 5    Receive Overrun Error (1=Error)  
 6    Receive Parity Error  (1=Error)  
 7    Interrupt Flag (see Control Bits 5-7) (IRQ pin is not connected)  

MC6850 Data Register (R/W) Data can be read when Status.Bit0=1, and written when Status.Bit1=1.  

**8-bit Data (Read/Write) $E005**

Rx has a 1-stage fifo, plus 1 shift register (2 stages in total) Tx has a 1-stage fifo, plus 1 shift register (2 stages in total)

### Western Digital 1791 Floppy controller

The emulated computer ignores access to this device and therefore does not support the disk functionality of SBUG monitor.

- Comreg EQU $E018  COMMAND REGISTER
- Drvreg EQU $E014  DRIVE REGISTER
- Secreg EQU $E01A  SECTOR REGISTER
- Datreg EQU $E01B  DATA REGISTER

- ADDREG EQU $F000  ADDRESS REGISTER
- CNTREG EQU $F002  COUNT REGISTER
- CCREG  EQU $F010  CHANNEL CONTROL REGISTER
- PRIREG EQU $F014  DMA PRIORITY REGISTER
- AAAREG EQU $F015  ???
- BBBREG EQU $F016  ???
- COMREG EQU $F020  1791 COMMAND REGISTER
- SECREG EQU $F022  SECTOR REGISTER
- DRVREG EQU $F024  DRIVE SELECT LATCH
- CCCREG EQU $F040  ???
