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


/*use old GIF encoder, new one is doing errors*/
#define NEWGIFE 0
/*use old GIF decoder, new one is down*/
#define NEWGIFD 0
/************ Important defines *************
#define DeuTex  for DOS .EXE  Unix Linux OS/2
#define DeuSF   for DOS .EXE  Unix Linux OS/2
*********************************************/

#if defined (__alpha)        /*__ALPHA__ for Alpha processor?*/
/*long = int64 on a 64bit processor*/
typedef char           Int8;
typedef short          Int16;
typedef int            Int32;
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned int   UInt32;
#else
/*long = Int32 on a 32 bit processor*/
typedef char           Int8;
typedef short          Int16;
typedef long           Int32;
typedef unsigned char  UInt8;
typedef unsigned short UInt16;
typedef unsigned long  UInt32;
#endif

#if defined DeuTex
#if defined DeuSF
/*DeuTex and DeuSF are mutualy exclusive*/
#error You cant compile DeuTex and DeuSF at the same time
#else /*compiling DeuTex*/
#define DEUTEXNAME   "DeuTex"
#define COMMANDNAME  "deutex"
#endif
#else /*compiling DeuSF*/
#if defined DeuSF
#define DEUTEXNAME   "DeuSF"
#define COMMANDNAME  "deusf"
#else /*one of DeuTex or DeuSF must be defined*/
#error Please define one of DeuTex or DeuSF (with -DDeuTex or -DDeuSF)
#endif
#endif

/* DeuTex version */
extern const char deutex_version[];

/* Try to guess the target OS and set DT_OS accordingly */
#if defined __MSDOS__	/* Borland C and DJGPP define __MSDOS__ */\
 || defined MSDOS	/* Microsoft C defines MSDOS */
#  define DT_OS 'd'
#elif defined __OS2__	/* IBM C Set++ and Borland C define __OS2__ ? */\
  || defined OS2	/* Microsoft C defines OS2 (I think) */
#  define DT_OS 'o'
#else			/* None of the above. Assume Unix. */
#  define DT_OS 'u'
#endif

/* Try to guess the compiler and set DT_CC accordingly */
#if defined __BORLANDC__
#  define DT_CC 'b'	/* Borland C */
#elif defined __CYGWIN__ || defined __CYGWIN32__
#  define DT_CC 'c'	/* GCC/EGCS with Cygwin */
#elif defined __DJGPP__
#  define DT_CC 'd'	/* DJGPP */
#elif defined __GNUC__
#  define DT_CC 'g'	/* GCC/EGCS */
#elif defined MSDOS || defined _MSC_VER
#  define DT_CC 'm'	/* Microsoft C */
#elif defined __OS2__ && ! defined __BORLANDC__
#  define DT_CC 'i'	/* IBM C Set++ */
#else
#  define DT_CC '?'	/* Unknown compiler */
#endif

/* For real-mode 16-bit compilers, the huge memory model is
   required. */
#if DT_OS == 'd' && DT_CC == 'b'
#  if ! defined __HUGE__
#    error Please compile in  memory model (-mh)
#  endif
#  error Sorry, BC has no way to make pointers huge by default.\
 Try compiling with DJGPP instead.
#elif DT_OS == 'd' && DT_CC == 'm'
#  if ! defined M_I86HM
#    error Please compile in  memory model (-AH)
#  endif
#elif DT_OS == 'o' && DT_CC == 'i'  /* Not sure about this one */
   /* Do nothing */
#endif

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
#define BLEVEL 		(0x01)
#define BLUMP		(0x02)
#define BSOUND		(0x04)
#define BTEXTUR		(0x08)
#define BGRAPHIC	(0x10)
#define BSPRITE		(0x20)
#define BPATCH		(0x40)
#define BFLAT		(0x80)
#define BMUSIC		(0x100)
#define BSNEAP          (0x200)
#define BSNEAT          (0x400)
#define BSNEA           (BSNEAP|BSNEAT)
#define BALL		(0x7FF)

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
#define PGRAPH	(2)
#define	PWEAPN	(4)
#define	PSPRIT	(6)
#define	PPATCH	(8)
#define	PFLAT	(0xA)
#define	PLUMP	(0xC)
#define PSNEAP  (0xd)
#define PSNEAT  (0xe)

typedef Int16 ENTRY;
#define EMASK	(0xFF00)
#define EVOID	(0)
#define ELEVEL	(0x100)
#define EMAP	(0x200)/*Levels (doom1) and Maps(Doom2)*/
#define ELUMP	(0x300)
#define ETEXTUR	(0x400)
#define EPNAME	(0x500)/*textures*/
#define ESOUND	(0x600)
#define 	ESNDPC	(0x601)
#define 	ESNDWAV (0x602)
#define EGRAPHIC (0x700)
#define ESPRITE	(0x800)
#define EPATCH 	(0x900)
#define 	EPATCH1	(0x901)
#define 	EPATCH2	(0x902)
#define 	EPATCH3	(0x903)
#define EFLAT	(0xA00)
#define 	EFLAT1	(0xA01)
#define 	EFLAT2	(0xA02)
#define 	EFLAT3	(0xA03)
#define EMUSIC	(0xB00)
#define EDATA	(0x1000)
#define ESNEA	(0x2000)
#define		ESNEAP   (0x2001)  /* Snea using PLAYPAL */
#define		ESNEAT   (0x2002)  /* Snea using TITLEPAL */
#define EZZZZ	(0x7F00)   /*unidentified entries*/



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
typedef enum { PF_NORMAL, PF_ALPHA,    PF_PR                } picture_format_t;
typedef enum { TF_NORMAL, TF_NAMELESS, TF_NONE, TF_STRIFE11 } texture_format_t;
typedef enum { TL_NORMAL, TL_TEXTURES, TL_NONE              } texture_lump_t;
extern picture_format_t picture_format;
extern texture_format_t input_texture_format;
extern texture_format_t output_texture_format;
extern texture_lump_t   texture_lump;
extern const char *debug_ident;
extern int old_music_ident_method;

