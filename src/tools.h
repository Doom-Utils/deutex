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


void check_types (void);

void PrintCopyright(void);
void NoCommandGiven(void);


void PrintInit(Bool asFile);
void PrintVerbosity(Int16 level);
void PrintExit(void);

void ProgErrorCancel(void);
void ProgErrorAction(void (*action)(void));
void ProgError( char *, ...);    /*fatal error. halt.*/
void Bug( char *, ...);          /*fatal bug. halt*/
void Warning( char *str, ...);   /*I'm not happy*/
void Output(char *str, ...); /*command text output*/
void Info(char *str, ...);   /*useful indications*/
void Phase(char *str, ...);  /*phase of executions*/
void Detail(char *str, ...); /*technical details*/
void Legal(char *str, ...);  /*legal output*/





/******************tools.c***********************/

/** FILE name , for lumps and BMP **/
void ToLowerCase(char *file);
void MakeDir(char file[128], char *path,char *dir,char *sdir);
Bool MakeFileName(char file[128], char *path,char *dir,char *sdir,char *name,char *extens);
void GetNameOfWAD(char name[8],char *path);
Int16 Chsize(int handle,Int32 newsize);
Int32 GetFileTime(char *path);
void SetFileTime(char *path, Int32 time);
void Unlink(char *file);
void Memcpy(void huge *dest,const void huge *src, Int32 n);
void Memset(void huge *dest,char car, Int32 n);
void huge *Malloc( Int32 size);
void huge *Realloc( void huge *old, Int32 size);
void Free( void huge *);


void Normalise(char dest[8], char *src);

void Progress(void);
void ProgressEnds(void);
