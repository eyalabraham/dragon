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
    /* Auto generated from stack.lst
     */
                                        //      ;
                                        //      ; stack.asm
                                        //      ;
                                        //      ; MC6809E emulator test code for stack operations.
                                        //      ; All tests will use direct or immediate addressing for simplicity,
                                        //      ; other addressing modes will be tested separately.
                                        //      ;
    0x7e, 0x00, 0x0a,                   // 0000             jmp     start
                                        //      ;
                                        // 0001 setcf:      equ     $01
                                        // 00fe clrcf:      equ     $fe
                                        //      ;
                                        // 0003 varstart    equ     *
                                        //      ;
    0xff,                               // 0003 var0:       fcb     $ff
    0x55,                               // 0004 var1:       fcb     $55
    0xaa,                               // 0005 var2:       fcb     $aa
    0x01,                               // 0006 var3:       fcb     $01
    0x80,                               // 0007 var4:       fcb     $80
    0x00,                               // 0008 var5:       fcb     $00
                                        //      ;
                                        // 0009 varend:     equ     *
                                        // 0006 varlen:     equ     varend-varstart
                                        //      ;
    0x00,                               // 0009 temp:       fcb     0
                                        //      ;
    0x1c, 0x00,                         // 000a start:      andcc   #0              ; zero CC bits
                                        //      ;
    0x10, 0xce, 0xff, 0xff,             // 000c             lds     #$ffff
    0xce, 0xff, 0x00,                   // 0010             ldu     #$ff00
                                        //      ;
    0x20, 0x14,                         // 0013             bra     jump1
                                        //      ;
    0xbd, 0x00, 0x2c,                   // 0015 jump2:      jsr     sub1
                                        //      ;
    0x96, 0x03,                         // 0018             lda     var0
    0xd6, 0x04,                         // 001a             ldb     var1
    0x9e, 0x05,                         // 001c             ldx     var2
    0x10, 0x9e, 0x07,                   // 001e             ldy     var4
    0x17, 0x00, 0x0a,                   // 0021             lbsr    sub2
    0x21, 0xe4,                         // 0024             brn     start
    0x16, 0x00, 0x10,                   // 0026             lbra    end
                                        //      ;
    0x16, 0xff, 0xe9,                   // 0029 jump1:      lbra    jump2
                                        //      ;
    0x12,                               // 002c sub1:       nop
    0x39,                               // 002d             rts
                                        //      ;
    0x36, 0x36,                         // 002e sub2:       pshu    a,b,x,y
    0x4c,                               // 0030             inca
    0x5c,                               // 0031             incb
    0x1f, 0x01,                         // 0032             tfr     d,x
    0x1f, 0x12,                         // 0034             tfr     x,y
    0x37, 0x36,                         // 0036             pulu    a,b,x,y
    0x39,                               // 0038             rts
                                        //      ;
    0x12,                               // 0039 end:        nop
                                        //      ;
                                        //      ; End of test
   -1,                                  // --- end of code ---
};
