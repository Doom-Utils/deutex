/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2017 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.


  SPDX-License-Identifier: GPL-2.0+
*/
/*
 *	usedidx.h
 *	Palette index usage statistics (cf -usedidx)
 */


#ifndef DT_USEDIDX_H
#define DT_USEDIDX_H


/* This block is used in conjunction with -usedidx. Avoid
   messing with its fields directly. Use the API instead. */
struct cusage_s
{
  /* Per-lump: */
  char lump_name[8];
  unsigned long *lump_uses;

  /* Totals: */
  unsigned long uses[NCOLOURS];
  unsigned long nlumps[NCOLOURS];
  char where_first[NCOLOURS][8];
};

void usedidx_begin_lump (cusage_t *cusage, const char *name);
void usedidx_pixel (cusage_t *cusage, unsigned char idx);
void usedidx_end_lump (cusage_t *cusage);
void usedidx_rectangle (const char *buf, long buf_size, const char *name,
    cusage_t *cusage);


#endif
