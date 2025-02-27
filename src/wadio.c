/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/
/*
 *      wadio.c
 *      Wad low level I/O routines, without error checking.
 *      AYM 1999-03-06
 */

#include "deutex.h"
#include <ctype.h>
#include "endianio.h"
#include "wadio.h"

/*
 *      wad_read_i16
 *      wad_read_i32
 *      wad_write_i16
 *      wad_write_i32
 *
 *      Read and write 16-bit and 32-bit signed integers, with
 *      the defined endianness. By default, all wad I/O is done
 *      in little-endian mode.
 *
 *      Return 0 on success, non-zero on failure.
 */
int (*wad_write_i16) (FILE *, int16_t) = fwrite_i16_le;
int (*wad_write_i32) (FILE *, int32_t) = fwrite_i32_le;
int (*wad_read_i16) (FILE *, int16_t *) = fread_i16_le;
int (*wad_read_i32) (FILE *, int32_t *) = fread_i32_le;

/*
 *      set_input_wad_endianness
 *      set_output_wad_endianness
 *
 *      Define the endianness to use for wad input and output
 *      respectively. Normally, all wads are little-endian.
 */
void set_output_wad_endianness(int big_endian)
{
    wad_write_i16 = big_endian ? fwrite_i16_be : fwrite_i16_le;
    wad_write_i32 = big_endian ? fwrite_i32_be : fwrite_i32_le;
}

void set_input_wad_endianness(int big_endian)
{
    wad_read_i16 = big_endian ? fread_i16_be : fread_i16_le;
    wad_read_i32 = big_endian ? fread_i32_be : fread_i32_le;
}

/*
 *      wad_read_name
 *      wad_write_name
 *
 *      Read and write a directory name to a wad. The name is
 *      truncated to 8 characters, upper-cased, terminated and
 *      padded to 8 with NULs.
 *
 *      Return 0 on success, non-zero on failure.
 */
int wad_read_name(FILE * fd, char name[8])
{
    size_t n;
    int end = 0;
    for (n = 0; n < 8; n++) {
        int c = getc(fd);
        name[n] = end ? '\0' : toupper(c);
        if (c == '\0')
            end = 1;
    }
    return feof(fd) || ferror(fd);
}

int wad_write_name(FILE * fd, const char *name)
{
    size_t n;
    for (n = 0; n < 8 && name[n]; n++)
        putc(toupper(name[n]), fd);
    for (; n < 8; n++)
        putc('\0', fd);
    return ferror(fd);
}
