/*
 *	endianio.c
 *	File I/O with explicit endianness.
 *
 *	Those functions allow to read little-endian and
 *	big-endian integers from a file regardless of the
 *	endianness of the CPU.
 *
 *	This code has been tested on 16-bit and 32-bit C
 *	compilers.
 *
 *	Adapted from Yadex by AYM on 1999-03-06.
 */


/*
This file is Copyright © 1999-2000 André Majorel.

This file is free software; you can redistribute it and/or modify it under
the terms of the GNU Library General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your option) any
later version.

This file is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Library General Public License for more
details.

You should have received a copy of the GNU Library General Public License
along with this file; if not, write to the Free Software Foundation, Inc.,
59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
*/


#include "deutex.h"
#include "endianio.h"


/*
 *	fread_i16_be
 *	Read a big-endian 16-bit signed integer	from file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fread_i16_be (FILE *fd, i16 *buf)
{
  *buf = (getc (fd) << 8)
        | getc (fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fread_i16_le
 *	Read a little-endian 16-bit signed integer from file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fread_i16_le (FILE *fd, i16 *buf)
{
  *buf = getc (fd)
      | (getc (fd) << 8);
  return feof (fd) || ferror (fd);
}


/*
 *	fread_i32_be
 *	Read a big-endian 32-bit signed integer from file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fread_i32_be (FILE *fd, i32 *buf)
{
  *buf = ((i32) getc (fd) << 24)
       | ((i32) getc (fd) << 16)
       | ((u16) getc (fd) << 8)
       |        getc (fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fread_i32_le
 *	Read a little-endian 32-bit signed integer from file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fread_i32_le (FILE *fd, i32 *buf)
{
  *buf =    getc (fd)
   | ((u16) getc (fd) << 8)
   | ((i32) getc (fd) << 16)
   | ((i32) getc (fd) << 24);
  return feof (fd) || ferror (fd);
}


/*
 *	fread_u16_le
 *	Read a little-endian 16-bit unsigned integer from file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fread_u16_le (FILE *fd, u16 *buf)
{
  *buf = getc (fd)
      | (getc (fd) << 8);
  return feof (fd) || ferror (fd);
}


/*
 *	fwrite_i16_le
 *	Write a little-endian 16-bit signed integer to file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fwrite_i16_le (FILE *fd, i16 buf)
{
  putc (       buf & 0xff, fd);
  putc ((buf >> 8) & 0xff, fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fwrite_i16_be
 *	Write a big-endian 16-bit signed integer to file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fwrite_i16_be (FILE *fd, i16 buf)
{
  putc ((buf >> 8) & 0xff, fd);
  putc (       buf & 0xff, fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fwrite_i32_le
 *	Write a little-endian 32-bit signed integer to file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fwrite_i32_le (FILE *fd, i32 buf)
{
  putc (        buf & 0xff, fd);
  putc ((buf >>  8) & 0xff, fd);
  putc ((buf >> 16) & 0xff, fd);
  putc ((buf >> 24) & 0xff, fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fwrite_i32_be
 *	Write a big-endian 32-bit signed integer to file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fwrite_i32_be (FILE *fd, i32 buf)
{
  putc ((buf >> 24) & 0xff, fd);
  putc ((buf >> 16) & 0xff, fd);
  putc ((buf >>  8) & 0xff, fd);
  putc (        buf & 0xff, fd);
  return feof (fd) || ferror (fd);
}


/*
 *	fwrite_u16_le
 *	Write a little-endian 16-bit unsigned integer to file <fd>.
 *	Returns 0 on success, != 0 on failure.
 */
int fwrite_u16_le (FILE *fd, u16 buf)
{
  putc (       buf & 0xff, fd);
  putc ((buf >> 8) & 0xff, fd);
  return feof (fd) || ferror (fd);
}

