/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2017 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.


  SPDX-License-Identifier: GPL-2.0+
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
#include "sscript.h"
#include "usedidx.h"


/************** Begin XTRACT WAD module **************/
/*
** here we go for some real indecent programming. sorry
*/

extern char file[128];

/*
** try to save entry as BitMap .BMP
*/
static bool XTRbmpSave(int16_t *pinsrX,int16_t *pinsrY,struct WADDIR  *entry,
		 PICTYPE type,const char *DataDir,const char *dir,struct
		 WADINFO *info,IMGTYPE Picture,bool WSafe, cusage_t *cusage)
{  bool res;
   int32_t start=entry->start;
   int32_t size =entry->size;
   char *name=entry->name;
   char  *buffer;
   char *extens=NULL;

   if(size<8L) return false;
   switch(Picture)
   { 
	 case PICPNG: extens="PNG";break;
	 case PICGIF: extens="GIF";break;
     case PICBMP: extens="BMP";break;
     case PICPPM: extens="PPM";break;
     default: Bug("EX47", "Invalid img type %d", (int) Picture);
   }
   res = MakeFileName(file,DataDir,dir,"",name,extens);
   if((WSafe==true)&&(res==true))
   { Warning("EX48", "Will not overwrite file %s",file);
     return true;
   }
   buffer=(char  *)Malloc(size);
   WADRseek(info,start);
   WADRreadBytes(info,buffer,size);
   res = PICsaveInFile(file,type,buffer,size,pinsrX,pinsrY,Picture, name,
       cusage);
   if(res==true)Detail("EX49", "Saved picture as %s", fname (file));
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
    *wadin, const char *wadinfo, IMGTYPE Picture,SNDTYPE Sound,
    NTRYB select, char trnR, char trnG, char trnB,bool WSafe,
    cusage_t *cusage)
{ static struct WADINFO pwad;
  static struct WADINFO iwad;
  static struct WADINFO lwad;
  struct WADDIR  *pdir;
  int16_t pnb;
  ENTRY  *piden;
  int16_t p;
  int32_t ostart,osize;
  char  *buffer;
  bool res;
  int16_t insrX=0,insrY=0;
  bool EntryFound;
  char *extens=NULL;
  /*text file to write*/
  static struct TXTFILE *TXT = NULL;
  Phase("EX00", "Extracting entries from wad %s", wadin);
  /*open iwad,get iwad directory*/
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);

  /* If -usedidx, we're only interested in graphics. */
  if (cusage != NULL)
     select &= (BGRAPHIC | BSPRITE | BPATCH | BFLAT | BSNEAP | BSNEAT | BWALL);

  /*read WAD*/
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  pdir=pwad.dir;
  pnb=(int16_t)pwad.ntry;

  /*find PNAMES*/
  { int16_t pnm=WADRfindEntry(&iwad,"PNAMES");
    char *Pnam=NULL;
    int32_t Pnamsz=0;
    if(pnm<0)
      Warning("EX01", "Iwad: no PNAMES lump");
    else
      Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
    piden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
    if(Pnam!=NULL)
      Free(Pnam);
  }

  /*
  ** prepare for graphics
  */

  /* Read PLAYPAL */
  {
    const char     *lumpname = palette_lump;
    struct WADINFO *wad;
    int16_t           lumpnum;
    char           *lumpdata = NULL;
    int32_t           lumpsz;

    wad = &pwad;
    lumpnum = WADRfindEntry (wad, lumpname);
    if (lumpnum >= 0)
      lumpdata = WADRreadEntry (wad, lumpnum, &lumpsz);
    else
    {
      wad = &iwad;
      lumpnum = WADRfindEntry (wad, lumpname);
      if (lumpnum >= 0)
	lumpdata = WADRreadEntry (wad, lumpnum, &lumpsz);
      else
      {
	long n;

	wad = NULL;
	lumpdata = Malloc (768);
	Warning ("EX02", "No %s lump found, making up a palette", lumpname);
	for (n = 0; n < 256; n++)
	{
	  lumpdata[3*n]   = n;
	  lumpdata[3*n+1] = (n & 0x7f) << 1;
	  lumpdata[3*n+2] = (n & 0x3f) << 2;
	}
      }
    }
    COLinit (trnR, trnG, trnB, lumpdata, (int16_t) lumpsz,
	(wad == NULL) ? "(nofile)" : wad->filename, lumpname);
    Free (lumpdata);
  }

  /* If TITLEPAL exists, read the first 768 bytes of it. But
     don't prepare COLpal2 because we don't know yet whether we
     need it. Indeed, if there are no sneats to extract, we're
     not interested in seeing any TITLEPAL-related warnings. */
  {
    int n;
    char *titlepal_data = NULL;
    int32_t titlepal_size = 3 * NCOLOURS;

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
  do
  { int16_t pnm=WADRfindEntry(&pwad,"PNAMES");
    char *Pnam;
    int32_t lumpsz;
    if(pnm>=0)
      Pnam=WADRreadEntry(&pwad,pnm,&lumpsz);
    else
    { pnm=WADRfindEntry(&iwad,"PNAMES");
      if(pnm<0)
      { Warning("EX03", "Iwad: no PNAMES lump (2)");
	break;
      }
      Pnam=WADRreadEntry(&iwad,pnm,&lumpsz);
    }
    PNMinit(Pnam,lumpsz);
    Free(Pnam);
  }
  while (0);

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
       sprintf (comment, PACKAGE_NAME " %.32s by Olivier Montanuy", PACKAGE_VERSION);
       TXTaddComment (TXT, comment);
     }
     TXTaddComment(TXT,"PWAD creation directives");
  }

  /*
  ** LEVELS
  */
  if(select&BLEVEL)
  { Phase("EX10", "Extracting levels...");
    for(EntryFound=false,p=0;p<pnb;p++)
    { switch(piden[p]&EMASK)
      { case ELEVEL: case EMAP:
        if(EntryFound!=true)
        {  MakeDir(file,DataDir,"LEVELS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of levels");
           TXTaddSection(TXT,"levels");
           EntryFound=true;
        }
        /* entries to save in WAD, named from Level name*/
	{
	  int pmax;
	  for (pmax = p; pmax < pnb && piden[pmax] == piden[p]; pmax++)
	    ;
	  res=MakeFileName(file,DataDir,"LEVELS","",pdir[p].name,"WAD");
	  if((WSafe==true)&&(res==true))
	    Warning("EX11", "Will not overwrite file %s",file);
	  else
	  { WADRopenW(&lwad,file,PWAD, 0);
	    ostart=WADRposition(&lwad);/*BC++ 4.5 bug*/
	    WADRdirAddEntry(&lwad,ostart,0L,pdir[p].name);
	    WADRwriteWADlevelParts (&lwad, &pwad, p, pmax - p);
	    WADRwriteDir(&lwad, 0);
	    WADRclose(&lwad);
	  }
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
	  p = pmax - 1;
	}
      }
    }
  }


  /*
  ** LUMPS
  */
  if(select&BLUMP)
  { Phase("EX15", "Extracting lumps...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ELUMP)
      { if(EntryFound!=true)
        { MakeDir(file,DataDir,"LUMPS","");
	  TXTaddEmptyLine (TXT);
          TXTaddComment(TXT,"List of data Lumps");
          TXTaddSection(TXT,"lumps");
          EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          res=false;
          if(osize==64000L)/*lumps that are pics*/
          { res=XTRbmpSave(&insrX,&insrY,&pdir[p],PLUMP,DataDir,"LUMPS",&pwad,
	      Picture,WSafe, cusage);
          }
          if(res!=true)   /*normal lumps*/
          { res=MakeFileName(file,DataDir,"LUMPS","",pdir[p].name,"LMP");
            if((WSafe==true)&&(res==true))
            {  Warning("EX16", "Will not overwrite file %s",file);
            }
            else
            { WADRsaveEntry(&pwad,p,file);
              Detail("EX17", "Saved lump as   %s", fname (file));
            }
          }
          TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
        }
      }
    }
  }

  /*
  ** TEXTURES
  */
  if(select&BTEXTUR)
  { EntryFound=false;
    for(p=0;p<pnb;p++)
    { if(piden[p]==ETEXTUR+1)
      { if(EntryFound!=true)
	{  MakeDir(file,DataDir,"TEXTURES","");
	   EntryFound=true;
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
	   TXTaddEntry(TXT,name,NULL,INVALIDINT,INVALIDINT,false,false);
	   res=MakeFileName(file,DataDir,"TEXTURES","",name,"TXT");
	}
	if((WSafe==true)&&(res==true))
	{         Warning("EX21", "Will not overwrite file %s",file);
	}
	else
	{ buffer=(char  *)Malloc(pdir[p].size);
	  WADRseek(&pwad,pdir[p].start);
	  WADRreadBytes(&pwad,buffer,pdir[p].size);
	  TXUinit();
	  TXUreadTEXTURE(pdir[p].name, buffer, pdir[p].size, NULL, 0, true);
	  Free(buffer);
	  TXUwriteTexFile(file);
	  TXUfree();
	}
      }
    }
    for(p=0;p<pnb;p++)
    { if(piden[p]==ETEXTUR+2)
      { if(EntryFound!=true)
	{  MakeDir(file,DataDir,"TEXTURES","");
	   EntryFound=true;
	}
	TXTaddEmptyLine (TXT);
	TXTaddComment(TXT,"List of definitions for TEXTURE2");
	TXTaddSection(TXT,"texture2");
	TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
	res=MakeFileName(file,DataDir,"TEXTURES","",pdir[p].name,"TXT");
	if((WSafe==true)&&(res==true))
	{         Warning("EX22", "Will not overwrite file %s",file);
	}
	else
	{ buffer=(char  *)Malloc(pdir[p].size);
	  WADRseek(&pwad,pdir[p].start);
	  WADRreadBytes(&pwad,buffer,pdir[p].size);
	  TXUinit();
	  TXUreadTEXTURE(pdir[p].name, buffer, pdir[p].size, NULL, 0, true);
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
  { Phase("EX25", "Extracting sounds...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESOUND)
      { if(EntryFound!=true)
        { MakeDir(file,DataDir,"SOUNDS","");
          TXTaddEmptyLine (TXT);
          TXTaddComment(TXT,"List of Sounds");
          TXTaddSection(TXT,"sounds");
          EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          switch(piden[p])
          { case ESNDPC:
              res=MakeFileName(file,DataDir,"SOUNDS","",pdir[p].name,"TXT");
              if((WSafe==true)&&(res==true))
              {  Warning("EX26", "Will not overwrite file %s",file);
              }
              else
              {
		char name[33];
		strcpy(name, lump_name(pdir[p].name));
		buffer=(char  *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsavePCSound(name, file, buffer, pdir[p].size);
                Free(buffer);
                Detail("EX27", "Saved PC sound as   %s", fname (file));
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,
		  false);
              break;
             case ESNDWAV:
                 switch (Sound) {
                 case SNDWAV:
                     extens = "WAV";
                     break;
                 default:
                     Bug("EX28", "Invalid snd type %d", Sound);
                 }
              res=MakeFileName(file,DataDir,"SOUNDS","",pdir[p].name,extens);
              if((WSafe==true)&&(res==true))
              { Warning("EX29", "Will not overwrite file %s", fname (file));
              }
              else
              { buffer=(char  *)Malloc(pdir[p].size);
                WADRseek(&pwad,pdir[p].start);
                WADRreadBytes(&pwad,buffer,pdir[p].size);
                SNDsaveSound(file,buffer,pdir[p].size,Sound,pdir[p].name);
                Detail("EX30", "Saved sound as   %s", fname (file));
                Free(buffer);
              }
              TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,
		  false);
              break;
            default:
              Bug("EX31", "Invalid snd type %d", piden[p]);
          }
        }
      }
    }
  }

  /*
  ** MUSICS
  */
  if(select&BMUSIC)
  { Phase("EX32", "Extracting musics...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EMUSIC)
      { if(EntryFound!=true)
        {  MakeDir(file,DataDir,"MUSICS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Musics");
           TXTaddSection(TXT,"musics");
           EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          res=MakeFileName(file,DataDir,"MUSICS","",pdir[p].name,"MUS");
          if((WSafe==true)&&(res==true))
          {  Warning("EX33", "Will not overwrite file %s", fname (file));
          }
          else
          {
	    Detail("EX34", "Saving music as %s", fname (file));
	    WADRsaveEntry(&pwad,p,file);
          }
          TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
        }
      }
    }
  }

  /*
  ** GRAPHICS
  */
  if(select&BGRAPHIC)
  { Phase("EX35", "Extracting graphics...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EGRAPHIC)
      { if(EntryFound!=true && cusage == NULL)
        { MakeDir(file,DataDir,"GRAPHICS","");
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PGRAPH,DataDir,"GRAPHICS",&pwad,
	      Picture,WSafe, cusage)==true)
          { if(EntryFound!=true)
            { 
              TXTaddEmptyLine (TXT);
	      TXTaddComment(TXT,"List of Pictures (with insertion point)");
              TXTaddSection(TXT,"graphics");
              EntryFound=true;
            }
            TXTaddEntry(TXT,pdir[p].name,NULL,insrX,insrY,false,true);
          }
          else if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"LUMPS",&pwad,
	      Picture,WSafe, cusage)==true)
          { /*Was saved as graphic lump*/
	    char *name = Malloc (sizeof pdir[p].name + 1);
	    sprintf (name, "%.*s", (int) sizeof pdir[p].name, pdir[p].name);
	    TXTaddComment(TXT, name);
	    Free (name);
          }
          else if (cusage == NULL)
          { if(MakeFileName(file,DataDir,"LUMPS","",pdir[p].name,"LMP")==true)
            {  Warning("EX36", "Will not overwrite file %s", fname (file));
            }
            else
            { WADRsaveEntry(&pwad,p,file);
              Detail("EX37", "Saved lump as   %s", fname (file));
            }
	    {
	      char *name = Malloc (sizeof pdir[p].name + 1);
	      sprintf (name, "%.*s", (int) sizeof pdir[p].name, pdir[p].name);
	      TXTaddComment(TXT, name);
	      Free (name);
	    }
          }
        }
      }
    }
  }

  /*
  ** SPRITES
  */
  if(select&BSPRITE)
  { Phase("EX40", "Extracting sprites...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==ESPRITE)
      { if(EntryFound!=true)
        {  if (cusage == NULL)
	      MakeDir(file,DataDir,"SPRITES","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Sprites");
           TXTaddSection(TXT,"sprites");
           EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
        }
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PSPRIT,DataDir,"SPRITES",&pwad,
	      Picture,WSafe, cusage)!=true)
          { Warning("EX41", "Failed to write sprite %s",
	      lump_name (pwad.dir[p].name));
          }
          else
          { TXTaddEntry(TXT,pdir[p].name,NULL,insrX,insrY,false,true);
          }
        }
      }
    }
  }

  /*
  ** PATCHES
  */
  if (select & BPATCH)
  { Phase("EX45", "Extracting patches...");
    for(EntryFound=false,p=0;p<pnb;p++)
     { if((piden[p] & EMASK)==EPATCH)
       { if(EntryFound!=true)
         { 
	   if (cusage == NULL)
	      MakeDir(file,DataDir,"PATCHES","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of patches");
           TXTaddSection(TXT,"patches");
           EntryFound=true;
         }
         if(XTRbmpSave(&insrX,&insrY,&pdir[p],PPATCH,DataDir,"PATCHES",&pwad,
	     Picture,WSafe,cusage)==true)
         { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
         }
         else
         { Warning("EX46", "Failed to write patch %s",
	     lump_name (pwad.dir[p].name));
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
  if (select & BFLAT)
  { Phase("EX50", "Extracting flats...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EFLAT)
      { if(EntryFound!=true)
        {  if (cusage == NULL)
	      MakeDir(file,DataDir,"FLATS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of Floors and Ceilings");
           TXTaddSection(TXT,"flats");
           EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
	}
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PFLAT,DataDir,"FLATS",&pwad,
	      Picture,WSafe,cusage)!=true)
          { if(strncmp(pwad.dir[p].name,"F_SKY1",6)!=0)
            Warning("EX51", "Failed to write flat %s",
		lump_name (pwad.dir[p].name));
          }
          else
            TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
        }
      }
    }
  }

  /* Extract all sneaps */
  if (select & BSNEAP)
  {
    ENTRY type = ESNEAP;
    Phase ("EX55", "Extracting %s...", entry_type_plural (type));
    ostart = 0x80000000L;
    osize = 0;

    for(EntryFound=false,p=0;p<pnb;p++)
    { if(piden[p] == type)
      { if(EntryFound!=true)
	{
	   char comment[40];
	   if (cusage == NULL)
	      MakeDir(file,DataDir,entry_type_dir(type),"");
           TXTaddEmptyLine (TXT);
	   sprintf (comment, "List of %.20s", entry_type_plural (type));
	   TXTaddComment (TXT, comment);
	   TXTaddSection (TXT, entry_type_section (type));
	   EntryFound=true;
	}
	if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
	{
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
	}
	else
	{ ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
	  if(XTRbmpSave(&insrX,&insrY,&pdir[p],entry_type_pictype (type),
	      DataDir, entry_type_dir (type),&pwad, Picture, WSafe,
	      cusage) != true)
	  {
	    Warning("EX56", "Failed to write %.20s %s",
		entry_type_name (type), lump_name (pwad.dir[p].name));
	  }
	  else
	  {
	    TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,
		false);
	  }
	}
      }
    }
  }

  /* Extract all sneats */
  if (select & BSNEAT)
  {
    ENTRY type = ESNEAT;
    Phase ("EX60", "Extracting %s...", entry_type_plural (type));
    ostart = 0x80000000L;
    osize = 0;

    for(EntryFound=false,p=0;p<pnb;p++)
    { if(piden[p] == type)
      { if(EntryFound!=true)
	{
	   char comment[40];
	   if (cusage == NULL)
	      MakeDir(file,DataDir,entry_type_dir(type),"");
           TXTaddEmptyLine (TXT);
	   sprintf (comment, "List of %.20s", entry_type_plural (type));
	   TXTaddComment (TXT, comment);
	   TXTaddSection (TXT, entry_type_section (type));
	   EntryFound=true;
	}
	if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
	{
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
	}
	else
	{ ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
	  if(XTRbmpSave(&insrX,&insrY,&pdir[p],entry_type_pictype (type),
	      DataDir, entry_type_dir (type),&pwad, Picture, WSafe,
	      cusage) != true)
	  {
	    Warning("EX61", "Failed to write %.20s %s",
		entry_type_name (type), lump_name (pwad.dir[p].name));
	  }
	  else
	  {
	    TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,
		false);
	  }
	}
      }
    }
  }

  /* Extract all Strife scripts */
  if (select & BSCRIPT)
  {
    ENTRY type = ESSCRIPT;
    Phase ("EX65", "Extracting %s...", entry_type_plural (type));
    ostart = 0x80000000L;
    osize = 0;

    for(EntryFound=false,p=0;p<pnb;p++)
    { if(piden[p] == type)
      { if(EntryFound!=true)
	{
	   char comment[40];
	   if (cusage == NULL)
	      MakeDir(file,DataDir,entry_type_dir(type),"");
           TXTaddEmptyLine (TXT);
	   sprintf (comment, "List of %.20s", entry_type_plural (type));
	   TXTaddComment (TXT, comment);
	   TXTaddSection (TXT, entry_type_section (type));
	   EntryFound=true;
	}
	if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
	{
	  TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
	}
	else
	{ ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
	  { res = MakeFileName (file, DataDir, entry_type_dir(type), "",
	      pdir[p].name, "txt");
            if (WSafe==true && res==true)
            {  Warning("EX66", "Will not overwrite file %s",file);
            }
            else
            { if (sscript_save (&pwad, p, file))
	      {
		Warning("EX67", "Failed to write %.20s %s",
		  entry_type_name (type), lump_name (pwad.dir[p].name));
	      }
	      else
	      {
		TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,
		false);
	      }
            }
          }
	}
      }
    }
  }

  /* Extract all ROTT walls. They're raw 64x64 bitmaps, between
     WALLSTRT and WALLSTOP. This is based on the regular Doom
     flats extraction code above. */
  if (ROTT && (select & BWALL))
  { Phase("EX70", "Extracting walls...");
    ostart=0x80000000L;osize=0;
    for(EntryFound=false,p=0;p<pnb;p++)
    { if((piden[p]&EMASK)==EWALL)
      { if(EntryFound!=true)
        {  if (cusage == NULL)
	      MakeDir(file,DataDir,"WALLS","");
           TXTaddEmptyLine (TXT);
           TXTaddComment(TXT,"List of walls");
           TXTaddSection(TXT,"walls");
           EntryFound=true;
        }
        if((ostart==pwad.dir[p].start)&&(osize==pwad.dir[p].size))
        { TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,true,false);
	}
        else
        { ostart=pwad.dir[p].start; osize=pwad.dir[p].size;
          if(XTRbmpSave(&insrX,&insrY,&pdir[p],PWALL,DataDir,"WALLS",&pwad,
	      Picture,WSafe,cusage)!=true)
	    Warning("EX71", "Failed to write wall %s",
		lump_name (pwad.dir[p].name));
          else
            TXTaddEntry(TXT,pdir[p].name,NULL,INVALIDINT,INVALIDINT,false,false);
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
  Phase("EX99", "End of extraction");
}

/*********** End Xtract Module ***************/


void XTRgetEntry(const char *doomwad, const char *DataDir, const char *wadin,
    const char *entry, IMGTYPE Picture,SNDTYPE Sound, char trnR,
    char trnG, char trnB)
{ static struct WADINFO pwad;
  static struct WADINFO iwad;
  static char Name[8];
  int16_t e;
  char  *Entry; int32_t Entrysz;
  char  *Colors=NULL;
  int16_t insrX,insrY;
  char *extens=NULL;
  bool Found=false;

  Normalise(Name,entry);
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
   /*find PLAYPAL*/
  e=WADRfindEntry(&iwad,palette_lump);
  if(e>=0)
    Colors=WADRreadEntry(&iwad,e,&Entrysz);
  else
    ProgError("GE00", "%s: no %s lump in the iwad",
      fname (iwad.filename), lump_name (palette_lump));
  WADRclose(&iwad);
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  e=WADRfindEntry(&pwad,palette_lump);
  if(e>=0)
  {  Free(Colors);
     Colors=WADRreadEntry(&pwad,e,&Entrysz);
  }
  COLinit(trnR,trnG,trnB,Colors,(int16_t)Entrysz, pwad.filename, palette_lump);
  Free(Colors);
  e=WADRfindEntry(&pwad,Name);
  if(e<0)
    ProgError("GE01", "%s: %s: lump not found",
	fname (pwad.filename), lump_name (entry));
  Phase("GE02", "%s: %s: extracting", fname (wadin), lump_name (entry));
  Entry=WADRreadEntry(&pwad,e,&Entrysz);
  /*try graphic*/
  if(Found!=true)
    if(Entrysz>8)
    { switch(Picture)
      { 
		case PICPNG: extens="PNG";break;
		case PICGIF: extens="GIF";break;
        case PICBMP: extens="BMP";break;
        case PICPPM: extens="PPM";break;
        case PICTGA: extens="TGA";break;
        default: Bug("GE03", "Invalid img type %d", (int) Picture);
      }
      MakeFileName(file,DataDir,"","",Name,extens);
      if(PICsaveInFile(file,PGRAPH,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==true)
      { Info("GE04", "Picture insertion point is (%d,%d)",insrX,insrY);
        Found=true;
      }
      /* FIXME try wall as well ? */
      else if((Entrysz==0x1000)||(Entrysz==0x1040))
      { if(PICsaveInFile(file,PFLAT,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==true)
        { Found=true;
        }
      }
      else if(Entrysz==64000L)
      { if(PICsaveInFile(file,PLUMP,Entry,Entrysz,&insrX,&insrY,Picture, Name,
	  NULL) ==true)
        { Found=true;
        }
      }
    }
  if (Found!=true)
    if (peek_i16_le (Entry) == 3)
      if (Entrysz >= 8 + peek_i32_le (Entry + 4))
      { /*save as sound*/
          switch (Sound) {
          case SNDWAV:
              extens = "WAV";
              break;
          default:
              Bug("GE05", "Invalid snd type %d", (int) Sound);
          }

          MakeFileName(file,DataDir,"","",Name,extens);
          SNDsaveSound(file,Entry,Entrysz,Sound,Name);
          Found=true;
      }
  if(Found!=true)
  { /*save as lump*/
    MakeFileName(file,DataDir,"","",Name,"LMP");
    WADRsaveEntry(&pwad,e,file);
  }
  Free(Entry);
  WADRclose(&pwad);
}
