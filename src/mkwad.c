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

#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#include "tools.h"
#include "mkwad.h"
#include "ident.h"
#include "wadio.h"

/*************Begin WAD module **********************
** Open PWAD file. *pcount=global byte counter
** leaves number of entries and directory pointer NULL
*/


/******************End WAD module ******************/




const int WADR_READ=1;
const int WADR_WRITE=2;
const int WADR_RDWR=3;
const int WADR_PIPO=8;



/************ begin   WAD module ***********/

void WADRopenPipo(struct WADINFO *info,Int32 ntry)
{  /*directory */
   if((info->ok&WADR_RDWR)) Bug("WR80", "WadPpk");
   info->ok=WADR_PIPO;
   if(ntry<=0)             Bug("WR81", "WadPpo");
   info->maxdir=ntry;
   info->dir=(struct WADDIR  *)Malloc((info->maxdir)*sizeof(struct WADDIR));
   info->maxpos=ntry*sizeof(struct WADDIR);
   info->ntry=0;
   info->wposit=info->maxpos;
}

struct WADDIR  *WADRclosePipo(struct WADINFO *info,Int32  *ntry)
{  if((info->ok!=WADR_PIPO)) Bug("WR84", "WadPpc");
   info->ok=FALSE;
   if(info->ntry<0)info->ntry=0;
   info->dir=(struct WADDIR  *)
             Realloc(info->dir,(info->ntry)*sizeof(struct WADDIR));
   *ntry=info->ntry;
   return info->dir;
}

Int32 WADRdirAddPipo(struct WADINFO *info,Int32 start,Int32 size, const char
    *entry)
{ Int16 n;
  if(info->ok!=WADR_PIPO) Bug("WR82", "WadDaP");
  n=(Int16)info->ntry; /*position of new entry*/
  if(n<0) Bug("WR83", "WadDa2");
  if(n<info->maxdir) /*can add to the dir*/
  { info->ntry++; /*new dir size*/
    info->dir[n].size=size;
    info->dir[n].start=start;
    Normalise(info->dir[n].name,entry);
  }
  return n; /* nb entries */
}

void WADRopenR(struct WADINFO *info, const char *wadin)
{  /*directory */
   Int32 ntry,dirpos;
   Int16 n;
   struct WADDIR dir;
   if((info->ok&WADR_RDWR)) Bug("WR01", "%s: WadOpr", fname (wadin));
   info->fd=fopen(wadin,FOPEN_RB);
   if((info->fd)==NULL)
     ProgError("WR03", "%s: %s", fname (wadin), strerror (errno));
   info->filename = Malloc (strlen (wadin) + 1);
   strcpy (info->filename, wadin);
   info->ok=WADR_READ;
   /*signature*/
   switch(WADRreadShort(info))
   { case PWAD: case IWAD: break;
     default:   ProgError("WR05", "%s: invalid wad magic", fname (wadin));
   }
   if(WADRreadShort(info)!=WADMAGIC)
     ProgError("WR07", "%s: read error in header", fname (wadin));
   /*start of directory*/
   ntry  = WADRreadLong(info);
   if(ntry<=0)
     ProgError("WR09", "%s: zero entries", fname (wadin));
   if(ntry>=0x2000)
     ProgError("WR11", "%s: too many entries", fname (wadin));
   info->dirpos= dirpos= WADRreadLong(info);
   if((dirpos<0)||(dirpos>0x10000000L))
     ProgError("WR13", "%s: invalid directory offset %08lX",
	 fname (wadin), (unsigned long) dirpos);
   /*allocate directory*/
   info->maxdir=ntry;
   info->dir=(struct WADDIR  *)Malloc((info->maxdir)*sizeof(struct WADDIR));
   /*read directory, calculate further byte in wad*/
   info->maxpos=dirpos+(ntry*sizeof(struct WADDIR));
   WADRseek(info,dirpos);
   info->ntry=0;
   for(n=0;n<ntry;n++)
   {
     if (wad_read_i32 (info->fd, &dir.start)
	  || wad_read_i32 (info->fd, &dir.size)
	  || wad_read_name (info->fd, dir.name))
       ProgError ("WR15", "%s: read error in directory", fname (wadin));
     WADRdirAddEntry (info, dir.start, dir.size, dir.name);
   }
   if(info->ntry!=ntry)
     Bug("WR17", "WadOrN %ld %ld", (long) ntry, (long) info->ntry);
   info->wposit=info->maxpos;
   Phase("WR19", "Reading WAD %s:\t(%ld entries)", fname (wadin), (long) ntry);
}

static char signature[19 + 32];
void WADRopenW(struct WADINFO *info, const char *wadout,WADTYPE type,
    int verbose)
{
   if (verbose)
     Phase("WW01", "Creating %s %s",
	 (type==IWAD) ? "iwad" : "pwad", fname (wadout));
   if((info->ok&WADR_RDWR)) Bug("WW02", "%s: WadOpW", fname (wadout));

   /*check file*/
   if(clobber==CLOBBER_NO)
   { info->fd = fopen(wadout, FOPEN_RB);
     if(info->fd!=NULL)
       ProgError("WW03", "Won't overwrite existing file %s", fname (wadout));
   }
   /*open file*/
   info->fd = fopen(wadout, FOPEN_WB);
   if(info->fd==NULL)
     ProgError("WW04",  "%s: %s", fname (wadout), strerror (errno));
   info->filename = Malloc (strlen (wadout) + 1);
   strcpy (info->filename, wadout);
   info->ok=WADR_WRITE;
   info->wposit=0;
   info->ntry  =0;
   info->maxdir=MAXPWADDIR;
   info->dir   =(struct WADDIR  *)Malloc((info->maxdir)*sizeof(struct WADDIR));
   WADRwriteShort(info,type);   /* WAD type: PW or IW*/
   WADRwriteShort(info,WADMAGIC); /* WAD type: AD*/
   /* will be fixed when closing the WAD*/
   WADRwriteLong(info,-1);     /* no counter of dir entries */
   WADRwriteLong(info,-1);     /* no dir pointer */
   /* DeuTex notice*/
   sprintf(signature," " PACKAGE_NAME " %.32s %cOJM 94 ", PACKAGE_VERSION, 0xB8);
                     /********----**----**-********/
   WADRwriteBytes(info,signature,strlen(signature));
   WADRalign4(info);
}

/*
** Assumes file already opened write
** if not, open it first
** OPEN READ-WRITE,not APPEND
** because APPEND can't FSEEK to start of file
*/
void WADRopenA(struct WADINFO *info, const char *wadinout)
{  Phase("WW10", "Modifying wad %s", fname (wadinout));
   if((info->ok&WADR_WRITE)) Bug("WW11", "%s: WadOpA", fname (wadinout));
   if(!(info->ok&WADR_READ))
   { WADRopenR(info,wadinout);
   }
   /*reopen for append*/
   fclose(info->fd);
   info->fd = fopen( wadinout, FOPEN_RBP); /*rb+ = read/write binary*/
   if(info->fd==NULL)
     ProgError("WW12", "%s: %s", fname (wadinout), strerror (errno));
   info->filename = Malloc (strlen (wadinout) + 1);
   strcpy (info->filename, wadinout);
   info->ok = WADR_RDWR;
   WADRseek(info,info->wposit);
}



/***************** Directory ***************/
/*
** Add a new entry in the directory
** increase ntry, redim dir
** update maxdir and maxpos
** returns entry ref
*/
Int32 WADRdirAddEntry(struct WADINFO *info,Int32 start,Int32 size, const char
    *entry)
{ Int16 n;
  Int32 sz;
  if(!(info->ok&(WADR_RDWR))) Bug("WW40", "%s: WadDAE", fname (info->filename));
  n=(Int16)info->ntry; /*position of new entry*/
  if(n>=info->maxdir) /*shall we move the dir?*/
  { info->maxdir+=MAXPWADDIR;
    info->dir=(struct WADDIR  *)Realloc((char  *)info->dir,(info->maxdir)*sizeof(struct WADDIR));
  }
  info->ntry++; /*new dir size*/
  info->dir[n].size=size;
  info->dir[n].start=start;
  Normalise(info->dir[n].name,entry);
  sz=start+size;
  if(sz>info->maxpos)
    info->maxpos=sz;
  return n; /* nb entries */
}

/*
** write the directory (names, counts, lengths)
** then update the number of  entries and dir pointer
*/
void WADRwriteDir(struct WADINFO *info, int verbose)
{  Int16 n;
   if(!(info->ok&WADR_WRITE)) Bug("WW20", "%s: WadWD", fname (info->filename));
   WADRalign4(info);      /*align entry on Int32 word*/
   info->dirpos=info->wposit; /*current position*/
   /* write the new WAD directory*/
   for(n=0;n<info->ntry;n++)
   {
     if (wad_write_i32  (info->fd, info->dir[n].start)
	 || wad_write_i32  (info->fd, info->dir[n].size)
	 || wad_write_name (info->fd, info->dir[n].name))
       ProgError ("WW22", "%s: write error in directory",
	   fname (info->filename));
   }
   /* fix up the directory start information */
   WADRsetDirRef(info,info->ntry,info->dirpos);
   n=(Int16)( info->dirpos+(sizeof(struct WADDIR)*info->ntry));
   if(n>info->maxpos) info->maxpos=n;
   if (verbose)
     Phase("WW28", "%s: wad is complete, %ld entries, %ld bytes",
	 fname (info->filename), (long) info->ntry, (long) info->wposit);
}

/***************** Wad structure *******************/

void WADRsetDirRef(struct WADINFO *info,Int32 ntry,Int32 dirpos)
{
#if 0
  struct { Int32 ntry;Int32 dirpos;} Head;
#endif
   if(!(info->ok&WADR_WRITE))Bug("WW24", "%s: WadSDR", fname (info->filename));
#if 0
   Head.ntry=BE_Int32(ntry);
   Head.dirpos=BE_Int32(dirpos);
#endif
   WADRseek(info,4);
   if (wad_write_i32 (info->fd, ntry)
       || wad_write_i32 (info->fd, dirpos))
     ProgError ("WW26", "%s: write error while fixing up the header",
	 fname (info->filename));
#if 0
   if(fwrite(&Head,sizeof(Head),1,info->fd)!=1)
   { Warning("XX99", "That WAD might not be usable anymore!");
     Warning("XX99", "Restore bytes 4 to 11 manually if you can.");
     ProgError("XX99", "Failed writing WAD directory References");
   }
#endif
   WADRseek(info,info->wposit);
   info->ntry=ntry;
   info->dirpos=dirpos;
}

void WADRchsize(struct WADINFO *info,Int32 fsize)
{ if(!(info->ok&WADR_WRITE)) Bug("WW93", "WadcSz");
  if(Chsize(fileno(info->fd),fsize)!=0)
    ProgError("WW95", "%s: %s", fname (info->filename), strerror (errno));
  /*@ AEO while trying the change the size of the wad */
  info->maxpos=fsize;
  info->wposit=fsize;
}

#if 0
Bool WADRchsize2(struct WADINFO *info,Int32 fsize)
{ if(!(info->ok&WADR_WRITE)) Bug("XX99", "WadcSz");
  if(Chsize(fileno(info->fd),fsize)!=0) return FALSE;
  return TRUE;
}
#endif


/****************Read********************/



void WADRseek(struct WADINFO *info,Int32 position)
{
   long ofs;
   if(!(info->ok&WADR_RDWR)) Bug("WR31", "WadSk");
   ofs = ftell (info->fd);
   if(position>info->maxpos) Bug("WR33", "WadSk>");
   if(fseek(info->fd,position,SEEK_SET))
     ProgError("WR35", "%s: Can't seek to %06lXh (%s)",
	   fnameofs (info->filename, ofs),
	   (unsigned long) position,
	   strerror (errno));
}

/*
 *	WADRreadBytes2
 *	Attempt to read up to <nbytes> bytes from wad <info>.
 *	Return the actual number of bytes read.
 */
iolen_t WADRreadBytes2 (struct WADINFO *info, char *buffer, iolen_t nbytes)
{
  size_t attempt = MEMORYCACHE;
  iolen_t bytes_read = 0;

  if (!(info->ok&WADR_READ))
    Bug("WR41", "WadRdB");

  while (nbytes > 0)
  {
    size_t result;
    if (attempt > nbytes)
      attempt = nbytes;
#if 0
    
    {					/* TEST */
    const unsigned char *p;
    printf ("%p", info->fd);
    for (p = (const unsigned char *) info->fd; (FILE*) p < info->fd + 1; p += 4)
      printf (" %02X%02X%02X%02X", p[3], p[2], p[1], p[0]);
    putchar ('\n');
    clearerr (info->fd);  /* DEBUG */
    }
#endif
    result = fread (buffer, 1, attempt, info->fd);
    bytes_read += result;
    if (result == 0)  /* Hit EOF */
      break;
    buffer += result;
    nbytes -= result;
  }

  return bytes_read;
}

/*
 *	WADRreadBytes
 *	Attempt to read <nbytes> bytes from wad <info>. If EOF
 *	is hit too soon, trigger a fatal error. Else, return
 *	<nbytes>.
 */
iolen_t WADRreadBytes (struct WADINFO *info, char *buffer, iolen_t nbytes)
{
  long    ofs    = ftell (info->fd);
  iolen_t result = WADRreadBytes2 (info, buffer, nbytes);
  if (result != nbytes)
    if (ferror (info->fd))
      ProgError ("WR43", "%s: read error (got %lu/%lu bytes)",
	  fnameofs (info->filename, ofs),
	  (unsigned long) result,
	  (unsigned long) nbytes);
    else
      ProgError ("WR45", "%s: unexpected EOF (got %lu/%lu bytes)",
	  fnameofs (info->filename, ofs),
	  (unsigned long) result,
	  (unsigned long) nbytes);

  return nbytes;
}

Int16 WADRreadShort(struct WADINFO *info)
{ Int16 res;
  long ofs = ftell (info->fd);
  if (!(info->ok&WADR_READ)) Bug("WR51", "WadRdS");
  if (wad_read_i16 (info->fd, &res))
    if (ferror (info->fd))
      ProgError ("WR53", "%s: read error", fnameofs (info->filename, ofs));
    else
      ProgError ("WR55", "%s: unexpected EOF", fnameofs (info->filename, ofs));
   return res;
}

Int32 WADRreadLong(struct WADINFO *info)
{ Int32 res;
  long ofs = ftell (info->fd);
  if (!(info->ok&WADR_READ)) Bug("WR61", "WadRdL");
  if (wad_read_i32 (info->fd, &res))
    if (ferror (info->fd))
      ProgError ("WR63", "%s: read error", fnameofs (info->filename, ofs));
    else
      ProgError ("WR65", "%s: unexpected EOF", fnameofs (info->filename, ofs));
  return res;
}

void  WADRclose(struct WADINFO *info)
{  if(!(info->ok&WADR_RDWR)) Bug("WR97", "WadClo");
   info->ok=FALSE;
   Free (info->filename);
   Free (info->dir);
   fclose (info->fd);
}

Int16 WADRfindEntry(struct WADINFO *info, const char *entry)
{ Int16 i;
  static char name[8];
  struct WADDIR  *dir;
  if(!(info->ok&WADR_RDWR)) Bug("WR91", "WadFE");
  for(i=0,dir=info->dir;i<info->ntry;i++,dir+=1)
  { Normalise(name,dir->name);
    if(strncmp(name,entry,8)==0)
        return i;
  }
  return -1;
}

/*
** load data in buffer
*/
char  *WADRreadEntry(struct WADINFO *info,Int16 n,Int32 *psize)
{ char  *buffer;
  Int32 start,size;
  if(!(info->ok&WADR_READ)) Bug("WR71", "WadRE");
  if(n>=(info->ntry))Bug("WR73", "WadRE>");
  start = info->dir[n].start;
  size  = info->dir[n].size;
  buffer=(char  *)Malloc(size);
#if 0
  fclose (info->fd);  /* TEST */
  info->fd = fopen(info->filename,"r+b");  /* TEST */
  if (info->fd == NULL)  /* TEST */
    Warning("XX99", "Ugh!");
  printf ("after %p\n", info->fd);
#endif
  WADRseek(info,start);
  WADRreadBytes(info,buffer,size);
  *psize=size;
  return buffer;
}

/*
 *	WADRreadEntry2
 *	Read at most the *psize first bytes of a lump.
 *	If the lump is shorter than *psize, it's _not_ an error.
 *	Return the actual number of bytes read in *psize.
 */
char *WADRreadEntry2 (struct WADINFO *info, Int16 n, Int32 *psize)
{
  char *buffer;
  long   start;
  iolen_t size;
  iolen_t actual_size;

  if (! (info->ok & WADR_READ))
    Bug ("WR75", "WadRE");
  if (n >= info->ntry)
    Bug ("WR76", "WadRE>");
  start = info->dir[n].start;
  size  = *psize < info->dir[n].size ? *psize : info->dir[n].size;
  buffer = Malloc (size);
  WADRseek (info, start);
  actual_size = WADRreadBytes2 (info, buffer, size);
  if (actual_size < size)
    if (ferror (info->fd))
      ProgError ("WR78", "%s: Lump %s: read error at byte %ld",
	  fnameofs (info->filename, start + actual_size),
	  lump_name (info->dir[n].name),
	  (long) actual_size);
    else
      ProgError ("WR79", "%s: Lump %s: unexpected EOF at byte %ld",
	  fnameofs (info->filename, start + actual_size),
	  lump_name (info->dir[n].name),
	  (long) actual_size);
  *psize = actual_size;
  return buffer;
}

/*
**  copy data from WAD to file
*/
void WADRsaveEntry(struct WADINFO *info,Int16 n, const char *file)
{  Int32 wsize,sz=0;
   char  *buffer;
   Int32 start,size;
   FILE *fd;
   if(!(info->ok&WADR_READ)) Bug("WR86", "WadSE");
   if(n>=(info->ntry))Bug("WR87", "WadSE>");
   start = info->dir[n].start;
   size  = info->dir[n].size;
   fd=fopen(file,FOPEN_WB);
   if (fd==NULL)
     ProgError ("WR88", "%s: Can't open for writing (%s)",
	 fname (file),
	 strerror (errno));
   buffer = (char  *)Malloc( MEMORYCACHE);
   WADRseek(info,start);
   for(wsize=0; wsize<size ;wsize+=sz)
   {  sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE : size-wsize;
      WADRreadBytes(info,buffer,sz);
      if(fwrite(buffer,(size_t)sz,1,fd)!=1)
      { Free(buffer);
        ProgError ("WR89", "%s: write error", fname (file));
      }
   }    /*declare in WAD directory*/
   Free( buffer);
   if (fclose(fd))
     ProgError ("WR90", "%s: %s", fname (file), strerror (errno));
}

/******************** Write ************************/

void WADRsetLong(struct WADINFO *info,Int32 pos,Int32 val)
{
  if(!(info->ok&WADR_WRITE))		Bug("WW56", "WadStL");
  if(pos>(info->maxpos))		Bug("WW57", "WadSL>");
  if(fseek(info->fd, pos, SEEK_SET))
    ProgError ("WW58", "%s: Can't seek to %06lXh (%s)",
	fname (info->filename),
	(unsigned long) pos,
	strerror (errno));
  if(wad_write_i32 (info->fd, val))
    ProgError ("WW59", "%s: write error", fnameofs (info->filename, pos));
}

void WADRsetShort(struct WADINFO *info,Int32 pos,Int16 val)
{
  if(!(info->ok&WADR_WRITE))		Bug("WW51", "WadStS");
  if(pos>(info->maxpos))		Bug("WW52", "WadSS>");
  if(fseek(info->fd, pos, SEEK_SET))
    ProgError ("WW53", "%s: Can't seek to %06lXh (%s)",
	fname (info->filename),
	(unsigned long) pos,
	strerror (errno));
  if(wad_write_i16 (info->fd, val))
    ProgError ("WW54", "%s: write error", fnameofs (info->filename, pos));
}

/*
** internal functions
**
*/
static void WADRcheckWritePos(struct WADINFO *info)
{ if(!(info->ok&WADR_WRITE)) Bug("WW61", "WadCkW");
  if (fseek( info->fd, info->wposit, SEEK_SET))
    ProgError ("WW63", "%s: Can't seek to %06lXh (%s)",
	fnameofs (info->filename, ftell (info->fd)),
	info->wposit,
	strerror (errno));
}

static Int32 WADRwriteBlock(struct WADINFO *info,char  *data,Int32 sz)
{ if(fwrite(data,(size_t)sz,1,info->fd) != 1)
    ProgError("WW65",  "%s: write error", fname (info->filename));
  info->wposit += sz;
  if(info->maxpos<info->wposit)info->maxpos=info->wposit;
  return sz;
}

/*
** align, give position
*/
void WADRalign4(struct WADINFO *info)
{ Int16 remain;
  static char buffer[] ={0,0x24,0x6,0x68};
  WADRcheckWritePos(info);
  remain = (Int16)(info->wposit&0x03);   /*0 to 3*/
  if(remain>0) WADRwriteBytes(info,buffer,4-remain);
}
/*must be equal to ftell*/
Int32 WADRposition(struct WADINFO *info)
{ WADRcheckWritePos(info);
  return info->wposit;
}

/*
** write
*/
Int32 WADRwriteLong(struct WADINFO *info,Int32 val)
{
  WADRcheckWritePos(info);
  if (wad_write_i32 (info->fd, val))
    ProgError ("WW67", "%s: write error", fname (info->filename));
  info->wposit += sizeof val;
  if (info->maxpos < info->wposit)
    info->maxpos = info->wposit;
  return sizeof val;
}

Int32 WADRwriteShort(struct WADINFO *info,Int16 val)
{
  WADRcheckWritePos(info);
  if (wad_write_i16 (info->fd, val))
    ProgError ("WW69", "%s: write error", fname (info->filename));
  info->wposit += sizeof val;
  if (info->maxpos < info->wposit)
    info->maxpos = info->wposit;
  return sizeof val;
}

Int32 WADRwriteBytes(struct WADINFO *info,char  *data,Int32 size)
{ Int32 wsize,sz=0;
  WADRcheckWritePos(info);
  if(size<=0) Bug("WW71", "WadWb<");
  for(wsize=0;wsize<size;)
  { sz=(size-wsize>MEMORYCACHE)? MEMORYCACHE : size-wsize;
    wsize+=WADRwriteBlock(info,&data[wsize],sz);
  }
  return wsize;
}

static Int32 WADRwriteBlock2(struct WADINFO *info,char  *data,Int32 sz)
{ if(fwrite(data,(size_t)sz,1,info->fd) != 1)return -1;
  info->wposit += sz;
  if(info->maxpos<info->wposit)info->maxpos=info->wposit;
  return sz;
}

Int32 WADRwriteBytes2(struct WADINFO *info,char  *data,Int32 size)
{ Int32 wsize,sz=0;
  WADRcheckWritePos(info);
  if(size<=0) Bug("WW73", "WadWb<");
  for(wsize=0;wsize<size;)
  { sz=(size-wsize>MEMORYCACHE)? MEMORYCACHE : size-wsize;
    sz=WADRwriteBlock2(info,&data[wsize],sz);
    if(sz<0) return -1;
    wsize+=sz;
  }
  return wsize;
}

/*
**  copy data from SOURCE WAD to WAD
*/
Int32 WADRwriteWADbytes(struct WADINFO *info,struct WADINFO *src,Int32 start,Int32 size)
{  Int32 wsize,sz=0;
   char  *data;
   data = (char  *)Malloc( MEMORYCACHE);
   WADRseek(src,start);
   WADRcheckWritePos(info);
   for(wsize=0; wsize<size;)
   {  sz= (size-wsize>MEMORYCACHE)? MEMORYCACHE : size-wsize;
      WADRreadBytes(src,data,sz);
      wsize+=WADRwriteBlock(info,data,sz);
   }    /*declare in WAD directory*/
   Free( data);
   return wsize;
}



/*
** copy lump from file into WAD
** returns size
*/
Int32 WADRwriteLump(struct WADINFO *info, const char *file)
{  Int32      size,sz=0;
   FILE      *fd;
   char  *data;
   WADRcheckWritePos(info);
   /*Look for entry in master directory */
   fd=fopen(file,FOPEN_RB);
   if (fd==NULL)
     ProgError("WW75", "%s: %s",
	 fname (file),
	 strerror (errno));
   data = (char  *)Malloc( MEMORYCACHE);
   for(size=0;;)
   { sz = fread(data,1,(size_t)MEMORYCACHE,fd);
     if(sz<=0)break;
     size+=WADRwriteBlock(info,data,sz);
   }
   Free( data);
   fclose(fd);
   return size;
}

Int32 WADRwriteWADentry(struct WADINFO *info,struct WADINFO *src,Int16 n)
{  if(n>(src->ntry)) Bug("WW77", "WadWW>");
   return WADRwriteWADbytes(info,src,src->dir[n].start,src->dir[n].size);
}

/*
** copy level parts
*/
void WADRwriteWADlevelParts(struct WADINFO *info,struct WADINFO *src,Int16 N,
    size_t nlumps)
{ Int32 start,size;
  Int16 n;

  for (n = N + 1; n < src->ntry && n < N + nlumps; n++)
  {
    WADRalign4 (info);
    start = WADRposition (info);
    size = WADRwriteWADentry (info, src, n);
    WADRdirAddEntry (info, start, size, src->dir[n].name);
  }
}

/*
** copy level from WAD
** try to match level name (multi-level)
** if level name not found, then take the first level...
*/
void WADRwriteWADlevel(struct WADINFO *info, const char *file, const char *level)
{ Int16 N,l;
  Int32 pos;
  /*char Level[8];*/
  struct WADINFO src;
  if(IDENTlevel(level)<0)ProgError("WW79", "Bad level name %s",level);
  src.ok=0;
  WADRopenR(&src,file);
  /*search for entry in level*/
  N=WADRfindEntry(&src,level);
  if(N<0) /* no? then search first level*/
  { for(N=0;;N++)
    { l=IDENTlevel(src.dir[N].name);
      if(l>=0)break;
      if(N>=src.ntry) ProgError("WW81", "No level in WAD %s",file);
    }
  }
  /*set level name*/
  WADRalign4(info);
  pos=WADRposition(info); /*BC++ 4.5 bug!*/
  WADRdirAddEntry(info,pos,0L,level);
  /* 9999 is a way of saying "copy all the lumps". The rationale
     is "let's assume the user knows what he/she is doing. If
     he/she wants us to include a 100-lump level, let's do it".

     On the other hand, this stance is in contradiction with using
     WADRfindEntry() (see above). This needs to be fixed.
     
     There are two choices :
     - make this function discriminating and prevent
       experimentation
     - make it dumb but allow one to put multi-level wads in
       levels/.

     My real motivation for doing things the way I did was that I
     didn't want to copy-paste from IDENTdirLevels() into
     WADRwriteWADlevelParts() (these things should be at a single
     place). */
  WADRwriteWADlevelParts(info,&src,N, 9999);
  WADRclose(&src);
}

/*
** replace dir of rwad by dir of newwad
** prepare to write at the end of rwad
** open for append
*/
Int32 WADRprepareAppend(const char *wadres,struct WADINFO *rwad,
     struct WADDIR  *NewDir,Int32 NewNtry,
     Int32 *dirpos,Int32 *ntry, Int32 *size)
{ Int32 ewadstart;
  Int32 rwadsize;
  Int32 time;
  time=GetFileTime(wadres);
  /* append to the Result WAD*/
   WADRopenA(rwad,wadres);
  /*get original size*/
   rwadsize=rwad->maxpos;
   /*last warning*/
   Output("The WAD file %s will be modified, but it can be restored with:\n",wadres);
   Output("%s -res %s\n", PACKAGE, wadres);
   Output("Restoration may fail if you modified the WAD with another tool.\n");
   Output("In case of failure, you can salvage your WAD by:\n");
   /* Assuming wad is little endian... */
   Output("- setting bytes 4-7  to \t%02Xh %02Xh %02Xh %02Xh\n",
       (unsigned short) (rwad->ntry & 0xff),
       (unsigned short) ((rwad->ntry >> 8) & 0xff),
       (unsigned short) ((rwad->ntry >> 16) & 0xff),
       (unsigned short) ((rwad->ntry >> 24) & 0xff));
   Output("- and setting bytes 8-11 to \t%02Xh %02Xh %02Xh %02Xh\n",
       (unsigned short) (rwad->dirpos & 0xff),
       (unsigned short) ((rwad->dirpos >> 8) & 0xff),
       (unsigned short) ((rwad->dirpos >> 16) & 0xff),
       (unsigned short) ((rwad->dirpos >> 24) & 0xff));
   Output("If possible, set the file size to %ld bytes.\n",rwadsize);
   /*align*/
   ewadstart=((rwadsize+0xF)&(~0xFL));
   if((ewadstart|rwadsize)&EXTERNAL) ProgError("WW91", "Too big WADs");
   *dirpos=rwad->dirpos;
   *ntry=rwad->ntry;
   *size=rwadsize;
   /*Change size*/
   WADRchsize(rwad,ewadstart);
   /*Write will start at file end*/
   rwad->maxpos=ewadstart;
   rwad->wposit=ewadstart;
   WADRseek(rwad,ewadstart);
   /*Change to New directory*/
   Free(rwad->dir);
   rwad->dir=NewDir;
   rwad->ntry=NewNtry;
   rwad->maxdir=NewNtry;
   rwad->dirpos = -1;
   return time;
}
