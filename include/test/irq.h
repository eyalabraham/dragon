/********************************************************************
 * .h
 *
 *  Auto-generated by lst2h.awk
 *
 *******************************************************************/

#define     LOAD_ADDRESS    0x0000      // Change as required
#define     RUN_ADDRESS     0x0000      // Change as required

int code[] =
{
    /* Auto generated from irq.lst
     */
                                        //      ;
                                        //      ; irq.asm
                                        //      ;
                                        //      ; MC6809E emulator test code for hardware interrupts.
                                        //      ;
                                        //      ; This assembly code is used to test emulation interrupt handling
                                        //      ; for IRQ, FIRQ and NMI interrupts of the MC6809.
                                        //      ; The code sets up a 'background' process of printing three decimal numbers
                                        //      ; that are manipulated by the three interrupt routines. Three interrupt
                                        //      ; service routines are hooked to respond to three hardware interrupts
                                        //      ; sources: NMI, IRQ, FIRQ each service routine increments a decimal number
                                        //      ; The IRQ and FIRQ need to be acknowledged by a write to an IO device
                                        //      ; which will remove its interrupt request signal.
                                        //      ; Code will load at address $0000, with entry point at $0000
                                        //      ;
    0x7e, 0x00, 0x5a,                   // 0000             jmp     start
                                        //      ;
                                        // fffc nmi_vec:    equ     $fffc
                                        // fff6 firq_vec:   equ     $fff6
                                        // fff8 irq_vec:    equ     $fff8
                                        //      
                                        // 0040 firq_dis:   equ     %01000000
                                        // 00bf firq_ena:   equ     %10111111
                                        // 0010 irq_dis:    equ     %00010000
                                        // 00ef irq_ena:    equ     %11101111
                                        //      ;
                                        // 8000 stack:      equ     $8000
                                        //      ;
                                        // f000 acia_stat:  equ     $f000           ; ACIA IO address
                                        // f001 acia_data:  equ     $f001
                                        // 0001 rx_rdy:     equ     %00000001
                                        // 0002 tx_rdy:     equ     %00000010
                                        //      ;
                                        // f002 firq_ack:   equ     $f002           ; A write to this IO device
                                        // f003 irq_ack:    equ     $f003           ; resets/acknowledges the interrupt
                                        //      ;
                                        // 000d cr:         equ     13
                                        // 000a lf:         equ     10
                                        //      ;
                                        // 0003 varstart    equ     *
                                        //      ;
    0x00,                               // 0003 var0:       fcb     $00             ; Incremented bu NMI services.
    0x00,                               // 0004 var1:       fcb     $00             ; Incremented by FIRQ service.
    0x00,                               // 0005 var2:       fcb     $00             ; Incremented by IRQ service.
    0x00,                               // 0006 col:        fcb     0
    0x00,                               // 0007 row:        fcb     0
                                        //      ;
    0x1b,                               // 0008 banner:     fcb     $1b             ; VT100 codes for printing
    0x5b, 0x32, 0x4a,                   // 0009             fcc     '[2J'
    0x49, 0x6e, 0x74, 0x20,             // 000c             fcc     'Int '
    0x74, 0x65, 0x73, 0x74,             // 0010             fcc     'test'
    0x00,                               // 0014             fcb     0
                                        //      ;
    0x1b,                               // 0015 pos1:       fcb     $1b
    0x5b, 0x33, 0x3b, 0x32, 0x48,       // 0016             fcc     '[3;2H'
    0x4e,                               // 001b             fcc     'N'
    0x00,                               // 001c             fcb     0
    0x1b,                               // 001d pos2:       fcb     $1b
    0x5b, 0x34, 0x3b, 0x32, 0x48,       // 001e             fcc     '[4;2H'
    0x46,                               // 0023             fcc     'F'
    0x00,                               // 0024             fcb     0
    0x1b,                               // 0025 pos3:       fcb     $1b
    0x5b, 0x35, 0x3b, 0x32, 0x48,       // 0026             fcc     '[5;2H'
    0x49,                               // 002b             fcc     'I'
    0x00,                               // 002c             fcb     0
                                        //      ;
                                        // 002d varend:     equ     *
                                        // 002a varlen:     equ     varend-varstart
                                        //      ;
    0x96, 0x03,                         // 002d nmi_serv:   lda     var0            ; Get counter
    0x8b, 0x01,                         // 002f             adda    #1              ; increment it
    0x19,                               // 0031             daa
    0x81, 0x99,                         // 0032             cmpa    #$99            ; check its limits
    0x26, 0x01,                         // 0034             bne     done_nmi
    0x4f,                               // 0036             clra
    0x97, 0x03,                         // 0037 done_nmi:   sta     var0            ; and save for next round.
    0x3b,                               // 0039             rti
                                        //      ;
    0xb7, 0xf0, 0x02,                   // 003a firq_serv:  sta     firq_ack        ; Acknowledge to FIRQ to the device.
    0x96, 0x04,                         // 003d             lda     var1            ; Get counter
    0x8b, 0x01,                         // 003f             adda    #1              ; increment
    0x19,                               // 0041             daa
    0x81, 0x99,                         // 0042             cmpa    #$99            ; check its limits
    0x26, 0x01,                         // 0044             bne     done_firq
    0x4f,                               // 0046             clra
    0x97, 0x04,                         // 0047 done_firq:  sta     var1            ; and save for next round.
    0x3b,                               // 0049             rti
                                        //      ;
    0xb7, 0xf0, 0x03,                   // 004a irq_serv:   sta     irq_ack         ; Acknowledge the IRQ to the device
    0x96, 0x05,                         // 004d             lda     var2            ; Get counter
    0x8b, 0x01,                         // 004f             adda    #1              ; increment it
    0x19,                               // 0051             daa
    0x81, 0x99,                         // 0052             cmpa    #$99            ; check its limits
    0x26, 0x01,                         // 0054             bne     done_irq
    0x4f,                               // 0056             clra
    0x97, 0x05,                         // 0057 done_irq:   sta     var2            ; and save for next round.
    0x3b,                               // 0059             rti
                                        //      ;
                                        //      ; Main routine with an endless loop.
                                        //      ;
    0x10, 0xce, 0x80, 0x00,             // 005a start:      lds     #stack          ; Set up the stack.
    0x8e, 0x00, 0x2d,                   // 005e             ldx     #nmi_serv       ; Set up the interrupt vectors.
    0xbf, 0xff, 0xfc,                   // 0061             stx     nmi_vec
    0x8e, 0x00, 0x3a,                   // 0064             ldx     #firq_serv
    0xbf, 0xff, 0xf6,                   // 0067             stx     firq_vec
    0x8e, 0x00, 0x4a,                   // 006a             ldx     #irq_serv
    0xbf, 0xff, 0xf8,                   // 006d             stx     irq_vec
                                        //      ;
    0x1c, 0xbf,                         // 0070             andcc   #firq_ena       ; Enable interrupts.
    0x1c, 0xef,                         // 0072             andcc   #irq_ena
                                        //      ;
    0x8e, 0x00, 0x08,                   // 0074             ldx     #banner         ; Clear screen and print
    0x8d, 0x37,                         // 0077             bsr     out_str         ; a banner
                                        //      ;
    0x8e, 0x00, 0x15,                   // 0079 print_loop: ldx     #pos1           ; Print variables
    0x8d, 0x32,                         // 007c             bsr     out_str
    0x96, 0x03,                         // 007e             lda     var0
    0x8d, 0x3b,                         // 0080             bsr     out_bcd
                                        //      ;
    0x8e, 0x00, 0x1d,                   // 0082             ldx     #pos2
    0x8d, 0x29,                         // 0085             bsr     out_str
    0x96, 0x04,                         // 0087             lda     var1
    0x8d, 0x32,                         // 0089             bsr     out_bcd
                                        //      ;
    0x8e, 0x00, 0x25,                   // 008b             ldx     #pos3
    0x8d, 0x20,                         // 008e             bsr     out_str
    0x96, 0x05,                         // 0090             lda     var2
    0x8d, 0x29,                         // 0092             bsr     out_bcd
                                        //      ;
    0x20, 0xe3,                         // 0094             bra     print_loop      ; forever.
                                        //      ;
                                        //      ; Input a character from ACIA.
                                        //      ; Wait/block for ACIA to be ready and
                                        //      ; return character in Acc-A
                                        //      ;
    0xb6, 0xf0, 0x00,                   // 0096 in_char:    lda     acia_stat       ; Read ACIA status
    0x85, 0x01,                         // 0099             bita    #rx_rdy         ; and check Rx ready bit
    0x27, 0xf9,                         // 009b             beq     in_char         ; wait for it to be ready.
    0xb6, 0xf0, 0x01,                   // 009d             lda     acia_data       ; Read a character into Acc-A
    0x39,                               // 00a0             rts                     ; and return.
                                        //      ;
                                        //      ; Print a character to ACIA.
                                        //      ; Eait/block for ACIA to be ready and
                                        //      ; send character from Acc-A
                                        //      ;
    0x34, 0x02,                         // 00a1 out_char:   pshs    a               ; Save Acc-A
    0xb6, 0xf0, 0x00,                   // 00a3 wait:       lda     acia_stat       ; Read ACIA status
    0x85, 0x02,                         // 00a6             bita    #tx_rdy         ; and check Tx ready bit
    0x27, 0xf9,                         // 00a8             beq     wait            ; wait for it to signal ready.
    0x35, 0x02,                         // 00aa             puls    a               ; Restore Acc-A
    0xb7, 0xf0, 0x01,                   // 00ac             sta     acia_data       ; send it
    0x39,                               // 00af             rts                     ; and return.
                                        //      ;
                                        //      ; Print a zero-terminated string to the console.
                                        //      ; String address in X. Outputs until
                                        //      ; encounters a '0' in the string.
                                        //      ; Preserve all registers.
                                        //      ;
    0x34, 0x12,                         // 00b0 out_str:    pshs    a,x
    0xa6, 0x80,                         // 00b2 loop_char:  lda     ,x+
    0x27, 0x04,                         // 00b4             beq     eof_str
    0x8d, 0xe9,                         // 00b6             bsr     out_char
    0x20, 0xf8,                         // 00b8             bra     loop_char
    0x35, 0x12,                         // 00ba eof_str:    puls    a,x
    0x39,                               // 00bc             rts
                                        //      ;
                                        //      ; Print a two-digit decimal number.
                                        //      ; Converts decimal number in Acc-A to BCD
                                        //      ; and prints to console. 
                                        //      ;
    0x34, 0x06,                         // 00bd out_bcd:    pshs    a,b
    0x1f, 0x89,                         // 00bf             tfr     a,b
    0x44,                               // 00c1             lsra
    0x44,                               // 00c2             lsra
    0x44,                               // 00c3             lsra
    0x44,                               // 00c4             lsra
    0x84, 0x0f,                         // 00c5             anda    #$0f
    0x8b, 0x30,                         // 00c7             adda    #'0
    0x8d, 0xd6,                         // 00c9             bsr     out_char
    0x1f, 0x98,                         // 00cb             tfr     b,a
    0x84, 0x0f,                         // 00cd             anda    #$0f
    0x8b, 0x30,                         // 00cf             adda    #'0
    0x8d, 0xce,                         // 00d1             bsr     out_char
    0x35, 0x06,                         // 00d3             puls    a,b
    0x39,                               // 00d5             rts
                                        //      ;
                                        //      ; End of test
   -1,                                  // --- end of code ---
};
