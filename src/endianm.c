/*
 *	endianm.c
 *	Endianness-independant memory access.
 *
 *	Those functions allow to retrieve little-endian and
 *	big-endian integers from a memory area regardless of
 *	the endianness of the CPU.
 *
 *	This code has been tested on 16-bit and 32-bit C
 *	compilers.
 *
 *	AYM 1999-07-04
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
#include "endianm.h"


/*
 *	read_i16_le
 *	Read a little-endian 16-bit signed integer from memory area
 *	pointed to by <ptr>.
 */
void read_i16_le (const void *ptr, i16 *buf)
{
  *buf = ((const unsigned char *) ptr)[0]
      | (((const unsigned char *) ptr)[1] << 8);
}


/*
 *	read_i32_le
 *	Read a little-endian 32-bit signed integer from memory area
 *	pointed to by <ptr>.
 */
void read_i32_le (const void *ptr, i32 *buf)
{
  *buf =    ((const unsigned char *) ptr)[0]
   | ((u16) ((const unsigned char *) ptr)[1] << 8)
   | ((i32) ((const unsigned char *) ptr)[2] << 16)
   | ((i32) ((const unsigned char *) ptr)[3] << 24);
}


/*
 *	peek_i16_le
 *	Read a little-endian 16-bit signed integer from memory area
 *	pointed to by <ptr>.
 */
i16 peek_i16_le (const void *ptr)
{
  return ((const unsigned char *) ptr)[0]
      | (((const unsigned char *) ptr)[1] << 8);
}


/*
 *	peek_u16_le
 *	Read a little-endian 16-bit unsigned integer from memory area
 *	pointed to by <ptr>.
 */
u16 peek_u16_le (const void *ptr)
{
  return ((const unsigned char *) ptr)[0]
      | (((const unsigned char *) ptr)[1] << 8);
}


/*
 *	peek_i32_be
 *	Read a big-endian 32-bit signed integer from memory area
 *	pointed to by <ptr>.
 */
i32 peek_i32_be (const void *ptr)
{
  return ((i32) (((const unsigned char *) ptr)[0]) << 24)
       | ((i32) (((const unsigned char *) ptr)[1]) << 16)
       | ((u16) (((const unsigned char *) ptr)[2]) << 8)
       |         ((const unsigned char *) ptr)[3];
}


/*
 *	peek_i32_le
 *	Read a little-endian 32-bit signed integer from memory area
 *	pointed to by <ptr>.
 */
i32 peek_i32_le (const void *ptr)
{
  return    ((const unsigned char *) ptr)[0]
   | ((u16) ((const unsigned char *) ptr)[1] << 8)
   | ((i32) ((const unsigned char *) ptr)[2] << 16)
   | ((i32) ((const unsigned char *) ptr)[3] << 24);
}


/*
 *	write_i16_le
 *	Write a little-endian 16-bit signed integer to memory area
 *	pointed to by <ptr>.
 */
void write_i16_le (void *ptr, i16 val)
{
  ((unsigned char *) ptr)[0] = val;
  ((unsigned char *) ptr)[1] = val >> 8;
}


/*
 *	write_i32_be
 *	Write a big-endian 32-bit signed integer to memory area
 *	pointed to by <ptr>.
 */
void write_i32_be (void *ptr, i32 val)
{
  ((unsigned char *) ptr)[0] = val >> 24;
  ((unsigned char *) ptr)[1] = val >> 16;
  ((unsigned char *) ptr)[2] = val >> 8;
  ((unsigned char *) ptr)[3] = val;
}


/*
 *	write_i32_le
 *	Write a little-endian 32-bit signed integer to memory area
 *	pointed to by <ptr>.
 */
void write_i32_le (void *ptr, i32 val)
{
  ((unsigned char *) ptr)[0] = val;
  ((unsigned char *) ptr)[1] = val >> 8;
  ((unsigned char *) ptr)[2] = val >> 16;
  ((unsigned char *) ptr)[3] = val >> 24;
}

