; Startup code for cc65 and Shiru's NES library
; based on code by Groepaz/Hitmen <groepaz@gmx.net>, Ullrich von Bassewitz <uz@cc65.org>



FT_BASE_ADR		= $0100		;page in RAM, should be $xx00
FT_DPCM_OFF		= $f000		;$c000..$ffc0, 64-byte steps
FT_SFX_STREAMS	= 1			;number of sound effects played at once, 1..4

FT_THREAD       = 1		;undefine if you call sound effects in the same thread as sound update
FT_PAL_SUPPORT	= 1		;undefine to exclude PAL support
FT_NTSC_SUPPORT	= 1		;undefine to exclude NTSC support
FT_DPCM_ENABLE  = 0		;undefine to exclude all DMC code
FT_SFX_ENABLE   = 0		;undefine to exclude all sound effects code







    .export _exit,__STARTUP__:absolute=1
	.exportzp _FRAME_CNT, _SECONDS_CNT, _PAD_STATE, _PAD_STATET, _SPR_PRIORITY
	
	.import push0,popa,popax,_main,zerobss,copydata

; Linker generated symbols
	.import __STACK_START__   ,__STACKSIZE__ ;changed
	.import __ROM0_START__  ,__ROM0_SIZE__
	.import __STARTUP_LOAD__,__STARTUP_RUN__,__STARTUP_SIZE__
	.import	__CODE_LOAD__   ,__CODE_RUN__   ,__CODE_SIZE__
	.import	__RODATA_LOAD__ ,__RODATA_RUN__ ,__RODATA_SIZE__
	.import NES_MAPPER, NES_PRG_BANKS, NES_CHR_BANKS, NES_MIRRORING

    .include "zeropage.inc"


PPU_CTRL	=$2000
PPU_MASK	=$2001
PPU_STATUS	=$2002
PPU_OAM_ADDR=$2003
PPU_OAM_DATA=$2004
PPU_SCROLL	=$2005
PPU_ADDR	=$2006
PPU_DATA	=$2007
PPU_OAM_DMA	=$4014
PPU_FRAMECNT=$4017
DMC_FREQ	=$4010
CTRL_PORT1	=$4016
CTRL_PORT2	=$4017

OAM_BUF				=$0200
PAL_BUF				=$01c0
PPU_COMMANDS_BUF 	=$0180

.segment "ZEROPAGE"

NTSC_MODE: 			.res 1
FRAME_CNT1: 		.res 1
_FRAME_CNT = FRAME_CNT1
FRAME_CNT2: 		.res 1
VRAM_UPDATE: 		.res 1
NAME_UPD_ADR: 		.res 2
NAME_UPD_ENABLE: 	.res 1
PAL_UPDATE: 		.res 1
PAL_BG_PTR: 		.res 2
PAL_SPR_PTR: 		.res 2
SCROLL_X: 			.res 1
SCROLL_Y: 			.res 1
SCROLL_X1: 			.res 1
SCROLL_Y1: 			.res 1
PAD_STATE: 			.res 2		;one byte per controller
_PAD_STATE = PAD_STATE
PAD_STATEP: 		.res 2
PAD_STATET: 		.res 2
_PAD_STATET = PAD_STATET
PPU_CTRL_VAR: 		.res 1
PPU_CTRL_VAR1: 		.res 1
PPU_MASK_VAR: 		.res 1
RAND_SEED: 			.res 2
FT_TEMP: 			.res 3
TEMP: 				.res 11
SPRID:				.res 1
SPRSTEP:			.res 1
SPR_PRIORITY:		.res 1
_SPR_PRIORITY = SPR_PRIORITY
PAD_BUF		=TEMP+1
PTR			=TEMP	;word
LEN			=TEMP+2	;word
NEXTSPR		=TEMP+4
SCRX		=TEMP+5
SCRY		=TEMP+6
SRC			=TEMP+7	;word
DST			=TEMP+9	;word
RLE_LOW		=TEMP
RLE_HIGH	=TEMP+1
RLE_TAG		=TEMP+2
RLE_BYTE	=TEMP+3
;nesdoug code requires
VRAM_INDEX:			.res 1


TEMP_VAR: .res 1
COUNTER:	  .res 1
BRICK_CHAR_X: .res 1
BRICK_CHAR_Y: .res 1
CHR_BYTE: .res 1
MASK_CHR_BYTE: .res 1
LOW_PTR_BYTE: .res 1
HIGH_PTR_BYTE: .res 1
SPR_X: .res 1
SPR_Y: .res 1
BUFFER_POS: .res 1
BG_PAL_NUMBER: .res 1
SECONDS_CNT: .res 1
_SECONDS_CNT = SECONDS_CNT




.segment "HEADER"

    .byte $4e,$45,$53,$1a
	.byte <NES_PRG_BANKS
	.byte <NES_CHR_BANKS
	.byte <NES_MIRRORING|(<NES_MAPPER<<4)
	.byte <NES_MAPPER&$f0
	.res 8,0



.segment "STARTUP"

start:
_exit:

    sei
	lda	#$10
	sta	PPU_CTRL	; background of	second chr generator
	cld
	ldx	#2

	@wait:			
	lda	PPU_STATUS
	bpl	@wait
	lda	#$06
	sta	PPU_MASK
	sta PPU_MASK_VAR	
	dex
	bne	@wait
	ldx	#$7F 
	txs
	inx
	stx DMC_FREQ
	ldx #$40
	stx CTRL_PORT2
    inx
	lda #0
	sta BUFFER_POS


	jsr	copydata
    lda #<(__STACK_START__+__STACKSIZE__) 
    sta	sp
    lda	#>(__STACK_START__+__STACKSIZE__)
    sta	sp+1            ; Set argument stack ptr


	lda PPU_STATUS
	and #$80
	sta <NTSC_MODE

	lda #0
	ldx #0
	jsr _set_vram_update
	lda #4
	sta SPRSTEP
	lda #%10000000
	sta <PPU_CTRL_VAR	
	sta PPU_CTRL		;enable NMI

	jmp _main
	
	.include "lib/neslib.s"
	.include "game.s"

	
nmi:
	pha
	txa
	pha
	tya
	pha
	php

	lda <PPU_MASK_VAR	;if rendering is disabled, do not access the VRAM at all
	and #%00011000
	bne @doUpdate
	jmp	@skipAll

@doUpdate:
	lda #0
	sta PPU_OAM_ADDR
	lda #>OAM_BUF		
	sta PPU_OAM_DMA

	lda <VRAM_UPDATE
	beq @skipUpd
	lda #0
	sta <VRAM_UPDATE
	
@skipUpd:

	lda PPU_STATUS      ; Reset VBlank Occurance
	jsr _update_screen
	lda BG_PAL_NUMBER
	bmi @skipPalLoad
	jsr _load_bg_pal

@skipPalLoad:
	lda <PPU_CTRL_VAR
	ora #%10110000     ; Typical BC PPU configuration (Sprites always 8x16 (width x height))
	sta PPU_CTRL

	lda <SCROLL_X
	sta PPU_SCROLL
	lda <SCROLL_Y
	sta PPU_SCROLL
	
@skipAll:	
	lda <PPU_MASK_VAR
	sta PPU_MASK
	jsr _reset_oam_buffer
	inc <FRAME_CNT1
	inc <FRAME_CNT2
	lda <FRAME_CNT2
	cmp #6
	bne @skipNtsc
	lda #0
	sta <FRAME_CNT2

@skipNtsc:
	lda <FRAME_CNT1
	and #63
	bne @skipSecondsUpdate
	inc SECONDS_CNT
@skipSecondsUpdate:
	plp
	pla
	tay
	pla
	tax
	pla

irq:

    rti
	
; Execute commands from ppu_commands_buf
_update_screen:
	ldx BUFFER_POS
	lda #0
	sta PPU_COMMANDS_BUF,x
	tax

@loop:
	cpx BUFFER_POS
	beq @done				;if position is zero (no commands were added during frame), do nothing.
	lda PPU_COMMANDS_BUF,x
	inx
	sta PPU_ADDR
	lda PPU_COMMANDS_BUF,x
	inx
	sta PPU_ADDR

@writeLoop:
	lda PPU_COMMANDS_BUF,x
	inx
	cmp #$FF
	bne @write				; normal data byte
	lda PPU_COMMANDS_BUF,x	; found 0xFF. Peek next byte, not incrementing X
	cmp #$FF				
	bne @loop				; if next byte not $FF - start new command
	lda PPU_COMMANDS_BUF-1,x; yes, it's second FF, load first FF byte and write it by next command

@write:
	sta PPU_DATA
	jmp @writeLoop

@done:
	lda #0
	sta BUFFER_POS
	rts	

;reset OAM buffer and cycle fill direction
_reset_oam_buffer:
	ldx     SPRID
	lda     SPRSTEP; 4 or -4 for sprite cycling
	eor     #$FF
	clc
	adc     #1              ; gap := -gap


	sta     SPRSTEP; 4 or -4 for sprite cycling
@writeLoop:
	txa
	clc
	adc     SPRSTEP ; start filling from end
	tax
	lda     #$F0 ; 'ё'
	sta     OAM_BUF,x
	cpx     #4
	bne     @writeLoop
	stx     SPRID
	rts

.segment "VECTORS"

    .word nmi	;$fffa vblank nmi
    .word start	;$fffc reset
   	.word start	;$fffe irq / brk


.segment "CHARS"

	.incbin "assets/graphics.chr"
