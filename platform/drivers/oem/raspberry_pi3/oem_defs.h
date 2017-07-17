#define MAX_BUF_SIZE  100

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

enum oem_profiles_index {
        DEVINFO_INDEX,
        IPADDR_INDEX,
        RAM_INDEX,
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
        LPO
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

enum ipaddr_profile_index {
        IP,
        IP_PROF_END
};

/* Hashing size, this really depends on the how many profiles it has,
 */

#define HASH_BUCKET_SZ  48
#define HASH_CHAIN_SZ   5

typedef struct {
        const char *oem_chr_name;
        int16_t  grp_indx;
        int16_t chr_indx;
} oem_hash_table_t;

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

static oem_profile_t oem_prof_data[NUM_PROF] = {
        [DEVINFO_INDEX] = {
                "DINF",
                "DeviceInfo",
                g_chrt_device_info,
                sizeof(g_chrt_device_info) / sizeof(oem_char_t),
                NULL
        },
        [IPADDR_INDEX] = {
                "IPADDR",
                "IP Address",
                g_chrt_OEM_Ipaddr,
                sizeof(g_chrt_ipaddr) / sizeof(oem_char_t),
                init_ipaddr_profile
        },
        [RAM_INDEX] = {
                "RAM",
                "RAM",
                g_chrt_ram,
                sizeof(g_chrt_ram) / sizeof(oem_char_t),
                NULL
        },

};
