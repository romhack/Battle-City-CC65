#ifndef SOUND_H
#define SOUND_H

/* Sound IDs — identical order to the original Sound_PtrTbl, so the
 * numeric value doubles as "priority" (lower id = higher priority)
 * when two sounds want the same hardware channel at once. */
typedef enum {
    SND_PAUSE = 0,
    SND_BATTLE1,
    SND_BATTLE2,
    SND_BATTLE3,
    SND_ANCILLARY_LIFE1,
    SND_ANCILLARY_LIFE2,
    SND_BONUS_TAKEN,
    SND_PLAYER_EXPLODE,
    SND_UNKNOWN1,
    SND_BONUS_APPEARS,
    SND_ENEMY_EXPLODE,
    SND_HQ_EXPLODE,
    SND_BRICK_RICOCHET,
    SND_ARMOUR_RICOCHET_WALL,
    SND_ARMOUR_RICOCHET_TANK,
    SND_SHOOT,
    SND_ICE,
    SND_MOVE,
    SND_ENGINE,
    SND_PTS_COUNT1,
    SND_PTS_COUNT2,
    SND_RECORD_PTS1,
    SND_RECORD_PTS2,
    SND_RECORD_PTS3,
    SND_GAME_OVER1,
    SND_GAME_OVER2,
    SND_GAME_OVER3,
    SND_BONUS_PTS
} SoundId;

/* Call once at boot (after reset, before the first sound_update()). */
void sound_init(void);

/* Call once per logical frame (right after ppu_wait_nmi() is fine —
 * it does not need to run inside the real NMI handler). Advances the
 * bytecode interpreter for every active channel and pushes any
 * changed registers to the APU. */
void sound_update(void);

/* Silences everything and clears all channel state
 * (equivalent to the original's Sound_Stop). */
void sound_stop_all(void);

/* Request playback of a sound effect/music track. Resolves to one of
 * the 4 hardware channels via the sound's own data (byte 0 of its
 * stream). Re-requesting the same id that is already playing
 * restarts it from the top.
 * Returns 1 if this claimed the channel (now playing / restarted),
 * or 0 if the channel is busy with a lower-numbered (= higher
 * priority) sound and the request was ignored. If you need this
 * sound to eventually play no matter what -- e.g. SND_MOVE while a
 * direction is held -- check the return value and keep retrying each
 * frame until it's 1; don't assume success, or a request denied once
 * (by whatever else happened to be on that channel at that moment)
 * is lost for good with nothing left to ask again. Calling this
 * again while already 1 does nothing harmful (see above) but do
 * avoid it anyway once you have a confirmed 1, since it restarts. */
unsigned char __fastcall__ sound_play(unsigned char id);

/* Like sound_play(), but also registers 'id' as its channel's
 * background/ambient sound: whenever that channel goes idle again —
 * because whatever preempted it finishes on its own, or because it
 * is explicitly sound_stop()'d — 'id' automatically resumes (from
 * the top; there's no mid-note resume, same as any sound_play()).
 * Call this once, e.g. at level start, instead of manually re-playing
 * it after every other sound sharing its channel finishes — which,
 * for SND_ENGINE, is most of them (Pause, BonusTaken, every
 * ricochet, HQ explode, SND_MOVE, PtsCount1, ...). 'id' still has to
 * be a genuine "infinite loop" stream like SND_ENGINE/SND_MOVE that
 * never reaches its own StopReset — this only handles it being
 * preempted and returning, not it stopping itself. */
void __fastcall__ sound_play_background(unsigned char id);

/* Force-stop a specific sound if it currently owns a channel.
 * Needed for the two "infinite loop" streams (SND_MOVE, SND_ENGINE)
 * which never reach a StopReset opcode on their own — the game has
 * to explicitly turn them off, exactly like original Snd_Move=0.
 * If 'id' was registered via sound_play_background(), this also
 * cancels that registration (so it won't come back on its own —
 * use this, not just letting something else preempt it, whenever you
 * want it gone for good, e.g. at level end). */
void __fastcall__ sound_stop(unsigned char id);

/* paused != 0: only the channel used by SND_PAUSE keeps advancing and
 * stays audible; every other channel is muted immediately (matches
 * the original's actual behaviour — its per-frame SilencingLoop mutes
 * every channel not scanned that frame, and while paused only 1 of 28
 * slots is scanned). paused == 0: any channel left idle by something
 * that finished mid-pause (e.g. the pause jingle itself, which shares
 * SND_ENGINE's channel) resumes its registered background sound, if
 * any, right away. */
void __fastcall__ sound_set_paused(unsigned char paused);

/* Returns 1 if 'id' currently owns its channel -- is genuinely the
 * thing sounding there right now -- 0 otherwise, including if
 * something else has preempted it since you last successfully
 * sound_play()'d it. Read-only, no side effects.
 * A sound_play() success only tells you "at that instant"; on a
 * heavily-shared channel almost anything can preempt it later (see
 * sound_play()'s doc). If you keep your own "is this on?" latch for
 * something that must stay playing whenever some condition holds
 * (SND_MOVE while a direction is held, say), don't trust the latch
 * alone -- check this every time too, e.g.:
 *   if (moving && (!snd_move_active || !sound_is_playing(SND_MOVE)))
 *       if (sound_play(SND_MOVE)) snd_move_active = 1;
 * so a silent mid-play preemption gets retried instead of leaving the
 * latch stuck "on" for something that's no longer actually sounding. */
unsigned char __fastcall__ sound_is_playing(unsigned char id);

/* Re-pushes every currently-playing channel's cached registers to
 * hardware without advancing playback at all (no bytecode step, no
 * duration countdown). Call this periodically from inside a long
 * CPU-bound stall that can't call sound_update() at the normal ~60Hz
 * pace (e.g. a stage decompress/build loop), so the hardware's
 * automatic length-counter mute doesn't cut off whatever's currently
 * playing -- it just holds on the current note until sound_update()
 * resumes. Does not need to run at any particular rate; call it every
 * so often inside the stall's own loop. */
void __fastcall__ sound_keepalive(void);

void __fastcall__ sound_set_muted(unsigned char muted);

#endif