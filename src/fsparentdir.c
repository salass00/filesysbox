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
#include <string.h>

BOOL FbxParentPath(struct FbxFS *fs, char *pathbuf) {
	if (IsRoot(pathbuf)) {
		// can't parent root
		return FALSE;
	}
	char *p = strrchr(pathbuf, '/');
	if (p != NULL) {
		if (p == pathbuf) p++; // leave the root '/' alone
		*p = '\0';
		return TRUE;
	}
	// should never happen
	return FALSE;
}

struct FbxLock *FbxLocateParent(struct FbxFS *fs, struct FbxLock *lock) {
	char pname[FBX_MAX_PATH];
	const char *name;

	PDEBUGF("FbxLocateParent(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(NULL);

	if (lock == NULL) {
		fs->r2 = 0; // yes
		return NULL;
	}

	CHECKLOCK(lock, NULL);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return NULL;
	}

	FbxStrlcpy(fs, pname, lock->entry->path, FBX_MAX_PATH);
	if (!FbxParentPath(fs, pname)) {
		// can't parent root
		fs->r2 = 0; // yes
		return NULL;
	}

	name = pname;
	if (*name == '/') name++; // skip preceding slash character
	lock = FbxLocateObject(fs, NULL, name, SHARED_LOCK);
	if (lock == NULL) return NULL;

	fs->r2 = 0;
	return lock;
}

