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
#include "wadio.h"

/* add new patchs by cluster of 64 */
#define NEWPATCHS                0x40
/* add new textures by cluster of 64 */
#define NEWTEXTURES                0x40
/* more than 8000 textures is unreasonable */
#define MAXTEXTURES                0x2000
/* no more than 256 patches per cluster */
#define MAXPATCHPERTEX                0x100
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
static Int16    PNMtop;
static Int16        PNMmax;
static Int16    PNMknown;
static Bool        PNMok = FALSE;



void PNMinit(char  *buffer,Int32 size)
{  Int16 n,i;
   char picname[8];
   Int32 pnames;
   /*find the number of entries in PNAME*/
   pnames=0;
   if(size>4L)
   { pnames = peek_i32_le (buffer);
     if(pnames>0x7FFFL) ProgError("Too many patches");
     if(size<(4L+8L*pnames)) ProgError("Wrong size of PNAMES entry");
   }
   /*initialise*/
   PNMmax=(Int16)(pnames+NEWPATCHS);
   PNMpatchs=(struct PNMP  *)Malloc(PNMmax*sizeof(struct PNMP));
   PNMtop=(Int16)pnames;
   PNMknown=0;
   /*Read patches*/
   if(pnames<=0)return;
   for(n=0;n<PNMtop;n++)
   { for(i=0;i<8;i++)picname[i]=buffer[4L+8L*n+i];
     Normalise(PNMpatchs[n].name,picname);
   }
   PNMknown=PNMtop;
   PNMok=TRUE;
}
/*
** check if PNAME exists (ident.c)
*/
Int16 PNMindexOfPatch(char  *patch)
{ Int16 idx;
  char name[8];
  if(PNMok!=TRUE)Bug("PNMok");
  Normalise(name,patch);
  /*check index already exists*/
  for(idx=0;idx<PNMtop;idx++)
    if(strncmp(PNMpatchs[idx].name,name,8)==0) /*if equal*/
          return idx;
  return -1;
}
/* Try to locate a Patch, from name
** if it doesn't exist, add it to the list
*/
static Int16 PNMgetPatchIndex(char  *patch)
{ Int16 idx;
  char name[8];
  if(PNMok!=TRUE)Bug("PNMok");
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
void PNMgetPatchName(char name[8],Int16 index)
{ if(PNMok!=TRUE)Bug("PNMok");
  if(index>=PNMtop) Bug("PnmGP>");
  Normalise(name,PNMpatchs[index].name);
}

/*
** Insert in directory all the entries which are not
** referenced in DOOM.WAD
*/
Int16  PNMgetNbOfPatch(void)
{    return PNMtop;
} 
Bool PNMisNew(Int16 idx)
{  if(PNMok!=TRUE)Bug("PNMok");
   if(idx>=PNMtop)Bug("PnmIN>");
   /*check if patch was added after initial definition*/
   if(idx>=PNMknown) return TRUE;
   return FALSE;
}
void PNMfree(void)
{  PNMok=FALSE;
   Free(PNMpatchs);
}
/********Write PNAME entry in WAD********/
Int32 PNMwritePNAMEtoWAD(struct WADINFO *info)
{  Int16 idx; Int32 size =0;
   char buffer[8];
   if(PNMok!=TRUE)Bug("PNMok");
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



/*compile only for DeuTex*/

/********** TEXU  texture entry compilation ***********/

struct TEXTUR {char Name[8]; Int16 szX; Int16 szY; Int16 Npatches;};
struct PATCH {Int16 Pindex; Int16 ofsX; Int16 ofsY;};
/*IMPLICIT: if Name[0] = '\0', then texture was deleted*/

/*Textures: name, size*/
struct TEXTUR *TXUtex;
  Int16 TXUtexCur;  /*currently edited in TEXUtex*/
  Int16 TXUtexTop; /*top of in TEXUtex*/
  Int16 TXUtexMax;
/*Textures: patches composing textures*/
struct PATCH *TXUpat;
  Int16 TXUpatTop; /*top of TEXUpat */
  Int16 TXUpatMax;

Bool TXUok=FALSE;

void TXUinit(void)
{  TXUtexMax=NEWTEXTURES;
   TXUtexTop=0;
   TXUtex= (struct TEXTUR *) Malloc(TXUtexMax*sizeof(struct TEXTUR));
   TXUpatMax=NEWPATCHESDEF;
   TXUpatTop=0;
   TXUpat= (struct PATCH *) Malloc(TXUpatMax*sizeof(struct PATCH));
   TXUok=TRUE;
}

static void TXUdefineCurTex(char name[8],Int16 X,Int16 Y,Bool Redefn)
{  int t;
   if(TXUok!=TRUE) Bug("TXUok");
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
         { if(Redefn==TRUE)
           { TXUtex[t].Name[0]='\0';
             Detail("Warning: Texture %s is redefined\n", lump_name (name));
           }
           else /*don't redefine textures*/
           { TXUtex[TXUtexCur].Name[0]='\0';
             break;
           }
         }
   }
}
static void TXUaddPatchToCurTex(Int16 pindex,Int16 ofsX,Int16 ofsY)
{
   char pname[8];
   if(TXUok!=TRUE)                 Bug("TXUok");
   if(TXUpatTop>=TXUpatMax)
   {  TXUpatMax+=NEWPATCHESDEF;
      TXUpat=(struct PATCH *) Realloc(TXUpat,TXUpatMax*sizeof(struct PATCH));
   }
   if(TXUtexCur<0)                Bug("TXUTxC");
   PNMgetPatchName(pname,pindex);  /*check if index correct*/
   TXUtex[TXUtexCur].Npatches+=1;  /*increase texture's patch counter*/
   TXUpat[TXUpatTop].Pindex=pindex;/*declare patch*/
   TXUpat[TXUpatTop].ofsX=ofsX;
   TXUpat[TXUpatTop].ofsY=ofsY;
   TXUpatTop+=1;
}
void TXUfree(void)
{  if(TXUok!=TRUE) Bug("TXUok");
   Free(TXUpat);
   Free(TXUtex);
   TXUok=FALSE;
}

Bool TXUexist(char *Name)
{  int t;
   if(TXUok!=TRUE) Bug("TXUok");
   for(t=0;t<TXUtexTop;t++)
   { if(strncmp(TXUtex[t].Name,Name,8)==0)
       return TRUE;
   }
   return FALSE;
}

/*
** find the number of real textures
*/
static Int32 TXUrealTexture(void)
{ Int16 t;
  Int32 NbOfTex=0; /*real top of texs*/
  for(t=0; t<TXUtexTop; t++)
  { if(TXUtex[t].Npatches<1)
    { Warning("Ignored empty texture %s", lump_name (TXUtex[t].Name));
      TXUtex[t].Name[0]='\0';
    }
    if(TXUtex[t].Name[0]!='\0') NbOfTex++;
  }
  return NbOfTex;
}

/********** TEXTURE entry in WAD ********/
Int32 TXUwriteTEXTUREtoWAD(struct WADINFO *info)
{  Int16 t,tt,p,pat;
   Int32 size,ofsTble;
   Int32 NbOfTex;

   if(TXUok!=TRUE) Bug("TXUok");
   if(TXUtexTop<1) Bug("TxuNTx");
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
       if(tt>=NbOfTex)Bug("TxuRT");
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
	   Bug("TxuP>D");/*number of patches exceeds definitions*/
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
void TXUreadTEXTURE(char  *Data,Int32 DataSz,char  *Patch, Int32 PatchSz,Bool Redefn)
{ Int32 Pos,  Numtex, Numpat, dummy;
  /* texture data*/
  Int16 t,p,i, Xsize, Ysize; /* size x and y  */
  /* nb of wall patches used to build it */
  /* wall patch inside a texture */
  Int16 Xofs, Yofs,Pindex;         /* x,y coordinate in texture space*/
  /* patch name index in PNAMES table */
  Int32  MaxPindex;
  static char tname[8];     /*texture name*/
  static char pname[8];     /*patch name*/
  size_t header_size;
  size_t item_size;
  int    have_texture_name;
  int    have_header_dummies;

  if (input_texture_format == TF_NAMELESS)
  {
    header_size         = 14;
    item_size           = 10;
    have_texture_name   = 0;
    have_header_dummies = 1;
  }
  else if (input_texture_format == TF_NONE)
  {
    Warning ("No texture definitions to read.");
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
    Bug ("TXUreadTEXTURE: bad itf %d", (int) input_texture_format);
  }

  /*get number of patches*/
  if(PatchSz>0)
  { MaxPindex = peek_i32_le (Patch);
    dummy=4L+(MaxPindex*8L);
    if(dummy>PatchSz) ProgError("Bad PNAMES format");
  }
  else
  { MaxPindex = PNMgetNbOfPatch();
  }
  if((MaxPindex<0)||(MaxPindex>0x7FFF)) ProgError("Bad PNAMES entry");
  /*get number of textures*/
  Numtex = peek_i32_le (Data);
  if(Numtex<0) ProgError("Bad TEXTURE entry");
  if(Numtex>MAXTEXTURES) ProgError("Too many TEXTUREs");

  /*read textures*/
  for (t = 0; t <Numtex; t++)
  { Pos = peek_i32_le (Data + 4L + t * 4L);
    if (Pos + header_size > DataSz)
      ProgError("TEXTURE entry too small");

    if (have_texture_name)
    {
      Normalise (tname, Data + Pos);
      Pos += 8;
    }
    else  /* No name. Make one up (TEXnnnn)*/
    {
      if (t > 9999)
      {
	Warning ("More than 10000 textures. Ignoring excess.");
        break;
      }
      sprintf (tname, "TEX%04d", (int) t);
    }

    Pos += 4;  /* Skip 2 dummy unused fields */

    Xsize = peek_i16_le (Data + Pos);
    Pos += 2;
    if((Xsize<0)||(Xsize>4096))
      ProgError ("Texture %s: width out of bound (%d)",
	  lump_name (tname), (int) Xsize);

    Ysize = peek_i16_le (Data + Pos);
    Pos += 2;
    if((Ysize<0)||(Ysize>4096))
      ProgError ("Texture %s: height out of bound (%d)",
	  lump_name (tname), (int) Ysize);

    if (have_header_dummies)
      Pos += 4;  /* Skip 2 dummy unused fields */

    Numpat = peek_i16_le (Data + Pos)&0xFFFF;
    Pos += 2;
    if((Numpat<0)||(Numpat>MAXPATCHPERTEX))
      ProgError("Texture %s: too many patches (%d)",
	  lump_name (tname), (int) Numpat);

    /* declare texture */
    TXUdefineCurTex(tname,Xsize,Ysize,Redefn);
    if (Pos + (long) Numpat * item_size > DataSz)
      ProgError("TEXTURE entry too small");
    for (p = 0; p < Numpat; p++, Pos += item_size)
    {
      Xofs = peek_i16_le (Data + Pos);
      if((Xofs<-4096)||(Xofs>4096))
	ProgError("Texture %s(%d/%d): bad patch X-offset %d",
	    lump_name (tname), (int) p, (int) Numpat, (int) Xofs);
      Yofs = peek_i16_le (Data + Pos + 2);
      if((Yofs<-4096)||(Yofs>4096))
	ProgError("Texture %s(%d/%d): bad patch Y-offset %d",
	    lump_name (tname), (int) p, (int) Numpat, (int) Yofs);
      Pindex = peek_i16_le (Data + Pos + 4);
      if((Pindex<0)||(Pindex>MaxPindex))
	ProgError("Texture %s(%d/%d): bad patch index %d",
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

Bool TXUcheckTex(Int16 npatch,Int16  *PszX)
{  Int16 t,tt,p,pat,col,top,found;
   Int16 bit,C,b;        /*bit test*/
   Int16 Meduza;
   Bool Res=TRUE;
   if(TXUok!=TRUE) Bug("TXUok");
   Output("Checking textures\n");
   if(TXUtexTop<1) Bug("TxuNTx");
   if(TXUtexTop<100) Output("Warning: Some textures could be missing! (less than 100 defined)\n");
   for(pat=0, t=0; t<TXUtexTop; t++)
   { if(TXUtex[t].Npatches<1)
     { Output("Warning: Texture %s is empty\n", lump_name (TXUtex[t].Name));
       Res=FALSE;
     }
     top=pat+TXUtex[t].Npatches;
     if(top>TXUpatTop) Bug("TxuP>D");
     /*
     ** check width
     */
     for(bit=1,C=0,b=0;b<16;b++,bit<<=1) if((TXUtex[t].szX)&bit) C++;
     if(C>1)
     { Output("Warning: Width of %s is not a power of 2\n",
	 lump_name (TXUtex[t].Name));
       Res=FALSE;
     }
     /*
     ** check height
     */
     if(TXUtex[t].szY>128)
     { Output("Warning: Height of %s is more than 128\n",
	 lump_name (TXUtex[t].Name));
       Res=FALSE;
     }
     /*
     ** check patch for:
     ** - void columns            (crashes the game at boot)
     ** - possible meduza effect  (if the patch is used on 2S walls)
     */
     Meduza=0;
     for(col=0;col<TXUtex[t].szX;col++)
     { if(Meduza<2)Meduza=0; /*no Meduza effect found yet*/
       found=FALSE;
       for(p=0; p<(TXUtex[t].Npatches); p++)
       { if(TXUpat[pat+p].Pindex>=npatch)
             Bug("~TxuP>D");
         if(col>=TXUpat[pat+p].ofsX)
         { top=PszX[TXUpat[pat+p].Pindex]+TXUpat[pat+p].ofsX;
           if(col<top)
           { found=TRUE;
             if(Meduza>=2)
               break; /*two patches on same column. Meduza effect*/
             else
               Meduza++; /*keep looking for patches*/
           }
         }
       }
       if(found==FALSE)
       { Output("Warning: Empty column %d in texture %s\n",
	   col, lump_name (TXUtex[t].Name));
         Res=FALSE;
       }
     }
     if(Meduza>=2)  /*there is a colum with two patches*/
     { Output("Warning: Texture %s should not be used on a two sided wall.\n",
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
       { Output("Warning: texture %s is duplicated\n",
	   lump_name (TXUtex[t].Name));
         Res=FALSE;
       }
   }
   return Res;
}



/*
** this function shall only be used to add fake
** textures in order to list textures in levels
*/
void TXUfakeTex(char Name[8])
{  if(TXUok!=TRUE) Bug("TXUok");
   /*if already exist*/
   if(TXUexist(Name)==TRUE) return;
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
{  Int16 t;
   if(TXUok!=TRUE) Bug("TXUok");
   for (t= 0; t <TXUtexTop; t++)
   { if(TXUtex[t].Name[0]!='\0') 
       Output("%s\n", lump_name (TXUtex[t].Name)); 
   }                                                
}

#if defined DeuTex
/*
** write texture as text file
*/
void TXUwriteTexFile(const char *file)
{  Int16 t,p,pat,top;
   char pname[8];
   FILE *out;

   if(TXUok!=TRUE)         Bug("TXUok");
   if(TXUtexTop<1)         Bug("TxunTx");

   out=fopen(file,FOPEN_WT);
   if(out==NULL)         ProgError("Can't write file %s\n",file);
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
          if(top>=TXUpatTop) Bug("TxuP>D");
          PNMgetPatchName(pname,TXUpat[pat+p].Pindex);
          fprintf(out,"*\t%-8.8s    ",pname);
          fprintf(out,"\t%d\t%d\n",TXUpat[pat+p].ofsX,TXUpat[pat+p].ofsY);
        }
      }
      pat+=TXUtex[t].Npatches;
   }
   fprintf(out,";End\n");
   fclose(out);                                                
}
/*
** read texture as text file
**
*/
void TXUreadTexFile(const char *file,Bool Redefn)
{  Int16 Pindex;
   Int16 xsize=0,ysize=0,ofsx=0,ofsy=0;
   char tname[8];  
   char pname[8];
   Int16 t,p,bit,C,b;    /*to check Xsize*/
   struct TXTFILE *TXT;
   TXT=TXTopenR(file);
/*   if(TXTbeginBloc(TXT,"TEXTURES")!=TRUE)ProgError("Invalid texture file format: %s",file);
*/
   for(t=0;t<MAXTEXTURES;t++)
   { if(TXTreadTexDef(TXT,tname,&xsize,&ysize)==FALSE) break;
     /* check X size */
     if((xsize<0)||(xsize>4096))
       ProgError("texture width out of bound");
     for(bit=1,C=0,b=0;b<16;b++,bit<<=1) if(xsize&bit) C++;
     if(C>1)
       Warning("Bogus texture %s. Width should be a power of 2",
	   lump_name (tname));
     /* check Y size */
     if((ysize<0)||(ysize>4096))
       ProgError("texture height out of bound");
     // AYM 1999-05-17
     if(ysize>509)
       Warning("Bogus texture %s. Heights above 509 are ignored",
	   lump_name (tname));
     /* declare texture */
     TXUdefineCurTex(tname,xsize,ysize, Redefn);
     for(p=0;p<MAXPATCHPERTEX;p++)
     { /* exit if no new patch to be read*/
       if(TXTreadPatchDef(TXT,pname,&ofsx,&ofsy)==FALSE) break;
       /* declare a patch wall, getting index or declaring a new index */
       Pindex=PNMgetPatchIndex(pname);
       TXUaddPatchToCurTex(Pindex,ofsx,ofsy);
     }
   }
   Phase("Read %d Textures in %s\n",t,file);
   TXTcloseR(TXT);
}
/***************End TEXTURE module*********************/


#endif /*DeuTex*/
