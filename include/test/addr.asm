;
; addr.asm
;
; MC6809E emulator test code for addressing modes.
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
the_start:  fdb     varstart
temp:       fdb     var0
;
start:      andcc   #0              ; zero CC bits
;
;   LEA
;
; Acc A, B, and D offsets
            ldu     #varstart
            lda     #6
            ldb     #-3
            leas    a,u
            leas    b,u
            ldd     #32
            leax    d,u
            ldd     #-32
            leas    d,u
; 5, 8, and 16-bit offset
            ldx     #varstart
            leay    0,x
            leau    7,x
            leau    -3,x
            leas    32,x
            leas    -32,x
            leas    256,x
            leas    -256,x
; 16-bit extended indirect
            ldx     [temp]
; LEAX indirect, auto increment by 2, and PC relative
            leax    [#the_start,pcr]
            ldb     #varlen
            asrb
loop_lea3:  ldy     ,x++
            decb
            bne     loop_lea3
;
; LEAX, auto increment, and PC relative
            leax    #varstart,pcr
            ldb     #varlen
loop_lea2:  lda     ,x+
            decb
            bne     loop_lea2
;
; LEAY, auto decrement. and PC relative
            leay    #done,pcr
            ldb     #6
loop_lea1:  lda     ,-y
            decb
            bne     loop_lea1
;
            bra     done
;
var10:      fcb     1
var11:      fcb     2
var12:      fcb     3
var13:      fcb     4
var14:      fcb     5
var15:      fcb     6
;
done:       nop
;
; End of test
