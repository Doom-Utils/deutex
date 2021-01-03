/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2021 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/* DOOM specifics*/

/* Entry Identification */
ENTRY *IDENTentriesIWAD(struct WADINFO * wad, char *Pnam, int32_t Pnamsz,
                        bool Fast);
ENTRY *IDENTentriesPWAD(struct WADINFO *wad, char *Pnam, int32_t Pnamsz);
int16_t IDENTlevel(const char *buffer);
/* Level Part Identification */
int IDENTlevelPart(const char *name);
/* Insertion point determination*/
int16_t IDENTinsrY(PICTYPE type, int16_t insrY, int16_t szy);
int16_t IDENTinsrX(PICTYPE type, int16_t insrX, int16_t szx);
ENTRY IDENTsneap(struct WADINFO *info, int16_t n);

const char *entry_type_name(ENTRY type);        /* For EFLATS, return "flat" */
const char *entry_type_plural(ENTRY type);      /* For EFLATS, return "flats" */
const char *entry_type_dir(ENTRY type); /* For EFLATS, return "flats" */
const char *entry_type_section(ENTRY type);     /* For EFLATS, return "flats" */
PICTYPE entry_type_pictype(ENTRY type); /* For EFLATS, return PFLAT */
