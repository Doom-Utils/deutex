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


/*
** This code should contain all the tricky O/S related
** functions. If you're porting DeuTex/DeuSF, look here!
*/

#include "deutex.h"
#include "tools.h"

/*MSDOS*/
#if DT_OS == 'd'
#  define SEPARATOR "\\"
#  if DT_CC == 'd'		/* DJGPP for DOS */
#    include <malloc.h>
#    include <dos.h>
#    include <dir.h>
#    include <io.h>
#  elif DT_CC == 'b'		/* Borland C for DOS */
#    include <alloc.h>
#    include <dir.h>
#    include <io.h>
#  else				/* Other compiler (MSC ?) for DOS */
#    include <malloc.h>
#    include <direct.h>
#    include <io.h>
#  endif
/*OS/2*/
#elif DT_OS == 'o'
#  define SEPARATOR "\\"
#  include <malloc.h>
#  include <direct.h>
#  include <io.h>
/*UNIX*/
#else
#  define SEPARATOR "/"
#  include <unistd.h>
#  include <malloc.h>
#  include <memory.h>
#endif

#if DT_OS == 'o' && DT_CC == 'i'\
 || DT_OS == 'd' && DT_CC == 'm'
#  include <sys/utime.h>
#else
#  include <utime.h>
#endif

#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>


/*
 *	check_types
 *	Sanity checks on the specified-width types.
 *	Will help catching porting errors.
 */
typedef struct
{
  const char *name;
  size_t mandated_size;
  size_t actual_size;
} type_check_t;

static const type_check_t type_checks[] =
{
  { "Int8",   1, sizeof (Int8)   },
  { "Int16",  2, sizeof (Int16)  },
  { "Int32",  4, sizeof (Int32)  },
  { "UInt8",  1, sizeof (UInt8)  },
  { "UInt16", 2, sizeof (UInt16) },
  { "UInt32", 4, sizeof (UInt32) }
};

void check_types (void)
{
  const type_check_t *t;
  for (t = type_checks; t - type_checks < sizeof type_checks / sizeof *t; t++)
  {
    if (t->actual_size != t->mandated_size)
      ProgError ("Type %s has size %d (should be %d)."
	  " Fix deutex.h and recompile.",
	  t->name, (int) t->actual_size, (int) t->mandated_size);
  }
}


/*
** Resize a file
** returns   0 if okay    -1 if failed.
*/
Int16 Chsize(int handle,Int32 newsize)
{
#if DT_OS == 'd'
#  if DT_CC == 'd'
  return (Int16)ftruncate(handle, newsize);
#  elif DT_CC == 'b' || DT_CC == 'm'
  return (Int16)chsize(handle,newsize);
#  else
#    error Chsize unimplemented
#  endif
#elif DT_OS == 'o'
#  if DT_CC == 'b'
   return (Int16)chsize(handle,newsize);
#  else
   return (Int16)_chsize(handle,newsize);
#  endif
#else
  return (Int16)ftruncate(handle, newsize);
#endif
}

/*
** Delete a file
*/
void Unlink(char *file)
{  remove (file);
}

/*
** Get a file time stamp. (date of last modification)
*/
Int32 GetFileTime(char *path)
{ Int32 time;
  struct stat statbuf;
  stat(path,&statbuf);
  time =statbuf.st_ctime;
  return time;
}
/*
** Set a file time stamp.
*/
void SetFileTime(char *path, Int32 time)
{
  struct utimbuf stime;
  stime.modtime=stime.actime=time;
#if DT_OS == 'o' && DT_CC != 'b'
  _utime(path, &stime);
#else
  utime(path, &stime);
#endif
}
/*
** Copy memory
*/
void Memcpy(void huge *dest,const void huge *src, Int32 n)
{ if(n<0) Bug("MovInf"); /*move inf to zero*/
  if(n==0)return;
#if DT_OS == 'd'
#  if DT_CC == 'd'
  memcpy((char huge *)dest,(char huge *)src,(size_t)n);
#  else
  if(n>0x10000L) Bug("MovSup"); /*DOS limit: not more than 0x10000*/
  _fmemcpy(dest,src,(size_t)n);
#  endif
#elif DT_OS == 'o'
  memcpy((char huge *)dest,(char huge *)src,(size_t)n);
#else
  memcpy((char huge *)dest,(char huge *)src,(size_t)n);
#endif
}
/*
** Set memory
*/
void Memset(void huge *dest,char car, Int32 n)
{ if(n<0) Bug("MStInf"); /*set inf to zero*/
  if(n==0)return;
#if DT_OS == 'd'
#  if DT_CC == 'd'
  memset(dest,car,(size_t)n);
#  else
   if(n>0x10000L) Bug("MStSup"); /*DOS limit: not more than 0x10000*/
  _fmemset(dest,car,(size_t)n);
#  endif
#elif DT_OS == 'o'
  memset(dest,car,(size_t)n);
#else
  memset(dest,car,(size_t)n);
#endif
}
/*
** Allocate memory
*/
/*code derived from DEU*/
#define SIZE_THRESHOLD        0x400L
#define SIZE_OF_BLOCK        0xFFFL
/* actually, this is (size - 1) */
void huge *Malloc( Int32 size)
{
   void huge *ret;
   if(size<1)
   {  Warning("Attempt to allocate %ld bytes",size);
      size=1;
   }
#if DT_OS == 'd' && DT_CC == 'b'
   ret = farmalloc( size);
#else
   if ((size_t) size != size)
      ProgError ("Tried to allocate %ld b but couldn't."
        " Use another compiler.", size);
   ret = malloc((size_t) size);
#endif
   if (ret==NULL)
      ProgError("Out of memory (Needed %ld bytes)", size);
   return ret;
}
/*
** Reallocate memory
*/
void huge *Realloc( void huge *old, Int32 size)
{  void huge *ret;

   if(size<1)
   {  Warning("Attempt to allocate %ld bytes",size);
      size=1;
   }
#if DT_OS == 'd' && DT_CC == 'b'
   ret = farrealloc( old, size);
#else
   if ((size_t) size != size)
      ProgError ("Tried to realloc %ld b but couldn't."
        " Use another compiler.", size);
   ret = realloc( old, (size_t)size);
#endif
   if (ret==NULL)
      ProgError( "Out of memory (Needed %ld bytes)", size);
   return ret;
}
/*
** Free
*/
void Free( void huge *ptr)
{
#if DT_OS == 'd' && DT_CC == 'b'
   farfree( ptr);
#else
   free( ptr );
#endif
}
/*****************************************************/
/*
** Use only lower case file names
*/
void ToLowerCase(char *file)
{ Int16 i;
  for(i=0;(i<128)&&(file[i]!='\0');i++)
         file[i]=tolower((((Int16)file[i])&0xFF));
}
static void NameDir(char file[128], char *path,char *dir,char *sdir)
{
   file[0]='.';
   file[1]='\0';
   if(path!=NULL) if(strlen(path)>0){ strncpy(file,path,80);}
   if(dir!=NULL)  if(strlen(dir)>0)
   { strcat(file,SEPARATOR);strncat(file,dir,12);}
   if(sdir!=NULL) if(strlen(sdir)>0)
   { strcat(file,SEPARATOR);strncat(file,sdir,12);}
   ToLowerCase(file);
}
/*
** Create directory if it does not exists
*/
void MakeDir(char file[128], char *path,char *dir,char *sdir)
{  NameDir(file,path,dir,sdir);
#if DT_OS == 'd'
#  if DT_CC == 'd'
   mkdir(file,0); /*2nd argument not used in DOS*/
#  elif DT_CC == 'b' || DT_CC == 'm'
   mkdir(file);
#  else
#    error MakeDir unimplemented
#  endif
#elif DT_OS == 'o'
#  if DT_CC == 'b'
   mkdir(file);
#  else
   _mkdir(file);
#  endif
#else
   mkdir(file,(mode_t)S_IRWXU); /*read write exec owner*/
#endif
}
/*
** Create a file name, by concatenation
** returns TRUE if file exists FALSE otherwise
*/
Bool MakeFileName(char file[128], char *path,char *dir,char *sdir,char *name,char *extens)
{  FILE *fp;
   char name2[8];  /* AYM 1999-01-13: keep checker happy */
   /* deal with VILE strange name
   ** replace the VILE[ VILE\ VILE]
   ** by          VIL@A VIL@B VIL@C
   */
   Normalise(name2,name);

   /* FIXME AYM 1999-06-09: Not sure whether it is a good thing
      to keep this translation scheme for the Unix version.
      However, removing it would make the DOS version and the
      Unix version incompatible. */
   switch(name2[4])
   { case '[':  name2[4]='$';break;
     case '\\': name2[4]='@';break;
     case ']':  name2[4]='#';break;
   }
   switch(name2[6])
   { case '[':  name2[6]='$';break;
     case '\\': name2[6]='@';break;
     case ']':  name2[6]='#';break;
   }

   NameDir(file,path,dir,sdir);
   /*
   ** file name
   */
   strcat(file,SEPARATOR);
   strncat(file,name2,8);
   strcat(file,".");
   strncat(file,extens,4);
   ToLowerCase(file);
   /*
   ** check if file exists
   */
   fp=fopen(file,FOPEN_RB);
   if(fp!=NULL)
   { fclose(fp);  /* AYM 1999-01-??: fclose() used to be called even if
		     fopen() had returned NULL! It gave beautiful segfaults. */
     return TRUE;
   }
   return FALSE;
}
/*
** Get the root name of a WAD file
*/
void GetNameOfWAD(char name[8],char *path)
{ Int16 n, nam,len;
  len=(Int16)strlen(path);
  /*find end of DOS or Unix path*/
  for(nam=n=0;n<len;n++)
    switch(path[n])
    {
      /* FIXME AYM 1999-06-09: I don't understand what "$" is
	 doing here. */
      /* FIXME AYM 1999-06-09: Is it really a good idea to
         consider "\" a path separator under Unix ? */
      case '\\': case '/': case '$':  nam=n+1;
    }
  /*find root name*/
  /* FIXME AYM 1999-06-09: Do we really have to truncate to 8 ? */
  for(n=0;n<8;n++)
    switch(path[nam+n])
    { case '.': case '\0': case '\n':
      name[n]='\0'; return;
      default:   name[n]=toupper(path[nam+n]); break;
    }
    return;
}

/*****************************************************/



void NoCommandGiven(void)
{  Info("Suggestion: use WinTex 4.x as a WAD editor.\n");
	return;
}


void PrintCopyright(void)
{
  Legal("%s %s Copyright 1994,95 O.Montanuy, Copyright 1999 A.Majorel\n",
      DEUTEXNAME,deutex_version);
  Legal("  Ported to DOS, Unix, OS/2, Linux, SGiX, DEC Alpha.\n");
  Legal("  Thanx to M.Mathews, P.Allansson, C.Rossi, J.Bonfield, U.Munk.\n");
  Legal("  This program is freeware.\n");
  Legal("Type \"%s -help\" to get the list of commands.\n",COMMANDNAME);
  return;
}


/*****************************************************/

/* convert 8 byte string to upper case, 0 padded*/
void Normalise(char dest[8], char *src)  /*strupr*/
{ Int16 n;Bool pad=FALSE; char c='A';
  for(n=0;n<8;n++)
  { c= (pad==TRUE)? '\0': src[n];
	 if(c=='\0')   pad=TRUE;
	 else c=(isprint(c))? toupper(c) : '*';
	 dest[n] = c;}
}







/*
** Output and Error handling
*/
static Bool asFile=FALSE;
static Int16 Verbosity=2;
static FILE *Stdout;  /*command output*/
static FILE *Stderr;  /*errors*/
static FILE *Stdwarn;  /*warningss*/
static FILE *Stdinfo; /*infos*/
void PrintInit(Bool asfile)
{
#if DT_OS == 'o'
   setbuf(stdout,(char *)NULL);
#endif
  /*clear a previous call*/
  PrintExit();
  /* choose */
  if(asfile==TRUE)
  { if((Stdout=fopen("output.txt",FOPEN_WT))==NULL)
        ProgError("Can't open output.txt");
    if((Stderr=fopen("error.txt",FOPEN_WT))==NULL)
    {   Stderr=stderr;
        ProgError("Can't open error.txt");
    }
    Stdinfo=stdout;
    Stdwarn=Stderr;
  }
  else
  { Stdout=stdout;
    Stderr=stderr;
    Stdwarn=stderr;
    Stdinfo=stdout;
  }
  asFile=asfile;
}
void PrintVerbosity(Int16 level)
{ Verbosity=(level<0)? 0: (level>4)? 4:level;
}
void PrintExit(void)
{ if(asFile==TRUE)
  { fclose(Stdout);
    fclose(Stderr);
    /*fclose(Stdinfo);*/
  }
}


void ActionDummy(void)
{ return; }
static void (*Action)(void)=ActionDummy;
void ProgErrorCancel(void)
{ Action = ActionDummy;
}
void ProgErrorAction(void (*action)(void))
{ Action = action;
}
void ProgError( char *errstr, ...)
{
   va_list args;va_start( args, errstr);
   fprintf(Stderr, "\nError: *** ");
   vfprintf(Stderr, errstr, args);
   fprintf(Stderr, " ***\n");
   va_end( args);
   (*Action)();  /* execute error handler*/
   PrintExit();
   exit( -5);
}
void Bug( char *errstr, ...)
{  va_list args;va_start( args, errstr);
   fprintf(Stderr, "\nBug: *** ");
   vfprintf(Stderr, errstr, args);
   fprintf(Stderr, " ***\n");
   fprintf(Stderr, "Please report that bug.\n");
   va_end( args);  /* CloseWadFiles();*/
   PrintExit();
   exit( -10);
}
void Warning( char *str, ...)
{  va_list args;va_start( args, str);
   fprintf(Stdwarn, "Warning: ** ");
   vfprintf(Stdwarn, str, args);
   fprintf(Stdwarn, " **\n");
   va_end( args);
}
void Legal(char *str, ...)
{  va_list args;va_start( args, str);
   vfprintf(stdout, str, args);
   va_end( args);
}
void Output(char *str, ...)
{  va_list args;va_start( args, str);
   vfprintf(Stdout, str, args);
   va_end( args);
}
void Info(char *str, ...)
{  va_list args;va_start( args, str);
   if(Verbosity>=1)
     vfprintf(Stdinfo, str, args);
   va_end( args);
}
void Phase(char *str, ...)
{  va_list args;va_start( args, str);
   if(Verbosity>=2)
     vfprintf(Stdinfo, str, args);
   va_end( args);
}
void Detail(char *str, ...)
{  va_list args;va_start( args, str);
   if(Verbosity>=3)
     vfprintf(Stdinfo, str, args);
   va_end( args);
}
#if 0
Int16 NbP=0;
void Progress(void)
{ NbP++;
  if(NbP&0xF==0) fprintf(Stdinfo,".");
  if(NbP>0x400)
  { NbP=0;
    fprintf(Stdinfo,"\n");
  }
}
void ProgressEnds(void)
{ fprintf(Stdinfo,"\n");
}
#endif







