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

/*compile only for DeuTex*/
#if defined DeuTex

/************** Begin XTRACT WAD module **************/
/*
** here we go for some real indecent programming. sorry
*/

extern char file[128];

#if 0
static void stop (void)
{
  ;
}
#endif

/*
** try to save entry as BitMap .BMP
*/
static Bool XTRbmpSave(Int16 *pinsrX,Int16 *pinsrY,struct WADDIR huge *entry,
		 PICTYPE type,const char *DataDir,const char *dir,struct
		 WADINFO *info,IMGTYPE Picture,Bool WSafe)
{  Bool res;
   Int32 start=entry->start;
   Int32 size =entry->size;
   char *name=entry->name;
   char huge *buffer;
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
   buffer=(char huge *)Malloc(size);
   WADRseek(info,start);
   WADRreadBytes(info,buffer,size);
   /* DEBUG */
#if 0
   if (! strncmp (name, "PSYBA0", 8))
     stop ();
#endif
   res = PICsaveInFile(file,type,buffer,size,pinsrX,pinsrY,Picture);
   if(res==TRUE)Detail("Saved picture as %s\n",file);
   Free(buffer);
   return res;
}
/*
** extract entries from a WAD
*/
void XTRextractWAD(const char *doomwad, const char *DataDir, const char
    *wadin, const char *wadinfo, IMGTYPE Picture,SNDTYPE Sound,Bool
    fullSND,NTRYB select, char trnR, char trnG, char trnB,Bool WSafe)
{ static struct WADINFO pwad;
  static struct WADINFO iwad;
  static struct WADINFO lwad;
  struct WADDIR huge *pdir;
  Int16 pnb;
  ENTRY huge *piden;
  Int16 p;
  Int32 ostart,osize;
  char huge *buffer;
  char huge *Colors=NULL;
  Bool res;
  Int16 insrX=0,insrY=0;
  Bool EntryFound;
  /*PNAMES*/
  Int16 pnm;char huge *Pnam;Int32 Pnamsz;
  char *extens=NULL;
  /*text file to write*/
  static struct TXTFILE *TXT;
  Phase("Extracting entries from WAD %s\n",wadin);
  /*open iwad,get iwad directory*/
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);

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
  **
  ** prepare for graphics
  */
  /*find PLAYPAL*/
  pnm=WADRfindEntry(&pwad,"PLAYPAL");
  if(pnm>=0)
    Colors=WADRreadEntry(&pwad,pnm,&Pnamsz);
  else
  { pnm=WADRfindEntry(&iwad,"PLAYPAL");
    if(pnm>=0)
      Colors=WADRreadEntry(&iwad,pnm,&Pnamsz);
    else  ProgError("Can't find PLAYPAL in main WAD");
  }
  COLinit(trnR,trnG,trnB,Colors,(Int16)Pnamsz);
  Free(Colors);
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
  ** and save entries depending on their type
  */

  /*check if file exists*/
  TXT=TXTopenW(wadinfo);
  TXTaddComment(TXT," DeuTex by Olivier Montanuy");
  TXTaddComment(TXT," PWAD creation directives");
  TXTaddComment(TXT,"");
  /*
  ** LEVELS
  */
  if(select&BLEVEL)
  { Phase("Extracting Levels...\n");
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { switch(piden[p]&EMASK)
      { case ELEVEL: case EMAP:
        if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"LEVELS","");
           TXTaddComment(TXT,"List of levels");
           TXTaddSection(TXT,"LEVELS");
           EntryFound=TRUE;
        }
        /* entries to save in WAD, named from Level name*/
        res=MakeFileName(file,DataDir,"LEVELS","",pdir[p].name,"WAD");
        if((WSafe==TRUE)&&(res==TRUE))
          Warning("will not overwrite file %s",file);
        else
        { WADRopenW(&lwad,file,PWAD);
          ostart=WADRposition(&lwad);/*BC++ 4.5 bug*/
          WADRdirAddEntry(&lwad,ostart,0L,pdir[p].name);
          WADRwriteWADlevelParts(&lwad,&pwad,p);
          WADRwriteDir(&lwad);
          WADRclose(&lwad);
        }
        TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
        p+=11-1;
      }
    }
  }


  /*
  ** LUMPS
  */
  if(select&BLUMP)
  { Phase("Extracting Lumps...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ELUMP)
      { if(EntryFound!=TRUE)
        { MakeDir(file,DataDir,"LUMPS","");
          TXTaddComment(TXT,"List of data Lumps");
          TXTaddSection(TXT,"LUMPS");
          EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          res=FALSE;
          if(osize==64000L)/*lumps that are pics*/
          { res=XTRbmpSave(&insrX,&insrY,&pdir[p],PLUMP,DataDir,"LUMPS",&pwad,Picture,WSafe);
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
      TXTaddComment(TXT,"List of definitions for TEXTURE1");
      TXTaddSection(TXT,"TEXTURE1");
      
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
      { buffer=(char huge *)Malloc(pdir[p].size);
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
      TXTaddComment(TXT,"List of definitions for TEXTURE2");
      TXTaddSection(TXT,"TEXTURE2");
      TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
      res=MakeFileName(file,DataDir,"TEXTURES","",pdir[p].name,"TXT");
      if((WSafe==TRUE)&&(res==TRUE))
      {         Warning("will not overwrite file %s",file);
      }
      else
      { buffer=(char huge *)Malloc(pdir[p].size);
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
  { Phase("Extracting Sounds...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESOUND)
      { if(EntryFound!=TRUE)
        { MakeDir(file,DataDir,"SOUNDS","");
          TXTaddComment(TXT,"List of Sounds");
          TXTaddSection(TXT,"SOUNDS");
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
              { buffer=(char huge *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsavePCSound(file,buffer,pdir[p].size);
                Free(buffer);
                Detail("Saved PC Sound as   %s\n",file);
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
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
              { buffer=(char huge *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsaveSound(file,buffer,pdir[p].size,Sound,fullSND);
                Detail("Saved Sound as   %s\n",file);
                Free(buffer);
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
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
  { Phase("Extracting Musics...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EMUSIC)
      { if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"MUSICS","");
           TXTaddComment(TXT,"List of Musics");
           TXTaddSection(TXT,"MUSICS");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        {          TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
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
  { Phase("Extracting Graphics...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EGRAPHIC)
      { if(EntryFound!=TRUE)
        { MakeDir(file,DataDir,"GRAPHICS","");
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PGRAPH,DataDir,"GRAPHICS",&pwad,Picture,WSafe)==TRUE)
          { if(EntryFound!=TRUE)
            { TXTaddComment(TXT,"List of Pictures (with insertion point)");
              TXTaddSection(TXT,"GRAPHICS");
              EntryFound=TRUE;
            }
            TXTaddEntry(TXT,pdir[p].name,NULL,insrX,insrY,FALSE,TRUE);
          }
          else if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"LUMPS",&pwad,Picture,WSafe)==TRUE)
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
  { Phase("Extracting Sprites...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESPRITE)
      { if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"SPRITES","");
           TXTaddComment(TXT,"List of Sprites");
           TXTaddSection(TXT,"SPRITES");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PSPRIT,DataDir,"SPRITES",&pwad,Picture,WSafe)!=TRUE)
          { Warning("failed to write sprite %.8s",pwad.dir[p].name);
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
  { Phase("Extracting Patches...\n");
    for(EntryFound=FALSE,p=0;p<pnb;p++)
     { if((piden[p] & EMASK)==EPATCH)
       { if(EntryFound!=TRUE)
         { MakeDir(file,DataDir,"PATCHES","");
           TXTaddComment(TXT,"List of wall patches");
           TXTaddSection(TXT,"PATCHES");
           EntryFound=TRUE;
         }
         if(XTRbmpSave(&insrX,&insrY,&pdir[p],PPATCH,DataDir,"PATCHES",&pwad,Picture,WSafe)==TRUE)
         { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
         }
         else
         { Warning("failed to write patch %.8s",pwad.dir[p].name);
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
  { Phase("Extracting Flats...\n");
    ostart=0x80000000L;osize=0;
    for(EntryFound=FALSE,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EFLAT)
      { if(EntryFound!=TRUE)
        {  MakeDir(file,DataDir,"FLATS","");
           TXTaddComment(TXT,"List of Floors and Ceilings");
           TXTaddSection(TXT,"FLATS");
           EntryFound=TRUE;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,TRUE,FALSE);
            }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"FLATS",&pwad,Picture,WSafe)!=TRUE)
          { if(strncmp(pwad.dir[p].name,"F_SKY1",6)!=0)
            Warning("failed to write flat %.8s",pwad.dir[p].name);
          }
          else
            TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,FALSE,FALSE);
        }
      }
    }
  }
  /*
  ** exit graphics and end
  */
  COLfree();
  Free(piden);
  WADRclose(&pwad);
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
  char huge *Entry; Int32 Entrysz;
  char huge *Colors=NULL;
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
  if(e<0) ProgError("Can't find entry %.8s in WAD",Name);
  Phase("Extracting entry %.8s from WAD %s\n",entry,wadin);
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
      if(PICsaveInFile(file,PGRAPH,Entry,Entrysz,&insrX,&insrY,Picture)==TRUE)
      { Info("Picture insertion point is (%d,%d)",insrX,insrY);
        Found=TRUE;
      }
      else if((Entrysz==0x1000)||(Entrysz==0x1040))
      {        if(PICsaveInFile(file,PFLAT,Entry,Entrysz,&insrX,&insrY,Picture)==TRUE)
        { Found=TRUE;
        }
      }
      else if(Entrysz==64000L)
      {        if(PICsaveInFile(file,PLUMP,Entry,Entrysz,&insrX,&insrY,Picture)==TRUE)
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
        SNDsaveSound(file,Entry,Entrysz,Sound,fullSND);
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

