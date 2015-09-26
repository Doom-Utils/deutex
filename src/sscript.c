/*
 *	sscript.c - read and write Strife scripts
 *	AYM 2000-05-13
 */

/*
This file is copyright André Majorel 2000-2005.

This program is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/


#include "deutex.h"
#include <errno.h>
#include <ctype.h>
#include "mkwad.h"
#include "tools.h"
#include "endianm.h"
#include "sscript.h"


#define PAGESZ   1516
#define OPTIONSZ 228
enum field_type_t
{
  FT_I32,
  FT_NAME,
  FT_S16,
  FT_S32,
  FT_S80,
  FT_S320
};


static void dump_field (FILE *fp, const unsigned char *buf, unsigned long *ofs,
    enum field_type_t type, const char *desc);
static void dump_junk (FILE *fp, const unsigned char *buf, unsigned long *ofs,
    size_t nbytes, const char *desc);
static int mem_is_zero (const char *buf, size_t buf_size);


/*
 *	sscript_save - save a script lump to file
 */
int sscript_save (struct WADINFO *wad, Int16 n, const char *file)
{
  FILE *fp = NULL;
  unsigned char *data = NULL;
  Int32 size = 0;
  const char *lumpname = wad->dir[n].name;

  data = (unsigned char *) WADRreadEntry (wad, n, &size);
  if (size % PAGESZ)
  {
    Warning ("SS10", "script %s: weird size %ld",
	lump_name (lumpname), (long) size);
    size = PAGESZ * (size / PAGESZ);
  }
  fp = fopen (file, "w");
  if (fp == NULL)
  {
    nf_err ("SS15", "%s: %s", file, strerror (errno));
    return 1;
  }
  fprintf (fp, "# DeuTex Strife script source version 1\n");
  /* Save all pages */
  {
    int p;
    for (p = 0; p < size / PAGESZ; p++)
    {
      unsigned long ofs = PAGESZ * p;
      int o;

      fputc ('\n', fp);
#ifdef SHOW_OFFSETS
	fprintf (fp, "/* %05lXh */ ", ofs);
#endif
      fprintf (fp, "page %d {\n", p);
      dump_field (fp, data, &ofs, FT_I32,  "    unknown0   ");
      dump_field (fp, data, &ofs, FT_I32,  "    unknown1   ");
      dump_field (fp, data, &ofs, FT_I32,  "    unknown2   ");
      dump_field (fp, data, &ofs, FT_I32,  "    unknown3   ");
      dump_field (fp, data, &ofs, FT_I32,  "    unknown4   ");
      dump_field (fp, data, &ofs, FT_I32,  "    unknown5   ");
      dump_field (fp, data, &ofs, FT_S16,  "    character  ");
      dump_field (fp, data, &ofs, FT_NAME, "    voice      ");
      dump_field (fp, data, &ofs, FT_NAME, "    background ");
      dump_field (fp, data, &ofs, FT_S320, "    statement  ");

      for (o = 0; o < 5; o++)
      {
	/* If the option is zeroed out, omit it */
	if (mem_is_zero ((char *) (data + ofs), OPTIONSZ))
	{
	  ofs += OPTIONSZ;
	  continue;
	}

	/* Else, decode it */
	fputc ('\n', fp);
#ifdef SHOW_OFFSETS
	fprintf (fp, "/* %05lXh */ ", ofs);
#endif
	fprintf (fp, "    option %d {\n", o);
	dump_field (fp, data, &ofs, FT_I32,  "        whata   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whatb   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whatc   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whatd   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whate   ");
	dump_field (fp, data, &ofs, FT_I32,  "        price   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whatg   ");
	dump_field (fp, data, &ofs, FT_S32,  "        option  ");
	dump_field (fp, data, &ofs, FT_S80,  "        success ");
	dump_field (fp, data, &ofs, FT_I32,  "        whath   ");
	dump_field (fp, data, &ofs, FT_I32,  "        whati   ");
	dump_field (fp, data, &ofs, FT_S80,  "        failure ");
#ifdef SHOW_OFFSETS
	fprintf (fp, "/* %05lXh */ ", ofs);
#endif
        fputs ("    }\n", fp);
      }

#ifdef SHOW_OFFSETS
      fprintf (fp, "/* %05lXh */ ", ofs);
#endif
      fputs ("}\n", fp);
      /* Sanity check */
      if (ofs % PAGESZ)
	Bug ("SS20", "script %s: page %d: bad script offset %d",
	    lump_name (lumpname), p, (int) ofs);
    }
  }

  Free (data);
  if (fclose (fp))
  {
    nf_err ("SS45", "%s: %s", file, strerror (errno));
    return 1;
  }
  return 0;
}


/*
 *	sscript_load
 */
int sscript_load (void)
{
  printf ("Oops! sscript_load not written yet!\n");
  return 1;
}


/*
 *	dump_field - write a field of a page to file
 */
static void dump_field (FILE *fp, const unsigned char *buf, unsigned long *ofs,
    enum field_type_t type, const char *desc)
{
#ifdef SHOW_OFFSETS
  fprintf (fp, "/* %05lXh */ ", (unsigned long) *ofs);
#endif
  fprintf (fp, "%-10s ", desc);
  if (type == FT_I32)
  {
    fprintf (fp, "%ld", (long) peek_i32_le (buf + *ofs));
    *ofs += 4;
  }
  else if (type == FT_NAME)
  {
    int n;
    putc ('"', fp);
    for (n = 0; n < 8 && buf[*ofs + n] != '\0'; n++)
      putc (tolower (buf[*ofs + n]), fp);
    putc ('"', fp);
    *ofs += 8;
  }
  else if (type == FT_S16)
  {
    fprintf (fp, "\"%.16s\"", buf + *ofs);
    *ofs += 16;
  }
  else if (type == FT_S32)
  {
    fprintf (fp, "\"%.32s\"", buf + *ofs);
    *ofs += 32;
  }
  else if (type == FT_S80)
  {
    fprintf (fp, "\"%.80s\"", buf + *ofs);
    *ofs += 80;
  }
  else if (type == FT_S320)
  {
    fprintf (fp, "\"%.320s\"", buf + *ofs);
    *ofs += 320;
  }
  else
  {
    Bug ("SS25", "bad field type %d", (int) type);
  }
  fputs (";\n", fp);
}


/*
 *	dump_junk - dump 32-bit little-endian integer at buf
 */
static void dump_junk (FILE *fp, const unsigned char *buf, unsigned long *ofs,
    size_t nbytes, const char *desc)
{
  int n;
  const int BPL = 16;

  for (n = 0; n < nbytes; n++)
  {
    if (n % BPL == 0)
    {
#ifdef SHOW_OFFSETS
      fprintf (fp, "/* %05lXh */ ", (unsigned long) *ofs);
#endif
      if (n == 0)
	fprintf (fp, "%-10s `", desc);
      else
	fprintf (fp, "%-10s  ", "");
    }
    else
      putc (' ', fp);
    fprintf (fp, "%02X", buf[*ofs + n]);
    if (n + 1 >= nbytes)
      fputs ("`;\n", fp);
    else if (n % BPL == BPL - 1)
      putc ('\n', fp);
  }
  *ofs += nbytes;
}


static int mem_is_zero (const char *buf, size_t buf_size)
{
  int n;

  for (n = 0; n < buf_size; n++)
    if (buf[n] != '\0')
      return 0;
  return 1;
}

