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


/*****************texture.c**********************/


/** PNM module **/
void  PNMinit(char  *buffer,Int32 size);
Int16 PNMindexOfPatch(char  *patch);
	/* check if patch exists. for ident.c*/
Int32  PNMwritePNAMEtoWAD(struct WADINFO *info);
	/* write PNAME entry in Wad*/
void  PNMfree(void);
Int16 PNMgetNbOfPatch(void);
	/* compose.c  get nb of patches*/
void  PNMgetPatchName(char name[8],Int16 index);
	/* compose.c  returns name of patch, from index*/
Bool  PNMisNew(Int16 idx);
	/* compose.c is the patch not in IWAD? */
/** TXU module **/
void TXUinit(void);  /*requires the the patches are init*/
void TXUfree(void);
Bool TXUexist(char *Name);
	/*does it exist?*/
Int32 TXUwriteTEXTUREtoWAD(struct WADINFO *info);
	/*write the TEXTURE entry in Wad*/
void TXUreadTEXTURE(char  *Data,Int32 DataSz,char  *Patch,Int32 PatchSz,Bool Redefn);
	/*read texture from raw data = TEXTURE entry */
void TXUwriteTexFile(const char *file);
	/*write the TEXTURE entry to a file*/
void TXUreadTexFile(const char *file,Bool Redefn);
	/*checkif the composition of textues in okay*/
Bool TXUcheckTex(Int16 npatch,Int16  *PszX);
	/*declare a fake texure. to list textures*/
void TXUfakeTex(char Name[8]);
	/*list all defined textures*/
void TXUlistTex(void);
