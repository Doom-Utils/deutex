/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2021 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/
/*
 *      wadio.h
 *      Wad low level I/O routines
 *      AYM 1999-03-06
 */

void set_output_wad_endianness(int big_endian);
void set_input_wad_endianness(int big_endian);
extern int (*wad_write_i16) (FILE *, int16_t);
extern int (*wad_write_i32) (FILE *, int32_t);
extern int (*wad_read_i16) (FILE *, int16_t *);
extern int (*wad_read_i32) (FILE *, int32_t *);
int wad_read_name(FILE * fd, char name[8]);
int wad_write_name(FILE * fd, const char *name);
