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


void check_types (void);

void PrintCopyright (void);
void print_version (void);
void NoCommandGiven (void);


char *fnameofs (const char *name, long ofs);
char *fname (const char *name);
char *lump_name (const char *name);
char *short_dump (const char *data, size_t size);

void PrintInit(Bool asFile);
void PrintVerbosity(Int16 level);
void PrintExit(void);

void ProgErrorCancel(void);
void ProgErrorAction(void (*action)(void));
void ProgError (const char *, ...);    /*fatal error. halt.*/
void Bug (const char *, ...);          /*fatal bug. halt*/
void Warning (const char *str, ...);   /*I'm not happy*/
void LimitedWarn (int *left, const char *fmt, ...);
void LimitedEpilog (int *left, const char *fmt, ...);
void Output (const char *str, ...); /*command text output*/
void Info (const char *str, ...);   /*useful indications*/
void Phase (const char *str, ...);  /*phase of executions*/
void Detail (const char *str, ...); /*technical details*/
void Legal (const char *str, ...);  /*legal output*/





/******************tools.c***********************/

/** FILE name , for lumps and BMP **/
void ToLowerCase(char *file);
void MakeDir(char file[128], const char *path, const char *dir, const char
    *sdir);
Bool MakeFileName(char file[128], const char *path, const char *dir, const
    char *sdir, const char *name, const char *extens);
void GetNameOfWAD(char name[8], const char *path);
Int16 Chsize(int handle,Int32 newsize);
Int32 GetFileTime(const char *path);
void SetFileTime(const char *path, Int32 time);
void Unlink(const char *file);
void Memcpy (void  *dest,const void  *src, long n);
void Memset (void  *dest,char car, long n);
void  *Malloc (long size);
void  *Realloc (void  *old, long size);
void Free( void  *);


void Normalise(char dest[8], const char *src);

void Progress(void);
void ProgressEnds(void);
