; Sysop keyboard, read in the immediate VBI.
; SIO masks the OS keyboard IRQ, and FujiNet is polled all the time, so the OS loses keys
; (or delivers a held key twice). The immediate VBI still runs during SIO: it watches
; POKEY's "key down" bit and queues the key code once per press. The OS keyboard IRQ is
; switched off while the driver is installed.

        .include "atari.inc"

        .export _kbd_init, _kbd_exit, _kbd_get

QSIZE   = 8

        .bss
held:   .res 1
head:   .res 1
tail:   .res 1
queue:  .res QSIZE

        .code

vbi:    lda SKSTAT
        and #$04                ; 0 = a key is down
        bne @up
        lda held
        bne chain               ; same press, already queued
        inc held
        ldx head
        lda KBCODE
        sta queue,x
        inx
        txa
        and #QSIZE-1
        cmp tail
        beq chain               ; queue full: drop
        sta head
        jmp chain
@up:    lda #0
        sta held
chain:  jmp $FFFF               ; old VVBLKI, patched by kbd_init

_kbd_init:
        lda VVBLKI
        sta chain+1
        lda VVBLKI+1
        sta chain+2
        lda #0
        sta head
        sta tail
        lda #1                  ; a key held right now does not count
        sta held
        sei
        lda POKMSK
        and #$BF                ; OS keyboard IRQ off (SIO keeps POKMSK)
        sta POKMSK
        sta IRQEN
        cli
        lda #6                  ; immediate VBI
        ldy #<vbi
        ldx #>vbi
        jmp SETVBV

_kbd_exit:
        lda #6
        ldy chain+1
        ldx chain+2
        jsr SETVBV
        sei
        lda POKMSK
        ora #$40
        sta POKMSK
        sta IRQEN
        cli
        lda #$FF
        sta CH
        rts

; int kbd_get(void): POKEY key code, or -1
_kbd_get:
        ldx tail
        cpx head
        beq @none
        lda queue,x
        pha
        inx
        txa
        and #QSIZE-1
        sta tail
        pla
        ldx #0
        rts
@none:  lda #$FF
        tax
        rts
