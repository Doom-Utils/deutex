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


/* DOOM specifics*/

/* Entry Identification */
ENTRY  *IDENTentriesIWAD(struct WADINFO *wad,char  *Pnam,Int32 Pnamsz,Bool Fast);
ENTRY  *IDENTentriesPWAD(struct WADINFO *wad,char  *Pnam,Int32 Pnamsz);
Int16 IDENTlevel(const char *buffer);
/* Level Part Identification */
int   IDENTlevelPart(const char *name);
/* Insertion point determination*/
Int16 IDENTinsrY(PICTYPE type,Int16 insrY,Int16 szy);
Int16 IDENTinsrX(PICTYPE type,Int16 insrX,Int16 szx);
ENTRY IDENTsneap (struct WADINFO *info, Int16 n);

const char *entry_type_name    (ENTRY type);  /* For EFLATS, return "flat" */
const char *entry_type_plural  (ENTRY type);  /* For EFLATS, return "flats" */
const char *entry_type_dir     (ENTRY type);  /* For EFLATS, return "flats" */
const char *entry_type_section (ENTRY type);  /* For EFLATS, return "flats" */
PICTYPE     entry_type_pictype (ENTRY type);  /* For EFLATS, return PFLAT */


