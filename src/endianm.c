/*
  This file is Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2019 contributors to the DeuTex project.

  Those functions allow to read little-endian and big-endian integers
  from memory regardless of the endianness of the CPU.

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "deutex.h"
#include "endianm.h"

/*
 *      read_i16_le
 *      Read a little-endian 16-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
void read_i16_le(const void *ptr, i16 * buf)
{
    *buf = ((const unsigned char *) ptr)[0]
        | (((const unsigned char *) ptr)[1] << 8);
}

/*
 *      read_i32_le
 *      Read a little-endian 32-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
void read_i32_le(const void *ptr, i32 * buf)
{
    *buf = ((const unsigned char *) ptr)[0]
        | ((u16) ((const unsigned char *) ptr)[1] << 8)
        | ((i32) ((const unsigned char *) ptr)[2] << 16)
        | ((i32) ((const unsigned char *) ptr)[3] << 24);
}

/*
 *      read_i32_be
 *      Read a big-endian 32-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
void read_i32_be(const void *ptr, i32 * buf)
{
    *buf = ((i32) ((const unsigned char *) ptr)[0] << 24)
        | ((i32) ((const unsigned char *) ptr)[1] << 16)
        | ((u16) ((const unsigned char *) ptr)[2] << 8)
        | (((const unsigned char *) ptr)[3]);
}

/*
 *      peek_i16_le
 *      Read a little-endian 16-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
i16 peek_i16_le(const void *ptr)
{
    return ((const unsigned char *) ptr)[0]
        | (((const unsigned char *) ptr)[1] << 8);
}

/*
 *      peek_u16_le
 *      Read a little-endian 16-bit unsigned integer from memory area
 *      pointed to by <ptr>.
 */
u16 peek_u16_le(const void *ptr)
{
    return ((const unsigned char *) ptr)[0]
        | (((const unsigned char *) ptr)[1] << 8);
}

/*
 *      peek_i32_be
 *      Read a big-endian 32-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
i32 peek_i32_be(const void *ptr)
{
    return ((i32) (((const unsigned char *) ptr)[0]) << 24)
        | ((i32) (((const unsigned char *) ptr)[1]) << 16)
        | ((u16) (((const unsigned char *) ptr)[2]) << 8)
        | ((const unsigned char *) ptr)[3];
}

/*
 *      peek_i32_le
 *      Read a little-endian 32-bit signed integer from memory area
 *      pointed to by <ptr>.
 */
i32 peek_i32_le(const void *ptr)
{
    return ((const unsigned char *) ptr)[0]
        | ((u16) ((const unsigned char *) ptr)[1] << 8)
        | ((i32) ((const unsigned char *) ptr)[2] << 16)
        | ((i32) ((const unsigned char *) ptr)[3] << 24);
}

/*
 *      write_i16_le
 *      Write a little-endian 16-bit signed integer to memory area
 *      pointed to by <ptr>.
 */
void write_i16_le(void *ptr, i16 val)
{
    ((unsigned char *) ptr)[0] = val;
    ((unsigned char *) ptr)[1] = val >> 8;
}

/*
 *      write_i32_be
 *      Write a big-endian 32-bit signed integer to memory area
 *      pointed to by <ptr>.
 */
void write_i32_be(void *ptr, i32 val)
{
    ((unsigned char *) ptr)[0] = val >> 24;
    ((unsigned char *) ptr)[1] = val >> 16;
    ((unsigned char *) ptr)[2] = val >> 8;
    ((unsigned char *) ptr)[3] = val;
}

/*
 *      write_i32_le
 *      Write a little-endian 32-bit signed integer to memory area
 *      pointed to by <ptr>.
 */
void write_i32_le(void *ptr, i32 val)
{
    ((unsigned char *) ptr)[0] = val;
    ((unsigned char *) ptr)[1] = val >> 8;
    ((unsigned char *) ptr)[2] = val >> 16;
    ((unsigned char *) ptr)[3] = val >> 24;
}
