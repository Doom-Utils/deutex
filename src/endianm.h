/*
  This file is Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2019 contributors to the DeuTex project.

  Those functions allow to read little-endian and big-endian integers
  from memory regardless of the endianness of the CPU.

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

/* Use the names DeuTex provides */
#ifndef i16
#define i16 int16_t
#endif

#ifndef i32
#define i32 int32_t
#endif

#ifndef u16
#define u16 uint16_t
#endif

#ifndef u32
#define u32 uint32_t
#endif

void read_i16_le(const void *ptr, i16 * buf);
void read_i32_le(const void *ptr, i32 * buf);
void read_i32_be(const void *ptr, i32 * buf);
i16 peek_i16_le(const void *ptr);
u16 peek_u16_le(const void *ptr);
i32 peek_i32_be(const void *ptr);
i32 peek_i32_le(const void *ptr);
void write_i16_le(void *ptr, i16 val);
void write_i32_be(void *ptr, i32 val);
void write_i32_le(void *ptr, i32 val);
