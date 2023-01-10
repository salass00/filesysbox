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
#include "codesets.h"
#include <libraries/iffparse.h> /* For MAKE_ID() */

enum
{
	FBX_CS_ISO_8859_1,
	FBX_CS_ISO_8859_1_EUR,
	FBX_CS_ISO_8859_2,
	FBX_CS_ISO_8859_3,
	FBX_CS_ISO_8859_7,
	FBX_CS_ISO_8859_9,
	FBX_CS_ISO_8859_13,
	FBX_CS_ISO_8859_15,
	FBX_CS_AMIGA_1251,
	FBX_CS_MAX
};

/* static void gen_iso_8859_1(ULONG *maptab); */
static void gen_iso_8859_1_euro(ULONG *maptab);
static void gen_iso_8859_2(ULONG *maptab);
/* static void gen_iso_8859_3(ULONG *maptab); */

static const struct FbxCodeSet codesets[FBX_CS_MAX] =
{
	[FBX_CS_ISO_8859_1]     = { NULL                },
	[FBX_CS_ISO_8859_1_EUR] = { gen_iso_8859_1_euro },
	[FBX_CS_ISO_8859_2]     = { gen_iso_8859_2      },
	/* [FBX_CS_ISO_8859_3]     = { gen_iso_8859_3      }, */
};

struct locmap
{
	const char *language;
	ULONG       carplatecode;
	ULONG       iso3166code;
	LONG        csi;
};

static const struct locmap loc2cs[] =
{
	{ "bosanski",          MAKE_ID('B','A',0,0),   MAKE_ID('B','I','H',0), FBX_CS_ISO_8859_2     },
	{ "catal\xE0",         MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ "czech",             MAKE_ID('C','Z',0,0),   MAKE_ID('C','Z','E',0), FBX_CS_ISO_8859_2     },
	{ "dansk",             MAKE_ID('D','K',0,0),   MAKE_ID('D','N','K',0), FBX_CS_ISO_8859_1_EUR },
	{ "deutch",            MAKE_ID('D',0,0,0),     MAKE_ID('D','E','U',0), FBX_CS_ISO_8859_1_EUR },
	{ "english",           MAKE_ID('G','B',0,0),   MAKE_ID('G','B','R',0), FBX_CS_ISO_8859_1_EUR },
	{ "esperanto",         MAKE_ID(0,0,0,0),       MAKE_ID(0,0,0,0),       FBX_CS_ISO_8859_3     },
	{ "eesti",             MAKE_ID('E','E',0,0),   MAKE_ID('E','S','T',0), FBX_CS_ISO_8859_15    },
	{ "\xE8""e\xB9""tina", MAKE_ID('C','Z',0,0),   MAKE_ID('C','Z','E',0), FBX_CS_ISO_8859_2     },
	{ "espa\xF1""ol",      MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ "fran\xE7""ais",     MAKE_ID('F',0,0,0),     MAKE_ID('F','R','A',0), FBX_CS_ISO_8859_1_EUR },
	{ "gaeilge",           MAKE_ID(0,0,0,0),       MAKE_ID(0,0,0,0),       FBX_CS_ISO_8859_15    },
	{ "galego",            MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ "greek",             MAKE_ID('G','R',0,0),   MAKE_ID('G','R','C',0), FBX_CS_ISO_8859_7     },
	{ "hrvatski",          MAKE_ID('H','R',0,0),   MAKE_ID('H','R','V',0), FBX_CS_ISO_8859_2     },
	{ "italiano",          MAKE_ID('I',0,0,0),     MAKE_ID('I','T','A',0), FBX_CS_ISO_8859_1_EUR },
	{ "lietuvi",           MAKE_ID('L','T',0,0),   MAKE_ID('L','T','U',0), FBX_CS_ISO_8859_13    },
	{ "magyar",            MAKE_ID('H','U',0,0),   MAKE_ID('H','U','N',0), FBX_CS_ISO_8859_2     },
	{ "nederlands",        MAKE_ID('N','L',0,0),   MAKE_ID('N','L','D',0), FBX_CS_ISO_8859_1_EUR },
	{ "norsk",             MAKE_ID('N',0,0,0),     MAKE_ID('N','O','R',0), FBX_CS_ISO_8859_1_EUR },
	{ "polski",            MAKE_ID('P','L',0,0),   MAKE_ID('P','O','L',0), FBX_CS_ISO_8859_2     },
	{ "portugu\xEA""s",    MAKE_ID('P','T',0,0),   MAKE_ID('P','R','T',0), FBX_CS_ISO_8859_1_EUR },
	{ "russian",           MAKE_ID('R','U',0,0),   MAKE_ID('R','U','S',0), FBX_CS_AMIGA_1251     },
	{ "slovak",            MAKE_ID('S','K',0,0),   MAKE_ID('S','V','K',0), FBX_CS_ISO_8859_2     },
	{ "slovensko",         MAKE_ID('S','I',0,0),   MAKE_ID('S','V','N',0), FBX_CS_ISO_8859_2     },
	{ "srpski",            MAKE_ID('R','S',0,0),   MAKE_ID('S','R','B',0), FBX_CS_ISO_8859_2     },
	{ "suomi",             MAKE_ID('F','I','N',0), MAKE_ID('F','I','N',0), FBX_CS_ISO_8859_1_EUR },
	{ "svenska",           MAKE_ID('S',0,0,0),     MAKE_ID('S','W','E',0), FBX_CS_ISO_8859_1_EUR },
	{ "t\xFC""rk\xE7""e",  MAKE_ID('T','R',0,0),   MAKE_ID('T','U','R',0), FBX_CS_ISO_8859_9     }
};

const struct FbxCodeSet *FbxFindCodeSetByCountry(struct FbxFS *fs, ULONG country)
{
	int i;

	if (country == 0)
		return NULL;

	for (i = 0; i < (sizeof(loc2cs) / sizeof(loc2cs[0])); i++)
	{
		if (country == loc2cs[i].carplatecode ||
			country == loc2cs[i].iso3166code)
		{
			return &codesets[loc2cs[i].csi];
		}
	}

	return NULL;
}

const struct FbxCodeSet *FbxFindCodeSetByLanguage(struct FbxFS *fs, CONST_STRPTR language) {
	struct Library *UtilityBase = fs->utilitybase;
	int i;

	if (language == NULL || language[0] == '\0')
		return NULL;

	for (i = 0; i < (sizeof(loc2cs) / sizeof(loc2cs[0])); i++) {
		if (Stricmp(language, (CONST_STRPTR)loc2cs[i].language) == 0) {
			return &codesets[loc2cs[i].csi];
		}
	}

	return NULL;
}

/* static void gen_iso_8859_1(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		maptab[i] = i;
	}
} */

static void gen_iso_8859_1_euro(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i == 164)
			maptab[i] = 0x20AC; /* Euro sign */
		else
			maptab[i] = i;
	}
}

static const UWORD iso_8859_2_to_ucs4[0x100 - 0xA0] =
{
	0x0A0,0x104,0x2D8,0x141,0x0A4,0x13D,0x15A,0x0A7,0x0A8,0x160,0x15E,0x164,0x179,0x0AD,0x17D,0x17B,
	0x0B0,0x105,0x2DB,0x142,0x0B4,0x13E,0x15B,0x2C7,0x0B8,0x161,0x15F,0x165,0x17A,0x2DD,0x17E,0x17C,
	0x154,0x0C1,0x0C2,0x102,0x0C4,0x139,0x106,0x0C7,0x10C,0x0C9,0x118,0x0CB,0x11A,0x0CD,0x0CE,0x10E,
	0x110,0x143,0x147,0x0D3,0x0D4,0x150,0x0D6,0x0D7,0x158,0x16E,0x0DA,0x170,0x0DC,0x0DD,0x162,0x0DF,
	0x155,0x0E1,0x0E2,0x103,0x0E4,0x13A,0x107,0x0E7,0x10D,0x0E9,0x119,0x0EB,0x11B,0x0ED,0x0EE,0x10F,
	0x111,0x144,0x148,0x0F3,0x0F4,0x151,0x0F6,0x0F7,0x159,0x16F,0x0FA,0x171,0x0FC,0x0FD,0x163,0x2D9
};

static void gen_iso_8859_2(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xA0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_2_to_ucs4[i - 0xA0];
	}
}

/* static const UWORD iso_8859_3_to_ucs4[0x100 - 0xA0] =
{
	0x0A0,0x126,0x2D8,0x0A3,0x0A4,0x???,0x124,0x0A7,0x0A8,0x130,0x15E,0x11E,0x134,0x0AD,0x???,0x17B,
	0x0B0,0x127,0x0B2,0x0B3,0x0B4,0x0B5,0x125,0x0B7,0x???,0x0B8,0x131,0x15F,0x11F,0x135,0x0BD,0x17C,
	0x0C0,
};

static void gen_iso_8859_3(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xA0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_3_to_ucs4[i - 0xA0];
	}
} */

