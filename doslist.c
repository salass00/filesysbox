/*
 * Copyright (c) 2013-2018 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <dos/dostags.h>
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
#ifndef __AROS__
	struct Message *msg;
#endif
	struct FbxAsyncMsg *async_msg;

	proc = (struct Process *)FindTask(NULL);
	port = &proc->pr_MsgPort;

#ifdef __AROS__
	libBase = proc->pr_Task.tc_UserData;
#else
	WaitPort(port);
	msg = GetMsg(port);
	libBase = (struct FileSysBoxBase *)msg->mn_Node.ln_Name;
#endif
	DOSBase = libBase->dosbase;

	for (;;) {
		Wait(SIGBREAKF_CTRL_C | (1UL << port->mp_SigBit));

		if (!IsListEmpty(&port->mp_MsgList)) {
			LockDosList(LDF_ALL|LDF_WRITE);
			while ((async_msg = (struct FbxAsyncMsg *)GetMsg(port)) != NULL) {
				struct FbxVolume *vol = async_msg->vol;
				int cmd = async_msg->cmd;
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
						FbxStrlcpy(async_msg->fs, vol->volname, async_msg->name, CONN_VOLUME_NAME_BYTES);
						vol->volnamelen = strlen(vol->volname);
						AddDosEntry(&vol->dol);
					}
					break;
				}
				FreeMem(async_msg, async_msg->msg.mn_Length);
			}
			UnLockDosList(LDF_ALL|LDF_WRITE);
		}

		if (libBase->dlproc_refcount == 0) {
			ObtainSemaphore(&libBase->dlproc_sem);
			if (libBase->dlproc_refcount == 0)
				break;
			ReleaseSemaphore(&libBase->dlproc_sem);
		}
	}

	while ((async_msg = (struct FbxAsyncMsg *)GetMsg(port)) != NULL)
		FreeMem(async_msg, async_msg->msg.mn_Length);

	Forbid();
	libBase->dlproc = NULL;
	ReleaseSemaphore(&libBase->dlproc_sem);
	return RETURN_OK;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

struct Process *StartDosListProc(struct FileSysBoxBase *libBase) {
	struct Library *DOSBase = libBase->dosbase;
	struct Process *dlproc;

	dlproc = CreateNewProcTags(
		NP_Entry,       (IPTR)FbxDosListProc,
#ifdef __AROS__
		NP_UserData,    (IPTR)libBase,
#endif
		NP_StackSize,   4096,
		NP_Name,        (IPTR)"FileSysBox DosList handler",
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

#ifndef __AROS__
	if (dlproc != NULL) {
		struct Library *SysBase = libBase->sysbase;
		static struct Message msg;

		bzero(&msg, sizeof(msg));
		msg.mn_Node.ln_Type = NT_MESSAGE;
		msg.mn_Node.ln_Name = (STRPTR)libBase;
		msg.mn_Length = sizeof(msg);
		PutMsg(&dlproc->pr_MsgPort, &msg);
	}
#endif

	return dlproc;
}

static int FbxAsyncDosListCmd(struct FbxFS *fs, struct FbxVolume *vol, int cmd, const char *name) {
	struct Library *SysBase = fs->sysbase;
	struct Library *DOSBase = fs->dosbase;
	struct DosList *dl = NULL;
	int i, res;

	if (vol == NULL)
		return FALSE;

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

