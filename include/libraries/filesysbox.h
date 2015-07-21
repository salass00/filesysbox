/*********************************************************************/
/********** Public structures and definitions of Filesysbox **********/
/*********************************************************************/

#ifndef LIBRARIES_FILESYSBOX_H
#define LIBRARIES_FILESYSBOX_H

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

#ifndef __AROS__
typedef ULONG IPTR;
typedef LONG  SIPTR;
#endif

typedef QUAD fbx_off_t;

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

#ifndef S_IREAD
#define S_IREAD  0400
#define S_IWRITE 0200
#define S_IEXEC  0100
#endif

#define XATTR_CREATE  0x1 /* set the value, fail if attr already exists */
#define XATTR_REPLACE 0x2 /* set the value, fail if attr does not exist */

#define FILESYSBOX_VERSION 53
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

// tags for FbxSetupFS()
#define FBXT_FSFLAGS                 (TAG_USER + 1)
#define FBXT_FSSM                    (TAG_USER + 2)
#define FBXT_DOSTYPE                 (TAG_USER + 3)
#define FBXT_GET_CONTEXT             (TAG_USER + 4)
#define FBXT_ACTIVE_UPDATE_TIMEOUT   (TAG_USER + 5) // default: 10000 ms
#define FBXT_INACTIVE_UPDATE_TIMEOUT (TAG_USER + 6) // default: 500 ms

typedef int (*fuse_fill_dir_t) (void *udata, const char *fsname, const struct fbx_stat *stbuf, fbx_off_t off);

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
	int (*getattr) (const char *, struct fbx_stat *);
	int (*readlink) (const char *, char *, size_t);
	int (*mknod) (const char *, mode_t, dev_t);
	int (*mkdir) (const char *, mode_t);
	int (*unlink) (const char *);
	int (*rmdir) (const char *);
	int (*symlink) (const char *, const char *);
	int (*rename) (const char *, const char *);
	int (*link) (const char *, const char *);
	int (*chmod) (const char *, mode_t);
	int (*chown) (const char *, uid_t, gid_t);
	int (*truncate) (const char *, fbx_off_t);
	int (*utime) (const char *, struct utimbuf *);
	int (*open) (const char *, struct fuse_file_info *);
	int (*read) (const char *, char *, size_t, fbx_off_t, struct fuse_file_info *);
	int (*write) (const char *, const char *, size_t, fbx_off_t, struct fuse_file_info *);
	int (*statfs) (const char *, struct statvfs *);
	int (*flush) (const char *, struct fuse_file_info *);
	int (*release) (const char *, struct fuse_file_info *);
	int (*fsync) (const char *, int, struct fuse_file_info *);
	int (*setxattr) (const char *, const char *, const char *, size_t, int);
	int (*getxattr) (const char *, const char *, char *, size_t);
	int (*listxattr) (const char *, char *, size_t);
	int (*removexattr) (const char *, const char *);
	int (*opendir) (const char *, struct fuse_file_info *);
	int (*readdir) (const char *, void *, fuse_fill_dir_t, fbx_off_t, struct fuse_file_info *);
	int (*releasedir) (const char *, struct fuse_file_info *);
	int (*fsyncdir) (const char *, int, struct fuse_file_info *);
	void *(*init) (struct fuse_conn_info *conn);
	void (*destroy) (void *);
	int (*access) (const char *, int);
	int (*create) (const char *, mode_t, struct fuse_file_info *);
	int (*ftruncate) (const char *, fbx_off_t, struct fuse_file_info *);
	int (*fgetattr) (const char *, struct fbx_stat *, struct fuse_file_info *);
	int (*lock) (const char *, struct fuse_file_info *, int cmd, struct flock *);
	int (*utimens) (const char *, const struct timespec tv[2]);
	int (*bmap) (const char *, size_t blocksize, UQUAD *idx);
	int (*format) (const char *, ULONG);
	int (*relabel) (const char *);
};

typedef void (*FbxSignalCallbackFunc)(ULONG matching_signals);
typedef void (*FbxTimerCallbackFunc)(void);

struct FbxTimerCallbackData;

#ifdef __cplusplus
}
#endif

#endif

