/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0+
*/

/*
** simplified TEXT parsing
*/
struct TXTFILE {
    FILE *fp;
    int16_t Lines;
    int16_t LastChar;
    int16_t SectionStart;
    int16_t SectionEnd;
    char Section[8];
    char pathname[1];
};
/* A special instance of struct TXTFILE that is treated by the
   TXT*() output functions as the equivalent of /dev/null.
   Writing into it is a no-op. This convention is _not_
   supported by the input functions ! */
extern struct TXTFILE TXTdummy;

typedef uint16_t FLAGS;
#define F_RAW    0x0001  /* insert/extract with no conversion */

/*
** For any Reading of TEXT files
*/
void TXTinit(void);
struct TXTFILE *TXTopenR(const char *file, int silent);
void TXTcloseR(struct TXTFILE *TXT);
/*
** To read entries
*/
bool TXTskipComment(struct TXTFILE *TXT);
bool TXTseekSection(struct TXTFILE *TXT, const char *def);
bool TXTparseEntry(char *name, char *filenam, int16_t * x, int16_t * y,
                   FLAGS *flags, bool *repeat, struct TXTFILE *TXT, bool XY);
/*
** To read textures
*/
bool TXTreadTexDef(struct TXTFILE *TXT, char name[8], int16_t * szx,
                   int16_t * szy);
bool TXTreadPatchDef(struct TXTFILE *TXT, char name[8], int16_t * ofsx,
                     int16_t * ofsy);
/*
** To read PC sounds
*/
int16_t TXTreadShort(struct TXTFILE *TXT);

/*
** For any Writing of text files
*/
struct TXTFILE *TXTopenW(const char *file);     /*open, and init if needed */
void TXTcloseW(struct TXTFILE *TXT);
/*
** To write entries
*/
void TXTaddSection(struct TXTFILE *TXT, const char *def);
void TXTaddEntry(struct TXTFILE *TXT, const char *name, const char
                 *filenam, int16_t x, int16_t y, FLAGS flags,
                 bool repeat, bool XY);
void TXTaddComment(struct TXTFILE *TXT, const char *text);
void TXTaddEmptyLine(struct TXTFILE *TXT);
