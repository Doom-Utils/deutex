/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

DeuTex is Copyright © 1994-1995 Olivier Montanuy,
          Copyright © 1999 André Majorel.

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
ENTRY huge *IDENTentriesIWAD(struct WADINFO *wad,char huge *Pnam,Int32 Pnamsz,Bool Fast);
ENTRY huge *IDENTentriesPWAD(struct WADINFO *wad,char huge *Pnam,Int32 Pnamsz);
Int16 IDENTlevel(char *buffer);
/* Level Part Identification */
Int16 IDENTlevelPart(char *name);
/* Insertion point determination*/
Int16 IDENTinsrY(PICTYPE type,Int16 insrY,Int16 szy);
Int16 IDENTinsrX(PICTYPE type,Int16 insrX,Int16 szx);
