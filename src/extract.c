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
#include "text.h"
#include "mkwad.h"
#include "texture.h"
#include "ident.h"
#include "color.h"
#include "picture.h"
#include "sound.h"
#include "usedidx.h"

/*compile only for DeuTex*/
#if defined DeuTex

/************** Begin XTRACT WAD module **************/
/*
** here we go for some real indecent programming. sorry
*/

extern char file[128];

/* DEBUG */
#if 0
static void stop (void)
{
  ;
}
#endif

/*
** try to save entry as BitMap .BMP
*/
static Bool XTRbmpSave(Int16 *pinsrX,Int16 *pinsrY,struct WADDIR  *entry,
		 PICTYPE type,const char *DataDir,const char *dir,struct
		 WADINFO *info,IMGTYPE Picture,Bool WSafe, cusage_t *cusage)
{  Bool res;
   Int32 start=entry->start;
   Int32 size =entry->size;
   char *name=entry->name;
   char  *buffer;
   char *extens=NULL;

   if(size<8L) return FALSE;
   switch(Picture)
   { case PICGIF: extens="GIF";break;
     case PICBMP: extens="BMP";break;
     case PICPPM: extens="PPM";break;
     case PICTGA: extens="TGA";break;
     default: Bug("img type");
   }
   res = MakeFileName(file,DataDir,dir,"",name,extens);
   if((WSafe==TRUE)&&(res==TRUE))
   { Warning("Will not overwrite file %s",file);
     return TRUE;
   }
   buffer=(char  *)Malloc(size);
   WADRseek(info,start);
   WADRreadBytes(info,buffer,size);
   res = PICsaveInFile(file,type,buffer,size,pinsrX,pinsrY,Picture, name,
       cusage);
   if(res==TRUE)Detail("Saved picture as %s\n",file);
   Free(buffer);
   return res;
}

/*
** extract entries from a WAD
**
** Called with cusage == NULL, (-xtract) this function extracts
** everything in the wad to separate files.
**
** Called with cusage != NULL, (-usedidx) this function creates
** no files but print statistics about which colours are used
** in the wad.
*/
void XTRextractWAD(const char *doomwad, const char *DataDir, const char
    *wadin, const char *wadinfo, IMGTYPE Picture,SNDTYPE Sound,Bool
    fullSND,NTRYB select, char trnR, char trnG, char trnB,Bool WSafe,
    cusage_t *cusage)
{ static struct WADINFO pwad;
  static struct WADINFO iwad;
  static struct WADINFO lwad;
  struct WADDIR  *pdir;
  Int16 pnb;
  ENTRY  *piden;
  Int16 p;
  Int32 ostart,osize;
  char  *buffer;
  Bool res;
  Int16 insrX=0,insrY=0;
  Bool EntryFound;
  /*PNAMES*/
  Int16 pnm;char  *Pnam;Int32 Pnamsz;
  char *extens=NULL;
  /*text file to write*/
  static struct TXTFILE *TXT = NULL;
  Phase("Extracting entries from WAD %s\n",wadin);
  /*open iwad,get iwad directory*/
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);

  /* If -usedidx, we're only interested in graphics. */
  if (cusage != NULL)
     select &= (BGRAPHIC | BSPRITE | BPATCH | BFLAT | BSNEAP | BSNEAT);

  /*find PNAMES*/
  pnm=WADRfindEntry(&iwad,"PNAMES");
  if(pnm<0) ProgError("Can't find PNAMES in main WAD");
  Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
  /*read WAD*/
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  pnb=(Int16)pwad.ntry;
  pdir=pwad.dir;
  piden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
  /**/
  Free(Pnam);

  /*
  ** prepare for graphics
  */

  /* Read PLAYPAL */
  {
    char  *Colors=NULL;
    pnm=WADRfindEntry(&pwad,"PLAYPAL");
    if(pnm>=0)
      Colors=WADRreadEntry(&pwad,pnm,&Pnamsz);
    else
    { pnm=WADRfindEntry(&iwad,"PLAYPAL");
      if(pnm>=0)
	Colors=WADRreadEntry(&iwad,pnm,&Pnamsz);
      else
	/* FIXME Should not be fatal, if all you want is to extract levels. */
	ProgError("Can't find PLAYPAL.");
    }
    COLinit(trnR,trnG,trnB,Colors,(Int16)Pnamsz);
    Free(Colors);
  }

  /* If TITLEPAL exists, read the first 768 bytes of it. But
     don't prepare COLpal2 because we don't know yet whether we
     need it. Indeed, if there are no sneats to extract, we're
     not interested in seeing any TITLEPAL-related warnings. */
  {
    int n;
    char *titlepal_data = NULL;
    Int32 titlepal_size = 3 * NCOLOURS;

    n = WADRfindEntry (&pwad, "TITLEPAL");
    if (n >= 0)
      titlepal_data = WADRreadEntry2 (&pwad, n, &titlepal_size);
    else
    {
      n = WADRfindEntry (&iwad, "TITLEPAL");
      if (n >= 0)
	titlepal_data = WADRreadEntry2 (&iwad, n, &titlepal_size);
      else
      {
	titlepal_data = NULL;
	titlepal_size = 0;
      }
    }
    COLinitAlt (titlepal_data, titlepal_size);
  }

  /*
  ** read the PNAMES entry in PWAD
  ** or in DOOM.WAD if it does not exist elsewhere
  */
  pnm=WADRfindEntry(&pwad,"PNAMES");
  if(pnm>=0)
    Pnam=WADRreadEntry(&pwad,pnm,&Pnamsz);
  else
  { pnm=WADRfindEntry(&iwad,"PNAMES");
    if(pnm>=0)
      Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
    else ProgError("Can't find PNAMES in main WAD");
  }
  PNMinit(Pnam,Pnamsz);
  Free(Pnam);
  /*
  ** iwad not needed anymore
  */
  WADRclose(&iwad);

  /*
  ** output WAD creation directives
  ** and save entries depending on their type.
  ** If -usedidx, do _not_ create a directives file.
  */
  if (cusage != NULL)
     TXT = &TXTdummy;  /* Notional >/dev/null */ 
  else
  {
     /*check if file exists*/
     TXT=TXTopenW(wadinfo);
     {
       char comment[81];
       sprintf (comment, "DeuTex %.32s by Olivier Montanuy", deutex_version);
       TXTaddComment (TXT, comment);
     }
     TXTaddComment(TXT,"PWAD creation directives");
  }

  /*
  ** LEVELS
  */
  if(select&BLEVEL)
  { Phase("Extracting levels...\n");
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { switch(piden[p]&EMASK)
      { case ELEVEL: case EMAP:
        if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"LEVELS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of levels");
           TXTaddSection(TXT,"levels");
           EntryFound=TRUE;
        }
        /* entries to save in WAD, named from Level name*/
	{
	  int pmax;
	  for (pmax = p; pmax < pnb && piden[pmax] == piden[p]; pmax++)
	    ;
	  res=MakeFileName(file,DataDir,"LEVELS","",pdir[p].name,"WAD");
	  if((WSafe==TRUE)&&(res==TRUE))
	    Warning("will not overwrite file %s",file);
	  else
	  { WADRopenW(&lwad,file,PWAD, 0);
	    ostart=WADRposition(&lwad);/*BC++ 4.5 bug*/
	    WADRdirAddEntry(&lwad,ostart,0L,pdir[p].name);
	    WADRwriteWADlevelParts (&lwad, &pwad, p, pmax - p);
	    WADRwriteDir(&lwad, 0);
	    WADRclose(&lwad);
	  }
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
	  p = pmax - 1;
	}
      }
    }
  }


  /*
  ** LUMPS
  */
  if(select&BLUMP)
  { Phase("Extracting lumps...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ELUMP)
      { if(EntryFound!=TRUE)
        { MakeDir(file,DataDir,"LUMPS","");
	  TXTaddEmptyLine (TXT);
          TXTaddComment(TXT,"List of data Lumps");
          TXTaddSection(TXT,"lumps");
          EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          res=FALSE;
          if(osize==64000L)/*lumps that are pics*/
          { res=XTRbmpSave(&insrX,&insrY,&pdir[p],PLUMP,DataDir,"LUMPS",&pwad,
	      Picture,WSafe, cusage);
          }
          if(res!=TRUE)   /*normal lumps*/
          { res=MakeFileName(file,DataDir,"LUMPS","",pdir[p].name,"LMP");
            if((WSafe==TRUE)&&(res==TRUE))
            {  Warning("will not overwrite file %s",file);
            }
            else
            { WADRsaveEntry(&pwad,p,file);
              Detail("Saved Lump as   %s\n",file);
            }
          }
          TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
        }
      }
    }
  }

  /*
  ** TEXTURES
  */
if(select&BTEXTUR)
{ EntryFound=FALSE;
  for(p=0;p<pnb;p++)
  { if(piden[p]==ETEXTUR+1)
    { if(EntryFound!=TRUE)
      {  MakeDir(file,DataDir,"TEXTURES","");
         EntryFound=TRUE;
      }
      TXTaddEmptyLine (TXT);
      TXTaddComment(TXT,"List of definitions for TEXTURE1");
      TXTaddSection(TXT,"texture1");
      
      {
	 const char *name;
         /* Always extract TEXTURES as texture1.txt ! -- AYM 1999-09-18 */
	 if (texture_lump == TL_TEXTURES)
	    name = "TEXTURE1";
	 else
	    name = pdir[p].name;
	 TXTaddEntry(TXT,name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
	 res=MakeFileName(file,DataDir,"TEXTURES","",name,"TXT");
      }
      if((WSafe==TRUE)&&(res==TRUE))
      {         Warning("will not overwrite file %s",file);
      }
      else
      { buffer=(char  *)Malloc(pdir[p].size);
        WADRseek(&pwad,pdir[p].start);
        WADRreadBytes(&pwad,buffer,pdir[p].size);
        TXUinit();
        TXUreadTEXTURE(buffer,pdir[p].size,NULL,0,TRUE);
        Free(buffer);
        TXUwriteTexFile(file);
        TXUfree();
      }
    }
  }
  for(p=0;p<pnb;p++)
  { if(piden[p]==ETEXTUR+2)
    { if(EntryFound!=TRUE)
      {  MakeDir(file,DataDir,"TEXTURES","");
         EntryFound=TRUE;
      }
      TXTaddEmptyLine (TXT);
      TXTaddComment(TXT,"List of definitions for TEXTURE2");
      TXTaddSection(TXT,"texture2");
      TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
      res=MakeFileName(file,DataDir,"TEXTURES","",pdir[p].name,"TXT");
      if((WSafe==TRUE)&&(res==TRUE))
      {         Warning("will not overwrite file %s",file);
      }
      else
      { buffer=(char  *)Malloc(pdir[p].size);
        WADRseek(&pwad,pdir[p].start);
        WADRreadBytes(&pwad,buffer,pdir[p].size);
        TXUinit();
        TXUreadTEXTURE(buffer,pdir[p].size,NULL,0,TRUE);
        Free(buffer);
        TXUwriteTexFile(file);
        TXUfree();
      }
    }
   }
 }


  /*
  ** SOUNDS
  */
  if(select&BSOUND)
  { Phase("Extracting sounds...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESOUND)
      { if(EntryFound!=TRUE)
        { MakeDir(file,DataDir,"SOUNDS","");
          TXTaddEmptyLine (TXT);
          TXTaddComment(TXT,"List of Sounds");
          TXTaddSection(TXT,"sounds");
          EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          switch(piden[p])
          { case ESNDPC:
              res=MakeFileName(file,DataDir,"SOUNDS","",pdir[p].name,"TXT");
              if((WSafe==TRUE)&&(res==TRUE))
              {  Warning("will not overwrite file %s",file);
              }
              else
              { buffer=(char  *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsavePCSound(file,buffer,pdir[p].size);
                Free(buffer);
                Detail("Saved PC Sound as   %s\n",file);
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,
		  FALSE);
              break;
             case ESNDWAV:
              switch(Sound)
              { case SNDAU:  extens="AU";break;
                case SNDWAV: extens="WAV";break;
                case SNDVOC: extens="VOC";break;
                default: Bug("snd type");
              }
              res=MakeFileName(file,DataDir,"SOUNDS","",pdir[p].name,extens);
              if((WSafe==TRUE)&&(res==TRUE))
              { Warning("will not overwrite file %s",file);
              }
              else
              { buffer=(char  *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsaveSound(file,buffer,pdir[p].size,Sound,fullSND,
		    pdir[p].name);
                Detail("Saved Sound as   %s\n",file);
                Free(buffer);
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,
		  FALSE);
              break;
            default:
              Bug("Snd type");
          }
        }
      }
    }
  }

  /*
  ** MUSICS
  */
  if(select&BMUSIC)
  { Phase("Extracting musics...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EMUSIC)
      { if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"MUSICS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Musics");
           TXTaddSection(TXT,"musics");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          res=MakeFileName(file,DataDir,"MUSICS","",pdir[p].name,"MUS");
          if((WSafe==TRUE)&&(res==TRUE))
          {  Warning("will not overwrite file %s",file);
          }
          else
          { WADRsaveEntry(&pwad,p,file);
            Detail("Saved Music as %s\n",file);
          }
          TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
        }
      }
    }
  }

  /*
  ** GRAPHICS
  */
  if(select&BGRAPHIC)
  { Phase("Extracting graphics...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EGRAPHIC)
      { if(EntryFound!=TRUE && cusage == NULL)
        { MakeDir(file,DataDir,"GRAPHICS","");
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PGRAPH,DataDir,"GRAPHICS",&pwad,
	      Picture,WSafe, cusage)==TRUE)
          { if(EntryFound!=TRUE)
            { 
              TXTaddEmptyLine (TXT);
	      TXTaddComment(TXT,"List of Pictures (with insertion point)");
              TXTaddSection(TXT,"graphics");
              EntryFound=TRUE;
            }
            TXTaddEntry(TXT,pdir[p].name,NULL,insrX,insrY,FALSE,TRUE);
          }
          else if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"LUMPS",&pwad,
	      Picture,WSafe, cusage)==TRUE)
          { /*Was saved as graphic lump*/
            TXTaddComment(TXT,pdir[p].name);
          }
          else
          { if(MakeFileName(file,DataDir,"LUMPS","",pdir[p].name,"LMP")==TRUE)
            {  Warning("Will not overwrite file %s",file);
            }
            else
            { WADRsaveEntry(&pwad,p,file);
              Detail("Saved Lump as   %s\n",file);
            }
            TXTaddComment(TXT,pdir[p].name);
          }
        }
      }
    }
  }

  /*
  ** SPRITES
  */
  if(select&BSPRITE)
  { Phase("Extracting sprites...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESPRITE)
      { if(EntryFound!=TRUE)
        {  if (cusage == NULL)
	      MakeDir(file,DataDir,"SPRITES","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Sprites");
           TXTaddSection(TXT,"sprites");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PSPRIT,DataDir,"SPRITES",&pwad,
	      Picture,WSafe, cusage)!=TRUE)
          { Warning("failed to write sprite %s", lump_name (pwad.dir[p].name));
          }
          else
          { TXTaddEntry(TXT,pdir[p].name,NULL,insrX,insrY,FALSE,TRUE);
          }
        }
      }
    }
  }

  /*
  ** PATCHES
  */
  if(select&BPATCH)
  { Phase("Extracting patches...\n");
    for(EntryFound=FALSE,p=0;p<pnb;p++)
     { if((piden[p] & EMASK)==EPATCH)
       { if(EntryFound!=TRUE)
         { 
	   if (cusage == NULL)
	      MakeDir(file,DataDir,"PATCHES","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of wall patches");
           TXTaddSection(TXT,"patches");
           EntryFound=TRUE;
         }
         if(XTRbmpSave(&insrX,&insrY,&pdir[p],PPATCH,DataDir,"PATCHES",&pwad,
	     Picture,WSafe,cusage)==TRUE)
         { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
         }
         else
         { Warning("failed to write patch %s", lump_name (pwad.dir[p].name));
         }
       }
     }
  }

  /*
  ** PNAMES not needed anymore
  */
  PNMfree();

  /*
  ** FLATS
  */
  if(select&BFLAT)
  { Phase("Extracting flats...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EFLAT)
      { if(EntryFound!=TRUE)
        {  if (cusage == NULL)
	      MakeDir(file,DataDir,"FLATS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Floors and Ceilings");
           TXTaddSection(TXT,"flats");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
            }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"FLATS",&pwad,
	      Picture,WSafe,cusage)!=TRUE)
          { if(strncmp(pwad.dir[p].name,"F_SKY1",6)!=0)
            Warning("failed to write flat %s", lump_name (pwad.dir[p].name));
          }
          else
            TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
        }
      }
    }
  }

  /* Extract all sneaps */
  if (select & BSNEAP)
  {
    ENTRY type = ESNEAP;
    Phase ("Extracting %s...\n", entry_type_plural (type));
    ostart = 0x80000000L;
    osize = 0;

    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if(piden[p] == type)
      { if(EntryFound!=TRUE)
	{
	   char comment[40];
	   if (cusage == NULL)
	      MakeDir(file,DataDir,entry_type_dir(type),"");
           TXTaddEmptyLine (TXT);
	   sprintf (comment, "List of %.20s", entry_type_plural (type));
	   TXTaddComment (TXT, comment);
	   TXTaddSection (TXT, entry_type_section (type));
	   EntryFound=TRUE;
	}
	if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
	{
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
	}
	else
	{ ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
	  if(XTRbmpSave(&insrX,&insrY,&pdir[p],entry_type_pictype (type),
	      DataDir, entry_type_dir (type),&pwad, Picture, WSafe,
	      cusage) != TRUE)
	  {
	    Warning("failed to write %.20s %s",
		entry_type_name (type), lump_name (pwad.dir[p].name));
	  }
	  else
	  {
	    TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,
		FALSE);
	  }
	}
      }
    }
  }

  /* Extract all sneats */
  if (select & BSNEAT)
  {
    ENTRY type = ESNEAT;
    Phase ("Extracting %s...\n", entry_type_plural (type));
    ostart = 0x80000000L;
    osize = 0;

    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if(piden[p] == type)
      { if(EntryFound!=TRUE)
	{
	   char comment[40];
	   if (cusage == NULL)
	      MakeDir(file,DataDir,entry_type_dir(type),"");
           TXTaddEmptyLine (TXT);
	   sprintf (comment, "List of %.20s", entry_type_plural (type));
	   TXTaddComment (TXT, comment);
	   TXTaddSection (TXT, entry_type_section (type));
	   EntryFound=TRUE;
	}
	if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
	{
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
	}
	else
	{ ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
	  if(XTRbmpSave(&insrX,&insrY,&pdir[p],entry_type_pictype (type),
	      DataDir, entry_type_dir (type),&pwad, Picture, WSafe,
	      cusage) != TRUE)
	  {
	    Warning("failed to write %.20s %s",
		entry_type_name (type), lump_name (pwad.dir[p].name));
	  }
	  else
	  {
	    TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,
		FALSE);
	  }
	}
      }
    }
  }

  /* If -usedidx, print statistics */
  if (cusage != NULL)
  {
    int n;

    printf ("Npixels    Idx  Nlumps  First lump\n");
    printf ("---------  ---  ------  ----------\n");
    for (n = 0; n < NCOLOURS; n++)
      printf ("%9lu  %3d  %5lu   %s\n",
	  cusage->uses[n],
	  n,
	  cusage->nlumps[n],
	  (cusage->uses[n] == 0) ? "" : lump_name (cusage->where_first[n]));
    putchar ('\n');
  }

  /*
  ** exit graphics and end
  */
  COLfree();
  Free(piden);
  WADRclose(&pwad);
  TXTaddEmptyLine (TXT);
  TXTaddComment(TXT,"End of extraction");
  TXTcloseW(TXT);
  Phase("End of extraction.\n");
}

/*********** End Xtract Module ***************/


void XTRgetEntry(const char *doomwad, const char *DataDir, const char *wadin,
    const char *entry, IMGTYPE Picture,SNDTYPE Sound,Bool fullSND, char trnR,
    char trnG, char trnB)
{ static struct WADINFO pwad;
  static struct WADINFO iwad;
  static char Name[8];
  Int16 e;
  char  *Entry; Int32 Entrysz;
  char  *Colors=NULL;
  Int16 insrX,insrY;
  char *extens=NULL;
  Bool Found=FALSE;

  Normalise(Name,entry);
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
   /*find PLAYPAL*/
  e=WADRfindEntry(&iwad,"PLAYPAL");
  if(e>=0) Colors=WADRreadEntry(&iwad,e,&Entrysz);
  else  ProgError("Can't find PLAYPAL in main WAD");
  WADRclose(&iwad);
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  e=WADRfindEntry(&pwad,"PLAYPAL");
  if(e>=0)
  {  Free(Colors);
     Colors=WADRreadEntry(&pwad,e,&Entrysz);
  }
  COLinit(trnR,trnG,trnB,Colors,(Int16)Entrysz);
  Free(Colors);
  e=WADRfindEntry(&pwad,Name);
  if(e<0) ProgError("Can't find entry %s in WAD", lump_name (Name));
  Phase("Extracting entry %s from WAD %s\n", lump_name (entry), wadin);
  Entry=WADRreadEntry(&pwad,e,&Entrysz);
  /*try graphic*/
  if(Found!=TRUE)
    if(Entrysz>8)
    { switch(Picture)
      { case PICGIF: extens="GIF";break;
        case PICBMP: extens="BMP";break;
        case PICPPM: extens="PPM";break;
        case PICTGA: extens="TGA";break;
        default: Bug("img type");
      }
      MakeFileName(file,DataDir,"","",Name,extens);
      if(PICsaveInFile(file,PGRAPH,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==TRUE)
      { Info("Picture insertion point is (%d,%d)",insrX,insrY);
        Found=TRUE;
      }
      else if((Entrysz==0x1000)||(Entrysz==0x1040))
      { if(PICsaveInFile(file,PFLAT,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==TRUE)
        { Found=TRUE;
        }
      }
      else if(Entrysz==64000L)
      { if(PICsaveInFile(file,PLUMP,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==TRUE)
        { Found=TRUE;
        }
      }
    }
  if (Found!=TRUE)
    if (peek_i16_le (Entry) == 3)
      if (Entrysz >= 8 + peek_i32_le (Entry + 4))
      { /*save as sound*/
        switch(Sound)
        { case SNDAU:  extens="AU";break;
          case SNDWAV: extens="WAV";break;
          case SNDVOC: extens="VOC";break;
          default: Bug("snd type");
        }
        MakeFileName(file,DataDir,"","",Name,extens);
        SNDsaveSound(file,Entry,Entrysz,Sound,fullSND, Name);
        Found=TRUE;
      }
  if(Found!=TRUE)
  { /*save as lump*/
    MakeFileName(file,DataDir,"","",Name,"LMP");
    WADRsaveEntry(&pwad,e,file);
  }
  Free(Entry);
  WADRclose(&pwad);
}


#endif /*DeuTex*/

