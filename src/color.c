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


/*select your quantisation method*/
/*#define QUANTSLOW*/
#define QUANTHASH 


#include "deutex.h"
#include "tools.h"
#include "color.h"


/*compile only for DeuTex*/
#if defined DeuTex

#ifdef QUANTHASH
/*************** COL module: quantisation ************/
/*
** hash table
**
*/
UInt8 COLindex( UInt8 R, UInt8 G, UInt8 B,UInt8 index);
UInt8 COLpalMatch( UInt8 R, UInt8 G, UInt8 B);


static struct PIXEL *COLpal;  /* The game palette (comes from PLAYPAL) */
static struct PIXEL *COLpalAlt = NULL;  /* Alternate palette (TITLEPAL) */
static struct PIXEL COLinv;
static  UInt8 COLinvisib;
static Bool COLok=FALSE;


/*
 *	pixel_cmp
 *	Compare two PIXEL structure à la memcmp()
 */
static int pixel_cmp (const void *pixel1, const void *pixel2)
{
  const struct PIXEL *const p1 = (const struct PIXEL *) pixel1;
  const struct PIXEL *const p2 = (const struct PIXEL *) pixel2;
  if (p1->R != p2->R)
     return p1->R - p2->R;
  if (p1->G != p2->G)
     return p1->G - p2->G;
  if (p1->B != p2->B)
     return p1->B - p2->B;
  return 0;

}

const int COLsame = 3;
Int16 COLdiff( UInt8 R,  UInt8 G,  UInt8 B, UInt8 idx)
{
   register struct PIXEL  *pixel = &COLpal[(Int16)(idx&0xFF)];
   register Int16 d; /*signed*/
   register Int16 e=0;
   d= (((Int16)R)&0xFF) - (((Int16) pixel->R) &0xFF);
   d>>=1;
   e+= d*d;
   d= (((Int16)G)&0xFF) - (((Int16) pixel->G) &0xFF);
   d>>=1;
   e+= d*d;
   d= (((Int16)B)&0xFF) - (((Int16) pixel->B) &0xFF);
   d>>=1;
   e+= d*d;
   if(e<0) return 0x7FFF;
   return e;
}

UInt8 COLpalMatch( UInt8 R, UInt8 G, UInt8 B)
{  Int16 i,test,min=0x7FFF;
   UInt8 idxmin='\0';
   if(COLok!=TRUE) Bug("COLok");
   for(i=0;i<256;i++)
   {  if((UInt8)i!=COLinvisib)
      { test=COLdiff(R,G,B,(UInt8)i);
	if(test<min) {min=test;idxmin=(UInt8)i;}
	if(min<COLsame)
	{ break;
        }
      }
   }
   return idxmin;
}



/* Choose only 10,11...16 */
#define POWER 12
const Int16 HashP2 = POWER;                   /* 10=1024*/
const Int16 HashSz = 1<<POWER;		/* 1<< HashP2	*/
const Int16 HashMask = (1<<POWER)-1;		/* HashSz-1	*/
/*const Int16 HashStop = -1;*/
static UInt8  *COLhash;              /*hash table*/
/*static Int16  *COLnext;*/


Int16 Hash(UInt8 r,UInt8 g,UInt8 b)
{ int res;
  UInt8 R=r&0xFC,G=g&0xFC,B=b&0xFC;
  res = (((R<<3)^G)<<2)^B;
  res = (res<<3)+(R&0xC3)+(G&0x61)+(~B&0x98);
  res = (res<<5)+ (R&0x3B)+(~G&0x95)+(B&0x33);
  res ^= res >>8;
  res &= HashMask;
  return  (Int16) res;
}

/*
void COLhashPrint(void)
{ Int16 idx,i;
  UInt8 res;
  UInt8 buff[64];
  Int16 count=0;
  for(idx=0;idx<HashSz;idx+=64)
  { for(i=0;i<64;i++)
    { res=COLhash[(idx+i)&HashMask];
      if(res==COLinvisib)
      buff[i]=' ';
      else
      { buff[i]=('0'+(res&0x3F));
		count++;
	  }
	}
   fprintf(COLfp,"[%.32s]\n",buff);
  }
   fprintf(COLfp,"\nHash used %d out of %d\n",count,HashSz);
}
*/

/*original colors*/
void COLputColHash(Int16 index,UInt8 R,UInt8 G,UInt8 B)
{ Int16 count,idx,nextidx;
  idx=Hash(R,G,B);
  for(count=0;count<8;count++)
  { nextidx=(idx+count)&HashMask;
	if(COLhash[nextidx]==COLinvisib)
	{ COLhash[nextidx]=(UInt8)(index&0xFF);
	  return;
	}
  }
  Bug("Can't hash Doom pal ");
}

/*new colors, with matching*/
UInt8 COLgetIndexHash(UInt8 R,UInt8 G,UInt8 B)
{ Int16 idx,nextidx,count;
  UInt8 res;
  idx = Hash(R,G,B);
  for(count=0;count<8;count++)
  { nextidx=(idx+count)&HashMask;
	res=COLhash[nextidx];
	if(res==COLinvisib)/*free*/
	{ COLhash[nextidx]=res=COLpalMatch(R,G,B);
	  return res;
	}
	else if (COLdiff(R,G,B,res)<COLsame)
	{
/*
	  if(count>2)
	  { if(res==COLhash[(nextidx-1)&HashMask])
	  fprintf(COLfp,"multi %d in %d\n",(int)res,nextidx);
	idx=((Int16)res)&0xFF;
	fprintf(COLfp,"%d\t%d\t%d\t*\n",(int)(R-COLpal[idx].R),(int)(G-COLpal[idx].G),(int)(B-COLpal[idx].B));

	  }
*/
	  return res;
	}
  }
  /*no good solution. slow match*/
  return COLpalMatch(R,G,B);
}



/*
 *	Normal palette (PLAYPAL)
 */
void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,char  *Colors, Int16 Colsz)
{  Int16 i;
   const char *name = NULL;
   /*Int16 R,G,B;*/
   if(COLok!=FALSE) Bug("COLok");
   if(Colsz< 256*sizeof(struct PIXEL)) Bug("Color entry too small");
   COLok=TRUE;
   COLpal= (struct PIXEL  *)Malloc(256*sizeof(struct PIXEL));
   for(i=0;i< NCOLOURS;i++)
   {
     COLpal[i].R=Colors[i*3+0];
     COLpal[i].G=Colors[i*3+1];
     COLpal[i].B=Colors[i*3+2];
   }
#if 1  /*supposedly exact...*/
   if (COLpal[0].R == 0 && COLpal[0].G == 0 && COLpal[0].B == 0
     && COLpal[0xf7].R == 0 && COLpal[0xf7].G == 0 && COLpal[0xf7].B == 0)
   { i = 0xf7;
     name = "Doom";
   }
   else if (COLpal[35].R == 255 && COLpal[35].G == 255 && COLpal[35].B == 255
     && COLpal[255].R == 255 && COLpal[255].G == 255 && COLpal[255].B == 255)
   { i = 0xff;
     name = "Heretic";
   }
#if 0
   else if (0)  /* FIXME */
   {
     name = "Hexen";
   }
#endif
   else if (COLpal[0].R == 0 && COLpal[0].G == 0 && COLpal[0].B == 0
     && COLpal[240].R == 0 && COLpal[240].G == 0 && COLpal[240].B == 0)
   { i = 0xf0;
     name = "Strife";
   }
   else
   { i = 0xff;
     name = NULL;
   }
#else  /*screws some little points, on some sprites*/
   i=0xFF; /*DOOM and HERETIC*/
#endif
   /*
   ** correction to doom palette
   */
   COLinvisib= (UInt8)(i&0xFF);
   Info("Color palette is %s\n", name ? name : "unknown");
   if (name == NULL)
       Info ("WARNING unknown palette. Using colour 0xff as transparent"
	     " colour.\nSome graphics may appear moth-eaten.\n");
   COLinv.R=COLpal[i].R=invR;
   COLinv.G=COLpal[i].G=invG;
   COLinv.B=COLpal[i].B=invB;

   /* Init hash table.
      We take special care of hashing only unique RGB triplets.
      This precaution is unnecessary for Doom, Heretic, Hexen
      and Strife but Doom alpha O.2, 0.4 and 0.5 have a PLAYPAL
      that contains many duplicates that would fill the hash
      table with useless data. -- AYM 1999-09-18 */
   {
     struct PIXEL *unique = Malloc (NCOLOURS * sizeof *unique);
     for (i = 0; i < NCOLOURS; i++)
       unique[i] = COLpal[i];
     qsort (unique, NCOLOURS, sizeof *unique, pixel_cmp);
     COLhash=(UInt8  *)Malloc(HashSz);
     Memset(COLhash,COLinvisib,HashSz); /*clear hash table*/
     for (i = 0; i< NCOLOURS; i++)
     { if ((UInt8)i!=COLinvisib
	 && (i == 0 || pixel_cmp (unique + i, unique + i - 1) != 0))
	     COLputColHash (i, unique[i].R, unique[i].G, unique[i].B);
     }
     Free (unique);
   }
}

void COLfree(void)
{ if(COLok!=TRUE) Bug("COLok");
  COLok=FALSE;
  Free(COLpal);
  Free(COLhash);
  if (COLpalAlt != NULL)
    Free (COLpalAlt);
}

 UInt8 COLinvisible(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLinvisib;
}

struct PIXEL  *COLdoomPalet(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLpal;
}

UInt8 COLindex( UInt8 R, UInt8 G, UInt8 B, UInt8 index)
{  Int16 i;
   if(COLok!=TRUE) Bug("COLok");
   /*check for invisible color*/
   if(R==COLinv.R && G==COLinv.G && B==COLinv.B)
      return COLinvisib;
   /*check for DOOM palette*/
   i= ((Int16)index)&0xFF;
   if(R==COLpal[i].R)
	 if(G==COLpal[i].G)
	   if(B==COLpal[i].B)
		 return index;
   /*else, check hash palette*/
   i=(Int16)COLgetIndexHash(R,G,B);
   return  (UInt8)i;
}

#endif /*QUANTHASH*/



/*
 *	Alternate palette (TITLEPAL)
 */
static char *titlepal_data  = NULL;
static size_t titlepal_size = 0;

void COLinitAlt (char *_titlepal_data, Int32 _titlepal_size)
{
  titlepal_data = _titlepal_data;
  titlepal_size = _titlepal_size;
}

struct PIXEL *COLaltPalet(void)
{
  if (COLpalAlt != NULL)
    return COLpalAlt;

  /* What follows is done only once : */
  if (titlepal_data == NULL)
  {
    int n;

    Warning ("TITLEPAL not found. Using PLAYPAL instead.");
    COLpalAlt = (struct PIXEL *) Malloc (NCOLOURS * sizeof *COLpalAlt);
    for (n = 0; n < NCOLOURS; n++)
      COLpalAlt[n] = COLpal[n];
  }
  else
  {
    struct PIXEL *p;
    struct PIXEL *pmax;
    const unsigned char *d    = (const unsigned char *) titlepal_data;
    const unsigned char *dmax = d + titlepal_size;

    if (titlepal_size < 3 * NCOLOURS)
      Warning ("TITLEPAL too short (%ld). Filling with black.",
	  (long) titlepal_size);
    COLpalAlt = (struct PIXEL *) Malloc (NCOLOURS * sizeof *COLpalAlt);
    /* Copy the contents of TITLEPAL into COLpalAlt */
    for (p = COLpalAlt, pmax = p + NCOLOURS; p < pmax; p++)
    {
      p->R = d < dmax ? *d++ : 0;
      p->G = d < dmax ? *d++ : 0;
      p->B = d < dmax ? *d++ : 0;
    }
    Free (titlepal_data);
    titlepal_data = NULL;  /* Paranoia */
  }

  return COLpalAlt;
}


#ifdef QUANTSLOW
/*unused!*//*************** COL module: quantisation ************/
           /*
           ** implemented as the most stupid  color quantisation
           ** ever to be seen on this sector of the galaxy
           **
           */
/*unused!*/
/*unused!*/
/*unused!*/static struct PIXEL  *COLpal;
/*unused!*/static struct PIXEL COLinv;
/*unused!*/static  UInt8 COLinvisib;
/*unused!*/static Bool COLok=FALSE;
/*unused!*/
/*unused!*/
/*unused!*/
/*unused!*/void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,UInt8  *Colors, Int16 Colsz)
/*unused!*/{  Int16 i;
/*unused!*/   UInt8 r,g,b;
/*unused!*/
/*unused!*/   if(COLok!=FALSE) Bug("COLok");
/*unused!*/   if(Colsz< 256*sizeof(struct PIXEL)) Bug("Color entry too small");
/*unused!*/   COLok=TRUE;
/*unused!*/   COLpal= (struct PIXEL  *)Malloc(256*sizeof(struct PIXEL));
              /*
              ** possible bug: the color corresponding to the
              ** CYAN is assumed to be the second one with 0 0 0
              ** this is because CYAN is supressed.
              */
/*unused!*/   for(i=0;i<256;i++)
/*unused!*/   { r=Colors[i*3+0];
/*unused!*/     g=Colors[i*3+1];
/*unused!*/     b=Colors[i*3+2];
/*unused!*/     COLpal[i].R=r;
/*unused!*/     COLpal[i].G=g;
/*unused!*/     COLpal[i].B=b;
/*unused!*/     if(r==0)if(g==0)if(b==0)
/*unused!*/       COLinvisib=( UInt8)(i&0xFF);
/*unused!*/   }
/*unused!*/   if(COLinvisib!=(UInt8)0xF7)Warning("Strange PLAYPAL invisible color");
/*unused!*/   if(COLinvisib==0)ProgError("PLAYPAL is not correct");
	      /*
	      ** correction to doom palette
	      */
/*unused!*/   i=((Int16)COLinvisib)&0xFF;
/*unused!*/   COLinv.R=COLpal[i].R=invR;
/*unused!*/   COLinv.G=COLpal[i].G=invG;
/*unused!*/   COLinv.B=COLpal[i].B=invB;
/*unused!*/}
/*unused!*/ UInt8 COLinvisible(void)
/*unused!*/{ if(COLok!=TRUE) Bug("COLok");
/*unused!*/  return COLinvisib;
/*unused!*/}
/*unused!*/
/*unused!*/struct PIXEL  *COLdoomPalet(void)
/*unused!*/{ if(COLok!=TRUE) Bug("COLok");
/*unused!*/  return COLpal;
/*unused!*/}
/*unused!*/
/*unused!*/
/*unused!*/
/*unused!*/Int16 COLdiff( UInt8 r, UInt8 g, UInt8 b, UInt8 idx);
/*unused!*/UInt8 COLindex( UInt8 R, UInt8 G, UInt8 B,UInt8 index)
/*unused!*/{  Int16 i;
/*unused!*/   Int16 test,min=0x7FFF;
/*unused!*/   UInt8  idx,idxmin;
/*unused!*/   if(COLok!=TRUE) Bug("COLok");
/*unused!*/
/*unused!*/   if(R==COLinv.R)if(G==COLinv.G)if(B==COLinv.B) return COLinvisib;
/*unused!*/   /*check for DOOM palette*/
/*unused!*/   i= ((Int16)index)&0xFF;
/*unused!*/   if(R==COLpal[i].R)
/*unused!*/	if(G==COLpal[i].G)
/*unused!*/		if(B==COLpal[i].B)
/*unused!*/                	return index;
/*unused!*/   /*Best color match: slow*/
/*unused!*/   idx=(UInt8)0;
/*unused!*/   for(i=0;i<256;i++)
/*unused!*/   {  if(idx!=COLinvisib)
/*unused!*/      { test=COLdiff(R,G,B,idx);
/*unused!*/        if(test<min) {min=test;idxmin=idx;}
/*unused!*/      }
/*unused!*/      if(min==0)  break;
/*unused!*/      idx ++;
/*unused!*/   }
/*unused!*/   return idxmin;
/*unused!*/}
/*unused!*/void COLfree(void)
/*unused!*/{ if(COLok!=TRUE) Bug("COLok");
/*unused!*/  COLok=FALSE;
/*unused!*/  Free(COLpal);
/*unused!*/}
/*unused!*/
/*unused!*/
/*unused!*/ Int16 COLdiff( UInt8 r,  UInt8 g,  UInt8 b, UInt8  idx)
/*unused!*/{  Int16 d;
/*unused!*/   Int16 e=0;
/*unused!*/   Int16 index = (Int16)(idx&0xFF);
/*unused!*/   d= ((Int16)r) - ((Int16) COLpal[index].R);
/*unused!*/   e+= (d>0)? d:-d;
/*unused!*/   d= ((Int16)g) - ((Int16) COLpal[index].G);
/*unused!*/   e+= (d>0)? d:-d;
/*unused!*/   d= ((Int16)b) - ((Int16) COLpal[index].B);
/*unused!*/   e+= (d>0)? d:-d;
/*unused!*/   return e;
/*unused!*/}
#endif /*QUANTSLOW*/

#endif /*DeuTex*/
