/*
 * Copyright (c) 2013-2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <exec/resident.h>
#include "filesysbox_internal.h"
#include "filesysbox.library_rev.h"

static const char USED_VAR verstag[] = VERSTAG;

struct AROSSupportBase *DebugAROSBase;
struct Library *aroscbase;

#ifdef __AROS__
static AROS_UFH3(struct FileSysBoxBase *, LibInit,
	AROS_UFHA(struct FileSysBoxBase *, libBase, D0),
	AROS_UFHA(BPTR, seglist, A0),
	AROS_UFHA(struct Library *, SysBase, A6))
{
	AROS_USERFUNC_INIT
#else
static struct FileSysBoxBase *LibInit (REG(d0, struct FileSysBoxBase *libBase),
	REG(a0, BPTR seglist), REG(a6, struct Library *SysBase))
{
#endif

	libBase->libnode.lib_Node.ln_Type = NT_LIBRARY;
	libBase->libnode.lib_Node.ln_Pri  = 0;
	libBase->libnode.lib_Node.ln_Name = (char *)"filesysbox.library";
	libBase->libnode.lib_Flags        = LIBF_SUMUSED|LIBF_CHANGED;
	libBase->libnode.lib_Version      = VERSION;
	libBase->libnode.lib_Revision     = REVISION;
	libBase->libnode.lib_IdString     = (STRPTR)VSTRING;

	libBase->seglist = seglist;
	libBase->sysbase = SysBase;

	libBase->dosbase = OpenLibrary((CONST_STRPTR)"dos.library", 39);
	if (libBase->dosbase == NULL) goto error;

#ifdef __AROS__
	DebugAROSBase = (struct AROSSupportBase *)((struct ExecBase *)SysBase)->DebugAROSBase;

	aroscbase = OpenLibrary((CONST_STRPTR)"arosc.library", 41);
	if (aroscbase == NULL) goto error;
#endif

	InitSemaphore(&libBase->dlproc_sem);

	return libBase;

error:
	CloseLibrary(libBase->dosbase);

	FreeMem((BYTE *)libBase - libBase->libnode.lib_NegSize,
		libBase->libnode.lib_NegSize + libBase->libnode.lib_PosSize);

	return NULL;

#ifdef __AROS__
	AROS_USERFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH1(struct FileSysBoxBase *, LibOpen,
	AROS_LHA(ULONG, version, D0),
	struct FileSysBoxBase *, libBase, 1, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
struct FileSysBoxBase *LibOpen(
	REG(d0, ULONG version),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif

	if (version > VERSION) return NULL;

	/* Add any specific open code here
	   Return 0 before incrementing OpenCnt to fail opening */

	/* Add up the open count */
	libBase->libnode.lib_OpenCnt++;

	return libBase;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH0(BPTR, LibClose,
	struct FileSysBoxBase *, libBase, 2, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
BPTR LibClose(
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif

	/* Make sure to undo what open did */

    /* Make the close count */
    libBase->libnode.lib_OpenCnt--;

    return ZERO;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH0(BPTR, LibExpunge,
	struct FileSysBoxBase *, libBase, 3, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
BPTR LibExpunge(
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library *SysBase = libBase->sysbase;
	BPTR result = ZERO;

	if (libBase->libnode.lib_OpenCnt == 0 && libBase->dlproc == NULL) {
		result = libBase->seglist;

		/* Undo what the init code did */

		Remove((struct Node *)libBase);
		FreeMem((BYTE *)libBase - libBase->libnode.lib_NegSize,
			libBase->libnode.lib_NegSize + libBase->libnode.lib_PosSize);
	} else {
		libBase->libnode.lib_Flags |= LIBF_DELEXP;
	}

	return result;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#ifdef __AROS__
AROS_LH0(BPTR, LibReserved,
	struct FileSysBoxBase *, libBase, 4, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
BPTR LibReserved(
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif

	return NULL;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

#include "filesysbox_vectors.c"

static const CONST_APTR LibInitTab[] = {
	(APTR)sizeof(struct FileSysBoxBase),
	LibVectors,
	NULL,
	LibInit
};

static const struct Resident USED_VAR lib_res = {
	RTC_MATCHWORD,
	(struct Resident *)&lib_res,
	(APTR)(&lib_res + 1),
	RTF_AUTOINIT,
	VERSION,
	NT_LIBRARY,
	0,
	(STRPTR)"filesysbox.library",
	(STRPTR)VSTRING,
	(APTR)LibInitTab
};

