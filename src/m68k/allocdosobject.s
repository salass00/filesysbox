;
; Copyright (c) 2013-2026 Fredrik Wikstrom
;
; This code is released under AROS PUBLIC LICENSE 1.1
; See the file LICENSE.APL
;

Functions:
 dc.w FbxAllocDosObject-Functions
 dc.w FbxFreeDosObject-Functions

SysAllocDosObject ds.l 1
SysFreeDosObject  ds.l 1
SysBase           ds.l 1
UtilityBase       ds.l 1

 include <exec/nodes.i>
 include <exec/memory.i>
 include <exec/execbase.i>
 include <dos/dos.i>
 include <dos/dosextens.i>
 include <dos/dostags.i>

 include <lvo/exec_lib.i>
 include <lvo/utility_lib.i>

DOS_EXAMINEDATA EQU 11

QUAD MACRO
	STRUCT \1,8
	ENDM

UQUAD MACRO
	STRUCT \1,8
	ENDM

 STRUCTURE ExamineData,0
	STRUCT exd_node,MLN_SIZE
	ULONG  exd_info1
	ULONG  exd_info2

	APTR   exd_FSPrivate
	APTR   exd_DOSPrivate

	ULONG  exd_StructSize
	ULONG  exd_Type
	QUAD   exd_FileSize
	STRUCT exd_Date,ds_SIZEOF
	ULONG  exd_RefCount
	UQUAD  exd_ObjectID
	APTR   exd_Name
	ULONG  exd_NameSize
	APTR   exd_Comment
	ULONG  exd_CommentSize
	APTR   exd_Link
	ULONG  exd_LinkSize
	ULONG  exd_Protection
	ULONG  exd_OwnerUID
	ULONG  exd_OwnerGID
	STRUCT exd_Reserved,12
	LABEL  exd_SIZEOF

ADO_ExamineData_NameSize    EQU ADO_Dummy+20
ADO_ExamineData_CommentSize EQU ADO_Dummy+21
ADO_ExamineData_LinkSize    EQU ADO_Dummy+22

FbxAllocDosObject:
	cmpi.l  #DOS_EXAMINEDATA,d1
	beq.s   alloc_exd

	; Use system AllocDosObject
	move.l  SysAllocDosObject(pc),a0
	jmp     (a0)

alloc_exd:
	movem.l d2-d4/a6,-(a7)

	; Taglist scanning
	move.l  UtilityBase(pc),a6
	move.l  d2,-(a7)
	moveq.l #1,d2 ; default exd_NameSize
	moveq.l #1,d3 ; default exd_CommentSize
	moveq.l #1,d4 ; default exd_LinkSize
	bra.s   next_tag
have_tag:
	move.l  ti_Tag(a0),d0
	cmpi.l  #ADO_ExamineData_NameSize,d0
	bne.s   not_name_size
	move.l  ti_Data(a0),d1
	beq.s   next_tag
	move.l  d1,d2
	bra.s   next_tag
not_name_size:
	cmpi.l  #ADO_ExamineData_CommentSize,d0
	bne.s   not_comment_size
	move.l  ti_Data(a0),d1
	beq.s   next_tag
	move.l  d1,d3
	bra.s   next_tag
not_comment_size:
	cmpi.l  #ADO_ExamineData_LinkSize,d0
	bne.s   next_tag
	move.l  ti_Data(a0),d1
	beq.s   next_tag
	move.l  d1,d4
next_tag:
	move.l  a7,a0
	jsr     _LVONextTagItem(a6)
	tst.l   d0
	movea.l d0,a0
	bne.s   have_tag
	addq.w  #4,a7

	; Allocate ExamineData struct
	move.l  SysBase(pc),a6
	moveq.l #exd_SIZEOF,d0
	add.l   d2,d0
	add.l   d3,d0
	add.l   d4,d0
	move.l  #MEMF_PUBLIC|MEMF_CLEAR,d1
	jsr     _LVOAllocVec(a6)
	tst.l   d0
	movea.l d0,a0
	bne.s   have_mem

	; Not enough free memory
	move.l  ThisTask(a6),a0
	move.l  #ERROR_NO_FREE_STORE,pr_Result2(a0)
	movem.l (a7)+,d2-d4/a6
	rts

	; Initialize structure
have_mem:
	move.l  #exd_SIZEOF,exd_StructSize(a0)
	moveq.l #-1,d0
	move.l  d0,exd_FileSize(a0)
	move.l  d0,exd_FileSize+4(a0)
	lea     exd_SIZEOF(a0),a1
	move.b  #0,(a1)
	move.l  a1,exd_Name(a0)
	move.l  d2,exd_NameSize(a0)
	add.l   d2,a1
	move.b  #0,(a1)
	move.l  a1,exd_Comment(a0)
	move.l  d3,exd_CommentSize(a0)
	add.l   d3,a1
	move.b  #0,(a1)
	move.l  a1,exd_Link(a0)
	move.l  d4,exd_LinkSize(a0)
	movem.l (a7)+,d2-d4/a6
	rts

FbxFreeDosObject:
	cmpi.l  #DOS_EXAMINEDATA,d1
	beq.s   free_exd

	; Use system FreeDosObject
	move.l  SysFreeDosObject(pc),a0
	jmp     (a0)

free_exd:
	move.l a6,-(a7)
	move.l SysBase(pc),a6
	move.l d2,a1
	jsr    _LVOFreeVec(a6)
	move.l (a7)+,a6
	rts

