VERSION = 54
REVISION = 1

.macro DATE
.ascii "14.1.2023"
.endm

.macro VERS
.ascii "filesysbox.library 54.1"
.endm

.macro VSTRING
.ascii "filesysbox.library 54.1 (14.1.2023)"
.byte 13,10,0
.endm

.macro VERSTAG
.byte 0
.ascii "$VER: filesysbox.library 54.1 (14.1.2023)"
.byte 0
.endm
