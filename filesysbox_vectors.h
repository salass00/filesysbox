/*
 * Copyright (c) 2013-2015 Fredrik Wikstrom
 *
 * This code is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef EXEC_EXEC_H
#include <exec/exec.h>
#endif
#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif
#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif
#ifndef LIBRARIES_FILESYSBOX_H
#include <libraries/filesysbox.h>
#endif

#ifdef __AROS__

#include <aros/libcall.h>

AROS_LD1(struct FileSysBoxBase *, LibOpen,
	AROS_LDA(ULONG, version, D0),
	struct FileSysBoxBase *, libBase, 1, FileSysBox);

AROS_LD0(BPTR, LibClose,
	struct FileSysBoxBase *, libBase, 2, FileSysBox);

AROS_LD0(BPTR, LibExpunge,
	struct FileSysBoxBase *, libBase, 3, FileSysBox);

AROS_LD0(APTR, LibReserved,
	struct FileSysBoxBase *, libBase, 4, FileSysBox);

AROS_LD2(APTR, FbxQueryMountMsg,
	AROS_LDA(struct Message *, msg, A0),
	AROS_LDA(LONG, attr, D0),
	struct FileSysBoxBase *, libBase, 5, FileSysBox);

AROS_LD5(struct FbxFS *, FbxSetupFS,
	AROS_LDA(struct Message *, msg, A0),
	AROS_LDA(const struct TagItem *, tags, A1),
	AROS_LDA(const struct fuse_operations *, ops, A2),
	AROS_LDA(LONG, opssize, D0),
	AROS_LDA(APTR, udata, A3),
	struct FileSysBoxBase *, libBase, 6, FileSysBox);

AROS_LD1(LONG, FbxEventLoop,
	AROS_LDA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 7, FileSysBox);

AROS_LD1(void, FbxCleanupFS,
	AROS_LDA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 8, FileSysBox);

AROS_LD3(void, FbxReturnMountMsg,
	AROS_LDA(struct Message *, msg, A0),
	AROS_LDA(SIPTR, r1, D0),
	AROS_LDA(SIPTR, r2, D1),
	struct FileSysBoxBase *, libBase, 9, FileSysBox);

AROS_LD0(LONG, FbxFuseVersion,
	struct FileSysBoxBase *, libBase, 10, FileSysBox);

AROS_LD0(LONG, FbxVersion,
	struct FileSysBoxBase *, libBase, 11, FileSysBox);

AROS_LD3(void, FbxSetSignalCallback,
	AROS_LDA(struct FbxFS *, fs, A0),
	AROS_LDA(FbxSignalCallbackFunc, func, A1),
	AROS_LDA(ULONG, signals, D0),
	struct FileSysBoxBase *, libBase, 12, FileSysBox);

AROS_LD3(struct FbxTimerCallbackData *, FbxInstallTimerCallback,
	AROS_LDA(struct FbxFS *, fs, A0),
	AROS_LDA(FbxTimerCallbackFunc, func, A1),
	AROS_LDA(ULONG, period, D0),
	struct FileSysBoxBase *, libBase, 13, FileSysBox);

AROS_LD2(void, FbxUninstallTimerCallback,
	AROS_LDA(struct FbxFS *, fs, A0),
	AROS_LDA(struct FbxTimerCallbackData *, cb, A1),
	struct FileSysBoxBase *, libBase, 14, FileSysBox);

AROS_LD1(void, FbxSignalDiskChange,
	AROS_LDA(struct FbxFS *, fs, A0),
	struct FileSysBoxBase *, libBase, 15, FileSysBox);

AROS_LD3(void, FbxCopyStringBSTRToC,
	AROS_LDA(BSTR, src, A0),
	AROS_LDA(STRPTR, dst, A1),
	AROS_LDA(ULONG, size, D0),
	struct FileSysBoxBase *, libBase, 16, FileSysBox);

AROS_LD3(void, FbxCopyStringCToBSTR,
	AROS_LDA(CONST_STRPTR, src, A0),
	AROS_LDA(BSTR, dst, A1),
	AROS_LDA(ULONG, size, D0),
	struct FileSysBoxBase *, libBase, 17, FileSysBox);

#else

#include <SDI/SDI_compiler.h>

struct FileSysBoxBase *LibOpen(
	REG(d0, ULONG version),
	REG(a6, struct FileSysBoxBase *libBase));

BPTR LibClose(
	REG(a6, struct FileSysBoxBase *libBase));

BPTR LibExpunge(
	REG(a6, struct FileSysBoxBase *libBase));

APTR LibReserved(
	REG(a6, struct FileSysBoxBase *libBase));

APTR FbxQueryMountMsg(
	REG(a0, struct Message *msg),
	REG(d0, LONG attr),
	REG(a6, struct FileSysBoxBase *libBase));

struct FbxFS *FbxSetupFS(
	REG(a0, struct Message *msg),
	REG(a1, const struct TagItem *tags),
	REG(a2, const struct fuse_operations *ops),
	REG(d0, LONG opssize),
	REG(a3, APTR udata),
	REG(a6, struct FileSysBoxBase *libBase));

LONG FbxEventLoop(
	REG(a0, struct FbxFS *fs),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxCleanupFS(
	REG(a0, struct FbxFS * fs),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxReturnMountMsg(
	REG(a0, struct Message *msg),
	REG(d0, SIPTR r1),
	REG(d1, SIPTR r2),
	REG(a6, struct FileSysBoxBase *libBase));

LONG FbxFuseVersion(
	REG(a6, struct FileSysBoxBase *libBase));

LONG FbxVersion(
	REG(a6, struct FileSysBoxBase *libBase));

void FbxSetSignalCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, FbxSignalCallbackFunc func),
	REG(d0, ULONG signals),
	REG(a6, struct FileSysBoxBase *libBase));

struct FbxTimerCallbackData *FbxInstallTimerCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, FbxTimerCallbackFunc func),
	REG(d0, ULONG period),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxUninstallTimerCallback(
	REG(a0, struct FbxFS *fs),
	REG(a1, struct FbxTimerCallbackData *cb),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxSignalDiskChange(
	REG(a0, struct FbxFS *fs),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxCopyStringBSTRToC(
	REG(a0, BSTR src),
	REG(a1, STRPTR dst),
	REG(d0, ULONG size),
	REG(a6, struct FileSysBoxBase *libBase));

void FbxCopyStringCToBSTR(
	REG(a0, CONST_STRPTR src),
	REG(a1, BSTR dst),
	REG(d0, ULONG size),
	REG(a6, struct FileSysBoxBase *libBase));

#endif

