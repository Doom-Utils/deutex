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
** This code should contain all the tricky O/S related
** functions. If you're porting DeuTex/DeuSF, look here!
*/

#include "deutex.h"
#include "tools.h"
#include "text.h"
#include <ctype.h>

/*compile only for DeuTex*/
#if defined DeuTex

/* A special instance of struct TXTFILE that is treated by the
   TXT*() output functions as the equivalent of /dev/null.
   Writing into it is a no-op. This convention is _not_
   supported by the input functions ! */
struct TXTFILE TXTdummy;

/*****************************************************/

const Int16 SPACE    = 0x2;
const Int16 NEWLINE  = 0x4;
const Int16 COMMENT  = 0x8;
const Int16 SECTION  = 0x10;  /*ok for SECTION header*/
const Int16 NAME     = 0x20;  /*valid as a name identifier*/
const Int16 NUMBER   = 0x40;  /*valid for a number*/
const Int16 STPATCH  = 0x80;  /*start of patch?*/
const Int16 EXESTRNG = 0x100; /*valid in #string# */
const Int16 BOUNDARY = 0x200; /*#*/
const Int16 STEQUAL  = 0x400; /*=*/
static Int16  TXTval[256];
static Bool TXTok=FALSE;



void TXTinit(void)
{ Int16 n,val;
  for(n=0;n<256;n++)
    { val=0;
      switch(n)
      { case '#':                   /*comment,string boundary*/
	   val |=BOUNDARY+COMMENT;break;
	case ';':		    /*comment*/
	   val |=COMMENT+EXESTRNG;break;
	case '\0': case '\n':       /*newline*/
	   val |=NEWLINE;break;
	case '_':                   /*in name*/
	case '\\':		/* deal with VILE strange name*/
	   val |=NAME+EXESTRNG;break;
	case '[':		/* deal with VILE strange name*/
	case ']':		/* deal with VILE strange name*/
	   val |=SECTION+NAME+EXESTRNG;break;
	case '-': case '+':         /*in integer number*/
	   val |=NUMBER+EXESTRNG;break;
	case '*':                   /*start of patch wall*/
	   val |= EXESTRNG+STPATCH;break;
	case '=':
	   val |= STEQUAL+EXESTRNG;break;
	case '?': case '!':
	case '.': case ',': case '\'':
	case '&': case '(': case ')':
	case '$': case '%': case '@':
	case '/': case '<': case '>':
	case ' ': case '^': case '\"':
	case ':':
	   val |=EXESTRNG;break;
	default: break;
      }
      if(isdigit(n)) val |= NUMBER+EXESTRNG;
      if(isalpha(n)) val |= SECTION+NAME+EXESTRNG;
      if(isspace(n)) val |= SPACE;
      if (n == '%')	// Deal with Strife's "INVFONG%" and "INVFONY%"
	val |= NAME;
      TXTval[n]=val;
    }
  TXTok=TRUE;
}
void TXTcloseR(struct TXTFILE *TXT)
{ 
  if(TXTok!=TRUE) Bug("TxtClo");
  fclose(TXT->fp);
  Free(TXT);
}

struct TXTFILE *TXTopenR(const char *file)
{ struct TXTFILE *TXT;
  /*characters*/
  if(TXTok!=TRUE)  TXTinit();
  TXT = (struct TXTFILE *)Malloc(sizeof(struct TXTFILE));
  /*some inits */
  TXT->Lines	   =1;/*start in line 1*/
  TXT->SectionStart=0;
  TXT->SectionEnd  =0;
  TXT->fp 	   = fopen(file,FOPEN_RT);
  if(TXT->fp==NULL)  ProgError("Could not open file %s for reading",file);
  return TXT;
}

static Bool TXTgetc(struct TXTFILE *TXT,Int16 *c,Int16 *val)
{ Int16 cc=(Int16)getc(TXT->fp );
  TXT->LastChar=cc;
  if(cc==EOF) return FALSE;
  *c = cc = (cc&0xFF);
  *val=TXTval[cc];
  if(TXTval[cc]&NEWLINE) TXT->Lines++;
  return TRUE;
}
static void TXTungetc(struct TXTFILE *TXT)
{ Int16 cc=TXT->LastChar;
  cc = (cc&0xFF);
  ungetc(cc,TXT->fp);
  if(TXTval[cc]&NEWLINE) TXT->Lines--;
}
/*skip lines beginning with # or ; */
Bool TXTskipComment(struct TXTFILE *TXT)
{ Int16 c=0,val=0; Bool comment;
  for(comment=FALSE;;)
  { if(TXTgetc(TXT,&c,&val)!=TRUE)return FALSE;
    if(val & NEWLINE)        /*eat newlines*/
       { comment=FALSE; continue;}
    if(val & COMMENT)        /*eat commentaries*/
       { comment=TRUE;  continue;}   
    if(val & SPACE)          /*eat space*/
       { continue;}
    if(comment==FALSE)
       { TXTungetc(TXT);return TRUE;}
  }
}
/* find '*' */
static Bool TXTcheckStartPatch(struct TXTFILE *TXT)
{ Int16 c=0, val=0;
  if(TXTgetc(TXT,&c,&val)!=TRUE) return FALSE;
  if(val & STPATCH) return TRUE;
  TXTungetc(TXT); return FALSE;
}
/*read string, skip space before, stop space/\n*/
static Bool TXTread(struct TXTFILE *TXT,char name[8],Int16 valid)
{  Int16 c=0,val=0,n=0;
   while(1)
   { if(TXTgetc(TXT,&c,&val)!=TRUE)return FALSE;
     if(val & NEWLINE) continue;
     if(val & SPACE)   continue;
     if(val & valid) break;
     ProgError("Line %d: Illegal char '%c'",TXT->Lines,c);
   }
   name[0]=(char)c;
   for(n=1; n<256; n++ )
   { if(TXTgetc(TXT,&c,&val)!=TRUE) break;  
     if(val&SPACE)
     { TXTungetc(TXT);break;}
     if(!(val&valid))
        ProgError("Line %d: Illegal char '%c' in word",TXT->Lines,c);
     if(n<8) name[n]=(char)c;
   }
   if(n<8)name[n]='\0';
   return TRUE;
}
Int16 TXTreadShort(struct TXTFILE *TXT)
{ static char buffer[8];
  TXTread(TXT,buffer,NUMBER);
  buffer[8]='\0';
  return (Int16)atoi(buffer);
}
static Bool TXTboundSection(struct TXTFILE *TXT);
static Bool TXTreadIdent(struct TXTFILE *TXT,char name[8])
{ if(TXTok!=TRUE) Bug("TxtRid");
  if(TXTskipComment(TXT)==FALSE) return FALSE;
  /*check end of section*/
  if((TXT->Lines)>(TXT->SectionEnd))
  { if(TXTboundSection(TXT)==FALSE)
      return FALSE;   /*no other section*/
  }
  if(TXTread(TXT,name,NAME|NUMBER)!=TRUE)      	ProgError("Line %d: Expecting identifier or 'END:'",TXT->Lines);
  Normalise(name,name);
  return TRUE;
}

/*
** STPATCH is also used to indicate repetition
*/
static Bool TXTreadOptionalRepeat(struct TXTFILE *TXT)
{ Int16 c=0,val=0;
  while(1)
  {  if(TXTgetc(TXT,&c,&val)!=TRUE) return FALSE;
     if(!(val & NEWLINE))
     { if(val & STPATCH) break;  /*look for STPATCH*/
       if(val & SPACE) continue; /*skip space*/
     }
     TXTungetc(TXT);
     return FALSE;
  }
  return TRUE;               /*found*/
}
/*
** STEQUAL is used to indicate alternate name
*/
static void TXTreadOptionalName(struct TXTFILE *TXT,char name[8])
{ Int16 c=0,val=0;
  while(1)
  {  if(TXTgetc(TXT,&c,&val)!=TRUE) return;
     if(!(val & NEWLINE))
     { if(val & STEQUAL) continue; /*skip '='*/
       if(val & SPACE) continue; /*skip space*/
       if(val & (NAME&(~NUMBER))) break;
     }
     TXTungetc(TXT);
     return; /*name is NOT modified*/
  }
  TXTungetc(TXT);
  if(TXTread(TXT,name,NAME|NUMBER)!=TRUE)
  { ProgError("invalid optional name");
  }
}
/*
** Read integer if exist before NEWLINE,
** but don't eat NEWLINE
*/
static Int16 TXTreadOptionalShort(struct TXTFILE *TXT)
{ static char name[8];
  Int16 n,c=0,val=0;
  while(1)
  { if(TXTgetc(TXT,&c,&val)!=TRUE)return INVALIDINT;
    if(!(val & NEWLINE))
    { if(val & SPACE)   continue; /*skip space*/
      if(val & STEQUAL) continue; /*skip '='*/
      if(val & NUMBER)  break;    /*look for number*/
    }
    TXTungetc(TXT);
    return INVALIDINT;               /*not a number. abort*/
   }
   name[0]=(char)c;
   for(n=1; n<256; n++ )
   { if(TXTgetc(TXT,&c,&val)!=TRUE) break;
     if(val&NEWLINE) {TXTungetc(TXT);break;}
     if(val&SPACE)break;
     if(!(val&NUMBER))
	ProgError("Line %d: Illegal char '%c' in number",TXT->Lines,c);
     if(n<8) name[n]=(char)c;
   }
   if(n<8)name[n]='\0';
   name[8]='\0';
   return (Int16)atoi(name);
}

/* read Blocks of the form
** [BLOCKID]
** identifier   ... anything ...
** identifier   ... anything ...
*/
static Bool TXTfindSection(struct TXTFILE *TXT,Bool Match)
{ Int16 c=0,val=0,n;
  char buffer[8];
  while(1)
  { if(TXTskipComment(TXT)!=TRUE) return FALSE;
    if(TXTgetc(TXT,&c,&val)!=TRUE)return FALSE;
    if(c=='[')
    { for(n=0;n<256;n++)
      { if(TXTgetc(TXT,&c,&val)!=TRUE) return FALSE;
	if(c==']')
	{ if(n<8)buffer[n]='\0';
	  if(Match==FALSE) return TRUE;/*any section is ok*/
	  Normalise(buffer,buffer);        /*the right section?*/
	  if(strncmp(buffer,TXT->Section,8)==0) return TRUE;
	  break; 			   /*not the right section*/
	}
	if(!(val & (NAME|NUMBER))) break;  /*not a section*/
	if(n<8) buffer[n]=c;
      }
    }
    while(1) /*look for end of line*/
    { if(TXTgetc(TXT,&c,&val)!=TRUE)return FALSE;
      if(val & NEWLINE) break;
    }
  }
}
/*
** find the section boundaries, from current position in file
*/
static Bool TXTboundSection(struct TXTFILE *TXT)
{ Int16 c=0,val=0;
  if(TXTfindSection(TXT,TRUE)!=TRUE) return FALSE;
  TXT->SectionStart=TXT->Lines+1;
  /*check that we don't read twice the same section*/
  if(TXT->SectionEnd>TXT->SectionStart) Bug("TxtBdS");
  if(TXTfindSection(TXT,FALSE)==TRUE)
    TXT->SectionEnd=TXT->Lines-1;
  else
    TXT->SectionEnd=TXT->Lines;
  /* set pointer to first section line*/
  fseek(TXT->fp,0,SEEK_SET);
  TXT->Lines	   =1;/*start in line 1*/
  while(TXT->Lines<TXT->SectionStart)
  { if(TXTgetc(TXT,&c,&val)!=TRUE)return FALSE;
  }
  return TRUE;
}
Bool TXTseekSection(struct TXTFILE *TXT,const char *section)
{
  if(TXTok!=TRUE) Bug("TxtSks");
  /*seek begin of file*/
  TXT->SectionStart=0;
  TXT->SectionEnd  =0;
  Normalise(TXT->Section,section);
  fseek(TXT->fp,0L,SEEK_SET);
  TXT->Lines	   =1;/*start in line 1*/
  /*skipping comments, look for a line with
  <space>[section]*/
  return TXTboundSection(TXT);
}



/*read a texture definition*/
/*return FALSE if read End*/
Bool TXTreadTexDef(struct TXTFILE *TXT,char name[8],Int16 *szx,Int16 *szy)
{ if(TXTok!=TRUE) Bug("TxtTxd");
  if(TXTskipComment(TXT)==FALSE) return FALSE; /*End*/
  if(TXTread(TXT,name,NAME|NUMBER)!=TRUE)	ProgError("Line %d: Expecting identifier",TXT->Lines);
  Normalise(name,name);
  *szx=TXTreadShort(TXT);
  *szy=TXTreadShort(TXT);
  return TRUE;
}
/*read a patch def.  Return FALSE if could not find '*' */
Bool TXTreadPatchDef(struct TXTFILE *TXT,char name[8],Int16 *ofsx,Int16 *ofsy)
{ if(TXTok!=TRUE) Bug("TxtRpd");
  if(TXTskipComment(TXT)==FALSE) return FALSE;
  if(TXTcheckStartPatch(TXT)!=TRUE) return FALSE; /*not a patch line*/
  if(TXTread(TXT,name,NAME|NUMBER)!=TRUE)
	   ProgError("Line %d: Expecting identifier",TXT->Lines);
  Normalise(name,name);
  *ofsx=TXTreadShort(TXT);
  *ofsy=TXTreadShort(TXT);
  return TRUE;
}
/*
**TXTentryParse(name,filenam,&X,&Y,&Repeat,TXT,TRUE)
*/
Bool TXTentryParse(char *name,char *filenam,Int16 *x,Int16 *y,Bool
    *repeat, struct TXTFILE *TXT, Bool XY)
{ Int16 c=0,val=0; Bool comment;
  Int16 xx=INVALIDINT,yy=INVALIDINT;
  if(TXTreadIdent(TXT,name)!=TRUE) return FALSE;
  /* skip the equal*/
  if(TXTgetc(TXT,&c,&val)!=TRUE) return FALSE;
  if(c!='=') TXTungetc(TXT);
  /* read integer*/
  if(XY==TRUE)
  { xx=TXTreadOptionalShort(TXT);
    yy=TXTreadOptionalShort(TXT);
  }
  Normalise(filenam,name);
  TXTreadOptionalName(TXT,filenam);
  if(XY==TRUE)
  { if(xx==INVALIDINT) xx=TXTreadOptionalShort(TXT);
    if(yy==INVALIDINT) yy=TXTreadOptionalShort(TXT);
  }
  *repeat= TXTreadOptionalRepeat(TXT);
  *x=xx;
  *y=yy;
  for(comment=FALSE;;)
  { if(TXTgetc(TXT,&c,&val)!=TRUE)break;
    if(val & NEWLINE) break;
    if(val & COMMENT)        /*eat commentaries*/
       { comment=TRUE;  continue;}
    if(val & SPACE)          /*eat space*/
       { continue;}
    if(comment==FALSE)
       { ProgError("Line %d: bad entry format",TXT->Lines);}
  }
  return TRUE;
}








/*
** For any Writing of text files
*/
struct TXTFILE *TXTopenW(const char *file) /*open, and init if needed*/
{ struct TXTFILE *TXT;
  /*characters*/
  if(TXTok!=TRUE)  TXTinit();
  TXT = (struct TXTFILE *)Malloc(sizeof(struct TXTFILE));
  /*some inits */
  TXT->Lines	   =1;/*start in line 1*/
  TXT->SectionStart=0;
  TXT->SectionEnd  =0;
  TXT->fp 	   = fopen(file,FOPEN_RT);
  if(TXT->fp==NULL)
  { TXT->fp 	   = fopen(file,FOPEN_WT);
  }
  else
  { fclose(TXT->fp);
    TXT->fp 	   = fopen(file,FOPEN_AT);
    Warning("Appending to file %s",file);
  }
  if(TXT->fp==NULL)  ProgError("Could not write file %s",file);
  return TXT;
}

void TXTcloseW(struct TXTFILE *TXT)
{
  if (TXT == &TXTdummy)
     return;
  if(TXTok!=TRUE) Bug("TxtClo");
  fclose(TXT->fp);
  Free(TXT);
}

/*
** To write entries
*/
void TXTaddSection(struct TXTFILE *TXT,const char *def)
{ 
  if (TXT == &TXTdummy)
     return;
  if(TXTok!=TRUE) Bug("TxtAdS");
  fprintf(TXT->fp,"[%.8s]\n",def);
}

void TXTaddEntry(struct TXTFILE *TXT,const char *name,const char *filenam,Int16 x,Int16 y,Bool repeat, Bool XY)
{ 
  if (TXT == &TXTdummy)
     return;
  if(TXTok!=TRUE) Bug("TxtAdE");
  fprintf(TXT->fp,"%.8s",name);
/* fprintf(TXT->fp,"%.8s=",name);*/
  if(filenam!=NULL)
    fprintf(TXT->fp,"\t%.8s",filenam);
  if(XY==TRUE)
    fprintf(TXT->fp,"\t%d\t%d",x,y);
  if(repeat==TRUE)
    fprintf(TXT->fp,"\t*");
  fprintf(TXT->fp,"\n");
}

void TXTaddComment(struct TXTFILE *TXT,const char *text)
{ 
  if (TXT == &TXTdummy)
     return;
   if(TXTok!=TRUE) Bug("TxtAdC");
  fprintf(TXT->fp,"# %.256s\n",text);
}

void TXTaddEmptyLine (struct TXTFILE *TXT)
{
  if (TXT == &TXTdummy)
     return;
  if (TXTok != TRUE)
    Bug ("TxtAdL");
  putc ('\n', TXT->fp);
}

#endif /*DeuTex*/

