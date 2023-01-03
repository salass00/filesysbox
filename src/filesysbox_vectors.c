/*
 * Copyright (c) 2013-2023 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_vectors.h"

#ifdef __AROS__
#define LibOpen AROS_SLIB_ENTRY(LibOpen,FileSysBox,1)
#define LibClose AROS_SLIB_ENTRY(LibClose,FileSysBox,2)
#define LibExpunge AROS_SLIB_ENTRY(LibExpunge,FileSysBox,3)
#define LibReserved AROS_SLIB_ENTRY(LibReserved,FileSysBox,4)
#define FbxQueryMountMsg AROS_SLIB_ENTRY(FbxQueryMountMsg,FileSysBox,5)
#define FbxSetupFS AROS_SLIB_ENTRY(FbxSetupFS,FileSysBox,6)
#define FbxEventLoop AROS_SLIB_ENTRY(FbxEventLoop,FileSysBox,7)
#define FbxCleanupFS AROS_SLIB_ENTRY(FbxCleanupFS,FileSysBox,8)
#define FbxReturnMountMsg AROS_SLIB_ENTRY(FbxReturnMountMsg,FileSysBox,9)
#define FbxFuseVersion AROS_SLIB_ENTRY(FbxFuseVersion,FileSysBox,10)
#define FbxVersion AROS_SLIB_ENTRY(FbxVersion,FileSysBox,11)
#define FbxSetSignalCallback AROS_SLIB_ENTRY(FbxSetSignalCallback,FileSysBox,12)
#define FbxInstallTimerCallback AROS_SLIB_ENTRY(FbxInstallTimerCallback,FileSysBox,13)
#define FbxUninstallTimerCallback AROS_SLIB_ENTRY(FbxUninstallTimerCallback,FileSysBox,14)
#define FbxSignalDiskChange AROS_SLIB_ENTRY(FbxSignalDiskChange,FileSysBox,15)
#define FbxCopyStringBSTRToC AROS_SLIB_ENTRY(FbxCopyStringBSTRToC,FileSysBox,16)
#define FbxCopyStringCToBSTR AROS_SLIB_ENTRY(FbxCopyStringCToBSTR,FileSysBox,17)
#define FbxQueryFS AROS_SLIB_ENTRY(FbxQueryFS,FileSysBox,18)
#define FbxGetSysTime AROS_SLIB_ENTRY(FbxGetSysTime,FileSysBox,19)
#endif

static const CONST_APTR LibVectors[] = {
	LibOpen,
	LibClose,
	LibExpunge,
	LibReserved,
	FbxQueryMountMsg,
	FbxSetupFS,
	FbxEventLoop,
	FbxCleanupFS,
	FbxReturnMountMsg,
	FbxFuseVersion,
	FbxVersion,
	FbxSetSignalCallback,
	FbxInstallTimerCallback,
	FbxUninstallTimerCallback,
	FbxSignalDiskChange,
	FbxCopyStringBSTRToC,
	FbxCopyStringCToBSTR,
	FbxQueryFS,
	FbxGetSysTime,
	(APTR)-1
};

