#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
extract_assets.py - recreate the graphics and stage data this port needs.

That data belongs to the original game, so it is not stored in the repository.
The script takes it from the user's own ROM so the port can be built locally.
See README.md for the ROM to supply and for the legal notes.
"""

import argparse
import hashlib
import os
import sys

# Checksums of the expected ROM and of the blocks cut out of it, so that a
# wrong or patched dump fails instead of quietly producing a broken build.
ROM_MD5 = "cd4fe2e78df0696dbe652f02c19541a1"
CHR_MD5 = "f1df9774b21682185195ecd772be2ef0"
LEVELS_MD5 = "1ec0f706a73d126e665d17c0df0057f5"

# Layout of the original ROM.
INES_HEADER = 16
PRG_BANK = 16384
CHR_OFFSET = 0x4010
CHR_LENGTH = 8192
LEVELS_OFFSET = 0x308A
LEVEL_BYTES = 91
LEVEL_ROWS = 13
LEVEL_COLS = 13
LEVEL_COUNT = 36  # 35 stages + the demo stage
LEVELS_LENGTH = LEVEL_BYTES * LEVEL_COUNT

ROM_FILENAME = "Battle City (J).nes"
ROM_SEARCH_DIRS = ("", "rom", "roms", "assets")

CHR_OUT = "graphics.chr"
LEVELS_OUT = "levels.bin"

# Nibble -> character for the ASCII stage previews.
LEVEL_CHARS = {
    0x0: "]",
    0x1: "_",
    0x2: "[",
    0x3: "-",
    0x4: "#",
    0x5: "}",
    0x6: "v",
    0x7: "{",
    0x8: "^",
    0x9: "@",
    0xA: "~",
    0xB: "T",
    0xC: "*",
    0xD: " ",
    0xE: " ",
    0xF: " ",
}


def project_root():
    return os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def md5_of(data):
    return hashlib.md5(data).hexdigest()


def md5_of_file(path):
    h = hashlib.md5()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(65536), b""):
            h.update(chunk)
    return h.hexdigest()


def rel(path, root):
    try:
        short = os.path.relpath(path, root)
    except ValueError:
        return path
    if short.split(os.sep)[0] == os.pardir:
        return path
    return short.replace(os.sep, "/")


def find_rom(root, explicit):
    if explicit:
        if not os.path.isfile(explicit):
            fail("ROM not found: %s" % explicit)
        return os.path.abspath(explicit)

    env = os.environ.get("BATTLE_CITY_ROM")
    if env:
        if not os.path.isfile(env):
            fail("BATTLE_CITY_ROM points at a missing file: %s" % env)
        return os.path.abspath(env)

    dirs = []
    for name in ROM_SEARCH_DIRS:
        d = os.path.join(root, name) if name else root
        if os.path.isdir(d):
            dirs.append(d)

    # 1) the canonical file name, case-insensitively
    for d in dirs:
        for entry in sorted(os.listdir(d)):
            if entry.lower() == ROM_FILENAME.lower() and \
                    os.path.isfile(os.path.join(d, entry)):
                return os.path.join(d, entry)

    # 2) any other .nes file directly in those directories
    candidates = []
    for d in dirs:
        for entry in sorted(os.listdir(d)):
            if entry.lower().endswith(".nes") and os.path.isfile(os.path.join(d, entry)):
                candidates.append(os.path.join(d, entry))

    if len(candidates) == 1:
        return candidates[0]

    if candidates:
        print("Several .nes files were found:")
        for c in candidates:
            print("  %s" % rel(c, root))
        fail("pass the right one with --rom PATH")
    return None


def fail(message):
    print("")
    print("ERROR: %s" % message)
    sys.exit(1)


def parse_offset(text):
    try:
        return int(text, 0)
    except ValueError:
        fail("'%s' is not a valid number (use decimal or 0x... hex)" % text)


def chr_offset_from_header(rom):
    if len(rom) < INES_HEADER or rom[0:4] != b"NES\x1a":
        return None, "the file has no iNES header"
    prg_banks = rom[4]
    chr_banks = rom[5]
    offset = INES_HEADER + prg_banks * PRG_BANK
    note = "iNES header: %d x 16 KB PRG, %d x 8 KB CHR" % (prg_banks, chr_banks)
    if chr_banks * 8192 != CHR_LENGTH:
        note += " (warning: header declares %d bytes of CHR, expected %d)" % (
            chr_banks * 8192, CHR_LENGTH)
    return offset, note


def check_slice(rom, offset, length, what):
    if offset < 0:
        fail("%s offset is negative" % what)
    if offset + length > len(rom):
        fail("%s needs bytes 0x%X-0x%X but the ROM is only %d bytes long "
             "(0x%X)" % (what, offset, offset + length - 1, len(rom), len(rom)))


def dump_levels(blob, out_dir):
    if not os.path.isdir(out_dir):
        os.makedirs(out_dir)
    written = []
    for idx in range(LEVEL_COUNT):
        stage = blob[idx * LEVEL_BYTES:(idx + 1) * LEVEL_BYTES]
        lines = []
        for r in range(LEVEL_ROWS):
            row = stage[r * 7:(r + 1) * 7]
            nibbles = []
            for byte in row:
                nibbles.append(byte >> 4)
                nibbles.append(byte & 0x0F)
            lines.append("".join(LEVEL_CHARS[n] for n in nibbles[:LEVEL_COLS]))
        if idx == LEVEL_COUNT - 1:
            name = "demo level.txt"
        else:
            name = "%d_level.txt" % (idx + 1)
        path = os.path.join(out_dir, name)
        with open(path, "w", newline="\n") as fh:
            fh.write("\n".join(lines) + "\n")
        written.append(path)
    return written


def write_if_needed(path, data, force):
    """Return 'written', 'unchanged' or 'replaced'."""
    existed = os.path.isfile(path)
    if existed and not force:
        try:
            if md5_of_file(path) == md5_of(data):
                return "unchanged"
        except OSError:
            pass
    d = os.path.dirname(path)
    if d and not os.path.isdir(d):
        os.makedirs(d)
    with open(path, "wb") as fh:
        fh.write(data)
    return "replaced" if existed else "written"


def main(argv=None):
    parser = argparse.ArgumentParser(
        prog="extract_assets.py",
        description="Recreate the graphics and stage data this port needs from "
                    "the original ROM.  See README.md.")
    parser.add_argument("--rom", metavar="PATH",
                        help="path to the original 'Battle City (J).nes' "
                             "(default: search ./ , ./rom/ , ./roms/ , "
                             "./assets/ , or $BATTLE_CITY_ROM)")
    parser.add_argument("--out-dir", metavar="DIR",
                        help="where to write graphics.chr and levels.bin "
                             "(default: <project>/assets)")
    parser.add_argument("--check", action="store_true",
                        help="verify the ROM and the files on disk, write nothing")
    parser.add_argument("--dump-levels", action="store_true",
                        help="also write ASCII stage previews to "
                             "build/level_preview/ (derived data, gitignored)")
    parser.add_argument("--clean", action="store_true",
                        help="delete the generated asset files and exit")
    parser.add_argument("--force", action="store_true",
                        help="rewrite the generated files even if up to date")
    parser.add_argument("--allow-any-rom", action="store_true",
                        help="skip the known-ROM checksum gate (for other "
                             "dumps/revisions); offsets are still validated")
    parser.add_argument("--chr-offset", metavar="N",
                        help="override the CHR file offset (default 0x4010, or "
                             "computed from the iNES header)")
    parser.add_argument("--levels-offset", metavar="N",
                        help="override the stage data file offset "
                             "(default 0x308A)")
    args = parser.parse_args(argv)

    root = project_root()
    out_dir = os.path.abspath(args.out_dir) if args.out_dir \
        else os.path.join(root, "assets")
    chr_path = os.path.join(out_dir, CHR_OUT)
    levels_path = os.path.join(out_dir, LEVELS_OUT)

    if args.clean:
        removed = []
        for path in (chr_path, levels_path):
            if os.path.isfile(path):
                os.remove(path)
                removed.append(path)
        for path in removed:
            print("removed %s" % rel(path, root))
        if not removed:
            print("nothing to clean")
        return 0

    print("Battle City (J) asset extractor")
    print("-------------------------------")

    rom_path = find_rom(root, args.rom)
    if rom_path is None:
        print("")
        print("No ROM was found.")
        print("")
        print("Put your copy of the original game next to this project (or in")
        print("rom/) as:")
        print("")
        print("    %s" % ROM_FILENAME)
        print("    MD5 %s" % ROM_MD5)
        print("")
        print("...or pass its location with --rom PATH, or point the")
        print("BATTLE_CITY_ROM environment variable at it.")
        sys.exit(1)

    rom = open(rom_path, "rb").read()
    rom_md5 = md5_of(rom)
    print("ROM      : %s" % rel(rom_path, root))
    print("           %d bytes, MD5 %s" % (len(rom), rom_md5))

    if rom_md5 != ROM_MD5:
        if not args.allow_any_rom:
            print("")
            print("This is not the expected ROM.  Expected MD5")
            print("    %s" % ROM_MD5)
            print("Refusing to extract from an unknown dump, because a wrong or")
            print("patched ROM would silently produce a broken build.")
            print("")
            print("If you know what you are doing, re-run with --allow-any-rom")
            print("(optionally with --chr-offset/--levels-offset).")
            sys.exit(1)
        print("           WARNING: unknown ROM, checksum not verified")
    else:
        print("           checksum verified")

    if args.chr_offset is not None:
        chr_offset = parse_offset(args.chr_offset)
        chr_note = "offset given on the command line"
    else:
        header_offset, header_note = chr_offset_from_header(rom)
        if header_offset is None:
            chr_offset = CHR_OFFSET
            chr_note = "no iNES header, assuming offset 0x%04X" % CHR_OFFSET
        else:
            chr_offset = header_offset
            chr_note = header_note
    check_slice(rom, chr_offset, CHR_LENGTH, "CHR bank")
    chr = rom[chr_offset:chr_offset + CHR_LENGTH]

    levels_offset = parse_offset(args.levels_offset) if args.levels_offset \
        is not None else LEVELS_OFFSET
    check_slice(rom, levels_offset, LEVELS_LENGTH, "stage data")
    levels = rom[levels_offset:levels_offset + LEVELS_LENGTH]

    problems = []
    chr_md5 = md5_of(chr)
    levels_md5 = md5_of(levels)
    if chr_md5 != CHR_MD5:
        problems.append("CHR bank     : got MD5 %s, expected %s" %
                        (chr_md5, CHR_MD5))
    if levels_md5 != LEVELS_MD5:
        problems.append("stage data   : got MD5 %s, expected %s" %
                        (levels_md5, LEVELS_MD5))
    if problems and not args.allow_any_rom:
        print("")
        for p in problems:
            print("MISMATCH %s" % p)
        print("")
        print("The ROM matches, but the extracted blocks do not.  Pass")
        print("--chr-offset/--levels-offset, or --allow-any-rom to accept it.")
        sys.exit(1)

    print("")
    print("CHR bank : 0x%04X-0x%04X (%d bytes)  MD5 %s%s"
          % (chr_offset, chr_offset + CHR_LENGTH - 1, CHR_LENGTH, chr_md5,
             "" if chr_md5 == CHR_MD5 else "  [UNVERIFIED]"))
    print("           %s" % chr_note)
    print("Stage data: 0x%04X-0x%04X (%d bytes, %d stages x %d bytes)  MD5 %s%s"
          % (levels_offset, levels_offset + LEVELS_LENGTH - 1, LEVELS_LENGTH,
             LEVEL_COUNT, LEVEL_BYTES, levels_md5,
             "" if levels_md5 == LEVELS_MD5 else "  [UNVERIFIED]"))
    print("           35 playable stages + the title-screen demo stage")
    print("")

    if args.check:
        ok = True
        for path, data in ((chr_path, chr), (levels_path, levels)):
            if not os.path.isfile(path):
                print("MISSING  %s" % rel(path, root))
                ok = False
            elif md5_of_file(path) != md5_of(data):
                print("STALE    %s" % rel(path, root))
                ok = False
            else:
                print("ok       %s" % rel(path, root))
        if not ok:
            print("")
            print("Run the extractor without --check to refresh the assets.")
            return 1
        print("")
        print("All assets are up to date.")
        return 0

    for path, data in ((chr_path, chr), (levels_path, levels)):
        action = write_if_needed(path, data, args.force)
        print("%-9s %s (%d bytes)" % (action, rel(path, root), len(data)))

    if args.dump_levels:
        preview_dir = os.path.join(root, "build", "level_preview")
        written = dump_levels(levels, preview_dir)
        print("")
        print("Wrote %d ASCII stage previews to %s"
              % (len(written), rel(preview_dir, root)))

    print("")
    print("Done.  Build the ROM with compile.bat.")
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(1)
