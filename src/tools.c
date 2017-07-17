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


/*
** This code should contain all the tricky O/S related
** functions. If you're porting DeuTex, look here!
*/

#include "deutex.h"
#include <errno.h>
#include <stdarg.h>
#include "tools.h"

#define SEPARATOR "/"
#include <unistd.h>
#include <memory.h>

#include <utime.h>

#include <time.h>
#include <stdarg.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>


static const char hex_digit[16] = "0123456789ABCDEF";


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
  { "int8_t",   1, sizeof (int8_t)   },
  { "int16_t",  2, sizeof (int16_t)  },
  { "int32_t",  4, sizeof (int32_t)  },
  { "uint8_t",  1, sizeof (uint8_t)  },
  { "uint16_t", 2, sizeof (uint16_t) },
  { "uint32_t", 4, sizeof (uint32_t) }
};

void check_types (void)
{
  const type_check_t *t;
  for (t = type_checks; t - type_checks < sizeof type_checks / sizeof *t; t++)
  {
    /* FIXME Perhaps too strict. Wouldn't "<" suffice ? */
    if (t->actual_size != t->mandated_size)
      ProgError ("AA01", "Type %s has size %d (should be %d);"
	  " fix deutex.h and recompile",
	  t->name, (int) t->actual_size, (int) t->mandated_size);
  }
}


/*
** Resize a file
** returns   0 if okay    -1 if failed.
*/
int16_t Chsize(int handle,int32_t newsize)
{
  return (int16_t)ftruncate(handle, newsize);
}

/*
** Delete a file
*/
void Unlink(const char *file)
{  remove (file);
}

/*
** Get a file time stamp. (date of last modification)
*/
int32_t GetFileTime(const char *path)
{ int32_t time;
  struct stat statbuf;
  stat(path,&statbuf);
  time =statbuf.st_ctime;
  return time;
}
/*
** Set a file time stamp.
*/
void SetFileTime(const char *path, int32_t time)
{
  struct utimbuf stime;
  stime.modtime=stime.actime=time;
  utime(path, &stime);
}
/*
** Copy memory
*/
void Memcpy(void  *dest,const void  *src, long n)
{ if(n<0) Bug("MM21", "MovInf"); /*move inf to zero*/
  if(n==0)return;
  memcpy((char  *)dest,(char  *)src,(size_t)n);
}
/*
** Set memory
*/
void Memset(void  *dest,char car, long n)
{ if(n<0) Bug("MM11", "MStInf"); /*set inf to zero*/
  if(n==0)return;
  memset(dest,car,(size_t)n);
}
/*
** Allocate memory
*/
/*code derived from DEU*/
#define SIZE_THRESHOLD        0x400L
#define SIZE_OF_BLOCK        0xFFFL
/* actually, this is (size - 1) */
void  *Malloc (long size)
{
   void  *ret;
   if(size<1)
   {  Warning("MM02", "Attempt to allocate %ld bytes",size);
      size=1;
   }
   if ((size_t) size != size)
      ProgError ("MM03",
	"Tried to allocate %ld b but couldn't; use another compiler", size);
   ret = malloc((size_t) size);
   if (ret==NULL)
      ProgError("MM04", "Out of memory (needed %ld bytes)", size);
   return ret;
}
/*
** Reallocate memory
*/
void  *Realloc (void  *old, long size)
{  void  *ret;

   if(size<1)
   {  Warning("MM05", "Attempt to allocate %ld bytes",size);
      size=1;
   }
   if ((size_t) size != size)
      ProgError ("MM06",
	"Tried to realloc %ld b but couldn't; use another compiler", size);
   ret = realloc( old, (size_t)size);
   if (ret==NULL)
      ProgError("MM07", "Out of memory (needed %ld bytes)", size);
   return ret;
}
/*
** Free
*/
void Free( void  *ptr)
{
   free (ptr);
}
/*****************************************************/
/*
** Use only lower case file names
*/
void ToLowerCase(char *file)
{ int16_t i;
  for(i=0;(i<128)&&(file[i]!='\0');i++)
         file[i]=tolower((((int16_t)file[i])&0xFF));
}

static void NameDir(char file[128], const char *path, const char *dir, const
    char *sdir)
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
void MakeDir(char file[128], const char *path, const char *dir, const char
    *sdir)
{  NameDir(file,path,dir,sdir);
   mkdir(file,(mode_t)S_IRWXU); /*read write exec owner*/
}

/*
** Create a file name, by concatenation
** returns true if file exists false otherwise
*/
bool MakeFileName(char file[128], const char *path, const char *dir, const
    char *sdir, const char *name, const char *extens)
{  FILE *fp;
   char name2[8];  /* AYM 1999-01-13: keep checker happy */
   /* deal with VILE strange name
   ** replace the VILE\ 
   ** by          VIL^B 
   */
   Normalise(name2,name);
 
   /* Replace backslash as it is an illegal character on 
      Windows.  */
   switch(name2[4])
   {
     case '\\': name2[4]='^';break;
   }
   switch(name2[6])
   { 
     case '\\': name2[6]='^';break;
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
     return true;
   }
   return false;
}
/*
** Get the root name of a WAD file
*/
void GetNameOfWAD(char name[8], const char *path)
{ int16_t n, nam,len;
  len=(int16_t)strlen(path);
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

/* convert 8 byte string to upper case, 0 padded*/
void Normalise(char dest[8], const char *src)  /*strupr*/
{ int16_t n;bool pad=false; char c='A';
  for(n=0;n<8;n++)
  { c= (pad==true)? '\0': src[n];
	 if(c=='\0')   pad=true;
	 else c=(isprint(c))? toupper(c) : '*';
	 dest[n] = c;}
}





/*
** Output auxilliary functions
*/

/*
 *	fnameofs - return string containing file name and offset
 *
 *	Not reentrant (returns pointer on static buffer).
 *	FIXME: should encode non-printable characters.
 *	FIXME: should have shortening heuristic (E.G. print only basename).
 */
char *fnameofs (const char *name, long ofs)
{
  static char buf[81];
  *buf = '\0';
  strncat (buf, name, sizeof buf - 12);
  sprintf (buf + strlen (buf), "(%06lXh)", ofs);
  return buf;
}


/*
 *	fname - return string containing file name
 *
 *	Not reentrant (returns pointer on static buffer).
 *	FIXME: should encode non-printable characters.
 *	FIXME: should have shortening heuristic (E.G. print only basename).
 */
char *fname (const char *name)
{
  static char buf[81];
  *buf = '\0';
  strncat (buf, name, sizeof buf - 1);
  return buf;
}


/*
 *	lump_name - return string containing lump name
 *
 *	Partially reentrant (returns pointer on one of two
 *	static buffers). The string is guaranteed to have at
 *	most 32 characters and to contain only graphic
 *	characters.
 */
char *lump_name (const char *name)
{
  static char buf1[9];
  static char buf2[9];
  static int  buf_toggle = 0;
  const char *const name_end = name + 8;
  char *buf = buf_toggle ? buf2 : buf1;
  char *p   = buf;

  buf_toggle = ! buf_toggle;
  if (*name == '\0')
    strcpy (buf, "(empty)");
  else
  {
    for (; *name != '\0' && name < name_end; name++)
    {
      if (isgraph ((unsigned char) *name))
	*p++ = toupper ((unsigned char) *name);
      else
      {
	*p++ = '\\';
	*p++ = 'x';
	*p++ = ((unsigned char) *name) >> 4;
	*p++ = *name & 0x0f;
      }
    }
    *p = '\0';
  }
  return buf;
}


/*
 *	short_dump - return string containing hex dump of buffer
 *
 *	Not reentrant (returns pointer on static buffer). Length
 *	is silently limited to 16 bytes.
 */
char *short_dump (const char *data, size_t size)
{
#define MAX_BYTES 16
  static char buf[3 * MAX_BYTES];
  char *b = buf;
  size_t n;

  for (n = 0; n < size && n < MAX_BYTES; n++)
  {
    if (n > 0)
      *b++ = ' ';
    *b++ = hex_digit[((unsigned char) data[n]) >> 4];
    *b++ = hex_digit[((unsigned char) data[n]) & 0x0f];
  }
  *b++ = '\0';
  return buf;
}


/*
 *	quotechar - return the safe representation of a char
 *
 *	Not reentrant (returns pointer on static buffer). The string is
 *	guaranteed to be exactly three characters long and not contain
 *	any control or non-ASCII characters.
 */
const char *quotechar (char c)
{
  static char buf[4];

  if (c >= 32 && c <= 126)
  {
    buf[0] = '"';
    buf[1] = c;
    buf[2] = '"';
    buf[3] = '\0';
  }
  else
  {
    buf[0] = hex_digit[((unsigned char) c) >> 4];
    buf[1] = hex_digit[((unsigned char) c) & 0x0f];
    buf[2] = 'h';
    buf[3] = '\0';
  }
  return buf;
}


/*
** Output and Error handling
*/
static bool asFile=false;
static int16_t Verbosity=2;
static FILE *Stdout;  /*command output*/
static FILE *Stderr;  /*errors*/
static FILE *Stdwarn;  /*warningss*/
static FILE *Stdinfo; /*infos*/

#define stderr_  (Stderr  != NULL ? Stderr  : stderr)
#define stdinfo_ (Stdinfo != NULL ? Stdinfo : stdout)
#define stdout_  (Stdout  != NULL ? Stdout  : stdout)
#define stdwarn_ (Stdwarn != NULL ? Stdwarn : stderr)

void PrintInit(bool asfile)
{
  /*clear a previous call*/
  PrintExit();
  /* choose */
  if(asfile==true)
  { if((Stdout=fopen("output.txt",FOPEN_WT))==NULL)
        ProgError ("DI10", "output.txt: %s", strerror (errno));
    if((Stderr=fopen("error.txt",FOPEN_WT))==NULL)
    {   Stderr=stderr;
        ProgError ("DI20", "error.txt: %s", strerror (errno));
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
void PrintVerbosity(int16_t level)
{ Verbosity=(level<0)? 0: (level>4)? 4:level;
}
void PrintExit(void)
{ if(asFile==true)
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

void ProgError (const char *code, const char *fmt, ...)
{
   va_list args;

   fflush (stdout_);
   fprintf(stderr_, "E %s ", code);
   va_start(args, fmt);
   vfprintf(stderr_, fmt, args);
   va_end(args);
   va_start(args, fmt);
   va_end(args);
   fputc('\n', stderr_);
   (*Action)();  /* execute error handler*/
   PrintExit();
   exit(2);
}

/*
 *	nf_err - non fatal error message
 */
void nf_err (const char *code, const char *fmt, ...)
{
  va_list args;

  fflush(stdout_);
  fprintf(stderr_, "%c %s ", MSGCLASS_ERR, code);
  va_start(args, fmt);
  vfprintf(stderr_, fmt, args);
  va_end(args);
  va_start(args, fmt);
  va_end(args);
  fputc('\n', stderr_);
  fflush(stderr_);
}

void Bug (const char *code, const char *fmt, ...)
{
  va_list args;

  fflush(stdout_);
  fprintf(stdwarn_, "%c %s ", MSGCLASS_BUG, code);
  va_start(args, fmt);
  vfprintf(stderr_, fmt, args);
  va_end(args);
  va_start(args, fmt);
  va_end(args);
  fputc('\n', stderr_);
  fputs("Please report that bug\n", stderr_);
  /* CloseWadFiles();*/
  PrintExit();
  exit(3);
}

void Warning (const char *code, const char *fmt, ...)
{
  va_list args;

  fflush(stdout_);
  fprintf(stdwarn_, "%c %s ", MSGCLASS_WARN, code);
  va_start(args, fmt);
  vfprintf(stdwarn_, fmt, args);
  va_end(args);
  va_start(args, fmt);
  va_end(args);
  fputc('\n', stdwarn_);
}

void LimitedWarn (int *left, const char *code, const char *fmt, ...)
{
  if (left == NULL || (left != NULL && *left > 0))
  {
    va_list args;

    fflush(stdout_);
    fprintf(stdwarn_, "%c %s ", MSGCLASS_WARN, code);
    va_start(args, fmt);
    vfprintf(stdwarn_, fmt, args);
    va_end(args);
    va_start(args, fmt);
    va_end(args);
    fputc('\n', stdwarn_);
  }
  if (left != NULL)
    (*left)--;
}

void LimitedEpilog (int *left, const char *code, const char *fmt, ...)
{
  if (left != NULL && *left < 0)
  {
    fflush (stdout_);
    if (fmt != NULL)
    {
      va_list args;
      fprintf(stdwarn_, "%c %s ", MSGCLASS_WARN, code);
      va_start (args, fmt);
      vfprintf (stdwarn_, fmt, args);
      va_end (args);
      va_start (args, fmt);
      va_end (args);
    }
    fprintf (stdwarn_, "%d warnings omitted\n", - *left);
  }
}

void Output (const char *fmt, ...)
{
  va_list args;

  va_start(args, fmt);
  vfprintf(stdout_, fmt, args);
  va_end(args);
  va_start(args, fmt);
  va_end(args);
}

void Info (const char *code, const char *fmt, ...)
{
  if (Verbosity >= 1)
  {
    va_list args;

    fprintf(stdinfo_, "%c %s ", MSGCLASS_INFO, code);
    va_start(args, fmt);
    vfprintf(stdinfo_, fmt, args);
    va_end(args);
    va_start(args, fmt);
    va_end(args);
    fputc('\n', stdinfo_);
  }
}

void Phase (const char *code, const char *fmt, ...)
{
  if (Verbosity >= 2)
  {
    va_list args;

    fprintf(stdinfo_, "%c %s ", MSGCLASS_INFO, code);
    va_start(args, fmt);
    vfprintf(stdinfo_, fmt, args);
    va_end(args);
    va_start(args, fmt);
    va_end(args);
    fputc('\n', stdinfo_);
  }
}

void Detail (const char *code, const char *fmt, ...)
{
  if (Verbosity >= 3)
  {
    va_list args;

    fprintf (stdinfo_, "%c %s ", MSGCLASS_INFO, code);
    va_start(args, fmt);
    vfprintf(stdinfo_, fmt, args);
    va_end(args);
    va_start(args, fmt);
    va_end(args);
    fputc ('\n', stdinfo_);
  }
}

#if 0
int16_t NbP=0;
void Progress(void)
{ NbP++;
  if(NbP&0xF==0) fprintf(stdinfo_,".");
  if(NbP>0x400)
  { NbP=0;
    fprintf(stdinfo_,"\n");
  }
}

void ProgressEnds(void)
{ fprintf(stdinfo_,"\n");
}
#endif
