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


/*for merging directories*/
void WADRopenPipo(struct WADINFO *info,Int32 ntry);
struct WADDIR huge *WADRclosePipo(struct WADINFO *info,Int32 huge *ntry);
Int32 WADRdirAddPipo(struct WADINFO *info,Int32 start,Int32 size,char *entry);

/*Open a WAD file for read*/
void  WADRopenR(struct WADINFO *info,char *wadin);
/*Open a WAD file for write*/
void WADRopenW(struct WADINFO *info,char *wadout, WADTYPE type);
/*Open a WAD file for append*/
void  WADRopenA(struct WADINFO *info,char *wadinout);
/*Close a WAD file*/
void  WADRclose(struct WADINFO *info);

/*WAD file structure*/
/*set position of internal WAD directory*/
void  WADRsetDirRef(struct WADINFO *info,Int32 ntry,Int32 dirpos);
/*change size of a WAD*/
void  WADRchsize(struct WADINFO *info,Int32 fsize);
/*increase size of WAD, do not update position*/
Bool WADRchsize2(struct WADINFO *info,Int32 fsize);


/*composition of internal WAD directory*/
/*add an entry to the directory*/
Int32  WADRdirAddEntry(struct WADINFO *info,Int32 start,Int32 size,char *name);
/*write the directory (and set it's position)*/
void  WADRwriteDir(struct WADINFO *info);
/*find an entry in the directory*/
Int16 WADRfindEntry(struct WADINFO *info,char *entry);  /*-1 or index of entry in directory*/

/*set data in a WAD (write position doesn't change)*/
void  WADRsetLong(struct WADINFO *info,Int32 pos,Int32 val);
void  WADRsetShort(struct WADINFO *info,Int32 pos,Int16 val);
/*align on long*/
void  WADRalign4(struct WADINFO *info);   /*align on long word, for next entry*/
/*tell position of pointer*/
Int32  WADRposition(struct WADINFO *info); /*current position*/
/*write date (write position increase)*/
Int32  WADRwriteLong(struct WADINFO *info,Int32 val);
Int32  WADRwriteShort(struct WADINFO *info,Int16 val);
Int32  WADRwriteBytes(struct WADINFO *info,char huge *data,Int32 size);
Int32  WADRwriteBytes2(struct WADINFO *info,char huge *data,Int32 size);
Int32  WADRwriteLump(struct WADINFO *info,char *file);
Int32  WADRwriteWADbytes(struct WADINFO *info,struct WADINFO *src,Int32 start,Int32 size);
Int32  WADRwriteWADentry(struct WADINFO *info,struct WADINFO *src,Int16 n);
void  WADRwriteWADlevelParts(struct WADINFO *info,struct WADINFO *src,Int16 n);
void  WADRwriteWADlevel(struct WADINFO *info,char *file,char *level);

/*read data*/
void  WADRseek(struct WADINFO *info,Int32 position);
Int32  WADRreadBytes(struct WADINFO *info,char huge *buffer,Int32 nb);
Int16 WADRreadShort(struct WADINFO *info);
Int32  WADRreadLong(struct WADINFO *info);
char huge *WADRreadEntry(struct WADINFO *info,Int16 N,Int32 *psize);
void  WADRsaveEntry(struct WADINFO *info,Int16 N, char *file);

/*make some preparations before appending data to an existing WAD*/
/*so that it can be restored later*/
Int32 WADRprepareAppend(char *wadres,struct WADINFO *rwad,struct WADDIR huge *NewDir,Int32 NewNtry,
     Int32 *dirpos,Int32 *ntry, Int32 *size);
