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


/*extract.c: list dir and extract entries*/
void XTRextractWAD(const char *doomwad, const char *DataDir, const char
    *wadin, const char *texout, IMGTYPE Picture,SNDTYPE Sound,Bool
    fullSND,NTRYB select, char trnR, char trnG, char trnB,Bool WSafe,
    cusage_t *cusage);

/*extract.c: get a single entry*/
void XTRgetEntry(const char *doomwad, const char *DataDir, const char *wadin,
    const char *entry, IMGTYPE Picture,SNDTYPE Sound,Bool fullSND, char trnR,
    char trnG, char trnB);

/*obsolete junk*/
void XTRtextureList(char *doomwad,char *DataDir);

void XTRpatchList(char *doomwad,char *DataDir,IMGTYPE Picture,
		char trnR, char trnG, char trnB);
