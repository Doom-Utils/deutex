/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0+
*/

#include "deutex.h"
#include <errno.h>
#include "tools.h"
#include "text.h"
#include <ctype.h>

/* A special instance of struct TXTFILE that is treated by the
   TXT*() output functions as the equivalent of /dev/null.
   Writing into it is a no-op. This convention is _not_
   supported by the input functions ! */
struct TXTFILE TXTdummy;

/*****************************************************/

const int16_t SPACE = 0x2;
const int16_t NEWLINE = 0x4;
const int16_t COMMENT = 0x8;
const int16_t SECTION = 0x10;   /*ok for SECTION header */
const int16_t NAME = 0x20;      /*valid as a name identifier */
const int16_t NUMBER = 0x40;    /*valid for a number */
const int16_t STPATCH = 0x80;   /*start of patch? */
const int16_t EXESTRNG = 0x100; /*valid in #string# */
const int16_t BOUNDARY = 0x200; /*# */
const int16_t STEQUAL = 0x400;  /*=*/
const int16_t STFLAG = 0x800;   /*start of flag? */
static int16_t TXTval[256];
static bool TXTok = false;

void TXTinit(void)
{
    int16_t n, val;
    for (n = 0; n < 256; n++) {
        val = 0;
        switch (n) {
        case '#':              /*comment,string boundary */
            val |= BOUNDARY + COMMENT;
            break;
        case ';':              /*comment */
            val |= COMMENT + EXESTRNG;
            break;
        case '\0':
        case '\n':             /*newline */
            val |= NEWLINE;
            break;
        case '_':              /*in name */
        case '\\':             /* deal with VILE strange name */
            val |= NAME + EXESTRNG;
            break;
        case '[':              /* deal with VILE strange name */
        case ']':              /* deal with VILE strange name */
            val |= SECTION + NAME + EXESTRNG;
            break;
        case '-':
        case '+':              /*in integer number */
            val |= NUMBER + EXESTRNG;
            break;
        case '*':              /*start of patch wall */
            val |= EXESTRNG + STPATCH;
            break;
        case '=':
            val |= STEQUAL + EXESTRNG;
            break;
        case '/':
            val |= STFLAG + EXESTRNG;
            break;
        case '?':
        case '!':
        case '.':
        case ',':
        case '\'':
        case '&':
        case '(':
        case ')':
        case '$':
        case '%':
        case '@':
        case '<':
        case '>':
        case ' ':
        case '^':
        case '\"':
        case ':':
            val |= EXESTRNG;
            break;
        default:
            break;
        }
        if (isdigit(n))
            val |= NUMBER + EXESTRNG;
        if (isalpha(n))
            val |= SECTION + NAME + EXESTRNG;
        if (isspace(n))
            val |= SPACE;
        if (n == '%')    // Deal with Strife's "INVFONG%" and "INVFONY%"
            val |= NAME;
        TXTval[n] = val;
    }
    TXTok = true;
}

void TXTcloseR(struct TXTFILE *TXT)
{
    if (!TXTok)
        Bug("TR91", "TxtClo");
    fclose(TXT->fp);
    free(TXT);
}

struct TXTFILE *TXTopenR(const char *file, int silent)
{
    struct TXTFILE *TXT;
    size_t pathname_len;
    /*characters */
    if (!TXTok)
        TXTinit();

    pathname_len = strlen(file);
    TXT = (struct TXTFILE *) Malloc(sizeof(struct TXTFILE) + pathname_len);
    /*some inits */
    strcpy(TXT->pathname, file);
    TXT->Lines = 1;             /*start in line 1 */
    TXT->SectionStart = 0;
    TXT->SectionEnd = 0;
    TXT->fp = fopen(file, FOPEN_RT);
    if (TXT->fp == NULL) {
        if (silent) {
            free(TXT);
            return NULL;
        }
        ProgError("TR03", "%s: %s", fname(file), strerror(errno));
    }
    return TXT;
}

static bool TXTgetc(struct TXTFILE *TXT, int16_t * c, int16_t * val)
{
    int16_t cc = (int16_t) getc(TXT->fp);
    TXT->LastChar = cc;
    if (cc == EOF)
        return false;
    *c = cc = (cc & 0xFF);
    *val = TXTval[cc];
    if (TXTval[cc] & NEWLINE)
        TXT->Lines++;
    return true;
}

static void TXTungetc(struct TXTFILE *TXT)
{
    int16_t cc = TXT->LastChar;
    cc = (cc & 0xFF);
    ungetc(cc, TXT->fp);
    if (TXTval[cc] & NEWLINE)
        TXT->Lines--;
}

/*skip lines beginning with # or ; */
bool TXTskipComment(struct TXTFILE *TXT)
{
    int16_t c = 0, val = 0;
    bool comment;
    for (comment = false;;) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (val & NEWLINE) {    /*eat newlines */
            comment = false;
            continue;
        }
        if (val & COMMENT) {    /*eat commentaries */
            comment = true;
            continue;
        }
        if (val & SPACE) {      /*eat space */
            continue;
        }
        if (!comment) {
            TXTungetc(TXT);
            return true;
        }
    }
}

/* find '*' */
static bool TXTcheckStartPatch(struct TXTFILE *TXT)
{
    int16_t c = 0, val = 0;
    if (!TXTgetc(TXT, &c, &val))
        return false;
    if (val & STPATCH)
        return true;
    TXTungetc(TXT);
    return false;
}

/*read string, skip space before, stop space/\n*/
static bool TXTread(struct TXTFILE *TXT, char name[8], int16_t valid)
{
    int16_t c = 0, val = 0, n = 0;
    while (1) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (val & NEWLINE)
            continue;
        if (val & SPACE)
            continue;
        if (val & valid)
            break;
        ProgError("TR11", "%s(%ld): illegal char %s", TXT->pathname,
                  (long) TXT->Lines, quotechar(c));
    }
    name[0] = (char) c;
    for (n = 1; n < 256; n++) {
        if (!TXTgetc(TXT, &c, &val))
            break;
        if (val & SPACE) {
            TXTungetc(TXT);
            break;
        }
        if (!(val & valid))
            ProgError("TR13", "%s(%ld): illegal char %s in word",
                      TXT->pathname, (long) TXT->Lines, quotechar(c));
        if (n < 8)
            name[n] = (char) c;
    }
    if (n < 8)
        name[n] = '\0';
    return true;
}

int16_t TXTreadShort(struct TXTFILE * TXT)
{
    static char buffer[9];
    TXTread(TXT, buffer, NUMBER);
    buffer[8] = '\0';
    return (int16_t) atoi(buffer);
}

static bool TXTboundSection(struct TXTFILE *TXT);
static bool TXTreadIdent(struct TXTFILE *TXT, char name[8])
{
    if (!TXTok)
        Bug("TR21", "%s: TxtRid", fname(TXT->pathname));
    if (!TXTskipComment(TXT))
        return false;
    /*check end of section */
    if ((TXT->Lines) > (TXT->SectionEnd)) {
        if (!TXTboundSection(TXT))
            return false;       /*no other section */
    }
    if (!TXTread(TXT, name, NAME | NUMBER))
        ProgError("TR23", "%s(%ld): expected identifier or \"END:\"",
                  TXT->pathname, (long) TXT->Lines);
    Normalise(name, name);
    return true;
}

/*
** STPATCH is also used to indicate repetition
*/
static bool TXTreadOptionalRepeat(struct TXTFILE *TXT)
{
    int16_t c = 0, val = 0;
    while (1) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (!(val & NEWLINE)) {
            if (val & STPATCH)
                break;          /*look for STPATCH */
            if (val & SPACE)
                continue;       /*skip space */
        }
        TXTungetc(TXT);
        return false;
    }
    return true;                /*found */
}

/*
** STEQUAL is used to indicate alternate name
*/
static bool TXTreadOptionalName(struct TXTFILE *TXT, char name[8])
{
    int16_t c = 0, val = 0;

    /* check for the equal */
    while (1) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (c == '=')
            break;
        if (!(val & NEWLINE) && (val & SPACE))
            continue;       /*skip space */

        TXTungetc(TXT);
        return false;
    }

    if (!TXTread(TXT, name, NAME | NUMBER)) {
        ProgError("TR32", "%s(%ld): invalid optional name", TXT->pathname,
                  (long) TXT->Lines);
    }

    return true;
}

/*
** Read integer if exist before NEWLINE,
** but don't eat NEWLINE
*/
static int16_t TXTreadOptionalShort(struct TXTFILE *TXT)
{
    static char name[9];
    int16_t n, c = 0, val = 0;
    while (1) {
        if (!TXTgetc(TXT, &c, &val))
            return INVALIDINT;
        if (!(val & NEWLINE)) {
            if (val & SPACE)
                continue;       /*skip space */
            if (val & STEQUAL)
                continue;       /*skip '=' */
            if (val & NUMBER)
                break;          /*look for number */
        }
        TXTungetc(TXT);
        return INVALIDINT;      /*not a number. abort */
    }
    name[0] = (char) c;
    for (n = 1; n < 256; n++) {
        if (!TXTgetc(TXT, &c, &val))
            break;
        if (val & NEWLINE) {
            TXTungetc(TXT);
            break;
        }
        if (val & SPACE)
            break;
        if (!(val & NUMBER))
            ProgError("TR42", "%s(%ld): illegal char %s in number",
                      fname(TXT->pathname), (long) TXT->Lines,
                      quotechar(c));
        if (n < 8)
            name[n] = (char) c;
    }
    if (n < 8)
        name[n] = '\0';
    name[8] = '\0';
    return (int16_t) atoi(name);
}

/*
** Parse a flag, which is a keyword preceded by a '/'.
*/
static bool TXTreadOptionalFlag(struct TXTFILE *TXT, FLAGS *flags)
{
    uint16_t c = 0, val = 0;
    char flag_name[9];

    /* check for the slash */
    while (1) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (c == '/')
            break;
        if (!(val & NEWLINE) && (val & SPACE))
            continue;       /*skip space */

        TXTungetc(TXT);
        return false;
    }

    if (!TXTread(TXT, flag_name, NAME | NUMBER)) {
        ProgError("TR33", "%s(%ld): invalid flag name", TXT->pathname,
                  (long) TXT->Lines);
    }

    flag_name[8] = 0;
    Normalise(flag_name, flag_name);

    if (strcmp(flag_name, "RAW") == 0) {
        (*flags) |= F_RAW;
    } else {
        ProgError("TR34", "%s(%ld): unknown flag /%s", TXT->pathname,
                  (long) TXT->Lines, flag_name);
    }

    return true;
}

static void TXTreadOptionalFlags(struct TXTFILE *TXT, FLAGS *flags)
{
    while (TXTreadOptionalFlag(TXT, flags)) {
    }
}

/* read Blocks of the form
** [BLOCKID]
** identifier   ... anything ...
** identifier   ... anything ...
*/
static bool TXTfindSection(struct TXTFILE *TXT, bool Match)
{
    int16_t c = 0, val = 0, n;
    char buffer[8];
    while (1) {
        if (!TXTskipComment(TXT))
            return false;
        if (!TXTgetc(TXT, &c, &val))
            return false;
        if (c == '[') {
            for (n = 0; n < 256; n++) {
                if (!TXTgetc(TXT, &c, &val))
                    return false;
                if (c == ']') {
                    if (n < 8)
                        buffer[n] = '\0';
                    if (!Match)
                        return true;    /*any section is ok */
                    Normalise(buffer, buffer);  /*the right section? */
                    if (strncmp(buffer, TXT->Section, 8) == 0)
                        return true;
                    break;      /*not the right section */
                }
                if (!(val & (NAME | NUMBER)))
                    break;      /*not a section */
                if (n < 8)
                    buffer[n] = c;
            }
        }
        while (1) {             /*look for end of line */
            if (!TXTgetc(TXT, &c, &val))
                return false;
            if (val & NEWLINE)
                break;
        }
    }
}

/*
** find the section boundaries, from current position in file
*/
static bool TXTboundSection(struct TXTFILE *TXT)
{
    int16_t c = 0, val = 0;
    if (!TXTfindSection(TXT, true))
        return false;
    TXT->SectionStart = TXT->Lines + 1;
    /*check that we don't read twice the same section */
    if (TXT->SectionEnd > TXT->SectionStart)
        Bug("TR51", "TxtBdS");
    if (TXTfindSection(TXT, false))
        TXT->SectionEnd = TXT->Lines - 1;
    else
        TXT->SectionEnd = TXT->Lines;
    /* set pointer to first section line */
    fseek(TXT->fp, 0, SEEK_SET);
    TXT->Lines = 1;             /*start in line 1 */
    while (TXT->Lines < TXT->SectionStart) {
        if (!TXTgetc(TXT, &c, &val))
            return false;
    }
    return true;
}

bool TXTseekSection(struct TXTFILE * TXT, const char *section)
{
    if (!TXTok)
        Bug("TR61", "TxtSks");
    /*seek begin of file */
    TXT->SectionStart = 0;
    TXT->SectionEnd = 0;
    Normalise(TXT->Section, section);
    fseek(TXT->fp, 0L, SEEK_SET);
    TXT->Lines = 1;             /*start in line 1 */
    /*skipping comments, look for a line with
      <space>[section] */
    return TXTboundSection(TXT);
}

/*read a texture definition*/
/*return false if read End*/
bool TXTreadTexDef(struct TXTFILE * TXT, char name[8], int16_t * szx,
                   int16_t * szy)
{
    if (!TXTok)
        Bug("TR71", "TxtTxd");
    if (!TXTskipComment(TXT))
        return false;           /*End */
    if (!TXTread(TXT, name, NAME | NUMBER))
        ProgError("TR73", "%s(%ld): expecting identifier",
                  fname(TXT->pathname), (long) TXT->Lines);
    Normalise(name, name);
    *szx = TXTreadShort(TXT);
    *szy = TXTreadShort(TXT);
    return true;
}

/*read a patch def.  Return false if could not find '*' */
bool TXTreadPatchDef(struct TXTFILE * TXT, char name[8], int16_t * ofsx,
                     int16_t * ofsy)
{
    if (!TXTok)
        Bug("TR81", "TxtRpd");
    if (!TXTskipComment(TXT))
        return false;
    if (!TXTcheckStartPatch(TXT))
        return false;           /*not a patch line */
    if (!TXTread(TXT, name, NAME | NUMBER))
        ProgError("TR83", "%s(%ld): expecting identifier",
                  fname(TXT->pathname), (long) TXT->Lines);
    Normalise(name, name);
    *ofsx = TXTreadShort(TXT);
    *ofsy = TXTreadShort(TXT);
    return true;
}

bool TXTparseEntry(char *name, char *filenam, int16_t * x, int16_t * y,
                   FLAGS *flags, bool *repeat, struct TXTFILE * TXT, bool XY)
{
    int16_t c = 0, val = 0;
    bool comment;
    int16_t xx = INVALIDINT, yy = INVALIDINT;
    if (!TXTreadIdent(TXT, name))
        return false;
    *flags = 0;
    TXTreadOptionalFlags(TXT, flags);
    /* read integer */
    if (XY) {
        xx = TXTreadOptionalShort(TXT);
        yy = TXTreadOptionalShort(TXT);
    }
    Normalise(filenam, name);
    if (TXTreadOptionalName(TXT, filenam)) {
        /* flags and/or XY values can appear either after the base name or
           after the optional filename */
        TXTreadOptionalFlags(TXT, flags);
        if (XY) {
            if (xx == INVALIDINT)
                xx = TXTreadOptionalShort(TXT);
            if (yy == INVALIDINT)
                yy = TXTreadOptionalShort(TXT);
        }
    }
    *repeat = TXTreadOptionalRepeat(TXT);
    *x = xx;
    *y = yy;
    for (comment = false;;) {
        if (!TXTgetc(TXT, &c, &val))
            break;
        if (val & NEWLINE)
            break;
        if (val & COMMENT) {    /*eat commentaries */
            comment = true;
            continue;
        }
        if (val & SPACE) {      /*eat space */
            continue;
        }
        if (!comment)
            ProgError("TR87", "%s(%ld): bad entry format",
                      fname(TXT->pathname), (long) TXT->Lines);
    }
    return true;
}

/*
** For any Writing of text files
*/
struct TXTFILE *TXTopenW(const char *file)
{                               /*open, and init if needed */
    struct TXTFILE *TXT;
    size_t pathname_len;

    /*characters */
    if (!TXTok)
        TXTinit();
    pathname_len = strlen(file);
    TXT = (struct TXTFILE *) Malloc(sizeof(struct TXTFILE) + pathname_len);
    /*some inits */
    strcpy(TXT->pathname, file);
    TXT->Lines = 1;             /*start in line 1 */
    TXT->SectionStart = 0;
    TXT->SectionEnd = 0;
    TXT->fp = fopen(file, FOPEN_RT);
    if (TXT->fp == NULL) {
        TXT->fp = fopen(file, FOPEN_WT);
    } else {
        fclose(TXT->fp);
        TXT->fp = fopen(file, FOPEN_AT);
        Warning("TW03", "%s: already exists, appending to it",
                fname(file));
    }
    if (TXT->fp == NULL)
        ProgError("TW05", "%s: %s", fname(file), strerror(errno));
    return TXT;
}

void TXTcloseW(struct TXTFILE *TXT)
{
    if (TXT == &TXTdummy)
        return;
    if (!TXTok)
        Bug("TW91", "TxtClo");
    fclose(TXT->fp);
    free(TXT);
}

/*
** To write entries
*/
void TXTaddSection(struct TXTFILE *TXT, const char *def)
{
    if (TXT == &TXTdummy)
        return;
    if (!TXTok)
        Bug("TW11", "TxtAdS");
    fprintf(TXT->fp, "[%.8s]\n", def);
}

void TXTaddEntry(struct TXTFILE *TXT, const char *name,
                 const char *filenam, int16_t x, int16_t y,
                 FLAGS flags, bool repeat, bool XY)
{
    if (TXT == &TXTdummy)
        return;
    if (!TXTok)
        Bug("TW21", "TxtAdE");
    fprintf(TXT->fp, "%.8s", name);
    if (flags & F_RAW)
        fprintf(TXT->fp, "\t/raw", filenam);
    if (XY)
        fprintf(TXT->fp, "\t%d\t%d", x, y);
    if (filenam != NULL)
        fprintf(TXT->fp, "\t= %.8s", filenam);
    if (repeat)
        fprintf(TXT->fp, "\t*");
    fprintf(TXT->fp, "\n");
}

void TXTaddComment(struct TXTFILE *TXT, const char *text)
{
    if (TXT == &TXTdummy)
        return;
    if (!TXTok)
        Bug("TW31", "TxtAdC");
    fprintf(TXT->fp, "# %.256s\n", text);
}

void TXTaddEmptyLine(struct TXTFILE *TXT)
{
    if (TXT == &TXTdummy)
        return;
    if (!TXTok)
        Bug("TW41", "TxtAdL");
    putc('\n', TXT->fp);
}
