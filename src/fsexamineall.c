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

int FbxExamineAll(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR len,
	int type, struct ExAllControl *ctrl)
{
	struct Library *SysBase     = fs->sysbase;
	struct Library *DOSBase     = fs->dosbase;
	struct Library *UtilityBase = fs->utilitybase;
	struct FbxDirData *ed = NULL;
	int error, iptrs;
	struct FbxExAllState *exallstate;
	IPTR *lptr, *start, *prev;
	struct DateStamp ds;
	struct fbx_stat statbuf;
	char fullpath[FBX_MAX_PATH];
	const char *pname, *pcomment;

	CHECKVOLUME(DOSFALSE);

	CHECKLOCK(lock, DOSFALSE);

	if (lock->fsvol != fs->currvol) {
		fs->r2 = ERROR_NO_DISK;
		return DOSFALSE;
	}

	ctrl->eac_Entries = 0;

	if (ctrl->eac_LastKey == (IPTR)NULL) {
		exallstate = AllocFbxExAllState(fs);
		if (exallstate == NULL) {
			fs->r2 = ERROR_NO_FREE_STORE;
			return DOSFALSE;
		}

		FreeFbxDirDataList(fs, &lock->dirdatalist);
		NEWLIST(&exallstate->freelist);

		// read in entries
		if (!FbxReadDir(fs, lock)) {
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			return DOSFALSE;
		}

		switch (type) {
		case ED_NAME:
			iptrs = 2;
			break;
		case ED_TYPE:
			iptrs = 3;
			break;
		case ED_SIZE:
			iptrs = 4;
			break;
		case ED_PROTECTION:
			iptrs = 5;
			break;
		case ED_DATE:
			iptrs = 8;
			break;
		case ED_COMMENT:
			iptrs = 9;
			break;
		case ED_OWNER:
			iptrs = 10;
			break;
		default:
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			fs->r2 = ERROR_BAD_NUMBER; // unsupported ED_XXX
			return DOSFALSE;
		}

		exallstate->iptrs = iptrs;
		ctrl->eac_LastKey = (IPTR)exallstate;
	} else if (ctrl->eac_LastKey == (IPTR)-1) {
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		exallstate = (struct FbxExAllState *)ctrl->eac_LastKey;
		// free previous exdata
		FreeFbxDirDataList(fs, &exallstate->freelist);
	}

	lptr = buffer;
	start = prev = NULL;
	*lptr = 0; // clear next pointer
	iptrs = exallstate->iptrs;
	while ((char *)&lptr[iptrs] <= ((char *)buffer + len)) {
		ed = (struct FbxDirData *)RemHead((struct List *)&lock->dirdatalist);
		if (ed == NULL) break;

		pname = ed->name;
		pcomment = NULL;

#ifdef ENABLE_CHARSET_CONVERSION
		if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
			char adname[FBX_MAX_NAME];
			size_t namelen;
			if ((namelen = FbxUTF8ToLocal(fs, adname, ed->fsname, FBX_MAX_NAME))
				>= FBX_MAX_NAME)
			{
				FreeFbxDirData(fs, ed);
				fs->r2 = ERROR_LINE_TOO_LONG;
				return DOSFALSE;
			}
			ed->name = AllocVecPooled(fs->mempool, namelen + 1);
			if (ed->name == NULL) {
				FreeFbxDirData(fs, ed);
				fs->r2 = ERROR_NO_FREE_STORE;
				return DOSFALSE;
			}
			strlcpy(ed->name, adname, namelen + 1);
			pname = ed->name;
		}
#endif

		start = lptr;
		*lptr = (IPTR)NULL; // clear next pointer

		if (ctrl->eac_MatchString != NULL &&
			ctrl->eac_MatchFunc == NULL &&
			!MatchPatternNoCase(ctrl->eac_MatchString, (STRPTR)pname))
		{
			FreeFbxDirData(fs, ed);
			continue;
		}

		if (type >= ED_TYPE) {
			if (!FbxLockName2Path(fs, lock, ed->fsname, fullpath)) {
				FreeFbxDirData(fs, ed);
				fs->r2 = ERROR_INVALID_COMPONENT_NAME;
				return DOSFALSE;
			}

			if (fs->fsflags & FBXF_USE_FILL_DIR_STAT) {
				statbuf = ed->stat;
			} else {
				error = Fbx_getattr(fs, fullpath, &statbuf);
				if (error) {
					FreeFbxDirData(fs, ed);
					fs->r2 = FbxFuseErrno2Error(error);
					return DOSFALSE;
				}
			}
		}

		if (type >= ED_COMMENT) {
			char comment[FBX_MAX_COMMENT];

			FbxGetComment(fs, fullpath, comment, FBX_MAX_COMMENT);
			if (comment[0] != '\0') {
				const char *src;
				size_t srclen;
#ifdef ENABLE_CHARSET_CONVERSION
				char adcomment[FBX_MAX_COMMENT];

				if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {

					if ((srclen = FbxUTF8ToLocal(fs, adcomment, comment, FBX_MAX_COMMENT))
						>= FBX_MAX_COMMENT)
					{
						FreeFbxDirData(fs, ed);
						fs->r2 = ERROR_LINE_TOO_LONG;
						return DOSFALSE;
					}
					src = adcomment;
				} else {
					src = comment;
					srclen = strlen(comment);
				}
#else
				src = comment;
				srclen = strlen(comment);
#endif
				ed->comment = AllocVecPooled(fs->mempool, srclen + 1);
				if (ed->comment == NULL) {
					FreeFbxDirData(fs, ed);
					fs->r2 = ERROR_NO_FREE_STORE;
					return DOSFALSE;
				}
#ifdef ENABLE_CHARSET_CONVERSION
				strlcpy(ed->comment, src, srclen + 1);
#else
				FbxStrlcpy(fs, ed->comment, src, srclen + 1);
#endif
				pcomment = ed->comment;
			}
		}

		lptr++; // skip next pointer

		if (type >= ED_NAME) *lptr++ = (IPTR)pname;
		if (type >= ED_TYPE) *lptr++ = FbxMode2EntryType(statbuf.st_mode);
		if (type >= ED_SIZE) *lptr++ = (statbuf.st_size > 0xffffffff) ? 0 : statbuf.st_size;
		if (type >= ED_PROTECTION) {
			*lptr = FbxMode2Protection(statbuf.st_mode);
			*lptr++ |= FbxGetAmigaProtectionFlags(fs, fullpath);
		}
		if (type >= ED_DATE) {
			FbxTimeSpec2DS(fs, &statbuf.st_mtim, &ds);
			*lptr++ = ds.ds_Days;
			*lptr++ = ds.ds_Minute;
			*lptr++ = ds.ds_Tick;
		}
		if (type >= ED_COMMENT) *lptr++ = (pcomment != NULL) ? (IPTR)pcomment : (IPTR)"";
		if (type >= ED_OWNER) {
			ULONG uid = FbxUnix2AmigaOwner(statbuf.st_uid);
			ULONG gid = FbxUnix2AmigaOwner(statbuf.st_gid);
			*lptr++ = (uid << 16)|gid;
		}

		if (ctrl->eac_MatchFunc != NULL &&
			!CallHookPkt(ctrl->eac_MatchFunc, &type, start))
		{
			FreeFbxDirData(fs, ed);
			lptr = start;
			continue;
		}

		AddTail((struct List *)&exallstate->freelist, (struct Node *)ed);

		ctrl->eac_Entries++;

		if (prev != NULL) *prev = (IPTR)start; // link us in
		prev = start;
	}

	if (IsMinListEmpty(&lock->dirdatalist)) {
		if (ctrl->eac_Entries == 0) {
			FreeFbxDirDataList(fs, &lock->dirdatalist);
			FreeFbxExAllState(fs, exallstate);
			ctrl->eac_LastKey = (IPTR)-1;
		}
		fs->r2 = ERROR_NO_MORE_ENTRIES;
		return DOSFALSE;
	} else {
		fs->r2 = 0;
		return DOSTRUE;
	}
}

