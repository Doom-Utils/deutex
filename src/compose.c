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
#include "mkwad.h"
#include "texture.h"
#include "ident.h"
#include "color.h"
#include "picture.h"
#include "sound.h"
#include "text.h"

/*compile only for DeuTex*/
#if defined DeuTex

static void AddSomeJunk(const char *file);
/************Begin Tex2Ascii module *************************
**
** Translate TEXTURE1,TEXTURE2 and PNAME in
** a texture list for future modifications
*/
extern char file[128];

/***************End Tex2ascii module ******************/























/*
**
** make a PWAD from creation directives
** load levels,lumps,
**  create textures
** load sounds, pics, sprites, patches, flats,
*/


/*
** Can't handle PATCHES redefined from WAD
*/
Bool CMPOcopyFromWAD(Int32 *size,struct WADINFO *rwad,const char *DataDir,
    const char *Dir,const char *nam, const char *filenam)
{  static struct WADINFO pwad;
   Int16 entry;
   if(MakeFileName(file,DataDir,Dir,"",filenam,"WAD")!=TRUE)
     return FALSE;
   WADRopenR(&pwad,file);
   entry=WADRfindEntry(&pwad,nam);
   if(entry>=0)
   { *size=WADRwriteWADentry(rwad,&pwad,entry);
   }
   WADRclose(&pwad);
   if(entry<=0)return FALSE;
   return TRUE;
}
/*
** find a picture.
** must=TRUE is picture must exist
** returns picture type
*/
Int16 CMPOloadPic(Int32 *size,struct WADINFO *rwad, char *file, const
    char *DataDir,const char *Dir, const char *nam, const char *filenam, Int16
    Type, Int16 OfsX, Int16 OfsY)
{ int res=PICNONE;
  if(MakeFileName(file,DataDir,Dir,"",filenam,"ppm")==TRUE)
     res=PICPPM;
  else if(MakeFileName(file,DataDir,Dir,"",filenam,"bmp")==TRUE)
     res=PICBMP;
  else if(MakeFileName(file,DataDir,Dir,"",filenam,"gif")==TRUE)
  {
    static int gif_warning = 0;
    res=PICGIF;
    if (! gif_warning)
    {
      Warning ("GIF support may go away in the future (see");
      Warning ("http://lpf.ai.mit.edu/Patents/Gif/Gif.html).");
      Warning ("Switch to PPM or BMP.");
      gif_warning = 1;
    }
  }
  else if(CMPOcopyFromWAD(size,rwad,DataDir,Dir,nam,filenam)==TRUE)
     return PICWAD;
  if(res!=PICNONE)
    *size = PICsaveInWAD(rwad,file,Type,OfsX,OfsY,res);
  else if(Type!=PLUMP)
    Warning("Could not find file %s, .ppm, .bmp or .gif",file);
  return res;
}

struct WADINFO *CMPOrwad;
const char *CMPOwadout=NULL;
void CMPOerrorAction(void)
{ if(CMPOwadout==NULL) return;
  WADRclose(CMPOrwad); /*close file*/
  Unlink(CMPOwadout);  /*delete file*/
}



void CMPOmakePWAD(const char *doomwad,WADTYPE type, const char *PWADname,
		     const char *DataDir, const char *texin, NTRYB select,
		     char trnR, char trnG, char trnB, Bool George)
{  /*
   ** type PWAD as we are generating a real PWAD
   */
   Int32 start=0, size=0;
   static char name[8];
   static char filenam[8];
   /*PNAMES */
   Int16  nbPatchs,p;
   Bool NeedPNAME=FALSE;
   Bool FoundOne=FALSE;
   Bool Repeat;
   IMGTYPE Picture;
   /*optional insertion point*/
   Int16 X,Y;
   /*text file to read*/
   static struct TXTFILE *TXT;
   /*DOOM wad*/
   static struct WADINFO iwad,pwad;
   /*result wad file*/
   static struct WADINFO rwad;
   /*for Pnames*/
   Int16 entry;char  *EntryP;Int32 EntrySz=0;
   char  *Colors;
   /* initialisation*/

   Info("Translating %s into a %cWAD %s\n",texin,(type==IWAD)?'I':'P',PWADname);

   /*open iwad,get iwad directory*/
   iwad.ok=0;
   WADRopenR(&iwad,doomwad);

   TXT= TXTopenR(texin);
   WADRopenW(&rwad,PWADname,type, 1); 		/* fake IWAD or real PWAD */
   /*
   ** dirty: set error handler to delete the wad out file,
   ** if an error occurs.
   */
   CMPOrwad = &rwad;
   CMPOwadout = PWADname;
   ProgErrorAction(CMPOerrorAction);
   /*
   ** levels! add your own new levels to DOOM!
   ** read level from a PWAD file
   */
   if(select&BLEVEL)
   {  if(TXTseekSection(TXT,"LEVELS"))
      { while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ p=IDENTlevel(name);
	  if(p<0) ProgError("Illegal level name %s", lump_name (name));
	  if(MakeFileName(file,DataDir,"LEVELS","",filenam,"WAD")!=TRUE)
		ProgError("Can't find Level WAD %s",file);
	  Detail("Reading level WAD file %s\n",file);
	  WADRwriteWADlevel(&rwad,file,name);
	}
      }
   }
   /*
   ** prepare palette for graphics
   */
   /*find PLAYPAL*/
   if(select&(BGRAPHIC|BSPRITE|BPATCH|BFLAT))
   { /*should read playpal file if exist*/
     entry=WADRfindEntry(&iwad,"PLAYPAL");
     if(entry<0) ProgError("Can't find PLAYPAL in main WAD");
     Colors=WADRreadEntry(&iwad,entry,&EntrySz);
     COLinit(trnR,trnG,trnB,Colors,(Int16)EntrySz);
     Free(Colors);
   }
   /*
   **
   **   lumps. non graphic raw data for DOOM
   */
   if(select&BLUMP)
   {  start=size=0;
      if(TXTseekSection(TXT,"LUMPS"))
      { Phase("Making Lumps\n");
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{  if(Repeat!=TRUE)
	   { WADRalign4(&rwad);     /*align entry on Int32 word*/
	     start=WADRposition(&rwad);
	     if(MakeFileName(file,DataDir,"LUMPS","",filenam,"LMP")==TRUE)
	     { size=WADRwriteLump(&rwad,file);
	     }
	     else
	     { Picture=CMPOloadPic(&size,&rwad,file,DataDir,"LUMPS",name,
		 filenam,PLUMP,X,Y);
	       if(Picture==PICNONE)
		 if(CMPOcopyFromWAD(&size,&rwad,DataDir,"LUMPS",name,filenam)
		     !=TRUE)
		   ProgError("Can't find Lump or picture file %s.",file);
	     }
	   }
	   WADRdirAddEntry(&rwad,start,size,name);
	}
      }
   }
   /*
   ** initialise list of patch names
   */
   if(select&(BTEXTUR|BPATCH))
   { entry=WADRfindEntry(&iwad,"PNAMES");
     if(entry<0) ProgError("Can't find PNAMES in main WAD");
     EntryP=WADRreadEntry(&iwad,entry,&EntrySz);
     PNMinit(EntryP,EntrySz);
     Free(EntryP);
     NeedPNAME = FALSE;
   }
   /*
   ** read texture1
   */
   if(select&BTEXTUR)
   {  if(TXTseekSection(TXT,"TEXTURE1"))
      { Phase("Making Texture1\n");
	TXUinit();
	entry=WADRfindEntry(&iwad,"TEXTURE1");
	if(entry>=0)
	{ EntryP=WADRreadEntry(&iwad,entry,&EntrySz);
	  TXUreadTEXTURE(EntryP,EntrySz,NULL,0,TRUE);
	  Free(EntryP);
	}
	else Warning("Can't find TEXTURE1 in main WAD");
	FoundOne=FALSE;
	 /*read TEXTURES composing TEXTURE1*/
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ if(MakeFileName(file,DataDir,"TEXTURES","",name,"TXT")==TRUE)
	  { Detail("Reading texture file %s\n",file);
	    TXUreadTexFile(file,TRUE);
	    NeedPNAME=TRUE;
	    FoundOne=TRUE;
	  }
	  else if(MakeFileName(file,DataDir,"TEXTURES","",name,"WAD")==TRUE)
	  { Detail("Reading texture WAD %s\n",file);
	    WADRopenR(&pwad,file);
	    entry=WADRfindEntry(&pwad,"TEXTURE1");
	    if(entry>=0)
	    { EntryP=WADRreadEntry(&pwad,entry,&EntrySz);
	      TXUreadTEXTURE(EntryP,EntrySz,NULL,0,TRUE);
	      Free(EntryP);
	      NeedPNAME=TRUE;
	      FoundOne=TRUE;
	    }
	    WADRclose(&pwad);
	  }
	  else
	    ProgError("Can't find texture list %s",file);
	}
	/*write texture*/
	if(FoundOne==TRUE)
	{ WADRalign4(&rwad);     /*align entry on Int32 word*/
	  start= WADRposition(&rwad);
	  size = TXUwriteTEXTUREtoWAD(&rwad);
	  WADRdirAddEntry(&rwad,start,size,"TEXTURE1");
	}
	TXUfree();
      }
   }
   /*
   ** read texture2
   */
   if(select&BTEXTUR)
   {  if(TXTseekSection(TXT,"TEXTURE2"))
      { Phase("Making Texture2\n");
	TXUinit();
	entry=WADRfindEntry(&iwad,"TEXTURE2");
	if(entry>=0)
	{ EntryP=WADRreadEntry(&iwad,entry,&EntrySz);
	  TXUreadTEXTURE(EntryP,EntrySz,NULL,0,TRUE);
	  Free(EntryP);
	}
	else Warning("Can't find TEXTURE2 in main WAD");
	FoundOne=FALSE;
	 /*read TEXTURES composing TEXTURE2*/
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ if(MakeFileName(file,DataDir,"TEXTURES","",name,"TXT")==TRUE)
	  { Detail("Reading texture file %s\n",file);
	    TXUreadTexFile(file,TRUE);
	    NeedPNAME=TRUE;
	    FoundOne=TRUE;
	  }
	  else if(MakeFileName(file,DataDir,"TEXTURES","",name,"WAD")==TRUE)
	  { Detail("Reading texture WAD %s\n",file);
	    WADRopenR(&pwad,file);
	    entry=WADRfindEntry(&pwad,"TEXTURE2");
	    if(entry>=0)
	    { EntryP=WADRreadEntry(&pwad,entry,&EntrySz);
	      TXUreadTEXTURE(EntryP,EntrySz,NULL,0,TRUE);
	      Free(EntryP);
	      NeedPNAME=TRUE;
	      FoundOne=TRUE;
	    }
	    WADRclose(&pwad);
	  }
	  else
	    ProgError("Can't find texture list %s",file);
	}
	/*write texture*/
	if(FoundOne==TRUE)
	{ WADRalign4(&rwad);     /*align entry on Int32 word*/
	  start= WADRposition(&rwad);
	  size = TXUwriteTEXTUREtoWAD(&rwad);
	  WADRdirAddEntry(&rwad,start,size,"TEXTURE2");
	}
	TXUfree();
      }
   }
   /*
   ** PNAME
   */
   if(select&BTEXTUR)
   {  if(NeedPNAME)    /*write PNAME in PWAD*/
      { /*write entry PNAME*/
	Phase("Making Pnames\n");
	WADRalign4(&rwad);     /*align entry on Int32 word*/
	start=WADRposition(&rwad);
	size =PNMwritePNAMEtoWAD(&rwad);
	WADRdirAddEntry(&rwad,start,size,"PNAMES");
      }
   }
   /*
   **
   **   sounds. all sounds entries
   */
   if(select&BSOUND)
   {  start=size=0;
      if(TXTseekSection(TXT,"SOUNDS"))
      { Phase("Making Sounds\n");
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ if(Repeat!=TRUE)
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    if(MakeFileName(file,DataDir,"SOUNDS","",filenam,"TXT")==TRUE)
	    { size=SNDcopyPCSoundInWAD(&rwad,file);
	      Detail("Read PC Sound as file %s\n",file);
	    }
	    else
	    { if(MakeFileName(file,DataDir,"SOUNDS","",filenam,"WAV")==TRUE)
	      { size=SNDcopyInWAD(&rwad,file,SNDWAV);
	      }
	      else if(MakeFileName(file,DataDir,"SOUNDS","",filenam,"AU")==TRUE)
	      { size=SNDcopyInWAD(&rwad,file,SNDAU);
	      }
	      else if(MakeFileName(file,DataDir,"SOUNDS","",filenam,"VOC")==TRUE)
	      { size=SNDcopyInWAD(&rwad,file,SNDVOC);
	      }
	      else if(CMPOcopyFromWAD(&size,&rwad,DataDir,"SOUNDS",name,
		    filenam)!=TRUE)
		ProgError("Can't find Sound %s, AU or WAV or WAD",file);
	      Detail("Read Sound in file %s\n",file);
	    }
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
      }
   }
   /*
   **
   **   Musics
   */
   if(select&BMUSIC)
   {  start=size=0;
      if(TXTseekSection(TXT,"MUSICS"))
      { Phase("Making Musics\n");
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ if(Repeat!=TRUE)
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    /*Music*/
	    if(MakeFileName(file,DataDir,"MUSICS","",filenam,"MUS")==TRUE)
	    { size=WADRwriteLump(&rwad,file);
	      Detail("Read Music as MUS file %s\n",file);
	    }
	    else if(CMPOcopyFromWAD(&size,&rwad,DataDir,"MUSICS",name,
		  filenam)!=TRUE)
	      ProgError("Can't find Music %s",file);
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
      }
   }
   /*
   **  ordinary graphics
   */
   if(select&BGRAPHIC)
   {  start=size=0;
      if(TXTseekSection(TXT,"GRAPHICS"))
      { Phase("Making Graphics\n");
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,TRUE)==TRUE)
	{ if(Repeat!=TRUE)
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    Picture=CMPOloadPic(&size,&rwad,file,DataDir,"GRAPHICS",name,
		filenam,PGRAPH,X,Y);
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
      }
   }
   /*
   **  SS_START
   **  sprites
   **  SS_END
   */
   if(select&BSPRITE)
   {  start=size=0;
      if(TXTseekSection(TXT,"SPRITES"))
      { Phase("Making Sprites\n");
	FoundOne=FALSE;
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,TRUE)==TRUE)
	{ /* first sprite seen? */
	  if((Repeat!=TRUE)||(FoundOne!=TRUE))
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    if(FoundOne!=TRUE)
	    { if(type==IWAD)
		WADRdirAddEntry(&rwad,start,0L,"S_START");
	      else
		WADRdirAddEntry(&rwad,start,0L,"SS_START");
	    }
	    FoundOne=TRUE;
	    CMPOloadPic(&size,&rwad,file,DataDir,"SPRITES",name,
		filenam,PSPRIT,X,Y);
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
	if(FoundOne==TRUE)
	{ WADRalign4(&rwad);
	  start=WADRposition(&rwad);
	  if((type==IWAD)||(George==TRUE))
	    WADRdirAddEntry(&rwad,start,0L,"S_END");
	  else
	    WADRdirAddEntry(&rwad,start,0L,"SS_END");
	}
      }
   }
   /*
   ** Try to load WALL patches
   **   even if no new textures (old patches could be redefined)
   */
   /* write new patches  in PWAD*/
   /* read the name of the new textures and insert them*/
   /* between P_START and P_END for future completion*/

   if(select&BPATCH)
   {  FoundOne=FALSE;
      /*
      ** First look for patches in [PATCHES]
      */
      start=size=0;
      if(TXTseekSection(TXT,"PATCHES"))
      { Phase("Making Wall Patches\n");
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,TRUE)==TRUE)
	{ if((Repeat!=TRUE)||(FoundOne!=TRUE))
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    if(FoundOne==FALSE)
	    { if(type==IWAD)
	      {
		WADRdirAddEntry(&rwad,start,0L,"P_START");
		WADRdirAddEntry(&rwad,start,0L,"P1_START");
	      }
	      else
	      {
		WADRdirAddEntry(&rwad,start,0L,"PP_START");
	      }
	    }
	    FoundOne=TRUE;
	    CMPOloadPic(&size,&rwad,file,DataDir,"PATCHES",name,
		filenam,PPATCH,X,Y);
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
      }
      /*
      ** Check if all the needed patches are defined.
      */
      nbPatchs=PNMgetNbOfPatch();
      for(p=0;p<nbPatchs;p++)
      {
	if(PNMisNew(p)!=TRUE)
	{
	  continue;/*if old patch, forget it*/
	}
	PNMgetPatchName(name,p);
        Normalise(filenam,name);
	/*search in main IWAD directory*/
	if(WADRfindEntry(&iwad,name)>=0)
	{ Output("Reusing DOOM entry %s as patch\n", lump_name (name));
	}
	/*search in current PWAD*/
	else if(WADRfindEntry(&rwad,name)<0)
	{ /*PATCH not found in current WAD, load automatically
	  **from the PATCH directory
	  */
	  WADRalign4(&rwad);     /*align entry on Int32 word*/
	  start=WADRposition(&rwad);
	  Picture=CMPOloadPic(&size,&rwad,file,DataDir,"PATCHES",name,
	      filenam,PPATCH,INVALIDINT,INVALIDINT);
	  if(Picture!=PICNONE)
	  { if(FoundOne==FALSE)
	    { Phase("Making Wall Patches\n");
	      if(type==IWAD)
	      { WADRdirAddEntry(&rwad,start,0L,"P_START");
		WADRdirAddEntry(&rwad,start,0L,"P1_START");
	      }
	      else
		WADRdirAddEntry(&rwad,start,0L,"PP_START");
	    }
	    FoundOne=TRUE;
	    WADRdirAddEntry(&rwad,start,size,name);
	  }
	}
      }
      if(FoundOne==TRUE)
      { WADRalign4(&rwad);     /*align entry on Int32 word*/
	start=WADRposition(&rwad);
	if(type==IWAD)
	{ WADRdirAddEntry(&rwad,start,0L,"P1_END");
	  WADRdirAddEntry(&rwad,start,0L,"P2_START");
	  WADRdirAddEntry(&rwad,start,0L,"P2_END");
	  WADRdirAddEntry(&rwad,start,0L,"P3_START");
	  WADRdirAddEntry(&rwad,start,0L,"P3_END");
	  WADRdirAddEntry(&rwad,start,0L,"P_END");
	}
	else
	  WADRdirAddEntry(&rwad,start,0L,"PP_END");
      }
   }
   /*
   ** clear off Pnames
   */
   if(select&(BTEXTUR|BPATCH))
   { PNMfree();
   }
   /*  FF_START
   **  Flats
   **  F_END   AYM 1998-12-22 (was FF_END)
   */
   if(select&BFLAT)
   {  if(TXTseekSection(TXT,"FLATS"))
      { Phase("Making Flats\n");
	FoundOne=FALSE;
	while(TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,FALSE)==TRUE)
	{ if((Repeat!=TRUE)||(FoundOne!=TRUE))
	  { WADRalign4(&rwad);     /*align entry on Int32 word*/
	    start=WADRposition(&rwad);
	    if(FoundOne==FALSE)
	    { if(type==IWAD)
	      { WADRdirAddEntry(&rwad,start,0L,"F_START");
		WADRdirAddEntry(&rwad,start,0L,"F1_START");
	      }
	      else
		WADRdirAddEntry(&rwad,start,0L,"FF_START");
	    }
	    FoundOne=TRUE;
	    CMPOloadPic(&size,&rwad,file,DataDir,"FLATS",name,
		filenam,PFLAT,INVALIDINT,INVALIDINT);
	  }
	  WADRdirAddEntry(&rwad,start,size,name);
	}
	if(FoundOne==TRUE)
	{ start=WADRposition(&rwad);
	  if(type==IWAD)
	  { WADRdirAddEntry(&rwad,start,0L,"F1_END");
	    WADRdirAddEntry(&rwad,start,0L,"F2_START");
	    WADRdirAddEntry(&rwad,start,0L,"F2_END");
	    WADRdirAddEntry(&rwad,start,0L,"F3_START");
	    WADRdirAddEntry(&rwad,start,0L,"F3_END");
	    WADRdirAddEntry(&rwad,start,0L,"F_END");
	  }
	  else
	    WADRdirAddEntry(&rwad,start,0L,"F_END");  /* AYM 1998-12-22 */
	}
      }
   }
   /*
   ** exit from graphic
   */
   if(select&(BGRAPHIC|BSPRITE|BPATCH|BFLAT)) COLfree();
   /*
   ** iwad not needed anymore
   */
   WADRclose(&iwad);
   /*
   ** the end
   */
   TXTcloseR(TXT);
   WADRwriteDir(&rwad, 1);  /* write the WAD directory */
   ProgErrorCancel();
   WADRclose(&rwad);
   /*add some junk at end of wad file, for DEU 5.21*/
   if(type==PWAD)  AddSomeJunk(PWADname);
}









/***************** Hack the IWAD *********************/
extern Int16 HowMuchJunk;
static char Junk[]
  ="This junk is here for DEU 5.21. I am repeating myself anyway... ";
static void AddSomeJunk(const char *file)
{  FILE *out;Int16 n;
   out=fopen(file,FOPEN_AB); /*open R/W at the end*/
   if(out==NULL) ProgError("Can't write file %s\n",file);
   for(n=0;n<HowMuchJunk;n++)
      if(fwrite(Junk,1,64,out)<64)  Warning("can't insert my junk!");
   fclose(out);

}
/**************** End Hack the IWAD *******************************/


#endif /*DeuTex*/

