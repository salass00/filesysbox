/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

SIPTR FbxDoPacket(struct FbxFS *fs, struct DosPacket *pkt) {
	LONG type;
	SIPTR r1;
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	#define BTOC(arg) ((const char *)(arg))
	#define BTOC2(arg) ((const char *)(arg))
#else
	char namebuf[256], namebuf2[256];
	#define BTOC(arg) ({ \
		CopyStringBSTRToC((arg), namebuf, sizeof(namebuf)); \
		namebuf;})
	#define BTOC2(arg) ({ \
		CopyStringBSTRToC((arg), namebuf2, sizeof(namebuf2)); \
		namebuf2;})
#endif

	PDEBUGF("FbxDoPacket(%#p, %#p)\n", fs, pkt);

#ifndef NODEBUG
	struct Task *callertask = pkt->dp_Port->mp_SigTask;
	PDEBUGF("action %ld task %#p '%s'\n", pkt->dp_Type, callertask, callertask->tc_Node.ln_Name);
#endif

	type = pkt->dp_Type;
	switch (type) {
	case ACTION_FINDUPDATE:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_READWRITE);
		break;
	case ACTION_FINDINPUT:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_OLDFILE);
		break;
	case ACTION_FINDOUTPUT:
		r1 = FbxOpenFile(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			MODE_NEWFILE);
		break;
	case ACTION_READ:
		r1 = FbxReadFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(APTR)pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_WRITE:
		r1 = FbxWriteFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(CONST_APTR)pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_SEEK:
		r1 = FbxSeekFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_END:
		r1 = FbxCloseFile(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_SET_FILE_SIZE:
		r1 = FbxSetFileSize(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			pkt->dp_Arg2, pkt->dp_Arg3);
		break;
	case ACTION_LOCATE_OBJECT:
		r1 = (SIPTR)MKBADDR(FbxLocateObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			BTOC(pkt->dp_Arg2), pkt->dp_Arg3));
		break;
	case ACTION_COPY_DIR:
	case ACTION_COPY_DIR_FH:
		r1 = (SIPTR)MKBADDR(FbxDupLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1)));
		break;
	case ACTION_FREE_LOCK:
		r1 = FbxUnLockObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_EXAMINE_OBJECT:
	case ACTION_EXAMINE_FH:
		r1 = FbxExamineLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FileInfoBlock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_EXAMINE_NEXT:
		r1 = FbxExamineNext(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FileInfoBlock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_CREATE_DIR:
		r1 = (SIPTR)MKBADDR(FbxCreateDir(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			BTOC(pkt->dp_Arg2)));
		break;
	case ACTION_DELETE_OBJECT:
		r1 = FbxDeleteObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2));
		break;
	case ACTION_RENAME_OBJECT:
		r1 = FbxRenameObject(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
			(struct FbxLock *)BADDR(pkt->dp_Arg3), BTOC2(pkt->dp_Arg4));
		break;
	case ACTION_PARENT:
	case ACTION_PARENT_FH:
		r1 = (SIPTR)MKBADDR(FbxLocateParent(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1)));
		break;
	case ACTION_SET_PROTECT:
		r1 = FbxSetProtection(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			pkt->dp_Arg4);
		break;
	case ACTION_SET_COMMENT:
		r1 = FbxSetComment(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			BTOC2(pkt->dp_Arg4));
		break;
	case ACTION_SET_DATE:
		r1 = FbxSetDate(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			(CONST_APTR)pkt->dp_Arg4);
		break;
	case ACTION_FH_FROM_LOCK:
		r1 = FbxOpenLock(fs, (struct FileHandle *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_SAME_LOCK:
		r1 = FbxSameLock(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(struct FbxLock *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_MAKE_LINK:
		if (pkt->dp_Arg4 == LINK_HARD) {
			r1 = FbxMakeHardLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
				(struct FbxLock *)BADDR(pkt->dp_Arg3));
		} else {
			r1 = FbxMakeSoftLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), BTOC(pkt->dp_Arg2),
				(const char *)pkt->dp_Arg3);
		}
		break;
	case ACTION_READ_LINK:
		r1 = FbxReadLink(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1),
			(const char *)pkt->dp_Arg2, (char *)pkt->dp_Arg3, pkt->dp_Arg4);
		break;
	case ACTION_CHANGE_MODE:
		if (pkt->dp_Arg1 == CHANGE_FH) {
			struct FileHandle *fh = BADDR(pkt->dp_Arg2);
			r1 = FbxChangeMode(fs, (struct FbxLock *)BADDR(fh->fh_Arg1), pkt->dp_Arg3);
		} else {
			r1 = FbxChangeMode(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), pkt->dp_Arg3);
		}
		break;
	case ACTION_EXAMINE_ALL:
		r1 = FbxExamineAll(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (APTR)pkt->dp_Arg2,
			pkt->dp_Arg3, pkt->dp_Arg4, (struct ExAllControl *)pkt->dp_Arg5);
		break;
	case ACTION_EXAMINE_ALL_END:
		r1 = FbxExamineAllEnd(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (APTR)pkt->dp_Arg2,
			pkt->dp_Arg3, pkt->dp_Arg4, (struct ExAllControl *)pkt->dp_Arg5);
		break;
	case ACTION_ADD_NOTIFY:
		r1 = FbxAddNotify(fs, (struct NotifyRequest *)pkt->dp_Arg1);
		break;
	case ACTION_REMOVE_NOTIFY:
		r1 = FbxRemoveNotify(fs, (struct NotifyRequest *)pkt->dp_Arg1);
		break;
	case ACTION_CURRENT_VOLUME:
		r1 = (SIPTR)FbxCurrentVolume(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_DISK_INFO:
		r1 = FbxDiskInfo(fs, (struct InfoData *)BADDR(pkt->dp_Arg1));
		break;
	case ACTION_INFO:
		r1 = FbxInfo(fs, (struct FbxLock *)BADDR(pkt->dp_Arg1), (struct InfoData *)BADDR(pkt->dp_Arg2));
		break;
	case ACTION_INHIBIT:
		r1 = FbxInhibit(fs, pkt->dp_Arg1);
		break;
	case ACTION_WRITE_PROTECT:
		r1 = FbxWriteProtect(fs, pkt->dp_Arg1, pkt->dp_Arg2);
		break;
	case ACTION_FORMAT:
		r1 = FbxFormat(fs, BTOC(pkt->dp_Arg1), pkt->dp_Arg2);
		break;
	case ACTION_RENAME_DISK:
		r1 = FbxRelabel(fs, BTOC(pkt->dp_Arg1));
		break;
	case ACTION_SET_OWNER:
		r1 = FbxSetOwnerInfo(fs, (struct FbxLock *)BADDR(pkt->dp_Arg2), BTOC(pkt->dp_Arg3),
			pkt->dp_Arg4 >> 16, pkt->dp_Arg4 & 0xffff);
		break;
	case ACTION_DIE:
		r1 = FbxDie(fs);
		break;
	case ACTION_IS_FILESYSTEM:
		r1 = DOSTRUE;
		fs->r2 = 0;
		break;
	case 0:
		r1 = DOSFALSE;
		fs->r2 = 0;
		break;
	default:
		r1 = DOSFALSE;
		fs->r2 = ERROR_ACTION_NOT_KNOWN;
		break;
	}

	PDEBUGF("Done with packet %#p. r1 %#p r2 %#p\n\n", pkt, (APTR)r1, (APTR)fs->r2);

	return r1;
}

