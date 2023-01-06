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
#include "fuse_stubs.h"
#include <errno.h>

int FbxAddNotify(struct FbxFS *fs, struct NotifyRequest *notify) {
	struct Library *SysBase = fs->sysbase;
	struct fbx_stat statbuf;
	struct FbxEntry *e;
	struct FbxNotifyNode *nn;
	LONG etype;
	int error;
	const char *fullname;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsfullname[FBX_MAX_PATH];
#endif

	PDEBUGF("FbxAddNotify(%#p, %#p)\n", fs, notify);

	CHECKVOLUME(DOSFALSE);

	notify->nr_notifynode = (IPTR)NULL;
	notify->nr_MsgCount = 0;

	fullname = (const char *)notify->nr_FullName;
#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (local_to_utf8(fsfullname, fullname, FBX_MAX_NAME, fs->maptable) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		fullname = fsfullname;
	}
#endif

	if (!FbxLockName2Path(fs, NULL, fullname, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	error = Fbx_getattr(fs, fullpath, &statbuf);
	if (error) {
		if (fs->r2 == -ENOENT) { // file did not exist
			NDEBUGF("FbxAddNotify: file '%s' did not exist.\n", fullpath);

			nn = AllocFbxNotifyNode();
			if (nn == NULL) {
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}

			nn->nr = notify;
			nn->entry = NULL;

			notify->nr_notifynode = (IPTR)nn;
			// lets put request on the unresolved list
			AddTail((struct List *)&fs->currvol->unres_notifys, (struct Node *)&nn->chain);
		} else {
			NDEBUGF("FbxAddNotify: getattr() error %d.\n", error);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	} else { // file existed
		NDEBUGF("FbxAddNotify: file '%s' existed.\n", fullpath);

		if (S_ISREG(statbuf.st_mode)) {
			etype = ETYPE_FILE;
		} else {
			etype = ETYPE_DIR;
		}

		e = FbxFindEntry(fs, fullpath);
		if (e == NULL) {
			e = FbxSetupEntry(fs, fullpath, etype, statbuf.st_ino);
			if (e == NULL) return DOSFALSE;
		}

		nn = AllocFbxNotifyNode();
		if (nn == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		nn->nr = notify;
		nn->entry = e;

		notify->nr_notifynode = (IPTR)nn;
		AddTail((struct List *)&e->notifylist, (struct Node *)&nn->chain);

		if (notify->nr_Flags & NRF_NOTIFY_INITIAL) {
			FbxDoNotifyRequest(fs, notify);
		}
	}

	notify->nr_Handler = fs->fsport;

	AddTail((struct List *)&fs->currvol->notifylist, (struct Node *)&nn->volumechain);

	fs->r2 = 0;
	return DOSTRUE;
}

