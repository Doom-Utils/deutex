/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Rapha�l Quinet and Brendon Wyber.

DeuTex is Copyright � 1994-1995 Olivier Montanuy,
          Copyright � 1999 Andr� Majorel.

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


/*init colors before any operation*/
void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,char huge *Colors,Int16 Colsz);
void COLfree(void);


/*cross reference for picture.c only*/
struct PIXEL{ UInt8 R; UInt8 G; UInt8 B;};
UInt8 COLindex(UInt8 R, UInt8 G, UInt8 B,UInt8 idx);
UInt8 COLinvisible(void);
struct PIXEL huge *COLdoomPalet(void);
/*end of cross reference for picture.c*/
