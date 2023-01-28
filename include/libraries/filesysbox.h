/*********************************************************************/
/********** Public structures and definitions of Filesysbox **********/
/*********************************************************************/

#ifndef LIBRARIES_FILESYSBOX_H
#define LIBRARIES_FILESYSBOX_H

#ifndef DEVICES_TIMER_H
#include <devices/timer.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <utime.h>
#include <fcntl.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDARGS
#ifdef __GNUC__
#define STDARGS __stdargs
#else
#define STDARGS
#endif
#endif /* !STDARGS */

#if !defined(__AROS__) && !defined(AROS_TYPES_DEFINED)
#define AROS_TYPES_DEFINED
typedef ULONG              IPTR;
typedef LONG               SIPTR;
typedef unsigned long long UQUAD;
typedef signed long long   QUAD;
#endif

typedef QUAD fbx_off_t;

#ifdef __AROS__
#undef st_atime
#undef st_mtime
#undef st_ctime
#endif

struct fbx_stat {
	mode_t          st_mode;
	UQUAD           st_ino;
	nlink_t         st_nlink;
	uid_t           st_uid;
	gid_t           st_gid;
	dev_t           st_rdev;
	union {
		struct {
			time_t       st_atime;
			unsigned int st_atimensec;
		};
		struct timespec  st_atim;
	};
	union {
		struct {
			time_t       st_mtime;
			unsigned int st_mtimensec;
		};
		struct timespec  st_mtim;
	};
	union {
		struct {
			time_t       st_ctime;
			unsigned int st_ctimensec;
		};
		struct timespec  st_ctim;
	};
	fbx_off_t       st_size;
	QUAD            st_blocks;
	LONG            st_blksize;
};

#ifndef S_IREAD
#define S_IREAD  0400
#define S_IWRITE 0200
#define S_IEXEC  0100
#endif

#define XATTR_CREATE  0x1 /* set the value, fail if attr already exists */
#define XATTR_REPLACE 0x2 /* set the value, fail if attr does not exist */

#define FILESYSBOX_VERSION 54
#define FILESYSBOX_NAME "filesysbox.library"

#define FUSE_VERSION 26

#define FBXQMM_MOUNT_NAME 1
#define FBXQMM_MOUNT_CONTROL 2
#define FBXQMM_FSSM 3
#define FBXQMM_ENVIRON 4

#define CONN_VOLUME_NAME_BYTES 128

struct fuse_conn_info {
	/* not used yet, just cleared */
	unsigned proto_major;
	unsigned proto_minor;
	unsigned async_read;
	unsigned max_write;
	unsigned max_readahead;
	unsigned reserved[27];
	/* filesysbox additions */
	char     volume_name[CONN_VOLUME_NAME_BYTES]; // for .init() to fill 
};

// filesysbox sets flags and reads nonseekable for now. 
// rest is cleared and untouched. "fh_old" and "fh" are
// safe to be poked by FS.
struct fuse_file_info {
	int flags;
	unsigned long fh_old;
	int writepage;
	unsigned int direct_io : 1;
	unsigned int keep_cache : 1;
	unsigned int flush : 1;
	unsigned int nonseekable : 1; // fuse 2.9
	unsigned int padding : 28;
	long long fh;
	long long lock_owner; /* not used */
};

// flags  for SFST_FSFLAGS
#define FBXF_ENABLE_UTF8_NAMES            1 // set to enable utf8 names for FS
#define FBXF_ENABLE_DISK_CHANGE_DETECTION 2 // set to enable disk change detection
#define FBXF_USE_INO                      8 // (V54) filesystem sets st_ino
#define FBXF_USE_FILL_DIR_STAT            16 // (V54) valid stat data passed to readdir() callback

// tags for FbxSetupFS()
#define FBXT_FSFLAGS                 (TAG_USER + 1)
#define FBXT_FSSM                    (TAG_USER + 2)
#define FBXT_DOSTYPE                 (TAG_USER + 3)
#define FBXT_GET_CONTEXT             (TAG_USER + 4)
#define FBXT_ACTIVE_UPDATE_TIMEOUT   (TAG_USER + 5) // default: 10000 ms
#define FBXT_INACTIVE_UPDATE_TIMEOUT (TAG_USER + 6) // default: 500 ms

/* tags for FbxQueryFS() */
#define FBXT_GMT_OFFSET              (TAG_USER + 101) /* equivalent to TZA_UTCOffset */

typedef STDARGS int (*fuse_fill_dir_t) (void *udata, const char *fsname, const struct fbx_stat *stbuf, fbx_off_t off);

typedef void *fuse_dirfil_t;

#define fuse_get_context() _fuse_context_
#define fuse_version() (int)FbxFuseVersion()
#define fuse_new(msg,tags,ops,opssize,udata) FbxSetupFS(msg,tags,ops,opssize,udata)
#define fuse_loop(fs) FbxEventLoop(fs)
#define fuse_destroy(fs) FbxCleanupFS(fs)

struct FbxFS;

struct fuse_context {
	// return value of FbxSetupFS()
	struct FbxFS *fuse; 
   
	// these are all zero for now, but may change in future
	uid_t uid;
	gid_t gid;
	pid_t pid;
   
	// user_data argument of FbxSetupFS()
	void *private_data; 
};

struct fuse_operations {
	unsigned int flag_nullpath_ok : 1;
	unsigned int flag_reserved : 31;
	STDARGS int (*getattr) (const char *, struct fbx_stat *);
	STDARGS int (*readlink) (const char *, char *, size_t);
	STDARGS int (*mknod) (const char *, mode_t, dev_t);
	STDARGS int (*mkdir) (const char *, mode_t);
	STDARGS int (*unlink) (const char *);
	STDARGS int (*rmdir) (const char *);
	STDARGS int (*symlink) (const char *, const char *);
	STDARGS int (*rename) (const char *, const char *);
	STDARGS int (*link) (const char *, const char *);
	STDARGS int (*chmod) (const char *, mode_t);
	STDARGS int (*chown) (const char *, uid_t, gid_t);
	STDARGS int (*truncate) (const char *, fbx_off_t);
	STDARGS int (*utime) (const char *, struct utimbuf *);
	STDARGS int (*open) (const char *, struct fuse_file_info *);
	STDARGS int (*read) (const char *, char *, size_t, fbx_off_t, struct fuse_file_info *);
	STDARGS int (*write) (const char *, const char *, size_t, fbx_off_t, struct fuse_file_info *);
	STDARGS int (*statfs) (const char *, struct statvfs *);
	STDARGS int (*flush) (const char *, struct fuse_file_info *);
	STDARGS int (*release) (const char *, struct fuse_file_info *);
	STDARGS int (*fsync) (const char *, int, struct fuse_file_info *);
	STDARGS int (*setxattr) (const char *, const char *, const char *, size_t, int);
	STDARGS int (*getxattr) (const char *, const char *, char *, size_t);
	STDARGS int (*listxattr) (const char *, char *, size_t);
	STDARGS int (*removexattr) (const char *, const char *);
	STDARGS int (*opendir) (const char *, struct fuse_file_info *);
	STDARGS int (*readdir) (const char *, void *, fuse_fill_dir_t, fbx_off_t, struct fuse_file_info *);
	STDARGS int (*releasedir) (const char *, struct fuse_file_info *);
	STDARGS int (*fsyncdir) (const char *, int, struct fuse_file_info *);
	STDARGS void *(*init) (struct fuse_conn_info *conn);
	STDARGS void (*destroy) (void *);
	STDARGS int (*access) (const char *, int);
	STDARGS int (*create) (const char *, mode_t, struct fuse_file_info *);
	STDARGS int (*ftruncate) (const char *, fbx_off_t, struct fuse_file_info *);
	STDARGS int (*fgetattr) (const char *, struct fbx_stat *, struct fuse_file_info *);
	STDARGS int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *);
	STDARGS int (*utimens) (const char *, const struct timespec tv[2]);
	STDARGS int (*bmap) (const char *, size_t blocksize, UQUAD *idx);
	STDARGS int (*format) (const char *, ULONG);
	STDARGS int (*relabel) (const char *);
};

typedef STDARGS void (*FbxSignalCallbackFunc)(ULONG matching_signals);
typedef STDARGS void (*FbxTimerCallbackFunc)(void);

struct FbxTimerCallbackData;

#ifdef __cplusplus
}
#endif

#endif

