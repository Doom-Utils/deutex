/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* first call COLinit();*/
bool PICsaveInFile(char *file, PICTYPE type, char *pic, int32_t picsz,
                   int16_t * pXinsr, int16_t * pYinsr, IMGTYPE Picture,
                   const char *name, cusage_t * cusage);
int32_t PICsaveInWAD(struct WADINFO *info, char *file, PICTYPE type,
                     int16_t Xinsr, int16_t Yinsr, IMGTYPE Picture);
/* last call COLfree();*/

/*picture.c: only for test*/
void GIFtoBMP(char *file, char *bmpdir, char *name);
void BMPtoGIF(char *file, char *bmpdir, char *name);

/*
 *      pic_head_t
 *      parse_pic_header() returns header information
 *      through this structure.
 */
typedef struct {
    int16_t width;              /* Width of picture */
    int16_t height;             /* Height of picture */
    int16_t xofs;               /* X-offset of picture */
    int16_t yofs;               /* Y-offset of picture */
    const void *colofs;         /* Pointer on array of column offsets */
    const unsigned char *data;  /* Pointer on column data */
    size_t colofs_size;         /* Size of a column offset in bytes (2 or 4) */
    int dummy_bytes;            /* Are there dummy bytes around post data ? */
} pic_head_t;

int parse_pic_header(const char *buf, long size, pic_head_t * h,
                     char *message);
