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
    const unsigned char *buf,
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
   if (picture_format != PF_ALPHA)
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
   else
   {
      if (p + 4 - 1 > buf_end)
	 FAIL0 ("header too short");
      h->width  = *p++;
      h->height = *p++;
      h->xofs   = *p++;
      h->yofs   = *p++;
   }

   /* Sanity checks on picture size and offsets */
   if (h->width < 1)
      FAIL1 ("width < 1 (%d)", (int) h->width);
   if (h->width > 4096)
      FAIL1 ("width > 4096 (%d)", (int) h->width);
   if (h->height < 1)
      FAIL1 ("height < 1 (%d)", (int) h->height);
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

   /* Array of column offsets. Column data. */ 
   h->colofs = p;
   p += (long) h->colofs_size * h->width;
   if (p > buf_end)
      FAIL0 ("offsets table too short");
   h->data = p;
   return 0;  /* Success */
}


/*compile only for DeuTex*/
#if defined DeuTex



/* BMP,GIF,DoomPIC conversion
** intermediary format: RAW    (FLAT= RAW 64x64 or RAW 64x65)
**   Int16 Xsz Int16 Ysz
**   char idx[Xsz*Ysz]
**      colors= those of DOOM palette
**      position (x,y) = idx[x+xsz*y]
** insertion point not set
*/



/*
**
*/


static char *PICtoRAW (Int16 *prawX, Int16 *prawY, Int16 *pXinsr,
    Int16 *pYinsr, const char *pic,Int32 picsz, char transparent,
    const char *name, cusage_t *cusage);
static char *RAWtoPIC (Int32 *ppicsz, char *raw, Int16 rawX, Int16 rawY,
    Int16 Xinsr, Int16 Yinsr, char transparent);
static char *snea_to_raw (Int16 *prawX, Int16 *prawY, Int16 *pXinsr,
    Int16 *pYinsr, const char *snea, Int32 sneasz, const char *name,
    cusage_t *cusage);
static void RAWtoBMP (char *file, char *raw, Int16 rawX, Int16 rawY,
    struct PIXEL *doompal);
static char *BMPtoRAW(Int16 *prawX, Int16 *prawY, char *file);
static void RAWtoPPM (char *file, char *raw, Int16 rawX, Int16 rawY,
    struct PIXEL *doompal);
static char *PPMtoRAW (Int16 *prawX, Int16 *prawY, char *file);
static char *GIFtoRAW (Int16 *rawX, Int16 *rawY, char *file);
static void RAWtoGIF (char *file, char *raw, Int16 rawX, Int16 rawY,
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
   Int16 rawX,rawY;
   struct PIXEL  * doompal;

   Phase("BMP->RAW\n");
   MakeFileName(file,bmpdir,"","",name,"BMP");
   raw =BMPtoRAW(&rawX,&rawY,file);
   doompal = COLdoomPalet();
   Phase("RAW->GIF\n");
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
   Int16 rawX=0,rawY=0;
   struct PIXEL  * doompal;

   Phase("GIF->RAW\n");
   MakeFileName(file,bmpdir,"","",name,"GIF");
   raw =GIFtoRAW(&rawX,&rawY,file);
   doompal = COLdoomPalet();
   Phase("RAW->BMP\n");
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
Bool PICsaveInFile (char *file, PICTYPE type, char *pic, Int32 picsz,
    Int16 *pXinsr, Int16 *pYinsr, IMGTYPE Picture, const char *name,
    cusage_t *cusage)
{ char  *raw=NULL;
  Int16 rawX,rawY;
  struct PIXEL *doompal;
  char transparent;

  transparent =(char)COLinvisible();
  *pXinsr=INVALIDINT; /*default insertion point X*/
  *pYinsr=INVALIDINT; /*default insertion point Y*/
  switch(type)
  { case PGRAPH:
    case PWEAPN:
    case PSPRIT:
    case PPATCH:
      raw = PICtoRAW (&rawX, &rawY, pXinsr, pYinsr, pic, picsz, transparent,
	  name, cusage);
      if (raw==NULL) return FALSE;    /*was not a valid DoomPic*/
      break;

    case PFLAT:
      if(picsz==0x1000L)     { rawX=64;rawY=64;}
      else if(picsz==0x1040) { rawX=64;rawY=65;}/*bug?*/
      else if(picsz==0x2000) { rawX=64;rawY=128;}/*bug?*/
      else return FALSE;/*Wrong size for FLAT. F_SKY1*/
      raw=pic;    /*flat IS raw already*/
      if (cusage != NULL)
	usedidx_rectangle (pic, picsz, name, cusage);
      break;

    case PSNEAP:
    case PSNEAT:
      raw = snea_to_raw (&rawX, &rawY, pXinsr, pYinsr, pic, picsz, name,
	  cusage);
      if (raw == NULL)
	return FALSE;
      break;

    case PLUMP:
      /* Heretic, Hexen and Strife store some large bitmaps this way. */
      if(picsz==64000L)
      {
        rawX=320;
	rawY=200;
      }
      else
	return FALSE;/*Wrong size for LUMP. F_SKY1*/
      raw=pic;    /*flat IS raw already*/
      if (cusage != NULL)
	usedidx_rectangle (pic, picsz, name, cusage);
      break;

    default:
        Bug("picsv (%d)", (int) type);
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
	Bug("psvit (%d)", (int) Picture);
     }
  }

  switch(type)
  { case PGRAPH: case PWEAPN: case PSPRIT: case PPATCH:
    case PSNEAP: case PSNEAT:
     Free(raw);
     break;
    case PFLAT: /*don't free pic!*/
    case PLUMP:
     break;
  }
  return TRUE;
}


Int32 PICsaveInWAD(struct WADINFO *info,char *file,PICTYPE type,Int16 Xinsr,Int16 Yinsr,IMGTYPE Picture)
{
  char  *raw=NULL;
  Int16 rawX=0,rawY=0;
  char  *pic=NULL;
  Int32 picsz;
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
     Bug("pwvit");
  }
  if((rawY<1)||(rawX<1))ProgError("Bad picture size");
  // AYM (256 -> 509)
  if(rawY>509)ProgError("Picture height can't be more than 509");
  // AYM (512 -> 1024)
  if(rawX>1024)ProgError("Picture width can't be more than 1024");
  switch(type)
  { case PGRAPH: case PWEAPN: case PSPRIT: case PPATCH:
      break;
    case PFLAT: /*flats*/
      if(rawX!=64)
	Warning ("%s: weird width for a flat (not 64)", fname (file));
      if(rawY!=64 && rawY!=65 && rawY != 128)
	Warning ("%s: weird height for a flat (not 64, 65 or 128)", fname (file));
      break;
    case PLUMP: /*special heretic lumps*/
      if(rawX!=320)
        Warning ("%s: weird width for a lump (not 320)", fname (file));
      if(rawY!=200)
        Warning ("%s: weird height for a lump (not 200)", fname (file));
      break;
    default:
      Bug("picwt");
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
        picsz= ((Int32)rawX)*((Int32)rawY);
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
Int16 Xsz;               /*nb of columns*/
Int16 Ysz;               /*nb of rows*/
Int16 Xinsr;             /*insertion point*/
Int16 Yinsr;             /*insertion point*/
};
/*
** RAW to DoomPIC conversion
**
** in:   Int16 Xinsr; Int16 Yinsr;
**       Int16 rawX; Int16 rawY;  char transparent;
**       char raw[Xsize*Ysize]
** out:  Int32 picsz;           size of DoomPIC
**       char pic[picsz];      buffer for DoomPIC
*/
#if DT_OS == 'd'
#define OPTIMEM 1 /*DOS: alloc < 64k if possible*/
#else
#define OPTIMEM 0
#endif
char  *RAWtoPIC(Int32 *ppicsz, char  *raw, Int16 rawX, Int16 rawY,Int16 Xinsr,Int16 Yinsr, char transparent)
{  Int16 x,y;
   char pix,lastpix;            /*pixels*/
   /*Doom PIC */
   char  *pic;               /*picture*/
   Int32 picsz,rawpos;
   struct PICHEAD  *pichead; /*header*/
   Int32  *ColOfs;            /*position of column*/
   /*columns composed of sets */
   char  *Set=NULL;
   Int32 colnbase,colnpos,setpos;
   Int16 setcount=0;

  /*offset of first column*/
  colnbase= sizeof(struct PICHEAD)+((Int32)rawX)*sizeof(Int32);
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
  picsz= colnbase + ((Int32)rawX) * worst_case;
  }
#if OPTIMEM /*optimisation*/
  if(picsz>0x10000L) picsz=0x10000L;
#endif
  pic = (char  *)Malloc(picsz);
  ColOfs=(Int32  *)&(pic[sizeof(struct PICHEAD)]);
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
      rawpos=((Int32)x)+((Int32)rawX)*((Int32)y);
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
	{ printf ("setcount=%d y=%d lastpix=%02X pix=%02X\n",
	    (int) setcount, (int) y, (int) lastpix, (int) pix);
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
	{ Warning("Column has more than 509 pixels."
	    " Truncating at (%d,%d).\n", (int) x, (int) y);
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
    colnpos+=(Int32)(setpos+1);          /*position of next column*/
#if OPTIMEM /*optimisation*/
    if((colnpos+((Int32)(5*rawY)/2)+1)>=picsz) /*avoid crash during next column*/
    { /*pic size was underestimated. need more pic size*/
      /*Bug("Pic size too small");*/
#if 1
      picsz= colnpos+0x4000;/*better make it incremental... not 161k!*/
#else
      picsz= colnbase + ((Int32)rawX) * (1+5*(((Int32)rawY+1)/2));
#endif
      pic = (char  *)Realloc(pic,picsz);
      ColOfs=(Int32  *)&(pic[sizeof(struct PICHEAD)]);
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
** in:   Int32 picsz;        transparent;
**       char pic[picsz];
** out:  Int16 rawX; Int16 rawY;
**       char raw[rawX*rawY];
**  NULL if it's not a valid pic
**
** The <cusage> parameter is normally NULL. If -usedidx is given,
** it points to an object of type cusage_t.
** That block is for colour usage statistics and we update it.
*/
char  *PICtoRAW(Int16 *prawX,Int16 *prawY,Int16 *pXinsr,Int16 *pYinsr,
      const char  *pic,Int32 picsz,char transparent, const char *name,
      cusage_t *cusage)
{  Int16 x,y;         /*pixels*/
   const void *offsets;
   /* Last byte of pic buffer */
   const unsigned char *pic_end = ((const unsigned char *) pic) + picsz - 1;
   /*raw picture*/
   char col,notransp;
   char  *raw;
   Int32 rawpos,rawsz;
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
	 Warning ("Picture %s: %s. Skipping picture.",
	     lump_name (name), message_buf);
	 return NULL;
      }
   }

   offsets = h.colofs;

   /* Read all columns, post by post */
   /* allocate raw. (care: free it if error, before exit) */
   rawsz = (long) h.width * h.height;
   raw = (char  *) Malloc (rawsz);
   Memset (raw, transparent, rawsz);
   for (x = 0; x < h.width; x++)
   {
      const unsigned char *post;
      if (h.colofs_size == 4)
      {
	 Int32 ofs;
	 read_i32_le (((const Int32 *) offsets) + x, &ofs);
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
	 UInt16 ofs; 
	 read_i16_le (((const Int16 *) offsets) + x, (Int16 *) &ofs);
	 post = (const unsigned char *) (pic + ofs);
      }
      else
      {
	 Bug ("Bad colofs_size %d", (int) h.colofs_size);  /* Can't happen */
      }
      if (post < (const unsigned char *) pic)
      {
	 LimitedWarn (&nw,
	     "Picture %s(%d): column has bad offset %ld. Skipping column.",
	     lump_name (name),
	     (int) x,
	     (long) ((const char *) pic - (const char *) post));
	 continue;
      }
      if (post < h.data)
      {
	 LimitedWarn (&nw,
	     "Picture %s(%d): column has suspicious offset %ld.",
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
	    "Picture %s(%d): post starts past EOL. Skipping rest of column.",
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
	      "Picture %s(%d): post spans EOL. Skipping rest of column.",
	      lump_name (name), (int) x);
	  goto done_with_column;  /* Used to be fatal--AYM 1999-10-16 */
	}
	/* Read the middle of the post */
        for (;post < post_end; y++)
        { if (y >= h.height)
	  {
	    LimitedWarn (&nw,
	  "Picture %s(%d): post Y-offset too high. Skipping rest of column.",
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
   LimitedEpilog (&nw, "Picture %s: ", lump_name (name));
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
static char *snea_to_raw (Int16 *prawX, Int16 *prawY, Int16 *pXinsr,
    Int16 *pYinsr, const char *sneabuf, Int32 sneasz,
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
    Warning ("Snea %s: mismatch between size and geometry. Skipping snea.",
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






/*
  color index convertion bmp->doom
*/
static UInt8 Idx2Doom[256];



/******************* BMP module ************************/

struct BMPPALET { UInt8 B; UInt8 G; UInt8 R; UInt8 Zero;};
struct BMPPIXEL { UInt8 B; UInt8 G; UInt8 R;};
/*
** bitmap conversion
*/

struct BMPHEAD {
Int32  bmplen;          /*02 total file length  Size*/
Int32  reserved;                /*06 void Reserved1 Reserved2*/
Int32  startpix;                /*0A start of pixels   OffBits*/
/*bitmap core header*/
Int32  headsz;          /*0E Size =nb of bits in bc header*/
Int32  szx;             /*12 X size = width     Int16 width*/
Int32  szy;             /*16 Y size = height    Int16 height*/
Int32  planebits;       /*1A equal to 1         word planes*/
                        /*1C nb of bits         word bitcount  1,4,8,24*/
/**/
Int32  compress;                /*1E Int32 compression = BI_RGB = 0*/
Int32  pixlen;          /*22 Int32 SizeImage    size of array necessary for pixels*/
Int32  XpixRes;         /*26   XPelsPerMeter   X resolution no one cares*/
Int32  YpixRes;            /*2A  YPelPerMeter    Y resolution  but code1a=code1b*/
Int32  ColorUsed;               /*2C ClrUsed       nb of colors in palette*/
Int32  ColorImp;                /*32 ClrImportant   nb of important colors in palette*/
/*palette pos: ((UInt8 *)&headsz) + headsz */
/*palette size = 4*nb of colors. order is Blue Green Red (Black? always0)*/
/*bmp line size is xsize*bytes_per_pixel aligned on Int32 */
/*pixlen = ysize * line size */
};
/*
** BMP to RAW conversion
**
** in: UInt8 *bmp;    Int32 bmpsize;     UInt8 COLindex(R,G,B);
**
** out:  UInt8 *raw;  Int16 rawX; Int16 rawY;
*/
char  *BMPtoRAW(Int16 *prawX,Int16 *prawY,char *file)
{
   struct BMPHEAD    *head;      /*bmp header*/
   struct BMPPALET   *palet;     /*palet. 8 bit*/
   Int32 paletsz;
   UInt8  *line;                  /*line of BMP*/
   Int32 startpix,linesz=0;
   struct BMPPIXEL   *pixs;      /*pixels. 24 bit*/
   UInt8   *bmpidxs;                     /*color indexs in BMP 8bit*/
   Int16 szx,szy,x,y,p,nbits;
   Int32 ncols;
   Int32  bmpxy;
   char   *raw;                  /*point to pixs. 8 bit*/
   Int32 rawpos;
   UInt8 col='\0';
   FILE *fd;
   char sig[2];
   /*
   ** read BMP header for size
   */

   fd=fopen(file,FOPEN_RB);
   if(fd==NULL)
      ProgError("Can't open %s for reading",file);
   if(fread(sig,2,1,fd)!=1)
      ProgError("Can't read sig of BMP %s",file);
   if(strncmp(sig,"BM",2)!=0)
      ProgError("Bmp: signature incorrect");

   head=(struct BMPHEAD  *)Malloc(sizeof(struct BMPHEAD));
   if(fread(head,sizeof(struct BMPHEAD),1,fd)!=1)
     ProgError("Bmp: can't read header");
   /*
   ** check the BMP header
   */
   if (peek_i32_le (&head->compress) != 0) ProgError("Bmp: not RGB");
   read_i32_le (&head->startpix, &startpix);
   szx = (Int16) peek_i32_le (&head->szx);
   szy = (Int16) peek_i32_le (&head->szy);
   if(szx<1) ProgError("Bmp: bad width");
   if(szy<1) ProgError("Bmp: bad height");
   ncols = peek_i32_le (&head->ColorUsed);
   /*
   ** Allocate memory for raw bytes
   */
   raw=(char  *)Malloc(((Int32)szx)*((Int32)szy));
   /*
   ** Determine line size and palet (if needed)
   */
   nbits = (Int16) ((peek_i32_le (&head->planebits)>>16)&0xFFFFL);
   switch(nbits)
   { case 24:
       {
	 static warned_once = 0;

	 if (! warned_once)
	 {
	   Info("Warning: Color quantisation is slow. Use 8 bit BMP.\n");
	   warned_once = 1;
	 }
       }
       linesz=((((Int32)szx)*sizeof(struct BMPPIXEL))+3L)&~3L; /*RGB, aligned mod 4*/
       break;
     case 8:
       linesz=(((Int32)szx)+3L)&~3L;     /*Idx, aligned mod 4*/
       if(ncols>256)                    ProgError("Bmp: palette should be 256 color");
       if(ncols<=0) ncols=256;   /*Bug of PaintBrush*/
       paletsz=(ncols)*sizeof(struct BMPPALET);
       /*set position to palette  (a bit hacked)*/
       if(fseek(fd,0xEL+peek_i32_le(&head->headsz),SEEK_SET))ProgError("Bmp: seek failed");
       /*load palette*/
       palet=(struct BMPPALET  *)Malloc(paletsz);
       if(fread(palet,(size_t)paletsz,1,fd)!=1)   ProgError("Bmp: can't read palette");
       for(p=0;p<ncols;p++)
       {  Idx2Doom[p]=COLindex(palet[p].R,palet[p].G,palet[p].B,(UInt8)p);
       }
       Free(palet);
       break;
     default:
        ProgError("Bmp: type not supported (%d bits)",nbits);
   }
   bmpxy=((Int32)linesz)*((Int32)szy);
   /*Most windows application bug on one of the other*/
   /*picture publisher: bmplen incorrect*/
   /*Micrographix: bmplen over estimated*/
   /*PaintBrush: pixlen is null*/
   if(head->pixlen<bmpxy)
     if(peek_i32_le(&head->bmplen)<(startpix+bmpxy))
       ProgError("Bmp: size of pixel area incorrect");

   Free(head);
   /* seek start of pixels */
   if(fseek(fd,startpix,SEEK_SET))      ProgError("Bmp: seek failed");
   /* read lines */
   line = (UInt8  *)Malloc(linesz);
   bmpidxs=(UInt8  *)line;
   pixs=(struct BMPPIXEL  *)line;
   /*convert bmp pixels/bmp indexs into doom indexs*/
   for(y=szy-1;y>=0;y--)
   {  if(fread(line,(size_t)linesz,1,fd)!=1)    ProgError("Can't read BMP line");
      for(x=0;x<szx;x++)
      {  switch(nbits)
         {  case 24:
              col=COLindex(pixs[x].R,pixs[x].G,pixs[x].B,0);
            break;
            case 8:
              col=Idx2Doom[((Int16)bmpidxs[x])&0xFF];
            break;
         }
         rawpos=((Int32)x)+((Int32)szx)*((Int32)y);
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
** in:   Int16 rawX; Int16 rawY;  struct BMPPALET doompal[256]
**       UInt8 *raw
** out:  Int32 bmpsz;
**       UInt8 bmp[bmpsz];
*/
void RAWtoBMP(char *file,char  *raw,Int16 rawX, Int16 rawY,struct PIXEL  *doompal)
{
   struct BMPHEAD    *head; /*bmp header*/
   struct BMPPALET   *palet;/*palet. 8 bit*/
   UInt8   *bmpidxs;                /*color indexs in BMP 8bit*/
   Int16 linesz;                /*size of line in Bmp*/
   Int32  startpix,pixlen,bmplen,paletsz;
   Int16 x,y,ncol;
   Int32  rawpos;
   FILE *fd;
   char sig[2];
   /*BMP 8 bits avec DOOM Palette*/

  fd=fopen(file,FOPEN_WB);

   if(fd==NULL)
     ProgError("Can't create %s (%s)", file, strerror (errno));
   ncol=256;
   paletsz=ncol*sizeof(struct BMPPALET);
   linesz=(rawX+3)&(~3);  /*aligned mod 4*/
   pixlen=((Int32)linesz)*((Int32)rawY);
   startpix=2+sizeof(struct BMPHEAD)+paletsz;
   bmplen=startpix+pixlen;
   strncpy(sig,"BM",2);
   if(fwrite(sig,2,1,fd)!=1)   ProgError("Can't write file %s",file);
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
     ProgError("Can't write file %s",file);
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
     ProgError("Can't write file %s",file);
   Free(palet);
   /*
   ** set data
   **
   */
   bmpidxs=(UInt8  *)Malloc(linesz);
   for(y=rawY-1;y>=0;y--)
   { for(x=0;x<rawX;x++)
     {   rawpos=((Int32)x)+((Int32)rawX)*((Int32)y);
         bmpidxs[((Int32)x)]=raw[rawpos];
     }
     for(;x<linesz;x++)
     {   bmpidxs[((Int32)x)]=0;
     }
     if(fwrite(bmpidxs,1,linesz,fd)!=linesz)
       ProgError("Can't write file %s",file);
   }
   Free(bmpidxs);
   /*done*/
   fclose(fd);
}
/******************* end BMP module ************************/




















/******************* PPM module ****************/
void RAWtoPPM(char *file, char *raw, Int16 rawX, Int16 rawY,
    struct PIXEL *doompal)
{
   FILE *fp;

   fp=fopen(file,FOPEN_WB);
   if(fp==NULL)
     ProgError("Can't create %s (%s)", file, strerror (errno));
   /* header */
   fprintf(fp,"P6\n%d %d\n255\n",rawX,rawY);
   /* data */
   {
     const unsigned char *p    = (const unsigned char *) raw;
     const unsigned char *pmax = p + (long) rawX * rawY;
     for (p = raw; p < pmax; p++)
       fwrite (doompal + *p, sizeof *doompal, 1, fp);
   }
   fclose(fp);
}

char *PPMtoRAW(Int16 *prawX,Int16 *prawY,char *file)
{
   Int32 rawpos, rawSz;
   Int16 rawX,rawY;
   char  *raw;                  /*point to pixs. 8 bit*/
   struct PIXEL pix;
   char buff[20];
   UInt8 c;Int16 n;
   FILE *fd;
   static warned_once = 0;

   /*BMP 8 bits avec DOOM Palette*/

   fd=fopen(file,FOPEN_RB);
   if(fd==NULL)     ProgError("Can't open %s",file);
   if(getc(fd)!='P' || getc(fd) != '6')
   {
     fclose(fd);
     ProgError("%s is not a rawbits PPM (P6) file");
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
   if(rawX<1) {fclose(fd);ProgError("%s: bad width", file);}
   if(rawY<1) {fclose(fd);ProgError("%s: bad height", file);}
   rawSz=((Int32)rawX)*((Int32)rawY);
   raw=(char  *)Malloc(rawSz);
   if (! warned_once)
   {
     Info("Warning: Color quantisation is slow. Use ppmquant.\n");
     warned_once = 1;
   }
   for(rawpos=0;rawpos<rawSz;rawpos++)
   { if(fread(&pix,sizeof(struct PIXEL),1,fd)!=1)
     {fclose(fd);ProgError("%s: unexpected EOF");}
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
{ UInt16        Transparent;
  UInt16        DelayTime;
  UInt16        InputFlag;
  UInt16        Disposal;
} Gif89 = { -1, -1, -1, 0 };
static struct
{ Int16 Width;
  Int16 Height;
  Int16 BitPixel;
  Int16 ColorRes;
  Int16 Backgnd;
  Int16 AspRatio;
} GifScreen;
static char  GifIdent[6];
const int GIFHEADsz = 2+2+1+1+1;
static struct GIFHEAD   /*size =7*/
{ Int16 xsize;
  Int16 ysize;
  UInt8  info;     /*b7=colmap b6-4=colresol-1 b2-1=bitperpix-1*/
  UInt8  backgnd;  /*Backg color*/
  UInt8  aspratio; /*Aspect ratio*/
} GifHead;
struct PIXEL GifColor[256]; /*color map*/
const int GIFIMAGEsz = 2+2+2+2+1;
static struct GIFIMAGE  /*size =9*/
{  Int16 ofsx;  /*0,1 left offset*/
   Int16 ofsy;  /*2,3 top offset*/
   Int16 xsize; /*4,5 width*/
   Int16 ysize; /*6,7 heigth*/
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
extern Int16 LWZReadByte( FILE *fd, Int16 flag, Int16 input_code_size );
#endif
#if NEWGIF
#else
extern void compressInit(void);
extern void compressFree(void);
#endif


static char  *GIFreadPix(FILE *fd,Int16 Xsz,Int16 Ysz);
static void GIFextens(FILE *fd);
static char  *GIFintlace(char  *org,Int16 Xsz,Int16 Ysz);

/*
**  Read a Gif file
*/
char  *GIFtoRAW(Int16 *rawX,Int16 *rawY,char *file)
{   Int16  Xsz=0,Ysz=0;
    static Bool IntLace=FALSE;
    Int16  bitPixel;
    Int32  rawSz,rawpos;
    Int16  c;
    int chr;
    FILE        *fd;
    char    *raw = NULL;

   fd=fopen(file,FOPEN_RB);
   if (fd == NULL) ProgError("Can't open %s",file);
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
     ProgError("GIF: read error");
   }
   if (strncmp (GifIdent,"GIF87a",6) && strncmp (GifIdent,"GIF89a",6))
   {
     fclose(fd);
     ProgError("GIF: not a 87a or 89a GIF");
   }
   if (fread_u16_le (fd, &GifHead.xsize)
    || fread_u16_le (fd, &GifHead.ysize)
    || (chr = fgetc (fd), GifHead.info     = chr, chr == EOF)   /* Training */
    || (chr = fgetc (fd), GifHead.backgnd  = chr, chr == EOF)   /* for the */
    || (chr = fgetc (fd), GifHead.aspratio = chr, chr == EOF))  /* IOCCC */
   {
     fclose(fd);
     ProgError("GIF: read error");
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
     { fclose(fd);ProgError("GIF: read error");
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
	{ Warning("GIF: Other images discarded.");
	  break;
	}
	if (fread_u16_le (fd, &GifImage.ofsx)
	 || fread_u16_le (fd, &GifImage.ofsy)
	 || fread_u16_le (fd, &GifImage.xsize)
	 || fread_u16_le (fd, &GifImage.ysize)
	 || (chr = fgetc (fd), GifHead.info = chr, chr == EOF))
	{
	  fclose(fd);
	  ProgError("GIF: read error");
	}
	/* GifImage.ofsx,ofsy  X,Y offset ignored */
	bitPixel = 1<<((GifImage.info&0x07)+1);
	IntLace= (GifImage.info & INTERLACE)? TRUE:FALSE;
	Xsz = GifImage.xsize;
	Ysz = GifImage.ysize;
	if((Xsz<1)||(Ysz<1))
	{ fclose(fd);ProgError("GIF: bad size");
	}
	if(GifImage.info & COLORMAP)
	{ if(fread(GifColor,sizeof(struct PIXEL),bitPixel,fd)!=bitPixel)
	  { fclose(fd);ProgError("GIF: read error");
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
   if(raw == NULL) ProgError("GIF: No picture found");
   /*convert colors*/
   for (c=0; c<256; c++)
   { Idx2Doom[c]=(UInt8)COLindex((UInt8)GifColor[c].R,(UInt8)GifColor[c].G,(UInt8)GifColor[c].B,(UInt8)c);
   }
   rawSz=((Int32)Xsz) * ((Int32)Ysz);
   for (rawpos=0; rawpos<rawSz;rawpos++)
   { raw[rawpos]=Idx2Doom[((Int16)raw[rawpos])&0xFF];
   }
   /*unInterlace*/
   if(IntLace==TRUE)
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
Int16 GIFreadBlock(FILE *fd, char buff[256])
{ Int16 data,count,c;
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
  Int16 label;
  if((label=fgetc(fd))==EOF) return;
  switch (label&0xFF)
  { case 0x01:          /* Plain Text Extension */
      GIFreadBlock(fd, Buf);
      /*Int16 lpos; Int16 tpos;Int16 width;Int16 heigth;char cellw;char cellh;char foregr;char backgr*/
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
static char  *GIFreadPix(FILE *fd,Int16 Xsz,Int16 Ysz)
{  char  *raw=NULL;
   Int32 rawSz;
#if NEWGIFD
#else
   Int16 v;
   Int32 rawpos;
   unsigned char c=0;
#endif

   /*
   ** get some space
   */
   rawSz = ((Int32)Xsz)*((Int32)Ysz);
   raw = (char  *)Malloc(rawSz);
#if NEWGIFD
   InitDecoder( fd, 8, Xsz);
   Decode((UInt8  *)raw, rawSz);
   ExitDecoder();
#else
   /* Initialize the Compression routines */
   if (fread(&c,1,1,fd)!=1) ProgError("GIF: read error" );
   if (LWZReadByte(fd, TRUE, c) < 0) ProgError("GIF: bad code in image" );
   /* read the file */
   for(rawpos=0;rawpos<rawSz;rawpos++)
   { if ((v = LWZReadByte(fd,FALSE,c)) < 0 ) ProgError("GIF: too short");
     raw[rawpos]=(v&0xFF);
   }
   while (LWZReadByte(fd,FALSE,c)>=0);  /* ignore extra data */
#endif
   return raw;
}

/*
**  Un-Interlace a GIF
*/
static char  *GIFintlace(char  *org,Int16 Xsz,Int16 Ysz)
{ Int32 rawpos,orgpos;
  Int16 pass,Ys=0,Y0=0,y;
  char  *raw;
  rawpos = ((Int32)Xsz)*((Int32)Ysz);
  raw = (char  *)Malloc(rawpos);
  orgpos = 0;
  for(pass=0;pass<4;pass++)
  {  switch(pass)
     { case 0:  Y0=0;   Ys=8;   break;
       case 1:  Y0=4;   Ys=8;   break;
       case 2:  Y0=2;   Ys=4;   break;
       case 3:  Y0=1;   Ys=2;   break;
     }
     rawpos=(Int32)Y0*(Int32)Xsz;
     for(y=Y0;y<Ysz;y+=Ys)
     { Memcpy(&raw[rawpos],&org[orgpos],(size_t)Xsz);
       rawpos+= (Int32)Ys*(Int32)Xsz;
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
typedef Int16         code_int;
extern void compress( Int16 init_bits, FILE *outfile, code_int (* ReadValue)(void));
static char  *Raw;
static Int32 CountTop=0;
static Int32 CountCur=0;
static code_int NextPixel(void )
{  char c;
   if(CountCur>=CountTop) return EOF;
   c=Raw[ CountCur];
   CountCur++;
   return ((code_int)c &0xFF);
}
#endif

void RAWtoGIF(char *file,char  *raw,Int16 rawX,Int16 rawY,struct PIXEL *doompal )
{  FILE *fd;
   Int32 rawSz;

   fd=fopen(file,FOPEN_WB);
   if(fd==NULL)ProgError("Can't open GIF file %s",file);
   rawSz = (Int32)rawX * (Int32)rawY;
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
   Encode((UInt8  *)raw,rawSz);
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
   fclose(fd);    /*the end*/
}
/******************* End GIF module ************************/

#endif /*DeuTex*/
