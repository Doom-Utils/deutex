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


/*init colors before any operation*/
void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,char  *Colors,Int16 Colsz);
void COLinitAlt (char *_titlepal_data, Int32 _titlepal_size);
void COLfree(void);


/*cross reference for picture.c only*/
struct PIXEL{ UInt8 R; UInt8 G; UInt8 B;};
UInt8 COLindex(UInt8 R, UInt8 G, UInt8 B,UInt8 idx);
UInt8 COLinvisible(void);
struct PIXEL *COLdoomPalet(void);
struct PIXEL *COLaltPalet(void);
/*end of cross reference for picture.c*/
