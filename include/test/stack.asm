;
; stack.asm
;
; MC6809E emulator test code for stack operations.
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
            lds     #$ffff
            ldu     #$ff00
;
            bra     jump1
;
jump2:      jsr     sub1
;
            lda     var0
            ldb     var1
            ldx     var2
            ldy     var4
            lbsr    sub2
            brn     start
            lbra    end
;
jump1:      lbra    jump2
;
sub1:       nop
            rts
;
sub2:       pshu    a,b,x,y
            inca
            incb
            tfr     d,x
            tfr     x,y
            pulu    a,b,x,y
            rts
;
end:        nop
;
; End of test
