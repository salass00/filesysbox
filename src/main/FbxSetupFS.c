/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"
#include "codesets.h"

#include <errno.h>
#include <string.h>
#include <stdarg.h>

/****** filesysbox.library/FbxSetupFS ***************************************
*
*   NAME
*      FbxSetupFS -- Create a filesystem handle.
*      FbxSetupFSTags -- Vararg stub
*
*   SYNOPSIS
*      struct FbxFS * FbxSetupFS(struct Message *msg, 
*          const struct TagItem *tags, struct fuse_operations *ops, 
*          LONG opssize, APTR udata);
*
*   FUNCTION
*       Creates a filesystem handle and initialises it.
*       If "msg" is given, it will get replied with either success or failure.
*
*   INPUTS
*       msg - mount message, or NULL if no mesage is available.
*       tags - taglist for additional parameters.
*       ops - pointer to fuse_operations table.
*       opssize - size of above table.
*       udata - this value will get inserted into fuse_context.private_data.
*
*   TAGS
*       FBXT_FSFLAGS (ULONG)
*           Filesystem flags:
*
*           FBXF_ENABLE_UTF8_NAMES
*               Must be given if filesystem uses UTF-8 encoded filenames.
*               This will tell filesysbox to open needed resources for
*               character conversion.
*
*           FBXF_ENABLE_DISK_CHANGE_DETECTION
*               Enables disk change detection by using TD_ADDCHANGEINT.
*               Only usable with trackdisk device based filesystems
*               (fssm required).
*
*           FBXF_USE_INO (V54)
*               Indicates that st_ino is set by the filesystem. If this
*               flag is not set filesysbox will generate a hash from the
*               path string and use this instead of st_ino for the
*               ObjectID.
*
*       FBXT_FSSM (struct FileSysStartupMsg *)
*           Overrides the one in msg.
*           A NULL fssm is OK and will disable ACTION_GET_DISK_FSSM.
*
*       FBXT_DOSTYPE (ULONG)
*           Overrides the dostype from fssm->fssm_Environ.
*           Must be given if there is no msg.
*
*       FBXT_GET_CONTEXT (struct fuse_context **)
*           Puts the address of filesystem context in the pointer
*           variable given by reference.
*
*       FBXT_ACTIVE_UPDATE_TIMEOUT (ULONG)
*           Active update timeout in milliseconds. Defaults to 10000.
*           Setting this timeout to zero disables it.
*
*       FBXT_INACTIVE_UPDATE_TIMEOUT (ULONG)
*           Inactive update timeout in milliseconds. Defaults to 500.
*           Setting this timeout to zero disables it.
*
*   RESULT
*       A filesystem handle or NULL if setup failed.
*
*   EXAMPLE
*
*   NOTES
*       It should be noted that in the varargs version the tagitem list is
*       at the end while in the non-varargs version it is only the second
*       parameter (third if you count the hidden interface pointer).
*
*   BUGS
*
*   SEE ALSO
*       FbxCleanupFS()
*
*****************************************************************************
*
*/

static int dumopfunc(void) { return -ENOSYS; }
static int dumopfunc2(void) { return 0; }

static void FbxDiskChangeHandler(struct FbxFS *fs);
void FbxReadDebugFlags(struct FbxFS *fs);
#ifdef ENABLE_CHARSET_CONVERSION
static void FbxGetCharsetMapTable(struct FbxFS *fs);
#endif

#ifdef __AROS__
#define FbxReturnMountMsg(msg, r1, r2) \
	AROS_LC3(void, FbxReturnMountMsg, \
		AROS_LCA(struct Message *, (msg), A0), \
		AROS_LCA(LONG, (r1), D0), \
		AROS_LCA(LONG, (r2), D1), \
		struct FileSysBoxBase *, libBase, 9, FileSysBox)
#define FbxCleanupFS(fs) \
	AROS_LC1(void, FbxCleanupFS, \
		AROS_LCA(struct FbxFS *, (fs), A0), \
		struct FileSysBoxBase *, libBase, 8, FileSysBox)
#else
#define FbxReturnMountMsg(msg, r1, r2) FbxReturnMountMsg((msg), (r1), (r2), libBase)
#define FbxCleanupFS(fs) FbxCleanupFS((fs), libBase)
#endif

#ifdef __AROS__
AROS_LH5(struct FbxFS *, FbxSetupFS,
	AROS_LHA(struct Message *, msg, A0),
	AROS_LHA(const struct TagItem *, tags, A1),
	AROS_LHA(const struct fuse_operations *, ops, A2),
	AROS_LHA(LONG, opssize, D0),
	AROS_LHA(APTR, udata, A3),
	struct FileSysBoxBase *, libBase, 6, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
struct FbxFS *FbxSetupFS(
	REG(a0, struct Message *msg),
	REG(a1, const struct TagItem *tags),
	REG(a2, const struct fuse_operations *ops),
	REG(d0, LONG opssize),
	REG(a3, APTR udata),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library *SysBase     = libBase->sysbase;
	struct Library *DOSBase     = libBase->dosbase;
	struct Library *UtilityBase = libBase->utilitybase;
	struct Library *LocaleBase  = libBase->localebase;
	struct FbxFS *fs;
	struct TagItem *tstate;
	const struct TagItem *tag;

	ADEBUGF("FbxSetupFS(%#p, %#p, %#p, %ld, %#p)\n", msg, tags, ops, opssize, user_data);

	fs = AllocFbxFS();
	if (fs == NULL) goto error;

	fs->libbase = libBase;

	fs->dbgflagssig = -1;
	fs->diskchangesig = -1;

	fs->sysbase     = SysBase;
	fs->dosbase     = DOSBase;
	fs->utilitybase = UtilityBase;
	fs->localebase  = LocaleBase;

	fs->thisproc = (struct Process *)FindTask(NULL);

	fs->dostype = ID_DOS_DISK;

	fs->aut = ACTIVE_UPDATE_TIMEOUT_MILLIS;
	fs->iaut = INACTIVE_UPDATE_TIMEOUT_MILLIS;

	NEWLIST(&fs->volumelist);
	NEWLIST(&fs->timercallbacklist);

	if (msg != NULL) {
		struct DosPacket *pkt = (struct DosPacket *)msg->mn_Node.ln_Name;

		fs->devnode = BADDR(pkt->dp_Arg3);

		fs->fssm = FbxGetFSSM(SysBase, fs->devnode);
		if (fs->fssm) {
			struct DosEnvec *de = BADDR(fs->fssm->fssm_Environ);

			if (de->de_TableSize >= DE_DOSTYPE)
				fs->dostype = de->de_DosType;
		}
	}

	FbxReadDebugFlags(fs);

	if (LocaleBase != NULL) {
		struct Locale *locale;
		if ((locale = OpenLocale(NULL))) {
			fs->gmtoffset = (int)locale->loc_GMTOffset;
			CloseLocale(locale);
		}
	}

	if (FbxSetupTimerIO(fs) == NULL) goto error;

	InitSemaphore(&fs->fssema);

	FbxInitUpTime(fs);

	fs->mempool = CreatePool(MEMF_PUBLIC, 4096, 1024);
	if (fs->mempool == NULL) goto error;

	fs->dbgflagssig = AllocSignal(-1);
	if (fs->dbgflagssig == -1) goto error;

	fs->diskchangesig = AllocSignal(-1);
	if (fs->diskchangesig == -1) goto error;

	fs->fsport = CreateMsgPort();
	if (fs->fsport == NULL) goto error;

	fs->notifyreplyport = CreateMsgPort();
	if (fs->notifyreplyport == NULL) goto error;

	// make a copy of fuse_operations
	CopyMem((APTR)ops, &fs->ops, min(opssize, sizeof(fs->ops)));

	// install dummy functions in empty slots of fuse_operations array.
	#define FIX_SLOT(name) if (!FSOP name) FSOP name = (void *)&dumopfunc;
	#define FIX_SLOT2(name) if (!FSOP name) FSOP name = (void *)&dumopfunc2;
	FIX_SLOT(readlink)
	FIX_SLOT(mkdir)
	FIX_SLOT(unlink)
	FIX_SLOT(rmdir)
	FIX_SLOT(symlink)
	FIX_SLOT(rename)
	FIX_SLOT(link)
	FIX_SLOT(chmod)
	FIX_SLOT(chown)
	FIX_SLOT(truncate)
	FIX_SLOT(utime)
	FIX_SLOT2(open)
	FIX_SLOT(read)
	FIX_SLOT(write)
	FIX_SLOT(statfs)
	FIX_SLOT2(flush)
	FIX_SLOT2(release)
	FIX_SLOT(fsync)
	FIX_SLOT(fsyncdir)
	FIX_SLOT(access)
	FIX_SLOT(create)
	FIX_SLOT(format)
	FIX_SLOT(relabel)
	FIX_SLOT(getxattr)
	FIX_SLOT(setxattr)
	FIX_SLOT(removexattr)

	if (!FSOP opendir) FSOP opendir = FSOP open;
	if (!FSOP releasedir) FSOP releasedir = FSOP release;
	if (!FSOP fgetattr) FSOP fgetattr = (void*)FSOP getattr;
	if (!FSOP ftruncate) FSOP ftruncate = (void*)FSOP truncate;

	fs->xattr_amiga_comment = "user.amiga_comment";
	fs->xattr_amiga_protection = "user.amiga_protection";

	// scan tags
	tstate = (struct TagItem *)tags;
	while ((tag = NextTagItem(&tstate)) != NULL) {
		switch (tag->ti_Tag) {
		case FBXT_FSFLAGS:
			fs->fsflags = tag->ti_Data;
			break;
		case FBXT_FSSM:
			fs->fssm = (struct FileSysStartupMsg *)tag->ti_Data;
			break;
		case FBXT_DOSTYPE:
			fs->dostype = tag->ti_Data;
			break;
		case FBXT_GET_CONTEXT:
			*(struct fuse_context **)tag->ti_Data = &fs->fcntx;
			break;
		case FBXT_ACTIVE_UPDATE_TIMEOUT:
			fs->aut = tag->ti_Data;
			break;
		case FBXT_INACTIVE_UPDATE_TIMEOUT:
			fs->iaut = tag->ti_Data;
			break;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (fs->fsflags & FBXF_ENABLE_UTF8_NAMES) {
		FbxGetCharsetMapTable(fs);
	}
#endif

	if (fs->fsflags & FBXF_ENABLE_DISK_CHANGE_DETECTION) {
		if (FbxAddDiskChangeHandler(fs, FbxDiskChangeHandler) == NULL) goto error;
	}

	// doslist process
	ObtainSemaphore(&libBase->dlproc_sem);
	if (libBase->dlproc == NULL) libBase->dlproc = StartDosListProc(libBase);
	if (libBase->dlproc != NULL) {
		libBase->dlproc_refcount++;
		fs->dlproc_port = &libBase->dlproc->pr_MsgPort;
	}
	ReleaseSemaphore(&libBase->dlproc_sem);
	if (libBase->dlproc == NULL) goto error;

	fs->fcntx.fuse = fs;
	fs->fcntx.private_data = udata;

	if (fs->devnode != NULL) fs->devnode->dn_Task = fs->fsport;

	// reply startup packet with success
	if (msg != NULL) FbxReturnMountMsg(msg, DOSTRUE, 0);

	ADEBUGF("FbxSetupFS: DONE => %#p\n", fs);

	return fs;

error:
	ADEBUGF("FbxSetupFS: FAILED\n");

	if (msg != NULL) FbxReturnMountMsg(msg, DOSFALSE, 0);

	FbxCleanupFS(fs);

	return NULL;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

static void FbxDiskChangeHandler(struct FbxFS *fs) {
	struct Library *SysBase = fs->sysbase;
	fs->dosetup = TRUE;
	Signal(&fs->thisproc->pr_Task, 1UL << fs->diskchangesig);
}

static void HexToLong(CONST_STRPTR str, ULONG *res) {
	TEXT c;
	ULONG v = 0;
	if (str[0] == '0' && str[1] == 'x') {
		str += 2;
	}
	while ((c = *str++) != '\0') {
		if (c >= '0' && c <= '9') c -= '0';
		else if (c >= 'A' && c <= 'F') c -= ('A' - 10);
		else if (c >= 'a' && c <= 'f') c -= ('a' - 10);
		else break;
		v <<= 4;
		v |= c;
	}
	*res = v;
}

void FbxReadDebugFlags(struct FbxFS *fs) {
	struct Library *DOSBase = fs->dosbase;
	TEXT flagbuf[32];
	ULONG flags = 0;

	if (GetVar((CONST_STRPTR)"FBX_DBGFLAGS", flagbuf, sizeof(flagbuf), 0) > 0) {
		HexToLong(flagbuf, &flags);
	}

	fs->dbgflags = flags;
}

#ifdef ENABLE_CHARSET_CONVERSION
static void FbxGetCharsetMapTable(struct FbxFS *fs) {
	struct Library *LocaleBase = fs->localebase;
	struct Locale *locale;

	if (LocaleBase != NULL) {
		locale = OpenLocale(NULL);
		if (locale != NULL) {
			struct Library *SysBase = fs->sysbase;
			struct Library *DOSBase = fs->dosbase;
			const struct FbxCodeSet *cs = NULL;
			TEXT charset[32];

			if (GetVar((CONST_STRPTR)"CHARSET", charset, sizeof(charset), 0) > 0) {
				/* Search using charset name */
				cs = FbxFindCodeSetByName(fs, charset);
			}

			if (cs == NULL) {
				/* Search using country code */
				cs = FbxFindCodeSetByCountry(fs, locale->loc_CountryCode);
			}

			if (cs == NULL) {
				/* Search using language name */
				cs = FbxFindCodeSetByLanguage(fs, locale->loc_LanguageName);
			}

			if (cs != NULL && cs->gen_maptab != NULL) {
				fs->maptable = (FbxUCS *)AllocMem(256*sizeof(FbxUCS), MEMF_ANY);
				if (fs->maptable != NULL) {
					cs->gen_maptab(fs->maptable);

					/* Generate AVL tree for faster unicode to local mapping */
					fs->avlbuf = (struct FbxAVL *)AllocMem(128*sizeof(struct FbxAVL), MEMF_ANY);
					if (fs->avlbuf != NULL)
						FbxSetupAVL(fs);
					else {
						FreeMem(fs->maptable, 256*sizeof(FbxUCS));
						fs->maptable = NULL;
					}
				}
			}

			CloseLocale(locale);
		}
	}
}
#endif

