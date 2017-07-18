/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2017 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.


  SPDX-License-Identifier: GPL-2.0+
*/


#include "deutex.h"
#include <ctype.h>
#include "tools.h"
#include "endianm.h"
#include "mkwad.h"
#include "picture.h"
#include "texture.h"
#include "ident.h"

/*
** This file contains all the routines to identify DOOM specific
** entries and structures. Fear the bugs!
*/


/* Functions that are going to call IDENTsetType() or
   IDENTdirSet() put their name here before. Thus, with -di,
   IDENTsetType() and IDENTdirSet() can print the name of the
   function that actually did the identification--their caller. */
static const char *ident_func = NULL;


/****************IDENT module ***********************/
/* identify ExMx or MAPxx entries
** which begin a DOOM level.
** returns -1 if not correct
*/

/* Doom, Doom II, Heretic, Hexen and Strife */
static const struct
{
  const char *name;
  char mandatory;		/* If non-zero, warn if lump is missing */
  char level_format;		/* 'n' : denotes Doom/Heretic/Hexen/Strife
				   'a' : denotes Doom alpha
				   '?' : can belong to either */
  const char *flags;
} Part[] =
{
  { "name",     1, '?', NULL   },
  { "BEHAVIOR", 0, 'n', "n11"  },	/* Hexen only */
  { "BLOCKMAP", 1, 'n', "n10"  },
  { "FLATNAME", 1, 'a', "a1"   },	/* Doom alpha 0.4 and 0.5 only */
  { "LINEDEFS", 1, 'n', "n2"   },
  { "LINES",    1, 'a', "a3"   },	/* Doom alpha 0.4 and 0.5 only */
  { "NODES",    1, 'n', "n7"   },
  { "POINTS",   1, 'a', "a2"   },	/* Doom alpha 0.4 and 0.5 only */
  { "REJECT",   1, 'n', "n9"   },	/* Not in Doom PR */
  { "SECTORS",  1, '?', "a4n8" },
  { "SEGS",     1, 'n', "n5"   },
  { "SIDEDEFS", 1, 'n', "n3"   },
  { "SSECTORS", 1, 'n', "n6"   },
  { "THINGS",   1, '?', "a5n1" },
  { "VERTEXES", 1, 'n', "n4"   },
};

static int IDENTlevelPartMax (void)
{
  return sizeof Part / sizeof *Part - 1;
}

int IDENTlevelPart (const char *name)
{
  int n;
  for (n = 1; n < sizeof Part / sizeof *Part; n++)
  {
    if (strncmp (Part[n].name, name, 8) == 0)
      return n;
  }
  return -1;
}

int16_t IDENTlevel(const char *buffer)
{
  if (buffer[0] == 'E'
      && buffer[1] >= '1' && buffer[1] <= '9'
      && buffer[2] == 'M'
      && buffer[3] >= '1' && buffer[3] <= '9')
  {
    /* ExMy */
    if (buffer[4] == '\0')
      return ((buffer[1] & 0x0f) << 4) + (buffer[3] & 0x0f);

    /* ExMyz -- Doom alpha */
    if (buffer[4] >= '0' && buffer[4] <= '9'
	&& buffer[5] == '\0')
    {
      int r = 100 * (buffer[1] - '0') + 10 * (buffer[3] - '0') + buffer[4]-'0';
      if (r & EMASK)
	return -1;  /* Overflow. E2M55 is the limit. */
      return r;
    }
  }

  /* MAPxy */
  if (buffer[0] == 'M'
      && buffer[1]=='A'
      && buffer[2]=='P'
      && buffer[3]>='0' && buffer[3]<='9'
      && buffer[4]>='0' && buffer[4]<='9')
    return (int16_t)((buffer[3]&0xF)*10)+(buffer[4]&0xF);

  return -1;
}

/*
** calculate default insertion point
*/
int16_t IDENTinsrX(PICTYPE type,int16_t insrX,int16_t szx)
{
  if(insrX!=INVALIDINT)
    if(insrX > -4096)
      if(insrX < 4096)
	return insrX;
  /* default insertion point */
  switch(type)
  {
    case PPATCH:       /*mid, lower-5 ????*/
      return (int16_t)(szx/2);
    case PSPRIT:      /*mid, lower-5*/
      return (int16_t)(szx/2);
    case PWEAPN:      /*absolute, in 320*200*/
      return (int16_t)(-(320-szx)/2);   /* -160+X??*/
    case PFLAT:       /*no insertion point*/
    case PLUMP:       /*no insertion point*/
    case PWALL:       /*no insertion point*/
      return (int16_t)0;
    case PGRAPH:    /*0,0 by default*/
      return (int16_t)0;
    default:
      Bug("FB25", "Idinx (%d)", (int) type);
  }
  return (int16_t)0;
}

int16_t IDENTinsrY(PICTYPE type,int16_t insrY,int16_t szy)
{ 
  if(insrY!=INVALIDINT)
    if(insrY > -4096)
      if(insrY < 4096)
	return insrY;
  /* default insertion point */
  switch(type)
  {
    case PPATCH:       /*mid, lower-5 ????*/
      return (int16_t)(szy-5);
    case PSPRIT:      /*mid, lower-5*/
      return (int16_t)(szy-5);
    case PWEAPN:      /*absolute, in 320*200*/
      return (int16_t)(-(200-szy));
    case PFLAT:       /*no insertion point*/
    case PLUMP:       /*no insertion point*/
    case PWALL:       /*no insertion point*/
      return (int16_t)0;
    case PGRAPH:    /*0,0 by default*/
      return (int16_t)0;
    default:
      Bug("FB35", "Idiny (%d)", (int) type);
  }
  return 0;
}


/*
 *	IDENTgraphic
 *	Look at the contents of lump number <n> from wad <info>
 *	and return the probability that it contained a picture,
 *	from 0 to 100.
 */
int IDENTgraphic(struct WADINFO *info,int16_t n)
{
  int32_t start=info->dir[n].start;
  int32_t size=info->dir[n].size;
  char *buf;
  pic_head_t h;
  int x;
  int bad_order = 0;
  int32_t ofs_prev = 0xdeadbeef;

  /* Slurp the whole lump. */
  buf = Malloc (size);
  WADRseek(info,start);
  WADRreadBytes (info, buf, size);

  /* If parse_pic_header() chokes, it must not be a valid picture */
  if (parse_pic_header (buf, size, &h, NULL))
  {
     Free (buf);
     return 0;
  }

  /* Be even more paranoid than parse_pic_header(): check column offsets */
  bad_order = 0;
  for (x = 0; x < h.width; x++)
  {
    int32_t ofs = 0xdeadbeef;
    
    /* Cut and pasted from picture.c. Bleagh. */
    if (h.colofs_size == 4)
    {
       int32_t o;
       read_i32_le (((const int32_t *) h.colofs) + x, &o);
       ofs = o;
    }
    else if (h.colofs_size == 2)
    {
       /* In principle, the offset is signed. However, considering it
	  unsigned helps extracting patches larger than 32 kB, like
	  W18_1 (alpha) or SKY* (PR). Interestingly, Doom alpha and
	  Doom PR treat the offset as signed, which is why some
	  textures appear with tutti-frutti on the right. -- AYM
	  1999-09-18 */
       uint16_t o;
       read_i16_le (((const int16_t *) h.colofs) + x, (int16_t *) &o);
       ofs = o;
    }
    else
    {
       /* Can't happen */
       Bug ("ID65", "Invalid colofs_size %d", (int) h.colofs_size);
    }

    if (buf + ofs < h.data || ofs >= size)
    {
       Free (buf);
       return 0;
    }

    /* In a picture lump, columns appear in increasing X
       order. This is not mandated but, in practice, I think
       it's always true. If they're not, the lump is somewhat
       suspicious and therefore, this function returns only
       50. This additional checking allows us not to mistake
       Doom alpha 0.4 WORLD1 for a picture. It's really a snea
       but it passes all the other tests of picturehood. */
    if (x > 0)
    {
      int32_t delta_ofs = ofs - ofs_prev;
      if (delta_ofs < 1)
	bad_order++;
    }
    ofs_prev = ofs;
  }

  /*valid...graphic...maybe...*/
  Free (buf);
  if (bad_order)
    return 50;
  else
    return 100;
}


/*
 *	IDENTsnea
 *	Look at the contents of lump number <n> from wad <info>
 *	and return the probability that it contained a snea,
 *	from 0 to 100.
 *	
 *	The snea format was used for certain graphics in Doom
 *	alpha 0.4 and 0.5. It consists in a 2-byte header
 *	followed by an interleaved bitmap. The first byte, W, is
 *	the quarter of the width. The second byte, H is the
 *	height. The bitmap is made of 4xWxH bytes. The first WxH
 *	bytes contain the bitmap for columns 0, 4, 8, etc. The
 *	next WxH bytes contain the bitmap for columns 1, 5, 9,
 *	etc., and so on. No transparency.
 */
int IDENTsnea (struct WADINFO *info, int16_t n)
{
  char width;
  char height;

  if (info->dir[n].size < 2)
    return 0;
  WADRseek (info, info->dir[n].start);
  WADRreadBytes (info, &width, 1);
  WADRreadBytes (info, &height, 1);
  if (info->dir[n].size - 2 != 4l * width * height)
    return 0;
  return 100;
}


/*
** set identity of an entry with known name
** set only the first entry that match this name
*/
static void IDENTdirSet (ENTRY *ids, struct WADINFO *info, const char *name,
  ENTRY ident)
{ int16_t n;
  n=WADRfindEntry(info,name);
  if(n>=0)   /*found it?*/
    if(n<(info->ntry))
      if(ids[n]==EZZZZ)
      {
	if (debug_ident != NULL
	    && ((debug_ident[0] == '*' && debug_ident[1] == '\0')
		|| ! strncmp (debug_ident, name, 8)))
	  Info ("ID90", "Ident: %-8s as %-8.32s by %.32s",
	      lump_name (name), entry_type_name (ident), ident_func);
      ids[n]=ident;
      }
}


/*
 *	IDENTsetType
 *	Set the type of an entry
 */
static void IDENTsetType (ENTRY *ids, struct WADINFO *info, int n,
  ENTRY type)
{
  if (debug_ident != NULL
      && ((debug_ident[0] == '*' && debug_ident[1] == '\0')
	  || ! strncmp (debug_ident, info->dir[n].name, 8)))
    Info ("ID91", "Ident: %-8s as %-8.32s by %.32s",
	lump_name (info->dir[n].name), entry_type_name (type), ident_func);
  ids[n] = type;
}


/*
** identifies sprites from:
**  S_START SS_START S_END SS_END delimiters if exist
**  S_END SS_END delimiter and crawl back
**
** Precond: ids contains EZZZZ for unidentified entries
*/
static void IDENTdirSprites(ENTRY  *ids,struct WADINFO *info,bool Check)
{ int16_t s_end,s_start;
  int16_t n;

  ident_func = "IDENTdirSprites";
  /*
  ** check if there are sprites
  */
  s_end=WADRfindEntry(info,"S_END");
  if(s_end<0) s_end=WADRfindEntry(info,"SS_END");
  if(s_end<0) return;
  IDENTsetType (ids, info, s_end, EVOID);
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
      if(Check==true)
      {  
	if (IDENTgraphic(info,n) == 0)
	  break;
      }
      IDENTsetType (ids, info, n, ESPRITE);
    }
  }
  /*
  ** declare sprites
  */
  else
  { IDENTsetType (ids, info, s_start, EVOID);
    for(n=s_end-1;n>s_start;n--)
    { if(info->dir[n].size>8)
      { IDENTsetType (ids, info, n, ESPRITE);
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
static void IDENTdirFlats(ENTRY  *ids,struct WADINFO *info)
{ int16_t f_end,f_start;
  int16_t n;

  ident_func = "IDENTdirFlats";
  /*
  ** check if there are flats
  */
  f_end=WADRfindEntry(info,"F_END");
  if(f_end<0) f_end=WADRfindEntry(info,"FF_END");
  if(f_end<0) return;
  IDENTsetType (ids, info,f_end, EVOID);

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
      if((info->dir[n].size==0x1000)||(info->dir[n].size==0x2000)
	  ||(info->dir[n].size==0x1040))
      { IDENTsetType (ids, info, n, EFLAT);
      }
    }
  }
  /*
  ** declare flats
  */
  else
  { IDENTsetType (ids, info, f_start, EVOID);
    for(n=f_end-1;n>f_start;n--)
    { if((info->dir[n].size==0x1000)||(info->dir[n].size==0x2000)
	||(info->dir[n].size==0x1040))
      { IDENTsetType (ids, info, n, EFLAT);
      }
    }
  }
}


/*
 *	IDENTdirWalls - identify ROTT walls (WALLSTRT/WALLSTOP)
 *
 *	Precond: ids contains EZZZZ for unidentified entries
 */
static void IDENTdirWalls(ENTRY *ids, struct WADINFO *info)
{
  int16_t w_start, w_end;
  int16_t n;
  const int32_t WALL_SIZE = 4096;

  ident_func = "IDENTdirWalls";
  w_start = WADRfindEntry (info, "WALLSTRT");
  w_end   = WADRfindEntry (info, "WALLSTOP");
  if (w_start < 0)
    Warning ("IW05", "No WALLSTRT");
  if (w_start < 0)
    return;
  if (w_start >= 0 && w_end < 0)
    Warning ("IW06", "WALLSTRT but no WALLSTOP. Guessing where walls stop.");

  IDENTdirSet (ids, info, "WALLSTRT", EVOID);
  IDENTdirSet (ids, info, "WALLSTOP", EVOID);

  for (n = w_start + 1; n > 0; n++)
  {
    if (n >= info->ntry)
      break;
    if (w_end >= 0 && n >= w_end)
      break;
    if (w_end < 0 && info->dir[n].size != WALL_SIZE)
      break;
    if (info->dir[n].size == 0)  /* The iwad has empty walls. Ignore them. */
    {
      IDENTsetType (ids, info, n, EVOID);
      continue;
    }
    if (info->dir[n].size != WALL_SIZE)
      Warning ("IW10", "Wall with size != %ld", (long) WALL_SIZE);
    if (ids[n] != EZZZZ)
      Warning ("IW11", "Wall already identified as %d", (int) ids[n]);
    IDENTsetType (ids, info, n, EWALL);
  }
}

static void IDENTdirPatches(ENTRY  *ids,struct WADINFO *info, char  *Pnam, int32_t Pnamsz,bool Check)
{ int16_t p_end,p_start;
  int16_t n,p;
  char  *Pnames;

  ident_func = "IDENTdirPatches";
  /*
  **  find texture and pname entries
  */
  if (texture_lump == TL_NORMAL)
  {
    IDENTdirSet(ids,info,"TEXTURE1",ETEXTUR+1);
    IDENTdirSet(ids,info,"TEXTURE2",ETEXTUR+2);
  }
  else if (texture_lump == TL_TEXTURES)
  {
    IDENTdirSet(ids,info,"TEXTURES",ETEXTUR+1);
  }
  else if (texture_lump == TL_NONE)
  {
    ;  /* No texture lump. Do nothing */
  }
  else
  {
    Bug ("IP10", "Invalid tl %d", (int) texture_lump);
  }
  IDENTdirSet(ids,info,"PNAMES",EPNAME);
  /*
  ** check if there are flats
  */
  p_end=WADRfindEntry(info,"P_END");
  if(p_end<0) p_end=WADRfindEntry(info,"PP_END");
  if(p_end>=0)
  { IDENTsetType (ids, info, p_end, EVOID);
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
    { IDENTsetType (ids, info, p_start, EVOID);
      for(n=p_end-1;n>p_start;n--)
      { if(info->dir[n].size>8)
	  IDENTsetType (ids, info, n, EPATCH);
      }
    }
  }
  /*
  ** check for lost patches
  **
  */
  if(Check==true)
  { /*checkif PNAMES is redefined*/
    n=WADRfindEntry(info,"PNAMES");
    if(n>=0)
    { Pnames=(char  *)Malloc(info->dir[n].size);
      WADRseek(info,info->dir[n].start);
      WADRreadBytes(info,Pnames,info->dir[n].size);
      PNMinit(Pnames,info->dir[n].size);
      Free(Pnames);
    }
    else  /*init with default DOOM Pnames*/
    { if(Pnam!=NULL&&Pnamsz!=0)
	PNMinit(Pnam,Pnamsz);
    }
    /*check for lost patches*/
    for(n=0;n<info->ntry;n++)
    { if(ids[n]==EZZZZ)
	if(info->dir[n].size>8)
	{  p=PNMindexOfPatch(info->dir[n].name); /*Gcc*/
	   if(p>=0)
	   { if (IDENTgraphic(info,n) != 0)
	       IDENTsetType (ids, info, n, EPATCH);
	   }
	}
    }
    PNMfree();
  }
}


/*
** Ident unreferenced graphics
*/
static void IDENTdirGraphics(ENTRY  *ids,struct WADINFO *info)
{ int16_t n;
  ident_func = "IDENTdirGraphics";
  /*heretic fonts*/
  IDENTdirSet(ids,info,"FONTA_S",ELUMP);
  IDENTdirSet(ids,info,"FONTA_E",ELUMP);
  IDENTdirSet(ids,info,"FONTB_S",ELUMP);
  IDENTdirSet(ids,info,"FONTB_E",ELUMP);
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    { if(info->dir[n].size>8)
      { if(strncmp(info->dir[n].name,"FONT",4)==0)
	{ IDENTsetType (ids, info, n, EGRAPHIC);
	}
	else if(strncmp(info->dir[n].name,"M_",2)==0)
	{ IDENTsetType (ids, info, n, EGRAPHIC);
	}
      }
    }
  }
}

static void IDENTdirGraphics2(ENTRY  *ids,struct WADINFO *info,bool Check)
{  int16_t n;
  ident_func = "IDENTdirGraphics2";
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    { if(info->dir[n].size>8)
      { /* It's not quite clear to me why the following 6 lines
	   are here and not in IDENTdirGraphics(), since these
	   are name-based idents. -- AYM 1999-10-16 */
	if(strncmp(info->dir[n].name,"WI",2)==0)
	{ IDENTsetType (ids, info, n, EGRAPHIC);
	}
	else if(strncmp(info->dir[n].name,"ST",2)==0)
	{ IDENTsetType (ids, info, n, EGRAPHIC);
	}
	else if(Check==true)
	{
	  int is_picture = IDENTgraphic (info, n);
	  int is_snea    = IDENTsnea    (info, n);
	  /* Looks more like a picture */
	  if (is_picture > 0 && is_picture > is_snea)
	    IDENTsetType (ids, info, n, EGRAPHIC);
	  /* Looks more like a snea */
	  else if (is_snea > 0 && is_snea > is_picture)
	  {
	    if (! strncmp (info->dir[n].name, "TITLEPIC", 8))
	      IDENTsetType (ids, info, n, ESNEAT);  /* Snea, TITLEPAL */
	    else
	      IDENTsetType (ids, info, n, ESNEAP);  /* Snea, PLAYPAL */
	  }
	  /* Looks like something that the cat brought in :-) */
	  else
	  {
	    if (is_snea > 0 && is_picture > 0 && is_snea == is_picture)
	      Warning ("IG10", "Ambiguous type for %s (picture or snea ?)",
		  lump_name (info->dir[n].name));
	    IDENTsetType (ids, info, n, ELUMP);
	  }
	}
	else  /* Never used. Too dangerous, if you want my opinion. */
	{
	  IDENTsetType (ids, info, n, EGRAPHIC);
	}
      }
    }
  }
}
	
	
/*
 *	IDENTdirSscripts
 *	Identify Strife scripts (SCRIPTnn). This function does
 *	not make sense for other games than Strife.
 */
static void IDENTdirSscripts(ENTRY  *ids,struct WADINFO *info)
{ int16_t n;
  ident_func = "IDENTdirSscripts";

  for(n=0;n<info->ntry;n++)
  {
    if(ids[n]==EZZZZ)
    { 
      if(memcmp(info->dir[n].name,"SCRIPT",6)==0
	&& isdigit (info->dir[n].name[6])
	&& isdigit (info->dir[n].name[7]))
      {
	IDENTsetType (ids, info, n, ESSCRIPT);
      }
    }
  }
}


/*
** Ident PC sounds
*/
static void IDENTdirPCSounds(ENTRY  *ids,struct WADINFO *info,bool Check)
{ int16_t n;
  ident_func = "IDENTdirPCSounds";
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    { if(info->dir[n].size>4) /*works only for DOOM, not HERETIC*/
	if(strncmp(info->dir[n].name,"DP",2)==0)
	{ if(Check==true)
	  { WADRseek(info,info->dir[n].start);
	    if(WADRreadShort(info)==0x0)
	      IDENTsetType (ids, info, n, ESNDPC);
	  }
	}
    }
  }
}


/*
 *	IDENTdirMusics
 */
static void IDENTdirMusics(ENTRY  *ids,struct WADINFO *info,bool Check)
{ int16_t n;
  ident_func = "IDENTdirMusics";
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    {
      /* Pre-4.4 method. Does not work for Hexen. When sure that
	 the new method does not break merging and adding sprites
	 and flats, delete this block and the -musid option. */
      if (old_music_ident_method)
      {
	if (info->dir[n].size>8
	  && (strncmp(info->dir[n].name,"D_",2) == 0
	   || strncmp(info->dir[n].name,"MUS_",4) == 0))
	{
	  if (Check != true)
	  {
	    IDENTsetType (ids, info, n, EMUSIC);
	  }
	  else
	  {
	    /* Must start with "MUS\x1a" */
	    WADRseek(info,info->dir[n].start);
	    if (WADRreadShort(info)==0x554D
	     && WADRreadShort(info)==0x1A53)
	      IDENTsetType (ids, info, n, EMUSIC);
	  }
	}
      }
      /* New method. Slower but more correct. */
      else
      {
	if (info->dir[n].size >= 4)
	{
	  WADRseek(info,info->dir[n].start);
	  if (WADRreadShort(info)==0x554D
	   && WADRreadShort(info)==0x1A53)
	  {
	    IDENTsetType (ids, info, n, EMUSIC);
	  }
	}
      }
    }
  }
}


/*
** Ident sounds
*/
static void IDENTdirSounds(ENTRY  *ids,struct WADINFO *info, bool Doom)
{
  int16_t n;

  ident_func = "IDENTdirSounds";
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    { if(info->dir[n].size>8)
      {
	/*works only for DOOM, not HERETIC*/
	if(strncmp(info->dir[n].name,"DS",2)==0)
	{ IDENTsetType (ids, info, n, ESNDWAV);
	}
	else if(Doom==false)
	{ WADRseek(info,info->dir[n].start);
	  if(WADRreadShort(info)==0x3)
	    if(WADRreadShort(info)==0x2B11)
	      IDENTsetType (ids, info, n, ESNDWAV);
	}
      }
    }
  }
}


/*
 *	IDENTdirLevels
 *	This function is more complicated than I'd like it to be.
 */
static void IDENTdirLevels (ENTRY *ids, struct WADINFO *info)
{ int16_t n,l;
  char name[8];
  char level_name[8];
  ENTRY level=EVOID;
  /* int wrong_order = 0; */
  const int part_num_max = IDENTlevelPartMax ();
  int in_level = 0;
  int lump_present[20];  /* Really sizeof Parts / sizeof *Parts */
  char level_format = '\0';		/* Initialised to avoid a warning */
  int n0 = 0;				/* Initialised to avoid a warning */
  
  ident_func = "IDENTdirLevels";
  for (n = 0; n < info->ntry; n++)
  {
    Normalise (name,info->dir[n].name);

    if (! in_level)
    {
      if (ids[n] != EZZZZ)
	continue;
      l = IDENTlevel (name);
      if (l >= 0)
      {
	Normalise (level_name, info->dir[n].name);
	level = (*name == 'M') ? EMAP : ELEVEL;
	level |= l;
	n0 = n;
	lump_present[0] = 1;
	{
	  int n;
	  for (n = 1; n <= part_num_max; n++)
	    lump_present[n] = 0;
	}
	/* Don't know whether it's Doom alpha or
	   Doom/Heretic/Hexen/Strife yet. */
	level_format = '?';
	in_level = 1;
      }
    }
    else if (in_level)
    {
      int have_next = 0;
      char next_name[8];
      int l_next = 0;			/* Initialised to avoid a warning */
      int p;
      int p_next = 0;			/* Initialised to avoid a warning */

      p = IDENTlevelPart (name);
      lump_present[p] = 1;
      if (Part[p].level_format != '?')
	level_format = Part[p].level_format;

      /* Was that the last lump of the level ? */
      if (n + 1 < info->ntry && ids[n + 1] == EZZZZ)
      {
	have_next = 1;
        Normalise (next_name, info->dir[n + 1].name);
	l_next = IDENTlevel (next_name);
	p_next = IDENTlevelPart (next_name);
      }
      if (! have_next
	  || l_next >= 0
	  || p_next < 0
	  || (Part[p_next].level_format != '?'
	      && level_format != '?'
	      && Part[p_next].level_format != level_format)
	  || lump_present[p_next])
      {
	in_level = 0;
	{
	  int i;

	  for (i = n0; i <= n; i++)
	    IDENTsetType (ids, info, i, level);
	}
      }
    }

  }
  if (in_level)
    Bug ("IL11", "Reached EOD while in level");
}


/*
** IWAD: we assume all is correct
** if Fast = true then sounds and most graphics are reported as lumps
** (this is for merge. no problem. bad identification only to be feared in PWAD)
*/
ENTRY *IDENTentriesIWAD (struct WADINFO *info,char  *Pnam, int32_t Pnamsz,
  bool Fast)
{ int16_t n;
  bool Doom=false;
  ENTRY  *ids;
  Phase("ID50", "IWAD entry identification...");
  if(info->ok!=true)Bug("ID51", "IdnOeI");
  ids=(ENTRY  *)Malloc((info->ntry)*sizeof(ENTRY));
  if(WADRfindEntry(info,"ENDTEXT")<0)              /*Not Heretic*/
    if(WADRfindEntry(info,"ENDOOM")>=0) Doom=true;
  /*
  ** identify for IWAD
  */
  for(n=0;n<info->ntry;n++)
    ids[n]=EZZZZ;
  IDENTdirSprites(ids,info,false); /*fast*/
  IDENTdirFlats(ids,info);         /*fast*/
  if (ROTT)
    IDENTdirWalls(ids,info);
  IDENTdirLevels(ids,info);        /*fast*/
  IDENTdirMusics(ids,info,false);  /*fast*/
  IDENTdirPCSounds(ids,info,false);/*fast*/
  IDENTdirPatches(ids,info,Pnam,Pnamsz,false); /*fast*/
  IDENTdirGraphics(ids,info);      /*fast*/
  IDENTdirSscripts(ids,info);  /* FIXME Should not be called if not Strife ! */
  if(Fast!=true)
  { IDENTdirSounds(ids,info,Doom);   /*slow!*/
    IDENTdirGraphics2(ids,info,true);/*slow!*/
  }
  /* unidentified entries are considered LUMPs*/
  ident_func = "IDENTentriesIWAD";
  for(n=0;n<info->ntry;n++)
  { 
    if(ids[n]==EZZZZ)
    {
      if(info->dir[n].size>=6)
	IDENTsetType (ids, info, n, ELUMP);
      else
	IDENTsetType (ids, info, n, EDATA);
    }
  }
  /*
  ** check registration
  */
/*
  switch(check)
  { case 1: case 2: break;
	 default: ProgError("XX99", "please register your game");
  }
*/

  /*the end. WADR is still opened*/
  return ids;
}



ENTRY *IDENTentriesPWAD(struct WADINFO *info,char  *Pnam, int32_t Pnamsz)
{ int16_t n;
  ENTRY  *ids;
  Phase("ID10", "PWAD entry identification...");
  if(info->ok!=true)Bug("ID11", "IdnOeP");
  ids=(ENTRY  *)Malloc((info->ntry)*sizeof(ENTRY));
  /*
  ** identify for PWAD
  */
  for(n=0;n<info->ntry;n++)
    ids[n]=EZZZZ;
  IDENTdirSprites(ids,info,true);
  IDENTdirFlats(ids,info);
  if (ROTT)
  {
    IDENTdirWalls(ids,info);
  }
  IDENTdirLevels(ids,info);
  IDENTdirMusics(ids,info,true);
  IDENTdirPCSounds(ids,info,true);
  IDENTdirPatches(ids,info,Pnam,Pnamsz,true);
  IDENTdirGraphics(ids,info);
  IDENTdirSscripts(ids,info);  /* FIXME Should not be called if not Strife ! */
  IDENTdirSounds(ids,info,false);
  IDENTdirGraphics2(ids,info,true);
  ident_func = "IDENTentriesPWAD";
  for(n=0;n<info->ntry;n++)
  { if(ids[n]==EZZZZ)
    {
      if(info->dir[n].size>16)
	IDENTsetType (ids, info, n, ELUMP);
      else
	IDENTsetType (ids, info, n, EDATA);
    }
  }
  /*
  ** unidentified entries are considered LUMPs
  */
  /*the end. WADR is still opened*/
  return ids;
}


/*
 *	entry_type_name
 *	Return human-readable string for numeric entry type
 */
typedef struct
{
  ENTRY type;
  const char *name;
  const char *plural;
  PICTYPE pictype;
} entry_type_def_t;

static const entry_type_def_t entry_type_def[] =
{
  { EVOID,    "label",        "labels",   -1     },
  { ELEVEL,   "level(ExMy)",  "levels",   -1     },
  { EMAP,     "level(MAPxy)", "levels",   -1     },
  { ELUMP,    "lump",         "lumps",    PLUMP  },
  { ETEXTUR,  "texture",      "textures", -1     },
  { EPNAME,   "pname",        "pnames",   -1     },
  { ESOUND,   "sound",        "sounds",   -1     },
  { EGRAPHIC, "graphics",     "graphics", PGRAPH },
  { ESPRITE,  "sprite",       "sprites",  PSPRIT },
  { EPATCH,   "patch",        "patches",  PPATCH },
  { EFLAT,    "flat",         "flats",    PFLAT  },
  { EMUSIC,   "music",        "musics",   -1     },
  { EDATA,    "data",         "datas",    -1     },  /* Counting sheeps */
  { ESNEA,    "snea",         "sneas",    -1     },
  { ESNEAP,   "sneap",        "sneaps",   PSNEAP },
  { ESNEAT,   "sneat",        "sneats",   PSNEAT },
  { ESSCRIPT, "script",       "scripts",  -1     },
  { EWALL,    "wall",         "walls",    PWALL  },
  { 0,        NULL,           NULL,       -1     }
};

static ENTRY last_type = -1;
static const entry_type_def_t *last_def = NULL;

static const entry_type_def_t *get_entry_type_def (ENTRY type)
{
  const entry_type_def_t *p;
  if (type == last_type)
    return last_def;
  for (p = entry_type_def; p->name != NULL; p++)
    if (p->type == type)
    {
      last_type = type;
      last_def  = p;
      return p;
    }
  for (p = entry_type_def; p->name != NULL; p++)
    if (p->type == (type & EMASK))
    {
      last_type = type;
      last_def  = p;
      return p;
    }
  return NULL;
}

const char *entry_type_name (ENTRY type)
{
  const entry_type_def_t *def = get_entry_type_def (type);
  if (def == NULL)
    return "(unknown)";
  else
    return def->name;
}

const char *entry_type_plural (ENTRY type)
{
  const entry_type_def_t *def = get_entry_type_def (type);
  if (def == NULL)
    return "(unknown)";
  else
    return def->plural;
}

const char *entry_type_dir (ENTRY type)
{
  return entry_type_plural (type);
}

const char *entry_type_section (ENTRY type)
{
  return entry_type_plural (type);
}

PICTYPE entry_type_pictype (ENTRY type)
{
  const entry_type_def_t *def = get_entry_type_def (type);
  if (def == NULL)
    return -1;
  else
    return def->pictype;
}

/***************end IDENT module *******************/
