/*
 * Copyright (c) 2013-2025 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include <dos/dostags.h>
#include <string.h>

#define ACTION_COLLECT 3001

enum {
	ID_COLLECT_LOCK = 1,
	/* ID_COLLECT_FH, */
	ID_COLLECT_NOTIFYNODE
};

#ifdef __AROS__
static AROS_UFH3(int, FbxLockHandlerProc,
	AROS_UFHA(STRPTR, argstr, A0),
	AROS_UFHA(ULONG, arglen, D0),
	AROS_UFHA(struct Library *, SysBase, A6)
)
{
	AROS_USERFUNC_INIT
#else
static int FbxLockHandlerProc(void) {
	struct Library *SysBase = *(struct Library **)4;
#endif
	struct FileSysBoxBase *libBase;
	struct Library *DOSBase;
	struct Process *proc;
	struct MsgPort *port;
	struct Message *msg;
	struct DosPacket *pkt;
	LONG type;
	SIPTR r1, r2;
	struct MinList locklist;
	/* struct MinList fhlist; */
	struct MinList notifylist;

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

	NEWMINLIST(&locklist);
	/* NEWMINLIST(&fhlist); */
	NEWMINLIST(&notifylist);

	for (;;) {
		Wait(SIGBREAKF_CTRL_C | (1UL << port->mp_SigBit));

		while ((msg = GetMsg(port)) != NULL) {
			pkt = (struct DosPacket *)msg->mn_Node.ln_Name;
			type = pkt->dp_Type;
			switch (type) {
				case ACTION_IS_FILESYSTEM:
					r1 = DOSTRUE;
					r2 = 0;
					break;

				case ACTION_COLLECT:
				{
					int type;
					APTR item;
					struct MinList *list = NULL;
					struct Node *node;

					type = pkt->dp_Arg1;
					item = (APTR)pkt->dp_Arg2;

					switch (type) {
						case ID_COLLECT_LOCK:
							list = &locklist;
							break;
						/* case ID_COLLECT_FH:
							list = &fhlist;
							break; */
						case ID_COLLECT_NOTIFYNODE:
							list = &notifylist;
							break;
					}

					if (list == NULL) {
						r1 = DOSFALSE;
						r2 = ERROR_BAD_NUMBER;
						break;
					}

					node = AllocMem(sizeof(*node), MEMF_ANY);
					if (node == NULL) {
						r1 = DOSFALSE;
						r2 = ERROR_NO_FREE_STORE;
						break;
					}

					node->ln_Type = type;
					node->ln_Name = item;
					AddHead((struct List *)list, node);

					r1 = DOSTRUE;
					r2 = 0;
					break;
				}

				case ACTION_END:
				case ACTION_FREE_LOCK:
				{
					struct FbxLock *lock;
					struct Node *node;

					lock = (struct FbxLock *)BADDR(pkt->dp_Arg1);
					{
						BOOL found = FALSE;
						for (node = (struct Node *)locklist.mlh_Head; node->ln_Succ != NULL; node = node->ln_Succ) {
							if ((struct FbxLock *)node->ln_Name == lock) {
								Remove(node);
								FreeMem(node, sizeof(*node));

								if (lock->mempool != NULL) {
									DeletePool(lock->mempool);
								}
								FreeFbxLock(lock);

								found = TRUE;
								break;
							}
						}

						if (!found) {
							r1 = DOSFALSE;
							r2 = ERROR_INVALID_LOCK;
							break;
						}
					}

					r1 = DOSTRUE;
					r2 = 0;
					break;
				}

				case ACTION_EXAMINE_ALL_END:
				{
					struct FbxLock *lock;
					struct ExAllControl *ctrl;
					struct FbxExAllState *exallstate;

					lock = (struct FbxLock *)BADDR(pkt->dp_Arg1);
					ctrl = (struct ExAllControl *)pkt->dp_Arg5;

					if (ctrl != NULL) {
						exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
						if (exallstate) {
							if (exallstate != (APTR)-1) {
								FreeFbxDirDataList(lock, &exallstate->freelist);
								FreeFbxExAllState(lock, exallstate);
							}
							ctrl->eac_LastKey = (IPTR)NULL;
						}
					}

					if (lock != NULL) FreeFbxDirDataList(lock, &lock->dirdatalist);

					r1 = DOSTRUE;
					r2 = 0;
					break;
				}

				case ACTION_REMOVE_NOTIFY:
				{
					struct NotifyRequest *nr;
					struct FbxNotifyNode *nn;
					struct Node *node;

					nr = (struct NotifyRequest *)pkt->dp_Arg1;

					nn = (struct FbxNotifyNode *)nr->nr_notifynode;
					{
						BOOL found = FALSE;
						for (node = (struct Node *)notifylist.mlh_Head; node->ln_Succ != NULL; node = node->ln_Succ) {
							if ((struct FbxNotifyNode *)node->ln_Name == nn) {
								Remove(node);
								FreeMem(node, sizeof(*node));

								FreeFbxNotifyNode(nn);

								nr->nr_MsgCount = 0;
								nr->nr_notifynode = (IPTR)NULL;

								found = TRUE;
								break;
							}
						}

						if (!found) {
							r1 = DOSFALSE;
							r2 = 0;
							break;
						}
					}

					r1 = DOSTRUE;
					r2 = 0;
					break;
				}

				case ACTION_DISK_INFO:
				{
					struct InfoData *id;

					id = (struct InfoData *)BADDR(pkt->dp_Arg2);

					bzero(id, sizeof(*id));

					id->id_UnitNumber    = -1;
					id->id_DiskState     = ID_VALIDATING;
					id->id_BytesPerBlock = 512;
					id->id_DiskType      = ID_NO_DISK_PRESENT;

					r1 = DOSTRUE;
					r2 = 0;
					break;
				}

				case ACTION_READ:
				case ACTION_WRITE:
				case ACTION_SEEK:
				case ACTION_SET_FILE_SIZE:
					r1 = -1;
					r2 = ERROR_NO_DISK;
					break;

				case ACTION_LOCATE_OBJECT:
				case ACTION_COPY_DIR:
				case ACTION_COPY_DIR_FH:
				case ACTION_CREATE_DIR:
				case ACTION_PARENT:
				case ACTION_PARENT_FH:
				case ACTION_CURRENT_VOLUME:
					r1 = (SIPTR)ZERO;
					r2 = ERROR_NO_DISK;
					break;

				case ACTION_FINDUPDATE:
				case ACTION_FINDINPUT:
				case ACTION_FINDOUTPUT:
				case ACTION_EXAMINE_OBJECT:
				case ACTION_EXAMINE_FH:
				case ACTION_EXAMINE_NEXT:
				case ACTION_DELETE_OBJECT:
				case ACTION_RENAME_OBJECT:
				case ACTION_SET_PROTECT:
				case ACTION_SET_COMMENT:
				case ACTION_SET_DATE:
				case ACTION_FH_FROM_LOCK:
				case ACTION_SAME_LOCK:
				case ACTION_MAKE_LINK:
				case ACTION_READ_LINK:
				case ACTION_CHANGE_MODE:
				case ACTION_EXAMINE_ALL:
				case ACTION_ADD_NOTIFY:
				case ACTION_INFO:
				case ACTION_FORMAT:
				case ACTION_RENAME_DISK:
				case ACTION_SET_OWNER:
					r1 = DOSFALSE;
					r2 = ERROR_NO_DISK;
					break;

				default:
					r1 = DOSFALSE;
					r2 = ERROR_ACTION_NOT_KNOWN;
					break;
			}
			ReplyPkt(pkt, r1, r2);
		}

		if (libBase->lhproc_refcount == 0 &&
			IsMinListEmpty(&locklist) &&
			/* IsMinListEmpty(&fhlist) && */
			IsMinListEmpty(&notifylist))
		{
			ObtainSemaphore(&libBase->procsema);
			if (libBase->lhproc_refcount == 0)
				break;
			ReleaseSemaphore(&libBase->procsema);
		}
	}

	Forbid();
	libBase->lhproc = NULL;
	ReleaseSemaphore(&libBase->procsema);
	return RETURN_OK;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

struct Process *StartLockHandlerProc(struct FileSysBoxBase *libBase) {
	struct Library *DOSBase = libBase->dosbase;
	struct Process *lhproc;

	lhproc = CreateNewProcTags(
		NP_Entry,       (IPTR)FbxLockHandlerProc,
#ifdef __AROS__
		NP_UserData,    (IPTR)libBase,
#endif
		NP_StackSize,   4096,
		NP_Name,        (IPTR)"FileSysBox lock handler",
		NP_Priority,    5,
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
	if (lhproc != NULL) {
		struct Library *SysBase = libBase->sysbase;
		static struct Message msg;

		bzero(&msg, sizeof(msg));
		msg.mn_Node.ln_Type = NT_MESSAGE;
		msg.mn_Node.ln_Name = (char *)libBase;
		msg.mn_Length = sizeof(msg);
		PutMsg(&lhproc->pr_MsgPort, &msg);
	}
#endif

	return lhproc;
}

void FbxCollectLock(struct FbxFS *fs, struct FbxLock *lock) {
	struct Library *DOSBase = fs->dosbase;
	struct MsgPort *port = fs->lhproc_port;
	struct FileHandle *fh;

	lock->taskmp = port;
	if ((fh = lock->fh) != NULL) {
		fh->fh_Type = port;
	}

	DoPkt(port, ACTION_COLLECT, ID_COLLECT_LOCK, (SIPTR)lock, 0, 0, 0);
}

/* void FbxCollectFH(struct FbxFS *fs, struct FileHandle *fh) {
	struct Library *DOSBase = fs->dosbase;
	struct MsgPort *port = fs->lhproc_port;

	fh->fh_Type = port;

	DoPkt(port, ACTION_COLLECT, ID_COLLECT_FH, (SIPTR)fh, 0, 0, 0);
} */

void FbxCollectNotifyNode(struct FbxFS *fs, struct FbxNotifyNode *nn) {
	struct Library *DOSBase = fs->dosbase;
	struct MsgPort *port = fs->lhproc_port;
	struct NotifyRequest *nr;

	if ((nr = nn->nr) != NULL) {
		nr->nr_Handler = port;
	}

	DoPkt(port, ACTION_COLLECT, ID_COLLECT_NOTIFYNODE, (SIPTR)nn, 0, 0, 0);
}

