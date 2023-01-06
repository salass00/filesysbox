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

void FbxDoNotifyRequest(struct FbxFS *fs, struct NotifyRequest *nr) {
	struct Library *SysBase = fs->sysbase;
	struct NotifyMessage *notifymsg;

	NDEBUGF("FbxDoNotifyRequest(%#p, %#p)\n", fs, nr);

	if (nr->nr_Flags & NRF_SEND_MESSAGE) {
		if ((nr->nr_MsgCount > 0) && (nr->nr_Flags & NRF_WAIT_REPLY)) {
			nr->nr_Flags |= NRF_MAGIC;
			NDEBUGF("DoNotifyRequest: did magic! nreq %#p '%s'\n", nr, nr->nr_FullName);
		} else {
			notifymsg = AllocNotifyMessage();
			if (!notifymsg) return; // lets abort.
			nr->nr_MsgCount++;
			notifymsg->nm_ExecMessage.mn_Length = sizeof(*notifymsg);
			notifymsg->nm_ExecMessage.mn_ReplyPort = fs->notifyreplyport;
			notifymsg->nm_Class = NOTIFY_CLASS;
			notifymsg->nm_Code = NOTIFY_CODE;
			notifymsg->nm_NReq = nr;
			PutMsg(nr->nr_stuff.nr_Msg.nr_Port, (struct Message *)notifymsg);
			NDEBUGF("DoNotifyRequest: sent message to port of nreq %#p '%s'\n", nr, nr->nr_FullName);
		}
	} else if (nr->nr_Flags & NRF_SEND_SIGNAL) {
		Signal(nr->nr_stuff.nr_Signal.nr_Task, 1 << nr->nr_stuff.nr_Signal.nr_SignalNum);
		NDEBUGF("DoNotifyRequest: signaled task of nreq %#p '%s'\n", nr, nr->nr_FullName);
	}
}

void FbxDoNotifyEntry(struct FbxFS *fs, struct FbxEntry *entry) {
	struct MinNode *nnchain;
	struct NotifyRequest *nr;
	struct FbxNotifyNode *nn;

	NDEBUGF("FbxDoNotifyEntry(%#p, %#p)\n", fs, entry);

	nnchain = entry->notifylist.mlh_Head;
	while (nnchain->mln_Succ) {
		nn = FSNOTIFYNODEFROMCHAIN(nnchain);
		nr = nn->nr;
		FbxDoNotifyRequest(fs, nr);
		nnchain = nnchain->mln_Succ;
	}
}

void FbxDoNotify(struct FbxFS *fs, const char *path) {
	struct FbxEntry *e;
	char *pathbuf = fs->pathbuf[2];

	NDEBUGF("FbxDoNotify(%#p, '%s')\n", fs, path);

	FbxStrlcpy(fs, pathbuf, path, MAXPATHLEN);
	do {
		e = FbxFindEntry(fs, pathbuf);
		if (e != NULL) FbxDoNotifyEntry(fs, e);
		// parent dirs wants notify too
	} while (FbxParentPath(fs, pathbuf));
}

void FbxTryResolveNotify(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	struct FbxNotifyNode *nn;
	struct MinNode *chain, *succ;
	struct NotifyRequest *nr;
	char *fullpath = fs->pathbuf[2];

	NDEBUGF("FbxTryResolveNotify(%#p, %#p)\n", fs, e);

	chain = fs->currvol->unres_notifys.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		nn = FSNOTIFYNODEFROMCHAIN(chain);
		nr = nn->nr;
		if (FbxLockName2Path(fs, NULL, (const char *)nr->nr_FullName, fullpath) &&
			FbxStrcmp(fs, fullpath, e->path) == 0)
		{
			Remove((struct Node *)chain);
			AddTail((struct List *)&e->notifylist, (struct Node *)chain);
			nn->entry = e;
			PDEBUGF("try_resolve_nreqs: resolved nreq %#p, '%s'\n", nr, nr->nr_FullName);
		}
		chain = succ;
	}
}

void FbxUnResolveNotifys(struct FbxFS *fs, struct FbxEntry *e) {
	struct Library *SysBase = fs->sysbase;
	struct MinNode *chain, *succ;
	struct FbxNotifyNode *nn;

	NDEBUGF("unresolve_notifys(%#p, %#p)\n", fs, e);

	// move possible notifys onto unresolved list
	chain = e->notifylist.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		Remove((struct Node *)chain);
		AddTail((struct List *)&fs->currvol->unres_notifys, (struct Node *)chain);
		nn = FSNOTIFYNODEFROMCHAIN(chain);
		NDEBUGF("unresolve_notifys: removed nn %#p\n", nn);
		nn->entry = NULL; // clear
		chain = succ;
	}

	NDEBUGF("unresolve_notifys: DONE\n");
}

