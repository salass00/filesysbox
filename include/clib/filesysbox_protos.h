/* Automatically generated header (sfdc 1.4)! Do not edit! */

#ifndef CLIB_FILESYSBOX_PROTOS_H
#define CLIB_FILESYSBOX_PROTOS_H

/*
**	$VER: filesysbox_protos.h 1.0 (07.12.2014)
**
**	C prototypes. For use with 32 bit integers only.
**
**	Copyright � 2001 Amiga, Inc.
**	    All Rights Reserved
*/

#include <exec/types.h>
#include <dos/dos.h>
#include <utility/tagitem.h>
#include <libraries/filesysbox.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


/* "filesysbox.library" */
APTR FbxQueryMountMsg(struct Message * msg, LONG attr);
struct FbxFS * FbxSetupFS(struct Message * msg, const struct TagItem * tags, const struct fuse_operations * ops, LONG opssize, APTR udata);
LONG FbxEventLoop(struct FbxFS * fs);
void FbxCleanupFS(struct FbxFS * fs);
void FbxReturnMountMsg(struct Message * msg, SIPTR r1, SIPTR r2);
LONG FbxFuseVersion(void);
LONG FbxVersion(void);
void FbxSetSignalCallback(struct FbxFS * fs, FbxSignalCallbackFunc func, ULONG signals);
struct FbxTimerCallbackData * FbxInstallTimerCallback(struct FbxFS * fs, FbxTimerCallbackFunc func, ULONG period);
void FbxUninstallTimerCallback(struct FbxFS * fs, struct FbxTimerCallbackData * cb);
void FbxSignalDiskChange(struct FbxFS * fs);
void FbxCopyStringBSTRToC(BSTR src, STRPTR dst, ULONG size);
void FbxCopyStringCToBSTR(CONST_STRPTR src, BSTR dst, ULONG size);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CLIB_FILESYSBOX_PROTOS_H */
