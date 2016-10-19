/* Copyright(C) 2016 Verizon. All rights reserved. */

#ifndef __OTT_MSG
#define __OTT_MSG

/* This header defines limits of the field sizes of a regular OTT message. */

#define OTT_MAX_MSG_SZ		512
#define OTT_VER_SZ		1
#define OTT_CMD_SZ		1
#define OTT_LEN_SZ		2
#define OTT_OVERHEAD_SZ		(OTT_CMD_SZ + OTT_LEN_SZ)
#define OTT_DATA_SZ		(OTT_MAX_MSG_SZ - OTT_OVERHEAD_SZ - OTT_VER_SZ)
#define OTT_UUID_SZ		16

#endif
