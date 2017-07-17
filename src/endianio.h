/*
  This file is Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2017 contributors to the DeuTex project.

  Those functions allow to read little-endian and big-endian integers
  from a file regardless of the endianness of the CPU.

  SPDX-License-Identifier: LGPL-2.1+
*/


int fread_i16_le (FILE *fd, int16_t *buf);
int fread_i16_be (FILE *fd, int16_t *buf);
int fread_i32_le (FILE *fd, int32_t *buf);
int fread_i32_be (FILE *fd, int32_t *buf);
int fread_u16_le (FILE *fd, uint16_t *buf);
int fwrite_i16_le (FILE *fd, int16_t buf);
int fwrite_i16_be (FILE *fd, int16_t buf);
int fwrite_i32_le (FILE *fd, int32_t buf);
int fwrite_i32_be (FILE *fd, int32_t buf);
int fwrite_u16_le (FILE *fd, uint16_t buf);
