/*********************************************************************/
/********** Public structures and definitions of Filesysbox **********/
/*********************************************************************/

#ifndef LIBRARIES_FILESYSBOX_H
#define LIBRARIES_FILESYSBOX_H

#ifndef DEVICES_TIMER_H
#include <devices/timer.h>
#endif

#ifdef __VBCC__
#include <stddef.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <utime.h>
#include <fcntl.h>
#endif
#include <sys/statvfs.h>

#ifndef UTILITY_TAGITEM_H
#include <utility/tagitem.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef STDARGS
#if !defined(__AROS__) && defined(__GNUC__)
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

#ifdef __VBCC__
typedef unsigned int mode_t;
typedef unsigned short nlink_t;
typedef int uid_t;
typedef int gid_t;
typedef int pid_t;
typedef unsigned long dev_t;

#define S_IFMT  0170000
#define S_IFREG 0100000
#define S_IFLNK 0120000
#define S_IFDIR 0040000

#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)

#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001

#define S_IRWXU (S_IRUSR|S_IWUSR|S_IXUSR)
#define S_IRWXG (S_IRGRP|S_IWGRP|S_IXGRP)
#define S_IRWXO (S_IROTH|S_IWOTH|S_IXOTH)

struct timespec {
	time_t tv_sec;
	long   tv_nsec;
};

#define O_RDONLY  0x0000
#define O_WRONLY  0x0001
#define O_RDWR    0x0002
#define O_ACCMODE 0x0003

struct utimbuf {
	time_t actime;
	time_t modtime;
};
#endif /* __VBCC__ */

struct fbx_stat {
	mode_t          st_mode;
	UQUAD           st_ino;
	nlink_t         st_nlink;
	uid_t           st_uid;
	gid_t           st_gid;
	dev_t           st_rdev;
	struct timespec st_atim;
	struct timespec st_mtim;
	struct timespec st_ctim;
	fbx_off_t       st_size;
	QUAD            st_blocks;
	LONG            st_blksize;
};

#ifndef st_atime
#define st_atime st_atim.tv_sec
#endif
#ifndef st_atimensec
#define st_atimensec st_atim.tv_nsec
#endif
#ifndef st_mtime
#define st_mtime st_mtim.tv_sec
#endif
#ifndef st_mtimensec
#define st_mtimensec st_mtim.tv_nsec
#endif
#ifndef st_ctime
#define st_ctime st_ctim.tv_sec
#endif
#ifndef st_ctimensec
#define st_ctimensec st_ctim.tv_nsec
#endif

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

