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


/*merge a WAD into an IWAD*/
void PSTmergeWAD(const char *doomwad, const char *wadin,NTRYB select);
/*put all sprites in another WAD, like DMADDS*/
void ADDallSpriteFloor(const char *wadout,const char *doomwad,const char *wadin,NTRYB select);
/*append all sprites*/
void ADDappendSpriteFloor(const char *doomwad,const char *wadin,NTRYB select);
/*join two WADs, including textures and pnames*/
void ADDjoinWads(const char *doomwad,const char *wadres,const char *wadext,NTRYB select);
/*restore a modified WAD*/
void HDRrestoreWAD(const char *wadres);
