/*
This file is Copyright © 1994-1995 Olivier Montanuy,
             Copyright © 1999-2005 André Majorel.

It may incorporate code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/


#include "deutex.h"
#include <errno.h>
#include "tools.h"
#include "endianm.h"
#include "text.h"
#include "mkwad.h"
#include "texture.h"
#include "wadio.h"

/* The wad format limits the maximum number of patches per texture. */
#define MAX_PATCHES 32767

/* add new patchs by cluster of 64 */
#define NEWPATCHS                0x40
/* add new textures by cluster of 64 */
#define NEWTEXTURES                0x40
/* more than 8000 textures is unreasonable */
#define MAXTEXTURES                0x2000
/* add new patch defs by cluster of 128*/
#define NEWPATCHESDEF                0x80

/*
TEXU
 module that stores the textures definitions
         
PNM
 modules that stores the patch names

*/

/***************Begin PNAME module*****************/
static struct   PNMP{char name[8];}  *PNMpatchs;
static int16_t    PNMtop;
static int16_t        PNMmax;
static int16_t    PNMknown;
static bool        PNMok = false;



void PNMinit(char  *buffer,int32_t size)
{  int16_t n,i;
   char picname[8];
   int32_t pnames;
   /*find the number of entries in PNAME*/
   pnames=0;
   if(size>4L)
   { pnames = peek_i32_le (buffer);
     if(pnames>0x7FFFL) ProgError("PA01", "Too many patches");
     if(size<(4L+8L*pnames)) ProgError("PA02", "PNAMES has wrong size");
   }
   /*initialise*/
   PNMmax=(int16_t)(pnames+NEWPATCHS);
   PNMpatchs=(struct PNMP  *)Malloc(PNMmax*sizeof(struct PNMP));
   PNMtop=(int16_t)pnames;
   PNMknown=0;
   /*Read patches*/
   if(pnames<=0)return;
   for(n=0;n<PNMtop;n++)
   { for(i=0;i<8;i++)picname[i]=buffer[4L+8L*n+i];
     Normalise(PNMpatchs[n].name,picname);
   }
   PNMknown=PNMtop;
   PNMok=true;
}
/*
** check if PNAME exists (ident.c)
*/
int16_t PNMindexOfPatch(char  *patch)
{ int16_t idx;
  char name[8];
  Normalise(name,patch);
  if(PNMok!=true)
    return -1;
  /*check index already exists*/
  for(idx=0;idx<PNMtop;idx++)
    if(strncmp(PNMpatchs[idx].name,name,8)==0) /*if equal*/
          return idx;
  return -1;
}
/* Try to locate a Patch, from name
** if it doesn't exist, add it to the list
*/
static int16_t PNMgetPatchIndex(char  *patch)
{ int16_t idx;
  char name[8];
  if(PNMok!=true)Bug("PA90", "PNMok");
  Normalise(name,patch);
  idx=PNMindexOfPatch(patch);
  if(idx<0) /*No, it's a new patch, then*/
  {  idx = PNMtop;
     Normalise(PNMpatchs[idx].name,name);
     PNMtop++;  /* Increase top of patches*/
     if(PNMtop>=PNMmax)
     { PNMmax+=NEWPATCHS;
       PNMpatchs=(struct PNMP  *)Realloc(PNMpatchs,PNMmax*sizeof(struct PNMP));
     }
  }
  return idx;
}
/*
** get name from index
*/
void PNMgetPatchName(char name[8],int16_t index)
{ if(PNMok!=true)Bug("PA91", "PNMok");
  if(index>=PNMtop) Bug("PA92", "PnmGP>");
  Normalise(name,PNMpatchs[index].name);
}

/*
** Insert in directory all the entries which are not
** referenced in DOOM.WAD
*/
int16_t  PNMgetNbOfPatch(void)
{    return PNMtop;
} 
bool PNMisNew(int16_t idx)
{  if(PNMok!=true)Bug("PA93", "PNMok");
   if(idx>=PNMtop)Bug("PA94", "PnmIN>");
   /*check if patch was added after initial definition*/
   if(idx>=PNMknown) return true;
   return false;
}
void PNMfree(void)
{  PNMok=false;
   Free(PNMpatchs);
}
/********Write PNAME entry in WAD********/
int32_t PNMwritePNAMEtoWAD(struct WADINFO *info)
{  int16_t idx; int32_t size =0;
   char buffer[8];
   if(PNMok!=true)Bug("PA95", "PNMok");
   /*Write the Number of entries*/
   size+=WADRwriteLong(info,PNMtop);
   /*Then write the names , '\0' padded*/
   for(idx=0;idx<PNMtop;idx++)   
   { Normalise(buffer,PNMpatchs[idx].name);
     size +=WADRwriteBytes(info,buffer,8);
   }
   return size;
}
/*************End PNAME module **********************/



/********** TEXU  texture entry compilation ***********/

struct TEXTUR {char Name[8]; int16_t szX; int16_t szY; int16_t Npatches;};
struct PATCH {int16_t Pindex; int16_t ofsX; int16_t ofsY;};
/*IMPLICIT: if Name[0] = '\0', then texture was deleted*/

/*Textures: name, size*/
struct TEXTUR *TXUtex;
  int16_t TXUtexCur;  /*currently edited in TEXUtex*/
  int16_t TXUtexTop; /*top of in TEXUtex*/
  int16_t TXUtexMax;
/*Textures: patches composing textures*/
struct PATCH *TXUpat;
  long TXUpatTop; /*top of TEXUpat */
  long TXUpatMax;

bool TXUok=false;

void TXUinit(void)
{  TXUtexMax=NEWTEXTURES;
   TXUtexTop=0;
   TXUtex= (struct TEXTUR *) Malloc(TXUtexMax*sizeof(struct TEXTUR));
   TXUpatMax=NEWPATCHESDEF;
   TXUpatTop=0;
   TXUpat= (struct PATCH *) Malloc(TXUpatMax*sizeof(struct PATCH));
   TXUok=true;
}

static void TXUdefineCurTex(char name[8],int16_t X,int16_t Y,bool Redefn)
{  int t;
   if(TXUok!=true) Bug("TX89", "TXUok");
   TXUtexCur=TXUtexTop;             /*set current entry*/
   TXUtexTop+=1;                     /*find a free position*/
   if(TXUtexTop>=TXUtexMax)
   { TXUtexMax+=NEWTEXTURES;
         TXUtex=(struct TEXTUR *)Realloc(TXUtex,TXUtexMax*sizeof(struct TEXTUR));
   }
   Normalise(TXUtex[TXUtexCur].Name,name); /*declare texture*/
   TXUtex[TXUtexCur].szX=X;
   TXUtex[TXUtexCur].szY=Y;
   TXUtex[TXUtexCur].Npatches=0;
   /*check if we redefine other textures, and overide them.*/
   for(t=0;t<TXUtexCur;t++)
   { if(strncmp(TXUtex[t].Name,name,8)==0)
         { if(Redefn==true)
           { TXUtex[t].Name[0]='\0';
             Detail("TX90", "Warning: texture %s is redefined",
		 lump_name (name));
           }
           else /*don't redefine textures*/
           { TXUtex[TXUtexCur].Name[0]='\0';
             break;
           }
         }
   }
}
static void TXUaddPatchToCurTex(int16_t pindex,int16_t ofsX,int16_t ofsY)
{
   char pname[8];
   if(TXUok!=true)                 Bug("TX91", "TXUok");
   if(TXUpatTop>=TXUpatMax)
   {  TXUpatMax+=NEWPATCHESDEF;
      TXUpat=(struct PATCH *) Realloc(TXUpat,TXUpatMax*sizeof(struct PATCH));
   }
   if(TXUtexCur<0)                Bug("TX92", "TXUTxC");
   PNMgetPatchName(pname,pindex);  /*check if index correct*/
   TXUtex[TXUtexCur].Npatches+=1;  /*increase texture's patch counter*/
   TXUpat[TXUpatTop].Pindex=pindex;/*declare patch*/
   TXUpat[TXUpatTop].ofsX=ofsX;
   TXUpat[TXUpatTop].ofsY=ofsY;
   TXUpatTop+=1;
}
void TXUfree(void)
{  if(TXUok!=true) Bug("TX93", "TXUok");
   Free(TXUpat);
   Free(TXUtex);
   TXUok=false;
}

bool TXUexist(char *Name)
{  int t;
   if(TXUok!=true) Bug("TX94", "TXUok");
   for(t=0;t<TXUtexTop;t++)
   { if(strncmp(TXUtex[t].Name,Name,8)==0)
       return true;
   }
   return false;
}

/*
** find the number of real textures
*/
static int32_t TXUrealTexture(void)
{ int16_t t;
  int32_t NbOfTex=0; /*real top of texs*/
  for(t=0; t<TXUtexTop; t++)
  { if(TXUtex[t].Npatches<1)
    { Warning("TX10", "Ignored empty texture %s", lump_name (TXUtex[t].Name));
      TXUtex[t].Name[0]='\0';
    }
    if(TXUtex[t].Name[0]!='\0') NbOfTex++;
  }
  return NbOfTex;
}

/********** TEXTURE entry in WAD ********/
int32_t TXUwriteTEXTUREtoWAD(struct WADINFO *info)
{  int16_t t,tt,p,pat;
   int32_t size,ofsTble;
   int32_t NbOfTex;

   if(TXUok!=true) Bug("TX95", "TXUok");
   if(TXUtexTop<1) Bug("TX96", "TxuNTx");
   /*count real textures*/
   NbOfTex=TXUrealTexture();
   size=WADRwriteLong(info,NbOfTex);  /*number of entries*/
   ofsTble=WADRposition(info);
   for(tt=0;tt<NbOfTex;tt++)
   {  size+=WADRwriteLong(info,-1);     /*pointer, to be corrected later*/
   }
   for(pat=0,tt=0,t=0; t<TXUtexTop; t++)
   { if(TXUtex[t].Name[0]!='\0')
     {
       /*set texture direct pointer*/
       if(tt>=NbOfTex)Bug("TX97", "TxuRT");
       WADRsetLong(info,ofsTble+tt*4,size);
       tt++;
       /*write the begining of texture definition*/
       fseek (info->fd, info->wposit, SEEK_SET);  /* Ugly */
       if (output_texture_format != TF_NAMELESS)
         wad_write_name (info->fd, TXUtex[t].Name); size += 8;
       wad_write_i16  (info->fd, 0);              size += 2;
       wad_write_i16  (info->fd, 0);              size += 2;
       wad_write_i16  (info->fd, TXUtex[t].szX);  size += 2;
       wad_write_i16  (info->fd, TXUtex[t].szY);  size += 2;
       if (output_texture_format != TF_STRIFE11)
       {
	 wad_write_i16 (info->fd, 0); size += 2;
	 wad_write_i16 (info->fd, 0); size += 2;
       }
       wad_write_i16  (info->fd, TXUtex[t].Npatches); size += 2;
       for(p=0; p<(TXUtex[t].Npatches); p++)
       { if(pat+p>=TXUpatTop)
	   Bug("TX98", "TxuP>D");/*number of patches exceeds definitions*/
	 wad_write_i16 (info->fd, TXUpat[pat+p].ofsX);   size += 2;
	 wad_write_i16 (info->fd, TXUpat[pat+p].ofsY);   size += 2;
	 wad_write_i16 (info->fd, TXUpat[pat+p].Pindex); size += 2;
	 if (output_texture_format != TF_STRIFE11)
	 {
	   wad_write_i16 (info->fd, 0); size += 2;
	   wad_write_i16 (info->fd, 0); size += 2;
	 }
       }
       info->wposit = ftell (info->fd);  /* Ugly */
     }
     pat+=TXUtex[t].Npatches;
   }
   return size;
}


/*
**
** convert raw data (as read from wad) into Textures
** Data= texture entry
** if PatchSz>0, Patch defines the patch list
*/
void TXUreadTEXTURE(
  const char *texture1_name,
  const char *Data,
  int32_t DataSz,
  const char *Patch,
  int32_t PatchSz,
  bool Redefn
)
{ int32_t Pos,  Numtex, Numpat, dummy;
  /* texture data*/
  int16_t t,p,i, Xsize, Ysize; /* size x and y  */
  /* nb of patches used to build it */
  /* patch inside a texture */
  int16_t Xofs, Yofs,Pindex;         /* x,y coordinate in texture space*/
  /* patch name index in PNAMES table */
  int32_t  MaxPindex;
  static char tname[8];     /*texture name*/
  static char pname[8];     /*patch name*/
  size_t header_size		= 0;
  size_t item_size		= 0;
  int    have_texture_name	= 0;
  int    have_header_dummies	= 0;

  if (input_texture_format == TF_NAMELESS)
  {
    header_size         = 14;
    item_size           = 10;
    have_texture_name   = 0;
    have_header_dummies = 1;
  }
  else if (input_texture_format == TF_NONE)
  {
    Warning ("TX11", "No texture definitions to read");
    return;  /* FIXME is it OK to do that ? */
  }
  else if (input_texture_format == TF_NORMAL)
  {
    header_size         = 22;
    item_size           = 10;
    have_texture_name   = 1;
    have_header_dummies = 1;
  }
  else if (input_texture_format == TF_STRIFE11)
  {
    header_size         = 18;
    item_size           = 6;
    have_texture_name   = 1;
    have_header_dummies = 0;
  }
  else
  {
    Bug ("TX99", "invalid itf %d", (int) input_texture_format);
    return;
  }

  /*get number of patches*/
  if(PatchSz>0)
  { MaxPindex = peek_i32_le (Patch);
    dummy=4L+(MaxPindex*8L);
    if(dummy>PatchSz) ProgError("PA03", "PNAMES is corrupt");
  }
  else
  { MaxPindex = PNMgetNbOfPatch();
  }
  if((MaxPindex<0)||(MaxPindex>0x7FFF))
    ProgError("PA04", "PNAMES: invalid patch count %ld", (long) MaxPindex);
  /*get number of textures*/
  Numtex = peek_i32_le (Data);
  if(Numtex<0)
    ProgError("TX12", "%s: invalid texture count %ld",
	lump_name (texture1_name), (long) Numtex);
  if(Numtex>MAXTEXTURES)
    ProgError("TX13", "%s: too many textures (%ld/%ld)",
	lump_name (texture1_name), (long) Numtex, (long) MAXTEXTURES);

  /*read textures*/
  for (t = 0; t <Numtex; t++)
  { Pos = peek_i32_le (Data + 4L + t * 4L);
    if (Pos + header_size > DataSz)
      ProgError("TX14", "%s: unexpected EOL in entry %d",
	  lump_name (texture1_name), (long) t);

    if (have_texture_name)
    {
      Normalise (tname, Data + Pos);
      Pos += 8;
    }
    else  /* No name. Make one up (TEXnnnn)*/
    {
      if (t > 9999)
      {
	Warning ("TX15", "%s: more than 10000 textures, ignoring excess",
	    lump_name (texture1_name));
        break;
      }
      sprintf (tname, "TEX%04d", (int) t);
    }

    Pos += 4;  /* Skip 2 dummy unused fields */

    Xsize = peek_i16_le (Data + Pos);
    Pos += 2;
    if((Xsize<0)||(Xsize>4096))
      ProgError ("TX16", "%s: Texture %s: width out of bounds (%d)",
	  lump_name (texture1_name), lump_name (tname), (int) Xsize);

    Ysize = peek_i16_le (Data + Pos);
    Pos += 2;
    if((Ysize<0)||(Ysize>4096))
      ProgError ("TX17", "%s: Texture %s: height out of bounds (%d)",
	  lump_name (texture1_name), lump_name (tname), (int) Ysize);

    if (have_header_dummies)
      Pos += 4;  /* Skip 2 dummy unused fields */

    Numpat = peek_i16_le (Data + Pos)&0xFFFF;
    Pos += 2;
    if(Numpat<0)
    {
      nf_err("TX18",
	  "%s: Texture %s: negative patch count %d, skipping texture",
	  lump_name (texture1_name), lump_name (tname), (int) Numpat);
      continue;
    }

    /* declare texture */
    TXUdefineCurTex(tname,Xsize,Ysize,Redefn);
    if (Pos + (long) Numpat * item_size > DataSz)
      ProgError("TX19", "%s(%ld): Texture %s: unexpected EOL",
	  lump_name (texture1_name), (long) t, lump_name (tname));
    for (p = 0; p < Numpat; p++, Pos += item_size)
    {
      Xofs = peek_i16_le (Data + Pos);
      if((Xofs<-4096)||(Xofs>4096))
	ProgError("TX20", "%s(%ld): Texture %s(%d/%d): bad patch X-offset %d",
	    lump_name (texture1_name), (long) t,
	    lump_name (tname), (int) p, (int) Numpat, (int) Xofs);
      Yofs = peek_i16_le (Data + Pos + 2);
      if((Yofs<-4096)||(Yofs>4096))
	ProgError("TX21", "%s(%ld): Texture %s(%d/%d): bad patch Y-offset %d",
	    lump_name (texture1_name), (long) t,
	    lump_name (tname), (int) p, (int) Numpat, (int) Yofs);
      Pindex = peek_i16_le (Data + Pos + 4);
      if((Pindex<0)||(Pindex>MaxPindex))
	ProgError("TX22", "%s(%ld): Texture %s(%d/%d): bad patch index %d",
	    lump_name (texture1_name), (long) t,
	    lump_name (tname), (int) p, (int) Numpat, (int) Pindex);
      /*if new patch list, recalculate Pindex*/
      if(PatchSz>0)
      { for(dummy=(4L+(Pindex*8L)),i=0;i<8;i++)
          pname[i]=Patch[dummy+i];
        Pindex=PNMgetPatchIndex(pname);
      }
      /*declare patch*/
      TXUaddPatchToCurTex(Pindex,Xofs,Yofs);
    }
  }
}

bool TXUcheckTex(int16_t npatch,int16_t  *PszX)
{  int16_t t,tt,p,pat,col,top,found;
   int16_t bit,C,b;        /*bit test*/
   int16_t Meduza;
   bool Res=true;
   if(TXUok!=true) Bug("TX23", "TXUok");
   Output("Checking textures\n");
   if(TXUtexTop<1) Bug("TX24", "TxuNTx");
   /* FIXME assign a code to this message */
   if(TXUtexTop<100) Output("Warning: Some textures could be missing! (less than 100 defined)\n");
   for(pat=0, t=0; t<TXUtexTop; t++)
   { if(TXUtex[t].Npatches<1)
     /* FIXME assign a code to this message */
     { Output("Warning: Texture %s is empty\n", lump_name (TXUtex[t].Name));
       Res=false;
     }
     top=pat+TXUtex[t].Npatches;
     if(top>TXUpatTop) Bug("TX25", "TxuP>D");
     /*
     ** check width
     */
     for(bit=1,C=0,b=0;b<16;b++,bit<<=1) if((TXUtex[t].szX)&bit) C++;
     if(C>1)
     { Output("Warning: Width of %s is not a power of 2\n",
	 lump_name (TXUtex[t].Name)); /* FIXME assign a code to this message */
       Res=false;
     }
     /*
     ** check height
     */
     if(TXUtex[t].szY>128)
     { Output("Warning: Height of %s is more than 128\n",
	 lump_name (TXUtex[t].Name)); /* FIXME assign a code to this message */
       Res=false;
     }
     /*
     ** check patch for:
     ** - void columns            (crashes the game at boot)
     ** - possible meduza effect  (if the patch is used on 2S walls)
     */
     Meduza=0;
     for(col=0;col<TXUtex[t].szX;col++)
     { if(Meduza<2)Meduza=0; /*no Meduza effect found yet*/
       found=false;
       for(p=0; p<(TXUtex[t].Npatches); p++)
       { if(TXUpat[pat+p].Pindex>=npatch)
             Bug("TX26", "~TxuP>D");
         if(col>=TXUpat[pat+p].ofsX)
         { top=PszX[TXUpat[pat+p].Pindex]+TXUpat[pat+p].ofsX;
           if(col<top)
           { found=true;
             if(Meduza>=2)
               break; /*two patches on same column. Meduza effect*/
             else
               Meduza++; /*keep looking for patches*/
           }
         }
       }
       if(found==false)
       { /* FIXME assign a code to this message */
	 Output("Warning: Empty column %d in texture %s\n",
	   col, lump_name (TXUtex[t].Name));
         Res=false;
       }
     }
     if(Meduza>=2)  /*there is a colum with two patches*/
     { /* FIXME assign a code to this message */
       Output("Warning: Texture %s should not be used on a two sided wall.\n",
	 lump_name (TXUtex[t].Name));
     }
     pat+=TXUtex[t].Npatches;
   }
   /*
   ** check duplication
   */
   for(t=0; t<TXUtexTop; t++)
   { for(tt=t+1;tt<TXUtexTop;tt++)
       if(strncmp(TXUtex[t].Name,TXUtex[tt].Name,8)==0)
       { /* FIXME assign a code to this message */
	 Output("Warning: texture %s is duplicated\n",
	   lump_name (TXUtex[t].Name));
         Res=false;
       }
   }
   return Res;
}



/*
** this function shall only be used to add fake
** textures in order to list textures in levels
*/
void TXUfakeTex(char Name[8])
{  if(TXUok!=true) Bug("TX27", "TXUok");
   /*if already exist*/
   if(TXUexist(Name)==true) return;
   TXUtexCur=TXUtexTop;              /*set current entry*/
   TXUtexTop+=1;                     /*find a free position*/
   if(TXUtexTop>=TXUtexMax)
   { TXUtexMax+=NEWTEXTURES;
     TXUtex=(struct TEXTUR *)Realloc(TXUtex,TXUtexMax*sizeof(struct TEXTUR));
   }
   Normalise(TXUtex[TXUtexCur].Name,Name); /*declare texture*/
   TXUtex[TXUtexCur].szX=0;
   TXUtex[TXUtexCur].szY=0;
   TXUtex[TXUtexCur].Npatches=0;
}
/*
** list the names of the textures defined
*/
void TXUlistTex(void)
{  int16_t t;
   if(TXUok!=true) Bug("TX28", "TXUok");
   for (t= 0; t <TXUtexTop; t++)
   { if(TXUtex[t].Name[0]!='\0') 
       Output("%s\n", lump_name (TXUtex[t].Name)); 
   }                                                
}

/*
** write texture as text file
*/
void TXUwriteTexFile(const char *file)
{  int16_t t,p,pat,top;
   char pname[8];
   FILE *out;

   if(TXUok!=true)         Bug("TX29", "TXUok");
   if(TXUtexTop<1)         Bug("TX30", "TxunTx");

   out=fopen(file,FOPEN_WT);
   if(out==NULL)         ProgError("TX31", "%s: %s", file, strerror(errno));
   TXUrealTexture();
   fprintf(out,";Format of textures:\n");
   fprintf(out,";TextureName\tWidth\tHeight\n");
   fprintf(out,";*\tPatchName\tXoffset\tYoffset\n");
   for (pat=0, t= 0; t <TXUtexTop; t++)
   {  if(TXUtex[t].Name[0]!='\0') /*if tex was not redefined*/
      { fprintf(out,"%-8.8s    ",TXUtex[t].Name);
        fprintf(out,"\t\t%d\t%d\n",TXUtex[t].szX,TXUtex[t].szY);
        for (p = 0; p < TXUtex[t].Npatches; p++)
        { top=pat+p;
          if(top>=TXUpatTop) Bug("TX32", "TxuP>D");
          PNMgetPatchName(pname,TXUpat[pat+p].Pindex);
          fprintf(out,"*\t%-8.8s    ",pname);
          fprintf(out,"\t%d\t%d\n",TXUpat[pat+p].ofsX,TXUpat[pat+p].ofsY);
        }
      }
      pat+=TXUtex[t].Npatches;
   }
   fprintf(out,";End\n");
   if (fclose(out) != 0)
     ProgError("TX33", "%s: %s", file, strerror(errno));
}
/*
** read texture as text file
**
*/
void TXUreadTexFile(const char *file,bool Redefn)
{  int16_t Pindex;
   int16_t xsize=0,ysize=0,ofsx=0,ofsy=0;
   char tname[8];  
   char pname[8];
   int16_t t,bit,C,b;    /*to check Xsize*/
   struct TXTFILE *TXT;
   TXT=TXTopenR(file, 0);
/*   if(TXTbeginBloc(TXT,"TEXTURES")!=true)ProgError("TX34", "Invalid texture file format: %s",file);
*/
   for(t=0;t<MAXTEXTURES;t++)
   { if(TXTreadTexDef(TXT,tname,&xsize,&ysize)==false) break;
     /* check X size */
     if(xsize<0)
       ProgError("TX35", "Texture %s: width %d < 0",
	   lump_name (tname), (int) xsize);
     if (xsize>4096)
       Warning("TX36",
	   "Texture %s: width %d > 4096, XDoom 20001001 may crash",
	   lump_name (tname), (int) xsize);
     if (xsize>512)
       Warning("TX37",
	   "Texture %s: width %d > 512, Doom may freeze",
	   lump_name (tname), (int) xsize);
     for(bit=1,C=0,b=0;b<16;b++,bit<<=1) if(xsize&bit) C++;
     if(C>1)
       Warning("TX38", "Texture %s: width %d not a power of 2",
	   lump_name (tname), (int) xsize);
     /* check Y size */
     if(ysize<0)
       ProgError("TX39", "Texture %s: height %d < 0",
	   lump_name (tname), (int) ysize);
     if(ysize>128)
       Warning("TX40",
	   "Texture %s: height %d > 128, won't be rendered properly",
	   lump_name (tname), (int) ysize);
     /* declare texture */
     TXUdefineCurTex(tname,xsize,ysize, Redefn);
     {
       long npat = 0;

       while (TXTreadPatchDef(TXT,pname,&ofsx,&ofsy) == true)
       {
	 if (npat == MAX_PATCHES)
	   Warning("TX42",
	       "Texture %s: more than 32767 patches, ignoring excess",
	       lump_name (tname), ofsx);
	 else if (npat > MAX_PATCHES)
	   ;  /* Silently drop the rest */
	 else
	 {
	   /* declare a patch wall, getting index or declaring a new index */
	   Pindex=PNMgetPatchIndex(pname);
	   TXUaddPatchToCurTex(Pindex,ofsx,ofsy);
	 }
	 if (npat <= MAX_PATCHES)
	   npat++;
       }
     }
   }
   Phase("TX44", "Read %d textures from %s", t, file);
   TXTcloseR(TXT);
}
/***************End TEXTURE module*********************/
