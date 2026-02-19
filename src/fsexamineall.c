/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <string.h>
#include <stdint.h>

static char *FbxExAllStrdup(struct FbxFS *fs, struct FbxLock *lock, const char *src, size_t srclen) {
	struct Library *SysBase = fs->sysbase;
	char *dst;

	if (src == NULL)
		return NULL;

	dst = AllocVecPooled(lock->mempool, srclen + 1);
	if (dst == NULL)
		return NULL;

	CopyMem(src, dst, srclen + 1);
	return dst;
}

#define offset_after(type,member) (offsetof(type, member) + sizeof(((type *)0)->member))

int FbxExamineAll(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR bufsize,
	int type, struct ExAllControl *ctrl)
{
	struct Library *SysBase     = fs->sysbase;
	struct Library *DOSBase     = fs->dosbase;
	struct Library *UtilityBase = fs->utilitybase;
	struct FbxDirData *ed = NULL;
	struct FbxExAllState *exallstate;
	int error, eadsize;
	struct ExAllData *prevead, *curread;
	struct DateStamp ds;
	struct fbx_stat statbuf;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char name[FBX_MAX_NAME];
	size_t namelen;
#else
	const char *name;
#endif

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	ctrl->eac_Entries = 0;

	if (ctrl->eac_LastKey == (IPTR)NULL) {
		if (lock->mempool == NULL) {
			lock->mempool = CreatePool(MEMF_PUBLIC, 4096, 1024);
			if (lock->mempool == NULL) {
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}
		}

		exallstate = AllocFbxExAllState(lock);
		if (exallstate == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		FreeFbxDirDataList(lock, &lock->dirdatalist);
		NEWMINLIST(&exallstate->freelist);

		// read in entries
		if (!FbxReadDir(fs, lock)) {
			FreeFbxDirDataList(lock, &lock->dirdatalist);
			FreeFbxExAllState(lock, exallstate);
			return DOSFALSE;
		}

		switch (type) {
		case ED_NAME:
			eadsize = offset_after(struct ExAllData, ed_Name);
			break;
		case ED_TYPE:
			eadsize = offset_after(struct ExAllData, ed_Type);
			break;
		case ED_SIZE:
			eadsize = offset_after(struct ExAllData, ed_Size);
			break;
		case ED_PROTECTION:
			eadsize = offset_after(struct ExAllData, ed_Prot);
			break;
		case ED_DATE:
			eadsize = offset_after(struct ExAllData, ed_Ticks);
			break;
		case ED_COMMENT:
			eadsize = offset_after(struct ExAllData, ed_Comment);
			break;
		case ED_OWNER:
			eadsize = offset_after(struct ExAllData, ed_OwnerGID);
			break;
		default:
			FreeFbxDirDataList(lock, &lock->dirdatalist);
			FreeFbxExAllState(lock, exallstate);
			fs->r2 = ERROR_BAD_NUMBER; // unsupported ED_XXX
			return DOSFALSE;
		}

		exallstate->eadsize = eadsize;
		ctrl->eac_LastKey = (IPTR)exallstate;
	} else if (ctrl->eac_LastKey == (IPTR)-1) {
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
		// free previous exdata
		FreeFbxDirDataList(lock, &exallstate->freelist);
	}

	curread = (struct ExAllData *)buffer;
	prevead = NULL;
	curread->ed_Next = NULL;
	eadsize = exallstate->eadsize;
	while (((IPTR)curread + eadsize) <= ((IPTR)buffer + bufsize)) {
		ed = (struct FbxDirData *)RemHead((struct List *)&lock->dirdatalist);
		if (ed == NULL) break;

#ifdef ENABLE_CHARSET_CONVERSION
		if ((namelen = FbxUTF8ToLocal(fs, name, ed->fsname, FBX_MAX_NAME)) >= FBX_MAX_NAME)	{
			FreeFbxDirData(lock, ed);
			fs->r2 = ERROR_LINE_TOO_LONG;
			return DOSFALSE;
		}
#else
		name = ed->fsname;
#endif

		curread->ed_Next = NULL;

		if (ctrl->eac_MatchString != NULL &&
			ctrl->eac_MatchFunc == NULL &&
			!MatchPatternNoCase(ctrl->eac_MatchString, (STRPTR)name))
		{
			FreeFbxDirData(lock, ed);
			continue;
		}

		if (type >= ED_TYPE) {
			if (!FbxLockName2Path(fs, lock, ed->fsname, fullpath)) {
				FreeFbxDirData(lock, ed);
				fs->r2 = ERROR_INVALID_COMPONENT_NAME;
				return DOSFALSE;
			}

			if (fs->fsflags & FBXF_USE_FILL_DIR_STAT) {
				statbuf = ed->stat;
			} else {
				error = Fbx_getattr(fs, fullpath, &statbuf);
				if (error) {
					FreeFbxDirData(lock, ed);
					fs->r2 = FbxFuseErrno2Error(error);
					return DOSFALSE;
				}
			}
		}

		if (type >= ED_NAME) {
#ifdef ENABLE_CHARSET_CONVERSION
			ed->name = FbxExAllStrdup(fs, lock, name, namelen);
			if (ed->name == NULL) {
				FreeFbxDirData(lock, ed);
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}
			curread->ed_Name = (STRPTR)ed->name;
#else
			curread->ed_Name = (STRPTR)name;
#endif
		}
		if (type >= ED_TYPE) curread->ed_Type = FbxMode2EntryType(statbuf.st_mode);
		if (type >= ED_SIZE) {
			if (statbuf.st_size < 0)
				curread->ed_Size = 0;
			else if (sizeof(curread->ed_Size) == 4 && statbuf.st_size >= INT32_MAX)
				curread->ed_Size = INT32_MAX-1;
			else
				curread->ed_Size = statbuf.st_size;
		}
		if (type >= ED_PROTECTION) {
			curread->ed_Prot  = FbxMode2Protection(statbuf.st_mode);
			curread->ed_Prot |= FbxGetAmigaProtectionFlags(fs, fullpath);
		}
		if (type >= ED_DATE) {
			FbxTimeSpec2DS(fs, &statbuf.st_mtim, &ds);
			curread->ed_Days  = ds.ds_Days;
			curread->ed_Mins  = ds.ds_Minute;
			curread->ed_Ticks = ds.ds_Tick;
		}
		if (type >= ED_COMMENT) {
#ifdef ENABLE_CHARSET_CONVERSION
			char fscomment[FBX_MAX_COMMENT];
#endif
			char comment[FBX_MAX_COMMENT];
			size_t commentlen;
#ifdef ENABLE_CHARSET_CONVERSION
			FbxGetComment(fs, fullpath, fscomment, FBX_MAX_COMMENT);
			if ((commentlen = FbxUTF8ToLocal(fs, comment, fscomment, FBX_MAX_COMMENT)) >= FBX_MAX_COMMENT) {
				FreeFbxDirData(lock, ed);
				fs->r2 = ERROR_LINE_TOO_LONG;
				return DOSFALSE;
			}
#else
			FbxGetComment(fs, fullpath, comment, FBX_MAX_COMMENT);
			commentlen = strlen(comment);
#endif
			if (commentlen > 0) {
				ed->comment = FbxExAllStrdup(fs, lock, comment, commentlen);
				if (ed->comment == NULL) {
					FreeFbxDirData(lock, ed);
					fs->r2 = ERROR_NO_FREE_STORE;
					return DOSFALSE;
				}
			}
			if (ed->comment != NULL)
				curread->ed_Comment = (STRPTR)ed->comment;
			else
				curread->ed_Comment = (STRPTR)"";
		}
		if (type >= ED_OWNER) {
			curread->ed_OwnerUID = FbxUnix2AmigaOwner(statbuf.st_uid);
			curread->ed_OwnerGID = FbxUnix2AmigaOwner(statbuf.st_gid);
		}

		if (ctrl->eac_MatchFunc != NULL &&
			!CallHookPkt(ctrl->eac_MatchFunc, &type, curread))
		{
			FreeFbxDirData(lock, ed);
			continue;
		}

		AddTail((struct List *)&exallstate->freelist, (struct Node *)ed);

		ctrl->eac_Entries++;

		if (prevead != NULL) prevead->ed_Next = curread; // link us in
		prevead = curread;
		curread = (struct ExAllData *)((IPTR)curread + eadsize); // advance to next ead
	}

	if (IsMinListEmpty(&lock->dirdatalist)) {
		if (ctrl->eac_Entries == 0) {
			FreeFbxExAllState(lock, exallstate);
			ctrl->eac_LastKey = (IPTR)-1;
		}
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		fs->r2 = 0;
		return DOSTRUE;
	}
}

