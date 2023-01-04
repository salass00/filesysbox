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
#include <devices/inputevent.h>
#include <string.h>

static APTR Fbx_init(struct FbxFS *fs, struct fuse_conn_info *conn)
{
	ODEBUGF("Fbx_init(%#p, %#p)\n", fs, conn);

	if (FSOP init)
	{
		fs->initret = FSOP init(conn, &fs->fcntx);
		return fs->initret;
	}
	else
	{
		FbxStrlcpy(fs, conn->volume_name, BADDR(fs->devnode->dn_Name) + 1, CONN_VOLUME_NAME_BYTES);
		return (APTR)TRUE;
	}
}

static void Fbx_destroy(struct FbxFS *fs, APTR x)
{
	ODEBUGF("Fbx_destroy(%#p, %#p)\n", fs, x);

	if (FSOP destroy) FSOP destroy(x, &fs->fcntx);
}

struct FbxVolume *FbxSetupVolume(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
#ifndef NODEBUG
	struct Library *DOSBase = fs->dosbase;
#endif
	struct fuse_conn_info *conn = &fs->conn;
	APTR initret;
	int error, i, rc;
	struct FbxVolume *vol;
	struct statvfs st;
	struct fbx_stat statbuf;

	DEBUGF("FbxSetupVolume(%#p)\n", fs);

	if (OKVOLUME(fs->currvol)) {
		return fs->currvol;
	}

	if (fs->inhibit) {
		return fs->currvol = NULL;
	}

	bzero(conn, sizeof(*conn));

	initret = Fbx_init(fs, conn);
	if (initret == (APTR)-2) {
		// error setting up resources for accessing volume
		debugf("fbx fs failed to open resources for volume\n");
		return fs->currvol = (APTR)-2;
	} else if (initret == (APTR)-1) {
		// broken on-disk layout (or not formatted)
		return fs->currvol = (APTR)-1;
	} else if (initret == NULL) {
		// no disk in drive
		return fs->currvol = NULL;
	}

	DEBUGF("FbxSetupVolume: conn.volume_name '%s'\n", conn->volume_name);

	// if statfs fails, abort.
	error = Fbx_statfs(fs, "/", &st);
	if (error) {
		DEBUGF("FbxSetupVolume: statfs failed (not formatted ?) err %d\n", error);
		Fbx_destroy(fs, fs->initret);
		return FALSE;
	}

	error = Fbx_getattr(fs, "/", &statbuf);
	if (error) {
		DEBUGF("FbxSetupVolume: stat on root directory failed err %d\n", error);
		Fbx_destroy(fs, fs->initret);
		return FALSE;
	}

	vol = AllocFbxVolume();
	if (vol == NULL) {
		Fbx_destroy(fs, fs->initret);
		return NULL;
	}

	vol->dl.dl_Type     = DLT_VOLUME;
	vol->dl.dl_Task     = fs->fsport;
	vol->dl.dl_DiskType = fs->dostype;
#if defined(__AROS__) && defined(AROS_FAST_BSTR)
	vol->dl.dl_Name     = (STRPTR)vol->volname;
#else
	vol->dl.dl_Name     = MKBADDR(&vol->volnamelen);
#endif

	FbxTimeSpec2DS(fs, &statbuf.st_ctim, &vol->dl.dl_VolumeDate);
	FbxStrlcpy(fs, vol->volname, conn->volume_name, CONN_VOLUME_NAME_BYTES);
	vol->volnamelen = strlen(vol->volname);

	vol->fs           = fs;
	vol->writeprotect = FALSE;
	vol->vflags       = 0;

	NEWLIST(&vol->unres_notifys);
	NEWLIST(&vol->locklist);
	NEWLIST(&vol->notifylist);
	for (i = 0; i < ENTRYHASHSIZE; i++) {
		NEWLIST(&vol->entrytab[i]);
	}

	if (st.f_flag & ST_CASE_SENSITIVE) {
		vol->vflags |= FBXVF_CASE_SENSITIVE;
	}

	if (st.f_flag & ST_RDONLY) {
		vol->vflags |= FBXVF_READ_ONLY;
	}

	// add volume to doslist
	rc = FbxAsyncAddVolume(fs, vol);
	if (!rc) {
		DEBUGF("FbxSetupVolume: NBM_ADDDOSENTRY failed (name collision ?) err %d\n", (int)IoErr());
		Fbx_destroy(fs, fs->initret);
		FreeFbxVolume(vol);
		return NULL;
	}

	fs->currvol = vol;

	// tell input.device there was a change
	FbxNotifyDiskChange(fs, IECLASS_DISKINSERTED);

	DEBUGF("FbxSetupVolume: Volume %#p set up OK.\n", vol);

	return vol;
}

void FbxCleanupVolume(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	struct FbxVolume *vol = fs->currvol;

	// do nothing if we don't have a volume
	if (NOVOLUME(vol)) {
		fs->currvol = NULL;
		return;
	}

	// only notify about disk removal if we have a bad volume.
	if (BADVOLUME(vol)) {
		fs->currvol = NULL;
		// notify about disk removal
		FbxNotifyDiskChange(fs, IECLASS_DISKREMOVED);
		return;
	}

	struct MinNode *chain, *succ;
	chain = vol->locklist.mlh_Head;
	while ((succ = chain->mln_Succ) != NULL) {
		struct FbxLock *lock = FSLOCKFROMVOLUMECHAIN(chain);

		if (lock->info != NULL) {
			struct FbxEntry *e = lock->entry;
			Fbx_release(fs, e->path, lock->info);
			FreeFuseFileInfo(fs, lock->info);
			lock->info = NULL;
		}

		chain = succ;
	}

	// Make sure that dirty data is on disk before destroying
	FbxFlushAll(fs);

	Fbx_destroy(fs, fs->initret);

	if (IsMinListEmpty(&vol->locklist) &&
		IsMinListEmpty(&vol->notifylist) &&
		IsMinListEmpty(&vol->unres_notifys))
	{
		// Remove and free if no locks are pending
		FbxAsyncRemFreeVolume(fs, vol);
	} else {
		// Remove and add to list if there are locks pending
		FbxAsyncRemVolume(fs, vol);
		AddTail((struct List *)&fs->volumelist, (struct Node *)&vol->fschain);
	}

	fs->currvol = NULL;

	// notify about disk removal
	FbxNotifyDiskChange(fs, IECLASS_DISKREMOVED);

	// reset update timeouts
	FbxSetModifyState(fs, 0);
}

