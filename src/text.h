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


/*
** simplified TEXT parsing
*/
struct TXTFILE
{ FILE *fp;
  Int16 Lines;
  Int16 LastChar;
  Int16 SectionStart;
  Int16 SectionEnd;
  char Section[8];
};
/* A special instance of struct TXTFILE that is treated by the
   TXT*() output functions as the equivalent of /dev/null.
   Writing into it is a no-op. This convention is _not_
   supported by the input functions ! */
extern struct TXTFILE TXTdummy;

/*
** For any Reading of TEXT files
*/
void   TXTinit(void);
struct TXTFILE *TXTopenR(const char *file); /*open, and init if needed*/
void   TXTcloseR(struct TXTFILE *TXT);
/*
** To read entries
*/
Bool   TXTskipComment(struct TXTFILE *TXT);
Bool   TXTseekSection(struct TXTFILE *TXT,const char *def);
Bool   TXTentryParse(char *name,char *filenam,Int16 *x,Int16
    *y,Bool *repeat, struct TXTFILE *TXT, Bool XY);
/*
** To read textures
*/
Bool   TXTreadTexDef(struct TXTFILE *TXT,char name[8],Int16 *szx,Int16 *szy);
Bool   TXTreadPatchDef(struct TXTFILE *TXT,char name[8],Int16 *ofsx,Int16 *ofsy);
/*
** To read PC sounds
*/
Int16  TXTreadShort(struct TXTFILE *TXT);


/*
** For any Writing of text files
*/
struct TXTFILE *TXTopenW(const char *file); /*open, and init if needed*/
void   TXTcloseW(struct TXTFILE *TXT);
/*
** To write entries
*/
void TXTaddSection(struct TXTFILE *TXT,const char *def);
void TXTaddEntry(struct TXTFILE *TXT,const char *name,const char
    *filenam,Int16 x,Int16 y,Bool repeat, Bool XY);
void TXTaddComment(struct TXTFILE *TXT,const char *text);
void TXTaddEmptyLine (struct TXTFILE *TXT);

