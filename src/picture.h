/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

DeuTex is Copyright © 1994-1995 Olivier Montanuy,
          Copyright © 1999-2000 André Majorel.

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this library; if not, write to the Free Software Foundation, Inc., 59 Temple
Place, Suite 330, Boston, MA 02111-1307, USA.
*/


/* first call COLinit();*/
Bool PICsaveInFile (char *file, PICTYPE type, char *pic, Int32 picsz,
    Int16 *pXinsr, Int16 *pYinsr, IMGTYPE Picture, const char *name,
    cusage_t *cusage);
Int32 PICsaveInWAD(struct WADINFO *info,char *file,PICTYPE type,Int16
    Xinsr,Int16 Yinsr,IMGTYPE Picture);
/* last call COLfree();*/


/*picture.c: only for test*/
void GIFtoBMP(char *file,char *bmpdir,char *name);
void BMPtoGIF(char *file,char *bmpdir,char *name);


/*
 *	pic_head_t
 *	parse_pic_header() returns header information
 *	through this structure.
 */
typedef struct
{
  Int16 width;			/* Width of picture */
  Int16 height;			/* Height of picture */
  Int16 xofs;			/* X-offset of picture */
  Int16 yofs;			/* Y-offset of picture */
  const void *colofs;		/* Pointer on array of column offsets */
  const unsigned char *data;	/* Pointer on column data */
  size_t colofs_size;		/* Size of a column offset in bytes (2 or 4) */
  int dummy_bytes;		/* Are there dummy bytes around post data ? */
} pic_head_t;

int parse_pic_header (const unsigned char *buf, long size, pic_head_t *h,
    char *message);


