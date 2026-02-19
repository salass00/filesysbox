/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2026 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#include "filesysbox_internal.h"

static int Fbx_utimens(struct FbxFS *fs, const char *path, const struct timespec *tv)
{
	ODEBUGF("Fbx_utimens(%#p, '%s', %#p)\n", fs, path, tv);

	return FSOP utimens(path, tv, &fs->fcntx);
}

static int Fbx_utime(struct FbxFS *fs, const char *path, struct utimbuf *ubuf)
{
	ODEBUGF("Fbx_utime(%#p, '%s', %#p)\n", fs, path, ubuf);

	return FSOP utime(path, ubuf, &fs->fcntx);
}

static void FbxDS2TimeSpec(struct FbxFS *fs, const struct DateStamp *ds,
	struct timespec *ts)
{
	ULONG sec, nsec;

	sec = ds->ds_Days * (60*60*24);
	sec += ds->ds_Minute * 60;
	sec += ds->ds_Tick / TICKS_PER_SECOND;
	nsec = (ds->ds_Tick % TICKS_PER_SECOND) * ((1000*1000*1000) / TICKS_PER_SECOND);

	// add 8 years of seconds to adjust for different epoch.
	sec += UNIXTIMEOFFSET;

	/* convert to GMT */
	sec += fs->gmtoffset * 60;

	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

int FbxSetDate(struct FbxFS *fs, struct FbxLock *lock, const char *name,
	const struct DateStamp *date)
{
	struct timespec tv[2];
	int error;
	char fullpath[FBX_MAX_PATH];
#ifdef ENABLE_CHARSET_CONVERSION
	char fsname[FBX_MAX_NAME];
#endif

	PDEBUGF("FbxSetDate(%#p, %#p, '%s', %#p)\n", fs, lock, name, date);

	CHECKVOLUME(DOSFALSE);
	CHECKWRITABLE(DOSFALSE);

	if (lock != NULL) {
		CHECKLOCK(lock, DOSFALSE);

		if (lock->fsvol != fs->currvol) {
			fs->r2 = ERROR_NO_DISK;
			return DOSFALSE;
		}
	}

#ifdef ENABLE_CHARSET_CONVERSION
	if (FbxLocalToUTF8(fs, fsname, name, FBX_MAX_NAME) >= FBX_MAX_NAME) {
		fs->r2 = ERROR_LINE_TOO_LONG;
		return DOSFALSE;
	}
	name = fsname;
#else
	CHECKSTRING(name, DOSFALSE);
#endif

	if (!FbxLockName2Path(fs, lock, name, fullpath)) {
		fs->r2 = ERROR_OBJECT_NOT_FOUND;
		return DOSFALSE;
	}

	FbxDS2TimeSpec(fs, date, &tv[0]);
	tv[1] = tv[0];

	if (FSOP utimens) {
		error = Fbx_utimens(fs, fullpath, tv);
	} else {
		struct utimbuf ubuf;

		ubuf.actime  = tv[0].tv_sec;
		ubuf.modtime = tv[1].tv_sec;

		error = Fbx_utime(fs, fullpath, &ubuf);
	}
	if (error) {
		fs->r2 = FbxFuseErrno2Error(error);
		return DOSFALSE;
	}

	FbxDoNotify(fs, fullpath);

	FbxSetModifyState(fs, 1);

	fs->r2 = 0;
	return DOSTRUE;
}

