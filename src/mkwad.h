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


/*for merging directories*/
void WADRopenPipo(struct WADINFO *info,Int32 ntry);
struct WADDIR *WADRclosePipo(struct WADINFO *info,Int32  *ntry);
Int32 WADRdirAddPipo(struct WADINFO *info,Int32 start,Int32 size,const char
    *entry);

/*Open a WAD file for read*/
void WADRopenR(struct WADINFO *info, const char *wadin);
/*Open a WAD file for write*/
void WADRopenW(struct WADINFO *info, const char *wadout, WADTYPE type,
    int verbose);
/*Open a WAD file for append*/
void WADRopenA(struct WADINFO *info, const char *wadinout);
/*Close a WAD file*/
void WADRclose(struct WADINFO *info);

/*WAD file structure*/
/*set position of internal WAD directory*/
void WADRsetDirRef(struct WADINFO *info,Int32 ntry,Int32 dirpos);
/*change size of a WAD*/
void WADRchsize(struct WADINFO *info,Int32 fsize);
/*increase size of WAD, do not update position*/
Bool WADRchsize2(struct WADINFO *info,Int32 fsize);


/*composition of internal WAD directory*/
/*add an entry to the directory*/
Int32 WADRdirAddEntry(struct WADINFO *info,Int32 start,Int32 size, const char
    *name);
/*write the directory (and set it's position)*/
void  WADRwriteDir(struct WADINFO *info, int verbose);
/*find an entry in the directory*/
Int16 WADRfindEntry(struct WADINFO *info, const char *entry);  /*-1 or index of entry in directory*/

/*set data in a WAD (write position doesn't change)*/
void WADRsetLong(struct WADINFO *info,Int32 pos,Int32 val);
void WADRsetShort(struct WADINFO *info,Int32 pos,Int16 val);
/*align on long*/
void  WADRalign4(struct WADINFO *info);   /*align on long word, for next entry*/
/*tell position of pointer*/
Int32 WADRposition(struct WADINFO *info); /*current position*/
/*write date (write position increase)*/
Int32 WADRwriteLong(struct WADINFO *info,Int32 val);
Int32 WADRwriteShort(struct WADINFO *info,Int16 val);
Int32 WADRwriteBytes(struct WADINFO *info,char *data,Int32 size);
Int32 WADRwriteBytes2(struct WADINFO *info,char *data,Int32 size);
Int32 WADRwriteLump(struct WADINFO *info,const char *file);
Int32 WADRwriteWADbytes(struct WADINFO *info,struct WADINFO *src,Int32 start,Int32 size);
Int32 WADRwriteWADentry(struct WADINFO *info,struct WADINFO *src,Int16 n);
void WADRwriteWADlevelParts(struct WADINFO *info,struct WADINFO *src,Int16 n,
    size_t nlumps);
void WADRwriteWADlevel(struct WADINFO *info,const char *file,const char
    *level);

/*read data*/
void WADRseek(struct WADINFO *info,Int32 position);
iolen_t WADRreadBytes(struct WADINFO *info,char *buffer,iolen_t nbytes);
iolen_t WADRreadBytes(struct WADINFO *info,char *buffer,iolen_t nbytes);
Int16 WADRreadShort(struct WADINFO *info);
Int32 WADRreadLong(struct WADINFO *info);
char *WADRreadEntry(struct WADINFO *info,Int16 N,Int32 *psize);
char *WADRreadEntry2 (struct WADINFO *info, Int16 n, Int32 *psize);
void WADRsaveEntry(struct WADINFO *info,Int16 N, const char *file);

/*make some preparations before appending data to an existing WAD*/
/*so that it can be restored later*/
Int32 WADRprepareAppend(const char *wadres,struct WADINFO *rwad,struct WADDIR
     *NewDir,Int32 NewNtry, Int32 *dirpos,Int32 *ntry, Int32 *size);
