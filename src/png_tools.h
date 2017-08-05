/*
  This file is Copyright © 2017 contributors to the DeuTex project.

  SPDX-License-Identifier: GPL-2.0+
*/

#ifndef PNG_TOOLS_H
#define PNG_TOOLS_H

#include "deutex.h"

uint32_t gen_grAb_crc(unsigned char *buf);
unsigned char *read_whole_image(char *file, long *sz);
bool read_grAb_chunk(unsigned char *buffer, long sz, int32_t * xofs,
                     int32_t * yofs, long *grabpos);
void read_grAb(char *file, int16_t * Xinsr, int16_t * Yinsr);
void write_grAb(char *file, int16_t Xinsr, int16_t Yinsr);

#endif
