/*
 * Copyright (c) 2013-2019 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "../filesysbox_vectors.h"
#include "../filesysbox_internal.h"

#include <stdarg.h>

/****** filesysbox.library/FbxQueryFS ***************************************
*
*   NAME
*      FbxQueryFS -- Query function for FS
*      FbxQueryFSTags -- Vararg stub
*
*   SYNOPSIS
*      void FbxQueryFS(struct FbxFS * fs, const struct TagItem * tags);
*      void FbxQueryFSTags(struct FbxFS * fs, ...);
*
*   FUNCTION
*       Function for reading filesystem attributes.
*
*   INPUTS
*       fs - The result of FbxSetupFS().
*       tags - Tags to query.
*
*   TAGS
*       FBXT_FSFLAGS (ULONG)
*           Filesystem flags.
*
*       FBXT_FSSM (strut FileSysStartupMsg *)
*           FSSM pointer.
*
*       FBXT_DOSTYPE (ULONG)
*           Filesystem DOS type.
*
*       FBXT_GET_CONTEXT (struct fuse_context *)
*           Filesystem context pointer.
*
*       FBXT_ACTIVE_UPDATE_TIMEOUT (ULONG)
*           Active update timeout in milliseconds.
*
*       FBXT_INACTIVE_UPDATE_TIMEOUT (ULONG)
*           Inactive update timeout in milliseconds.
*
*       FBXT_GMT_OFFSET (LONG)
*           Returns a cached TZA_UTCOffset value. Its updated periodically
*           in case it changes because of a locale prefs change or because
*           of DST state change. Using GetTimezoneAttrs() directly from
*           any of the FUSE callbacks is not safe and can cause deadlocks.
*
*   RESULT
*       This function does not return a result
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*
*****************************************************************************
*
*/

#ifdef __AROS__
AROS_LH2(void, FbxQueryFS,
	AROS_LHA(struct FbxFS *, fs, A0),
	AROS_LHA(const struct TagItem *, tags, A1),
	struct FileSysBoxBase *, libBase, 18, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
void FbxQueryFS(
	REG(a0, struct FbxFS *fs),
	REG(a1, const struct TagItem *tags),
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	struct Library        *UtilityBase = libBase->utilitybase;
	struct TagItem        *tstate;
	const struct TagItem  *tag;

	tstate = (struct TagItem *)tags;
	while ((tag = NextTagItem(&tstate)) != NULL)
	{
		switch (tag->ti_Tag)
		{
			case FBXT_FSFLAGS:
				*(ULONG *)tag->ti_Data = fs->fsflags;
				break;

			case FBXT_FSSM:
				*(struct FileSysStartupMsg **)tag->ti_Data = fs->fssm;
				break;

			case FBXT_DOSTYPE:
				*(ULONG *)tag->ti_Data = fs->dostype;
				break;

			case FBXT_GET_CONTEXT:
				*(struct fuse_context **)tag->ti_Data = &fs->fcntx;
				break;

			case FBXT_ACTIVE_UPDATE_TIMEOUT:
				*(ULONG *)tag->ti_Data = fs->aut;
				break;

			case FBXT_INACTIVE_UPDATE_TIMEOUT:
				*(ULONG *)tag->ti_Data = fs->iaut;
				break;

			case FBXT_GMT_OFFSET:
				*(LONG *)tag->ti_Data = fs->gmtoffset;
				break;
		}
	}

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}
