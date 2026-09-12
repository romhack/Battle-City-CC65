;Hardware-dependent functions to be implemented in assembly specific for game

.export _draw_brick_char, _set_bg_pal_number, _reset_vram_cmd_queue, _add_vram_fill_cmd, _add_vram_cpy_cmd, _add_vram_cpy_based_cmd 
.export _add_sprite, _add_sprite_16x16

;void __fastcall__ draw_brick_char(unsigned char ascii_char, unsigned char x, unsigned char y);

_draw_brick_char:
    ; Get parameters from stack
	
	sta BRICK_CHAR_Y
	ldy #0
    lda (sp),y
    sta BRICK_CHAR_X
	iny
    lda (sp),y
    sta CHR_BYTE
	ldx BRICK_CHAR_X
	ldy BRICK_CHAR_Y
    
    ; Clean up stack (2 parameters)
    lda sp
    clc
    adc #2
    sta sp
    bcc @no_carry
    inc sp+1
@no_carry:
    
    ; Restore ch to A
    lda CHR_BYTE
    
    ; Now: A = ch, X = x, Y = y
    ; Original code starts here
    stx BRICK_CHAR_X
    tax                    ; X = ch
    tya
    clc
    adc #$20
    sta BRICK_CHAR_Y
    
    lda #$00
    sta LOW_PTR_BYTE
    lda #$10
    sta HIGH_PTR_BYTE
    
@add_10:
    dex
    bmi @done_multiply
    lda #$10
    jsr Inc_Ptr_on_A
    jmp @add_10
    
@done_multiply:
    lda HIGH_PTR_BYTE
    sta PPU_ADDR
    lda LOW_PTR_BYTE
    sta PPU_ADDR
    lda PPU_DATA          ; Dummy read
    
    lda #8
    sta COUNTER
    
@read_chr_byte:
    lda PPU_DATA
    pha
    dec COUNTER
    bne @read_chr_byte
    
    lda #8
    sta COUNTER
    
@next_byte:
    pla
    sta CHR_BYTE
    lda #$80
    sta MASK_CHR_BYTE
    
@next_bit:
    ldx BRICK_CHAR_X
    ldy BRICK_CHAR_Y
    jsr Get_SprCoord_InTiles
    
    lda CHR_BYTE
    and MASK_CHR_BYTE
    beq @empty_pixel
    
    jsr NT_Buffer_Process_OR
    jmp @pixel_processed
    
@empty_pixel:
    jsr NT_Buffer_Process_XOR
    
@pixel_processed:
    lda BRICK_CHAR_X
    clc
    adc #4
    sta BRICK_CHAR_X
    
    lsr MASK_CHR_BYTE
    bcc @next_bit
    
    lda BRICK_CHAR_X
    sec
    sbc #$20
    sta BRICK_CHAR_X
    
    lda BRICK_CHAR_Y
    sec
    sbc #4
    sta BRICK_CHAR_Y
    
    dec COUNTER
    bne @next_byte
    
    rts

NT_Buffer_Process_OR:
    lda (LOW_PTR_BYTE),y
    and #%11110000
    bne @done
    lda TEMP_VAR
    ora (LOW_PTR_BYTE),y
    sta (LOW_PTR_BYTE),y
@done:
    rts

Get_SprCoord_InTiles:
    stx SPR_X
    sty SPR_Y
    jsr GetCoord_InTiles
	lda     #1              ; Transforms TEMP_VAR depending on Spr_Coord
	sta     TEMP_VAR
	lda     SPR_Y
	and     #4
	beq     @skip
	asl     TEMP_VAR
	asl     TEMP_VAR

@skip:
	lda     SPR_X
	and     #4
	beq     @done
	asl     TEMP_VAR

@done:
	rts

GetCoord_InTiles:
    jsr XnY_div_8
	jsr CoordTo_PPUaddress
	sta HIGH_PTR_BYTE
	sty LOW_PTR_BYTE
	ldy #0
rts

XnY_div_8:
    tya
    lsr a
    lsr a
    lsr a
    tay
    txa
    lsr a
    lsr a
    lsr a
    tax
    rts
	
CoordTo_PPUaddress:
	lda	#0
	sta	TEMP_VAR		
	tya
	lsr	a
	ror	TEMP_VAR
	lsr	a
	ror	TEMP_VAR
	lsr	a		; y div	8
	ror	TEMP_VAR
	pha
	txa
	ora	TEMP_VAR		; (x + (y shl 5))
	tay
	pla
	ora	#4		; set second bit
	rts	

NT_Buffer_Process_XOR:
    lda (LOW_PTR_BYTE),y
    and #%11110000
    bne @done
    lda TEMP_VAR
    eor #$FF
    and (LOW_PTR_BYTE),y
    sta (LOW_PTR_BYTE),y
@done:
    rts

Inc_Ptr_on_A:
    clc
    adc LOW_PTR_BYTE
    sta LOW_PTR_BYTE
    bcc @done
    inc HIGH_PTR_BYTE
@done:
    rts
	
; Load background palette once. Called from crt0 NMI. Flag resets after load in order not to load each frame, but once.
_load_bg_pal:
	asl a
	asl a
	asl a
	asl a
	tax
	ldy #$10
	lda #$3F
	sta PPU_ADDR
	lda #0
	sta PPU_ADDR

@loop:
	lda palette_data,x
	sta PPU_DATA
	inx
	dey
	bne @loop
	
	lda #$FF
	sta BG_PAL_NUMBER
	
	lda #$3F
	sta PPU_ADDR
	lda #0
	sta PPU_ADDR
	sta PPU_ADDR
	sta PPU_ADDR
	rts
; Palette data - 9 palettes x 16 bytes each
palette_data:
	.byte $0F,$17,$06,$00, $0F,$3C,$10,$12, $0F,$29,$09,$0B, $0F,$00,$10,$20
	.byte $0F,$17,$06,$00, $0F,$3C,$12,$12, $0F,$29,$09,$0B, $0F,$00,$10,$20
	.byte $0F,$17,$06,$00, $0F,$12,$3C,$12, $0F,$29,$09,$0B, $0F,$00,$10,$20
	.byte $0F,$16,$16,$30, $0F,$3C,$10,$16, $0F,$29,$09,$27, $0F,$00,$10,$20
	.byte $0F,$17,$06,$00, $0F,$3C,$10,$00, $0F,$29,$09,$00, $0F,$00,$10,$00
	.byte $0F,$0F,$06,$00, $0F,$3C,$10,$00, $0F,$29,$09,$00, $0F,$00,$10,$00
	.byte $0F,$12,$06,$00, $0F,$3C,$10,$00, $0F,$29,$09,$00, $0F,$00,$10,$00
	.byte $0F,$00,$06,$00, $0F,$3C,$10,$00, $0F,$29,$09,$00, $0F,$00,$10,$00
	.byte $0F,$30,$06,$00, $0F,$3C,$10,$00, $0F,$29,$09,$00, $0F,$00,$10,$00	
	
	
;void __fastcall__ set_bg_pal_number(unsigned char pal_id);
; Set up command to load new palette set into bg palettes. Will be updated once during next NMI
_set_bg_pal_number:	
	sta BG_PAL_NUMBER
	rts

;void reset_vram_cmd_queue(void);
;
;Reset commands buffer position, used to not dump anything in commands buffer on next NMI, effectively clearing existing commands
;For example, when write to commands buffer is a side effect along with writing to screen buffer,
;like writing HQ tiles before stage select screen  

_reset_vram_cmd_queue:
    lda #0
    sta BUFFER_POS
    rts


; void __fastcall__ add_vram_fill_cmd(unsigned int ppu_addr, unsigned char value, unsigned char length);
;
; Add fill with single byte command to screen commands buffer, which is executed every nmi
; Calling convention:
;   length (last arg) -> A register
;   value -> stack top
;   ppu_addr -> stack 

_add_vram_fill_cmd:
    sta TEMP+2              ; Save length (from A)
    
    jsr popa                ; Get value from stack
    sta TEMP+3              ; Save value
    
    jsr popax               ; Get ppu_addr from stack (16-bit)
    ; X = high byte, A = low byte
    
    ldy BUFFER_POS
    
    ; Write PPU address (high, low)
    sta PPU_COMMANDS_BUF+1,y
	txa
    sta PPU_COMMANDS_BUF,y
    iny
	iny
    
    ; Fill with value
    ldx TEMP+2              ; Get length
    lda TEMP+3              ; Get fill value
@loop:
    sta PPU_COMMANDS_BUF,y
    iny
    dex
    bne @loop
    
    ; Write end marker
    lda #$FF
    sta PPU_COMMANDS_BUF,y
    iny
    
    ; Update buffer position
    sty BUFFER_POS
    
    rts	
	
; void __fastcall__ add_vram_cpy_cmd(unsigned int ppu_addr, const unsigned char *src, unsigned char length);
;
; Copy raw bytes to PPU commands buffer (char_base = 0)
; Calling convention:
;   length (last arg) -> A register
;   src    -> stack top (16-bit pointer)
;   ppu_addr -> stack (16-bit)
_add_vram_cpy_cmd:
    sta TEMP+5              ; Save length (from A)

    lda #0
    sta TEMP+2              ; char_base = 0

    jmp _add_vram_cpy_cmd_body


; void __fastcall__ add_vram_cpy_based_cmd(unsigned int ppu_addr, const unsigned char *src, unsigned char length, unsigned char char_base);
;
; Copy raw bytes to PPU commands buffer, adding char_base offset to each byte
; Calling convention:
;   char_base (last arg) -> A register
;   length   -> stack top (8-bit, pushed as 16-bit by cc65)
;   src      -> stack (16-bit pointer)
;   ppu_addr -> stack (16-bit)
_add_vram_cpy_based_cmd:
    sta TEMP+2              ; Save char_base (from A)

    jsr popa               ; Pop length from stack (cc65 pushes bytes as 16-bit)
    sta TEMP+5              ; Save length (low byte only)


_add_vram_cpy_cmd_body:
    jsr popax               ; Pop src pointer from stack
    sta TEMP+3              ; Save src pointer low
    stx TEMP+4              ; Save src pointer high

    jsr popax               ; Pop ppu_addr from stack
    ; X = high byte, A = low byte

    ldy BUFFER_POS

    ; Write PPU address (high byte first, then low)
    sta PPU_COMMANDS_BUF+1,y
    txa
    sta PPU_COMMANDS_BUF,y
    iny
    iny

    tya
    tax                     ; Buffer write position now in X

    ldy #0
@loop:
    cpy TEMP+5              ; Check if all bytes have been copied
    beq @done

    lda (TEMP+3),y          ; Read byte from source
    clc
    adc TEMP+2              ; Apply char_base offset
    sta PPU_COMMANDS_BUF,x  ; Write byte to commands buffer
    inx
    iny
    jmp @loop

@done:
    ; Write end marker
    lda #$FF
    sta PPU_COMMANDS_BUF,x
    inx
    stx BUFFER_POS

    rts	


;Writes to OAM buffer and advances to next slot
;CHR_BYTE - sprite index
;SPR_X, SPR_Y - coordinates
;TEMP_VAR - attribute
write_oam_entry:   
    ldx SPRID
    lda CHR_BYTE
	sta OAM_BUF+1,x
    lda SPR_Y
    sec
    sbc #8              ;both halves sprite will have Y - 8, as coordinates are of center of metasprite
	sta OAM_BUF+0,x     
    lda SPR_X
	sta OAM_BUF+3,x
    jsr _set_ptr_to_scrn_buffer
    ldy #0
    lda (PTR),Y
    cmp #$22            ;Check for tank sprite intersecting forest: $22 in Pattern Table - forest tile
    bne @skip_attrib
    lda TEMP_VAR
    ora SPR_PRIORITY            ;Switch to OAM_BEHIND/FRONT
    sta TEMP_VAR
@skip_attrib:
    lda TEMP_VAR
    sta OAM_BUF+2,x

	txa
	clc
	adc SPRSTEP
	sta SPRID
	rts       

; Sets PTR to the SCREEN_BUFFER at SPR_X and SPR_Y, used at add_sprite functions to check forest tile
_set_ptr_to_scrn_buffer:

    lda #0
    sta TEMP           ; Screen $20 tiles wide. High address byte in NT increases by 1, 
                        ;if from screen start will be $100 tiles or 8 tile rows (Y=8). 
                        ;Thus, high byte can be calculated by formula: (Y div 8) or (Y shr 3). 
                        ;Then in high byte bit #2 is set (high byte now not less than 4, as screen buffer is at 0x400): 
                        ;further, to high byte will be added $1c, so in result shouldn't get address less than $2000 (1st NT). 
                        ;Low byte in this case, can be calculated by formula: (X + Y*($20)) or (X + (Y shl 5)). 

    lda SPR_Y
    lsr a
    lsr a
    lsr a               ; Y div 8 - get rid of bits 0,1,2
    lsr a               ; Y bits 3,4,5 will become hi bits of low ptr byte, because X will be >> 3
    ror TEMP
    lsr a
    ror TEMP
    lsr a               
    ror TEMP
    ora #4              ; Set second bit as offset 0x400, and save Y bits 6,7 in hi ptr byte
	sta <PTR+1
    lda SPR_X
    clc
    adc #3
    lsr a
    lsr a
    lsr a
    ora TEMP            ; ((X + 3) >> 3 + ((Y >> 3) << 5))
    sta <PTR
    rts

;void __fastcall__ add_sprite(unsigned char x,unsigned char y,unsigned char chrnum,unsigned char pal);
_add_sprite:  
; Left half
    sta TEMP_VAR        ;A has attribute    
	ldy #0		        ;3 popa calls replacement
	lda (sp),y          ;chr number
	iny
    sta CHR_BYTE
	lda (sp),y          ;Y
    iny
    sta SPR_Y   
	lda (sp),y          ;X
    sta SPR_X
    jsr write_oam_entry

	lda <sp
	clc
	adc #3 ;4
	sta <sp
	bcc @1
	inc <sp+1
@1:
	rts  

;Writes to OAM buffer and advances to next slot (second half of a 16x16
;sprite). Reuses the Y-derived pointer bits (TEMP, PTR+1) left behind by
;the write_oam_entry call for the first half — valid because SPR_Y is
;identical between the two halves.
write_oam_entry_x_only:
    ldx SPRID
    lda CHR_BYTE
    sta OAM_BUF+1,x
    lda SPR_Y
    sec
    sbc #8
    sta OAM_BUF+0,x
    lda SPR_X
    sta OAM_BUF+3,x

    ; cheap X-only pointer recompute; PTR+1 and TEMP already valid
    lda SPR_X
    clc
    adc #3
    lsr a
    lsr a
    lsr a
    ora TEMP
    sta <PTR

    ldy #0
    lda (PTR),Y
    cmp #$22
    bne @skip_attrib
    lda TEMP_VAR
    ora SPR_PRIORITY
    sta TEMP_VAR
@skip_attrib:
    lda TEMP_VAR
    sta OAM_BUF+2,x

    txa
    clc
    adc SPRSTEP
    sta SPRID
    rts    

;void __fastcall__ add_sprite_16x16(unsigned char x,unsigned char y,unsigned char chrnum,unsigned char pal);
_add_sprite_16x16:
    sta TEMP_VAR
    pha
    ldy #0
    lda (sp),y
    iny
    sta CHR_BYTE
    lda (sp),y
    iny
    sta SPR_Y
    lda (sp),y
    pha
    sec
    sbc #8
    sta SPR_X
    jsr write_oam_entry          ; full pointer calc (X+Y) - unchanged

; Right half
    pla
    sta SPR_X
    pla
    sta TEMP_VAR
    lda CHR_BYTE
    clc
    adc #2
    sta CHR_BYTE
    jsr write_oam_entry_x_only   ; reuse cached Y bits, only redo X

    lda <sp
    clc
    adc #3
    sta <sp
    bcc @1
    inc <sp+1
@1:
    rts
	