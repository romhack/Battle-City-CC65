; Stage layout data.  The original data is not part of this repository, so the
; extractor writes it into assets/ before the build.  See README.md.

.export _levels

.segment "RODATA"

_levels:
        .incbin "assets/levels.bin"
