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
#include <string.h>
#include <errno.h>

static int Fbx_rename(struct FbxFS *fs, const char *path, const char *path2)
{
	ODEBUGF("Fbx_rename(%#p, '%s', '%s')\n", fs, path, path2);

	return FSOP rename(path, path2, &fs->fcntx);
}

static void FbxUpdatePaths(struct FbxFS *fs, const char *oldpath, const char *newpath) {
	struct Library *SysBase = fs->sysbase;
	char tstr[FBX_MAX_PATH];
	struct MinNode *chain, *succ;

	// TODO: unresolve+tryresolve notify for affected entries..

	// lets rename all subentries. subentries can be
	// found by comparing common path. do this by traversing
	// global hashtable and compare entry->path
	int plen = FbxStrlen(fs, oldpath);
	int a;
	if (strcmp(oldpath, "/") == 0) plen = 0;
	for (a = 0; a < ENTRYHASHSIZE; a++) {
		chain = fs->currvol->entrytab[a].mlh_Head;
		while ((succ = chain->mln_Succ) != NULL) {
			struct FbxEntry *e = FSENTRYFROMHASHCHAIN(chain);
			if (FbxStrncmp(fs, oldpath, e->path, plen) == 0 && e->path[plen] == '/') {
				// match! let's update path
				FbxStrlcpy(fs, tstr, newpath, FBX_MAX_PATH);
				FbxStrlcat(fs, tstr, e->path + plen, FBX_MAX_PATH);
				FbxSetEntryPath(fs, e, tstr);
				// and rehash it
				Remove((struct Node *)&e->hashchain);
				FbxAddEntry(fs, e);
			}
			chain = succ;
		}
	}
}

int FbxRenameObject(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	struct FbxLock *lock2, const char *name2)
{
	struct Library *SysBase = fs->sysbase;
	struct FbxEntry *e, *e2;
	struct fbx_stat statbuf;
	int error;
	char fullpath[FBX_MAX_PATH];
	char fullpath2[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
	char fsname2[FBX_MAX_NAME];
#endif

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}
	if (lock2 != NULL) {
		CHECKLOCK(lock2, DOSFALSE);

		if (lock2->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		name = fsname;
		if (FbxLocalToUTF8(fs, fsname2, name2, FBX_MAX_NAME) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
		name2 = fsname2;
	}
#else
	CHECKSTRING(name, DOSFALSE);
	CHECKSTRING(name2, DOSFALSE);
#endif

	if (!FbxLockName2Path(fs, lock, name, fullpath) ||
		!FbxLockName2Path(fs, lock2, name2, fullpath2))
	{
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (strcmp(fullpath, "/") == 0) {
		// can't rename root
		fs->r2 = ERROR_OBJECT_WRONG_TYPE;
		return DOSFALSE;
	}

	if (FbxIsParent(fs, fullpath, fullpath2)) {
		fs->r2 = ERROR_OBJECT_IN_USE;
		return DOSFALSE;
	}

	/* Make sure that the object to be renamed exists */
	e = FbxFindEntry(fs, fullpath);
	if (e == NULL)
	{
		error = Fbx_getattr(fs, fullpath, &statbuf);
		if (error)
		{
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	/* Check if source and destination are the same object */
	if (FbxStrcmp(fs, fullpath, fullpath2) == 0)
	{
		if ((fs->currvol->vflags & FBXVF_CASE_SENSITIVE) || strcmp(fullpath, fullpath2) == 0)
		{
			/* Nothing to do here */
			fs->r2 = 0;
			return DOSTRUE;
		}
	}
	else
	{
		/* Check if destination already exists */
		e2 = FbxFindEntry(fs, fullpath2);
		if (e2 != NULL) {
			fs->r2 = ERROR_OBJECT_EXISTS;
			return DOSFALSE;
		}
		error = Fbx_getattr(fs, fullpath2, &statbuf);
		if (error == 0) {
			fs->r2 = ERROR_OBJECT_EXISTS;
			return DOSFALSE;
		} else if (error != -ENOENT) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	error = Fbx_rename(fs, fullpath, fullpath2);
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	//e = FbxFindEntry(fs, fullpath); /* Already done in code above */
	if (e != NULL) {
		FbxUnResolveNotifys(fs, e);
		FbxSetEntryPath(fs, e, fullpath2);
		Remove((struct Node *)&e->hashchain);
		FbxAddEntry(fs, e);
		FbxTryResolveNotify(fs, e);
	}

	FbxDoNotify(fs, fullpath2);

	FbxUpdatePaths(fs, fullpath, fullpath2);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

