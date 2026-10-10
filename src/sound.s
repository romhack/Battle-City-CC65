; ============================================================================
; Battle City sound engine — ported from the original NSF-style bytecode
; player, restructured from "28 virtual slots" (224 bytes of Sound_DataBlocks
; + 16 bytes ZP) down to "4 hardware channels" (48 bytes RAM + 5 bytes ZP).
;
; Design notes / deliberate deviations from the original disassembly:
;
; 1) Channel instead of slot indexing.
;    Every sound's *first stream byte* is a fixed, ROM-baked channel number
;    (1=square1, 2=square2, 3=triangle, 4=noise) — it never changes at
;    runtime. The original discovered this dynamically every frame across
;    28 slots and arbitrated with a scratch SoundChannels[4] array. Since
;    the mapping is static, we resolve it once at sound_play() time and
;    just store *which sound id* currently owns each of the 4 channels.
;    This removes the whole 28-slot scan + claim/marker (+5 offset0 trick)
;    machinery entirely.
;
; 2) Register push only when something actually changed (chan_dirty).
;    Verified against the original: hardware registers are written when a
;    channel is first claimed AND whenever a genuine new pitch (bytecode
;    token 0x00-0x5F) is decoded; a bare "replay same pitch" token (0x60)
;    does NOT rewrite registers (that's the original's self-consistent
;    behaviour: rewriting $x003 mid-note would reset the length counter /
;    duty phase and click). We replicate this with an explicit per-channel
;    "dirty" flag instead of the original's reuse of the claim marker.
;
; 3) Loop counters stay GLOBAL (3 bytes), matching the original exactly.
;    Sound_LoopCounter0/1/2 are shared across whatever sound currently uses
;    opcodes $F0/$F1/$F2 — in the original data this is only ever Battle1/
;    2/3 (one counter each) simultaneously, and RecordPts2 reuses counter1
;    at a time when Battle music cannot be playing. We keep the exact same
;    3 shared bytes rather than inventing a per-channel scheme, so behaviour
;    (including this shared-resource property) matches the original 1:1.
;
; 4) Sound_Command_SetVolumeAlt ($EB) in the source disassembly ANDs with
;    "#Sound_CurrentData_Ptr" — clearly an IDA mis-resolution of an
;    immediate operand, not a real 6502 instruction. It is never used by
;    any of the 28 real sound streams, so it is reconstructed here as an
;    alias of SetVolume ($EA), which is the closest well-defined command.
;
; 5) All 18 original opcodes are implemented (per request), including the
;    6 duplicate "AdvancePointer" slots, even though only StopReset,
;    SetDutyVolume, ClearCounters/LoopCount* and JumpToOffset are actually
;    used by the shipped sound data.
;
; 6) Pause behaviour matches the original: only the channel used by
;    SND_PAUSE keeps advancing; the other 3 channels freeze completely
;    (registers left exactly as they were, not silenced).
;
; 7) Background/ambient sounds (chan_underlay), a deliberate addition.
;    The original's 28-slot model lets a lower-priority sound that loses
;    its channel keep running silently in the background and become
;    audible again the instant the higher-priority occupant stops -- for
;    free, as a side effect of every slot having independent state. Our
;    4-channel model can't do that (only one id's state exists per
;    channel), which is fine for one-shot SFX but breaks SND_ENGINE: on
;    real hardware it shares square2 with roughly a dozen other sounds
;    (Pause, BonusTaken, every ricochet, HQ explode, Move, ...), so it
;    would get evicted constantly and never come back. chan_underlay is
;    a 4-byte "what should auto-resume here" register, set only by
;    sound_play_background(). Whenever a channel goes idle -- via a
;    StopReset opcode (process_channel/do_allocate_channel's @stopped)
;    or an explicit sound_stop() -- maybe_resume_underlay re-launches
;    the registered id if there is one. A background sound's own stream
;    must never reach StopReset (loop forever, like Engine's existing
;    $F9 opcode does) or this would recurse.
; ============================================================================

.segment "ZEROPAGE"

snd_ptr:        .res 2      ; scratch ROM stream pointer
snd_temp:       .res 1      ; scratch (frequency LUT shift, opcode dispatch)
snd_temp2:      .res 1      ; scratch
snd_id:         .res 1      ; scratch: id being processed by sound_play
snd_muted:      .res 1

; Total ZP usage: 5 bytes 

.segment "BSS"

IDLE = $FF

chan_id:        .res 4      ; sound id owning this channel, IDLE = free
chan_dirty:     .res 4      ; 1 = registers need pushing to hardware
chan_reg1:      .res 4      ; duty/volume  -> $x000
chan_reg2:      .res 4      ; sweep/ramp   -> $x001
chan_reg3:      .res 4      ; FT (timer lo)-> $x002
chan_reg4:      .res 4      ; CT (timer hi)-> $x003
chan_streamlo:  .res 4      ; ROM stream base pointer (low)
chan_streamhi:  .res 4      ; ROM stream base pointer (high)
chan_streampos: .res 4      ; current read offset into that stream
chan_notedur:   .res 4      ; cached duration for the *next* note
chan_duration:  .res 4      ; live countdown for the *current* note
chan_underlay:  .res 4      ; id to auto-resume when this channel goes
                            ; idle, IDLE = none (see design note 7)
pause_saved_id: .res 1      ; whatever was on PAUSE_CHANNEL right before
                            ; pause started (SND_MOVE, SND_ENGINE, or
                            ; IDLE) -- restored once free again, see
                            ; sound_set_paused
loop_counter:   .res 3      ; global loop counters 0/1/2 (see note 3 above)
snd_paused:     .res 1

PAUSE_CHANNEL = 1            ; Sound_Pause's data always says channel 2
                              ; (1-based) = square2 = index 1 (0-based)

.segment "CODE"

; ----------------------------------------------------------------------
; Public API
; ----------------------------------------------------------------------

.export _sound_init
.export _sound_update
.export _sound_stop_all
.export _sound_play
.export _sound_play_background
.export _sound_stop
.export _sound_set_paused
.export _sound_keepalive
.export _sound_is_playing
.export _sound_set_muted

SND_MASTERCTRL_REG = $4015
SND_FRAMECOUNTER   = $4017

_sound_init:
    lda #%00001111
    sta SND_MASTERCTRL_REG      ; enable square1/2, triangle, noise
    lda #%11000000
    sta SND_FRAMECOUNTER        ; disable frame IRQ (matches original)
    lda #0
    sta snd_paused
    sta snd_muted 
    sta loop_counter+0
    sta loop_counter+1
    sta loop_counter+2
    lda #IDLE
    sta pause_saved_id
    ldx #3
@clear:
    lda #IDLE
    sta chan_id,x
    sta chan_underlay,x
    lda #0
    sta chan_dirty,x
    jsr silence_channel
    dex
    bpl @clear
    rts

_sound_stop_all:
    lda #IDLE
    sta pause_saved_id
    ldx #3
@loop:
    lda #IDLE
    sta chan_id,x
    sta chan_underlay,x
    lda #0
    sta chan_dirty,x
    jsr silence_channel
    dex
    bpl @loop
    rts

_sound_set_muted:
    sta snd_muted
    rts


; Re-pushes every currently-playing channel's cached registers to
; hardware unconditionally (bypassing chan_dirty) -- this reloads
; each channel's APU length counter without touching bytecode
; position, chan_duration, or the tune's pacing at all. Call this
; periodically from inside a long CPU-bound stall (e.g. a stage
; decompress) that can't call sound_update() at real ~60Hz pace, so
; whatever's currently sounding doesn't hit hardware's automatic
; length-counter mute and go silent -- it just holds on the current
; note until sound_update() resumes normally. Safe to call as often
; as convenient; does nothing to a channel that's IDLE.
_sound_keepalive:
    ldx #3
@loop:
    lda chan_id,x
    cmp #IDLE
    beq @next
    lda #1
    sta chan_dirty,x
    jsr push_registers
@next:
    dex
    bpl @loop
    rts

; A = id to check. Returns 1 if this id currently owns its channel --
; i.e. is genuinely the thing sounding there right now -- 0 if not,
; including if something else preempted it since the last time you
; successfully sound_play()'d it. Read-only, no side effects. Meant
; for callers with their own "is this on?" latch (like SND_MOVE's, in
; C) so they can tell a stale latch from a real one: a sound_play()
; success only means "at this instant"; if something higher-priority
; can preempt it later (which, on a channel this shared, is most
; things), the latch alone can't tell you it happened.
_sound_is_playing:
    jsr resolve_channel
    lda snd_id
    cmp chan_id,x
    beq @yes
    lda #0
    rts
@yes:
    lda #1
    rts

_sound_set_paused:
    sta snd_paused
    beq @unpausing

    ; Entering pause: remember whatever is currently on the shared
    ; pause/move/engine channel -- SND_MOVE, SND_ENGINE, some other
    ; SFX mid-play, or IDLE -- so it can be put back afterward. Without
    ; this, the pause jingle (priority 0, always wins) permanently
    ; evicts it exactly like any other preemption, and chan_underlay
    ; only remembers SND_ENGINE, never SND_MOVE, so Move would never
    ; come back on its own even though the player may still be holding
    ; a direction the whole time.
    ldx #PAUSE_CHANNEL
    lda chan_id,x
    sta pause_saved_id

    ; Mute every channel except the one SND_PAUSE uses. (The original's
    ; per-frame SilencingLoop does this implicitly every frame by only
    ; scanning 1 slot while paused; we do it once, here.)
    ldx #0
@mute_loop:
    cpx #PAUSE_CHANNEL
    beq @mute_next
    jsr silence_channel
@mute_next:
    inx
    cpx #4
    bne @mute_loop
    rts

@unpausing:
    ; If the pause channel is idle now (the jingle finished), restore
    ; exactly what was preempted when pause started -- once. If it's
    ; still busy (pause toggled again almost instantly), leave it;
    ; it'll finish and fall through to the generic chan_underlay
    ; fallback below like any other preempted sound.
    ldx #PAUSE_CHANNEL
    lda chan_id,x
    cmp #IDLE
    bne @skip_restore
    lda pause_saved_id
    jsr launch_id_on_channel
@skip_restore:
    lda #IDLE
    sta pause_saved_id

    ; Any channel (including PAUSE_CHANNEL, if the restore above didn't
    ; apply) left idle by something that finished mid-pause resumes its
    ; registered background sound, if any.
    ldx #3
@resume_loop:
    lda chan_id,x
    cmp #IDLE
    bne @resume_next
    jsr maybe_resume_underlay
@resume_next:
    dex
    bpl @resume_loop
    rts

; A = id to force-stop. Also cancels its background-sound registration
; (see design note 7) if it has one, so it does not pop back up when
; whatever's currently on its channel finishes.
_sound_stop:
    jsr resolve_channel       ; -> X = channel, snd_id = A on entry

    lda snd_id
    cmp chan_underlay,x
    bne @not_underlay
    lda #IDLE
    sta chan_underlay,x       ; stopping it also cancels the registration
@not_underlay:

    lda snd_id
    cmp chan_id,x
    bne @done                 ; this id isn't the one currently playing
    jsr silence_channel
    lda #IDLE
    sta chan_id,x
    jsr maybe_resume_underlay ; channel just freed -> resume whatever
@done:                        ; else is registered there, if anything
    rts

; A = id to request. __fastcall__: value already in A on entry.
; Returns (in A): 1 if this claimed the channel -- it's now playing,
; or was restarted if it was already playing -- 0 if the channel is
; busy with a lower-numbered (= higher priority) sound and the
; request was ignored. Any caller that must eventually get this
; sound playing (SND_MOVE while the player is holding a direction,
; say) needs to check this and keep retrying each frame until it
; sees 1, rather than assuming success -- otherwise a request denied
; once (e.g. by the stage-start jingle still playing) is lost for
; good, with nothing left to ever ask again.
_sound_play:
    ldx snd_muted
    bne @muted
    jsr resolve_channel
    jmp try_allocate
    @muted:
    lda #0
    rts

; A = id to request as this channel's background/ambient sound (see
; design note 7). Registers it so it auto-resumes whenever its channel
; goes idle, then attempts an immediate claim exactly like sound_play
; -- if the channel is currently busy with something higher-priority,
; the claim is silently deferred and happens automatically once that
; sound finishes or is sound_stop()'d.
_sound_play_background:
    ldx snd_muted
    bne @muted
    jsr resolve_channel
    lda snd_id
    sta chan_underlay,x
    jmp try_allocate
    @muted:
    lda #0
    rts

; ----------------------------------------------------------------------
; Internal: A = sound id -> snd_id = id, snd_ptr = its stream base,
; X = its (fixed, ROM-baked) channel 0..3. Shared by sound_play,
; sound_play_background and sound_stop.
; ----------------------------------------------------------------------

resolve_channel:
    sta snd_id
    asl a
    tax
    lda sound_ptr_tbl,x
    sta snd_ptr
    lda sound_ptr_tbl+1,x
    sta snd_ptr+1
    ldy #0
    lda (snd_ptr),y          ; first stream byte = channel, 1-based
    sec
    sbc #1
    tax                      ; X = channel 0..3
    rts

; ----------------------------------------------------------------------
; Internal: X = channel, snd_id/snd_ptr set by resolve_channel. Claims
; the channel for snd_id if priority allows (lower id wins; same id
; already playing restarts it; higher id is silently ignored).
; ----------------------------------------------------------------------

try_allocate:
    lda chan_id,x
    cmp #IDLE
    beq @allocate
    lda snd_id
    cmp chan_id,x
    beq @allocate            ; same id already playing -> restart it
    bcs @denied               ; new id numerically greater -> lower
                              ; priority than what's already playing
@allocate:
    jsr do_allocate_channel
    lda #1
    rts
@denied:
    lda #0
    rts

; ----------------------------------------------------------------------
; Internal: X = channel that just went idle. If a background sound is
; registered for it (design note 7), relaunch it. Only called from
; contexts where the interpreter for the *previous* occupant has fully
; unwound (never from inside an active interpret_loop frame), so it's
; safe for this to itself run a full do_allocate_channel.
; ----------------------------------------------------------------------

maybe_resume_underlay:
    lda snd_paused
    bne @paused                ; don't start anything new while paused --
                              ; e.g. SND_PAUSE finishing must not wake
                              ; SND_ENGINE mid-pause (same channel)
    lda chan_underlay,x
    jmp launch_id_on_channel   ; tail call; no-ops itself if id == IDLE
@paused:
    rts

; ----------------------------------------------------------------------
; Internal: A = id to launch (IDLE = do nothing), X = channel to launch
; it on (preserved). Only safe to call from a context where the
; interpreter for the *previous* occupant has fully unwound (see the
; comment above maybe_resume_underlay's call sites).
; ----------------------------------------------------------------------

launch_id_on_channel:
    cmp #IDLE
    beq @none
    sta snd_id
    asl a
    tay                       ; Y, not X: X must stay = channel
    lda sound_ptr_tbl,y
    sta snd_ptr
    lda sound_ptr_tbl+1,y
    sta snd_ptr+1
    jsr do_allocate_channel
@none:
    rts

; ----------------------------------------------------------------------
; sound_update — called once per logical frame
; ----------------------------------------------------------------------

_sound_update:
    lda snd_paused
    beq @normal

    ; Paused: only the channel that SND_PAUSE lives on keeps going.
    ldx #PAUSE_CHANNEL
    lda chan_id,x
    cmp #IDLE
    beq @done
    jsr process_channel
    rts

@normal:
    ldx #0
@loop:
    lda chan_id,x
    cmp #IDLE
    beq @next
    jsr process_channel
@next:
    inx
    cpx #4
    bne @loop
@done:
    rts

; ----------------------------------------------------------------------
; Internal: per-channel advance (duration countdown -> next opcode(s))
; X = channel
; ----------------------------------------------------------------------

process_channel:
    dec chan_duration,x
    lda chan_duration,x
    bne @push
    jsr interpret_loop
    lda chan_id,x
    cmp #IDLE
    beq @stopped
@push:
    jsr push_registers
    rts
@stopped:
    jsr silence_channel
    jsr maybe_resume_underlay
    rts

; ----------------------------------------------------------------------
; Internal: claim + parse header + run bytecode until first note/end.
; X = channel, snd_id = sound id, snd_ptr = stream base (byte 0 unread
; from caller's point of view is already consumed by the peek in
; sound_play, so we start the channel's own stream position at 1).
; ----------------------------------------------------------------------

do_allocate_channel:
    lda snd_ptr
    sta chan_streamlo,x
    lda snd_ptr+1
    sta chan_streamhi,x
    lda #1
    sta chan_streampos,x
    lda snd_id
    sta chan_id,x
    lda #1
    sta chan_dirty,x

    jsr read_next_byte          ; duty/volume
    sta chan_reg1,x
    jsr read_next_byte          ; sweep/ramp
    sta chan_reg2,x
    jsr read_next_byte          ; timer-hi placeholder
    sta chan_reg4,x

    cpx #3                      ; noise channel has one extra header byte
    bne @skip_noise
    jsr read_next_byte
    sta chan_reg3,x
@skip_noise:
    jsr interpret_loop

    lda chan_id,x
    cmp #IDLE
    beq @stopped
    jsr push_registers
    rts
@stopped:
    jsr silence_channel
    jsr maybe_resume_underlay
    rts

; ----------------------------------------------------------------------
; Internal: read one byte from this channel's stream, advance position.
; X = channel (preserved). Returns byte in A.
; ----------------------------------------------------------------------

read_next_byte:
    lda chan_streamlo,x
    sta snd_ptr
    lda chan_streamhi,x
    sta snd_ptr+1
    ldy chan_streampos,x
    lda (snd_ptr),y
    inc chan_streampos,x
    rts

; ----------------------------------------------------------------------
; Internal: bytecode interpreter body (X = channel, preserved throughout)
; Runs until: a note plays (yields, duration restarted) or the sound
; issues StopReset (chan_id becomes IDLE).
; ----------------------------------------------------------------------

interpret_loop:
@loop:
    jsr read_next_byte
    cmp #$E8
    bcs @special
    cmp #$60
    beq @note_repeat
    bcc @pitch_note
    ; duration-set byte, range $61-$E7
    sec
    sbc #$60
    sta chan_notedur,x
    jmp @loop
@pitch_note:
    jsr process_pitch
@note_repeat:
    lda chan_notedur,x
    sta chan_duration,x
    rts
@special:
    sbc #$E8                 ; carry still holds the result of "CMP #$E8"
    jsr dispatch_special
    lda chan_id,x
    cmp #IDLE
    beq @stopped
    jmp @loop
@stopped:
    rts

; ----------------------------------------------------------------------
; Internal: decode a pitch byte (0x00-0x5F) into the FT/CT register
; cache and mark the channel dirty. X = channel, A = pitch byte.
; ----------------------------------------------------------------------

process_pitch:
    pha
    and #$F8
    lsr a
    lsr a
    tay
    lda freq_lut,y
    sta snd_temp
    lda freq_lut+1,y
    sta snd_temp2
    pla
    and #$07
    beq @noshift
    tay
@shiftloop:
    lsr snd_temp
    ror snd_temp2
    dey
    bne @shiftloop
@noshift:
    lda chan_reg4,x
    and #$F8
    ora snd_temp
    sta chan_reg4,x
    lda snd_temp2
    sta chan_reg3,x
    lda #1
    sta chan_dirty,x
    rts

; ----------------------------------------------------------------------
; Internal: special-command dispatch. X = channel (preserved),
; A = opcode index 0..17.
; ----------------------------------------------------------------------

dispatch_special:
    asl a
    tay
    lda command_jumptable,y
    sta snd_temp
    lda command_jumptable+1,y
    sta snd_temp2
    jsr call_indirect
    rts

call_indirect:
    jmp (snd_temp)

; -- individual command handlers (all preserve X = channel) -----------

cmd_stop_reset:
    lda #IDLE
    sta chan_id,x
    rts

cmd_set_duty_cycle:
    jsr read_next_byte
    sta snd_temp
    lda chan_reg1,x
    and #%00111111
    ora snd_temp
    sta chan_reg1,x
    rts

cmd_set_volume:
    jsr read_next_byte
    sta snd_temp
    lda chan_reg1,x
    and #%11000000
    ora snd_temp
    sta chan_reg1,x
    rts

cmd_set_volume_alt:
    jmp cmd_set_volume        ; see design note 4 above

cmd_set_sweep:
    jsr read_next_byte
    sta chan_reg2,x
    rts

cmd_set_timer_high:
    jsr read_next_byte
    sta chan_reg4,x
    rts

cmd_set_duty_volume:
    jsr read_next_byte
    sta chan_reg1,x
    rts

cmd_clear_counters:
    lda #0
    sta loop_counter+0
    sta loop_counter+1
    sta loop_counter+2
    rts

cmd_loopcount0:
    ldy #0
    jmp loopcount_common
cmd_loopcount1:
    ldy #1
    jmp loopcount_common
cmd_loopcount2:
    ldy #2
    ; fall through

loopcount_common:
    sty snd_temp2             ; save loop-counter index (Y is clobbered
                              ; by read_next_byte below)
    jsr read_next_byte        ; A = threshold
    sta snd_temp               ; stash it (snd_temp/snd_temp2 untouched
                              ; by read_next_byte, safe here)
    ldy snd_temp2
    lda loop_counter,y         ; 6502 has no INC abs,Y -> increment via A
    clc
    adc #1
    sta loop_counter,y
    lda snd_temp                ; restore threshold
    cmp loop_counter,y          ; CMP abs,Y is legal
    bne cmd_jump_to_offset    ; not reached yet -> take the jump
    lda #0
    sta loop_counter,y
    jmp cmd_advance_pointer   ; reached -> just skip the target byte

cmd_advance_pointer:
    jsr read_next_byte        ; consume + discard the target byte
    rts

cmd_jump_to_offset:
    jsr read_next_byte        ; A = new stream offset
    sta chan_streampos,x
    rts

; ----------------------------------------------------------------------
; Internal: push cached registers to hardware if dirty. X = channel.
; ----------------------------------------------------------------------

push_registers:
    lda chan_dirty,x
    beq @done
    lda #0
    sta chan_dirty,x
    txa
    asl a
    asl a
    tay                       ; Y = channel*4 = register offset from $4000
    lda chan_reg1,x
    sta $4000,y
    lda chan_reg2,x
    sta $4001,y
    lda chan_reg3,x
    sta $4002,y
    lda chan_reg4,x
    sta $4003,y
@done:
    rts

; ----------------------------------------------------------------------
; Internal: silence one channel's control register. X = channel.
; Matches the original SilencingLoop's exact byte pattern.
; ----------------------------------------------------------------------

silence_channel:
    lda #0
    sta chan_dirty,x
    txa
    asl a
    asl a
    tay
    lda silence_lut,x
    sta $4000,y
    rts

silence_lut: .byte $10, $10, $00, $00

; ----------------------------------------------------------------------
; Command jump table (18 entries, opcode = $E8 + index)
; ----------------------------------------------------------------------

command_jumptable:
    .word cmd_stop_reset          ; $E8
    .word cmd_set_duty_cycle      ; $E9
    .word cmd_set_volume          ; $EA
    .word cmd_set_volume_alt      ; $EB (reconstructed, see note 4)
    .word cmd_set_sweep           ; $EC
    .word cmd_set_timer_high      ; $ED
    .word cmd_set_duty_volume     ; $EE
    .word cmd_clear_counters      ; $EF
    .word cmd_loopcount0          ; $F0
    .word cmd_loopcount1          ; $F1
    .word cmd_loopcount2          ; $F2
    .word cmd_advance_pointer     ; $F3
    .word cmd_advance_pointer     ; $F4
    .word cmd_advance_pointer     ; $F5
    .word cmd_advance_pointer     ; $F6
    .word cmd_advance_pointer     ; $F7
    .word cmd_advance_pointer     ; $F8
    .word cmd_jump_to_offset      ; $F9

; ----------------------------------------------------------------------
; Frequency lookup table — verbatim from the original ROM.
; ----------------------------------------------------------------------

freq_lut:
    .word $F207, $8007, $1407, $AE06, $4306
    .word $F405, $9E05, $4E05, $0205, $BA04
    .word $7604, $3604

; ----------------------------------------------------------------------
; Sound pointer table — order matches SoundId enum in sound.h exactly.
; ----------------------------------------------------------------------

sound_ptr_tbl:
    .word Sound_Pause
    .word Sound_Battle1
    .word Sound_Battle2
    .word sound_Battle3
    .word sound_Ancillary_Life1
    .word sound_Ancillary_Life2
    .word sound_BonusTaken
    .word sound_PlayerExplode
    .word sound_Unknown1
    .word sound_BonusAppears
    .word sound_EnemyExplode
    .word sound_HQExplode
    .word sound_BrickRicochet
    .word sound_ArmourRicochetWall
    .word sound_ArmourRicochetTank
    .word sound_Shoot
    .word sound_Ice
    .word sound_Move
    .word sound_Engine
    .word sound_PtsCount1
    .word sound_PtsCount2
    .word sound_RecordPts1
    .word sound_RecordPts2
    .word sound_RecordPts3
    .word sound_GameOver1
    .word sound_GameOver2
    .word sound_GameOver3
    .word sound_BonusPts

; ----------------------------------------------------------------------
; Sound data
; ----------------------------------------------------------------------

Sound_Pause:
    .byte 2,$82,$7F,$40,$64,$1B,$2B,$3B,$1C,$2C,$3C,$6C,$53,$E8

Sound_Battle1:
    .byte 1,$81,$7F,$40,$EF,$68,$1B,$2B,$33,$F0,2,6,$33,$43,$53,$F0
    .byte 2,$C,$43,$53,4,$F0,2,$12,$5B,$C,$1C,$F0,2,$18,$78,$1C
    .byte $68,$1C,$1C,$1C,$78,$1C,$E8

Sound_Battle2:
    .byte 3,$10,$7F,8,$78,$1A,$68,$1A,$F1,3,7,$78,$32,$68,$32,$F1
    .byte 3,$E,$78,$42,$68,$42,$F1,3,$15,$5A,$F1,3,$19,$B,$F1,3
    .byte $1D,$78,$52,$68,$52,$F1,3,$24,$78,$52,$E8

sound_Battle3:
    .byte 2,$81,$7F,$40,$78,$51,$68,$51,$F2,3,7,$78,$A,$68,$A,$F2
    .byte 3,$E,$78,$1A,$68,$1A,$F2,3,$15,$32,$F2,3,$19,$42,$F2,3
    .byte $1D,$78,$3A,$68,$3A,$F2,3,$24,$78,$3A,$E8

sound_Ancillary_Life1:
    .byte 1,$A0,$7F,$40,$66,$1C,$3C,$1C,$53,$1C,$3C,5,$72,$54,$E8

sound_Ancillary_Life2:
    .byte 2,$90,$7F,$40,$62,$38,$66,$EA,$20,$3B,$53,$3B,$1B,$3B,$53,$1C
    .byte $6A,$14,$E8

sound_BonusTaken:
    .byte 2,$80,$7F,$40,$63,$52,$1B,$3B,$53,$4A,$13,$33,$4B,$1B,$3B,$53
    .byte $1C,$3C,$E8

sound_PlayerExplode:
    .byte 4,$1F,$7F,$30,$A,$62,$49,$49,$EA,$1E,$49,$49,$EA,$1D,$49,$49
    .byte $EA,$1C,$49,$49,$EA,$1B,$49,$49,$EA,$1A,$49,$EA,$19,$49,$EA,$18
    .byte $49,$E8

sound_Unknown1:
    .byte 2,$1F,$7F,$30,$62,0,1,0,$EA,$1E,1,0,$EA,$1D,1,0
    .byte 1,0,$EA,$1C,1,$EA,$1B,0,$EA,$1A,1,$EA,$19,0,$E8

sound_HQExplode:
    .byte 2,$20,$7F,$30,$63,$1A,$12,$51,$31,$19,$11,$50,$30,$18,$E8

sound_EnemyExplode:
    .byte 4,$1F,$7F,$40,$A,$62,$51,$EA,$1E,$51,$EA,8,$6A,$51,$E8

sound_Shoot:
    .byte 1,$8F,$82,$10,$6F,$2C,$E8

sound_BonusTaken2 = sound_BonusTaken   ; (unused alias guard, harmless)

sound_BonusAppears:
    .byte 2,$60,$7F,$40,$64,$52,$3A,$52,3,$52,3,$13,$1B,$E8

sound_ArmourRicochetWall:
    .byte 2,$D5,$7F,0,$62,$1C,$1D,$E8

sound_BrickRicochet:
    .byte 3,7,$7F,8,$61,$3A,$13,$22,$E8

sound_ArmourRicochetTank:
    .byte 2,$40,$7F,0,$61,$3D,$62,$45,$EA,$10,$28,$E8

sound_PtsCount1:
    .byte 2,$80,$7F,$18,$61,$39,$E8

sound_PtsCount2:
    .byte 4,0,$7F,$28,$A,$61,$28,$E8

sound_Engine:
    .byte 2,$8C,$94,$40,$61,$10,$64,$18,$F9,5

sound_Move:
    .byte 2,$80,$94,$48,$62,$40,$48,$F9,5

sound_Ice:
    .byte 1,$1F,$7F,$28,$61,$22,$42,$5A,$1B,$E8

sound_RecordPts1:
    .byte 1,$B8,$7F,$40,$EF,$65,$C,$53,$F0,$C,5,$C,$53,$F0,$C,$B
    .byte $34,$24,$F0,8,$10,$EA,$30,$B0,$50,$EA,$20,$9C,$54,$E8

sound_RecordPts2:
    .byte 2,$B8,$7F,$40,$65,$43,$33,$F1,$C,4,$43,$33,$F1,$C,$A,$14
    .byte $4B,$F1,8,$F,$EA,$3A,$30,$50,9,$29,$31,$51,$A,$2A,$32,$52
    .byte $B,$2B,$33,$53,$C,$2C,$9C,$EA,$20,$2C,$E8

sound_RecordPts3:
    .byte 3,0,$7F,8,$A1,1,1,$EE,$15,$6A,$B,$B,$B,$EE,$22,$6F
    .byte $33,$65,$43,$7E,$EE,$33,$53,$6A,$EE,$15,$43,$33,$53,$6F,$EE,$22
    .byte $13,$65,$23,$7E,$EE,$33,$33,$6A,$EE,$15,$23,$13,$4A,$9C,$EE,$FF
    .byte $32,$E8

sound_GameOver1:
    .byte 1,$42,$7F,$40,$66,$1B,$B,$78,$1B,$68,$52,$42,$32,$1A,$1A,$1A
    .byte $78,$1A,$E8

sound_GameOver2:
    .byte 2,$82,$7F,$40,$66,$52,$52,$78,$52,$68,$32,$2A,$12,$1A,$1A,$1A
    .byte $78,$1A,$E8

sound_GameOver3:
    .byte 3,$10,$7F,8,$66,$3B,$33,$78,$3B,$68,$1B,$B,$52,$52,$52,$52
    .byte $78,$52,$E8

sound_BonusPts:
    .byte 2,$82,$7F,$40,$63,$53,$1B,$1C,$3B,$3C,$53,$6A,$54,$E8