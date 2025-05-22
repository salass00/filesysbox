/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"
#include "fuse_stubs.h"
#include <string.h>

#ifdef __AROS__
#define ID_BUSY_DISK AROS_MAKE_ID('B','U','S','Y')
#else
#define ID_BUSY_DISK (0x42555359L)
#endif

static void FbxFillInfoData(struct FbxFS *fs, struct InfoData *info) {
	struct FbxVolume *vol = fs->currvol;

	bzero(info, sizeof(*info));

	info->id_UnitNumber = fs->fssm ? fs->fssm->fssm_Unit : -1;

	if (OKVOLUME(vol)) {
		struct statvfs st;

		bzero(&st, sizeof(st));

		Fbx_statfs(fs, "/", &st);

		if (vol->writeprotect || (st.f_flag & ST_RDONLY))
			info->id_DiskState = ID_WRITE_PROTECTED;
		else
			info->id_DiskState = ID_VALIDATED;

		info->id_NumBlocks = st.f_blocks;
		info->id_NumBlocksUsed = st.f_blocks - st.f_bfree;
		info->id_BytesPerBlock = st.f_frsize;
#ifdef __AROS__
		info->id_DiskType = fs->dostype;
#else
		info->id_DiskType = ID_INTER_FFS_DISK;
#endif
		info->id_VolumeNode = MKBADDR(vol);

		if (!IsMinListEmpty(&vol->locklist) ||
			!IsMinListEmpty(&vol->notifylist) ||
			!IsMinListEmpty(&vol->unres_notifys))
		{
			info->id_InUse = DOSTRUE;
		}
	}
	else
	{
		info->id_DiskState     = ID_VALIDATING;
		info->id_BytesPerBlock = 512;

		if (BADVOLUME(vol))
		{
			//info->id_NumSoftErrors = -1;
			info->id_DiskType       = ID_NOT_REALLY_DOS;
		}
		else if (fs->inhibit)
		{
			info->id_DiskType       = ID_BUSY_DISK;
			info->id_InUse         = DOSTRUE;
		}
		else
		{
			info->id_DiskType       = ID_NO_DISK_PRESENT;
		}
	}
}

int FbxDiskInfo(struct FbxFS *fs, struct InfoData *info) {
	PDEBUGF("FbxDiskInfo(%#p, %#p)\n", fs, info);

	FbxFillInfoData(fs, info);

	fs->r2 = 0;
	return DOSTRUE;
}

int FbxInfo(struct FbxFS *fs, struct FbxLock *lock, struct InfoData *info) {
	PDEBUGF("FbxInfo(%#p, %#p, %#p)\n", fs, lock, info);

	CHECKVOLUME(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

	FbxFillInfoData(fs, info);

	fs->r2 = 0;
	return DOSTRUE;
}

