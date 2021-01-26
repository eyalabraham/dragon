;
; logic.asm
;
; MC6809E emulator test code for logic commands.
; Test for command correctness and flag settings.
; All tests will use direct or immediate addressing for simplicity,
; other addressing modes will be tested separately.
;
            jmp     start
;
setcf:      equ     $01
clrcf:      equ     $fe
;
varstart    equ     *
;
var0:       fcb     $ff
var1:       fcb     $55
var2:       fcb     $aa
var3:       fcb     $01
var4:       fcb     $80
var5:       fcb     $00
;
varend:     equ     *
varlen:     equ     varend-varstart
;
temp:       fcb     0
;
start:      andcc   #0              ; zero CC bits
;
;   COM
;
            lda     var1
            coma
            tfr     a,b
            comb
            stb     temp
            com     temp
            ldb     temp
;
;   ROL
;
            lda     #8
            ldb     var1
loop_rol:   rolb
            deca
            bne     loop_rol
;
;   ROR
;
            andcc   #0
            ldb     #8
            lda     var2
loop_ror:   rora
            decb
            lbne    loop_ror
;
;   LSR
;
            lda     #8
            ldb     var1
            stb     temp
loop_lsr:   lsr     temp
            ldb     temp
            deca
            bne     loop_lsr
;
;   ASL / LSL
;
            lda     #8
            ldb     var1
loop_asl:   aslb
            deca
            bne     loop_asl
;
;   ASR
;
            ldb     #8
            lda     var2
loop_asr:   asra
            decb
            lbne    loop_asr
;
;   AND
;
            lda     var1
            anda    var3
;
            ldb     var4
            andb    var2
;
;   OR
;
            lda     var2
            ora     var1
;
            ldb     var4
            orb     var3
;
;   EOR
;
            lda     var1
            eora    var2
;
            ldb     var0
            eorb    var3
;
            nop
;
; End of test
