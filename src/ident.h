/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

DeuTex is Copyright © 1994-1995 Olivier Montanuy,
          Copyright © 1999-2001 André Majorel.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


/* DOOM specifics*/

/* Entry Identification */
ENTRY  *IDENTentriesIWAD(struct WADINFO *wad,char  *Pnam,int32_t Pnamsz,bool Fast);
ENTRY  *IDENTentriesPWAD(struct WADINFO *wad,char  *Pnam,int32_t Pnamsz);
int16_t IDENTlevel(const char *buffer);
/* Level Part Identification */
int   IDENTlevelPart(const char *name);
/* Insertion point determination*/
int16_t IDENTinsrY(PICTYPE type,int16_t insrY,int16_t szy);
int16_t IDENTinsrX(PICTYPE type,int16_t insrX,int16_t szx);
ENTRY IDENTsneap (struct WADINFO *info, int16_t n);

const char *entry_type_name    (ENTRY type);  /* For EFLATS, return "flat" */
const char *entry_type_plural  (ENTRY type);  /* For EFLATS, return "flats" */
const char *entry_type_dir     (ENTRY type);  /* For EFLATS, return "flats" */
const char *entry_type_section (ENTRY type);  /* For EFLATS, return "flats" */
PICTYPE     entry_type_pictype (ENTRY type);  /* For EFLATS, return PFLAT */


