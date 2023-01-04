/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef FUSE_STUBS_H
#define FUSE_STUBS_H 1

int Fbx_getattr(struct FbxFS *fs, const char *path, struct fbx_stat *stat);
int Fbx_readlink(struct FbxFS *fs, const char *path, char *buf, size_t buflen);
int Fbx_mknod(struct FbxFS *fs, const char *path, mode_t mode, dev_t dev);
int Fbx_mkdir(struct FbxFS *fs, const char *path, mode_t mode);
int Fbx_unlink(struct FbxFS *fs, const char *path);
int Fbx_rmdir(struct FbxFS *fs, const char *path);
int Fbx_symlink(struct FbxFS *fs, const char *dest, const char *path);
int Fbx_rename(struct FbxFS *fs, const char *path, const char *path2);
int Fbx_link(struct FbxFS *fs, const char *dest, const char *path);
int Fbx_statfs(struct FbxFS *fs, const char *name, struct statvfs *stat);
int Fbx_open(struct FbxFS *fs, const char *path, struct fuse_file_info *fi);
int Fbx_release(struct FbxFS *fs, const char *path, struct fuse_file_info *fi);
int Fbx_fsync(struct FbxFS *fs, const char *path, int x, struct fuse_file_info *fi);
int Fbx_readdir(struct FbxFS *fs, const char *path, APTR udata, fuse_fill_dir_t func,
	QUAD offset, struct fuse_file_info *fi);
int Fbx_ftruncate(struct FbxFS *fs, const char *path, QUAD size, struct fuse_file_info *fi);
int Fbx_truncate(struct FbxFS *fs, const char *path, QUAD size);
int Fbx_fgetattr(struct FbxFS *fs, const char *path, struct fbx_stat *stat,
	struct fuse_file_info *fi);
int Fbx_utimens(struct FbxFS *fs, const char *path, const struct timespec *tv);
int Fbx_utime(struct FbxFS *fs, const char *path, struct utimbuf *ubuf);
int Fbx_chmod(struct FbxFS *fs, const char *path, mode_t mode);
int Fbx_chown(struct FbxFS *fs, const char *path, uid_t uid, gid_t gid);
int Fbx_format(struct FbxFS *fs, const char *volname, ULONG dostype);
int Fbx_relabel(struct FbxFS *fs, const char *volname);
int Fbx_create(struct FbxFS *fs, const char *path, mode_t mode, struct fuse_file_info *fi);
int Fbx_opendir(struct FbxFS *fs, const char *path, struct fuse_file_info *fi);
int Fbx_releasedir(struct FbxFS *fs, const char *path, struct fuse_file_info *fi);
int Fbx_setxattr(struct FbxFS *fs, const char *path, const char *attr,
	CONST_APTR buf, size_t len, int flags);
int Fbx_getxattr(struct FbxFS *fs, const char *path, const char *attr,
	APTR buf, size_t len);
int Fbx_removexattr(struct FbxFS *fs, const char *path, const char *attr);

#endif /* FUSE_STUBS_H */

