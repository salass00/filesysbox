/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2023 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef CODESETS_H
#define CODESETS_H 1

#include <exec/types.h>

struct FbxCodeSet
{
	void (*gen_maptab)(ULONG *maptab);
};

struct FbxFS;

const struct FbxCodeSet *FbxFindCodeSetByCountry(struct FbxFS *fs, ULONG country);
const struct FbxCodeSet *FbxFindCodeSetByLanguage(struct FbxFS *fs, CONST_STRPTR language);

#endif /* CODESETS_H */

