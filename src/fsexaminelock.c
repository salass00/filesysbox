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

void FbxGetComment(struct FbxFS *fs, const char *fullpath, char *comment, size_t size) {
	int res;

	res = Fbx_getxattr(fs, fullpath, fs->xattr_amiga_comment, comment, size-1);
	if (res < 0) {
		comment[0] = '\0';
		return;
	}

	comment[min(res, size-1)] = '\0';
	if (!FbxCheckString(fs, comment)) {
		comment[0] = '\0';
		return;
	}
}

LONG FbxMode2EntryType(const mode_t mode) {
	if (S_ISDIR(mode)) {
		return ST_USERDIR;
	} else if (S_ISREG(mode)) {
		return ST_FILE;
	} else if (S_ISLNK(mode)) {
		return ST_SOFTLINK;
	} else { // whatever..
		return ST_PIPEFILE;
	}
}

ULONG FbxMode2Protection(const mode_t mode) {
	ULONG prot = FIBF_READ|FIBF_WRITE|FIBF_EXECUTE|FIBF_DELETE;

	if (mode & S_IRUSR) prot &= ~(FIBF_READ);
	if (mode & S_IWUSR) prot &= ~(FIBF_WRITE|FIBF_DELETE);
	if (mode & S_IXUSR) prot &= ~(FIBF_EXECUTE);
	if (mode & S_IRGRP) prot |= FIBF_GRP_READ;
	if (mode & S_IWGRP) prot |= FIBF_GRP_WRITE|FIBF_GRP_DELETE;
	if (mode & S_IXGRP) prot |= FIBF_GRP_EXECUTE;
	if (mode & S_IROTH) prot |= FIBF_OTR_READ;
	if (mode & S_IWOTH) prot |= FIBF_OTR_WRITE|FIBF_OTR_DELETE;
	if (mode & S_IXOTH) prot |= FIBF_OTR_EXECUTE;

	return prot;
}

UWORD FbxUnix2AmigaOwner(const uid_t owner) {
	if (owner == (uid_t)0) return DOS_OWNER_ROOT;
	else if (owner == (uid_t)-2) return DOS_OWNER_NONE;
	else return (UWORD)owner;
}

static const char *FbxFilePart(const char *path) {
	const char *name = strrchr(path, '/');
	return name ? (name + 1) : path;
}

void FbxPathStat2FIB(struct FbxFS *fs, const char *fullpath, struct fbx_stat *stat,
	struct FileInfoBlock *fib)
{
	char *comment = fs->pathbuf[2];
	if (FbxStrcmp(fs, fullpath, "/") == 0) {
		FbxStrlcpy(fs, (char *)fib->fib_FileName + 1, fs->currvol->volname, sizeof(fib->fib_FileName));
		fib->fib_DirEntryType =
		fib->fib_EntryType = ST_ROOT;
	} else {
		FbxStrlcpy(fs, (char *)fib->fib_FileName + 1, FbxFilePart(fullpath), sizeof(fib->fib_FileName));
		fib->fib_DirEntryType =
		fib->fib_EntryType = FbxMode2EntryType(stat->st_mode);
	}
	fib->fib_FileName[0] = strlen((char *)fib->fib_FileName + 1);
	FbxGetComment(fs, fullpath, comment, MAXPATHLEN);
	FbxStrlcpy(fs, (char *)fib->fib_Comment + 1, comment, sizeof(fib->fib_Comment));
	fib->fib_Comment[0] = strlen((char *)fib->fib_Comment + 1);
	fib->fib_Size = stat->st_size;
	fib->fib_Protection = FbxMode2Protection(stat->st_mode);
	fib->fib_Protection |= FbxGetAmigaProtectionFlags(fs, fullpath);
	fib->fib_NumBlocks = stat->st_blocks;
	if (fs->fsflags & FBXF_USE_INO)
		fib->fib_DiskKey = (IPTR)stat->st_ino;
	else
		fib->fib_DiskKey = (IPTR)FbxHashPath(fs, fullpath);
	FbxTimeSpec2DS(fs, &stat->st_mtim, &fib->fib_Date);
	fib->fib_OwnerUID = FbxUnix2AmigaOwner(stat->st_uid);
	fib->fib_OwnerGID = FbxUnix2AmigaOwner(stat->st_gid);
}

int FbxExamineLock(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib) {
	struct fbx_stat statbuf;
	int error;

	PDEBUGF("FbxExamineLock(%#p, %#p, %#p)\n", fs, lock, fib);

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);	

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	FreeFbxDirDataList(fs, &lock->dirdatalist);

	if (!lock->info) {
		error = Fbx_getattr(fs, lock->entry->path, &statbuf);
	} else {
		error = Fbx_fgetattr(fs, lock->entry->path, &statbuf, lock->info);
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}
	FbxPathStat2FIB(fs, lock->entry->path, &statbuf, fib);
	lock->dirscan = FALSE;
	fs->r2 = 0;
	return DOSTRUE;
}

