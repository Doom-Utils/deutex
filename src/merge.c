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


#include "deutex.h"
#include "tools.h"
#include "endianm.h"
#include "mkwad.h"
#include "texture.h"
#include "ident.h"
#include "lists.h"
#include "merge.h"

#include <fcntl.h>


/********** HDR fiddling with WAD ******************/

/*
** old references
**
*/
static const Int32    HDRdirSz=5*sizeof(struct WADDIR);
static struct WADDIR HDRdir[6];
/*
**  Take some entries from a WAD
*/
static void HDRplunderWad(struct WADINFO *rwad,struct WADINFO *ewad)
{ char  *data;
  Int32 wsize,sz=0;
  Int32 ostart,osize;
  Int16 n;
  Int32 oldfilesz;
  /*
  ** calculate required size
  */
  oldfilesz=WADRposition(rwad); /*old file size*/
  /*
  ** copy entries from WAD
  */
  Phase("Copying entries from Wad (Please wait).\n");
  data = (char  *)Malloc(MEMORYCACHE);
  for(n=0;n<(rwad->ntry);n++)
  {
    if((n&0x7F)==0) Phase(".");
    ostart=rwad->dir[n].start;
    osize=rwad->dir[n].size;
    /*detect external entries */
    if(ostart&EXTERNAL)
    { /*update new directory*/
      WADRalign4(rwad);
      rwad->dir[n].start=WADRposition(rwad);
      /*get entry size*/
      if(osize>0)
      { /*copy old entry */
	WADRseek(ewad,ostart&(~EXTERNAL));
	for(wsize=0;wsize<osize;wsize+=sz)
	{ sz=(osize-wsize>MEMORYCACHE)? MEMORYCACHE:osize-wsize;
	  WADRreadBytes(ewad,data,sz);
	  if(WADRwriteBytes2(rwad,data,sz)<0)
	  { WADRchsize(rwad,oldfilesz);
	    ProgError("Not enough disk space.");
	    break;
	  }
	}
      }
    }
  }
  Phase("\n");
  Free(data);
}

/*
** Copy a WAD, and link to it's entries
*/
static Int32 HDRinsertWad(struct WADINFO *rwad,struct WADINFO *ewad,Int32 *pesize)
{ char  *data;
  Int32 wsize,sz=0;
  Int32 estart,esize;
  Int16 n;
  Int32 oldfilesz;
  /*
  ** calculate required size
  */
  oldfilesz=WADRposition(rwad); /*old file size*/
  WADRalign4(rwad);
  estart=WADRposition(rwad);
  WADRseek(ewad,0);
  esize=ewad->maxpos;
  Phase("Inserting Wad file into Wad.\n");
  data = (char  *)Malloc(MEMORYCACHE);
  for(wsize=0;wsize<esize;wsize+=sz)
  { sz=(esize-wsize>MEMORYCACHE)? MEMORYCACHE:esize-wsize;
    WADRreadBytes(ewad,data,sz);
    if(WADRwriteBytes2(rwad,data,sz)<0)
    { WADRchsize(rwad,oldfilesz);
      ProgError("Not enough disk space.");
      break;
    }
    if((wsize&0xF000)==0) Phase(".");
  }
  Free(data);
  for(n=0;n<(rwad->ntry);n++)
  { /*detect external entries */
    if((rwad->dir[n].start)&EXTERNAL)
    { /*update new directory*/
      rwad->dir[n].start &= (~EXTERNAL);
      rwad->dir[n].start += estart;
    }
  }
  Phase("\n");
  *pesize=  esize;
  return estart;
}
/*********** End fiddling with WAD ********************/



/******************* WAD restoration **********************/
void HDRrestoreWAD(const char *wadres)
{ struct WADINFO rwad;
  Int32 dirpos,ntry,n;
  Int32 rwadstart=0,rwadsize=0;
  Int32 ewadstart=0,ewadsize=0;
  static char ewadname[8];
  static char ewadfile[40];
  char  *data;
  Int32      size=0,wsize=0,sz=0;
  Int32 time;
  FILE *fp;
  Bool Fail;
  Phase("Attempting to restore WAD %s\n",wadres);
  /*open DOOM.WAD*/
  rwad.ok=0;
  WADRopenR(&rwad,wadres);

  /*get position of fake directory entry, reference to old dir*/
  dirpos = rwad.dirpos - HDRdirSz;
  WADRseek(&rwad,dirpos);
  WADRreadBytes(&rwad,(char  *)HDRdir,HDRdirSz);
  Fail=FALSE;
  if(peek_i32_le (&HDRdir[0].start) != 0x24061968L)  Fail=TRUE;
  if(peek_i32_le (&HDRdir[0].size)  != 666L)         Fail=TRUE;
  if(strncmp(HDRdir[0].name,"IZNOGOOD",8)!=0)       Fail=TRUE;
  if(Fail != FALSE)
  { if((n=WADRfindEntry(&rwad,"_DEUTEX_"))>=0)
      if(rwad.dir[n].size>=HDRdirSz)
	{ dirpos=rwad.dir[n].start;
	  WADRseek(&rwad,dirpos);
	  WADRreadBytes(&rwad,(char  *)HDRdir,HDRdirSz);
	  Fail=FALSE;
	  if(peek_i32_le (&HDRdir[0].start) != 0x24061968L)  Fail=TRUE;
	  if(peek_i32_le (&HDRdir[0].size)  != 666L)         Fail=TRUE;
	  if(strncmp(HDRdir[0].name,"IZNOGOOD",8)!=0) Fail=TRUE;
	}
  }
  if(Fail != FALSE) ProgError("Not a modified WAD");
  Phase("Restoration infos seem correct.\n");
  dirpos = peek_i32_le (&HDRdir[1].start);
  ntry   = peek_i32_le (&HDRdir[1].size);
  rwadstart = peek_i32_le (&HDRdir[2].start);
  rwadsize  = peek_i32_le (&HDRdir[2].size);
  ewadstart = peek_i32_le (&HDRdir[3].start);
  ewadsize  = peek_i32_le (&HDRdir[3].size);
  Normalise(ewadname,HDRdir[3].name);   /*name of WAD inside*/
  /*original file time*/
  time   = peek_i32_le (&HDRdir[4].size);
  if(peek_i32_le (&HDRdir[4].start)!=FALSE)
  { /*extract the PWAD*/
    sprintf(ewadfile,"%.8s.WAD",ewadname);
#if DT_OS == 'd'
#elif DT_OS == 'o'
    ToLowerCase(ewadfile);
#else
    ToLowerCase(ewadfile);
#endif
    fp=fopen(ewadfile,FOPEN_RB);
    if(fp!=NULL)
    { fclose(fp);
      Info("File %s already exist: internal WAD discarded.\n",ewadfile);
    }
    else
    { Phase("Restoring internal WAD %s\n",ewadfile);
      if((fp=fopen(ewadfile,FOPEN_WB))!=NULL)
      { data = (char  *)Malloc( MEMORYCACHE);
	size = ewadsize;
	WADRseek(&rwad,ewadstart);
	fseek(fp,0,SEEK_SET);
	for(wsize=0;wsize<size;wsize+=sz)
	{ sz=(size-wsize>MEMORYCACHE)? MEMORYCACHE : size-wsize;
	  WADRreadBytes(&rwad,data,sz);
	  if(fwrite(data,(size_t)sz,1,fp)!=1)
	  { Warning("Can't write %s",ewadstart);break;
	  }
	}
	Free(data);
	fclose(fp);
      }
    else
      Warning("Can't open %s",ewadfile);
    }
  }
  WADRopenA(&rwad,wadres);
  /*correct the directory reference of DOOM.WAD*/
  WADRsetDirRef(&rwad,ntry,dirpos);
  /*restore original size*/
  WADRchsize(&rwad,rwadstart+rwadsize);
  WADRclose(&rwad);
  SetFileTime(wadres,time);
  Output("Restoration of %s should be successful.\n",wadres);
}
/**************** End WAD restoration **********************/


static void HDRsetDir(struct WADINFO *rwad,Bool IsIwad,Bool Restore,
        Int32 time,Int32 dirpos,Int32 ntry,Int32 rsize,
        Int32 estart,Int32 esize,const char *wadext)
{ static char name[8];
  Int32 pos;
        /*Set the old references */
  Phase("Writing new WAD directory\n");
  write_i32_le (&HDRdir[0].start, 0x24061968L);
  write_i32_le (&HDRdir[0].size,  666L);
  Normalise(HDRdir[0].name,"IZNOGOOD");
        /*Set original WAD DIRECTORY*/
  write_i32_le (&HDRdir[1].start, dirpos);
  write_i32_le (&HDRdir[1].size,  ntry);
  Normalise(HDRdir[1].name,(IsIwad==TRUE)?"DOOM_DIR":"PWAD_DIR");
  /*Store original WAD size and start*/
  write_i32_le (&HDRdir[2].start, 0);
  write_i32_le (&HDRdir[2].size,  rsize);
  Normalise(HDRdir[2].name,"ORIGINAL");
  /*Store external WAD size and start*/
  write_i32_le (&HDRdir[3].start, estart);
  write_i32_le (&HDRdir[3].size,  esize);
  GetNameOfWAD(name,wadext);
  Normalise(HDRdir[3].name,name);
  /*old file time*/
  write_i32_le (&HDRdir[4].size,  time);
  write_i32_le (&HDRdir[4].start, Restore);
  Normalise(HDRdir[4].name,"TIME");
  /*save position of old ref if no previous _DEUTEX_ */
  WADRalign4(rwad);
  pos=(Int32)WADRfindEntry(rwad,"_DEUTEX_");
  if(pos<0)
  { pos=WADRposition(rwad);/*BC++ 4.5 bug*/
         WADRdirAddEntry(rwad,pos,HDRdirSz,"_DEUTEX_");
  }
  /*write old refs*/
  WADRwriteBytes(rwad,(char  *)HDRdir,HDRdirSz);
  /*write the directory*/
  rwad->dirpos = WADRposition(rwad);
  WADRwriteDir(rwad, 1);
}
/************ General services ******************/







/**************** Paste PWAD module ***********************/



/*
** merge WAD
**
*/

void PSTmergeWAD(const char *doomwad,const char *wadin,NTRYB select)
{
  static struct WADINFO iwad;
  static struct WADINFO pwad;
  ENTRY  *iiden;      /*identify entry in IWAD*/
  ENTRY  *piden;      /*identify entry in PWAD*/
  Int16 pnm;char  *Pnam;Int32 Pnamsz=0;
  Int32 dirpos,ntry,isize,pstart,psize,time;
  struct WADDIR  *NewDir;Int32 NewNtry;
  Phase("Attempting to merge IWAD %s and PWAD %s\n",doomwad,wadin);
  /*open iwad,get iwad directory*/
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
  /*find PNAMES*/
  pnm=WADRfindEntry(&iwad,"PNAMES");
  if(pnm<0) ProgError("Can't find PNAMES in main WAD");
  Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
  /* identify iwad*/
  iiden=IDENTentriesIWAD(&iwad, Pnam, Pnamsz,TRUE);
  /* get pwad directory, and identify*/
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  piden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
  /**/
  Free(Pnam);
  /*where to put pwad? at pwadstart*/
  /* merge the two directories */
  NewDir=LISmergeDir(&NewNtry,FALSE,TRUE,select,&iwad,iiden,0,&pwad,piden,
      EXTERNAL);
  /* prepare for append*/
  time=WADRprepareAppend(doomwad,&iwad,NewDir,NewNtry,&dirpos,&ntry,&isize);
  /* complete IWAD with PWAD, restorable*/
  pstart=HDRinsertWad(&iwad,&pwad,&psize);
  /* set directory */
  HDRsetDir(&iwad,TRUE,TRUE,time,dirpos,ntry,isize,pstart,psize,wadin);
  /* close */
  WADRclose(&pwad);
  WADRclose(&iwad);
}


/**************** End Paste PWAD module ****************/

















/**************** Append IWAD after PWAD module *****************/
/*
** -app complete a PWAD with DOOM entries
**
*/
void ADDappendSpriteFloor(const char *doomwad, const char *wadres,NTRYB select)
{
  struct WADINFO iwad;
  struct WADINFO pwad;
  ENTRY  *iiden;      /*identify entry in IWAD*/
  ENTRY  *piden;      /*identify entry in PWAD*/
  Int16 pnm;char  *Pnam;Int32 Pnamsz;
  Int32 dirpos,ntry,psize,time;
  struct WADDIR  *NewDir;Int32 NewNtry;
  Phase("Appending ");
  if(select&BSPRITE) Phase("Sprites ");
  if(select&BFLAT)   Phase("Flats ");
  Phase("from IWAD %s to PWAD %s\n",doomwad,wadres);
  /* get iwad directory, and identify */
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
  /*find PNAMES*/
  pnm=WADRfindEntry(&iwad,"PNAMES");
  if(pnm<0) ProgError("Can't find PNAMES in main WAD");
  Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
  /* identify iwad*/
  iiden=IDENTentriesIWAD(&iwad, Pnam, Pnamsz,TRUE);
  /* get pwad directory, and identify*/
  pwad.ok=0;
  WADRopenR(&pwad,wadres);
  piden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
  /**/
  Free(Pnam);
  /* merge the two directories */
  NewDir=LISmergeDir(&NewNtry,TRUE,TRUE,select,&iwad,iiden,EXTERNAL,&pwad,piden,0);
  /* prepare for append*/
  time=WADRprepareAppend(wadres,&pwad,NewDir,NewNtry,&dirpos,&ntry,&psize);
  /* append DOOM sprites to PWAD*/
  HDRplunderWad(&pwad,&iwad);
  /* set dir */
  HDRsetDir(&pwad,FALSE,FALSE,time,dirpos,ntry,psize,-1,-1,doomwad);
  /* close */
  WADRclose(&iwad);
  WADRclose(&pwad);
}



/*
** join: complete a PWAD with another PWAD entries
**
*/
void ADDjoinWads(const char *doomwad, const char *wadres, const char
    *wadext,NTRYB select)
{
  struct WADINFO iwad;  /*IWAD*/
  struct WADINFO ewad;  /*external Wad*/
  struct WADINFO rwad;
  ENTRY  *eiden;      /*identify entry in IWAD*/
  ENTRY  *riden;      /*identify entry in PWAD*/
  Int16 entry;char  *Entry;Int32 EntrySz;
  Int16 pnm;char  *Patch;Int32 PatchSz;
  Int32 start,size;
  Int16 etexu,rtexu;
  struct WADDIR  *NewDir;Int32 NewNtry;
  Bool TexuMrg = FALSE;
  Int32 dirpos,ntry,rsize,estart,esize,time;
  Phase("Merging PWAD %s into PWAD %s\n",wadext,wadres);
  /* get iwad directory, and identify */
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
  /*find PNAMES*/
  entry=WADRfindEntry(&iwad,"PNAMES");
  if(entry<0) ProgError("Can't find PNAMES in main WAD");
  Entry=WADRreadEntry(&iwad,entry,&EntrySz);
  /* get ewad directory, and identify */
  ewad.ok=0;
  WADRopenR(&ewad,wadext);
  eiden=IDENTentriesPWAD(&ewad, Entry, EntrySz);
  /* get rwad directory, and identify*/
  rwad.ok=0;
  WADRopenR(&rwad,wadres);
  riden=IDENTentriesPWAD(&rwad, Entry, EntrySz);
  /**/
  Free(Entry);
  /*merge texture1 if needed*/
  etexu=WADRfindEntry(&ewad,"TEXTURE1");
  rtexu=WADRfindEntry(&rwad,"TEXTURE1");
  if((etexu>=0)&&(rtexu>=0))
  { TexuMrg=TRUE;
   iwad.ok=0;
   WADRopenR(&iwad,doomwad);
   /*find PNAMES in IWAD and init*/
   pnm=WADRfindEntry(&iwad,"PNAMES");
   if(pnm<0) Bug("JnPnm");
   Entry=WADRreadEntry(&iwad,entry,&EntrySz);
   PNMinit(Entry,EntrySz);
   Free(Entry);
   /*declare TEXTURE1 from IWAD*/
   TXUinit();
  }
  WADRclose(&iwad);
  if(TexuMrg==TRUE)
  { /*add TEXTURE1 from rwad*/
   Phase("  With TEXTURE1 from %s\n",wadres);
   PatchSz=0;Patch=NULL;
   pnm=WADRfindEntry(&rwad,"PNAMES");
   if(pnm>=0)
   { Phase("  Declaring Patches from %s\n",wadres);
     riden[pnm]=EVOID;
     Patch=WADRreadEntry(&rwad,pnm,&PatchSz);
   }
   Entry=WADRreadEntry(&rwad,rtexu,&EntrySz);
   TXUreadTEXTURE(Entry,EntrySz,Patch,PatchSz,TRUE);
    if(PatchSz!=0)Free(Patch);
    Free(Entry);
    riden[rtexu]=EVOID; /* forget r texu*/
    /*TEXTURE1 from ewad*/
    Phase("  And TEXTURE1 from %s\n",wadext);
    PatchSz=0;Patch=NULL;
    pnm=WADRfindEntry(&ewad,"PNAMES");
    if(pnm>=0)
    { Phase("  Declaring Patches from %s\n",wadext);
      eiden[pnm]=EVOID;
      Patch=WADRreadEntry(&ewad,pnm,&PatchSz);
    }
    Entry=WADRreadEntry(&ewad,etexu,&EntrySz);
    TXUreadTEXTURE(Entry,EntrySz,Patch,PatchSz,FALSE);
    if(PatchSz!=0)Free(Patch);
    Free(Entry);
    eiden[etexu]=EVOID; /* forget e texu*/
  }
  /* merge the two directories, all entries */
  NewDir=LISmergeDir(&NewNtry,FALSE,FALSE,select,&rwad,riden,0,&ewad,eiden,EXTERNAL);
  /* prepare for append*/
  time=WADRprepareAppend(wadres,&rwad,NewDir,NewNtry,&dirpos,&ntry,&rsize);
  /* append PWAD into PWAD, restorable */
  estart=HDRinsertWad(&rwad,&ewad,&esize);
  /* append texu/pname*/
  if(TexuMrg==TRUE)
  { WADRalign4(&rwad);
    start=WADRposition(&rwad);
    size=TXUwriteTEXTUREtoWAD(&rwad);
    WADRdirAddEntry(&rwad,start,size,"TEXTURE1");
    TXUfree();
    WADRalign4(&rwad);
    start=WADRposition(&rwad);
    size=PNMwritePNAMEtoWAD(&rwad);
    WADRdirAddEntry(&rwad,start,size,"PNAMES");
    PNMfree();
  }

  /* set directory */
  HDRsetDir(&rwad,FALSE,TRUE,time,dirpos,ntry,rsize,estart,esize,wadext);
  /*end*/
  WADRclose(&rwad);
  WADRclose(&ewad);
}



















/*
** Add sprites and Floors
**
** must delete void entries (old DMADDS files)
** must select SPRITES of FLOORS
**
*/
/************** merge PWAD IWAD directory module ****************/
void ADDallSpriteFloor(const char *wadout, const char *doomwad, const char
    *wadres,NTRYB select)
{
  struct WADINFO iwad;
  struct WADINFO pwad;
  struct WADINFO rwad;
  Int16 n;
  ENTRY  *iiden;      /*identify entry in IWAD*/
  ENTRY  *piden;      /*identify entry in PWAD*/
  Int32 start,size,ostart,osize;
  Int16 pnm;char  *Pnam;Int32 Pnamsz;
  struct WADDIR  *NewDir;Int32 NewNtry;

  Phase("Copying ");
  if(select&BSPRITE) Phase("Sprites ");
  if(select&BFLAT)   Phase("Flats ");
  Phase("from IWAD %s and PWAD %s\n",doomwad,wadres);
  Phase("Into PWAD %s\n",wadout);
  /* get iwad directory, and identify */
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
  /*find PNAMES*/
  pnm=WADRfindEntry(&iwad,"PNAMES");
  if(pnm<0) ProgError("Can't find PNAMES in main WAD");
  Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
  /* identify iwad*/
  iiden=IDENTentriesIWAD(&iwad, Pnam, Pnamsz,TRUE);
  /* get pwad directory, and identify*/
  pwad.ok=0;
  WADRopenR(&pwad,wadres);
  piden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
  /**/
  Free(Pnam);
  /*where to put pwad? at pwadstart*/
  if((iwad.maxpos|pwad.maxpos)&EXTERNAL )Bug("AddExt");
  /* merge the two directories */
  NewDir=LISmergeDir(&NewNtry,TRUE,TRUE,select,&iwad,iiden,EXTERNAL,&pwad,piden,0);
  /* create a new PWAD*/
  rwad.ok=0;
  WADRopenW(&rwad,wadout,PWAD, 1);
  for(n=0;n<NewNtry;n++)
  { ostart=NewDir[n].start;
    osize=NewDir[n].size;
    WADRalign4(&rwad);
    start=WADRposition(&rwad);
    if(ostart&EXTERNAL)
      size=WADRwriteWADbytes(&rwad,&iwad,ostart&(~EXTERNAL),osize);
    else
      size=WADRwriteWADbytes(&rwad,&pwad,ostart,osize);
    WADRdirAddEntry(&rwad,start,size,NewDir[n].name);
  }
  Free(NewDir);
  /*close files memory*/
  WADRclose(&iwad);
  WADRclose(&pwad);
  WADRwriteDir(&rwad, 1);
  WADRclose(&rwad);
  Output("Addition of Sprites and Floors is complete\n");
}















