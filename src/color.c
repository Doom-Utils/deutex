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


static struct PIXEL huge *COLpal;
static struct PIXEL COLinv;
static  UInt8 COLinvisib;
static Bool COLok=FALSE;


const int COLsame = 3;
Int16 COLdiff( UInt8 R,  UInt8 G,  UInt8 B, UInt8 idx)
{
   register struct PIXEL huge *pixel = &COLpal[(Int16)(idx&0xFF)];
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
static UInt8 huge *COLhash;              /*hash table*/
/*static Int16 huge *COLnext;*/


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






void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,char huge *Colors, Int16 Colsz)
{  Int16 i;
   const char *name = NULL;
   /*Int16 R,G,B;*/
   if(COLok!=FALSE) Bug("COLok");
   if(Colsz< 256*sizeof(struct PIXEL)) Bug("Color entry too small");
   COLok=TRUE;
   COLpal= (struct PIXEL huge *)Malloc(256*sizeof(struct PIXEL));
   for(i=0;i<256;i++)
   { COLpal[i].R=Colors[i*3+0];
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
   else if (0)  /* FIXME */
   {
     name = "Hexen";
   }
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
	     "colour.\nSome graphics may appear moth-eaten.\n");
   COLinv.R=COLpal[i].R=invR;
   COLinv.G=COLpal[i].G=invG;
   COLinv.B=COLpal[i].B=invB;
   /*
   ** init hash table
   */
   COLhash=(UInt8 huge *)Malloc(HashSz);
   Memset(COLhash,COLinvisib,HashSz); /*clear hash table*/

   for(i=0;i<256;i++)
   { if((UInt8)i!=COLinvisib)
	   COLputColHash(i,COLpal[i].R,COLpal[i].G,COLpal[i].B);
   }
}
void COLfree(void)
{ if(COLok!=TRUE) Bug("COLok");
  COLok=FALSE;
  Free(COLpal);
  Free(COLhash);
}
 UInt8 COLinvisible(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLinvisib;
}

struct PIXEL huge *COLdoomPalet(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLpal;
}
UInt8 COLindex( UInt8 R, UInt8 G, UInt8 B, UInt8 index)
{  Int16 i;
   if(COLok!=TRUE) Bug("COLok");
   /*check for invisible color*/
   if(R==COLinv.R)
	 if(G==COLinv.G)
	   if(B==COLinv.B)
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

















#ifdef QUANTSLOW
/*************** COL module: quantisation ************/
/*
** implemented as the most stupid  color quantisation
** ever to be seen on this sector of the galaxy
**
*/


static struct PIXEL huge *COLpal;
static struct PIXEL COLinv;
static  UInt8 COLinvisib;
static Bool COLok=FALSE;



void COLinit( UInt8 invR, UInt8 invG, UInt8 invB,UInt8 huge *Colors, Int16 Colsz)
{  Int16 i;
   UInt8 r,g,b;

   if(COLok!=FALSE) Bug("COLok");
   if(Colsz< 256*sizeof(struct PIXEL)) Bug("Color entry too small");
   COLok=TRUE;
   COLpal= (struct PIXEL huge *)Malloc(256*sizeof(struct PIXEL));
   /*
   ** possible bug: the color corresponding to the
   ** CYAN is assumed to be the second one with 0 0 0
   ** this is because CYAN is supressed.
   */
   for(i=0;i<256;i++)
   { r=Colors[i*3+0];
     g=Colors[i*3+1];
     b=Colors[i*3+2];
     COLpal[i].R=r;
     COLpal[i].G=g;
     COLpal[i].B=b;
     if(r==0)if(g==0)if(b==0)
       COLinvisib=( UInt8)(i&0xFF);
   }
   if(COLinvisib!=(UInt8)0xF7)Warning("Strange PLAYPAL invisible color");
   if(COLinvisib==0)ProgError("PLAYPAL is not correct");
   /*
   ** correction to doom palette
   */
   i=((Int16)COLinvisib)&0xFF;
   COLinv.R=COLpal[i].R=invR;
   COLinv.G=COLpal[i].G=invG;
   COLinv.B=COLpal[i].B=invB;
}
 UInt8 COLinvisible(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLinvisib;
}

struct PIXEL huge *COLdoomPalet(void)
{ if(COLok!=TRUE) Bug("COLok");
  return COLpal;
}



Int16 COLdiff( UInt8 r, UInt8 g, UInt8 b, UInt8 idx);
UInt8 COLindex( UInt8 R, UInt8 G, UInt8 B,UInt8 index)
{  Int16 i;
   Int16 test,min=0x7FFF;
   UInt8  idx,idxmin;
   if(COLok!=TRUE) Bug("COLok");

   if(R==COLinv.R)if(G==COLinv.G)if(B==COLinv.B) return COLinvisib;
   /*check for DOOM palette*/
   i= ((Int16)index)&0xFF;
   if(R==COLpal[i].R)
	if(G==COLpal[i].G)
		if(B==COLpal[i].B)
                	return index;
   /*Best color match: slow*/
   idx=(UInt8)0;
   for(i=0;i<256;i++)
   {  if(idx!=COLinvisib)
      { test=COLdiff(R,G,B,idx);
        if(test<min) {min=test;idxmin=idx;}
      }
      if(min==0)  break;
      idx ++;
   }
   return idxmin;
}
void COLfree(void)
{ if(COLok!=TRUE) Bug("COLok");
  COLok=FALSE;
  Free(COLpal);
}


 Int16 COLdiff( UInt8 r,  UInt8 g,  UInt8 b, UInt8  idx)
{  Int16 d;
   Int16 e=0;
   Int16 index = (Int16)(idx&0xFF);
   d= ((Int16)r) - ((Int16) COLpal[index].R);
   e+= (d>0)? d:-d;
   d= ((Int16)g) - ((Int16) COLpal[index].G);
   e+= (d>0)? d:-d;
   d= ((Int16)b) - ((Int16) COLpal[index].B);
   e+= (d>0)? d:-d;
   return e;
}
#endif /*QUANTSLOW*/

#endif /*DeuTex*/
