/*
 * Filesysbox filesystem layer/framework
 *
 * Copyright (c) 2008-2011 Leif Salomonsson [dev blubbedev net]
 * Copyright (c) 2013-2025 Fredrik Wikstrom [fredrik a500 org]
 *
 * This library is released under AROS PUBLIC LICENSE 1.1
 * See the file LICENSE.APL
 */

#ifndef CODESETS_H
#define CODESETS_H 1

#include <exec/types.h>

struct FbxCodeSet
{
	void (*gen_maptab)(FbxUCS *maptab);
};

struct FbxFS; /* Forward declaration */

const struct FbxCodeSet *FbxFindCodeSetByName(struct FbxFS *fs, CONST_STRPTR name);
const struct FbxCodeSet *FbxFindCodeSetByCountry(struct FbxFS *fs, ULONG country);
const struct FbxCodeSet *FbxFindCodeSetByLanguage(struct FbxFS *fs, CONST_STRPTR language);

#endif /* CODESETS_H */

