/* Automatically generated header (sfdc 1.12)! Do not edit! */

#ifndef _INLINE_FILESYSBOX_H
#define _INLINE_FILESYSBOX_H

#ifndef _SFDC_VARARG_DEFINED
#define _SFDC_VARARG_DEFINED
#ifdef __HAVE_IPTR_ATTR__
typedef APTR _sfdc_vararg __attribute__((iptr));
#else
typedef ULONG _sfdc_vararg;
#endif /* __HAVE_IPTR_ATTR__ */
#endif /* _SFDC_VARARG_DEFINED */

#ifndef __INLINE_MACROS_H
#include <inline/macros.h>
#endif /* !__INLINE_MACROS_H */

#ifndef FILESYSBOX_BASE_NAME
#define FILESYSBOX_BASE_NAME FileSysBoxBase
#endif /* !FILESYSBOX_BASE_NAME */

#define FbxQueryMountMsg(___msg, ___attr) \
      LP2(0x1e, APTR, FbxQueryMountMsg , struct Message *, ___msg, a0, LONG, ___attr, d0,\
      , FILESYSBOX_BASE_NAME)

#define FbxSetupFS(___msg, ___tags, ___ops, ___opssize, ___udata) \
      LP5(0x24, struct FbxFS *, FbxSetupFS , struct Message *, ___msg, a0, const struct TagItem *, ___tags, a1, const struct fuse_operations *, ___ops, a2, LONG, ___opssize, d0, APTR, ___udata, a3,\
      , FILESYSBOX_BASE_NAME)

#define FbxEventLoop(___fs) \
      LP1(0x2a, LONG, FbxEventLoop , struct FbxFS *, ___fs, a0,\
      , FILESYSBOX_BASE_NAME)

#define FbxCleanupFS(___fs) \
      LP1NR(0x30, FbxCleanupFS , struct FbxFS *, ___fs, a0,\
      , FILESYSBOX_BASE_NAME)

#define FbxReturnMountMsg(___msg, ___r1, ___r2) \
      LP3NR(0x36, FbxReturnMountMsg , struct Message *, ___msg, a0, SIPTR, ___r1, d0, SIPTR, ___r2, d1,\
      , FILESYSBOX_BASE_NAME)

#define FbxFuseVersion() \
      LP0(0x3c, LONG, FbxFuseVersion ,\
      , FILESYSBOX_BASE_NAME)

#define FbxVersion() \
      LP0(0x42, LONG, FbxVersion ,\
      , FILESYSBOX_BASE_NAME)

#define FbxSetSignalCallback(___fs, ___func, ___signals) \
      LP3NR(0x48, FbxSetSignalCallback , struct FbxFS *, ___fs, a0, FbxSignalCallbackFunc, ___func, a1, ULONG, ___signals, d0,\
      , FILESYSBOX_BASE_NAME)

#define FbxInstallTimerCallback(___fs, ___func, ___period) \
      LP3(0x4e, struct FbxTimerCallbackData *, FbxInstallTimerCallback , struct FbxFS *, ___fs, a0, FbxTimerCallbackFunc, ___func, a1, ULONG, ___period, d0,\
      , FILESYSBOX_BASE_NAME)

#define FbxUninstallTimerCallback(___fs, ___cb) \
      LP2NR(0x54, FbxUninstallTimerCallback , struct FbxFS *, ___fs, a0, struct FbxTimerCallbackData *, ___cb, a1,\
      , FILESYSBOX_BASE_NAME)

#define FbxSignalDiskChange(___fs) \
      LP1NR(0x5a, FbxSignalDiskChange , struct FbxFS *, ___fs, a0,\
      , FILESYSBOX_BASE_NAME)

#define FbxCopyStringBSTRToC(___src, ___dst, ___size) \
      LP3NR(0x60, FbxCopyStringBSTRToC , BSTR, ___src, a0, STRPTR, ___dst, a1, ULONG, ___size, d0,\
      , FILESYSBOX_BASE_NAME)

#define FbxCopyStringCToBSTR(___src, ___dst, ___size) \
      LP3NR(0x66, FbxCopyStringCToBSTR , CONST_STRPTR, ___src, a0, BSTR, ___dst, a1, ULONG, ___size, d0,\
      , FILESYSBOX_BASE_NAME)

#define FbxQueryFS(___fs, ___tags) \
      LP2NR(0x6c, FbxQueryFS , struct FbxFS *, ___fs, a0, const struct TagItem *, ___tags, a1,\
      , FILESYSBOX_BASE_NAME)

#ifndef NO_INLINE_STDARG
#define FbxQueryFSTags(___fs, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; FbxQueryFS((___fs), (const struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define FbxGetSysTime(___fs, ___tv) \
      LP2NR(0x72, FbxGetSysTime , struct FbxFS *, ___fs, a0, struct timeval *, ___tv, a1,\
      , FILESYSBOX_BASE_NAME)

#endif /* !_INLINE_FILESYSBOX_H */
