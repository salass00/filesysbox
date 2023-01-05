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
int Fbx_symlink(struct FbxFS *fs, const char *dest, const char *path);
int Fbx_link(struct FbxFS *fs, const char *dest, const char *path);
int Fbx_statfs(struct FbxFS *fs, const char *name, struct statvfs *stat);
int Fbx_release(struct FbxFS *fs, const char *path, struct fuse_file_info *fi);
int Fbx_fsync(struct FbxFS *fs, const char *path, int x, struct fuse_file_info *fi);
int Fbx_fgetattr(struct FbxFS *fs, const char *path, struct fbx_stat *stat,
	struct fuse_file_info *fi);
int Fbx_utimens(struct FbxFS *fs, const char *path, const struct timespec *tv);
int Fbx_utime(struct FbxFS *fs, const char *path, struct utimbuf *ubuf);
int Fbx_chmod(struct FbxFS *fs, const char *path, mode_t mode);
int Fbx_setxattr(struct FbxFS *fs, const char *path, const char *attr,
	CONST_APTR buf, size_t len, int flags);
int Fbx_getxattr(struct FbxFS *fs, const char *path, const char *attr,
	APTR buf, size_t len);
int Fbx_removexattr(struct FbxFS *fs, const char *path, const char *attr);

#endif /* FUSE_STUBS_H */

