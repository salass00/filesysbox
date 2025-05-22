/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <string.h>

static int Fbx_opendir(struct FbxFS *fs, const char *path, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_opendir(%#p, '%s', %#p)\n", fs, path, fi);

	return FSOP opendir(path, fi, &fs->fcntx);
}

static int Fbx_releasedir(struct FbxFS *fs, const char *path, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_releasedir(%#p, '%s', %#p)\n", fs, path, fi);

	return FSOP releasedir(path, fi, &fs->fcntx);
}

static int Fbx_readdir(struct FbxFS *fs, const char *path, APTR udata, fuse_fill_dir_t func,
	QUAD offset, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_readdir(%#p, '%s', %#p, %#p, %lld, %#p)\n", fs, path, udata, func, offset, fi);

	return FSOP readdir(path, udata, func, offset, fi, &fs->fcntx);
}

static STDARGS int dir_fill_func(void *udata, const char *name, const struct fbx_stat *stat, fbx_off_t offset) {
	struct FbxLock *lock = udata;
	struct FbxFS *fs = lock->fs;
	struct Library *SysBase = fs->sysbase;
	struct FbxDirData *ed;
	size_t len;

	if (name == NULL) return 2;

	if (!IsDotOrDotDot(name)) {
		if (FbxCheckString(fs, name)) {
			len = strlen(name) + 1;
			ed = AllocFbxDirData(lock, len);
			if (ed == NULL) return 1;

			ed->fsname = (char *)(ed + 1);

			/* Only used by ExAll() */
			ed->name    = NULL;
			ed->comment = NULL;

			FbxStrlcpy(fs, ed->fsname, name, len);
			if (stat != NULL)
			{
				ed->stat = *stat;
			}
			else
			{
				ed->stat.st_ino = 0;
			}

			AddTail((struct List *)&lock->dirdatalist, (struct Node *)ed);
		}
	}

	return 0;
}

int FbxReadDir(struct FbxFS *fs, struct FbxLock *lock) {
	int error;

	if (FSOP opendir != FSOP open) {
		struct Library *SysBase = fs->sysbase;
		struct fuse_file_info *fi;

		fi = AllocFuseFileInfo(fs);
		if (fi == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}
		bzero(fi, sizeof(*fi));

		error = Fbx_opendir(fs, lock->entry->path, fi);
		if (error) {
			FreeFuseFileInfo(fs, fi);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}

		error = Fbx_readdir(fs, lock->entry->path, lock, dir_fill_func, 0, fi);
		if (error) {
			Fbx_releasedir(fs, lock->entry->path, fi);
			FreeFuseFileInfo(fs, fi);
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}

		Fbx_releasedir(fs, lock->entry->path, fi);
		FreeFuseFileInfo(fs, fi);
	} else {
		error = Fbx_readdir(fs, lock->entry->path, lock, dir_fill_func, 0, NULL);
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	return DOSTRUE;
}

int FbxExamineNext(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib) {
	struct Library *SysBase = fs->sysbase;
	struct FbxDirData *ed;
	struct fbx_stat statbuf;
	int error;
	char fullpath[FBX_MAX_PATH];

	PDEBUGF("FbxExamineNext(%#p, %#p, %#p)\n", fs, lock, fib);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	if (!lock->dirscan) {
		if (lock->mempool == NULL) {
			lock->mempool = CreatePool(MEMF_PUBLIC, 4096, 1024);
			if (lock->mempool == NULL) {
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}
		}

		if (!FbxReadDir(fs, lock)) {
			FreeFbxDirDataList(lock, &lock->dirdatalist);
			return DOSFALSE;
		}
		lock->dirscan = TRUE;
	}

	ed = (struct FbxDirData *)RemHead((struct List *)&lock->dirdatalist);
	if (ed == NULL) {
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	}

	if (!FbxLockName2Path(fs, lock, ed->fsname, fullpath)) {
		FreeFbxDirData(lock, ed);
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	if (fs->fsflags & FBXF_USE_FILL_DIR_STAT) {
		statbuf = ed->stat;
		FreeFbxDirData(lock, ed);
	} else {
		FreeFbxDirData(lock, ed);
		error = Fbx_getattr(fs, fullpath, &statbuf);
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return DOSFALSE;
		}
	}

	FbxPathStat2FIB(fs, fullpath, &statbuf, fib);

	fs->r2 = 0;
	return DOSTRUE;
}

