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
static void gen_iso_8859_3(ULONG *maptab);
static void gen_iso_8859_7(ULONG *maptab);
static void gen_iso_8859_9(ULONG *maptab);
static void gen_iso_8859_13(ULONG *maptab);
static void gen_iso_8859_15(ULONG *maptab);

static const struct FbxCodeSet codesets[FBX_CS_MAX] =
{
	[FBX_CS_ISO_8859_1]     = { NULL                },
	[FBX_CS_ISO_8859_1_EUR] = { gen_iso_8859_1_euro },
	[FBX_CS_ISO_8859_2]     = { gen_iso_8859_2      },
	[FBX_CS_ISO_8859_3]     = { gen_iso_8859_3      },
	[FBX_CS_ISO_8859_7]     = { gen_iso_8859_7      },
	[FBX_CS_ISO_8859_9]     = { gen_iso_8859_9      },
	[FBX_CS_ISO_8859_13]    = { gen_iso_8859_13     },
	[FBX_CS_ISO_8859_15]    = { gen_iso_8859_15     },
};

static const struct
{
	ULONG carplatecode;
	ULONG iso3166code;
	WORD  csi;
}
country2cs[] =
{
	{ MAKE_ID('B','A',0,0),   MAKE_ID('B','I','H',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('C','Z',0,0),   MAKE_ID('C','Z','E',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('D','K',0,0),   MAKE_ID('D','N','K',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('D',0,0,0),     MAKE_ID('D','E','U',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('G','B',0,0),   MAKE_ID('G','B','R',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('E','E',0,0),   MAKE_ID('E','S','T',0), FBX_CS_ISO_8859_15    },
	{ MAKE_ID('C','Z',0,0),   MAKE_ID('C','Z','E',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('F',0,0,0),     MAKE_ID('F','R','A',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('E',0,0,0),     MAKE_ID('E','S','P',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('G','R',0,0),   MAKE_ID('G','R','C',0), FBX_CS_ISO_8859_7     },
	{ MAKE_ID('H','R',0,0),   MAKE_ID('H','R','V',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('I',0,0,0),     MAKE_ID('I','T','A',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('L','T',0,0),   MAKE_ID('L','T','U',0), FBX_CS_ISO_8859_13    },
	{ MAKE_ID('H','U',0,0),   MAKE_ID('H','U','N',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('N','L',0,0),   MAKE_ID('N','L','D',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('N',0,0,0),     MAKE_ID('N','O','R',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('P','L',0,0),   MAKE_ID('P','O','L',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('P','T',0,0),   MAKE_ID('P','R','T',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('R','U',0,0),   MAKE_ID('R','U','S',0), FBX_CS_AMIGA_1251     },
	{ MAKE_ID('S','K',0,0),   MAKE_ID('S','V','K',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('S','I',0,0),   MAKE_ID('S','V','N',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('R','S',0,0),   MAKE_ID('S','R','B',0), FBX_CS_ISO_8859_2     },
	{ MAKE_ID('F','I','N',0), MAKE_ID('F','I','N',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('S',0,0,0),     MAKE_ID('S','W','E',0), FBX_CS_ISO_8859_1_EUR },
	{ MAKE_ID('T','R',0,0),   MAKE_ID('T','U','R',0), FBX_CS_ISO_8859_9     }
};

static const struct
{
	const char *language;
	WORD        csi;
}
language2cs[] =
{
	{ "bosanski",          FBX_CS_ISO_8859_2     },
	{ "catal\xE0",         FBX_CS_ISO_8859_1_EUR },
	{ "czech",             FBX_CS_ISO_8859_2     },
	{ "dansk",             FBX_CS_ISO_8859_1_EUR },
	{ "deutch",            FBX_CS_ISO_8859_1_EUR },
	{ "english",           FBX_CS_ISO_8859_1_EUR },
	{ "esperanto",         FBX_CS_ISO_8859_3     },
	{ "eesti",             FBX_CS_ISO_8859_15    },
	{ "\xE8""e\xB9""tina", FBX_CS_ISO_8859_2     },
	{ "espa\xF1""ol",      FBX_CS_ISO_8859_1_EUR },
	{ "fran\xE7""ais",     FBX_CS_ISO_8859_1_EUR },
	{ "gaeilge",           FBX_CS_ISO_8859_15    },
	{ "galego",            FBX_CS_ISO_8859_1_EUR },
	{ "greek",             FBX_CS_ISO_8859_7     },
	{ "hrvatski",          FBX_CS_ISO_8859_2     },
	{ "italiano",          FBX_CS_ISO_8859_1_EUR },
	{ "lietuvi",           FBX_CS_ISO_8859_13    },
	{ "magyar",            FBX_CS_ISO_8859_2     },
	{ "nederlands",        FBX_CS_ISO_8859_1_EUR },
	{ "norsk",             FBX_CS_ISO_8859_1_EUR },
	{ "polski",            FBX_CS_ISO_8859_2     },
	{ "portugu\xEA""s",    FBX_CS_ISO_8859_1_EUR },
	{ "russian",           FBX_CS_AMIGA_1251     },
	{ "slovak",            FBX_CS_ISO_8859_2     },
	{ "slovensko",         FBX_CS_ISO_8859_2     },
	{ "srpski",            FBX_CS_ISO_8859_2     },
	{ "suomi",             FBX_CS_ISO_8859_1_EUR },
	{ "svenska",           FBX_CS_ISO_8859_1_EUR },
	{ "t\xFC""rk\xE7""e",  FBX_CS_ISO_8859_9     }
};

const struct FbxCodeSet *FbxFindCodeSetByCountry(struct FbxFS *fs, ULONG country)
{
	int i;

	if (country == 0)
		return NULL;

	for (i = 0; i < (sizeof(country2cs) / sizeof(country2cs[0])); i++)
	{
		if (country == country2cs[i].carplatecode ||
			country == country2cs[i].iso3166code)
		{
			return &codesets[country2cs[i].csi];
		}
	}

	return NULL;
}

const struct FbxCodeSet *FbxFindCodeSetByLanguage(struct FbxFS *fs, CONST_STRPTR language) {
	struct Library *UtilityBase = fs->utilitybase;
	int i;

	if (language == NULL || language[0] == '\0')
		return NULL;

	for (i = 0; i < (sizeof(language2cs) / sizeof(language2cs[0])); i++) {
		if (Stricmp(language, (CONST_STRPTR)language2cs[i].language) == 0) {
			return &codesets[language2cs[i].csi];
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
	0x00A0,0x0104,0x02D8,0x0141,0x00A4,0x013D,0x015A,0x00A7,
	0x00A8,0x0160,0x015E,0x0164,0x0179,0x00AD,0x017D,0x017B,
	0x00B0,0x0105,0x02DB,0x0142,0x00B4,0x013E,0x015B,0x02C7,
	0x00B8,0x0161,0x015F,0x0165,0x017A,0x02DD,0x017E,0x017C,
	0x0154,0x00C1,0x00C2,0x0102,0x00C4,0x0139,0x0106,0x00C7,
	0x010C,0x00C9,0x0118,0x00CB,0x011A,0x00CD,0x00CE,0x010E,
	0x0110,0x0143,0x0147,0x00D3,0x00D4,0x0150,0x00D6,0x00D7,
	0x0158,0x016E,0x00DA,0x0170,0x00DC,0x00DD,0x0162,0x00DF,
	0x0155,0x00E1,0x00E2,0x0103,0x00E4,0x013A,0x0107,0x00E7,
	0x010D,0x00E9,0x0119,0x00EB,0x011B,0x00ED,0x00EE,0x010F,
	0x0111,0x0144,0x0148,0x00F3,0x00F4,0x0151,0x00F6,0x00F7,
	0x0159,0x016F,0x00FA,0x0171,0x00FC,0x00FD,0x0163,0x02D9
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

static const UWORD iso_8859_3_to_ucs4[0x100 - 0xA0] =
{
	0x00A0,0x0126,0x02D8,0x00A3,0x00A4,0x0000,0x0124,0x00A7,
	0x00A8,0x0130,0x015E,0x011E,0x0134,0x00AD,0x0000,0x017B,
	0x00B0,0x0127,0x00B2,0x00B3,0x00B4,0x00B5,0x0125,0x00B7,
	0x00B8,0x0131,0x015F,0x011F,0x0135,0x00BD,0x0000,0x017C,
	0x00C0,0x00C1,0x00C2,0x0000,0x00C4,0x010A,0x0108,0x00C7,
	0x00C8,0x00C9,0x00CA,0x00CB,0x00CC,0x00CD,0x00CE,0x00CF,
	0x0000,0x00D1,0x00D2,0x00D3,0x00D4,0x0120,0x00D6,0x00D7,
	0x011C,0x00D9,0x00DA,0x00DB,0x00DC,0x016C,0x015C,0x00DF,
	0x00E0,0x00E1,0x00E2,0x0000,0x00E4,0x010B,0x0109,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
	0x0000,0x00F1,0x00F2,0x00F3,0x00F4,0x0121,0x00F6,0x00F7,
	0x011D,0x00F9,0x00FA,0x00FB,0x00FC,0x016D,0x015D,0x02D9
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
}

static const UWORD iso_8859_7_to_ucs4[0x100 - 0xA0] =
{
	0x00A0,0x2018,0x2019,0x00A3,0x20AC,0x20AF,0x00A6,0x00A7,
	0x00A8,0x00A9,0x037A,0x00AB,0x00AC,0x00AD,0x0000,0x2015,
	0x00B0,0x00B1,0x00B2,0x00B3,0x0384,0x0385,0x0386,0x00B7,
	0x0388,0x0389,0x038A,0x00BB,0x038C,0x00BD,0x038E,0x038F,
	0x0390,0x0391,0x0392,0x0393,0x0394,0x0395,0x0396,0x0397,
	0x0398,0x0399,0x039A,0x039B,0x039C,0x039D,0x039E,0x039F,
	0x03A0,0x03A1,0x0000,0x03A3,0x03A4,0x03A5,0x03A6,0x03A7,
	0x03A8,0x03A9,0x03AA,0x03AB,0x03AC,0x03AD,0x03AE,0x03AF,
	0x03B0,0x03B1,0x03B2,0x03B3,0x03B4,0x03B5,0x03B6,0x03B7,
	0x03B8,0x03B9,0x03BA,0x03BB,0x03BC,0x03BD,0x03BE,0x03BF,
	0x03C0,0x03C1,0x03C2,0x03C3,0x03C4,0x03C5,0x03C6,0x03C7,
	0x03C8,0x03C9,0x03CA,0x03CB,0x03CC,0x03CD,0x03CE,0x0000
};

static void gen_iso_8859_7(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xA0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_7_to_ucs4[i - 0xA0];
	}
}

static const UWORD iso_8859_9_to_ucs4[0x100 - 0xD0] =
{
	0x011E,0x00D1,0x00D2,0x00D3,0x00D4,0x00D5,0x00D6,0x00D7,
	0x00D8,0x00D9,0x00DA,0x00DB,0x00DC,0x0130,0x015E,0x00DF,
	0x00E0,0x00E1,0x00E2,0x00E3,0x00E4,0x00E5,0x00E6,0x00E7,
	0x00E8,0x00E9,0x00EA,0x00EB,0x00EC,0x00ED,0x00EE,0x00EF,
	0x011F,0x00F1,0x00F2,0x00F3,0x00F4,0x00F5,0x00F6,0x00F7,
	0x00F8,0x00F9,0x00FA,0x00FB,0x00FC,0x0131,0x015F,0x00FF,
};

static void gen_iso_8859_9(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xD0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_9_to_ucs4[i - 0xD0];
	}
}

static const UWORD iso_8859_13_to_ucs4[0x100 - 0xA0] =
{
	0x00A0,0x201D,0x00A2,0x00A3,0x00A4,0x201E,0x00A6,0x00A7,
	0x00D8,0x00A9,0x0156,0x00AB,0x00AC,0x00AD,0x00AE,0x00C6,
	0x00B0,0x00B1,0x00B2,0x00B3,0x201C,0x00B5,0x00B6,0x00B7,
	0x00F8,0x00B9,0x0157,0x00BB,0x00BC,0x00BD,0x00BE,0x00E6,
	0x0104,0x012E,0x0100,0x0106,0x00C4,0x00C5,0x0118,0x0112,
	0x010C,0x00C9,0x0179,0x0116,0x0122,0x0136,0x012A,0x013B,
	0x0160,0x0143,0x0145,0x00D3,0x014C,0x00D5,0x00D6,0x00D7,
	0x0172,0x0141,0x015A,0x016A,0x00DC,0x017B,0x017D,0x00DF,
	0x0105,0x012F,0x0101,0x0107,0x00E4,0x00E5,0x0119,0x0113,
	0x010D,0x00E9,0x017A,0x0117,0x0123,0x0137,0x012B,0x013C,
	0x0161,0x0144,0x0146,0x00F3,0x014D,0x00F5,0x00F6,0x00F7,
	0x0173,0x0142,0x015B,0x016B,0x00FC,0x017C,0x017E,0x2019,
};

static void gen_iso_8859_13(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xA0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_13_to_ucs4[i - 0xA0];
	}
}

static const UWORD iso_8859_15_to_ucs4[0xC0 - 0xA0] =
{
	0x00A0,0x00A1,0x00A2,0x00A3,0x20AC,0x00A5,0x0160,0x00A7,
	0x0161,0x00A9,0x00AA,0x00AB,0x00AC,0x00AD,0x00AE,0x00AF,
	0x00B0,0x00B1,0x00B2,0x00B3,0x017D,0x00B5,0x00B6,0x00B7,
	0x017E,0x00B9,0x00BA,0x00BB,0x0152,0x0153,0x0178,0x00BF,
};

static void gen_iso_8859_15(ULONG *maptab)
{
	int i;
	for (i = 0; i < 256; i++)
	{
		if (i < 0xA0 || i >= 0xC0)
			maptab[i] = i;
		else
			maptab[i] = iso_8859_15_to_ucs4[i - 0xA0];
	}
}

