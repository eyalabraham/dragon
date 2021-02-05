;
; profile.asm
;
; MC6809E CPU emulation for runing timing profile tests.
;
; This assembly code is used to test emulation timing.
; The code toggles an IO port to generate an external signal
; that can be timed with an oscilloscope.
; The code provide a general driver, and the measurements
; can be added in the "measurement" section for timing.
; Example, measure baseline then measure timing difference
; with a test code that has 10x NOP op-codes. The NOP
; execution time can be estimated by dividing the time difference
; by 10.
;
            jmp     start
;
io:         equ     $f000
;
io_high:    equ     1
io_low:     equ     0
;
stack:      equ     $8000
;
varstart    equ     *
;
temp:       fdb     0
;
varend:     equ     *
varlen:     equ     varend-varstart
;
; Main routine with an endless loop.
;
start:      lds     #stack          ; Set up the stack.
            clra
            sta     io              ; Initialize IO
;
test_loop:  inca
            sta     io
;
; *** Test section ***
;
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
;
;
; *** End of test section ***
;
            clra
            sta     io
            lbra    test_loop       ; forever.
;
; End of test
