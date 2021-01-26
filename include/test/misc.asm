;
; misc.asm
;
; MC6809E emulator test code for compare and value move.
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
;   TST
;
            lda     var1
            ldb     var2
            orcc    #$0f            ; flags NZVC
            tsta                    ; flags nzvC
            tstb                    ; flags NzvC
            tst     temp            ; flags nZvC
;
;   EXG
;
            lda     var0
            ldb     var1
            exg     a,b
            ldx     var2
            exg     d,x
;
;   TFR
;
            tfr     x,y
            tfr     y,d
            tfr     d,u
;
;   SEX
;
            ldb     var1
            sex
            ldb     var2
            sex
;
;   BIT
;
            lda     #$55
            bita    var3
            bita    var4
            ldb     #$aa
            bitb    var4
            bitb    var3
;
            nop
;
; End of test
