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
    /* Auto generated from logic.lst
     */
                                        //      ;
                                        //      ; logic.asm
                                        //      ;
                                        //      ; MC6809E emulator test code for logic commands.
                                        //      ; Test for command correctness and flag settings.
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
                                        //      ;   COM
                                        //      ;
    0x96, 0x04,                         // 000c             lda     var1
    0x43,                               // 000e             coma
    0x1f, 0x89,                         // 000f             tfr     a,b
    0x53,                               // 0011             comb
    0xd7, 0x09,                         // 0012             stb     temp
    0x03, 0x09,                         // 0014             com     temp
    0xd6, 0x09,                         // 0016             ldb     temp
                                        //      ;
                                        //      ;   ROL
                                        //      ;
    0x86, 0x08,                         // 0018             lda     #8
    0xd6, 0x04,                         // 001a             ldb     var1
    0x59,                               // 001c loop_rol:   rolb
    0x4a,                               // 001d             deca
    0x26, 0xfc,                         // 001e             bne     loop_rol
                                        //      ;
                                        //      ;   ROR
                                        //      ;
    0x1c, 0x00,                         // 0020             andcc   #0
    0xc6, 0x08,                         // 0022             ldb     #8
    0x96, 0x05,                         // 0024             lda     var2
    0x46,                               // 0026 loop_ror:   rora
    0x5a,                               // 0027             decb
    0x10, 0x26, 0xff, 0xfa,             // 0028             lbne    loop_ror
                                        //      ;
                                        //      ;   LSR
                                        //      ;
    0x86, 0x08,                         // 002c             lda     #8
    0xd6, 0x04,                         // 002e             ldb     var1
    0xd7, 0x09,                         // 0030             stb     temp
    0x04, 0x09,                         // 0032 loop_lsr:   lsr     temp
    0xd6, 0x09,                         // 0034             ldb     temp
    0x4a,                               // 0036             deca
    0x26, 0xf9,                         // 0037             bne     loop_lsr
                                        //      ;
                                        //      ;   ASL / LSL
                                        //      ;
    0x86, 0x08,                         // 0039             lda     #8
    0xd6, 0x04,                         // 003b             ldb     var1
    0x58,                               // 003d loop_asl:   aslb
    0x4a,                               // 003e             deca
    0x26, 0xfc,                         // 003f             bne     loop_asl
                                        //      ;
                                        //      ;   ASR
                                        //      ;
    0xc6, 0x08,                         // 0041             ldb     #8
    0x96, 0x05,                         // 0043             lda     var2
    0x47,                               // 0045 loop_asr:   asra
    0x5a,                               // 0046             decb
    0x10, 0x26, 0xff, 0xfa,             // 0047             lbne    loop_asr
                                        //      ;
                                        //      ;   AND
                                        //      ;
    0x96, 0x04,                         // 004b             lda     var1
    0x94, 0x06,                         // 004d             anda    var3
                                        //      ;
    0xd6, 0x07,                         // 004f             ldb     var4
    0xd4, 0x05,                         // 0051             andb    var2
                                        //      ;
                                        //      ;   OR
                                        //      ;
    0x96, 0x05,                         // 0053             lda     var2
    0x9a, 0x04,                         // 0055             ora     var1
                                        //      ;
    0xd6, 0x07,                         // 0057             ldb     var4
    0xda, 0x06,                         // 0059             orb     var3
                                        //      ;
                                        //      ;   EOR
                                        //      ;
    0x96, 0x04,                         // 005b             lda     var1
    0x98, 0x05,                         // 005d             eora    var2
                                        //      ;
    0xd6, 0x03,                         // 005f             ldb     var0
    0xd8, 0x06,                         // 0061             eorb    var3
                                        //      ;
    0x12,                               // 0063             nop
                                        //      ;
                                        //      ; End of test
   -1,                                  // --- end of code ---
};
