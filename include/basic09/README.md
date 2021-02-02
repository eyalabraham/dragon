# 6809 ROM BASIC

Microsoft Extended BASIC, as used in the Tandy Coco 2 modified for the SBC with all I/O via serial. Commands not applicable for the SBC have been removed. BASIC from $DB00 TO $FFFF.

The ```bacic09.c``` module emulate [Grant's 6-chip 6809 computer](http://searle.x10host.com/6809/Simple6809.html).

## memory map

Extracted from source code

```
RAM start     $0000
  FRETOP var  $0021-$0022
  TOPRAN var  $0071-$0071
  System var  $0216
  Free start  $0217
  Stack       $9F38 (=FRETOP) ?
  RAM top     $BFFF (=TOPRAM) ?

ACIA          $A000
  Data        $A001

ROM start     $DB00
  Reset       $DB46
  End         $FFFF
```

## ACIA IO emulation

The Extended BASIC code polls the ACIA status register for character input and character transmission.

**MC6850 Control Register (Write) $A000**

 0-1  Baudrate (0=CLK/64, 1=CLK/16, 2=CLK/1, 3=RESET)
 2-4  Mode (0..7 = 7e2,7o2,7e1,7o1,8n2,8n1,8e1,8o1) (data/stop bits, parity)
 5-6  Transmit Interrupt/RTS/Break control (0..3)
       0 = Output /RTS=low,  and disable Tx Interrupt
       1 = Output /RTS=low,  and enable Tx Interrupt
       2 = Output /RTS=high, and disable Tx Interrupt
       3 = Output /RTS=low,  and disable Tx Interrupt, and send a Break
 7    Receive Interrupt (1=Enable on buffer full/buffer overrun)

**MC6850 Status Register (Read) $A000**

 0    Receive Data  (0=No data, 1=Data can be read)
 1    Transmit Data (0=Busy, 1=Ready/Empty, Data can be written)
 2    /DCD level
 3    /CTS level
 4    Receive Framing Error (1=Error)
 5    Receive Overrun Error (1=Error)
 6    Receive Parity Error  (1=Error)
 7    Interrupt Flag (see Control Bits 5-7) (IRQ pin is not connected)

**MC6850 Data Register (R/W)**

Data can be read when Status.Bit0=1, and written when Status.Bit1=1.

**8-bit Data (Read/Write) $A001**

Rx has a 1-stage fifo, plus 1 shift register (2 stages in total) Tx has a 1-stage fifo, plus 1 shift register (2 stages in total)
