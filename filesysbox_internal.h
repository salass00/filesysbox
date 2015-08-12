/*********************************************************************/
/* Filesysbox filesystem layer/framework *****************************/
/*********************************************************************/
/* Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net] ******/
/* Copyright (c) 2013-2015 Fredrik Wikstrom [fredrik a500 org] *******/
/*********************************************************************/ 
/* This library is released under AROS PUBLIC LICENSE 1.1 ************/
/* See the file LICENSE.APL ******************************************/
/*********************************************************************/

// Internal structures constants and macros.

// All structures are PRIVATE to Filesysbox framework. 
// No peek/poke is allowed from outside. 
// Consider them black-box objects.
// Etc.

#include <libraries/filesysbox.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/intuition.h>
#include <proto/utility.h>
#include <proto/timer.h>
#ifdef __AROS__
#include <aros/arossupportbase.h>
#endif
#include <stdio.h>
#include <SDI/SDI_compiler.h>

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
    struct Library libnode;
    BPTR seglist;
	struct Library *sysbase;
	struct Library *dosbase;
	struct Process *dlproc;
	ULONG dlproc_refcount;
	struct SignalSemaphore dlproc_sem;
};

#ifdef __AROS__
extern struct AROSSupportBase *DebugAROSBase;
#define kprintf  (DebugAROSBase->kprintf)
#define vkprintf (DebugAROSBase->vkprintf)
#endif

int debugf(const char *fmt, ...);
int vdebugf(const char *fmt, va_list args);

#ifdef NODEBUG

#define DEBUGF(str,args...) ;
#define ADEBUGF(str,args...) ;
#define PDEBUGF(str,args...) ;
#define NDEBUGF(str,args...) ;
#define RDEBUGF(str,args...) ;
#define DIRDEBUGF(str,args...) ;
#define CDEBUGF(str,args...) ;
#define ODEBUGF(str,args...) ;

#else

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

#define FBXDB_GENERAL 0
#define FBXDB_PACKETS 1
#define FBXDB_NOTIFY  2
#define FBXDB_RESOLVE 3
#define FBXDB_DIR     4
#define FBXDB_CCONV   5
#define FBXDB_FSOPS   6

#define FBXDF_GENERAL (1 << FBXDB_GENERAL)
#define FBXDF_PACKETS (1 << FBXDB_PACKETS)
#define FBXDF_NOTIFY  (1 << FBXDB_NOTIFY)
#define FBXDF_RESOLVE (1 << FBXDB_RESOLVE)
#define FBXDF_DIR     (1 << FBXDB_DIR)
#define FBXDF_CCONV   (1 << FBXDB_CCONV)
#define FBXDF_FSOPS   (1 << FBXDB_FSOPS)

#define min(x,y) ((x)<(y)?(x):(y))
#define max(x,y) ((x)>(y)?(x):(y))

#define ENTRYHASHSIZE 256
#define ENTRYHASHMASK 255

#define FSOP fs->ops.

#define MAXPATHLEN 1024

#define ACCESS_PERMS (S_IRWXU|S_IRWXG|S_IRWXO) /* 0777 */
#define DEFAULT_PERMS (S_IRWXU) /* 0700 */

#ifdef __GNUC__
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - offsetof(type, member) );})
#else
#define container_of(ptr, type, member) ( (type *)( (char *)(ptr) - offsetof(type, member) ) )
#endif

struct FbxEntry {
	struct MinNode hashchain;
	char           path[MAXPATHLEN]; // full path to file (and root = "/")
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
	QUAD                 lastcall;
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
	int (*getattr) (const char *, struct fbx_stat *, struct fuse_context *);
	int (*readlink) (const char *, char *, size_t, struct fuse_context *);
	int (*mknod) (const char *, mode_t, dev_t, struct fuse_context *);
	int (*mkdir) (const char *, mode_t, struct fuse_context *);
	int (*unlink) (const char *, struct fuse_context *);
	int (*rmdir) (const char *, struct fuse_context *);
	int (*symlink) (const char *, const char *, struct fuse_context *);
	int (*rename) (const char *, const char *, struct fuse_context *);
	int (*link) (const char *, const char *, struct fuse_context *);
	int (*chmod) (const char *, mode_t, struct fuse_context *);
	int (*chown) (const char *, uid_t, gid_t, struct fuse_context *);
	int (*truncate) (const char *, fbx_off_t, struct fuse_context *);
	int (*utime) (const char *, struct utimbuf *, struct fuse_context *);
	int (*open) (const char *, struct fuse_file_info *, struct fuse_context *);
	int (*read) (const char *, char *, size_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	int (*write) (const char *, const char *, size_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	int (*statfs) (const char *, struct statvfs *, struct fuse_context *);
	int (*flush) (const char *, struct fuse_file_info *, struct fuse_context *);
	int (*release) (const char *, struct fuse_file_info *, struct fuse_context *);
	int (*fsync) (const char *, int, struct fuse_file_info *, struct fuse_context *);
	int (*setxattr) (const char *, const char *, const char *, size_t, int, struct fuse_context *);
	int (*getxattr) (const char *, const char *, char *, size_t, struct fuse_context *);
	int (*listxattr) (const char *, char *, size_t, struct fuse_context *);
	int (*removexattr) (const char *, const char *, struct fuse_context *);
	int (*opendir) (const char *, struct fuse_file_info *, struct fuse_context *);
	int (*readdir) (const char *, void *, fuse_fill_dir_t, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	int (*releasedir) (const char *, struct fuse_file_info *, struct fuse_context *);
	int (*fsyncdir) (const char *, int, struct fuse_file_info *, struct fuse_context *);
	void *(*init) (struct fuse_conn_info *conn, struct fuse_context *);
	void (*destroy) (void *, struct fuse_context *);
	int (*access) (const char *, int, struct fuse_context *);
	int (*create) (const char *, mode_t, struct fuse_file_info *, struct fuse_context *);
	int (*ftruncate) (const char *, fbx_off_t, struct fuse_file_info *, struct fuse_context *);
	int (*fgetattr) (const char *, struct fbx_stat *, struct fuse_file_info *, struct fuse_context *);
	int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *, struct fuse_context *); /* not used */
	int (*utimens) (const char *, const struct timespec tv[2], struct fuse_context *);
	int (*bmap) (const char *, size_t blocksize, UQUAD *idx, struct fuse_context *);
	int (*format) (const char *, ULONG, struct fuse_context *);
	int (*relabel) (const char *, struct fuse_context *);
};

// set by setupvolume, based on struct statvfs flags
#define FBXVF_CASE_SENSITIVE 1
#define FBXVF_READ_ONLY      2

typedef void (*FbxDiskChangeHandlerFunc)(struct FbxFS *fs);

struct FbxDiskChangeHandler {
	FbxDiskChangeHandlerFunc func;
	struct IOStdReq *io;
};

struct FbxFS {
	struct FileSysBoxBase       *libbase;
	struct MsgPort              *dlproc_port;
	struct Library              *sysbase;
	struct Library              *dosbase;
	struct Library              *intuitionbase;
	struct Library              *utilitybase;
	struct Device               *timerbase;
	struct timerequest          *timerio;
	UQUAD                        eclock_initial;
	struct DeviceNode           *devnode;
	struct FileSysStartupMsg    *fssm;
	struct Process              *thisproc;
	struct MsgPort              *fsport;
	struct MsgPort              *notifyreplyport;
	APTR                         mempool;
	struct FbxVolume            *currvol;
	struct MinList               volumelist;
	struct fbx_operations        ops; // copy
	struct fuse_context          fcntx; // passed to all operations
	SIPTR                        r2;
	ULONG                        fsflags; // set in SetupFS
	ULONG                        dostype; // set in SetupFS
	ULONG                        dbgflags; // set in SetupFS
	APTR                         initret;
	LONG                         shutdown; // read by FbxEventLoop()
	LONG                         dosetup; // read by FbxEventLoop()
	LONG                         inhibit;
	ULONG                        aut; // active auto update timeout
	ULONG                        iaut; // inactive auto update timeout
	QUAD                         firstmodify;
	QUAD                         lastmodify;
	LONG                         timerbusy;
	LONG                         dbgflagssig;
	LONG                         diskchangesig;
	struct FbxDiskChangeHandler *diskchangehandler;
	struct fuse_conn_info        conn;
	FbxSignalCallbackFunc        signalcallbackfunc;
	ULONG                        signalcallbacksignals;
	struct MinList               timercallbacklist;
	const char                  *xattr_amiga_comment;
	const char                  *xattr_amiga_protection;
	char                         pathbuf[3][MAXPATHLEN];
};

#define FBX_TIMER_MICROS 100000
#define ACTIVE_UPDATE_TIMEOUT_MILLIS 10000
#define INACTIVE_UPDATE_TIMEOUT_MILLIS 500

#define GetSysBase struct Library *SysBase = fs->sysbase;
#define GetDOSBase struct Library *DOSBase = fs->dosbase;
#define GetIntuitionBase struct Library *IntuitionBase = fs->intuitionbase;
#define GetUtilityBase struct Library *UtilityBase = fs->utilitybase;
#define GetTimerBase struct Device *TimerBase = fs->timerbase;

#define CHECKVOLUME(errbool) \
	if (NOVOLUME(fs->currvol)) { \
		if (fs->inhibit) \
			fs->r2 = ERROR_OBJECT_IN_USE; \
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
	BPTR                   link; // not used
	IPTR                   diskid;
	LONG                   access; // exclusive or shared
	struct MsgPort        *taskmp; // handler task's port
	BPTR                   volumebptr; // BPTR to DLT_VOLUME DosList entry
	struct FbxEntry       *entry;
	struct fuse_file_info *info; // if file and opened
	ULONG                  dostype;
	struct MinNode         entrychain;
	struct MinNode         volumechain;
	struct FbxVolume      *fsvol;
	struct FbxFS          *fs;
	struct FileHandle     *fh;
	struct MinList         dirdatalist;
	LONG                   dirscan;
	QUAD                   filepos;
	ULONG                  flags; // LOCKFLAG_XXX
};

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

struct FbxDirData {
	struct MinNode node;
	char           name[]; // variable length nil-term array
};

struct FbxExAllState { // exallctrl->lastkey points to this
	struct MinList freelist; // FbxDirData's to free from previous invocation of exall
	LONG           iptrs; // cached value
};

#define AllocStructure(name) (struct name *)AllocMem(sizeof(struct name), MEMF_PUBLIC|MEMF_CLEAR)
#define AllocStructureNoClear(name) (struct name *)AllocMem(sizeof(struct name), MEMF_PUBLIC)
#define FreeStructure(ptr,name) FreeMem((ptr), sizeof(struct name))

#define AllocStructurePooled(fs,name) (struct name *)AllocPooled((fs)->mempool, sizeof(struct name))
#define FreeStructurePooled(fs,ptr,name) FreePooled((fs)->mempool, (ptr), sizeof(struct name))

#define AllocFbxEntry(fs) AllocStructurePooled(fs, FbxEntry)
#define FreeFbxEntry(fs, e) FreeStructurePooled(fs, e, FbxEntry)

#define AllocFbxLock(fs) AllocStructurePooled(fs, FbxLock)
#define FreeFbxLock(fs, lock) FreeStructurePooled(fs, lock, FbxLock)

#define AllocFbxNotifyNode() AllocStructure(FbxNotifyNode)
#define FreeFbxNotifyNode(nn) FreeStructure(nn, FbxNotifyNode)

#define AllocNotifyMessage() AllocStructure(NotifyMessage)
#define FreeNotifyMessage(nm) FreeStructure(nm, NotifyMessage)

#define AllocFbxFS() AllocStructure(FbxFS)
#define FreeFbxFS(fs) FreeStructure(fs, FbxFS)

#define AllocFbxVolume() AllocStructure(FbxVolume)
#define FreeFbxVolume(vol) FreeStructure(vol, FbxVolume)

#define AllocFbxExAllState(fs) AllocStructurePooled(fs, FbxExAllState)
#define FreeFbxExAllState(fs,eas) FreeStructurePooled(fs, eas, FbxExAllState)

#ifdef __AROS__
#define AllocFbxDirData(fs,len) (struct FbxDirData *)AllocVecPooled((fs)->mempool, sizeof(struct FbxDirData) + (len))
#define FreeFbxDirData(fs,dd) FreeVecPooled((fs)->mempool, dd)
#else
#define AllocFbxDirData(fs,len) (struct FbxDirData *)FbxAllocVecPooled((fs), sizeof(struct FbxDirData) + (len))
#define FreeFbxDirData(fs,dd) FbxFreeVecPooled((fs), (dd))
#endif

#define AllocFuseFileInfo(fs) AllocStructurePooled(fs, fuse_file_info)
#define FreeFuseFileInfo(fs,ffi) FreeStructurePooled(fs, ffi, fuse_file_info)

#define AllocFbxTimerCallbackData(fs) AllocStructurePooled(fs, FbxTimerCallbackData)
#define FreeFbxTimerCallbackData(fs,cb) FreeStructurePooled(fs, cb, FbxTimerCallbackData)

#define UNIXTIMEOFFSET 252460800

#define OneInMinList(list) ((list)->mlh_Head == (list)->mlh_TailPred)

/* filesysbox.c */
void CopyStringBSTRToC(BSTR bstr, char *cstr, size_t size);
void CopyStringCToBSTR(const char *cstr, BSTR bstr, size_t size);
struct FbxDiskChangeHandler *FbxAddDiskChangeHandler(struct FbxFS *fs, FbxDiskChangeHandlerFunc func);
void FbxRemDiskChangeHandler(struct FbxFS *fs);
BOOL FbxCheckString(struct FbxFS *fs, const char *str);
size_t FbxStrlen(struct FbxFS *fs, const char *str);
int FbxStrcmp(struct FbxFS *fs, const char *s1, const char *s2);
int FbxStrncmp(struct FbxFS *fs, const char *s1, const char *s2, size_t n);
size_t FbxStrlcpy(struct FbxFS *fs, char *dst, const char *src, size_t dst_size);
size_t FbxStrlcat(struct FbxFS *fs, char *dst, const char *src, size_t dst_size);
void FbxCloseLibraries(struct FbxFS *fs);
struct timerequest *FbxSetupTimerIO(struct FbxFS *fs);
void FbxCleanupTimerIO(struct FbxFS *fs);
void FbxStopTimer(struct FbxFS *fs);
void FbxStartTimer(struct FbxFS *fs);
QUAD FbxGetUpTimeMillis(struct FbxFS *fs);
struct FbxVolume *FbxSetupVolume(struct FbxFS *fs);
void FbxCleanupVolume(struct FbxFS *fs);
void FbxHandlePackets(struct FbxFS *fs);
void FbxHandleNotifyReplies(struct FbxFS *fs);
void FbxHandleTimerEvent(struct FbxFS *fs);
void FbxHandleUserEvent(struct FbxFS *fs, ULONG signals);

/* uptime.c */
void FbxInitUpTime(struct FbxFS *fs);
void FbxGetUpTime(struct FbxFS *fs, struct timeval *tv);

/* doslist.c */
struct Process *StartDosListProc(struct FileSysBoxBase *libBase);
int FbxAsyncAddVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRemVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRemFreeVolume(struct FbxFS *fs, struct FbxVolume *vol);
int FbxAsyncRenameVolume(struct FbxFS *fs, struct FbxVolume *vol, const char *name);

/* utf8.c */
LONG utf8_decode_slow(const char **strp);
LONG utf8_decode_fast(const char **strp);
size_t utf8_strlen(const char *str);
int utf8_stricmp(const char *s1, const char *s2);
int utf8_strnicmp(const char *s1, const char *s2, size_t n);
size_t utf8_strlcpy(char *dst, const char *src, size_t dst_size);
size_t utf8_strlcat(char *dst, const char *src, size_t dst_size);

/* ucs4.c */
ULONG ucs4_toupper(ULONG c);

/* allocvecpooled.c */
#ifndef __AROS__
APTR FbxAllocVecPooled(struct FbxFS *fs, ULONG size);
void FbxFreeVecPooled(struct FbxFS *fs, APTR ptr);
#endif

