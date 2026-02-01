/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef FILESYSBOX_INTERNAL_H
#define FILESYSBOX_INTERNAL_H 1

// Internal structures constants and macros.

// All structures are PRIVATE to Filesysbox framework. 
// No peek/poke is allowed from outside. 
// Consider them black-box objects.
// Etc.

#include <libraries/filesysbox.h>
#include <exec/interrupts.h>
#include <dos/filehandler.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/utility.h>
#include <proto/locale.h>
#include <proto/timer.h>
#include <stdio.h>
#include <stdarg.h>
#include <stddef.h>
#include <SDI/SDI_compiler.h>

#if defined(__AROS__) && defined(__DOS64)
#error "FIXME: Implement DOS64 support"
#endif

#ifdef __GNUC__
#define STATIC_ASSERT(cond,msg) _Static_assert(cond, msg)
#else
#define STATIC_ASSERT(cond,msg)
#endif

#ifdef ENABLE_DP64_SUPPORT
struct DosPacket64
{
	struct Message *dp_Link;
	struct MsgPort *dp_Port;

	LONG dp_Type;
	LONG dp_Res0;

	LONG dp_Res2;
	BYTE dp_pad1[4];
	QUAD dp_Res1;

	LONG dp_Arg1;
	BYTE dp_pad2[4];
	QUAD dp_Arg2;
	LONG dp_Arg3;
	LONG dp_Arg4;
	QUAD dp_Arg5;
};

#define ACTION_CHANGE_FILE_POSITION64 8001
#define ACTION_GET_FILE_POSITION64    8002
#define ACTION_CHANGE_FILE_SIZE64     8003
#define ACTION_GET_FILE_SIZE64        8004
#endif /* ENABLE_DP64_SUPPORT */

#define DOS_OWNER_ROOT 65535
#define DOS_OWNER_NONE 0

#if defined(__AROS__) && defined(AROS_FAST_BPTR)
#define IS_VALID_BPTR(bptr) ((IPTR)(bptr) >= 4096)
#else
#define IS_VALID_BPTR(bptr) (((bptr) & 0xC0000000) == 0 && (bptr) >= 1024)
#endif

#ifndef NEWLIST
#define NEWLIST(list) \
	do { \
		(list)->lh_Head = (struct Node *)&(list)->lh_Tail; \
		(list)->lh_Tail = NULL; \
		(list)->lh_TailPred = (struct Node *)&(list)->lh_Head; \
	} while (0)
#endif
#ifndef NEWMINLIST
#define NEWMINLIST(list) \
	do { \
		(list)->mlh_Head = (struct MinNode *)&(list)->mlh_Tail; \
		(list)->mlh_Tail = NULL; \
		(list)->mlh_TailPred = (struct MinNode *)&(list)->mlh_Head; \
	} while (0)
#endif

#ifndef IsListEmpty
#define IsListEmpty(list) ((list)->lh_TailPred == (struct Node *)(list))
#endif
#ifndef IsMinListEmpty
#define IsMinListEmpty(list) ((list)->mlh_TailPred == (struct MinNode *)(list))
#endif

#define OneInList(list) ((list)->lh_Head == (list)->lh_TailPred)
#define OneInMinList(list) ((list)->mlh_Head == (list)->mlh_TailPred)

#ifndef ZERO
#define ZERO MKBADDR(NULL)
#endif

#ifndef FIBB_HOLD
#define FIBB_HOLD 7
#endif
#ifndef FIBF_HOLD
#define FIBF_HOLD (1<<FIBB_HOLD)
#endif

struct FileSysBoxBase {
    struct Library         libnode;
	UWORD                  pad;
    BPTR                   seglist;
	struct Library        *sysbase;
	struct Library        *dosbase;
	struct Library        *utilitybase;
	struct Library        *localebase;

	struct SignalSemaphore procsema;
	struct Process        *dlproc;
	struct DeviceList     *lhvolume;
	volatile ULONG         dlproc_refcount;
	volatile ULONG         lhproc_refcount;
#ifndef __AROS__
	struct Message         dlproc_startmsg;
	struct Message         lhproc_startmsg;
#endif
};

#ifdef NODEBUG

#define debugf(fmt, ...)
#define vdebugf(fmt, args)

#define DEBUGF(str,args...) ;
#define ADEBUGF(str,args...) ;
#define PDEBUGF(str,args...) ;
#define NDEBUGF(str,args...) ;
#define RDEBUGF(str,args...) ;
#define DIRDEBUGF(str,args...) ;
#define CDEBUGF(str,args...) ;
#define ODEBUGF(str,args...) ;

#else

typedef int (*fbx_putc_cb)(char ch, void *udata);

int FbxDoFmt(fbx_putc_cb cb, void *cb_data, const char *fmt, va_list arg);

int debugf(const char *fmt, ...);
int vdebugf(const char *fmt, va_list args);

// general debug
#ifdef DEBUG
#define DEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define DEBUGF(str,args...) if (fs->dbgflags & FBXDF_GENERAL) debugf("fbx: " str, ## args);
#endif

// API debug
#ifdef ADEBUG
#define ADEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define ADEBUGF(str,args...) /* fs not always available.. */;
#endif

// Packet debug
#ifdef PDEBUG
#define PDEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define PDEBUGF(str,args...) if (fs->dbgflags & FBXDF_PACKETS) debugf("fbx: " str, ## args);
#endif

// Notify debug
#ifdef NDEBUG
#define NDEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define NDEBUGF(str,args...) if (fs->dbgflags & FBXDF_NOTIFY) debugf("fbx: " str, ## args);
#endif

// Resolve debug
#ifdef RDEBUG
#define RDEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define RDEBUGF(str,args...) if (fs->dbgflags & FBXDF_RESOLVE) debugf("fbx: " str, ## args);
#endif

// Dir debug
#ifdef DIRDEBUG
#define DIRDEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define DIRDEBUGF(str,args...) if (fs->dbgflags & FBXDF_DIR) debugf("fbx: " str, ## args);
#endif

// Character conversion debug
#ifdef CDEBUG
#define CDEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define CDEBUGF(str,args...) if (fs->dbgflags & FBXDF_CCONV) debugf("fbx: " str, ## args);
#endif

// FS Operations debug
#ifdef ODEBUG
#define ODEBUGF(str,args...) debugf("fbx: " str, ## args);
#else
#define ODEBUGF(str,args...) if (fs->dbgflags & FBXDF_FSOPS) debugf("fbx: " str, ## args);
#endif

#endif /* NODEBUG */

/****** filesysbox.library/--env-variables-- ********************************
*
*   FBX_DBGFLAGS
*       This environment variable is a hexadecimal mask that enables the
*       various types of debug output in the library.
*
*       To enable all debug output:
*       SetEnv FBX_DBGFLAGS 0xff
*
*       The types of debug output that are currently defined are:
*       0x01 - general
*       0x02 - packets
*       0x04 - notify
*       0x08 - resolve
*       0x10 - dir
*       0x20 - cconv
*       0x40 - fsops
*
****************************************************************************/

#define FBXDB_GENERAL 0
#define FBXDB_PACKETS 1
#define FBXDB_NOTIFY  2
#define FBXDB_RESOLVE 3
#define FBXDB_DIR     4
#define FBXDB_CCONV   5
#define FBXDB_FSOPS   6

#define FBXDF_GENERAL (1 << FBXDB_GENERAL) /* 0x01 */
#define FBXDF_PACKETS (1 << FBXDB_PACKETS) /* 0x02 */
#define FBXDF_NOTIFY  (1 << FBXDB_NOTIFY)  /* 0x04 */
#define FBXDF_RESOLVE (1 << FBXDB_RESOLVE) /* 0x08 */
#define FBXDF_DIR     (1 << FBXDB_DIR)     /* 0x10 */
#define FBXDF_CCONV   (1 << FBXDB_CCONV)   /* 0x20 */
#define FBXDF_FSOPS   (1 << FBXDB_FSOPS)   /* 0x40 */

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define ENTRYHASHSIZE 256
#define ENTRYHASHMASK 255

#define FSOP fs->ops.

#define FBX_MAX_PATH    1024
#define FBX_MAX_NAME    256
#define FBX_MAX_COMMENT 256

#define ACCESS_PERMS (S_IRWXU|S_IRWXG|S_IRWXO) /* 0777 */
#define DEFAULT_PERMS (S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH) /* 0775 */

#ifdef __GNUC__
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) );})
#else
#define container_of(ptr, type, member) ( (type *)( (char *)(ptr) - offsetof(type, member) ) )
#endif

struct FbxEntry {
	struct MinNode hashchain;
	char           path[FBX_MAX_PATH]; // full path to file (and root = "/")
	struct MinList locklist; // list of locks
	struct MinList notifylist; // list of notifys
	BOOL           xlock; // true if exclusively locked
	LONG           type; // ETYPE_XXX
	QUAD           diskkey; // st_ino copy
};

#define FSENTRYFROMHASHCHAIN(chain) container_of(chain, struct FbxEntry, hashchain)

#define ETYPE_NONE 0
#define ETYPE_FILE 1
#define ETYPE_DIR  2

#define OKVOLUME(vol)  ((vol) != NULL && (IPTR)(vol) < (IPTR)-2)
#define BADVOLUME(vol) ((IPTR)(vol) >= (IPTR)-2)
#define NOVOLUME(vol)  ((vol) == NULL)

struct FbxVolume {
	union {
		struct DeviceList dl;
		struct DosList    dol;
	};
	struct MinNode fschain;
	struct FbxFS  *fs;
	IPTR           passkey;
	ULONG          writeprotect; // FbxWriteProtect()
	ULONG          vflags;
	struct MinList unres_notifys;
	struct MinList locklist;
	struct MinList notifylist;
	struct MinList entrytab[ENTRYHASHSIZE]; // hashtable
	UBYTE          volnamelen;
	char           volname[CONN_VOLUME_NAME_BYTES];
};

#define FSVOLUMEFROMFSCHAIN(chain) container_of(chain, struct FbxVolume, fschain)

struct FbxTimerCallbackData {
	struct MinNode       fschain;
	ULONG                lastcall;
	ULONG                period;
	FbxTimerCallbackFunc func;
};

#define FSTIMERCALLBACKDATAFROMFSCHAIN(chain) container_of(chain, struct FbxTimerCallbackData, fschain)

// private for now
/* 
** With the native Filesysbox operations API, the fuse_context is 
** passed directly to all fuse operations as an additional argument.
*/

struct fbx_operations {
	unsigned int flag_nullpath_ok : 1;
	unsigned int flag_reserved : 31;
	STDARGS int (*getattr) (const char *, struct fbx_stat *, struct fuse_context *);
	STDARGS int (*readlink) (const char *, char *, size_t, struct fuse_context *);
	STDARGS int (*mknod) (const char *, mode_t, dev_t, struct fuse_context *);
	STDARGS int (*mkdir) (const char *, mode_t, struct fuse_context *);
	STDARGS int (*unlink) (const char *, struct fuse_context *);
	STDARGS int (*rmdir) (const char *, struct fuse_context *);
	STDARGS int (*symlink) (const char *, const char *, struct fuse_context *);
	STDARGS int (*rename) (const char *, const char *, struct fuse_context *);
	STDARGS int (*link) (const char *, const char *, struct fuse_context *);
	STDARGS int (*chmod) (const char *, mode_t, struct fuse_context *);
	STDARGS int (*chown) (const char *, uid_t, gid_t, struct fuse_context *);
	STDARGS int (*truncate) (const char *, fbx_off_t, struct fuse_context *);
	STDARGS int (*utime) (const char *, struct utimbuf *, struct fuse_context *);
	STDARGS int (*open) (const char *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*read) (const char *, char *, size_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*write) (const char *, const char *, size_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*statfs) (const char *, struct statvfs *, struct fuse_context *);
	STDARGS int (*flush) (const char *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*release) (const char *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*fsync) (const char *, int, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*setxattr) (const char *, const char *, const char *, size_t, int, struct fuse_context *);
	STDARGS int (*getxattr) (const char *, const char *, char *, size_t, struct fuse_context *);
	STDARGS int (*listxattr) (const char *, char *, size_t, struct fuse_context *);
	STDARGS int (*removexattr) (const char *, const char *, struct fuse_context *);
	STDARGS int (*opendir) (const char *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*readdir) (const char *, void *, fuse_fill_dir_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*releasedir) (const char *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*fsyncdir) (const char *, int, struct fuse_file_info *, struct fuse_context *);
	STDARGS void *(*init) (struct fuse_conn_info *conn, struct fuse_context *);
	STDARGS void (*destroy) (void *, struct fuse_context *);
	STDARGS int (*access) (const char *, int, struct fuse_context *);
	STDARGS int (*create) (const char *, mode_t, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*ftruncate) (const char *, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*fgetattr) (const char *, struct fbx_stat *, struct fuse_file_info *, struct fuse_context *);
	STDARGS int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *, struct fuse_context *); /* not used */
	STDARGS int (*utimens) (const char *, const struct timespec tv[2], struct fuse_context *);
	STDARGS int (*bmap) (const char *, size_t blocksize, UQUAD *idx, struct fuse_context *);
	STDARGS int (*format) (const char *, ULONG, struct fuse_context *);
	STDARGS int (*relabel) (const char *, struct fuse_context *);
};

// set by setupvolume, based on struct statvfs flags
#define FBXVF_CASE_SENSITIVE 1
#define FBXVF_READ_ONLY      2

typedef void (*FbxDiskChangeHandlerFunc)(struct FbxFS *fs);

struct FbxDiskChangeHandler {
	FbxDiskChangeHandlerFunc func;
	struct IOStdReq *io;
};

typedef UWORD FbxUCS;

struct FbxAVL {
	struct FbxAVL *left; /* nodes for lower unicodes */
	struct FbxAVL *right; /* nodes for higher unicodes */
	struct FbxAVL *parent;
	FbxUCS         unicode;
	UBYTE          local;
	BYTE           balance; /* > 0 right heavy, < 0 left heavy */
};

struct FbxFS {
	struct FileSysBoxBase       *libbase;
	struct MsgPort              *dlproc_port;
	BPTR                         lhproc_volumebptr;
	struct MsgPort              *lhproc_port;
	struct Library              *sysbase;
	struct Library              *dosbase;
	struct Library              *utilitybase;
	struct Library              *localebase;
	struct Device               *timerbase;
	struct timerequest          *timerio;
	UQUAD                        eclock_initial;
	struct DeviceNode           *devnode;
	struct FileSysStartupMsg    *fssm;
	struct Process              *thisproc;
	struct MsgPort              *fsport;
	struct MsgPort              *notifyreplyport;
	struct SignalSemaphore       fssema;
	APTR                         mempool;
	struct FbxVolume            *currvol;
	struct MinList               volumelist;
	struct fbx_operations        ops; // copy
	struct fuse_context          fcntx; // passed to all operations
	SIPTR                        r2;
	ULONG                        fsflags; // set in SetupFS
	ULONG                        dostype; // set in SetupFS
#ifndef NODEBUG
	ULONG                        dbgflags; // set in SetupFS
	LONG                         dbgflagssig;
#endif
	APTR                         initret;
	LONG                         shutdown; // read by FbxEventLoop()
	LONG                         dosetup; // read by FbxEventLoop()
	LONG                         inhibit;
	ULONG                        aut; // active auto update timeout
	ULONG                        iaut; // inactive auto update timeout
	ULONG                        firstmodify;
	ULONG                        lastmodify;
	LONG                         timerbusy;
	LONG                         diskchangesig;
	struct FbxDiskChangeHandler *diskchangehandler;
	struct fuse_conn_info        conn;
	FbxSignalCallbackFunc        signalcallbackfunc;
	ULONG                        signalcallbacksignals;
	struct MinList               timercallbacklist;
	const char                  *xattr_amiga_comment;
	const char                  *xattr_amiga_protection;
	LONG                         gmtoffset;
#ifdef ENABLE_CHARSET_CONVERSION
	FbxUCS                      *maptable;
	struct FbxAVL               *maptree;
	struct FbxAVL               *avlbuf;
#endif
};

#define FBX_TIMER_MICROS 100000
#define ACTIVE_UPDATE_TIMEOUT_MILLIS 10000
#define INACTIVE_UPDATE_TIMEOUT_MILLIS 500

#define CHECKVOLUME(errbool) \
	if (NOVOLUME(fs->currvol)) { \
		if (fs->inhibit) \
			fs->r2 = ERROR_NOT_A_DOS_DISK; \
		else \
			fs->r2 = ERROR_NO_DISK; \
		return errbool; \
	} else if (BADVOLUME(fs->currvol)) { \
		fs->r2 = ERROR_NOT_A_DOS_DISK; \
		return errbool; \
	}

#define CHECKWRITABLE(errbool) \
	if (fs->currvol->writeprotect || (fs->currvol->vflags & FBXVF_READ_ONLY)) { \
		fs->r2 = ERROR_DISK_WRITE_PROTECTED; \
		return errbool; \
	}

#define CHECKLOCK(lock,errbool) \
	if (!FbxCheckLock(fs, lock)) { \
		fs->r2 = ERROR_INVALID_LOCK; \
		return errbool; \
	}

#define CHECKSTRING(string,errbool) \
	if (!FbxCheckString(fs, string)) { \
		fs->r2 = ERROR_INVALID_COMPONENT_NAME; \
		return errbool; \
	}

struct FbxLock {
	BPTR                   link; // not used (fl_Link)
	IPTR                   diskid; // ino (fl_Key)
	LONG                   access; // exclusive or shared (fl_Access)
	struct MsgPort        *taskmp; // handler task's port (fl_Task)
	BPTR                   volumebptr; // BPTR to DLT_VOLUME DosList entry (fl_Volume)
	struct FbxEntry       *entry;
	struct fuse_file_info *info; // if file and opened
	ULONG                  dostype;
	struct MinNode         entrychain;
	struct MinNode         volumechain;
	struct FbxVolume      *fsvol;
	struct FbxFS          *fs;
	struct FileHandle     *fh;
	APTR                   mempool;
	struct MinList         dirdatalist;
	LONG                   dirscan;
	QUAD                   filepos;
	ULONG                  flags; // LOCKFLAG_XXX
};

STATIC_ASSERT(offsetof(struct FbxLock, link) == offsetof(struct FileLock, fl_Link),
              "offset of fl_Link differs between FbxLock and FileLock.");
STATIC_ASSERT(offsetof(struct FbxLock, diskid) == offsetof(struct FileLock, fl_Key),
              "offset of fl_Key differs between FbxLock and FileLock.");
STATIC_ASSERT(offsetof(struct FbxLock, access) == offsetof(struct FileLock, fl_Access),
              "offset of fl_Access differs between FbxLock and FileLock.");
STATIC_ASSERT(offsetof(struct FbxLock, taskmp) == offsetof(struct FileLock, fl_Task),
              "offset of fl_Task differs between FbxLock and FileLock.");
STATIC_ASSERT(offsetof(struct FbxLock, volumebptr) == offsetof(struct FileLock, fl_Volume),
              "offset of fl_Volume differs between FbxLock and FileLock.");

#define LOCKFLAG_MODIFIED 1

#define FSLOCKFROMENTRYCHAIN(chain) container_of(chain, struct FbxLock, entrychain)
#define FSLOCKFROMVOLUMECHAIN(chain) container_of(chain, struct FbxLock, volumechain)

struct FbxNotifyNode {
	struct MinNode        chain; // either entry->notifylist or fs->unres_notifys for unresolved ones..
	struct MinNode        volumechain;
	struct FbxEntry      *entry; // NULL if not resolved yet.
	struct NotifyRequest *nr;
};

#define nr_notifynode nr_Reserved[0]

#define FSNOTIFYNODEFROMCHAIN(chain_) container_of(chain_, struct FbxNotifyNode, chain)
#define FSNOTIFYNODEFROMVOLUMECHAIN(chain_) container_of(chain_, struct FbxNotifyNode, volumechain)

struct FbxDirData {
	struct MinNode  node;
	char           *fsname;

	/* Only used by ExAll() */
	char           *name;
	char           *comment;

	struct fbx_stat stat;
};

#define FSDIRDATAFROMNODE(chain) container_of(chain, struct FbxDirData, node)

struct FbxExAllState { // exallctrl->lastkey points to this
	struct MinList freelist; // FbxDirData's to free from previous invocation of exall
	LONG           eadsize; // cached value
};

#define AllocStructure(name) (struct name *)AllocMem(sizeof(struct name), MEMF_PUBLIC|MEMF_CLEAR)
#define AllocStructureNoClear(name) (struct name *)AllocMem(sizeof(struct name), MEMF_PUBLIC)
#define FreeStructure(ptr,name) FreeMem((ptr), sizeof(struct name))

#define AllocStructurePooled(pool,name) (struct name *)AllocPooled((pool), sizeof(struct name))
#define FreeStructurePooled(pool,ptr,name) FreePooled((pool), (ptr), sizeof(struct name))

#define AllocFbxEntry(fs) AllocStructurePooled((fs)->mempool, FbxEntry)
#define FreeFbxEntry(fs, e) FreeStructurePooled((fs)->mempool, e, FbxEntry)

#define AllocFbxLock() AllocStructureNoClear(FbxLock)
#define FreeFbxLock(lock) FreeStructure(lock, FbxLock)

#define AllocFbxNotifyNode() AllocStructure(FbxNotifyNode)
#define FreeFbxNotifyNode(nn) FreeStructure(nn, FbxNotifyNode)

#define AllocNotifyMessage() AllocStructure(NotifyMessage)
#define FreeNotifyMessage(nm) FreeStructure(nm, NotifyMessage)

#define AllocFbxFS() AllocStructure(FbxFS)
#define FreeFbxFS(fs) FreeStructure(fs, FbxFS)

#define AllocFbxVolume() AllocStructure(FbxVolume)
#define FreeFbxVolume(vol) FreeStructure(vol, FbxVolume)

#define AllocFbxExAllState(lock) AllocStructurePooled((lock)->mempool, FbxExAllState)
#define FreeFbxExAllState(lock,eas) FreeStructurePooled((lock)->mempool, eas, FbxExAllState)

#define AllocFbxDirData(lock,len) (struct FbxDirData *)AllocVecPooled((lock)->mempool, sizeof(struct FbxDirData) + (len))

#define AllocFuseFileInfo(fs) AllocStructurePooled((fs)->mempool, fuse_file_info)
#define FreeFuseFileInfo(fs,ffi) FreeStructurePooled((fs)->mempool, ffi, fuse_file_info)

#define AllocFbxTimerCallbackData(fs) AllocStructurePooled((fs)->mempool, FbxTimerCallbackData)
#define FreeFbxTimerCallbackData(fs,cb) FreeStructurePooled((fs)->mempool, cb, FbxTimerCallbackData)

#define UNIXTIMEOFFSET 252460800

#define FbxUTF8ToLocal(fs, dst, src, dst_size) utf8_to_local(dst, src, dst_size, (fs)->maptree)
#define FbxLocalToUTF8(fs, dst, src, dst_size) local_to_utf8(dst, src, dst_size, (fs)->maptable)

static inline BOOL IsRoot(const char *path) {
	return (path[0] == '/' && path[1] == '\0');
}

static inline BOOL IsDotOrDotDot(const char *name) {
	return (name[0] == '.' && (name[1] == '\0' || (name[1] == '.' && name[2] == '\0')));
}

/* main/FbxSetupFS.c */
#ifndef NODEBUG
void FbxReadDebugFlags(struct FbxFS *fs);
#endif

/* main/FbxCopyStringBSTRToC.c */
void CopyStringBSTRToC(BSTR bstr, char *cstr, size_t size);

/* main/FbxCopyStringCToBSTR.c */
void CopyStringCToBSTR(const char *cstr, BSTR bstr, size_t size);

/* filesysbox.c */
struct FileSysStartupMsg *FbxGetFSSM(struct Library *sysbase, struct DeviceNode *devnode);
BOOL FbxCheckString(struct FbxFS *fs, const char *str);
size_t FbxStrlen(struct FbxFS *fs, const char *str);
int FbxStrcmp(struct FbxFS *fs, const char *s1, const char *s2);
int FbxStrncmp(struct FbxFS *fs, const char *s1, const char *s2, size_t n);
size_t FbxStrlcpy(struct FbxFS *fs, char *dst, const char *src, size_t dst_size);
size_t FbxStrlcat(struct FbxFS *fs, char *dst, const char *src, size_t dst_size);
unsigned int FbxHashPath(struct FbxFS *fs, const char *str);
struct FbxEntry *FbxFindEntry(struct FbxFS *fs, const char *path);
struct FbxLock *FbxLockEntry(struct FbxFS *fs, struct FbxEntry *e, int mode);
void FbxEndLock(struct FbxFS *fs, struct FbxLock *lock);
void FbxAddEntry(struct FbxFS *fs, struct FbxEntry *e);
BOOL FbxLockName2Path(struct FbxFS *fs, struct FbxLock *lock, const char *name, char *fullpathbuf);
int FbxFuseErrno2Error(int error);
void FbxSetEntryPath(struct FbxFS *fs, struct FbxEntry *e, const char *p);
struct FbxEntry *FbxSetupEntry(struct FbxFS *fs, const char *path, int type, QUAD id);
void FbxCleanupEntry(struct FbxFS *fs, struct FbxEntry *e);
BOOL FbxCheckLock(struct FbxFS *fs, struct FbxLock *lock);
void FbxNotifyDiskChange(struct FbxFS *fs, UBYTE ieclass);
void FbxSetModifyState(struct FbxFS *fs, int state);
BOOL FbxIsParent(struct FbxFS *fs, const char *parent, const char *child);
void FbxTimeSpec2DS(struct FbxFS *fs, const struct timespec *ts, struct DateStamp *ds);
int FbxFlushAll(struct FbxFS *fs);

/* diskchange.c */
struct FbxDiskChangeHandler *FbxAddDiskChangeHandler(struct FbxFS *fs, FbxDiskChangeHandlerFunc func);
void FbxRemDiskChangeHandler(struct FbxFS *fs);

/* timer.c */
struct timerequest *FbxSetupTimerIO(struct FbxFS *fs);
void FbxCleanupTimerIO(struct FbxFS *fs);
void FbxInitUpTime(struct FbxFS *fs);
void _FbxGetUpTime(struct FbxFS *fs, struct timeval *tv);
ULONG FbxGetUpTimeMillis(struct FbxFS *fs);

/* notify.c */
void FbxDoNotifyRequest(struct FbxFS *fs, struct NotifyRequest *nr);
void FbxDoNotifyEntry(struct FbxFS *fs, struct FbxEntry *entry);
void FbxDoNotify(struct FbxFS *fs, const char *path);
void FbxTryResolveNotify(struct FbxFS *fs, struct FbxEntry *e);
void FbxUnResolveNotifys(struct FbxFS *fs, struct FbxEntry *e);

/* dopacket.c */
SIPTR FbxDoPacket(struct FbxFS *fs, struct DosPacket *pkt);

/* dopacket64.c */
#ifdef ENABLE_DP64_SUPPORT
QUAD FbxDoPacket64(struct FbxFS *fs, struct DosPacket64 *pkt);
#endif

/* doslist.c */
struct Process *StartDosListProc(struct FileSysBoxBase *libBase);
int FbxAsyncAddVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRemVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRemFreeVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRenameVolume(struct FbxFS *fs, struct FbxVolume *vol, const char *name);

/* lockhandler.c */
struct DeviceList *StartLockHandlerProc(struct FileSysBoxBase *libBase);
void FbxCollectLock(struct FbxFS *fs, struct FbxLock *lock);
void FbxCollectNotifyNode(struct FbxFS *fs, struct FbxNotifyNode *nn);

/* fsaddnotify.c */
int FbxAddNotify(struct FbxFS *fs, struct NotifyRequest *notify);

/* fschangefileposition.c */
#ifdef ENABLE_DP64_SUPPORT
int FbxChangeFilePosition(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode);
#endif

/* fschangefilesize.c */
#ifdef ENABLE_DP64_SUPPORT
int FbxChangeFileSize(struct FbxFS *fs, struct FbxLock *lock, QUAD offs, int mode);
#endif

/* fschangemode.c */
int FbxChangeMode(struct FbxFS *fs, struct FbxLock *lock, int mode);

/* fsclose.c */
int FbxCloseFile(struct FbxFS *fs, struct FbxLock *lock);

/* fscreatedir.c */
struct FbxLock *FbxCreateDir(struct FbxFS *fs, struct FbxLock *lock, const char *name);

/* fscreatehardlink.c */
int FbxMakeHardLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	struct FbxLock *lock2);

/* fscreatesoftlink.c */
int FbxMakeSoftLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const char *softname);

/* fscurrentvolume.c */
BPTR FbxCurrentVolume(struct FbxFS *fs, struct FbxLock *lock);

/* fsdelete.c */
int FbxDeleteObject(struct FbxFS *fs, struct FbxLock *lock, const char *name);

/* fsdie.c */
int FbxDie(struct FbxFS *fs);

/* fsduplock.c */
struct FbxLock *FbxDupLock(struct FbxFS *fs, struct FbxLock *lock);

/* fsexamineall.c */
int FbxExamineAll(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR len,
	int type, struct ExAllControl *ctrl);

/* fsexamineallend.c */
void FreeFbxDirData(struct FbxLock *lock, struct FbxDirData *dd);
void FreeFbxDirDataList(struct FbxLock *lock, struct MinList *list);
int FbxExamineAllEnd(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, SIPTR len,
	int type, struct ExAllControl *ctrl);

/* fsexaminelock.c */
LONG FbxMode2EntryType(const mode_t mode);
ULONG FbxMode2Protection(const mode_t mode);
UWORD FbxUnix2AmigaOwner(const uid_t owner);
void FbxPathStat2FIB(struct FbxFS *fs, const char *fullpath, struct fbx_stat *stat,
	struct FileInfoBlock *fib);
int FbxExamineLock(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib);

/* fsexaminenext.c */
int FbxReadDir(struct FbxFS *fs, struct FbxLock *lock);
int FbxExamineNext(struct FbxFS *fs, struct FbxLock *lock, struct FileInfoBlock *fib);

/* fsformat.c */
int FbxFormat(struct FbxFS *fs, const char *volname, ULONG dostype);

/* fsgetfileposition.c */
QUAD FbxGetFilePosition(struct FbxFS *fs, struct FbxLock *lock);

/* fsgetfilesize.c */
QUAD FbxGetFileSize(struct FbxFS *fs, struct FbxLock *lock);

/* fsinfodata.c */
int FbxDiskInfo(struct FbxFS *fs, struct InfoData *info);
int FbxInfo(struct FbxFS *fs, struct FbxLock *lock, struct InfoData *info);

/* fsinhibit.c */
int FbxInhibit(struct FbxFS *fs, int inhibit);

/* fslock.c */
struct FbxLock *FbxLocateObject(struct FbxFS *fs, struct FbxLock *lock,
	const char *name, int lockmode);

/* fsopen.c */
int FbxOpenFile(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock,
	const char *name, int mode);

/* fsopenfromlock.c */
int FbxOpenLock(struct FbxFS *fs, struct FileHandle *fh, struct FbxLock *lock);

/* fsparentdir.c */
BOOL FbxParentPath(struct FbxFS *fs, char *pathbuf);
struct FbxLock *FbxLocateParent(struct FbxFS *fs, struct FbxLock *lock);

/* fsread.c */
int FbxReadFile(struct FbxFS *fs, struct FbxLock *lock, APTR buffer, int bytes);

/* fsreadlink.c */
int FbxReadLink(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	char *buffer, int len);

/* fsrelabel.c */
int FbxRelabel(struct FbxFS *fs, const char *volname);

/* fsremovenotify.c */
int FbxRemoveNotify(struct FbxFS *fs, struct NotifyRequest *nr);

/* fsrename.c */
int FbxRenameObject(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	struct FbxLock *lock2, const char *name2);

/* fssamelock.c */
int FbxSameLock(struct FbxFS *fs, struct FbxLock *lock, struct FbxLock *lock2);

/* fsseek.c */
QUAD FbxSeekFile64(struct FbxFS *fs, struct FbxLock *lock, QUAD pos, int mode);
SIPTR FbxSeekFile(struct FbxFS *fs, struct FbxLock *lock, SIPTR pos, int mode);

/* fssetcomment.c */
int FbxSetComment(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const char *comment);

/* fssetdate.c */
int FbxSetDate(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const struct DateStamp *date);

/* fssetfilesize.c */
QUAD FbxSetFileSize64(struct FbxFS *fs, struct FbxLock *lock, QUAD offs, int mode);
SIPTR FbxSetFileSize(struct FbxFS *fs, struct FbxLock *lock, SIPTR offs, int mode);

/* fssetownerinfo.c */
int FbxSetOwnerInfo(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	UWORD uid, UWORD gid);

/* fssetprotection.c */
int FbxSetProtection(struct FbxFS *fs, struct FbxLock *lock, const char *name, ULONG prot);

/* fsunlock.c */
int FbxUnLockObject(struct FbxFS *fs, struct FbxLock *lock);

/* fswrite.c */
int FbxWriteFile(struct FbxFS *fs, struct FbxLock *lock, CONST_APTR buffer, int bytes);

/* fswriteprotect.c */
int FbxWriteProtect(struct FbxFS *fs, int on_off, IPTR passkey);

/* volume.c */
struct FbxVolume *FbxSetupVolume(struct FbxFS *fs);
void FbxCleanupVolume(struct FbxFS *fs);

/* xattrs.c */
ULONG FbxGetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath);
int FbxSetAmigaProtectionFlags(struct FbxFS *fs, const char *fullpath, ULONG prot);
void FbxGetComment(struct FbxFS *fs, const char *fullpath, char *comment, size_t size);

/* utf8.c */
LONG utf8_decode_slow(const char **strp);
LONG utf8_decode_fast(const char **strp);
size_t utf8_strlen(const char *str);
int utf8_stricmp(const char *s1, const char *s2);
int utf8_strnicmp(const char *s1, const char *s2, size_t n);
size_t utf8_strlcpy(char *dst, const char *src, size_t dst_size);
size_t utf8_strlcat(char *dst, const char *src, size_t dst_size);
#ifdef ENABLE_CHARSET_CONVERSION
size_t utf8_to_local(char *dst, const char *src, size_t dst_size, const struct FbxAVL *maptree);
size_t local_to_utf8(char *dst, const char *src, size_t dst_size, const FbxUCS *maptable);
#endif

/* ucs4.c */
ULONG ucs4_toupper(ULONG c);

/* allocvecpooled.c */
#ifndef __AROS__
APTR AllocVecPooled(APTR mempool, ULONG size);
void FreeVecPooled(APTR mempool, APTR ptr);
#endif

/* strlcpy.c */
#ifndef __AROS__
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlcat(char *dst, const char *src, size_t size);
#endif

/* avl.c */
#ifdef ENABLE_CHARSET_CONVERSION
void FbxSetupAVL(struct FbxFS *fs);
#endif

#endif /* FILESYSBOX_INTERNAL_H */

