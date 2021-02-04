;
; irq.asm
;
; MC6809E emulator test code for hardware interrupts.
;
; This assembly code is used to test emulation interrupt handling
; for IRQ, FIRQ and NMI interrupts of the MC6809.
; The code sets up a 'background' process of printing three decimal numbers
; that are manipulated by the three interrupt routines. Three interrupt
; service routines are hooked to respond to three hardware interrupts
; sources: NMI, IRQ, FIRQ each service routine increments a decimal number
; The IRQ and FIRQ need to be acknowledged by a write to an IO device
; which will remove its interrupt request signal.
; Code will load at address $0000, with entry point at $0000
;
            jmp     start
;
nmi_vec:    equ     $fffc
firq_vec:   equ     $fff6
irq_vec:    equ     $fff8

firq_dis:   equ     %01000000
firq_ena:   equ     %10111111
irq_dis:    equ     %00010000
irq_ena:    equ     %11101111
;
stack:      equ     $8000
;
acia_stat:  equ     $f000           ; ACIA IO address
acia_data:  equ     $f001
rx_rdy:     equ     %00000001
tx_rdy:     equ     %00000010
;
firq_ack:   equ     $f002           ; A write to this IO device
irq_ack:    equ     $f003           ; resets/acknowledges the interrupt
;
cr:         equ     13
lf:         equ     10
;
varstart    equ     *
;
var0:       fcb     $00             ; Incremented bu NMI services.
var1:       fcb     $00             ; Incremented by FIRQ service.
var2:       fcb     $00             ; Incremented by IRQ service.
col:        fcb     0
row:        fcb     0
;
banner:     fcb     $1b             ; VT100 codes for printing
            fcc     '[2J'
            fcc     'Int '
            fcc     'test'
            fcb     0
;
pos1:       fcb     $1b
            fcc     '[3;2H'
            fcc     'N'
            fcb     0
pos2:       fcb     $1b
            fcc     '[4;2H'
            fcc     'F'
            fcb     0
pos3:       fcb     $1b
            fcc     '[5;2H'
            fcc     'I'
            fcb     0
;
varend:     equ     *
varlen:     equ     varend-varstart
;
nmi_serv:   lda     var0            ; Get counter
            adda    #1              ; increment it
            daa
            cmpa    #$99            ; check its limits
            bne     done_nmi
            clra
done_nmi:   sta     var0            ; and save for next round.
            rti
;
firq_serv:  sta     firq_ack        ; Acknowledge to FIRQ to the device.
            lda     var1            ; Get counter
            adda    #1              ; increment
            daa
            cmpa    #$99            ; check its limits
            bne     done_firq
            clra
done_firq:  sta     var1            ; and save for next round.
            rti
;
irq_serv:   sta     irq_ack         ; Acknowledge the IRQ to the device
            lda     var2            ; Get counter
            adda    #1              ; increment it
            daa
            cmpa    #$99            ; check its limits
            bne     done_irq
            clra
done_irq:   sta     var2            ; and save for next round.
            rti
;
; Main routine with an endless loop.
;
start:      lds     #stack          ; Set up the stack.
            ldx     #nmi_serv       ; Set up the interrupt vectors.
            stx     nmi_vec
            ldx     #firq_serv
            stx     firq_vec
            ldx     #irq_serv
            stx     irq_vec
;
            andcc   #firq_ena       ; Enable interrupts.
            andcc   #irq_ena
;
            ldx     #banner         ; Clear screen and print
            bsr     out_str         ; a banner
;
print_loop: ldx     #pos1           ; Print variables
            bsr     out_str
            lda     var0
            bsr     out_bcd
;
            ldx     #pos2
            bsr     out_str
            lda     var1
            bsr     out_bcd
;
            ldx     #pos3
            bsr     out_str
            lda     var2
            bsr     out_bcd
;
            bra     print_loop      ; forever.
;
; Input a character from ACIA.
; Wait/block for ACIA to be ready and
; return character in Acc-A
;
in_char:    lda     acia_stat       ; Read ACIA status
            bita    #rx_rdy         ; and check Rx ready bit
            beq     in_char         ; wait for it to be ready.
            lda     acia_data       ; Read a character into Acc-A
            rts                     ; and return.
;
; Print a character to ACIA.
; Eait/block for ACIA to be ready and
; send character from Acc-A
;
out_char:   pshs    a               ; Save Acc-A
wait:       lda     acia_stat       ; Read ACIA status
            bita    #tx_rdy         ; and check Tx ready bit
            beq     wait            ; wait for it to signal ready.
            puls    a               ; Restore Acc-A
            sta     acia_data       ; send it
            rts                     ; and return.
;
; Print a zero-terminated string to the console.
; String address in X. Outputs until
; encounters a '0' in the string.
; Preserve all registers.
;
out_str:    pshs    a,x
loop_char:  lda     ,x+
            beq     eof_str
            bsr     out_char
            bra     loop_char
eof_str:    puls    a,x
            rts
;
; Print a two-digit decimal number.
; Converts decimal number in Acc-A to BCD
; and prints to console. 
;
out_bcd:    pshs    a,b
            tfr     a,b
            lsra
            lsra
            lsra
            lsra
            anda    #$0f
            adda    #'0
            bsr     out_char
            tfr     b,a
            anda    #$0f
            adda    #'0
            bsr     out_char
            puls    a,b
            rts
;
; End of test
