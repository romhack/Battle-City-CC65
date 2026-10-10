# Battle City — NES port in C (cc65)

## What this is

The original NES *Battle City*, rewritten in C with
[cc65](https://cc65.github.io/) and built for the real console: mapper 0
(NROM), 32 KB PRG + 8 KB CHR, horizontal mirroring.

Game behaviour is preserved, down to the hidden message the original contains.
Nothing about the game itself was modified — this is a translation of the
existing game, not a remake.

## Why

* To find out whether compiled C can carry even a small commercial game on
  hardware this tight.
* To understand how the original works, especially its sound engine.
* To let other people study the internals in a more readable language.
* To give people who don't write 6502 assembly a lower barrier to modifying the
  game for real hardware.

Porting the game to other platforms is not a goal — SDL re-implementations
already exist on GitHub.

## What had to change

* **PRG grew from one 16 KB bank to two.** The C build is larger, and the
  original already used almost all of its single bank. The image is 40 KB
  instead of the original 24 KB.
* **The sound player was rewritten.** The original kept 28 virtual sound slots
  in RAM at all times; this version drives the four hardware channels directly
  and keeps only their state. That RAM is what the C runtime needed.

## Known issues

In the busiest moments — two players, six enemies and many bullets in flight —
the code sometimes fails to finish within a frame and the game stutters. It
follows from the larger instruction count compared with hand-written assembly.
The hot paths are heavily optimised (which is why parts of the code no longer
read like idiomatic C), but it is not enough yet.

## Building

Tested on Windows.

1. [cc65](https://cc65.github.io/), with `cc65`, `ca65` and `ld65` on `PATH`.
2. Python 3.
3. Your own copy of the original ROM, `Battle City (J).nes`, MD5
   `cd4fe2e78df0696dbe652f02c19541a1`, in the project root or in `rom/`
   (or point the extractor at it with `--rom PATH` or `BATTLE_CITY_ROM`).
4. Run `compile.bat`. The result is `build/battle-city-cc65.nes`.

The graphics and the stage layouts are not in the repository: the build reads
them out of your ROM into `assets/`, which git ignores.

Other platforms are untested, but nothing here is Windows-specific except the
batch file — `tools/extract_assets.py` is plain Python 3, and `compile.bat`
lists the `cc65`/`ca65`/`ld65` steps and flags to run by hand.

## Credits

The port was written following two sources:

* **[Programming NES games in C](https://shiru.untergrund.net/articles/programming_nes_games_in_c.htm)**
  by Shiru — the approach the whole port is based on. Shiru also wrote
  [`lib/neslib`](lib/neslib.h), the hardware layer used here, which is public
  domain.
* **[nesdoug](https://nesdoug.com/)** — the NES programming tutorial series.
  The neslib and startup code in this project started from nesdoug's versions
  of them.

## Legal

The code written for this project is released under the
[MIT License](LICENSE.md). That covers only this project's own code — it grants
no rights to the original game, its graphics, stage layouts or sound.

No graphics or level data from the original game is distributed here. The CHR
bank and the 36 stage layouts belong to the original publisher; the build takes
them from your own copy of the ROM, and the extracted files are ignored by git.
Please keep them out of any fork or release.

*Battle City* and *Namco* are trademarks of their respective owners.
