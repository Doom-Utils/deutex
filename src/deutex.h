/*
This file is part of DeuTex.

DeuTex incorporates code derived from DEU 5.21 that was put in the public
domain in 1994 by Raphaël Quinet and Brendon Wyber.

DeuTex is Copyright © 1994-1995 Olivier Montanuy,
          Copyright © 1999-2005 André Majorel.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "config.h"

#define ROTT 0  /* FIXME make it a run-time option */

/*use old GIF encoder, new one is doing errors*/
#define NEWGIFE 0
/*use old GIF decoder, new one is down*/
#define NEWGIFD 0

/* Fixed-size types */
#include <inttypes.h>

typedef int8_t   Int8;
typedef int16_t  Int16;
typedef int32_t  Int32;
typedef uint8_t  UInt8;
typedef uint16_t UInt16;
typedef uint32_t UInt32;


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* fopen() modes */
#define FOPEN_RB	"rb"
#define FOPEN_RBP	"rb+"
#define FOPEN_WB	"wb"
#define FOPEN_RT	"r"
#define FOPEN_WT	"w"
#define FOPEN_AT	"a"
#define FOPEN_AB        "a"

/* Number of bytes to read from or write to a file */
typedef unsigned long iolen_t;

#define MEMORYCACHE  (0x8000L)
/* steps to increase size of WAD directory*/
#define MAXPWADDIR	(128)
/*Add 64000 bytes of pure junk, because of a bug in DEU5.21 */
#define MAXJUNK64	(1000)
/*special value, means int is not valid*/
#define INVALIDINT      (-6666)
/*indicate an extern WAD entry*/
#define EXTERNAL  (0x80000000L)

typedef Int16 Bool;
#define TRUE			1
#define FALSE			0


/* Wad magic numbers */
#define IWADMAGIC     	0x5749  /* little-endian 16-bit int for "IW" */
#define PWADMAGIC       0x5750  /* little-endian 16-bit int for "PW" */
#define WADMAGIC	0x4441  /* little-endian 16-bit int for "AD" */

/*type of WAD files. correspond to 1st half of name*/
typedef Int16 WADTYPE;
#define IWAD (IWADMAGIC)
#define PWAD (PWADMAGIC)

/* Number of entries in the game palette */
#define NCOLOURS 256

/************ deutex.c ************/
/* Entry selection Bits*/
typedef Int16 NTRYB;
#define BLEVEL 		(0x0001)
#define BLUMP		(0x0002)
#define BSOUND		(0x0004)
#define BTEXTUR		(0x0008)
#define BGRAPHIC	(0x0010)
#define BSPRITE		(0x0020)
#define BPATCH		(0x0040)
#define BFLAT		(0x0080)
#define BMUSIC		(0x0100)
#define BSNEAP		(0x0200)
#define BSNEAT		(0x0400)
#define BSCRIPT		(0x0800)
#define BSNEA		(BSNEAP|BSNEAT)
#define BWALL		(0x1000)
#define BALL		(0x7FFF)

typedef Int16 SNDTYPE;
#define SNDNONE		(0)
#define SNDAU		(1)
#define SNDWAV		(2)
#define SNDVOC		(3)
typedef Int16 IMGTYPE;
#define PICNONE		(0)
#define PICBMP		(1)
#define PICGIF		(2)
#define PICPPM		(3)
#define PICTGA		(4)
#define PICWAD		(5)






/****************** mkwad.c ********************/
/*wad directory*/
struct WADDIR           /*same as in doom*/
{ Int32 start;           /*start of entry*/
  Int32 size;            /*size of entry*/
  char name[8];        /*name of entry*/
};

struct WADINFO
{ Int32 ntry;            /*entries in dir*/
  Int32 dirpos;          /*position of dir*/
  struct WADDIR  *dir;   /*directory */
  Int32 maxdir;		/*max nb of entry in dir*/
  Int32 wposit;		/*position for write*/
  Int32 maxpos;		/*farther referenced byte in WAD*/
  FILE *fd;	        /* File pointer */
  char *filename;	/* Pointer on block malloc'd by WADRopen*() and
			   free'd by WADRclose() and containing the name
			   of the file. */
  Bool ok;		/*security ok&1=read ok&2=write*/
};



/********************ident.c********************/
typedef Int16 PICTYPE;
#define PGRAPH	0x02
#define	PWEAPN	0x04
#define	PSPRIT	0x06
#define	PPATCH	0x08
#define	PFLAT	0x0a
#define	PLUMP	0x0c
#define PSNEAP  0x0d
#define PSNEAT  0x0e
#define PWALL   0x10

typedef Int16 ENTRY;
#define EMASK		0xFF00
#define EVOID		0x0000
#define ELEVEL		0x0100
#define EMAP		0x0200  /*Levels (doom1) and Maps(Doom2)*/
#define ELUMP		0x0300
#define ETEXTUR		0x0400
#define EPNAME		0x0500  /*textures*/
#define ESOUND		0x0600
#define   ESNDPC	0x0601
#define   ESNDWAV	0x0602
#define EGRAPHIC	0x0700
#define ESPRITE		0x0800
#define EPATCH		0x0900
#define   EPATCH1	0x0901
#define   EPATCH2	0x0902
#define   EPATCH3	0x0903
#define EFLAT		0x0A00
#define   EFLAT1	0x0A01
#define   EFLAT2	0x0A02
#define   EFLAT3	0x0A03
#define EMUSIC		0x0B00
#define EDATA		0x1000
#define ESNEA		0x2000
#define	  ESNEAP	0x2001  /* Snea using PLAYPAL */
#define	  ESNEAT	0x2002  /* Snea using TITLEPAL */
#define ESSCRIPT	0x3000  /* Strife script (SCRIPTnn) */
#define EWALL		0x4000  /* ROTT wall (WALLSTRT/WALLSTOP) */
#define EZZZZ		0x7F00  /*unidentified entries*/



/******************* lists.c ********************/


/*deutex.c: misc*/
Bool LetsHaveFunBaby(long guesswhat);
/*compose.c: TEXTURE list*/
/*compose.c: TEXTURE insertion...rather, WAD composition*/
void CMPOmakePWAD(const char *doomwad,WADTYPE type, const char *PWADname,
			 const char *DataDir, const char *texin,NTRYB select,
			 char trnR, char trnG, char trnB,Bool George);
/*substit.c: DOOM.EXE string substitution*/
void EXE2list(FILE *out,char *doomexe,Int32 start,Int16 thres);
void EXEsubstit(const char *texin,const char *doomexe,Int32 start,Int16 thres);

void XTRlistDir(const char *doomwad,const char *wadin,NTRYB select);

void XTRvoidSpacesInWAD(const char *wadin);

void XTRcompakWAD(const char *DataDir, const char *wadin, const char
    *texout,Bool ShowIdx);
void XTRstructureTest(const char *doomwad, const char *wadin);
void XTRtextureUsed(const char *wadin);


/*
 *	Types defined elsewhere
 */
struct cusage_s;
typedef struct cusage_s cusage_t;
 

/*
 *	Misc globals, set by command line arguments
 */
typedef enum { PF_NORMAL, PF_ALPHA,    PF_PR,   PF_ROTT     } picture_format_t;
typedef enum { TF_NORMAL, TF_NAMELESS, TF_NONE, TF_STRIFE11 } texture_format_t;
typedef enum { TL_NORMAL, TL_TEXTURES, TL_NONE              } texture_lump_t;
typedef enum { RP_REJECT, RP_FORCE,    RP_WARN, RP_ACCEPT   } rate_policy_t;
typedef enum { CLOBBER_YES, CLOBBER_NO, CLOBBER_ASK         } clobber_t;
extern picture_format_t picture_format;
extern texture_format_t input_texture_format;
extern texture_format_t output_texture_format;
extern texture_lump_t   texture_lump;
extern rate_policy_t    rate_policy;
extern clobber_t        clobber;
extern const char      *debug_ident;
extern int              old_music_ident_method;
extern const char      *palette_lump;
