/**
  * Copyright (C) 2017 Sycamore Labs, LLC. All Rights Reserved
  *
  * bmc156_nomad.h
  *
  */

#ifndef __bmc156_nomad_h__
#define __bmc156_nomad_h__
#include "pin_std_defs.h"

extern void bmc156_nomad_init(periph_t i2c);
extern int32_t bmc156_read_sensor(array_t *buffer_struct);

#endif //__bmc156_nomad_h__