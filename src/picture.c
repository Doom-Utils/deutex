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
#include <ctype.h>
#include <errno.h>
#include "tools.h"
#include "endianio.h"
#include "endianm.h"
#include "mkwad.h"
#include "picture.h"
#include "ident.h"
#include "color.h"
#include "usedidx.h"


static int wall_to_raw (char *data, int32_t datasz, const char *name);

/*
 *	parse_pic_header
 *
 *	buf      Data for the entire lump containing the
 *	         hypothethical image.
 *	bufsz    Size of buf in bytes.
 * 	h        Pointer on struct through which
 * 	         parse_pic_header() returns the characteristic
 * 	         of the picture header.
 *	message  Pointer on char[81]. If an error occurs (non-zero
 *	         return value) and <message> is not NULL, a
 *	         string of at most 80 characters is written
 *	         there.
 *
 *	Return 0 on success, non-zero on failure.
 */
#define FAIL0(s)   do{if (message) strcpy (message, s);     return 1;}while(0)
#define FAIL1(f,a) do{if (message) sprintf (message, f, a); return 1;}while(0)
int parse_pic_header (
    const char *buf,
    long bufsz,
    pic_head_t *h,
    char *message)
{
   /* Current byte of pic buffer */
   const unsigned char *p = (const unsigned char *) buf;
   /* Last byte of pic buffer */
   const unsigned char *buf_end = ((const unsigned char *) buf) + bufsz - 1;

   /* Details of picture format in wad */
   h->dummy_bytes = picture_format == PF_NORMAL;
   h->colofs_size = (picture_format == PF_NORMAL) ? 4 : 2;

   /* Read the picture header */
   if (picture_format == PF_ALPHA)
   {
      if (p + 4 - 1 > buf_end)
        FAIL0 ("header too short");
      h->width  = *p++;
      h->height = *p++;
      h->xofs   = *p++;
      h->yofs   = *p++;
   }
   else if (picture_format == PF_ROTT)
   {
     int16_t unknown;

     if (p + 10 - 1 > buf_end)
       FAIL0 ("header too short");
     read_i16_le (p, &unknown);		/* FIXME find out what it's for */
     p += 2;
     read_i16_le (p, &h->width);
     p += 2;
     read_i16_le (p, &h->height);
     p += 2;
     read_i16_le (p, &h->xofs);
     p += 2;
     read_i16_le (p, &h->yofs);
     p += 2;
   }
   else
   {
     if (p + 8 - 1 > buf_end)
       FAIL0 ("header too short");
     read_i16_le (p, &h->width);
     p += 2;
     read_i16_le (p, &h->height);
     p += 2;
     read_i16_le (p, &h->xofs);
     p += 2;
     read_i16_le (p, &h->yofs);
     p += 2;
   }

   /* Sanity checks on picture size and offsets */
   if (h->width < 1)
      FAIL1 ("width < 1 (%d)", (int) h->width);
   if (h->height < 1)
      FAIL1 ("height < 1 (%d)", (int) h->height);
#if 0
   /*if (h->width > 4096)
      FAIL1 ("width > 4096 (%d)", (int) h->width);*/
   if (h->height > 4096)
      FAIL1 ("height > 4096 (%d)", (int)h->height);
   if (h->xofs < -4096)
      FAIL1 ("X-offset < -4096 (%d)", (int) h->xofs);
   if (h->xofs > 4096)
      FAIL1 ("X-offset > 4096 (%d)", (int) h->xofs);
   if (h->yofs < -4096)
      FAIL1 ("Y-offset < -4096 (%d)", (int) h->yofs);
   if (h->yofs > 4096)
      FAIL1 ("Y-offset > 4096 (%d)", (int) h->yofs);
#endif

   /* Array of column offsets. Column data. */ 
   h->colofs = p;
   p += (long) h->colofs_size * h->width;
   if (p > buf_end)
      FAIL0 ("offsets table too short");
   h->data = p;
   return 0;  /* Success */
}


/* BMP,GIF,DoomPIC conversion
** intermediary format: RAW    (FLAT= RAW 64x64 or RAW 64x65)
**   int16_t Xsz int16_t Ysz
**   char idx[Xsz*Ysz]
**      colors= those of DOOM palette
**      position (x,y) = idx[x+xsz*y]
** insertion point not set
*/



/*
**
*/


static char *PICtoRAW (int16_t *prawX, int16_t *prawY, int16_t *pXinsr,
    int16_t *pYinsr, const char *pic,int32_t picsz, char transparent,
    const char *name, cusage_t *cusage);
static char *RAWtoPIC (int32_t *ppicsz, char *raw, int16_t rawX, int16_t rawY,
    int16_t Xinsr, int16_t Yinsr, char transparent);
static char *snea_to_raw (int16_t *prawX, int16_t *prawY, int16_t *pXinsr,
    int16_t *pYinsr, const char *snea, int32_t sneasz, const char *name,
    cusage_t *cusage);
static void RAWtoBMP (char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal);
static char *BMPtoRAW(int16_t *prawX, int16_t *prawY, char *file);
static void RAWtoPPM (char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal);
static char *PPMtoRAW (int16_t *prawX, int16_t *prawY, char *file);
static char *GIFtoRAW (int16_t *rawX, int16_t *rawY, char *file);
static void RAWtoGIF (char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal);

/*
**
**  this is only a test example
**  COLinit and COLfree must be called before
**  BMP->GIF
*/
#if 0
void PicDebug(char *file, const char *bmpdir, const char *name)
{  char  *raw;
   int16_t rawX,rawY;
   struct PIXEL  * doompal;

   Phase("XX99", "BMP->RAW");
   MakeFileName(file,bmpdir,"","",name,"BMP");
   raw =BMPtoRAW(&rawX,&rawY,file);
   doompal = COLdoomPalet();
   Phase("XX99", "RAW->GIF");
   MakeFileName(file,bmpdir,"","",name,"GIF");
   RAWtoGIF(file,raw,rawX,rawY,doompal);

   Free(raw);
}
#endif
/*
**  this is only a test example
**  GIF->BMP
*/
#if 1
void PicDebug(char *file,const char *bmpdir, const char *name)
{  char  *raw;
   int16_t rawX=0,rawY=0;
   struct PIXEL  * doompal;

   Phase("DB10", "GIF->RAW");
   MakeFileName(file,bmpdir,"","",name,"GIF");
   raw =GIFtoRAW(&rawX,&rawY,file);
   doompal = COLdoomPalet();
   Phase("DB20", "RAW->BMP");
   MakeFileName(file,bmpdir,"","",name,"BMP");
   RAWtoBMP(file,raw,rawX,rawY,doompal);

   Free(raw);
}
#endif
/*
** end of examples
**
*/

/*
** BMP, GIF or PPM
**
*/
bool PICsaveInFile (char *file, PICTYPE type, char *pic, int32_t picsz,
    int16_t *pXinsr, int16_t *pYinsr, IMGTYPE Picture, const char *name,
    cusage_t *cusage)
{ char  *raw=NULL;
  int16_t rawX = 0;			/* Initialise to placate GCC */
  int16_t rawY = 0;			/* Initialise to placate GCC */
  struct PIXEL *doompal;
  char transparent;

  /*Warning ("XX99", "PICsaveInFile: %s", fname (file));*/
  transparent =(char)COLinvisible();
  *pXinsr=INVALIDINT; /*default insertion point X*/
  *pYinsr=INVALIDINT; /*default insertion point Y*/
  switch(type)
  { case PGRAPH:
    case PWEAPN:
    case PSPRIT:
    case PPATCH:
      /*Warning ("XX99", "PICtoRAW: %s", fname (file));*/
      raw = PICtoRAW (&rawX, &rawY, pXinsr, pYinsr, pic, picsz, transparent,
	  name, cusage);
      if (raw==NULL) return false;    /*was not a valid DoomPic*/
      break;

    case PFLAT:
      if(picsz==0x1000L)     { rawX=64;rawY=64;}
      else if(picsz==0x1040) { rawX=64;rawY=65;}  /* Heretic scrolling flat */
      else if(picsz==0x2000) { rawX=64;rawY=128;}  /* Hexen scrolling flat */
      else return false;/*Wrong size for FLAT. F_SKY1*/
      raw=pic;    /*flat IS raw already*/
      if (cusage != NULL)
	usedidx_rectangle (pic, picsz, name, cusage);
      break;

    case PWALL:
      if(picsz==4096)        { rawX=64;rawY=64;}
      else return false;/*Wrong size for WALL.*/
      if (wall_to_raw (pic, picsz, name) != 0)
	ProgError ("GW88", "Wall %s: wall_to_raw error", lump_name (name));
      raw = pic;
      if (cusage != NULL)
	usedidx_rectangle (pic, picsz, name, cusage);
      break;

    case PSNEAP:
    case PSNEAT:
      raw = snea_to_raw (&rawX, &rawY, pXinsr, pYinsr, pic, picsz, name,
	  cusage);
      if (raw == NULL)
	return false;
      break;

    case PLUMP:
      /* Heretic, Hexen and Strife store some large bitmaps this way. */
      if(picsz==64000L)
      {
        rawX=320;
	rawY=200;
      }
      else
	return false;/*Wrong size for LUMP. F_SKY1*/
      raw=pic;    /*flat IS raw already*/
      if (cusage != NULL)
	usedidx_rectangle (pic, picsz, name, cusage);
      break;

    default:
      Bug("GW90", "Invalid type %d", (int) type);
  }

  if (cusage == NULL)
  {
     /*
     ** load game palette (PLAYPAL or TITLEPAL)
     */
     if (type == PSNEAT)
       doompal = COLaltPalet();
     else
       doompal = COLdoomPalet();
     /*
     ** convert to BMP/GIF/PPM
     */
     switch(Picture)
     { case PICGIF:
	RAWtoGIF(file,raw,rawX,rawY,doompal);
	break;
       case PICBMP:
	RAWtoBMP(file,raw,rawX,rawY,doompal);
	break;
       case PICPPM:
	RAWtoPPM(file,raw,rawX,rawY,doompal);
	break;
       default:
	Bug("GW91", "Invalid picture format %d", (int) Picture);
     }
  }

  switch(type)
  { case PGRAPH: case PWEAPN: case PSPRIT: case PPATCH:
    case PSNEAP: case PSNEAT:
     Free(raw);
     break;
    case PFLAT: /*don't free pic!*/
    case PLUMP:
    case PWALL:
     break;
  }
  return true;
}


int32_t PICsaveInWAD(struct WADINFO *info,char *file,PICTYPE type,int16_t Xinsr,int16_t Yinsr,IMGTYPE Picture)
{
  char  *raw=NULL;
  int16_t rawX=0,rawY=0;
  char  *pic=NULL;
  int32_t picsz = 0;			/* Initialise to placate GCC */
  char transparent;

  /*
  ** convert BMP to RAW
  */
  transparent =COLinvisible();
  switch(Picture)
  { case PICGIF:
     raw = GIFtoRAW(&rawX,&rawY,file);
     break;
    case PICBMP:
     raw = BMPtoRAW(&rawX,&rawY,file);
     break;
    case PICPPM:
     raw = PPMtoRAW(&rawX,&rawY,file);
     break;
    default:
     Bug("GB02", "Invalid picture conversion %d", (int)Picture);
  }
  if (rawX < 1)
    ProgError ("GB10", "%s: picture width < 1", fname (file));
  if (rawY < 1)
    ProgError ("GB11", "%s: picture height < 1", fname (file));
  /* AYM (256 -> 509) */
  if(rawY>509)
    ProgError ("GB13", "%s: picture height > 509", fname (file));
  switch(type)
  { case PGRAPH: case PWEAPN: case PSPRIT: case PPATCH:
      break;
    case PFLAT: /*flats*/
      if(rawX!=64)
	Warning ("FB10", "%s: weird width for a flat (not 64)", fname (file));
      if(rawY!=64 && rawY!=65 && rawY != 128)
	Warning ("FB11", "%s: weird height for a flat (not 64, 65 or 128)",
	    fname (file));
      break;
    case PLUMP: /*special heretic lumps*/
      if(rawX!=320)
        Warning ("LB10", "%s: weird width for a lump (not 320)", fname (file));
      if(rawY!=200)
        Warning ("LB11", "%s: weird height for a lump (not 200)", fname (file));
      break;
    case PWALL:
      if(rawX!=64)
	Warning("WB10", "%s: weird width for a wall (not 64)", fname (file));
      if(rawY!=64)
	Warning("WB11", "%s: weird height for a wall (not 64)", fname (file));
      break;
    default:
      Bug("GB91", "Invalid type %d", (int)type);
  }
  /*
  ** calculate insertion point
  */
  Xinsr=IDENTinsrX(type,Xinsr,rawX);
  Yinsr=IDENTinsrY(type,Yinsr,rawY);
  /*
  ** convert RAW to DoomPic
  */
  switch(type)
  { case PGRAPH: case PWEAPN: case PSPRIT: case PPATCH:
        pic=RAWtoPIC(&picsz,raw,rawX,rawY,Xinsr,Yinsr,transparent);
        Free(raw);
        WADRwriteBytes(info,pic,picsz);
        Free(pic);
        break;
    case PLUMP:    /*LUMP is RAW*/
    case PFLAT:    /*FLAT is RAW*/
        picsz= ((int32_t)rawX)*((int32_t)rawY);
        WADRwriteBytes(info,raw,picsz);
        Free(raw);
        break;
    case PWALL:  /* FIXME rotate 270° */
        picsz= ((int32_t)rawX)*((int32_t)rawY);
        WADRwriteBytes(info,raw,picsz);
        Free(raw);
        break;
  }
  /*
  ** write DoomPic in WAD
  */
  return picsz;
}




/******************* DoomPic module ************************/
/*
** doom pic
**
*/
struct PICHEAD {
int16_t Xsz;               /*nb of columns*/
int16_t Ysz;               /*nb of rows*/
int16_t Xinsr;             /*insertion point*/
int16_t Yinsr;             /*insertion point*/
};
/*
** RAW to DoomPIC conversion
**
** in:   int16_t Xinsr; int16_t Yinsr;
**       int16_t rawX; int16_t rawY;  char transparent;
**       char raw[Xsize*Ysize]
** out:  int32_t picsz;           size of DoomPIC
**       char pic[picsz];      buffer for DoomPIC
*/
#define OPTIMEM 0
static char *RAWtoPIC(int32_t *ppicsz, char  *raw, int16_t rawX, int16_t rawY,
    int16_t Xinsr,int16_t Yinsr, char transparent)
{  int16_t x,y;
   char pix,lastpix;            /*pixels*/
   /*Doom PIC */
   char  *pic;               /*picture*/
   int32_t picsz,rawpos;
   struct PICHEAD  *pichead; /*header*/
   int32_t  *ColOfs;            /*position of column*/
   /*columns composed of sets */
   char  *Set=NULL;
   int32_t colnbase,colnpos,setpos;
   int16_t setcount=0;

  /*offset of first column*/
  colnbase= sizeof(struct PICHEAD)+((int32_t)rawX)*sizeof(int32_t);
  /* worst expansion when converting from PIXEL column to
  ** list of sets: (5*Ysize/2)+1, corresponding to a dotted vertical
  ** transparent line. Ysize/2 dots, 4overhead+1pix, 1 last FF code.
  ** but if too big, allow only a 10 split mean per line
  */
  /*this memory optimisation doesn't work for big pics
        with many transparent areas. it limits the number of transp slots
        to a mean of 10. so a resize could be needed later.
        else, the biggest would be 162k
  */
  /* AYM 1999-05-09: changed the calculation of picsz.
     Previously, it was assumed that the worst case was when
     every second pixel was transparent. This is not true for
     textures that are only 2-pixel high; in that case, if the
     two pixels are opaque, the column uses 7 bytes. The old
     code thought that was 6 bytes, and overflowed the buffer
     for at least one picture (Strife's STBFN045). */
  /* Highest possible picture :
     Post 1  yofs = 0    length = 254
     Post 2  yofs = 254  length = 255
     Max height is 509 pixels. */
  {
  /* FIXME */
  int worst_case_1 = 5 * (((long) rawY + 1) / 2) + 1 + 1; /* "+ 1" matters ! */
  int worst_case_2 = 4 + rawY                    + 1;
  int worst_case = worst_case_1 > worst_case_2 ? worst_case_1 : worst_case_2;
  picsz= colnbase + ((int32_t)rawX) * worst_case;
  }
#if OPTIMEM /*optimisation*/
  if(picsz>0x10000L) picsz=0x10000L;
#endif
  pic = (char  *)Malloc(picsz);
  ColOfs=(int32_t  *)&(pic[sizeof(struct PICHEAD)]);
  pichead=(struct PICHEAD  *)pic;
  /*
  ** convert raw (doom colors) to PIC
  */
  write_i16_le (&pichead->Xsz,   rawX);
  write_i16_le (&pichead->Ysz,   rawY);
  write_i16_le (&pichead->Xinsr, Xinsr);
  write_i16_le (&pichead->Yinsr, Yinsr);
  colnpos=colnbase;
  for(x=0;x<rawX;x++)
  { write_i32_le (ColOfs + x, colnpos);
    setpos=0;
    lastpix=transparent;
    for(y=0;y<rawY;y++)
    {  /*get column pixel*/
      rawpos=((int32_t)x)+((int32_t)rawX)*((int32_t)y);
      pix=raw[rawpos];
      /* Start new post ? */
      if(pix!=transparent)
      { /* End current post and start new one if either :
	   - more than 255 consecutive non-transparent
	     pixels (a post cannot be longer than 255
	     pixels)
	   - current Y-offset is 254; since the maximum
	     start Y-offset for a post is 254, a new post
	     should be started when we reach that position.
	 */
	if((setcount==256 || y==254) && lastpix!=transparent)
	{ /*printf ("setcount=%d y=%d lastpix=%02X pix=%02X\n",
	    (int) setcount, (int) y, (int) lastpix, (int) pix);*/
	  if (setcount==256)
	    setcount--;  /* That's backtracking */
	  Set[1]=setcount;
	  Set[3+setcount]=lastpix;
	  setpos+= 3+setcount+1;
	  Set=(char  *)&(pic[colnpos+setpos]);
	  setcount=0;
	  Set[0]=y;			/* Y-offset */
	  Set[1]=0;			/* Count (updated later) */
	  Set[2]=pix;		/* Unused */
	}
	/* More than 509 pixels for this column. Quit. */
	if(y >= 509)
	{ Warning("PI20",
	    "Column has more than 509 pixels, truncating at (%d,%d)",
	    (int) x, (int) y);
	  break;
	}
	if(lastpix==transparent)  /* begining of post */
	{ Set=(char  *)&(pic[colnpos+setpos]);
	  setcount=0;
	  Set[0]=y;		/* y position */
	  Set[1]=0;		/*count (updated later)*/
	  Set[2]=pix;		/*unused*/
	}
	Set[3+setcount]=pix;/*non transparent pixel*/
	setcount++;          /*update count of pixel in set*/
      }
      else /*pix is transparent*/
      { if(lastpix!=transparent)/*finish the current set*/
	    { Set[1]=setcount;
	      Set[3+setcount]=lastpix;
	      setpos+= 3+setcount+1;/*1pos,1cnt,1dmy,setcount pixels,1dmy*/
	    }
	    /*else: not in set but in transparent area*/
      }
      lastpix=pix;
    }
    if(lastpix!=transparent)      /*finish current set, if any*/
    { Set[1]=setcount;
      Set[3+setcount]=lastpix;
      setpos+= 3+setcount+1;  /*1pos,1count,1dummy,setcount pixels,1dummy*/
    }
    pic[colnpos+setpos]=(char)0xFF; /*end of all sets*/
    colnpos+=(int32_t)(setpos+1);          /*position of next column*/
#if OPTIMEM /*optimisation*/
    if((colnpos+((int32_t)(5*rawY)/2)+1)>=picsz) /*avoid crash during next column*/
    { /*pic size was underestimated. need more pic size*/
      /*Bug("Pic size too small");*/
#if 1
      picsz= colnpos+0x4000;/*better make it incremental... not 161k!*/
#else
      picsz= colnbase + ((int32_t)rawX) * (1+5*(((int32_t)rawY+1)/2));
#endif
      pic = (char  *)Realloc(pic,picsz);
      ColOfs=(int32_t  *)&(pic[sizeof(struct PICHEAD)]);
    }
#endif
  }
  /*picsz was an overestimated size for PIC*/
  pic = (char  *)Realloc(pic,colnpos);
  *ppicsz= colnpos;/*real size of PIC*/;
  return  pic;
}


/*
** DoomPIC to RAW
**
** in:   int32_t picsz;        transparent;
**       char pic[picsz];
** out:  int16_t rawX; int16_t rawY;
**       char raw[rawX*rawY];
**  NULL if it's not a valid pic
**
** The <cusage> parameter is normally NULL. If -usedidx is given,
** it points to an object of type cusage_t.
** That block is for colour usage statistics and we update it.
*/
static char *PICtoRAW(int16_t *prawX,int16_t *prawY,int16_t *pXinsr,int16_t *pYinsr,
      const char  *pic,int32_t picsz,char transparent, const char *name,
      cusage_t *cusage)
{  int16_t x,y;         /*pixels*/
   const void *offsets;
   /* Last byte of pic buffer */
   const unsigned char *pic_end = ((const unsigned char *) pic) + picsz - 1;
   /*raw picture*/
   char col,notransp;
   char  *raw;
   int32_t rawpos,rawsz;
   pic_head_t h;
   int nw = 5;  /* Number of warnings left */

   if (cusage != NULL)
      usedidx_begin_lump (cusage, name);

   notransp=0;/* this is to avoid trouble when the transparent
		 index is used in a picture */

   /* Read the header of the picture */
   {
      char message_buf[81];
      if (parse_pic_header (pic, picsz, &h, message_buf))
      {
	 Warning ("PI10", "Picture %s: %s, skipping picture",
	     lump_name (name), message_buf);
	 return NULL;
      }
   }

   offsets = h.colofs;

   /* Read all columns, post by post */
   /* allocate raw. (care: free it if error, before exit) */
   /*Warning ("XX99", "%s: %d x %d",
      lump_name (name), (int) h.width, (int) h.height);*/
   rawsz = (long) h.width * h.height;
   raw = (char  *) Malloc (rawsz);
   Memset (raw, transparent, rawsz);
   for (x = 0; x < h.width; x++)
   {
      const unsigned char *post = NULL;	/* Initialised to avoid a warning */
      if (h.colofs_size == 4)
      {
	 int32_t ofs;
	 read_i32_le (((const int32_t *) offsets) + x, &ofs);
	 post = (const unsigned char *) (pic + ofs);
      }
      else if (h.colofs_size == 2)
      {
	 /* In principle, the offset is signed. However, considering it
	    unsigned helps extracting patches larger than 32 kB, like
	    W18_1 (alpha) or SKY* (PR). Interestingly, Doom alpha and
	    Doom PR treat the offset as signed, which is why some
	    textures appear with tutti-frutti on the right. -- AYM
	    1999-09-18 */
	 uint16_t ofs; 
	 read_i16_le (((const int16_t *) offsets) + x, (int16_t *) &ofs);
	 post = (const unsigned char *) (pic + ofs);
      }
      else
      {
	 Bug ("PI90", "Invalid colofs_size %d", (int) h.colofs_size);
	 /* Can't happen */
      }
      /*Warning ("XX99", "%s: %03d %p %p %p",
	  lump_name (name),
	  x,
	  pic,
	  post,
	  pic_end
	);*/

      if (post < (const unsigned char *) pic)
      {
	 LimitedWarn (&nw,
	     "PI11",
	     "Picture %s(%d): column has bad offset %ld, skipping column",
	     lump_name (name),
	     (int) x,
	     (long) ((const char *) pic - (const char *) post));
	 continue;
      }
      if (post < h.data)
      {
	 LimitedWarn (&nw,
	     "PI12",
	     "Picture %s(%d): column has suspicious offset %ld",
	     lump_name (name),
	     (int) x,
	     (long) ((const char *) pic - (const char *) post));
      }
      /* Read an entire post */
      for (;;)
      {
	int post_length;
	const unsigned char *post_end;
	if (post > pic_end)
	{
	   LimitedWarn (&nw,
	    "PI13",
	    "Picture %s(%d): post starts past EOL, skipping rest of column",
	       lump_name (name), (int) x); 
	   goto done_with_column;
	}
	/* Read the header of the post */
	if (*post == 0xff)
	   break;                         /* Last post */
	y           = *post++;
	post_length = *post++;
	if (h.dummy_bytes)
	   post++;
	post_end = post + post_length;
        if (post_end > pic_end)
	{
	  LimitedWarn (&nw,
	      "PI14",
	      "Picture %s(%d): post spans EOL, skipping rest of column",
	      lump_name (name), (int) x);
	  goto done_with_column;  /* Used to be fatal--AYM 1999-10-16 */
	}
	/* Read the middle of the post */
        for (;post < post_end; y++)
        { if (y >= h.height)
	  {
	    LimitedWarn (&nw,
		"PI15",
	  "Picture %s(%d): post Y-offset too high, skipping rest of column",
		lump_name (name), (int) x);
	    goto done_with_column;  /* Used to be fatal--AYM 1999-10-16 */
	  }
          col = *post++;
	  if (cusage != NULL)
	     usedidx_pixel (cusage, (unsigned char) col);
          if(col==transparent){col=notransp;}
          rawpos = x + (long) h.width * y;
          raw[rawpos]=col;
        }
	/* Read the trailer of the post */
	if (h.dummy_bytes)
	   post++;
      }
done_with_column:
      ;
   }

   /*return*/
   LimitedEpilog (&nw, "PI16", "Picture %s: ", lump_name (name));
   if (cusage != NULL)
     usedidx_end_lump (cusage);
   *pXinsr = h.xofs;
   *pYinsr = h.yofs;
   *prawX  = h.width;
   *prawY  = h.height;
   return raw;
}
/******************* End DoomPic module ************************/







/******************* Snea module ************************/
static char *snea_to_raw (int16_t *prawX, int16_t *prawY, int16_t *pXinsr,
    int16_t *pYinsr, const char *sneabuf, int32_t sneasz,
    const char *name, cusage_t *cusage)
{
  const unsigned char *snea        = (const unsigned char *) sneabuf;
  int                  width       = snea[0] * 4;
  int                  height      = snea[1];
  unsigned long        pixels      = (long) width * height;
  unsigned char       *raw         = Malloc (pixels);
  const unsigned char *s           = snea + 2;
  const unsigned char *end_of_snea = snea + sneasz;
  unsigned char       *r           = raw;
  unsigned char       *end_of_raw  = raw + pixels;

  if (pixels + 2 != sneasz)
  {
    Warning ("SN10",
	"Snea %s: mismatch between size and geometry, skipping snea",
	lump_name (name));
    Free (raw);
    return NULL;
  }

  while (s < end_of_snea)
  {
    *r = *s++;
    r += 4;
    if (r >= end_of_raw)
      r -= pixels - 1;
  }

  if (cusage != NULL)
    usedidx_rectangle (raw, pixels, name, cusage);

  *prawX = width;
  *prawY = height;
  return raw;
}

/******************* End snea module ************************/



/******************* ROTT wall module ************************/

/*
 *	wall_to_raw - convert ROTT wall data to a RAW bitmap
 *
 *	A ROTT wall is a 64x64 pixel matrix, read down and right. To
 *	convert it to RAW format, mirror it along the first diagonal
 *	((0,0)-(63,63)).
 */
#define WALL_DIM 64
static int wall_to_raw (char *data, int32_t datasz, const char *name)
{
  char *s = data + 1;
  char *d = s + WALL_DIM - 1;
  int i;
  int j;

  if (datasz != WALL_DIM * WALL_DIM)
  {
    Warning ("WA10", "Wall %s: size not %d",
	lump_name (name), WALL_DIM * WALL_DIM);
    return 0;
  }

  for (i = 1; i < WALL_DIM; i++)
  {
    for (j = i; j < WALL_DIM; j++)
    {
      char swap = *d;			/* Swap (*s) and (*d) */
      *d = *s;
      *s = swap;
      s++;
      d += WALL_DIM;
    }
    s += 1 + i;
    d = s + WALL_DIM - 1;
  }

  return 0;
}
/******************* End ROTT wall module ************************/



/*
  color index convertion bmp->doom
*/
static uint8_t Idx2Doom[256];



/******************* BMP module ************************/

struct BMPPALET { uint8_t B; uint8_t G; uint8_t R; uint8_t Zero;};
struct BMPPIXEL { uint8_t B; uint8_t G; uint8_t R;};
/*
** bitmap conversion
*/

struct BMPHEAD {
int32_t  bmplen;          /*02 total file length  Size*/
int32_t  reserved;                /*06 void Reserved1 Reserved2*/
int32_t  startpix;                /*0A start of pixels   OffBits*/
/*bitmap core header*/
int32_t  headsz;          /*0E Size =nb of bits in bc header*/
int32_t  szx;             /*12 X size = width     int16_t width*/
int32_t  szy;             /*16 Y size = height    int16_t height*/
int32_t  planebits;       /*1A equal to 1         word planes*/
                        /*1C nb of bits         word bitcount  1,4,8,24*/
/**/
int32_t  compress;                /*1E int32_t compression = BI_RGB = 0*/
int32_t  pixlen;          /*22 int32_t SizeImage    size of array necessary for pixels*/
int32_t  XpixRes;         /*26   XPelsPerMeter   X resolution no one cares*/
int32_t  YpixRes;            /*2A  YPelPerMeter    Y resolution  but code1a=code1b*/
int32_t  ColorUsed;               /*2C ClrUsed       nb of colors in palette*/
int32_t  ColorImp;                /*32 ClrImportant   nb of important colors in palette*/
/*palette pos: ((uint8_t *)&headsz) + headsz */
/*palette size = 4*nb of colors. order is Blue Green Red (Black? always0)*/
/*bmp line size is xsize*bytes_per_pixel aligned on int32_t */
/*pixlen = ysize * line size */
};
/*
** BMP to RAW conversion
**
** in: uint8_t *bmp;    int32_t bmpsize;     uint8_t COLindex(R,G,B);
**
** out:  uint8_t *raw;  int16_t rawX; int16_t rawY;
*/
static char *BMPtoRAW(int16_t *prawX,int16_t *prawY,char *file)
{
   struct BMPHEAD    *head;      /*bmp header*/
   struct BMPPALET   *palet;     /*palet. 8 bit*/
   int32_t paletsz;
   uint8_t  *line;                  /*line of BMP*/
   int32_t startpix,linesz=0;
   struct BMPPIXEL   *pixs;      /*pixels. 24 bit*/
   uint8_t   *bmpidxs;                     /*color indexs in BMP 8bit*/
   int16_t szx,szy,x,y,p,nbits;
   int32_t ncols;
   int32_t  bmpxy;
   char   *raw;                  /*point to pixs. 8 bit*/
   int32_t rawpos;
   uint8_t col='\0';
   FILE *fd;
   char sig[2];
   /*
   ** read BMP header for size
   */

   fd=fopen(file,FOPEN_RB);
   if(fd==NULL)
      ProgError("BR10", "%s: %s", fname (file), strerror (errno));
   if(fread(sig,2,1,fd)!=1)
      ProgError("BR11", "%s: read error in header", fname (file));
   if(strncmp(sig,"BM",2)!=0)
      ProgError("BR12", "%s: bad BMP magic", fname (file));

   head=(struct BMPHEAD  *)Malloc(sizeof(struct BMPHEAD));
   if(fread(head,sizeof(struct BMPHEAD),1,fd)!=1)
     ProgError("BR13", "%s: read error in header", fname (file));
   /*
   ** check the BMP header
   */
   if (peek_i32_le (&head->compress) != 0)
     ProgError("BR14", "%s: not an RGB BMP", fname (file));
   read_i32_le (&head->startpix, &startpix);
   szx = (int16_t) peek_i32_le (&head->szx);
   szy = (int16_t) peek_i32_le (&head->szy);
   if(szx<1) ProgError("BR15", "%s: bad width", fname (file));
   if(szy<1) ProgError("BR16", "%s: bad height", fname (file));
   ncols = peek_i32_le (&head->ColorUsed);
   /*
   ** Allocate memory for raw bytes
   */
   raw=(char  *)Malloc(((int32_t)szx)*((int32_t)szy));
   /*
   ** Determine line size and palet (if needed)
   */
   nbits = (int16_t) ((peek_i32_le (&head->planebits)>>16)&0xFFFFL);
   switch(nbits)
   { case 24:
       /*RGB, aligned mod 4*/
       linesz=((((int32_t)szx)*sizeof(struct BMPPIXEL))+3L)&~3L;
       break;
     case 8:
       linesz=(((int32_t)szx)+3L)&~3L;     /*Idx, aligned mod 4*/
       if(ncols>256)
	 ProgError("BR17", "%s: palette should have 256 entries", fname (file));
       if(ncols<=0) ncols=256;   /*Bug of PaintBrush*/
       paletsz=(ncols)*sizeof(struct BMPPALET);
       /*set position to palette  (a bit hacked)*/
       if(fseek(fd,0xEL+peek_i32_le(&head->headsz),SEEK_SET))
	 ProgError("BR18", "%s: can't seek to BMP palette (%s)",
	     fname (file),
	     strerror (errno));  /* FIXME mention the destination offset */
       /*load palette*/
       palet=(struct BMPPALET  *)Malloc(paletsz);
       if(fread(palet,(size_t)paletsz,1,fd)!=1)
	 ProgError("BR19", "%s: can't read palette", fname (file));
       for(p=0;p<ncols;p++)
       {  Idx2Doom[p]=COLindex(palet[p].R,palet[p].G,palet[p].B,(uint8_t)p);
       }
       Free(palet);
       break;
     default:
        ProgError("BR20", "%s: unsupported BMP type (%d bits)",
	    fname (file), nbits);
   }
   bmpxy=((int32_t)linesz)*((int32_t)szy);
   /*Most windows application bug on one of the other*/
   /*picture publisher: bmplen incorrect*/
   /*Micrographix: bmplen over estimated*/
   /*PaintBrush: pixlen is null*/
   if(head->pixlen<bmpxy)
     if(peek_i32_le(&head->bmplen)<(startpix+bmpxy))
       ProgError("BR21", "%s: size of pixel area incorrect", fname (file));

   Free(head);
   /* seek start of pixels */
   if(fseek(fd,startpix,SEEK_SET))
     ProgError("BR22", "%s: can't seek to pixel data (%s)",
	 fname (file),
	 strerror (errno));  /* FIXME mention the destination offset */
   /* read lines */
   line = (uint8_t  *)Malloc(linesz);
   bmpidxs=(uint8_t  *)line;
   pixs=(struct BMPPIXEL  *)line;
   /*convert bmp pixels/bmp indexs into doom indexs*/
   for(y=szy-1;y>=0;y--)
   {  if(fread(line,(size_t)linesz,1,fd)!=1)
         ProgError("BR23", "%s: read error in pixel data", fname (file));
      for(x=0;x<szx;x++)
      {  switch(nbits)
         {  case 24:
              col=COLindex(pixs[x].R,pixs[x].G,pixs[x].B,0);
            break;
            case 8:
              col=Idx2Doom[((int16_t)bmpidxs[x])&0xFF];
            break;
         }
         rawpos=((int32_t)x)+((int32_t)szx)*((int32_t)y);
         raw[rawpos]=col;
       }
   }
   Free(line);
   fclose(fd);
   *prawX=szx;
   *prawY=szy;
   return raw;
}
/*
** Raw to Bmp  8bit
**
** in:   int16_t rawX; int16_t rawY;  struct BMPPALET doompal[256]
**       uint8_t *raw
** out:  int32_t bmpsz;
**       uint8_t bmp[bmpsz];
*/
static void RAWtoBMP(char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal)
{
   struct BMPHEAD    *head; /*bmp header*/
   struct BMPPALET   *palet;/*palet. 8 bit*/
   uint8_t   *bmpidxs;                /*color indexs in BMP 8bit*/
   int16_t linesz;                /*size of line in Bmp*/
   int32_t  startpix,pixlen,bmplen,paletsz;
   int16_t x,y,ncol;
   int32_t  rawpos;
   FILE *fd;
   char sig[2];
   /*BMP 8 bits avec DOOM Palette*/

   fd=fopen(file,FOPEN_WB);

   if(fd==NULL)
     ProgError("BW10", "%s: %s",
	 fname (file),
	 strerror (errno));
   ncol=256;
   paletsz=ncol*sizeof(struct BMPPALET);
   linesz=(rawX+3)&(~3);  /*aligned mod 4*/
   pixlen=((int32_t)linesz)*((int32_t)rawY);
   startpix=2+sizeof(struct BMPHEAD)+paletsz;
   bmplen=startpix+pixlen;
   strncpy(sig,"BM",2);
   if(fwrite(sig,2,1,fd)!=1)
     ProgError("BW11", "%s: write error", fname (file));
   /*Header*/
   head=(struct BMPHEAD *)Malloc(sizeof(struct BMPHEAD));
   write_i32_le (&head->bmplen,    bmplen);
   write_i32_le (&head->reserved,  0);
   write_i32_le (&head->startpix,  startpix);
   write_i32_le (&head->headsz,    0x28);
   write_i32_le (&head->szx,       rawX);
   write_i32_le (&head->szy,       rawY);
   write_i32_le (&head->planebits, 0x80001);/*1 plane  8bits BMP*/
   write_i32_le (&head->compress,  0);   /* RGB */
   write_i32_le (&head->pixlen,    pixlen);
   write_i32_le (&head->XpixRes,   0);
   write_i32_le (&head->YpixRes,   0);
   write_i32_le (&head->ColorUsed, ncol);
   write_i32_le (&head->ColorImp,  ncol);
   if(fwrite(head,sizeof(struct BMPHEAD),1,fd)!=1)
     ProgError("BW12", "%s: write error", fname (file));
   Free(head);
   /*
   ** set palette
   **
   */
   palet=(struct BMPPALET *)Malloc(paletsz);
   for(x=0;x<ncol;x++)
   { palet[x].R = doompal[x].R;
     palet[x].G = doompal[x].G;
     palet[x].B = doompal[x].B;
     palet[x].Zero = 0;
   }
   if(fwrite(palet,(size_t)paletsz,1,fd)!=1)
     ProgError("BW13", "%s: write error", fname (file));
   Free(palet);
   /*
   ** set data
   **
   */
   bmpidxs=(uint8_t  *)Malloc(linesz);
   for(y=rawY-1;y>=0;y--)
   { for(x=0;x<rawX;x++)
     {   rawpos=((int32_t)x)+((int32_t)rawX)*((int32_t)y);
         bmpidxs[((int32_t)x)]=raw[rawpos];
     }
     for(;x<linesz;x++)
     {   bmpidxs[((int32_t)x)]=0;
     }
     if(fwrite(bmpidxs,1,linesz,fd)!=linesz)
       ProgError("BW14", "%s: write error", fname (file));
   }
   Free(bmpidxs);
   /*done*/
   if (fclose(fd))
     ProgError ("BW15", "%s: %s", fname (file), strerror (errno));
}
/******************* end BMP module ************************/




















/******************* PPM module ****************/
static void RAWtoPPM (char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal)
{
   FILE *fp;

   fp=fopen(file,FOPEN_WB);
   if(fp==NULL)
     ProgError("PW10", "%s: %s",
	 fname (file), strerror (errno));
   /* header */
   fprintf(fp,"P6\n%d %d\n255\n",rawX,rawY);
   /* data */
   {
     const unsigned char *p    = (const unsigned char *) raw;
     const unsigned char *pmax = p + (long) rawX * rawY;
     for (p = raw; p < pmax; p++)
       fwrite (doompal + *p, sizeof *doompal, 1, fp);
   }
   if (fclose(fp))
     ProgError ("PW11", "%s: %s", fname (file), strerror (errno));
}

static char *PPMtoRAW (int16_t *prawX, int16_t *prawY, char *file)
{
   int32_t rawpos, rawSz;
   int16_t rawX,rawY;
   char  *raw;                  /*point to pixs. 8 bit*/
   struct PIXEL pix;
   char buff[20];
   uint8_t c;int16_t n;
   FILE *fd;

   /*BMP 8 bits avec DOOM Palette*/

   fd=fopen(file,FOPEN_RB);
   if (fd==NULL)
     ProgError("PR10", "%s: %s", fname (file), strerror (errno));
   if(getc(fd)!='P' || getc(fd) != '6')
   {
     fclose(fd);
     ProgError ("PR11", "%s: not a rawbits PPM (P6) file", fname (file));
   }
   c=getc(fd);
   while(isspace(c))c=getc(fd);
   while(c=='#'){while(c!='\n'){c=getc(fd);}c=getc(fd);};
   while(isspace(c))c=getc(fd);
   for(n=0;n<10;n++){if(!isdigit(c))break;buff[n]=c;c=getc(fd);}buff[n]='\0';
   rawX=atoi(buff);
   while(isspace(c))c=getc(fd);
   while(c=='#'){while(c!='\n'){c=getc(fd);}c=getc(fd);};
   while(isspace(c))c=getc(fd);
   for(n=0;n<10;n++){if(!isdigit(c))break;buff[n]=c;c=getc(fd);}buff[n]='\0';
   rawY=atoi(buff);
   while(isspace(c))c=getc(fd);
   while(c=='#'){while(c!='\n'){c=getc(fd);}c=getc(fd);};
   while(isspace(c))c=getc(fd);
   for(n=0;n<10;n++){if(!isdigit(c))break;buff[n]=c;c=getc(fd);}buff[n]='\0';
   /* data */
   if(rawX<1)
   {
     fclose(fd);
     ProgError("PR12", "%s: width < 1 (%d)", fname (file), (int) rawX);
   }
   if(rawY<1)
   {
     fclose(fd);
     ProgError("PR13", "%s: height < 1 (%d)", fname (file), (int) rawY);
   }
   rawSz=((int32_t)rawX)*((int32_t)rawY);
   raw=(char  *)Malloc(rawSz);
   for(rawpos=0;rawpos<rawSz;rawpos++)
   { if(fread(&pix,sizeof(struct PIXEL),1,fd)!=1)
     {fclose(fd);ProgError("PR15", "%s: read error", fname (file));}
     raw[rawpos]=COLindex(pix.R,pix.G,pix.B,0);
   }
   fclose(fd);
   *prawX=rawX;
   *prawY=rawY;
   return raw;
}


/**************** end PPM module ***************/
















/******************* GIF module ************************/

static struct
{ uint16_t        Transparent;
  uint16_t        DelayTime;
  uint16_t        InputFlag;
  uint16_t        Disposal;
} Gif89 = { -1, -1, -1, 0 };
static struct
{ int16_t Width;
  int16_t Height;
  int16_t BitPixel;
  int16_t ColorRes;
  int16_t Backgnd;
  int16_t AspRatio;
} GifScreen;
static char  GifIdent[6];
const int GIFHEADsz = 2+2+1+1+1;
static struct GIFHEAD   /*size =7*/
{ int16_t xsize;
  int16_t ysize;
  uint8_t  info;     /*b7=colmap b6-4=colresol-1 b2-1=bitperpix-1*/
  uint8_t  backgnd;  /*Backg color*/
  uint8_t  aspratio; /*Aspect ratio*/
} GifHead;
struct PIXEL GifColor[256]; /*color map*/
const int GIFIMAGEsz = 2+2+2+2+1;
static struct GIFIMAGE  /*size =9*/
{  int16_t ofsx;  /*0,1 left offset*/
   int16_t ofsy;  /*2,3 top offset*/
   int16_t xsize; /*4,5 width*/
   int16_t ysize; /*6,7 heigth*/
   char info;   /*8   b7=colmap b6=interlace b2-1=bitperpix-1*/
} GifImage;

#define INTERLACE       0x40
#define COLORMAP        0x80



/*
** extern LZW routines
*/
#if NEWGIFE ||NEWGIFD
#include "gifcodec.h"
#endif
#if NEWGIFD
#else
extern void decompressInit(void);
extern void decompressFree(void);
extern int16_t LWZReadByte( FILE *fd, int16_t flag, int16_t input_code_size );
#endif
#if NEWGIF
#else
extern void compressInit(void);
extern void compressFree(void);
#endif


static char  *GIFreadPix(FILE *fd,int16_t Xsz,int16_t Ysz);
static void GIFextens(FILE *fd);
static char  *GIFintlace(char  *org,int16_t Xsz,int16_t Ysz);

/*
**  Read a Gif file
*/
static char *GIFtoRAW (int16_t *rawX, int16_t *rawY, char *file)
{   int16_t  Xsz=0,Ysz=0;
    static bool IntLace=false;
    int16_t  bitPixel;
    int32_t  rawSz,rawpos;
    int16_t  c;
    int chr;
    FILE        *fd;
    char    *raw = NULL;

   fd=fopen(file,FOPEN_RB);
   if (fd == NULL)
     ProgError("GR10", "%s: %s", fname (file), strerror (errno));
#if NEWGIFD
#else
   decompressInit();
#endif
   /*
   ** screen descriptor
   */
   if (fread (GifIdent,6,1,fd)!=1)
   {
     fclose(fd);
     ProgError ("GR11", "%s: read error in GIF magic", fname (file));
   }
   if (strncmp (GifIdent,"GIF87a",6) && strncmp (GifIdent,"GIF89a",6))
   {
     fclose(fd);
     ProgError ("GR12", "%s: not a 87a or 89a GIF", fname (file));
   }
   if (fread_i16_le (fd, &GifHead.xsize)
    || fread_i16_le (fd, &GifHead.ysize)
    || (chr = fgetc (fd), GifHead.info     = chr, chr == EOF)   /* Training */
    || (chr = fgetc (fd), GifHead.backgnd  = chr, chr == EOF)   /* for the */
    || (chr = fgetc (fd), GifHead.aspratio = chr, chr == EOF))  /* IOCCC */
   {
     fclose(fd);
     ProgError ("GR13", "%s: read error in GIF header", fname (file));
   }
   bitPixel=            1<<((GifHead.info&0x07)+1);
   GifScreen.BitPixel = bitPixel;
   GifScreen.ColorRes = (((GifHead.info>>3)&0xE)+1);
   GifScreen.Backgnd  = GifHead.backgnd;
   GifScreen.AspRatio = GifHead.aspratio;
   Memset(GifColor,0,256*sizeof(struct PIXEL));
   /* read Global Color Map */
   if ((GifHead.info)&COLORMAP)
   { if(fread(GifColor,sizeof(struct PIXEL),bitPixel,fd)!=bitPixel)
     { fclose(fd);
       ProgError ("GR14", "%s: read error in GIF data", fname (file));
     }
   }
   /*
   ** Read extension, images, etc...
   */
   while((c=getc(fd)) !=EOF)
   {  if(c==';') break; /* GIF terminator */
	  /*no need to test imagecount*/
      if(c=='!')        /* Extension */
	GIFextens(fd);
      else if(c==',')   /*valid image start*/
      { if(raw!=NULL)  /* only keep first image*/
	{ Warning("GR15", "%s: other GIF images discarded", fname (file));
	  break;
	}
	if (fread_i16_le (fd, &GifImage.ofsx)
	 || fread_i16_le (fd, &GifImage.ofsy)
	 || fread_i16_le (fd, &GifImage.xsize)
	 || fread_i16_le (fd, &GifImage.ysize)
	 || (chr = fgetc (fd), GifHead.info = chr, chr == EOF))
	{
	  fclose(fd);
	  ProgError ("GR16", "%s: read error", fname (file));
	}
	/* GifImage.ofsx,ofsy  X,Y offset ignored */
	bitPixel = 1<<((GifImage.info&0x07)+1);
	IntLace= (GifImage.info & INTERLACE)? true:false;
	Xsz = GifImage.xsize;
	Ysz = GifImage.ysize;
	if((Xsz<1)||(Ysz<1))
	{ fclose(fd);
	  ProgError ("GR17", "%s: bad size", fname (file));
	}
	if(GifImage.info & COLORMAP)
	{ if(fread(GifColor,sizeof(struct PIXEL),bitPixel,fd)!=bitPixel)
	  { fclose(fd);
	    ProgError ("GR18", "%s: read error", fname (file));
	  }
	}
	/*read the GIF. if many pictures, only the last
	  one is kept.
	*/
	raw= GIFreadPix(fd, Xsz,Ysz);
      }
      /*else, not a valid start character, skip to next*/
   }
   fclose(fd);
   if(raw == NULL)
     ProgError ("GR19", "%s: no picture found", fname (file));
   /*convert colors*/
   for (c=0; c<256; c++)
   { Idx2Doom[c]=(uint8_t)COLindex((uint8_t)GifColor[c].R,(uint8_t)GifColor[c].G,(uint8_t)GifColor[c].B,(uint8_t)c);
   }
   rawSz=((int32_t)Xsz) * ((int32_t)Ysz);
   for (rawpos=0; rawpos<rawSz;rawpos++)
   { raw[rawpos]=Idx2Doom[((int16_t)raw[rawpos])&0xFF];
   }
   /*unInterlace*/
   if(IntLace==true)
   { raw = GIFintlace(raw,Xsz,Ysz);
   }
   *rawX=Xsz;
   *rawY=Ysz;
   /* fclose(fd); */  /* Commented out AYM 1999-01-13. Why close twice ? */
#if NEWGIFD
#else
   decompressFree();
#endif
   return raw;
}
/*
** process the GIF extensions
*/
int16_t GIFreadBlock(FILE *fd, char buff[256])
{ int16_t data,count,c;
  if((data=fgetc(fd))==EOF) return -1;/*no data block*/
  count = data&0xFF;
  for(c=0;c<count;c++)
  {  if((data=fgetc(fd))==EOF) return -1;
     buff[c]= data&0xFF;
  }
  return count;
}
static void GIFextens(FILE *fd)
{ char  Buf[256];
  int16_t label;
  if((label=fgetc(fd))==EOF) return;
  switch (label&0xFF)
  { case 0x01:          /* Plain Text Extension */
      GIFreadBlock(fd, Buf);
      /*int16_t lpos; int16_t tpos;int16_t width;int16_t heigth;char cellw;char cellh;char foregr;char backgr*/
      break;
    case 0xff:          /* Application Extension */
      break;
    case 0xfe:          /* Comment Extension */
      break;
    case 0xf9:          /* Graphic Control Extension */
      GIFreadBlock(fd, Buf);
      Gif89.Disposal    = (Buf[0] >> 2) & 0x7;
      Gif89.InputFlag   = (Buf[0] >> 1) & 0x1;
      Gif89.DelayTime   = ((Buf[2]<<8)&0xFF00) + Buf[1];
      if(Buf[0] & 0x1)Gif89.Transparent = Buf[3];
      break;
    default:            /*Unknown GIF extension*/
      break;
  }
  while (GIFreadBlock(fd, Buf)>0);
}

/*
** Read Gif Indexes
*/
static char  *GIFreadPix(FILE *fd,int16_t Xsz,int16_t Ysz)
{  char  *raw=NULL;
   int32_t rawSz;
#if NEWGIFD
#else
   int16_t v;
   int32_t rawpos;
   unsigned char c=0;
#endif

   /*
   ** get some space
   */
   rawSz = ((int32_t)Xsz)*((int32_t)Ysz);
   raw = (char  *)Malloc(rawSz);
#if NEWGIFD
   InitDecoder( fd, 8, Xsz);
   Decode((uint8_t  *)raw, rawSz);
   ExitDecoder();
#else
   /* Initialize the Compression routines */
   if (fread(&c,1,1,fd)!=1) ProgError("GR50", "GIF: read error" );
   if (LWZReadByte(fd, true, c) < 0) ProgError("GR51", "GIF: bad code in image" );
   /* read the file */
   for(rawpos=0;rawpos<rawSz;rawpos++)
   { if ((v = LWZReadByte(fd,false,c)) < 0 ) ProgError("GR52", "GIF: too short");
     raw[rawpos]=(v&0xFF);
   }
   while (LWZReadByte(fd,false,c)>=0);  /* ignore extra data */
#endif
   return raw;
}

/*
**  Un-Interlace a GIF
*/
static char  *GIFintlace(char  *org,int16_t Xsz,int16_t Ysz)
{ int32_t rawpos,orgpos;
  int16_t pass,Ys=0,Y0=0,y;
  char  *raw;
  rawpos = ((int32_t)Xsz)*((int32_t)Ysz);
  raw = (char  *)Malloc(rawpos);
  orgpos = 0;
  for(pass=0;pass<4;pass++)
  {  switch(pass)
     { case 0:  Y0=0;   Ys=8;   break;
       case 1:  Y0=4;   Ys=8;   break;
       case 2:  Y0=2;   Ys=4;   break;
       case 3:  Y0=1;   Ys=2;   break;
     }
     rawpos=(int32_t)Y0*(int32_t)Xsz;
     for(y=Y0;y<Ysz;y+=Ys)
     { Memcpy(&raw[rawpos],&org[orgpos],(size_t)Xsz);
       rawpos+= (int32_t)Ys*(int32_t)Xsz;
       orgpos+= Xsz;
     }
  }
  Free(org);
  return raw;
}


/*
** write GIF
*/

#if NEWGIFE
#else
typedef int16_t         code_int;
extern void compress( int16_t init_bits, FILE *outfile, code_int (* ReadValue)(void));
static char  *Raw;
static int32_t CountTop=0;
static int32_t CountCur=0;
static code_int NextPixel(void )
{  char c;
   if(CountCur>=CountTop) return EOF;
   c=Raw[ CountCur];
   CountCur++;
   return ((code_int)c &0xFF);
}
#endif

static void RAWtoGIF (char *file, char *raw, int16_t rawX, int16_t rawY,
    struct PIXEL *doompal )
{  FILE *fd;
   int32_t rawSz;

   fd=fopen(file,FOPEN_WB);
   if (fd==NULL)
     ProgError("GW10", "%s: %s", fname (file), strerror (errno));
   rawSz = (int32_t)rawX * (int32_t)rawY;
   /* screen header */
   strncpy(GifIdent,"GIF87a",6);
   fwrite(GifIdent, 1, 6, fd );         /*header*/
   fwrite_u16_le (fd, rawX);				/* xsize */
   fwrite_u16_le (fd, rawY);				/* ysize */
   fputc (COLORMAP | ((8 - 1) << 4) | (8 - 1), fd);	/* info */
   /* global colormap, 256 colors, 7 bit per pixel*/
   fputc (0, fd);					/* backgnd */
   fputc (0, fd);					/* aspratio */
   fwrite(doompal,sizeof(struct PIXEL),256,fd); /*color map*/
   fputc (',',fd);                       /*Image separator*/
   /* image header */
   fwrite_u16_le (fd, 0);				/* ofsx */
   fwrite_u16_le (fd, 0);				/* ofsy */
   fwrite_u16_le (fd, rawX);				/* xsize*/
   fwrite_u16_le (fd, rawY);				/* ysize */
   fputc (0, fd);					/* info */
   /* image data */
   fputc(8,fd);     /* Write out the initial code size */
#if NEWGIFE
   InitEncoder(fd,8);
   Encode((uint8_t  *)raw,rawSz);
   ExitEncoder();
#else
   Raw      = raw;  /* init */
   CountTop = rawSz;
   CountCur = 0;
   compressInit();
   compress( 8+1, fd, NextPixel );  /*  write picture, InitCodeSize=8 */
   compressFree();
#endif
   /* termination */
   fputc(0,fd);   /*0 length packet to end*/
   fputc(';',fd); /*GIF file terminator*/
   if (fclose(fd))
     ProgError ("GW11", "%s: %s", fname (file), strerror (errno));
}
/******************* End GIF module ************************/
