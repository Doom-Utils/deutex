/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/*init colors before any operation*/
void COLinit(uint8_t invR, uint8_t invG, uint8_t invB, char *Colors,
             int16_t Colsz, const char *pathname, const char *lumpname);
void COLinitAlt(char *_titlepal_data, int32_t _titlepal_size);
void COLfree(void);

/*cross reference for picture.c only*/
struct PIXEL {
    uint8_t R;
    uint8_t G;
    uint8_t B;
};
uint8_t COLindex(uint8_t R, uint8_t G, uint8_t B, uint8_t idx);
uint8_t COLinvisible(void);
struct PIXEL *COLdoomPalet(void);
struct PIXEL *COLaltPalet(void);
/*end of cross reference for picture.c*/
