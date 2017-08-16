/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>

#define MAX_BUF_SIZE    100

typedef void (*oem_update_profile)(void);

typedef struct {
	const char *chr_short_name;
	char value[MAX_BUF_SIZE];
} oem_char_t;

typedef struct {
	const char *grp_short_name;
	oem_char_t *oem_char;
	uint32_t chr_count;
	oem_update_profile update_prof;
} oem_profile_t;

typedef struct {
	const char *oem_chr_name;
	int grp_indx;
	int chr_indx;
} oem_hash_table_t;

enum oem_profiles_index {
	DEVINFO_INDEX,
	RAM_INDEX,
	NW_INDEX,
	STRG_INDEX,
	IPADDR_INDEX,
	NUM_PROF
};

enum device_profile_index {
	BBV, /* Baseband chip version */
	BID, /* Firmware build id */
	DID, /* device id */
	ICCID,
	IMEI,
	IMSI,
	KEV, /* Kernel version */
	LNG, /* Language */
	LPO, /* Last power on */
	MNF, /* Board Manufacturer */
	MOD, /* Model */
	CHIP, /* chipset */
	NFC,
	NLP, /* Network Location Provider */
	OSV, /* OS Version */
	RTD, /* Rooted */
	SCRS, /* Screen Size */
	SCRT, /* Screen Timeout */
	SGPS, /* Standalone GPS */
	TZ, /* Time Zone */
	DEV_PROF_END
};

enum ram_profile_index {
	AVRAM, /* Available RAM */
	FRRAM, /* Free RAM */
	TLRAM, /* Total RAM */
	RAM_PROF_END
};

enum net_profile_index {
	XSIS, /* 1x Signal Strength */
	GSIS, /* 3G Signal Strength */
	G4SIS, /* 4G Signal Strength */
	BSID, /* Base Station ID */
	CST, /* Call State */
	CTP, /* Connection Type */
	ISCC, /* ISO Country Code */
	MCC, /* MCC */
	MNC, /* MNC */
	NWT, /* Network Typ */
	NWID, /* Network ID */
	RMN, /* Roaming */
	SIMST, /* SIM State */
	SST, /* Service State */
	SID, /* System ID */
	NW_PROF_END
};

enum storage_profile_index {
	AVST, /* Available storage */
	FRST, /* Free Storage */
	TLST, /* Total Storage */
	STRG_PROF_END
};

enum ipaddr_profile_index {
	IP,
	IP_PROF_END
};

/* Hashing size, this really depends on the how many profiles it has */
#define HASH_BUCKET_SZ  (NUM_PROF)
#define HASH_CHAIN_SZ   5

#define TOTAL_CHARS   (DEV_PROF_END + RAM_PROF_END + \
NW_PROF_END + STRG_PROF_END + IP_PROF_END)

/* Hashing size, this really depends on the how many characterisitic it has */
#define CHAR_BUCKET_SZ  ((TOTAL_CHARS))
#define CHAR_CHAIN_SZ   8

#define IP_BUF_SZ       18
#define DEV_ID_SZ       14
#define NET_INTFC       "eth0"
#define INVALID         -1

static oem_char_t g_chrt_device_info[DEV_PROF_END] = {
	[BBV] = {
		"BBV",
		"N/A"
	},
	[BID] = {
		"BID",
		"N/A"
	},
	[DID] = {
		"DID",
		"N/A"
	},
	[ICCID] = {
		"ICCID",
		"N/A"
	},
	[IMEI] = {
		"IMEI",
		"N/A"
	},
	[IMSI] = {
		"IMSI",
		"N/A"
	},
	[KEV] = {
		"KEV",
		"N/A"
	},
	[LNG] = {
		"LNG",
		"N/A"
	},
	[LPO] = {
		"LPO",
		"N/A"
	},
	[MNF] = {
		"MNF",
		"N/A"
	},
	[MOD] = {
		"MOD",
		"N/A"
	},
	[CHIP] = {
		"CHIP",
		"N/A"
	},
	[NFC] = {
		"NFC",
		"N/A"
	},
	[NLP] = {
		"NLP",
		"N/A"
	},
	[OSV] = {
		"OSV",
		"N/A"
	},
	[RTD] = {
		"RTD",
		"N/A"
	},
	[SCRS] = {
		"SCRS",
		"N/A"
	},
	[SCRT] = {
		"SCRT",
		"N/A"
	},
	[SGPS] = {
		"SGPS",
		"N/A"
	},
	[TZ] = {
		"TZ",
		"N/A"
	},
	};

static oem_char_t g_chrt_ipaddr[IP_PROF_END] = {
	[IP] = {
		"ETH0IP",
		"N/A"
	}
};

static oem_char_t g_chrt_ram[RAM_PROF_END] = {
	[AVRAM] = {
		"AVRAM",
		"N/A"
	},
	[FRRAM] = {
		"FRRAM",
		"N/A"
	},
	[TLRAM] = {
		"TLRAM",
		"N/A"
	},
};

static oem_char_t g_chrt_network[NW_PROF_END] = {
	[XSIS] = {
		"1xSIS",
		"N/A"
	},
	[GSIS] = {
		"3gSIS",
		"N/A"
	},
	[G4SIS] = {
		"4gSIS",
		"N/A"
	},
	[BSID] = {
		"BSID",
		"N/A"
	},
	[CST] = {
		"CST",
		"N/A"
	},
	[CTP] = {
		"CTP",
		"Ethernet"
	},
	[ISCC] = {
		"ISCC",
		"N/A"
	},
	[MCC] = {
		"MCC",
		"N/A"
	},
	[MNC] = {
		"MNC",
		"N/A"
	},
	[NWT] = {
		"NWT",
		"N/A"
	},
	[NWID] = {
		"NWID",
		"N/A"
	},
	[RMN] = {
		"RMN",
		"N/A"
	},
	[SIMST] = {
		"SIMST",
		"N/A"
	},
	[SST] = {
		"SST",
		"N/A"
	},
	[SID] = {
		"SID",
		"N/A"
	},
};

static oem_char_t g_chrt_storage[STRG_PROF_END] = {
	[AVST] = {
		"AVST",
		"N/A"
	},
	[FRST] = {
		"FRST",
		"N/A"
	},
	[TLST] = {
		"TLST",
		"N/A"
	},
};

