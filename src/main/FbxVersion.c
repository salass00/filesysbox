/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include <libraries/filesysbox.h>
#include "filesysbox_vectors.h"
#include "filesysbox_internal.h"

/****** filesysbox.library/FbxVersion ***************************************
*
*   NAME
*      FbxVersion -- Get Filesysbox version.
*
*   SYNOPSIS
*      LONG FbxVersion(void);
*
*   FUNCTION
*
*   INPUTS
*
*   RESULT
*       Version number.
*
*   EXAMPLE
*
*   NOTES
*
*   BUGS
*
*   SEE ALSO
*       FbxFuseVersion()
*
*****************************************************************************
*
*/

#ifdef __AROS__
AROS_LH0(LONG, FbxVersion,
	struct FileSysBoxBase *, libBase, 11, FileSysBox)
{
	AROS_LIBFUNC_INIT
#else
LONG FbxVersion(
	REG(a6, struct FileSysBoxBase *libBase))
{
#endif
	ADEBUGF("FbxVersion()\n");

	return FILESYSBOX_VERSION;

#ifdef __AROS__
	AROS_LIBFUNC_EXIT
#endif
}

