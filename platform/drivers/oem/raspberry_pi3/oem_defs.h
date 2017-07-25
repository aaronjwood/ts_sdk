/* Copyright(C) 2017 Verizon. All rights reserved. */

#include <stdint.h>

#define MAX_BUF_SIZE    100

typedef void (*oem_update_profile)(void);

typedef struct {
        const char *chr_short_name;
        const char *chr_full_name;
        char value[MAX_BUF_SIZE];
} oem_char_t;

typedef struct {
        const char *grp_short_name;
        const char *grp_full_name;
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
        WIFI_INDEX,
        NW_INDEX,
        STRG_INDEX,
        BTR_INDEX,
        BT_INDEX,
        IPADDR_INDEX,
        NUM_PROF
};

enum device_profile_index {
        BBV,
        BID,
        DID,
        ICCID,
        IMEI,
        IMSI,
        KEV,
        LNG,
        LPO,
        MNF,
        MOD,
        CHIP,
        NFC,
        NLP,
        OSV,
        RTD,
        SCRS,
        SCRT,
        SGPS,
        TZ,
        DEV_PROF_END
};

enum ram_profile_index {
        AVRAM,
        FRRAM,
        TLRAM,
        RAM_PROF_END
};

enum wifi_profile_index {
        BSSID,
        WEN,
        HSP,
        MAC,
        SSID,
        WSC,
        WSS,
        WSP,
        WIFI_PROF_END
};

enum net_profile_index {
        XSIS,
        GSIS,
        G4SIS,
        BSID,
        CST,
        CTP,
        ISCC,
        MCC,
        MNC,
        NWT,
        NWID,
        RMN,
        SIMST,
        SST,
        SID,
        NW_PROF_END
};

enum storage_profile_index {
        AVST,
        FRST,
        TLST,
        STRG_PROF_END
};

enum battery_profile_index {
        UDET,
        CSRC,
        HLTH,
        PERC,
        STS,
        TECH,
        TMP,
        VLT,
        BTR_PROF_END
};

enum bluetooth_profile_index {
        BTSS,
        BTDI,
        BTD,
        BTEN,
        BHED,
        BHEL,
        BTN,
        BTS,
        BL_PROF_END
};

enum ipaddr_profile_index {
        IP,
        IP1,
        IP2,
        IP3,
        IP4,
        IP5,
        IP_PROF_END
};

/* Hashing size, this really depends on the how many profiles it has */
#define HASH_BUCKET_SZ  (NUM_PROF * 2)
#define HASH_CHAIN_SZ   5

#define TOTAL_CHARS   (DEV_PROF_END + RAM_PROF_END + WIFI_PROF_END + \
        NW_PROF_END + STRG_PROF_END + BTR_PROF_END + BTR_PROF_END + IP_PROF_END)

/* Hashing size, this really depends on the how many characterisitic it has */
#define CHAR_BUCKET_SZ  ((TOTAL_CHARS) * 2)
#define CHAR_CHAIN_SZ   8

#define IP_BUF_SZ       18
#define DEV_ID_SZ       14
#define NET_INTFC       "eth0"
#define INVALID         -1

static oem_char_t g_chrt_device_info[DEV_PROF_END] = {
        [BBV] = {
                "BBV",
                "Baseband Version",
                "N/A"
        },
        [BID] = {
                "BID",
                "Build ID",
                "N/A"
        },
        [DID] = {
                "DID",
                "Device ID",
                "N/A"
        },
        [ICCID] = {
                "ICCID",
                "ICCID",
                "N/A"
        },
        [IMEI] = {
                "IMEI",
                "IMEI",
                "N/A"
        },
        [IMSI] = {
                "IMSI",
                "IMSI",
                "N/A"
        },
        [KEV] = {
                "KEV",
                "Kernel Version",
                "N/A"
        },
        [LNG] = {
                "LNG",
                "Language",
                "N/A"
        },
        [LPO] = {
                "LPO",
                "Last Power On",
                "N/A"
        },
        [MNF] = {
                "MNF",
                "Manufacturer",
                "N/A"
        },
        [MOD] = {
                "MOD",
                "Model",
                "N/A"
        },
        [CHIP] = {
                "CHIP",
                "Chipset",
                "N/A"
        },
        [NFC] = {
                "NFC",
                "NFC",
                "N/A"
        },
        [NLP] = {
                "NLP",
                "Network Location Provider",
                "N/A"
        },
        [OSV] = {
                "OSV",
                "OS Version",
                "N/A"
        },
        [RTD] = {
                "RTD",
                "Rooted",
                "N/A"
        },
        [SCRS] = {
                "SCRS",
                "Screen Size",
                "N/A"
        },
        [SCRT] = {
                "SCRT",
                "Screen Timeout",
                "N/A"
        },
        [SGPS] = {
                "SGPS",
                "Standalone GPS",
                "N/A"
        },
        [TZ] = {
                "TZ",
                "Time Zone",
                "N/A"
        },
};

static oem_char_t g_chrt_ipaddr[IP_PROF_END] = {
        [IP] = {
                "RU0I41",
                "rmnet_usb0_IPV4_1",
                "N/A"
        },
        [IP1] = {
                "RU0I61",
                "rmnet_usb0_IPV6_1",
                "N/A"
        },
        [IP2] = {
                "RU0I62",
                "rmnet_usb0_IPV6_2",
                "N/A"
        },
        [IP3] = {
                "RU1I41",
                "rmnet_usb1_IPV4_1",
                "N/A"
        },
        [IP4] = {
                "RU1I61",
                "rmnet_usb1_IPV6_1",
                "N/A"
        },
        [IP5] = {
                "RU1I62",
                "rmnet_usb1_IPV6_2",
                "N/A"
        },
};

static oem_char_t g_chrt_ram[RAM_PROF_END] = {
        [AVRAM] = {
                "AVRAM",
                "RAM Available",
                "N/A"
        },
        [FRRAM] = {
                "FRRAM",
                "RAM Free",
                "N/A"
        },
        [TLRAM] = {
                "TLRAM",
                "RAM Total",
                "N/A"
        },
};

static oem_char_t g_chrt_wifi[WIFI_PROF_END] = {
        [BSSID] = {
                "BSSID",
                "BSSID",
                "N/A"
        },
        [WEN] = {
                "WEN",
                "WIFI Enabled",
                "N/A"
        },
        [HSP] = {
                "HSP",
                "hotSpot",
                "N/A"
        },
        [MAC] = {
                "MAC",
                "MAC",
                "N/A"
        },
        [SSID] = {
                "SSID",
                "SSID",
                "N/A"
        },
        [WSC] = {
                "WSC",
                "Scanning",
                "N/A"
        },
        [WSS] = {
                "WSS",
                "Signal",
                "N/A"
        },
        [WSP] = {
                "WSP",
                "Speed",
                "N/A"
        },
};
static oem_char_t g_chrt_network[NW_PROF_END] = {
        [XSIS] = {
                "1xSIS",
                "1x Signal Strength",
                "N/A"
        },
        [GSIS] = {
                "3gSIS",
                "3G Signal Strength",
                "N/A"
        },
        [G4SIS] = {
                "4gSIS",
                "4G Signal Strength",
                "N/A"
        },
        [BSID] = {
                "BSID",
                "Base Station ID",
                "N/A"
        },
        [CST] = {
                "CST",
                "Call State",
                "N/A"
        },
        [CTP] = {
                "CTP",
                "Connection Type",
                "Ethernet"
        },
        [ISCC] = {
                "ISCC",
                "ISO Country Code",
                "N/A"
        },
        [MCC] = {
                "MCC",
                "MCC",
                "N/A"
        },
        [MNC] = {
                "MNC",
                "MNC",
                "N/A"
        },
        [NWT] = {
                "NWT",
                "Network Type",
                "N/A"
        },
        [NWID] = {
                "NWID",
                "Network ID",
                "N/A"
        },
        [RMN] = {
                "RMN",
                "Roaming",
                "N/A"
        },
        [SIMST] = {
                "SIMST",
                "SIM State",
                "N/A"
        },
        [SST] = {
                "SST",
                "Service State",
                "N/A"
        },
        [SID] = {
                "SID",
                "System ID",
                "N/A"
        },
};

static oem_char_t g_chrt_storage[STRG_PROF_END] = {
        [AVST] = {
                "AVST",
                "Storage Available",
                "N/A"
        },
        [FRST] = {
                "FRST",
                "Storage Free",
                "N/A"
        },
        [TLST] = {
                "TLST",
                "Storage Total",
                "N/A"
        },
};

static oem_char_t g_chrt_battery[BTR_PROF_END] = {
        [UDET] = {
                "UDET",
                "Usage Details",
                "N/A"
        },
        [CSRC] = {
                "CSRC",
                "Charging Source",
                "N/A"
        },
        [HLTH] = {
                "HLTH",
                "Health",
                "N/A"
        },
        [PERC] = {
                "PERC",
                "Percentage",
                "N/A"
        },
        [STS] = {
                "STS",
                "Status",
                "N/A"
        },
        [TECH] = {
                "TECH",
                "Technology",
                "N/A"
        },
        [TMP] = {
                "TMP",
                "Temperature",
                "N/A"
        },
        [VLT] = {
                "VLT",
                "Voltage",
                "N/A"
        },
};

static oem_char_t g_chrt_bluetooth[BL_PROF_END] = {
        [BTSS] = {
                "BTSS",
                "A2DP Stereo Device State",
                "N/A"
        },
        [BTDI] = {
                "BTDI",
                "Discoverable",
                "N/A"
        },
        [BTD] = {
                "BTD",
                "Discovering",
                "N/A"
        },
        [BTEN] = {
                "BTEN",
                "BT Enabled",
                "N/A"
        },
        [BHED] = {
                "BHED",
                "HEADSET Device State",
                "N/A"
        },
        [BHEL] = {
                "BHEL",
                "HEALTH DEVICE State",
                "N/A"
        },
        [BTN] = {
                "BTN",
                "Name",
                "N/A"
        },
        [BTS] = {
                "BTS",
                "State",
                "N/A"
        },
};
