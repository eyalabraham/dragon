;
; swi.asm
;
; MC6809E emulator test code SWI software interrups.
; Test for command correctness and flag settings.
; All tests will use direct or immediate addressing for simplicity,
; other addressing modes will be tested separately.
;
            jmp     start
;
setcf:      equ     $01
clrcf:      equ     $fe
swi_vec:    equ     $fffa
stack:      equ     $ff00
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
swi_serv:   ldd     var0
            exg     a,b
            std     var0
            rti
;
start:      andcc   #0              ; zero CC bits
;
            lds     #stack
;
            ldx     #swi_vec        ; Set SWI interrupt vector
            ldd     #swi_serv
            std     ,x
;
            ldd     var0
            swi                     ; initiate the interrupt call
            ldd     var0
;
done:       nop
;
; End of test
