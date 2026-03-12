/*
 * Copyright (c) 2013-2026 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_vectors.h"

#ifdef __AROS__
#define FBX_LIB_ENTRY(name,index) AROS_SLIB_ENTRY(name,FileSysBox,index)
#else
#define FBX_LIB_ENTRY(name,index) name
#endif

/* Library vector table used by the resident/autoinit setup.
 * This follows the Exec library initialization model expected by this code.
 */
static const CONST_APTR LibVectors[] = {
	FBX_LIB_ENTRY(LibOpen, 1),
	FBX_LIB_ENTRY(LibClose, 2),
	FBX_LIB_ENTRY(LibExpunge, 3),
	FBX_LIB_ENTRY(LibReserved, 4),
	FBX_LIB_ENTRY(FbxQueryMountMsg, 5),
	FBX_LIB_ENTRY(FbxSetupFS, 6),
#ifdef ENABLE_STACKSWAP
	FBX_LIB_ENTRY(FbxEventLoop_SS, 7),
#else
	FBX_LIB_ENTRY(FbxEventLoop, 7),
#endif
	FBX_LIB_ENTRY(FbxCleanupFS, 8),
	FBX_LIB_ENTRY(FbxReturnMountMsg, 9),
	FBX_LIB_ENTRY(FbxFuseVersion, 10),
	FBX_LIB_ENTRY(FbxVersion, 11),
	FBX_LIB_ENTRY(FbxSetSignalCallback, 12),
	FBX_LIB_ENTRY(FbxInstallTimerCallback, 13),
	FBX_LIB_ENTRY(FbxUninstallTimerCallback, 14),
	FBX_LIB_ENTRY(FbxSignalDiskChange, 15),
	FBX_LIB_ENTRY(FbxCopyStringBSTRToC, 16),
	FBX_LIB_ENTRY(FbxCopyStringCToBSTR, 17),
	FBX_LIB_ENTRY(FbxQueryFS, 18),
	FBX_LIB_ENTRY(FbxGetSysTime, 19),
	FBX_LIB_ENTRY(FbxGetUpTime, 20),
	(APTR)-1
};

