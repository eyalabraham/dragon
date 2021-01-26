;
; branch.asm
;
; MC6809E emulator test code for branching.
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
start:      andcc   #0              ; zero CC bits
;
            lda     #56
            cmpa    #56
            bne     fail
            bcs     fail
            bmi     fail
            bvs     fail
            bhi     fail
;
            cmpa    #41
            beq     fail
            ble     fail
            blt     fail
;
            cmpa    #58
            bgt     fail
            bge     fail
            bhs     fail
;
            lda     #$56
            adda    #$56
            bvc     fail
;
            lda     #$9a
            adda    #$9a
            bvc     fail
;
            lda     #$9a
            adda    #$56
            bvs     fail
;
pass:       andcc   #0              ; Clear CC.C to indicate pass
            bra     done
fail:       orcc    #1              ; Set CC.C to indicate fail
done:       nop
;
; End of test
