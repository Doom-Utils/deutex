/*
  DeuTex is a sequel to the DOOM editor DEU 5.21 (hence the name)
  Originaly it used lots of code from DEU, and even some DEU modules
  Now it doesn't use anymore DEU modules, but large parts of this
  code are still inspired by the great DEU code, though the style
  is quite different.

  I wrote this code with the intent to limit bug generation and
  propagation to a minimum. That means modules, no global variables,
  a lot of mess in the parameters.
  I deliberately coded very basic functions, not optimised at all.
  Optimisation will come later.
  That means: No hash table, No fast color quantisation...

  If you find that code verbose, you are damn right.
  But I'm quite proud of my coding time/testing time ratio of 5,
  considering that code already works on so many PWADs.

  I would strongly advise *not* to reuse this code, unless you
  like my bugs and take the engagement to treat them well.
  I'm so fond of them now...  -- Olivier Montanuy
*/


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


#define VERBOSE_COMMENT                true
#define AUTO_BUG_INSERT                true

#include "deutex.h"
#include <limits.h>
#include <stdarg.h>
#include <ctype.h>
#include "tools.h"
#include "mkwad.h"
#include "merge.h"
#include "extract.h"
#include "wadio.h"
#include "picture.h"
#include "usedidx.h"


/*
**  global variables for commands
*/
char file[128];        /* general use file name*/

static WADTYPE Type;        /*IWAD type*/
static NTRYB Select;

static const char *DataDir=NULL;
static const char *DoomDir=NULL;
static char MainWAD[128];	/* name of the main wad file */
static char WadInf[128];	/* name of the wadinfo file */
static bool WadInfOk;
int16_t HowMuchJunk;		/* junk to add*/
static IMGTYPE Picture;		/* save as PPM, BMP or GIF ? */
static SNDTYPE Sound;		/* save as WAV, VOC or AU ? */
static bool fullSND;
static bool WSafe;
static bool George;
char trnR,trnG,trnB;
picture_format_t picture_format         = PF_NORMAL;
texture_format_t input_texture_format   = TF_NORMAL;
texture_format_t output_texture_format  = TF_NORMAL;
texture_lump_t   texture_lump           = TL_NORMAL;
rate_policy_t    rate_policy            = RP_WARN;
clobber_t        clobber                = CLOBBER_NO;
const char      *debug_ident            = NULL;
int              old_music_ident_method = 0;
const char      *palette_lump           = "PLAYPAL";

static char anon[1]        = { '\0' };

typedef void (*comfun_t) (int argc, const char *argv[]);
static void opt_widths ();
static int is_prefix (const char *s1, const char *s2);
static void call_opt (comfun_t func, ...);


/*
** commands
*/
void COMhelp (int argc, const char *argv[]);
void COMvers (int argc, const char *argv[]);
void COMmanopt (int argc, const char *argv[]);
void COMformat (int argc, const char *argv[]);
void COMipf (int argc, const char *argv[]);
void COMtf (int argc, const char *argv[]);
void COMitl (int argc, const char *argv[]);

/*
** WinTex comand line replaces
** -out
** -v3
** -doom       doom directory
** -dir        data directory
** -sel        select byte
** -colRrGgBb  red = (R-'A')*16+(r-'A')
*/
void COMwintxn(int argc, const char *argv[])
{  const char *num;
   PrintInit(true);                /*-out*/
   PrintVerbosity(3);                /*-v3*/
   DoomDir=argv[1];                /*doom*/
   DataDir=argv[2];                /*data*/
   MakeFileName(WadInf,DataDir,"","",argv[3],"TXT");
   WadInfOk=true;
   Select|= atoi(argv[4]);        /*select*/
   num = argv[5];                /*colour*/
   if(strlen(num)<6) ProgError("AA24", "Invalid colour \"%.128s\"", num);
   trnR= (((num[0]&0xF)<<4)+(num[1]&0xF))&0xFF;
   trnG= (((num[2]&0xF)<<4)+(num[3]&0xF))&0xFF;
   trnB= (((num[4]&0xF)<<4)+(num[5]&0xF))&0xFF;
   Info("AA26", "Transparent colour is R=%d G=%d B=%d",
    ((int)trnR&0xFF),((int)trnG&0xFF),((int)trnB&0xFF));
   (void)argc;
}

void COMwintxm(int argc, const char *argv[])
{  PrintInit(true);                /*-out*/
   PrintVerbosity(3);                /*-v3*/
   DoomDir=argv[1];                /*doom*/
   DataDir=".";
   Select|= atoi(argv[2]);        /*select*/
   trnR= 0;trnG= 255;trnB= 255;
   (void)argc;
}

void COMwintex(int argc, const char *argv[])
{  PrintInit(true);                /*-out*/
   PrintVerbosity(3);                /*-v3*/
   DoomDir=NULL;
   strncpy(MainWAD,argv[1],128);/*main*/
   DataDir=".";
   Select|= BALL;                /*select*/
   trnR= 0;trnG= 255;trnB= 255;
   (void)argc;
}

void COMverbose(int argc, const char *argv[])
{  PrintVerbosity(argv[0][2]-'0');
   Info("AA10", "Verbosity level is %c",argv[0][2]);
   (void)argc;
}

void COMdoom(int argc, const char *argv[])
{  DoomDir=argv[1];
   Info("AA15", "Main directory: %s",DoomDir);
   (void)argc;
}

void COMdoom02(int argc, const char *argv[])
{  call_opt (COMdoom, anon, argv[1], NULL);
   call_opt (COMipf,  anon, "alpha", NULL);
   call_opt (COMtf,   "tf", "none",  NULL);
   call_opt (COMitl,  anon, "none",  NULL);
   (void)argc;
}

void COMdoom04(int argc, const char *argv[])
{  call_opt (COMdoom, anon, argv[1],    NULL);
   call_opt (COMipf,  anon, "alpha",    NULL);
   call_opt (COMtf,   "tf", "nameless", NULL);
   call_opt (COMitl,  anon, "textures", NULL);
   (void)argc;
}

void COMdoom05(int argc, const char *argv[])
{  call_opt (COMdoom, anon, argv[1],    NULL);
   call_opt (COMipf,  anon, "alpha",    NULL);
   call_opt (COMitl,  anon, "textures", NULL);
   (void)argc;
}

void COMdoompr(int argc, const char *argv[])
{  call_opt (COMdoom, anon, argv[1],  NULL);
   call_opt (COMipf,  anon, "pr",     NULL);
   (void)argc;
}

void COMstrife(int argc, const char *argv[])
{  call_opt (COMdoom, anon, argv[1],    NULL);
   call_opt (COMtf,   "tf", "strife11", NULL);
   (void)argc;
}

void COMmain(int argc, const char *argv[])
{  DoomDir=NULL;
   strncpy(MainWAD,argv[1],128);
   Info("AA16", "Main IWAD file: %s",MainWAD);
   (void)argc;
}

void COMwadir(int argc, const char *argv[])
{ XTRlistDir(MainWAD,((argc<2)? NULL: argv[1]),Select);
}

void COMadd(int argc, const char *argv[])
{ ADDallSpriteFloor(argv[2],MainWAD,argv[1],Select);
  (void)argc;
}

void COMapp(int argc, const char *argv[])
{ ADDappendSpriteFloor(MainWAD,argv[1],Select);
  (void)argc;
}

void COMapps(int argc, const char *argv[])
{ Select= (BALL) & (~BFLAT); /*no flats*/
  ADDappendSpriteFloor(MainWAD,argv[1],Select);
  (void)argc;
}

void COMappf(int argc, const char *argv[])
{ Select= (BALL) & (~BSPRITE); /*no sprites*/
  ADDappendSpriteFloor(MainWAD,argv[1],Select);
  (void)argc;
}

void COMjoin(int argc, const char *argv[])
{ ADDjoinWads(MainWAD,argv[1],argv[2],Select);
  (void)argc;
}

void COMmerge(int argc, const char *argv[])
{ Select     = BALL;
  PSTmergeWAD(MainWAD,argv[1],Select);
  (void)argc;
}

void COMrestor(int argc, const char *argv[])
{ HDRrestoreWAD((argc>=2)? argv[1]:MainWAD);
}


/*
** Selections
*/
void COMsprit(int argc, const char *argv[])
{ Select|=BSPRITE;
  Info("AA62", "Select sprites");
  (void)argc;(void)argv;
}

void COMflat(int argc, const char *argv[])
{ Select|=BFLAT;
  Info("AA63", "Select flats");
  (void)argc;(void)argv;
}

void COMlevel(int argc, const char *argv[])
{ Select|=BLEVEL;
  Info("AA64", "Select levels");
  (void)argc;(void)argv;
}

void COMlump(int argc, const char *argv[])
{ Select|=BLUMP;
  Info("AA65", "Select lumps");
  (void)argc;(void)argv;
}

void COMtextur(int argc, const char *argv[])
{ Select|=BTEXTUR;
  Info("AA66", "Select textures");
  (void)argc;(void)argv;
}

void COMpatch(int argc, const char *argv[])
{ Select|=BPATCH;
  Info("AA67", "Select patches");
  (void)argc;(void)argv;
}

void COMsound(int argc, const char *argv[])
{ Select|=BSOUND;
  Info("AA68", "Select sounds");
  (void)argc;(void)argv;
}

void COMmusic(int argc, const char *argv[])
{ Select|=BMUSIC;
  Info("AA69", "Select musics");
  (void)argc;(void)argv;
}

void COMgraphic(int argc, const char *argv[])
{ Select|=BGRAPHIC;
  Info("AA70", "Select graphics");
  (void)argc;(void)argv;
}

void COMsneas(int argc, const char *argv[])
{ Select|=BSNEA;
  Info("AA71", "Select sneas");
  (void)argc;(void)argv;
}

void COMsneaps(int argc, const char *argv[])
{ Select|=BSNEAP;
  Info("AA72", "Select sneaps");
  (void)argc;(void)argv;
}

void COMsneats(int argc, const char *argv[])
{ Select|=BSNEAT;
  Info("AA73", "Select sneats");
  (void)argc;(void)argv;
}

void COMscripts(int argc, const char *argv[])
{ Select|=BSCRIPT;
  Info("AA74", "Select scripts");
  (void)argc;(void)argv;
}

void COMgeorge(int argc, const char *argv[])
{ George=true;
  Info("AA32", "Using S_END for sprites");
  (void)argc;(void)argv;
}


void PicDebug(char *file, const char *DataDir, const char *name);


void COMdebug(int argc, const char *argv[])
{
#include "color.h"
  static struct WADINFO iwad;
  int16_t pnm;
  char  *Colors; int32_t Pnamsz=0;
  iwad.ok=0;
  WADRopenR(&iwad,MainWAD);
  pnm=WADRfindEntry(&iwad,palette_lump);
  if(pnm<0)
    ProgError("GD04", "Can't find %s in Main WAD", lump_name (palette_lump));
  Colors=WADRreadEntry(&iwad,pnm,&Pnamsz);
  COLinit(trnR,trnG,trnB,Colors,(int16_t)Pnamsz, iwad.filename, palette_lump);
  Free(Colors);
  WADRclose(&iwad);
  PicDebug(file,DataDir,(argc<2)? "test":argv[1]);
  COLfree();
  (void)argc;(void)argv;
}

void COMdi (int argc, const char *argv[])
{
  Info("ID01", "Debugging identification of entry %s", lump_name (argv[1]));
  debug_ident = argv[1];
  (void)argc;
}

void COMmusid (int argc, const char *argv[])
{
  Info ("AA19", "Using old music identification method");
  old_music_ident_method = 1;
  (void)argc;
  (void)argv;
}

void COMdeu(int argc, const char *argv[])
{ HowMuchJunk=MAXJUNK64;
  Info("AA33",
      "Will add 64 kB of junk at end of wad for DEU 5.21 compatibility");
  (void)argc;(void)argv;
}

void COMdir(int argc, const char *argv[])
{ DataDir=argv[1];
  Info("AA22", "Files will be saved in directory %s",DataDir);
  (void)argc;
}

void COMfullsnd(int argc, const char *argv[])
{ fullSND=true;
  Info("AA46", "Saving sounds beyond declared length");
  (void)argc;(void)argv;
}

void COMrate(int argc, const char *argv[])
{ 
  if (argc >= 2 && ! strcmp (argv[1], "reject"))
    rate_policy = RP_REJECT;
  else if (argc >= 2 && ! strcmp (argv[1], "force"))
    rate_policy = RP_FORCE;
  else if (argc >= 2 && ! strcmp (argv[1], "warn"))
    rate_policy = RP_WARN;
  else if (argc >= 2 && ! strcmp (argv[1], "accept"))
    rate_policy = RP_ACCEPT;
  else
    ProgError ("AA41", "Usage is \"-rate {reject|force|warn|accept}\"");
  Info("AA42", "Sample rate policy is \"%s\"", argv[1]);
  (void)argc; 
}

void COMstroy (int argc, const char *argv[])
{
  WSafe = false;
  Info ("AA28", "Overwrite existing files");
  (void) argc;
  (void) argv;
}

void COMgif (int argc, const char *argv[])
{
  Picture = PICGIF;
  Info ("AA50", "Saving pictures as GIF (.gif)");
  (void) argc;
  (void) argv;
}

void COMbmp (int argc, const char *argv[])
{
  Picture = PICBMP;
  Info ("AA51", "Saving pictures as BMP (.bmp)");
  (void) argc;
  (void) argv;
}

void COMppm (int argc, const char *argv[])
{
  Picture = PICPPM;
  Info ("AA52", "Saving pictures as rawbits PPM (P6, .ppm)");
  (void) argc;
  (void) argv;
}

void COMau (int argc, const char *argv[])
{
  Sound = SNDAU;
  Info ("AA43", "Save sounds as Sun audio (.au)");
  (void) argc;
  (void) argv;
}

void COMwave (int argc, const char *argv[])
{
  Sound = SNDWAV;
  Info ("AA45", "Save sounds as WAVE (.wav)");
  (void) argc;
  (void) argv;
}

void COMvoc (int argc, const char *argv[])
{
  Sound = SNDVOC;
  Info ("AA44", "Save sounds as voc (.voc)");
  (void) argc;
  (void) argv;
}

void COMrgb (int argc, const char *argv[])
{
  trnR = (char) (atoi (argv[1]) & 0xFF);
  trnG = (char) (atoi (argv[2]) & 0xFF);
  trnB = (char) (atoi (argv[3]) & 0xFF);
  Info ("AA21", "Transparent colour is R=%d G=%d B=%d",
    ((int) trnR & 0xFF),
    ((int) trnG & 0xFF),
    ((int) trnB & 0xFF));
  (void) argc;
}

void COMle (int argc, const char *argv[])
{
  set_input_wad_endianness (0);
  set_output_wad_endianness (0);
  (void) argc;
  (void) argv;
}

void COMbe (int argc, const char *argv[])
{
  set_input_wad_endianness (1);
  set_output_wad_endianness (1);
  (void) argc;
  (void) argv;
}

void COMile (int argc, const char *argv[])
{
  set_input_wad_endianness (0);
  (void) argc;
  (void) argv;
}

void COMibe (int argc, const char *argv[])
{
  set_input_wad_endianness (1);
  (void) argc;
  (void) argv;
}

void COMole (int argc, const char *argv[])
{
  set_output_wad_endianness (0);
  (void) argc;
  (void) argv;
}

void COMobe (int argc, const char *argv[])
{
  set_output_wad_endianness (1);
  (void) argc;
  (void) argv;
}

void COMipf (int argc, const char *argv[])
{
  if (argc >= 2 && ! strcmp (argv[1], "alpha"))
    picture_format = PF_ALPHA;
  else if (argc >= 2 && ! strcmp (argv[1], "normal"))
    picture_format = PF_NORMAL;
  else if (argc >= 2 && ! strcmp (argv[1], "pr"))
    picture_format = PF_PR;
  else if (argc >= 2 && ! strcmp (argv[1], "rott"))
    picture_format = PF_ROTT;
  else
    ProgError ("PI01", "Usage is \"-ipf {alpha|normal|pr|rott}\"");
  Info ("PI02", "Input picture format is \"%s\"", argv[1]);
}

void COMtf (int argc, const char *argv[])
{
  int set_in = 0;
  int set_out = 0;

  if (! strcmp (argv[0], "itf"))
    set_in = 1;
  else if (! strcmp (argv[0], "otf"))
    set_out = 1;
  else if (! strcmp (argv[0], "tf"))
  {
    set_in = 1;
    set_out = 1;
  }
  else
    Bug ("AA90", "Invalid argv[0] \"%.32s\"", argv[0]);

  if (argc >= 2 && ! strcmp (argv[1], "nameless"))
  {
    if (set_in)
      input_texture_format = TF_NAMELESS;
    if (set_out)
      output_texture_format = TF_NAMELESS;
  }
  else if (argc >= 2 && ! strcmp (argv[1], "none"))
  {
    if (set_in)
      input_texture_format = TF_NONE;
    if (set_out)
      output_texture_format = TF_NONE;
  }
  else if (argc >= 2 && ! strcmp (argv[1], "normal"))
  {
    if (set_in)
      input_texture_format = TF_NORMAL;
    if (set_out)
      output_texture_format = TF_NORMAL;
  }
  else if (argc >= 2 && ! strcmp (argv[1], "strife11"))
  {
    if (set_in)
      input_texture_format = TF_STRIFE11;
    if (set_out)
      output_texture_format = TF_STRIFE11;
  }
  else
    ProgError ("TX04", "Usage is \"-%.32s {nameless|none|normal|strife11}\"",
	argv[0]);
  if (set_in)
    Info ("TX05", "Input texture format is \"%s\"", argv[1]);
  if (set_out)
    Info ("TX06", "Output texture format is \"%s\"", argv[1]);
}

void COMitl (int argc, const char *argv[])
{
  if (argc >= 2 && ! strcmp (argv[1], "none"))
    texture_lump = TL_NONE;
  else if (argc >= 2 && ! strcmp (argv[1], "textures"))
    texture_lump = TL_TEXTURES;
  else if (argc >= 2 && ! strcmp (argv[1], "normal"))
    texture_lump = TL_NORMAL;
  else
    ProgError ("TX01", "Usage is \"-itl {none|textures|normal}\"");
  Info ("TX02", "Input texture lump is \"%s\"", argv[1]);
}

/*
** Build an IWAD
**
*/
void COMiwad(int argc, const char *argv[])
{ Type=IWAD;
  Info("AA31", "Building an iwad");
  (void)argc;(void)argv;
}
/*
** Main Commands
**
*/

void COMmake(int argc, const char *argv[])
{ const char *wadinf,*wadout;
  if(WadInfOk==false)
  { MakeFileName(WadInf,DataDir,"","","WADINFO","TXT");
  }
  if(argc<=2){wadinf=WadInf;wadout=argv[1];}
  else       {wadinf=argv[1];wadout=argv[2];}
  CMPOmakePWAD(MainWAD,Type,wadout,DataDir,wadinf,Select,trnR,trnG,trnB,George);
  (void)argc;
}

void COMxtra(int argc, const char *argv[])
{ const char *wadinf, *wadin;
  if(WadInfOk==false)
  { MakeFileName(WadInf,DataDir,"","","WADINFO","TXT");
  }
  if(argc<=1){wadin=MainWAD;}else{wadin=argv[1];}
  if(argc<=2){wadinf=WadInf;}else{wadinf=argv[2];}
  XTRextractWAD (MainWAD, DataDir, wadin, wadinf, Picture, Sound, fullSND,
      Select, trnR, trnG, trnB, WSafe, NULL);
}

void COMget(int argc, const char *argv[])
{ XTRgetEntry(MainWAD,DataDir,((argc<3)? MainWAD: argv[2]),argv[1],Picture,Sound,fullSND,trnR,trnG,trnB);
}

void COMpackNorm(int argc, const char *argv[])
{ XTRcompakWAD(DataDir,(argc>1)? argv[1]: MainWAD,(argc>2)? argv[2]:NULL,false);
}

void COMpackGfx(int argc, const char *argv[])
{ XTRcompakWAD(DataDir,(argc>1)? argv[1]: MainWAD,(argc>2)? argv[2]:NULL,true);
}

void COMvoid(int argc, const char *argv[])
{  XTRvoidSpacesInWAD(argv[1]);
   (void)argc;
}

void COMusedtex(int argc, const char *argv[])
{ XTRtextureUsed((argc>1)? argv[1]: MainWAD);
}

void COMusedidx(int argc, const char *argv[])
{ const char *wadinf, *wadin;
  cusage_t *cusage = NULL;
  if(WadInfOk==false)
  { MakeFileName(WadInf,DataDir,"","","WADINFO","TXT");  /* Not used anyway */
  }
  if(argc<=1){wadin=MainWAD;}else{wadin=argv[1];}
  if(argc<=2){wadinf=WadInf;}else{wadinf=argv[2];}
  cusage = Malloc (sizeof *cusage);
  {
    int n;
    for (n = 0; n < NCOLOURS; n++)
    {
      cusage->uses[n] = 0;
      cusage->nlumps[n] = 0;
      cusage->where_first[n][0] = '\0';
    }
  }
  XTRextractWAD (MainWAD, DataDir, wadin, wadinf, Picture, Sound, fullSND,
      Select, trnR, trnG, trnB, WSafe, cusage);
  Free (cusage);
}

void COMcheck(int argc, const char *argv[])
{ XTRstructureTest(MainWAD,argv[1]);
  (void)argc;
}


enum
{
  OC_MASK = 0xc0,
  OC_SEC  = 0x00,
  OC_OPT  = 0x40,
  OC_MOD  = 0x80,
  OC_END  = 0xc0,
};

#define PASS(comtype) ((comtype) & 0x1f)

typedef unsigned char COMTYPE;
#define SEC OC_SEC		/* Section (used by --help/-man)	*/
#define CM0 OC_MOD + 0		/* Modal (no banner, no log, no iwad)	*/
				/* -------- Banner is printed --------	*/
#define CM1 OC_MOD + 1		/* Modal (banner, no log, no iwad)	*/
#define OP1 OC_OPT + 2		/* Opt (-log)				*/
				/* -------- Log file is opened --------	*/
#define OP2 OC_OPT + 3		/* Opt (banner, log)			*/
#define CM3 OC_MOD + 0x04	/* Modal (banner, log, no iwad)		*/
#define CM4 OC_MOD + 0x24	/* Modal (banner, log, iwad required)	*/
#define END OC_END

typedef struct
{ COMTYPE type;
      char argc;
	char *com;
	            comfun_t exec;
				char *use;
							char *help;
} comdef_t;

/* FIXME should be at the top of the file but we need comdef_t */
static const comdef_t *parse_argv (int *argc, const char ***argv, int pass);

static comdef_t Com[]=
{
 {SEC,0,NULL,       NULL,	NULL,			"Modal options not requiring an iwad"},
 {CM1,0,"?",        COMhelp,	NULL,			"same as \1--help\3"},
 {CM1,0,"h",        COMhelp,	NULL,			"same as \1--help\3"},
 {CM1,0,"help",     COMhelp,	NULL,			"same as \1--help\3"},
 {CM1,0,"-help",    COMhelp,	NULL,			"print list of options"},
 {CM0,0,"man",      COMmanopt,	NULL,			"print list of options in troff -man format"},
 {CM0,0,"syntax",   COMformat,	NULL,			"print the syntax of wad creation directives"},
 {CM3,1,"unused",   COMvoid,	"<in.wad>",		"find unused spaces in a wad"},
 {CM0,0,"-version", COMvers,	NULL,			"print version number and exit successfully"},

 {SEC,0,NULL, NULL, NULL, "Modal options requiring an iwad"},
 {CM4,2,"add",      COMadd,	"<in.wad> <out.wad>",	"copy sp & fl of iwad and \2in.wad\3 to \2out.wad\3"},
 {CM4,1,"af",       COMappf,	"<flats.wad>",		"append all floors/ceilings to the wad"},
 {CM4,1,"append",   COMapp,	"<io.wad>",		"add sprites & flats of iwad to \2io.wad\3"},
 {CM4,1,"as",       COMapps,	"<sprite.wad>",		"append all sprites to the wad"},
 {CM4,1,"build",    COMmake,	"[<in.txt>] <out.wad>",	"make a pwad"},
 {CM4,1,"check",    COMcheck,	"<in.wad>",		"check the textures"},
 {CM4,1,"create",   COMmake,	"[<in.txt>] <out.wad>",	"same as \1-build\3"},
 {CM4,0,"debug",    COMdebug,	"[<file>]",		"debug colour conversion"},
 {CM4,0,"extract",  COMxtra,	"[<in.wad> [<out.txt>]]","same as \1-xtract\3"},
 {CM4,1,"get",      COMget,	"<entry> [<in.wad>]",	"get a wad entry from main wad or \2in.wad\3"},
 {CM4,2,"join",     COMjoin,	"<incomplete.wad> <in.wad>","append sprites & flats of Doom to a pwad"},
 {CM4,1,"make",     COMmake,	"[<in.txt>] <out.wad>",	"same as \1-build\3"},
 {CM4,1,"merge",    COMmerge,	"<in.wad>",		"merge doom.wad and a pwad"},
 {CM4,1,"pkgfx",    COMpackGfx,	"[<in.wad> [<out.txt>]]","detect identical graphics"},
 {CM4,1,"pknormal", COMpackNorm,"[<in.wad> [<out.txt>]]","detect identical normal"},
 {CM4,0,"restore",  COMrestor,	NULL,			"restore doom.wad and the pwad"},
 {CM4,1,"test",     COMcheck,	"<in.wad>",		"same as \1-check\3"},
 {CM4,0,"usedidx",  COMusedidx,	"[<in.wad>]",		"colour index usage statistics"},
 {CM4,0,"usedtex",  COMusedtex,	"[<in.wad>]",		"list textures used in all levels"},
 {CM4,0,"wadir",    COMwadir,	"[<in.wad>]",		"list and identify entries in a wad"},
 {CM4,0,"xtract",   COMxtra,	"[<in.wad> [<out.txt>]]","extract some/all entries from a wad"},

 {SEC,0,NULL,       NULL,	NULL,		"General options"},
 {OP2,0,"overwrite",COMstroy,	NULL,		"overwrite all"},
 {OP2,1,"dir",      COMdir,	"<dir>",	"extraction directory (default \1.\3)"},

 {SEC,0,NULL,       NULL,	NULL,		"Iwad"},
 {OP2,1,"doom",     COMdoom,	"<dir>",	"path to Doom iwad"},
 {OP2,1,"doom2",    COMdoom,	"<dir>",	"path to Doom II iwad"},
 {OP2,1,"doom02",   COMdoom02,	"<dir>",	"path to Doom alpha 0.2 iwad"},
 {OP2,1,"doom04",   COMdoom04,	"<dir>",	"path to Doom alpha 0.4 iwad"},
 {OP2,1,"doom05",   COMdoom05,	"<dir>",	"path to Doom alpha 0.5 iwad"},
 {OP2,1,"doompr",   COMdoompr,	"<dir>",	"path to Doom PR pre-beta iwad"},
 {OP2,1,"heretic",  COMdoom,	"<dir>",	"path to Heretic iwad"},
 {OP2,1,"hexen",    COMdoom,	"<dir>",	"path to Hexen iwad"},
 {OP2,1,"strife",   COMstrife,	"<dir>",	"path to Strife iwad"},
 {OP2,1,"strife10", COMdoom,	"<dir>",	"path to Strife 1.0 iwad"},

 {SEC,0,NULL,       NULL,	NULL,		"Wad options"},
 {OP2,0,"be",       COMbe,	NULL,		"assume all wads are big endian (default LE)"},
 {OP2,0,"deu",      COMdeu,	NULL,		"add 64k of junk for DEU 5.21 compatibility"},
 {OP2,0,"george",   COMgeorge,	NULL,		"same as \1-s_end\3"},
 {OP2,0,"ibe",      COMibe,	NULL,		"input wads are big endian (default LE)"},
 {OP2,0,"ile",      COMile,	NULL,		"input wads are little endian (default)"},
 {OP2,1,"ipf",      COMipf,	"<code>",	"picture format (\1alpha\3, *\1normal\3, \1pr\3, \1rott\3)"},
 {OP2,1,"itf",      COMtf,	"<code>",	"input texture format (\1nameless\3, \1none\3, *\1normal\3, \1strife11\3)"},
 {OP2,1,"itl",      COMitl,	"<code>",	"texture lump (\1none\3, *\1normal\3, \1textures\3)"},
 {OP2,0,"iwad",     COMiwad,	NULL,		"compose iwad, not pwad"},
 {OP2,0,"le",       COMle,	NULL,		"assume all wads are little endian (default)"},
 {OP2,0,"musid",    COMmusid,	NULL,		"use old music identification method"},
 {OP2,0,"obe",      COMobe,	NULL,		"create big endian wads (default LE)"},
 {OP2,0,"ole",      COMole,	NULL,		"create little endian wads (default)"},
 {OP2,1,"otf",      COMtf,	"<code>",	"output texture format (\1nameless\3, \1none\3, *\1normal\3, \1strife11\3)"},
 /*by request from George Hamlin*/
 {OP2,0,"s_end",    COMgeorge,	NULL,		"use \1S_END\3 for sprites, not \1SS_END\3"},
 {OP2,1,"tf",       COMtf,	"<code>",	"texture format (\1nameless\3, \1none\3, *\1normal\3, \1strife11\3)"},

 {SEC,0,NULL,       NULL,	NULL,		"Lump selection"},
 {OP2,0,"flats",    COMflat,	NULL,		"select flats"},
 {OP2,0,"graphics", COMgraphic,	NULL,		"select graphics"},
 {OP2,0,"levels",   COMlevel,	NULL,		"select levels"},
 {OP2,0,"lumps",    COMlump,	NULL,		"select lumps"},
 {OP2,0,"musics",   COMmusic,	NULL,		"select musics"},
 {OP2,0,"patches",  COMpatch,	NULL,		"select patches"},
 {OP2,0,"scripts",  COMscripts,	NULL,		"select Strife scripts"},
 {OP2,0,"sneas",    COMsneas,	NULL,		"select sneas (sneaps and sneats)"},
 {OP2,0,"sneaps",   COMsneaps,	NULL,		"select sneaps"},
 {OP2,0,"sneats",   COMsneats,	NULL,		"select sneats"},
 {OP2,0,"sounds",   COMsound,	NULL,		"select sounds"},
 {OP2,0,"sprites",  COMsprit,	NULL,		"select sprites"},
 {OP2,0,"textures", COMtextur,	NULL,		"select textures"},

 {SEC,0,NULL,       NULL,	NULL,		"Graphics"},
 {OP2,0,"bmp",      COMbmp,	NULL,		"save pictures as BMP (\1.bmp\3)"},
 {OP2,0,"gif",      COMgif,	NULL,		"save pictures as GIF (\1.gif\3)"},
 {OP2,0,"ppm",      COMppm,	NULL,		"save pictures as rawbits PPM (P6, \1.ppm\3)"},
 {OP2,3,"rgb",      COMrgb,	"<r> <g> <b>",	"specify the transparent colour (default 0 47 47)"},

 {SEC,0,NULL,       NULL,	NULL,		"Sound"},
 {OP2,0,"au",       COMau,	NULL,		"save sounds as Sun audio (\1.au\3)"},
 {OP2,0,"fullsnd",  COMfullsnd,	NULL,		"save sounds beyond declared length"},
 {OP2,1,"rate",     COMrate,	"<code>",	"policy for != 11025 Hz (\1reject\3, \1force\3, *\1warn\3, \1accept\3)"},
 {OP2,0,"voc",      COMvoc,	NULL,		"save sounds as voc (\1.voc\3)"},
 {OP2,0,"wav",      COMwave,	NULL,		"save sounds as WAVE (\1.wav\3)"},

 {SEC,0,NULL,       NULL,	NULL,		"Reporting"},
 {OP2,1,"di",       COMdi,	"<name>",	"debug identification of entry"},
 {OP2,0,"v0",       COMverbose,	NULL,		"set verbosity level to 0"},
 {OP2,0,"v1",       COMverbose,	NULL,		"set verbosity level to 1"},
 {OP2,0,"v2",       COMverbose,	NULL,		"set verbosity level to 2 (default)"},
 {OP2,0,"v3",       COMverbose,	NULL,		"set verbosity level to 3"},
 {OP2,0,"v4",       COMverbose,	NULL,		"set verbosity level to 4"},
 {OP2,0,"v5",       COMverbose,	NULL,		"set verbosity level to 5"},

 {SEC,0,NULL,       NULL,	NULL,		"WinTex-related options"},
 {OP2,5,"win",      COMwintxn,	"<doom> <data> <info> <select> <colour>", "WinTex shortcut"},
 {OP2,2,"wim",      COMwintxm,	"<doom> <select>",	"WinTex shortcut"},
 {OP2,1,"wtx",      COMwintex,	"<iwad>",		"WinTex shortcut"},

 {END,0,"",         COMhelp,	NULL,		""}
};

int main (int argc, char *argv_non_const[])
{
   /* Create argv[] identical to argv_[] but of type (const char **). We
      do this to avoid the warnings about initialising a (const char **)
      with a (char **). */
   const char **argv = malloc (argc * sizeof *argv);
   if (argv == NULL)
     ProgError ("MM69", "Out of memory (%d)", argc);
   {
     size_t n;

     for (n = 0; n < argc; n++)
       argv[n] = argv_non_const[n];
   }

   /* Do a first pass through argv to process the options where you
      don't want the banners (-version, -man) */
   {
     int c = argc - 1;
     const char **v = argv + 1;
     const comdef_t *d = parse_argv (&c, &v, 0);
     if (d != NULL)
     {
       d->exec (c, v);
       exit (0);
     }
   }

   /* Sanity checks */
   check_types ();

   /*
   ** default parameters
   */
   WadInfOk=false;
   George=false;
   Picture    = PICPNG;
   Sound      = SNDWAV;
   trnR=0;trnG=47;trnB=47;
   fullSND    = true;
   WSafe      = true;
   HowMuchJunk= 0;
   Select     = 0;
   Type       = PWAD;

   ProgErrorCancel();/*no error handler defined*/

   /*setbuf(stdout,(char *)NULL);*/
   setvbuf(stdout,NULL,_IOLBF,BUFSIZ);
   /*
   ** print on screen or file?
   */
   PrintInit(false);
   PrintVerbosity(2);

   /* Do a second pass through argv to catch options like --help that
      shouldn't cause the creation of a log file. */
   {
     int c = argc - 1;
     const char **v = argv + 1;
     const comdef_t *d = parse_argv (&c, &v, 1);
     if (d != NULL)
     {
       d->exec (c, v);
       exit (0);
     }
   }

   /* At this point, we have either (1) no modal option or (2) a modal
      option which requires writing to log. Make a third pass through
      argv to find out. */
   {
     int c = argc - 1;
     const char **v = argv + 1;
     const comdef_t *d = parse_argv (&c, &v, 4);
     if (d == NULL)
       ProgError ("AA96", "No command given");

     /* We now know we need to create a log. Do a pass through argv to
        catch -log. */
     {
       int c = argc - 1;
       const char **v = argv + 1;
       parse_argv (&c, &v, 2);
     }

     /* Default iwad directory */
     DataDir = ".";
     DoomDir = getenv ("DOOMWADDIR");
     if (DoomDir == NULL)
       DoomDir = ".";
     else
       Phase ("AA17", "Doom directory is %.128s", DoomDir);

     /* Honour the non-modal options */
     {
       int c = argc - 1;
       const char **v = argv + 1;
       parse_argv (&c, &v, 3);
     }

     /* If the modal option requires an iwad, find it. */
     if (d->type & 0x20)
     {
       static const char *wads[] =
       {
	 "doom",		/* Doom, Ultimate Doom, Doom alpha */
	 "doom2",		/* Doom II */
	 "plutonia",		/* Final Doom */
	 "tnt",			/* Final Doom */
	 "freedoom1",		/* Freedoom: Phase 1 */
	 "freedoom2",		/* Freedoom: Phase 2 */
	 "freedm",		/* FreeDM */
	 "heretic",		/* Heretic */
	 "hexen",		/* Hexen */
	 "strife1",		/* Strife */
	 "doompres",		/* Doom Press Release pre-beta */
	 "doom1",		/* Doom shareware */
	 "heretic1",		/* Heretic demo */
	 "strife0",		/* Strife demo */
	 NULL
       };
       int gotit = 0;
       const char **w;
       for (w = wads; *w != NULL; w++)
       {
	 if (MakeFileName (MainWAD, ".", "", "", *w, "wad") == true)
	 {
	   gotit = 1;
	   break;
	 }
       }
       if (! gotit)
       {
	 for (w = wads; *w != NULL; w++)
	 {
	   if (MakeFileName (MainWAD, DoomDir, "", "", *w, "wad") == true)
	   {
	     gotit = 1;
	     break;
	   }
	 }
       }
       if (! gotit)
       {
	 for (w = wads; *w != NULL; w++)
	 {
	   if (MakeFileName (MainWAD, "/usr/share/games/doom", "", "", *w, "wad") == true)
	   {
	     gotit = 1;
	     break;
	   }
	 }
       }
       if (! gotit)
	  ProgError ("AA18",
	      "Can't find any of doom.wad, doom2.wad, doompres.wad,"
	      " heretic.wad, hexen.wad, strife1.wad");
#if 0
       if(    MakeFileName(MainWAD,DoomDir,"","","doompres","wad")!=true
	   && MakeFileName(MainWAD,DoomDir,"","","doom",    "wad")!=true
	   && MakeFileName(MainWAD,DoomDir,"","","doom2",   "wad")!=true
	   && MakeFileName(MainWAD,DoomDir,"","","heretic", "wad")!=true
	   && MakeFileName(MainWAD,DoomDir,"","","hexen",   "wad")!=true
	   && MakeFileName(MainWAD,DoomDir,"","","strife1", "wad")!=true
	   && MakeFileName(MainWAD,".",    "","","doompres","wad")!=true
	   && MakeFileName(MainWAD,".",    "","","doom",    "wad")!=true
	   && MakeFileName(MainWAD,".",    "","","doom2",   "wad")!=true
	   && MakeFileName(MainWAD,".",    "","","heretic", "wad")!=true
	   && MakeFileName(MainWAD,".",    "","","hexen",   "wad")!=true
	   && MakeFileName(MainWAD,".",    "","","strife1", "wad")!=true)
	 ;
#endif
     }

     if (! (Select & BALL))
       Select = BALL;
     if (Picture == PICGIF)
       Warning ("GX10", "GIF support may go away in the future");
     if (ROTT)
       palette_lump = "PAL";
     d->exec (c, v);
   }

   Info ("AA99", "Normal exit");
   return 0;
}


/*
 *	parse_argv - parse the command line
 *
 *	Only the non-modal options whose group matches the <pass>
 *	argument are executed.
 *
 *	Modal options are never executed. If their group matches <pass>,
 *	the function returns a pointer to their definition in Com[].
 *	Otherwise, the function returns a null pointer.
 *
 *	Upon return, *argc and *argv point one past the last argument
 *	processed.
 */
static const comdef_t *parse_argv (int *argc, const char ***argv, int pass)
{
  /* Parse the command line from left to right. Try to match the each
     argument against the longest possible option defined. */
  while (*argc > 0)
  {
    const comdef_t *d = Com + sizeof Com / sizeof *Com - 1;
    const char *arg = **argv;
    if (*arg != '-')
      ProgError("AA92", "Argument \"%s\": not an option", arg);
    arg++;

    {
      const comdef_t *w;
      for (w = Com; w->type != END; w++)
      {
	int r;

	if ((w->type & OC_MASK) == OC_SEC)
	  continue;
	r = is_prefix (arg, w->com);
	if (r != 0)
	{
	  if (r > 1)  /* Exact match. */
	  {
	    d = w;
	    goto got_it;
	  }
	  if (d->type != END)  /* Ambiguous partial match. */
	    ProgError ("AA93", "\"-%s\" is ambiguous (-%s, -%s)",
		arg, d->com, w->com);
	  /* Unambiguous partial match. */
	  d = w;
	}
      }
    }
got_it:

    if (*argc - 1 < d->argc)
      ProgError("AA94", "Usage: -%s%s%s",
	  d->com,
	  d->use ? " "    : "",
	  d->use ? d->use : "");

    {
      int class = (d->type & 0xc0);
      int group = (d->type & 0x1f);

      if (class == OC_SEC)
      {
	;  /* Can't happen */
      }
      else if (class == OC_OPT)
      {
	if (group == pass)
	  d->exec (*argc, *argv);
      }
      else if (class == OC_MOD)
      {
	if (group == pass)
	  return d;
	else
	  return NULL;  /* Parsing ALWAYS stops at the first modal option. */
      }
      else if (class == OC_END)
      {
	ProgError("AA95", "Invalid option \"%s\"", **argv);
      }
      else
      {
	Bug ("AA97", "Com #%d: invalid class %02Xh", (int) (d - Com), class);
      }
    }

    *argv += d->argc + 1;
    *argc -= d->argc + 1;
  }

  return NULL;  /* Found no modal option for this pass number */
}


/*
** Print Help
*/
#define TTYCOL 79
#define OPTINDENT 2
#define COLSPACING 2
void COMhelp(int argc, const char *argv[])
{
  const comdef_t *d;
  size_t width1 = 22;
  size_t width2 = 22;
  int section = 0;

  printf ("Help for %s:\n", PACKAGE_NAME);
  opt_widths ();
  for (d = Com; d->type != END; d++)
  {
    /* Do a first pass on all the options for this section. Find out how
       wide the left and right columns need to be. */
    if (d->type == SEC)
    {
      if (section++)
	putchar ('\n');
      printf ("%s:\n", d->help);
      width1 = *((short *) &d->exec) + OPTINDENT;
      width2 = *((short *) &d->use);
      if (width1 + 1 + width2 > TTYCOL)
	width1 = TTYCOL - width2 - COLSPACING;
    }
    /* Now that we know how wide the left column needs to be, print all
       the options in this section. */
    else
    {
      char buf[200];
      size_t l;
      size_t desclen;
      const char *desc;

      sprintf (buf, "%*s-%s", OPTINDENT, "", d->com);
      if (d->use)
      {
	strcat (buf, " ");
	strcat (buf, d->use);
      }

      l = strlen (buf);

      desc = d->help;
      {
	const char *p;

	for (desclen = 0, p = desc; *p != '\0'; p++)
	  if (*p < '\1' || *p > '\3')
	    desclen++;
      }

      if (l > width1 || l + COLSPACING + desclen > TTYCOL)
	printf ("%s\n%*s", buf, (int) width1 + COLSPACING, "");
      else
	printf ("%-*s%*s", (int) width1, buf, COLSPACING, "");
      for (; *desc != '\0'; desc++)
      {
	if (*desc == '\1')
	  ;
	else if (*desc == '\2')
	  ;
	else if (*desc == '\3')
	  ;
	else
	  putchar (*desc);
      }
      putchar ('\n');
    }

  }
  (void)argc;(void)argv;
}

/*
 *	Print version and exit successfully.
 *	All --version does.
 */
void COMvers (int argc, const char *argv[])
{
  (void) argc;
  (void) argv;
  printf ("%s %.32s\n", PACKAGE_NAME, PACKAGE_VERSION);
  exit (0);
}

static char *Format[] =
{ "* Format of PWAD creation directives *",
  "This format is conform to MS-Windows .INI Files.",
  "Sections are named [LEVELS] [LUMPS] [SOUNDS]",
  "[MUSICS] [TEXTURE1] [TEXTURE2] [GRAPHICS]",
  "[SPRITES] [PATCHES] [FLATS] [SNEAPS] [SNEATS]",
  "Entries have format:",
  "{name}= {filename} {offsetX} {offsetY}",
  "A '*' at the end of the definition means that the",
  "entry will be exactly the same as the previous one.",
  NULL
};

void COMformat(int argc, const char *argv[])
{ int i;
  for(i=0;Format[i]!=NULL;i++)
    puts (Format[i]);
  (void)argc;(void)argv;
}


/*
 *	COMmanopt
 *	List options like COMhelp but in troff -man source format.
 */
static char stack[10];

static void spush (char c)
{
  if (strlen (stack) + 1 >= sizeof stack)
    return;
  stack[strlen (stack) + 1] = '\0';
  stack[strlen (stack)] = c;
}


static char spop (void)
{
  char c;
  if (! *stack)
    return '\0';
  c = stack[strlen (stack) - 1];
  stack[strlen (stack) - 1] = '\0';
  return c;
}


static char last_font;
static char troff_stack[10];

static void troff_start (void)
{
  last_font = '\0';
  *troff_stack = '\0';
}


static void troff_end (void)
{
  if (last_font)
    fputs ("\\fP", stdout);
  last_font = '\0';
}


static void troff_push (char font)
{
  if (strlen (troff_stack) + 1 >= sizeof troff_stack)
    return;
  troff_stack[strlen (troff_stack) + 1] = '\0';
  troff_stack[strlen (troff_stack)] = font;
}

 
static void troff_pop (void)
{
  if (! *troff_stack)
    return;
  troff_stack[strlen (troff_stack) - 1] = '\0';
}


static char troff_tos (void)
{
  if (! *troff_stack)
    return '\0';
  return troff_stack[strlen (troff_stack) - 1];
}


static void troff_putc (char c/*, char font*/)
{
  if (troff_tos () && troff_tos () != last_font)
  {
    if (last_font)
      fputs ("\\fP", stdout);
    fprintf (stdout, "\\f%c", troff_tos ());
    last_font = troff_tos ();
  }
  fputc (c, stdout);
}


static void troff_puts (const char *s/*, char font*/)
{
  if (troff_tos () && troff_tos () != last_font)
  {
    if (last_font)
      fputs ("\\fP", stdout);
    fprintf (stdout, "\\f%c", troff_tos ());
    last_font = troff_tos ();
  }
  fputs (s, stdout);
}


/* Number of columns guaranteed when troff formats a man page in a
   80-column width TTY. This is the minimum (Solaris 10). AIX, Digital
   Unix, HP-UX and Irix all use more columns. */
#define MANCOLS 60
#define COLSPACING_MAN 2
void COMmanopt(int argc, const char *argv[])
{ const comdef_t *d;
  size_t width1 = 0;
  size_t width2 = 0;

  opt_widths ();
  for (d = Com; d->type != END; d++)
  {
    if (d->type == SEC)
    {
      fprintf (stdout, ".SS %s\n", d->help);
      width1 = d->argc + COLSPACING_MAN;
      width2 = *((short *) &d->com) + 1;  /* + 1 because of the full stop */
    }
    else
    {
      const char *p = d->use;
      *stack = '\0';
      troff_start ();
      {
	char buf[30];
	if (width1 < 16 && width1 + width2 <= MANCOLS)
	  sprintf (buf, ".TP %lu\n", (unsigned long) width1);
	else
	  strcpy (buf, ".TP\n");
	troff_puts (buf);
      }

      /* Print the option name in bold */
      troff_push ('B');
      troff_puts ("\\-");
      troff_puts (d->com);
      troff_pop ();
      if (p)
	troff_putc (' ');

      /* Print the arguments. What's between < > is in italic,
	 [ ] { | } are in normal roman, the rest is in bold. */
      if (p)
	while (*p)			/* Print one arg. for each iteration */
	{
	  if (isspace (*p))
	    troff_putc (' ');
	  while (isspace (*p))
	    p++;
	  troff_push ('B');
	  for (; *p && *p != ' '; p++)
	  {
	    if (*p == '[' || *p == '{')
	    {
	      spush (*p);
	      troff_push ('R');
	      troff_putc (*p);
	      troff_pop ();
	    }
	    else if (*p == '<')
	    {
	      spush (*p);
	      troff_push ('I');
	    }
	    else if (*p == ']' || *p == '}')
	    {
	      spop ();
	      troff_push ('R');
	      troff_putc (*p);
	      troff_pop ();
	    }
	    else if (*p == '>')
	    {
	      spop ();
	      troff_pop ();
	    }
	    else if (*p == '|')
	    {
	      troff_push ('R');
	      troff_putc ('|');
	      troff_pop ();
	    }
	    else
	      troff_putc (*p);
	  }
	  troff_pop ();
	}
      troff_end ();
      troff_putc ('\n');

      /* Print the description */
      {
	const unsigned char *p = (const unsigned char *) d->help;

	if (islower (*p))
	{
	  troff_putc (toupper (*p));
	  p++;
	}
	for (; *p != '\0'; p++)
	{
	  if (*p == '\1')
	    fputs ("\\fB", stdout);
	  else if (*p == '\2')
	    fputs ("\\fI", stdout);
	  else if (*p == '\3')
	    fputs ("\\fP", stdout);
	  else if (*p == '-')
	    fputs ("\\-", stdout);
	  else
	    fputc (*p, stdout);
	}
	fputs (".\n", stdout);
      }
    }
  }
  (void)argc;(void)argv;
}


/*
 *	opt_widths - make a pass through Com and compute widths per section
 */
static void opt_widths ()
{
  comdef_t *d;
  comdef_t *current_section = NULL;
  size_t width1t = 0;
  size_t width1r = 0;
  size_t width2t = 0;
  size_t width2r = 0;

  for (d = Com; ; d++)
  {
    if (d->type == SEC || d->type == END)
    {
      /* Seen all the entries of a section. Set
	 - argc = maximum roff width of the first column,
	 - com  = maximum roff width of the second column,
	 - exec = maximum text width of the first column,
	 - use  = maximum text width of second column. */
      if (current_section != NULL)
      {
	current_section->argc = (char) width1r;
	if (current_section->argc != width1r)
	  current_section->argc = CHAR_MAX;  /* Can't happen */

	*((short *) &current_section->com) = (short) width2r;
	if (*((short *) &current_section->com) != width2r)
	  *((short *) &current_section->com) = SHRT_MAX;  /* Can't happen */

	*((short *) &current_section->exec) = (short) width1t;
	if (*((short *) &current_section->exec) != width1t)
	  *((short *) &current_section->exec) = SHRT_MAX;  /* Can't happen */

	*((short *) &current_section->use) = (short) width2t;
	if (*((short *) &current_section->use) != width2t)
	  *((short *) &current_section->use) = SHRT_MAX;  /* Can't happen */
      }
    }

    if (d->type == END)
      break;

    if (d->type == SEC)
    {
      current_section = d;
      width1r = 0;
      width1t = 0;
      width2r = 0;
      width2t = 0;
      continue;
    }

    {					/* Width of column 1 (synopsis) */
      size_t wr = 1 + strlen (d->com);
      size_t wt = wr;
      if (d->use != NULL)
      {
	const char *u;

	wr++;
	wt++;
	for (u = d->use; *u != '\0'; u++)
	{
	  if (*u != '<' && *u != '>')
	    wr++;
	  wt++;
	}
      }
      if (wr > width1r)
	width1r = wr;
      if (wt > width1t)
	width1t = wt;
    }

    {					/* Width of column 2 (description) */
      const char *desc;
      size_t wr = 0;
      size_t wt = 0;

      for (desc = d->help; *desc != '\0'; desc++)
      {
	if (*desc < '\1' || *desc > '\3')
	  wr++;
	if (*desc < '\1' || *desc > '\3')
	  wt++;
      }
      if (wr > width2r)
	width2r = wr;
      if (wt > width2t)
	width2t = wt;
    }
  }
}


/*
 *	is_prefix - tell whether a string an initial prefix of another
 *
 *	Return
 *	   0 if s1 is not a prefix of s2
 *	   1 if s1 is a prefix of s2
 *	  >1 if s1 is equal to s2
 */
static int is_prefix (const char *s1, const char *s2)
{
  for (;; s1++, s2++)
  {
    if (*s1 == '\0')
      return (*s2 == '\0') ? 2 : 1;
    if (*s2 != *s1)
      return 0;
  }
}


/*
 *	call_opt
 *	Equivalent to having the same option on the command line
 */
static void call_opt (comfun_t func, ...)
{
   int argc;
   const char *argv[10];
   va_list args;

   va_start (args, func);
   for (argc = 0; argc < sizeof argv / sizeof *argv; argc++)
   {
      argv[argc] = va_arg (args, const char *);
      if (argv[argc] == NULL)
      {
	 argc++;
	 break;
      }
   }
   func (argc, argv);
}
