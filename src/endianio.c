/*
  This file is Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2021 contributors to the DeuTex project.

  Those functions allow to read little-endian and big-endian integers
  from a file regardless of the endianness of the CPU.

  SPDX-License-Identifier: LGPL-2.1-or-later
*/

#include "deutex.h"
#include "endianio.h"

/*
 *      fread_i16_be
 *      Read a big-endian 16-bit signed integer from file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fread_i16_be(FILE * fd, int16_t * buf)
{
    *buf = (getc(fd) << 8)
        | getc(fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fread_i16_le
 *      Read a little-endian 16-bit signed integer from file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fread_i16_le(FILE * fd, int16_t * buf)
{
    *buf = getc(fd)
        | (getc(fd) << 8);
    return feof(fd) || ferror(fd);
}

/*
 *      fread_i32_be
 *      Read a big-endian 32-bit signed integer from file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fread_i32_be(FILE * fd, int32_t * buf)
{
    *buf = ((int32_t) getc(fd) << 24)
        | ((int32_t) getc(fd) << 16)
        | ((uint16_t) getc(fd) << 8)
        | getc(fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fread_i32_le
 *      Read a little-endian 32-bit signed integer from file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fread_i32_le(FILE * fd, int32_t * buf)
{
    *buf = getc(fd)
        | ((uint16_t) getc(fd) << 8)
        | ((int32_t) getc(fd) << 16)
        | ((int32_t) getc(fd) << 24);
    return feof(fd) || ferror(fd);
}

/*
 *      fread_u16_le
 *      Read a little-endian 16-bit unsigned integer from file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fread_u16_le(FILE * fd, uint16_t * buf)
{
    *buf = getc(fd)
        | (getc(fd) << 8);
    return feof(fd) || ferror(fd);
}

/*
 *      fwrite_i16_le
 *      Write a little-endian 16-bit signed integer to file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fwrite_i16_le(FILE * fd, int16_t buf)
{
    putc(buf & 0xff, fd);
    putc((buf >> 8) & 0xff, fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fwrite_i16_be
 *      Write a big-endian 16-bit signed integer to file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fwrite_i16_be(FILE * fd, int16_t buf)
{
    putc((buf >> 8) & 0xff, fd);
    putc(buf & 0xff, fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fwrite_i32_le
 *      Write a little-endian 32-bit signed integer to file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fwrite_i32_le(FILE * fd, int32_t buf)
{
    putc(buf & 0xff, fd);
    putc((buf >> 8) & 0xff, fd);
    putc((buf >> 16) & 0xff, fd);
    putc((buf >> 24) & 0xff, fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fwrite_i32_be
 *      Write a big-endian 32-bit signed integer to file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fwrite_i32_be(FILE * fd, int32_t buf)
{
    putc((buf >> 24) & 0xff, fd);
    putc((buf >> 16) & 0xff, fd);
    putc((buf >> 8) & 0xff, fd);
    putc(buf & 0xff, fd);
    return feof(fd) || ferror(fd);
}

/*
 *      fwrite_u16_le
 *      Write a little-endian 16-bit unsigned integer to file <fd>.
 *      Returns 0 on success, != 0 on failure.
 */
int fwrite_u16_le(FILE * fd, uint16_t buf)
{
    putc(buf & 0xff, fd);
    putc((buf >> 8) & 0xff, fd);
    return feof(fd) || ferror(fd);
}
