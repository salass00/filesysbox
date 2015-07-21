/*
 * Copyright (c) 2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <string.h>

struct FbxAsyncMsg {
	struct Message msg;
	struct FbxFS *fs;
	struct FbxVolume *vol;
	int cmd;
	char name[];
};

enum {
	FBX_ASYNC_ADD,
	FBX_ASYNC_REMOVE,
	FBX_ASYNC_REMOVE_FREE,
	FBX_ASYNC_RENAME
};

#ifdef __AROS__
static AROS_UFH3(int, FbxDosListProc,
	AROS_UFHA(STRPTR, argstr, A0),
	AROS_UFHA(ULONG, arglen, D0),
	AROS_UFHA(struct Library *, SysBase, A6)
)
{
	AROS_USERFUNC_INIT
#else
static int FbxDosListProc(void) {
	struct Library *SysBase = *(struct Library **)4;
#endif
	struct FileSysBoxBase *libBase;
	struct Library *DOSBase;
	struct Process *proc;
	struct MsgPort *port;
	struct FbxAsyncMsg *msg;
	ULONG signals;

	proc = (struct Process *)FindTask(NULL);
	port = &proc->pr_MsgPort;

	libBase = proc->pr_Task.tc_UserData;
	DOSBase = libBase->dosbase;

	for (;;) {
		signals = Wait(SIGBREAKF_CTRL_C|(1UL << port->mp_SigBit));

		if (!IsListEmpty(&port->mp_MsgList)) {
			LockDosList(LDF_ALL|LDF_WRITE);
			while ((msg = (struct FbxAsyncMsg *)GetMsg(port)) != NULL) {
				struct FbxVolume *vol = msg->vol;
				int cmd = msg->cmd;
				switch (cmd) {
				case FBX_ASYNC_ADD:
					AddDosEntry(&vol->dol);
					break;
				case FBX_ASYNC_REMOVE:
					RemDosEntry(&vol->dol);
					break;
				case FBX_ASYNC_REMOVE_FREE:
					if (RemDosEntry(&vol->dol))
						FreeFbxVolume(vol);
					break;
				case FBX_ASYNC_RENAME:
					if (RemDosEntry(&vol->dol)) {
						FbxStrlcpy(msg->fs, vol->volname, msg->name, CONN_VOLUME_NAME_BYTES);
						vol->volnamelen = strlen(vol->volname);
						AddDosEntry(&vol->dol);
					}
					break;
				}
				FreeMem(msg, msg->msg.mn_Length);
			}
			UnLockDosList(LDF_ALL|LDF_WRITE);
		}

		if (signals & SIGBREAKF_CTRL_C) {
			ObtainSemaphore(&libBase->dlproc_sem);
			if (libBase->dlproc_refcount == 0)
				break;
			ReleaseSemaphore(&libBase->dlproc_sem);
		}
	}

	while ((msg = (struct FbxAsyncMsg *)GetMsg(port)) != NULL)
		FreeMem(msg, msg->msg.mn_Length);

	libBase->dlproc = NULL;

	Forbid();
	ReleaseSemaphore(&libBase->dlproc_sem);
	return 0;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

struct Process *StartDosListProc(struct FileSysBoxBase *libBase) {
	struct Library *DOSBase = libBase->dosbase;

	return CreateNewProcTags(
		NP_Entry,       FbxDosListProc,
		NP_UserData,    libBase,
		NP_StackSize,   4096,
		NP_Name,        "FileSysBox DosList handler",
		NP_Priority,    15,
		NP_Cli,         FALSE,
		NP_WindowPtr,   -1,
		NP_CopyVars,    FALSE,
		NP_CurrentDir,  0,
		NP_HomeDir,     0,
		NP_Error,       0,
		NP_CloseError,  FALSE,
		NP_Input,       0,
		NP_CloseInput,  FALSE,
		NP_Output,      0,
		NP_CloseOutput, FALSE,
		NP_ConsoleTask, 0,
		TAG_END);
}

static int FbxAsyncDosListCmd(struct FbxFS *fs, struct FbxVolume *vol, int cmd, const char *name) {
	struct DosList *dl = NULL;
	int i, res;

	if (vol == NULL)
		return FALSE;

	GetSysBase
	GetDOSBase

	if (IsListEmpty(&fs->dlproc_port->mp_MsgList)) {
		for (i = 0; i < 10; i++) {
			dl = AttemptLockDosList(LDF_ALL|LDF_WRITE);
			if (dl != NULL)
				break;
			Delay(10);
		}
	}

	if (dl != NULL) {
		switch (cmd) {
		case FBX_ASYNC_ADD:
			res = AddDosEntry(&vol->dol);
			break;
		case FBX_ASYNC_REMOVE:
			res = RemDosEntry(&vol->dol);
			break;
		case FBX_ASYNC_REMOVE_FREE:
			if ((res = RemDosEntry(&vol->dol)))
				FreeFbxVolume(vol);
			break;
		case FBX_ASYNC_RENAME:
			if ((res = RemDosEntry(&vol->dol))) {
				FbxStrlcpy(fs, vol->volname, name, CONN_VOLUME_NAME_BYTES);
				vol->volnamelen = strlen(vol->volname);
				res = AddDosEntry(&vol->dol);
			}
			break;
		default:
			res = FALSE;
			break;
		}
		UnLockDosList(LDF_ALL|LDF_WRITE);
	} else {
		struct FbxAsyncMsg *msg;
		int len;

		len = sizeof(struct FbxAsyncMsg);
		if (name != NULL) len += strlen(name) + 1;

		for (i = 0; i < 10; i++) {
			msg = AllocMem(len, MEMF_PUBLIC|MEMF_CLEAR);
			if (msg != NULL)
				break;
			Delay(10);
		}
		if (msg != NULL) {
			msg->msg.mn_Node.ln_Type = NT_MESSAGE;
			msg->msg.mn_Length = len;
			msg->fs = fs;
			msg->vol = vol;
			msg->cmd = cmd;
			if (name != NULL) strcpy(msg->name, name);
			PutMsg(fs->dlproc_port, &msg->msg);
			res = TRUE;
		} else {
			res = FALSE;
		}
	}

	return res;
}

int FbxAsyncAddVolume(struct FbxFS *fs, struct FbxVolume *vol) {
	return FbxAsyncDosListCmd(fs, vol, FBX_ASYNC_ADD, NULL);
}

int FbxAsyncRemVolume(struct FbxFS *fs, struct FbxVolume *vol) {
	return FbxAsyncDosListCmd(fs, vol, FBX_ASYNC_REMOVE, NULL);
}

int FbxAsyncRemFreeVolume(struct FbxFS *fs, struct FbxVolume *vol) {
	return FbxAsyncDosListCmd(fs, vol, FBX_ASYNC_REMOVE_FREE, NULL);
}

int FbxAsyncRenameVolume(struct FbxFS *fs, struct FbxVolume *vol, const char *name) {
	return FbxAsyncDosListCmd(fs, vol, FBX_ASYNC_RENAME, name);
}

