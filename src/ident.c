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
#include "mkwad.h"
#include "texture.h"
#include "ident.h"

/*
** This file contains all the routines to identify DOOM specific
** entries and structures. Fear the bugs!
*/



/****************IDENT module ***********************/
/* identify ExMx or MAPxx entries
** which begin a DOOM level.
** returns -1 if not correct
*/


Int16 IDENTlevelPart(char *name)
{ Int16 n;
  static char *Part[]={"name","THINGS","LINEDEFS","SIDEDEFS","VERTEXES",
  "SEGS","SSECTORS","NODES","SECTORS","REJECT","BLOCKMAP","BEHAVIOR",NULL};
  for(n=1; Part[n]!=NULL; n++)
  { if(strncmp(Part[n],name,8)==0) return n;
  }
  return -1;
}


Int16 IDENTlevel(char *buffer)
{  switch(buffer[0])
	{ case 'E':
		  if(buffer[2]=='M')
			 if(buffer[4]=='\0')
				if((buffer[1]>='1')&&(buffer[1]<='9'))
				  if((buffer[3]>='1')&&(buffer[3]<='9'))
		  return (Int16)((buffer[1]&0xF)<<4)+(buffer[3]&0xF);
	  break;
	  case 'M':
		  if(buffer[1]=='A')
			 if(buffer[2]=='P')
				if((buffer[3]>='0')&&(buffer[3]<='9'))
				  if((buffer[4]>='0')&&(buffer[4]<='9'))
				  { return (Int16)((buffer[3]&0xF)*10)+(buffer[4]&0xF);
				  }
	  break;
	 default:
	  break;
	}
	return -1;
}
/*
** calculate default insertion point
*/
Int16 IDENTinsrX(PICTYPE type,Int16 insrX,Int16 szx)
{
  if(insrX!=INVALIDINT)
	if(insrX > -4096)
	  if(insrX < 4096)
		 return insrX;
  /* default insertion point */
  switch(type)
  {
    case PPATCH:       /*mid, lower-5 ????*/
      return (Int16)(szx/2);
    case PSPRIT:      /*mid, lower-5*/
      return (Int16)(szx/2);
    case PWEAPN:      /*absolute, in 320*200*/
      return (Int16)(-(320-szx)/2);   /* -160+X??*/
    case PFLAT:       /*no insertion point*/
      return (Int16)0;
    case PGRAPH:    /*0,0 by default*/
      return (Int16)0;
    default:
      Bug("idinx (%d)", (int) type);
  }
  return (Int16)0;
}
Int16 IDENTinsrY(PICTYPE type,Int16 insrY,Int16 szy)
{ 
  if(insrY!=INVALIDINT)
   if(insrY > -4096)
     if(insrY < 4096)
       return insrY;
  /* default insertion point */
  switch(type)
  {
    case PPATCH:       /*mid, lower-5 ????*/
      return (Int16)(szy-5);
    case PSPRIT:      /*mid, lower-5*/
      return (Int16)(szy-5);
    case PWEAPN:      /*absolute, in 320*200*/
      return (Int16)(-(200-szy));
    case PFLAT:       /*no insertion point*/
      return (Int16)0;
    case PGRAPH:    /*0,0 by default*/
      return (Int16)0;
    default:
      Bug("idiny (%d)", (int) type);
  }
  return 0;
}
/*
** identify a graphic entry
** from a read WAD
*/

struct PICHEAD {
Int16 Xsz;               /*nb of columns*/
Int16 Ysz;               /*nb of rows*/
Int16 Xinsr;             /*insertion point*/
Int16 Yinsr;             /*insertion point*/
};
ENTRY IDENTgraphic(struct WADINFO *info,Int16 n)
{
   Int32 start=info->dir[n].start;
   Int32 size=info->dir[n].size;
   Int16 x,insrX,insrY,Xsize,Ysize;
   Int32 huge *ofscol;
   static struct PICHEAD head;
   /*check X,Y size and insertion point*/
   if(size<8) return ELUMP;
   WADRseek(info,start);
   WADRreadBytes(info,(char huge *)&head,sizeof(struct PICHEAD));
   Xsize = peek_i16_le (&head.Xsz);
   if((Xsize<1)||(Xsize>320))return ELUMP;
   Ysize = peek_i16_le (&head.Ysz);
   if((Ysize<1)||(Ysize>200))return ELUMP;
   insrX = peek_i16_le (&head.Xinsr);  /*insertion point*/
   if((insrX<-4000)||(insrX>4000))return ELUMP;
   insrY = peek_i16_le (&head.Yinsr);   /*insertion point*/
   if((insrY<-4000)||(insrY>4000))return ELUMP;
   /*check picture size*/
   if(size<8+4*Xsize+1*Xsize) return ELUMP;
   /*check validity of columns*/
   ofscol=(Int32 huge *)Malloc(Xsize*sizeof(Int32));
   WADRreadBytes(info,(char huge *)ofscol,Xsize*sizeof(Int32));
   for(x=0;x<Xsize;x++)
   { if(peek_i32_le (ofscol + x) > size) /*check against size*/
     { Free(ofscol);return ELUMP;}
   }
   Free(ofscol);
   /*valid...graphic...maybe...*/
   return EGRAPHIC;
}

/*
** set identity of an entry with known name
** set only the first entry that match this name
*/
static void IDENTdirSet(ENTRY huge *ids,struct WADINFO *info,char *name,ENTRY ident)
{ Int16 n;
  n=WADRfindEntry(info,name);
  if(n>=0)   /*found it?*/
    if(n<(info->ntry))
      if(ids[n]==EZZZZ)
      { ids[n]=ident;
      }
}
/*
** identifies sprites from:
**  S_START SS_START S_END SS_END delimiters if exist
**  S_END SS_END delimiter and crawl back
**
** Precond: ids contains EZZZZ for unidentified entries
*/
static void IDENTdirSprites(ENTRY huge *ids,struct WADINFO *info,Bool Check)
{  Int16 s_end,s_start;
   Int16 n,s;
   /*
   ** check if there are sprites
   */
   s_end=WADRfindEntry(info,"S_END");
   if(s_end<0) s_end=WADRfindEntry(info,"SS_END");
   if(s_end<0) return;
   ids[s_end]=EVOID;
   /*
   ** check if there is a sprites begining
   */
   s_start=WADRfindEntry(info,"S_START");
   if(s_start<0) s_start=WADRfindEntry(info,"SS_START");
   /*
   ** guess sprite location
   */
   if(s_start<0)
   { for(n=s_end-1;n>=0;n--)
     { if(ids[n]!=EZZZZ) break; /*last sprite*/
       if(info->dir[n].size<8) break; /*last sprite*/
       if(Check==TRUE)
       {  s=IDENTgraphic(info,n);
          if(s==ELUMP) break;
       }
       ids[n]=ESPRITE;
     }
   }
   /*
   ** declare sprites
   */
   else
   { ids[s_start]=EVOID;
     for(n=s_end-1;n>s_start;n--)
     { if(info->dir[n].size>8)
       { ids[n]=ESPRITE;
       }
     }
   }
}

/*
** identifies flats from:
**  F_START FF_START F_END FF_END delimiters if exist
**  F_END FF_END delimiter and crawl back
**
** Precond: ids contains EZZZZ for unidentified entries
*/
static void IDENTdirFlats(ENTRY huge *ids,struct WADINFO *info)
{  Int16 f_end,f_start;
   Int16 n;
   /*
   ** check if there are flats
   */
   f_end=WADRfindEntry(info,"F_END");
   if(f_end<0) f_end=WADRfindEntry(info,"FF_END");
   if(f_end<0) return;
   ids[f_end]=EVOID;

   IDENTdirSet(ids,info,"F1_START",EVOID);
   IDENTdirSet(ids,info,"F1_END",EVOID);
   IDENTdirSet(ids,info,"F2_START",EVOID);
   IDENTdirSet(ids,info,"F2_END",EVOID);
   IDENTdirSet(ids,info,"F3_START",EVOID);
   IDENTdirSet(ids,info,"F3_END",EVOID);
   /*F_SKY1 is not a real flat, but it must be among them*/
   IDENTdirSet(ids,info,"F_SKY1",EFLAT);
   /*
   ** check if there is a flats begining
   */
   f_start=WADRfindEntry(info,"F_START");
   if(f_start<0) f_start=WADRfindEntry(info,"FF_START");
   /*
   ** guess flat location
   */
   if(f_start<0)
   { for(n=f_end-1;n>0;n--)
     { if(ids[n]!=EZZZZ)
        if(ids[n]!=EVOID)
          if(ids[n]!=EFLAT)
            break; /*last flat*/
       if((info->dir[n].size==0x1000)||(info->dir[n].size==0x2000)||(info->dir[n].size==0x1040))
       { ids[n]=EFLAT;
       }
     }
   }
   /*
   ** declare flats
   */
   else
   { ids[f_start]=EVOID;
     for(n=f_end-1;n>f_start;n--)
     { if((info->dir[n].size==0x1000)||(info->dir[n].size==0x2000)||(info->dir[n].size==0x1040))
       { ids[n]=EFLAT;
       }
     }
   }
}


static void IDENTdirLumps(ENTRY huge *ids,struct WADINFO *info)
{ IDENTdirSet(ids,info,"PLAYPAL",ELUMP);
  IDENTdirSet(ids,info,"COLORMAP",ELUMP);
  IDENTdirSet(ids,info,"ENDOOM",ELUMP);
  IDENTdirSet(ids,info,"ENDTEXT",ELUMP);
  IDENTdirSet(ids,info,"DEMO1",ELUMP);
  IDENTdirSet(ids,info,"DEMO2",ELUMP);
  IDENTdirSet(ids,info,"DEMO3",ELUMP);
  IDENTdirSet(ids,info,"LOADING",ELUMP); /*loading screen*/
  IDENTdirSet(ids,info,"DMXGUS",ELUMP);
  IDENTdirSet(ids,info,"GENMIDI",ELUMP);
  IDENTdirSet(ids,info,"TINTTAB",ELUMP);
}

static void IDENTdirPatches(ENTRY huge *ids,struct WADINFO *info, char huge *Pnam, Int32 Pnamsz,Bool Check)
{  Int16 p_end,p_start;
   Int16 n,p;
   char huge *Pnames;
   /*
   **  find texture and pname entries
   */
   IDENTdirSet(ids,info,"TEXTURE1",ETEXTUR+1);
   IDENTdirSet(ids,info,"TEXTURE2",ETEXTUR+2);
   IDENTdirSet(ids,info,"PNAMES",EPNAME);
   /*
   ** check if there are flats
   */
   p_end=WADRfindEntry(info,"P_END");
   if(p_end<0) p_end=WADRfindEntry(info,"PP_END");
   if(p_end>=0)
   { ids[p_end]=EVOID;
     /*
     ** check if there is a patch begining
     */
     IDENTdirSet(ids,info,"P1_START",EVOID);
     IDENTdirSet(ids,info,"P2_START",EVOID);
     IDENTdirSet(ids,info,"P3_START",EVOID);
     IDENTdirSet(ids,info,"P1_END",EVOID);
     IDENTdirSet(ids,info,"P2_END",EVOID);
     IDENTdirSet(ids,info,"P3_END",EVOID);
     p_start=WADRfindEntry(info,"P_START");
     if(p_start<0) p_start=WADRfindEntry(info,"PP_START");
     /*
     ** declare patches
     */
     if(p_start>=0)
     { ids[p_start]=EVOID;
       for(n=p_end-1;n>p_start;n--)
       { if(info->dir[n].size>8)
           ids[n]=EPATCH;
       }
     }
   }
   /*
   ** check for lost patches
   **
   */
   if(Check==TRUE)
   { /*checkif PNAMES is redefined*/
     n=WADRfindEntry(info,"PNAMES");
     if(n>=0)
     { Pnames=(char huge *)Malloc(info->dir[n].size);
       WADRseek(info,info->dir[n].start);
       WADRreadBytes(info,Pnames,info->dir[n].size);
       PNMinit(Pnames,info->dir[n].size);
       Free(Pnames);
     }
     else  /*init with default DOOM Pnames*/
     { PNMinit(Pnam,Pnamsz);
     }
     /*check for lost patches*/
     for(n=0;n<info->ntry;n++)
     { if(ids[n]==EZZZZ)
         if(info->dir[n].size>8)
         {  p=PNMindexOfPatch(info->dir[n].name); /*Gcc*/
            if(p>=0)
            { p=IDENTgraphic(info,n);
              if(p!=ELUMP) ids[n]=EPATCH;
            }
         }
     }
     PNMfree();
   }
}


/*
** Ident unreferenced graphics
*/
static void IDENTdirGraphics(ENTRY huge *ids,struct WADINFO *info)
{  Int16 n;
   IDENTdirSet(ids,info,"TITLEPIC",EGRAPHIC);
#if 0
   /* not true for heretic*/
   IDENTdirSet(ids,info,"HELP1",EGRAPHIC);
   IDENTdirSet(ids,info,"HELP2",EGRAPHIC);
   IDENTdirSet(ids,info,"HELP",EGRAPHIC);
   IDENTdirSet(ids,info,"CREDIT",EGRAPHIC);
   IDENTdirSet(ids,info,"TITLE",EGRAPHIC);
#endif
   /*heretic fonts*/
   IDENTdirSet(ids,info,"FONTA_S",ELUMP);
   IDENTdirSet(ids,info,"FONTA_E",ELUMP);
   IDENTdirSet(ids,info,"FONTB_S",ELUMP);
   IDENTdirSet(ids,info,"FONTB_E",ELUMP);
   for(n=0;n<info->ntry;n++)
   { if(ids[n]==EZZZZ)
     { if(info->dir[n].size>8)
       { if(strncmp(info->dir[n].name,"FONT",4)==0)
         { ids[n]=EGRAPHIC;
         }
         else if(strncmp(info->dir[n].name,"M_",2)==0)
         { ids[n]=EGRAPHIC;
         }
       }
     }
   }
}
static void IDENTdirGraphics2(ENTRY huge *ids,struct WADINFO *info,Bool Check)
{  Int16 n;
   for(n=0;n<info->ntry;n++)
   { if(ids[n]==EZZZZ)
     { if(info->dir[n].size>8)
       { if(strncmp(info->dir[n].name,"WI",2)==0)
         { ids[n]=EGRAPHIC;
         }
         else if(strncmp(info->dir[n].name,"ST",2)==0)
         { ids[n]=EGRAPHIC;
         }
         else if(Check==TRUE)
         { ids[n]=IDENTgraphic(info,n);
         }
         else
         {  ids[n]=EGRAPHIC;
         }
       }
     }
   }
}
/*
** Ident PC sounds
*/
static void IDENTdirPCSounds(ENTRY huge *ids,struct WADINFO *info,Bool Check)
{  Int16 n;
   for(n=0;n<info->ntry;n++)
   { if(ids[n]==EZZZZ)
     { if(info->dir[n].size>4) /*works only for DOOM, not HERETIC*/
         if(strncmp(info->dir[n].name,"DP",2)==0)
         { if(Check==TRUE)
           { WADRseek(info,info->dir[n].start);
             if(WADRreadShort(info)==0x0)
               ids[n]=ESNDPC;
           }
         }
     }
   }
}

/*
** Ident musics
*/
static void IDENTdirMusics(ENTRY huge *ids,struct WADINFO *info,Bool Check)
{  Int16 n;
   for(n=0;n<info->ntry;n++)
   { if(ids[n]==EZZZZ)
     { if(info->dir[n].size>8)
       { /*D_ for DOOM   MUS_ for HERETIC*/
         if(strncmp(info->dir[n].name,"D_",2)!=0)
           if(strncmp(info->dir[n].name,"MUS_",4)!=0)
              continue;
         if(Check==TRUE)
         { /*check format*/
           WADRseek(info,info->dir[n].start);
           if(WADRreadShort(info)==0x554D)
             if(WADRreadShort(info)==0x1A53)
                ids[n]=EMUSIC;
         }
         else
           ids[n]=EMUSIC;
       }
     }
   }
}
/*
** Ident sounds
*/
static void IDENTdirSounds(ENTRY huge *ids,struct WADINFO *info, Bool Doom)
{  Int16 n;
   for(n=0;n<info->ntry;n++)
   { if(ids[n]==EZZZZ)
     { if(info->dir[n].size>8)
       /*works only for DOOM, not HERETIC*/
       if(strncmp(info->dir[n].name,"DS",2)==0)
       { ids[n]=ESNDWAV;
       }
       else if(Doom==FALSE)
       { WADRseek(info,info->dir[n].start);
           if(WADRreadShort(info)==0x3)
             if(WADRreadShort(info)==0x2B11)
               ids[n]=ESNDWAV;
       }
     }
   }
}


static void IDENTdirLevels(ENTRY huge *ids,struct WADINFO *info)
{ Int16 n,l;Int16 inlvl;
  char name[8];
  ENTRY level=EVOID;
  for(inlvl=0,n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    { Normalise(name,info->dir[n].name);
      l=IDENTlevel(name);
      if(l>=0)
      { level=(name[0]=='M')? EMAP:ELEVEL;
        level|=l;
        inlvl=10;
        ids[n]=level;
      }
      else if(inlvl>0)
      { l=IDENTlevelPart(name);
        if(l>=0) /*level continues*/
        { ids[n]=level;inlvl--;
        }
        else     /*level ends*/
         inlvl=0;
      }
    }
  }
}




/*
** IWAD: we assume all is correct
** if Fast = TRUE then sounds and most graphics are reported as lumps
** (this is for merge. no problem. bad identification only to be feared in PWAD)
*/
ENTRY huge *IDENTentriesIWAD(struct WADINFO *info,char huge *Pnam, Int32 Pnamsz,Bool Fast)
{   Int16 n;
    Bool Doom=FALSE;
    ENTRY huge *ids;
    Phase("IWAD entry identification...");
    if(info->ok!=TRUE)Bug("IdnOeI");
    ids=(ENTRY huge *)Malloc((info->ntry)*sizeof(ENTRY));
    if(WADRfindEntry(info,"ENDTEXT")<0)              /*Not Heretic*/
      if(WADRfindEntry(info,"ENDOOM")>=0) Doom=TRUE;
    /*
    ** identify for IWAD
    */
    for(n=0;n<info->ntry;n++)
      ids[n]=EZZZZ;
    IDENTdirLumps(ids,info);         /*fast*/
    IDENTdirSprites(ids,info,FALSE); /*fast*/
    IDENTdirFlats(ids,info);         /*fast*/
    IDENTdirLevels(ids,info);        /*fast*/
    IDENTdirMusics(ids,info,FALSE);  /*fast*/
    IDENTdirPCSounds(ids,info,FALSE);/*fast*/
    IDENTdirPatches(ids,info,Pnam,Pnamsz,FALSE); /*fast*/
    IDENTdirGraphics(ids,info);      /*fast*/
    if(Fast!=TRUE)
    { IDENTdirSounds(ids,info,Doom);   /*slow!*/
		IDENTdirGraphics2(ids,info,TRUE);/*slow!*/
    }
    /* unidentified entries are considered LUMPs*/
    for(n=0;n<info->ntry;n++)
    { 
      if(ids[n]==EZZZZ)
      {
	if(info->dir[n].size>=6)
	  ids[n]=ELUMP;
	else
	  ids[n]=EDATA;
      }
    }
    Phase("done\n");
    /*
    ** check registration
    */
/*
    switch(check)
    { case 1: case 2: break;
	   default: ProgError("Please register your game.");
    }
*/

    /*the end. WADR is still opened*/
    return ids;

}



ENTRY huge *IDENTentriesPWAD(struct WADINFO *info,char huge *Pnam, Int32 Pnamsz)
{   Int16 n;
    ENTRY huge *ids;
    Phase("PWAD entry identification...");
    if(info->ok!=TRUE)Bug("IdnOeP");
    ids=(ENTRY huge *)Malloc((info->ntry)*sizeof(ENTRY));
    /*
    ** identify for PWAD
    */
    for(n=0;n<info->ntry;n++)
      ids[n]=EZZZZ;
#ifdef DEBUG
    Phase("\nLumps...");
#endif
    IDENTdirLumps(ids,info);
#ifdef DEBUG
    Phase("\nSprit...");
#endif
    IDENTdirSprites(ids,info,TRUE);
#ifdef DEBUG
    Phase("\nFlat...");
#endif
    IDENTdirFlats(ids,info);
#ifdef DEBUG
    Phase("\nLev...");
#endif
    IDENTdirLevels(ids,info);
#ifdef DEBUG
    Phase("\nMus...");
#endif
    IDENTdirMusics(ids,info,TRUE);
#ifdef DEBUG
    Phase("\nPCsnd...");
#endif
    IDENTdirPCSounds(ids,info,TRUE);
#ifdef DEBUG
    Phase("\nPatch...");
#endif
    IDENTdirPatches(ids,info,Pnam,Pnamsz,TRUE);
#ifdef DEBUG
    Phase("\nGraph...");
#endif
    IDENTdirGraphics(ids,info);
#ifdef DEBUG
    Phase("\nSnd...");
#endif
    IDENTdirSounds(ids,info,FALSE);
#ifdef DEBUG
    Phase("\nGraph2...");
#endif
    IDENTdirGraphics2(ids,info,TRUE);
    for(n=0;n<info->ntry;n++)
    {  if(ids[n]==EZZZZ)
       {
	  if(info->dir[n].size>16)
	     ids[n]=ELUMP;
	  else
	     ids[n]=EDATA;
       }
    }
    /*
    ** unidentified entries are considered LUMPs
    */
    Phase("done\n");
    /*the end. WADR is still opened*/
    return ids;
}
/***************end IDENT module *******************/

