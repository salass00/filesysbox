/* Automatically generated header (sfdc 1.11e)! Do not edit! */

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

#ifndef AROS_LIBCALL_H
#include <aros/libcall.h>
#endif /* !AROS_LIBCALL_H */

#ifndef FILESYSBOX_BASE_NAME
#define FILESYSBOX_BASE_NAME FileSysBoxBase
#endif /* !FILESYSBOX_BASE_NAME */

#define FbxQueryMountMsg(___msg, ___attr) \
      AROS_LC2(APTR, FbxQueryMountMsg, \
 AROS_LCA(struct Message *, (___msg), A0), \
 AROS_LCA(LONG, (___attr), D0), \
     struct Library *, FILESYSBOX_BASE_NAME, 5, Filesysbox)

#define FbxSetupFS(___msg, ___tags, ___ops, ___opssize, ___udata) \
      AROS_LC5(struct FbxFS *, FbxSetupFS, \
 AROS_LCA(struct Message *, (___msg), A0), \
 AROS_LCA(const struct TagItem *, (___tags), A1), \
 AROS_LCA(const struct fuse_operations *, (___ops), A2), \
 AROS_LCA(LONG, (___opssize), D0), \
 AROS_LCA(APTR, (___udata), A3), \
     struct Library *, FILESYSBOX_BASE_NAME, 6, Filesysbox)

#define FbxEventLoop(___fs) \
      AROS_LC1(LONG, FbxEventLoop, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
     struct Library *, FILESYSBOX_BASE_NAME, 7, Filesysbox)

#define FbxCleanupFS(___fs) \
      AROS_LC1(void, FbxCleanupFS, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
     struct Library *, FILESYSBOX_BASE_NAME, 8, Filesysbox)

#define FbxReturnMountMsg(___msg, ___r1, ___r2) \
      AROS_LC3(void, FbxReturnMountMsg, \
 AROS_LCA(struct Message *, (___msg), A0), \
 AROS_LCA(SIPTR, (___r1), D0), \
 AROS_LCA(SIPTR, (___r2), D1), \
     struct Library *, FILESYSBOX_BASE_NAME, 9, Filesysbox)

#define FbxFuseVersion() \
      AROS_LC0(LONG, FbxFuseVersion, \
     struct Library *, FILESYSBOX_BASE_NAME, 10, Filesysbox)

#define FbxVersion() \
      AROS_LC0(LONG, FbxVersion, \
     struct Library *, FILESYSBOX_BASE_NAME, 11, Filesysbox)

#define FbxSetSignalCallback(___fs, ___func, ___signals) \
      AROS_LC3(void, FbxSetSignalCallback, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
 AROS_LCA(FbxSignalCallbackFunc, (___func), A1), \
 AROS_LCA(ULONG, (___signals), D0), \
     struct Library *, FILESYSBOX_BASE_NAME, 12, Filesysbox)

#define FbxInstallTimerCallback(___fs, ___func, ___period) \
      AROS_LC3(struct FbxTimerCallbackData *, FbxInstallTimerCallback, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
 AROS_LCA(FbxTimerCallbackFunc, (___func), A1), \
 AROS_LCA(ULONG, (___period), D0), \
     struct Library *, FILESYSBOX_BASE_NAME, 13, Filesysbox)

#define FbxUninstallTimerCallback(___fs, ___cb) \
      AROS_LC2(void, FbxUninstallTimerCallback, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
 AROS_LCA(struct FbxTimerCallbackData *, (___cb), A1), \
     struct Library *, FILESYSBOX_BASE_NAME, 14, Filesysbox)

#define FbxSignalDiskChange(___fs) \
      AROS_LC1(void, FbxSignalDiskChange, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
     struct Library *, FILESYSBOX_BASE_NAME, 15, Filesysbox)

#define FbxCopyStringBSTRToC(___src, ___dst, ___size) \
      AROS_LC3(void, FbxCopyStringBSTRToC, \
 AROS_LCA(BSTR, (___src), A0), \
 AROS_LCA(STRPTR, (___dst), A1), \
 AROS_LCA(ULONG, (___size), D0), \
     struct Library *, FILESYSBOX_BASE_NAME, 16, Filesysbox)

#define FbxCopyStringCToBSTR(___src, ___dst, ___size) \
      AROS_LC3(void, FbxCopyStringCToBSTR, \
 AROS_LCA(CONST_STRPTR, (___src), A0), \
 AROS_LCA(BSTR, (___dst), A1), \
 AROS_LCA(ULONG, (___size), D0), \
     struct Library *, FILESYSBOX_BASE_NAME, 17, Filesysbox)

#define FbxQueryFS(___fs, ___tags) \
      AROS_LC2(void, FbxQueryFS, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
 AROS_LCA(const struct TagItem *, (___tags), A1), \
     struct Library *, FILESYSBOX_BASE_NAME, 18, Filesysbox)

#ifndef NO_INLINE_STDARG
#define FbxQueryFSTags(___fs, ___tags, ...) \
    ({_sfdc_vararg _tags[] = { ___tags, __VA_ARGS__ }; FbxQueryFS((___fs), (const struct TagItem *) _tags); })
#endif /* !NO_INLINE_STDARG */

#define FbxGetSysTime(___fs, ___tv) \
      AROS_LC2(void, FbxGetSysTime, \
 AROS_LCA(struct FbxFS *, (___fs), A0), \
 AROS_LCA(struct timeval *, (___tv), A1), \
     struct Library *, FILESYSBOX_BASE_NAME, 19, Filesysbox)

#endif /* !_INLINE_FILESYSBOX_H */
