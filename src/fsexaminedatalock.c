/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifdef ENABLE_EXD_SUPPORT
#include "filesysbox_internal.h"
#include "fuse_stubs.h"

static ULONG FbxMode2Type(const mode_t mode) {
	if (S_ISDIR(mode))
		return FSO_TYPE_DIRECTORY;
	else if (S_ISREG(mode))
		return FSO_TYPE_FILE;
	else if (S_ISLNK(mode))
		return FSO_TYPE_SOFTLINK|FSOF_LINK;
	else /* whatever.. */
		return FSO_TYPE_PIPE;
}

struct ExamineData *FbxExamineData(struct FbxFS *fs, const char *fullpath,
	struct fuse_file_info *fi)
{
	struct Library *SysBase = fs->sysbase;
	struct Library *DOSBase = fs->dosbase;
	struct fbx_stat statbuf;
	int error;
	struct ExamineData *exd;
#ifdef ENABLE_CHARSET_CONVERSION
	char name[FBX_MAX_NAME];
	char fscomment[FBX_MAX_COMMENT];
	char fssoftname[FBX_MAX_PATH];
#else
	const char *name;
#endif
	char comment[FBX_MAX_COMMENT];
	char softname[FBX_MAX_PATH];

	if (IsRoot(fullpath)) {
#ifdef ENABLE_CHARSET_CONVERSION
		strlcpy(name, fs->currvol->volname, FBX_MAX_NAME);
#else
		name = fs->currvol->volname;
#endif
	} else {
#ifdef ENABLE_CHARSET_CONVERSION
		if (FbxUTF8ToLocal(fs, name, FbxFilePart(fullpath), FBX_MAX_NAME) >= FBX_MAX_NAME) {
			fs->r2 = ERROR_LINE_TOO_LONG;
			return NULL;
		}
#else
		name = FbxFilePart(fullpath);
#endif
	}

	if (fi == NULL)
		error = Fbx_getattr(fs, fullpath, &statbuf);
	else
		error = Fbx_fgetattr(fs, fullpath, &statbuf, fi);

	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return NULL;
	}

#ifdef ENABLE_CHARSET_CONVERSION
	FbxGetComment(fs, fullpath, fscomment, FBX_MAX_COMMENT);
	FbxUTF8ToLocal(fs, comment, fscomment, FBX_MAX_COMMENT);
#else
	FbxGetComment(fs, fullpath, comment, FBX_MAX_COMMENT);
#endif

	softname[0] = '\0';
	if (S_ISLNK(statbuf.st_mode)) {
#ifdef ENABLE_CHARSET_CONVERSION
		error = Fbx_readlink(fs, fullpath, fssoftname, FBX_MAX_PATH);
#else
		error = Fbx_readlink(fs, fullpath, ssoftname, FBX_MAX_PATH);
#endif
		if (error) {
			fs->r2 = FbxFuseErrno2Error(error);
			return NULL;
		}
#ifdef ENABLE_CHARSET_CONVERSION
		FbxUTF8ToLocal(fs, softname, fssoftname, FBX_MAX_PATH);
#endif
	}

	exd = AllocDosObjectTags(DOS_EXAMINEDATA,
		ADO_ExamineData_NameSize,    strlen(name) + 1,
		ADO_ExamineData_CommentSize, strlen(comment) + 1,
		ADO_ExamineData_LinkSize,    strlen(softname) + 1,
		TAG_END);
	if (exd == NULL) {
		fs->r2 = ERROR_NO_FREE_STORE;
		return NULL;
	}

	CopyMem(name, exd->Name, exd->NameSize);
	CopyMem(comment, exd->Comment, exd->CommentSize);
	CopyMem(softname, exd->Link, exd->LinkSize);

	exd->Type = FbxMode2Type(statbuf.st_mode);
	exd->FileSize = S_ISREG(statbuf.st_mode) ? statbuf.st_size : -1LL;
	FbxTimeSpec2DS(fs, &statbuf.st_mtim, &exd->Date);
	exd->RefCount = statbuf.st_nlink;
	exd->Protection = FbxMode2Protection(statbuf.st_mode);
	exd->Protection |= FbxGetAmigaProtectionFlags(fs, fullpath);

	if (fs->fsflags & FBXF_USE_INO)
		exd->ObjectID = statbuf.st_ino;
	else
		exd->ObjectID = FbxHashPathIno(fs, fullpath);

	exd->OwnerUID = FbxUnix2AmigaOwner(statbuf.st_uid);
	exd->OwnerGID = FbxUnix2AmigaOwner(statbuf.st_gid);

	fs->r2 = 0;
	return exd;
}

struct ExamineData *FbxExamineDataLock(struct FbxFS *fs, struct FbxLock *lock) {
	PDEBUGF("FbxExamineDataLock(%#p, %#p)\n", fs, lock);

	CHECKVOLUME(NULL);

	CHECKLOCK(lock, NULL);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return NULL;
	}

	return FbxExamineData(fs, lock->entry->path, lock->info);
}

#endif /* ENABLE_EXD_SUPPORT */

