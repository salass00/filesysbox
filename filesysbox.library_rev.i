VERSION		EQU	54
REVISION	EQU	1

DATE	MACRO
		dc.b '14.1.2023'
		ENDM

VERS	MACRO
		dc.b 'filesysbox.library 54.1'
		ENDM

VSTRING	MACRO
		dc.b 'filesysbox.library 54.1 (14.1.2023)',13,10,0
		ENDM

VERSTAG	MACRO
		dc.b 0,'$VER: filesysbox.library 54.1 (14.1.2023)',0
		ENDM
