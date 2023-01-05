/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"

int Fbx_getattr(struct FbxFS *fs, const char *path, struct fbx_stat *stat)
{
	ODEBUGF("Fbx_getattr(%#p, '%s', %#p)\n", fs, path, stat);

	return FSOP getattr(path, stat, &fs->fcntx);
}

int Fbx_statfs(struct FbxFS *fs, const char *name, struct statvfs *stat)
{
	ODEBUGF("Fbx_statfs(%#p, '%s', %#p)\n", fs, name, stat);

	return FSOP statfs(name, stat, &fs->fcntx);
}

int Fbx_release(struct FbxFS *fs, const char *path, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_release(%#p, '%s', %#p)\n", fs, path, fi);

	return FSOP release(path, fi, &fs->fcntx);
}

int Fbx_fsync(struct FbxFS *fs, const char *path, int x, struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_fsync(%#p, '%s', %d, %#p)\n", fs, path, x, fi);

	return FSOP fsync(path, x, fi, &fs->fcntx);
}

int Fbx_fgetattr(struct FbxFS *fs, const char *path, struct fbx_stat *stat,
	struct fuse_file_info *fi)
{
	ODEBUGF("Fbx_fgetattr(%#p, '%s', %#p, %#p)\n", fs, path, stat, fi);

	return FSOP fgetattr(path, stat, fi, &fs->fcntx);
}

int Fbx_utimens(struct FbxFS *fs, const char *path, const struct timespec *tv)
{
	ODEBUGF("Fbx_utimens(%#p, '%s', %#p)\n", fs, path, tv);

	return FSOP utimens(path, tv, &fs->fcntx);
}

int Fbx_utime(struct FbxFS *fs, const char *path, struct utimbuf *ubuf)
{
	ODEBUGF("Fbx_utime(%#p, '%s', %#p)\n", fs, path, ubuf);

	return FSOP utime(path, ubuf, &fs->fcntx);
}

int Fbx_chmod(struct FbxFS *fs, const char *path, mode_t mode)
{
	ODEBUGF("Fbx_chmod(%#p, '%s', 0%o)\n", fs, path, mode);

	return FSOP chmod(path, mode, &fs->fcntx);
}

int Fbx_setxattr(struct FbxFS *fs, const char *path, const char *attr,
	CONST_APTR buf, size_t len, int flags)
{
	ODEBUGF("Fbx_setxattr(%#p, '%s', '%s', %#p, %lu, %d)\n", fs, path, attr, buf, len, flags);

	return FSOP setxattr(path, attr, buf, len, flags, &fs->fcntx);
}

int Fbx_getxattr(struct FbxFS *fs, const char *path, const char *attr,
	APTR buf, size_t len)
{
	ODEBUGF("Fbx_getxattr(%#p, '%s', '%s', %#p, %lu)\n", fs, path, attr, buf, len);

	return FSOP getxattr(path, attr, buf, len, &fs->fcntx);
}

int Fbx_removexattr(struct FbxFS *fs, const char *path, const char *attr)
{
	ODEBUGF("Fbx_removexattr(%#p, '%s', '%s')\n", fs, path, attr);

	return FSOP removexattr(path, attr, &fs->fcntx);
}

