;
; Copyright (c) 2013-2026 Fredrik Wikstrom
;
; This code is released under AROS PUBLIC LICENSE 1.1
; See the file LICENSE.APL
;

FBX_MIN_STACK EQU 16384

 include <exec/memory.i>
 include <exec/libraries.i>
 include <exec/tasks.i>
 include <exec/alerts.i>
 include <dos/dos.i>
 include <lvo/exec_lib.i>

 STRUCTURE FileSysBoxBase,LIB_SIZE
	UWORD fbxlib_pad
	BPTR  fbxlib_SegList
	APTR  fbxlib_SysBase
	APTR  fbxlib_DOSBase
	APTR  fbxlib_UtilityBase
	APTR  fbxlib_LocaleBase

 STRUCTURE FbxFS,StackSwapStruct_SIZEOF
	APTR  fbxfs_LibBase ; FileSysBoxBase

	XREF _FbxEventLoop

	; Parameters:
	;   a0 - struct FbxFS *fs
	;   a6 - struct FileSysBoxBase *libBase
	XDEF _FbxEventLoop_SS
_FbxEventLoop_SS:
	movem.l d7/a4-a5,-(a7) ; save non-volatile registers

	; save libbase and fs
	move.l  a6,a4
	move.l  a0,a5

	; get task pointer
	move.l  fbxlib_SysBase(a4),a6
	suba.l  a1,a1
	jsr     _LVOFindTask(a6)

	; check stack bounds and size
	move.l  d0,a0
	move.l  a7,d0
	cmp.l   TC_SPUPPER(a0),d0
	bhi.s   swapstack
	sub.l   TC_SPLOWER(a0),d0
	blo.s   swapstack
	cmpi.l  #FBX_MIN_STACK-12,d0
	blo.s   swapstack

	; enough stack -> restore registers and call FbxEventLoop
	move.l  a4,a6
	move.l  a5,a0
	movem.l (a7)+,d7/a4-a5
	jmp     _FbxEventLoop

swapstack:
	; allocate memory for replacement stack
	move.l  #FBX_MIN_STACK,d0
	moveq.l #MEMF_ANY,d1
	jsr     _LVOAllocMem(a6)

	; initialize stackswap struct
	move.l  d0,stk_Lower(a5)
	beq.s   nomem
	addi.l  #FBX_MIN_STACK,d0
	move.l  d0,stk_Upper(a5)
	move.l  d0,stk_Pointer(a5)

	move.l  a5,a0
	jsr     _LVOStackSwap(a6)

	move.l  a4,a6
	move.l  a5,a0
	jsr     _FbxEventLoop
	move.l  d0,d7 ; save return value

	move.l  fbxlib_SysBase(a4),a6
	move.l  a5,a0
	jsr     _LVOStackSwap(a6)

	move.l  stk_Lower(a5),a1
	move.l  #FBX_MIN_STACK,d0
	jsr     _LVOFreeMem(a6)

	; restore registers and return
	move.l  d7,d0
return:
	move.l  a4,a6
	movem.l (a7)+,d7/a4-a5
	rts

nomem:
	move.l  #AG_NoMemory,d7
	jsr     _LVOAlert(a6)
	moveq.l #-1,d0
	bra.s   return
