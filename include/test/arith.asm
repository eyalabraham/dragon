;
; arith.asm
;
; MC6809E emulator test code for arithmetic commands.
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
start:      andcc   #0              ; clear CC bits
;
; ADD
;
            ldb     #100
            stb     temp
            lda     #30
            adda    temp            ; a = 130 ($82, NzVc)
;
; SUB
;
            ldb     #30
            stb     temp
            lda     #50
            suba    temp            ; a = 20 ($14, nzvc)
            suba    temp            ; a = -10 ($f6, NzVC)
;
; SBC
;
            nop
;
; NEG
;
            lda     var1
            nega
            ldb     var2
            negb
            stb     temp
            neg     temp
            ldb     temp
;
; INC
;
            lda     #0
            ldb     #8
            stb     temp
loop_inc:   inca
            dec     temp
            lbne    loop_inc
;
; MUL
;
            lda     #8
            ldb     #34
            mul                     ; d = $0110
            lda     #255
            ldb     #128
            mul                     ; d = $7f80
;
; ABX
;
            ldx     #varstart
            ldb     #1
            abx                     ; X = X + 1, no flag change
            ldx     #varstart
            ldb     var4
            abx                     ; X = X + 128 (unsigned addition), no flag change
;
; ADC
;
            lda     var0
            orcc    #setcf
            adca    var5            ; a = $00, Z=1, C=1, V=1?
;
            ldb     var1
            andcc   #clrcf
            adcb    #10             ; b = 5f
;
            lda     var2
            orcc    #setcf
            adca    var4            ; a = 2b, C=1, V=1
;
; ADD
;
            lda     var0
            adda    var3            ; a = $00, Z=1, C=1, V=1?
;
            ldb     var2
            addb    var4            ; b = 2a, C=1, V=1
;
            ldd     var4            ; big-endian so d = $8000
            addd    #$800           ; d = $8800
;
; CLR
;
            lda     var0
            clra
; 
            ldb     var1
            clrb
;
            ldb     var1            ; b = $55
            clr     var1
            lda     var1            ; a = 0
            stb     var1
            lda     var1            ; a = $55
;
; CMP
;
            lda     var1            ; a = $55
            cmpa    var3            ; compare to $01
            cmpa    var2            ; compare to $aa
;
; DAA
;
            lda     #$25
            adda    #$48
            daa                     ; a = 73
;
            lda     #$39
            adda    #$48
            daa                     ; a = 87
;
            lda     #$72
            adda    #$73
            daa                     ; a = (1)45
;
            nop
;
; End of test
