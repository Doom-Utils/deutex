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


#include "deutex.h"
#include "tools.h"


/*compile only for DeuTex*/
#if defined DeuTex



/****** DOOM EXE hacking module *********************/
/*
Int16 const RAWSIZE   = 0x2000;
Int16 const STRMAX    = 256;
Int16 const SHOWPRGRS = 0x4000;

void EXE2list(FILE *out,char *doomexe,Int32 start,Int16 thres)
{ FILE *exe;
  char  *raw;	Int16   iraw,remains;
  char  *str;	Int16   istr,count;
  Int16 c;
  Int32 lastpos,posit;
  Int32 found=0;
  count=0;
  lastpos = posit=start;
  iraw=0;raw= (char  *)Malloc(RAWSIZE*sizeof(char));
  istr=0;str= (char  *)Malloc((STRMAX+2)*sizeof(char));
  exe=fopen(doomexe,FOPEN_RB);
  if(exe==NULL)ProgError("Can't open %s",doomexe);
  if(fseek(exe,posit,SEEK_SET))ProgError("Can't seek in %s",doomexe);

  TXTinit();
  for(iraw=0,remains=0;;remains--,iraw++)
  { if(remains==0)
      { remains=fread(raw,1,RAWSIZE,exe); iraw=0;}
    if(remains<=0) break;
    c=raw[iraw];
    posit++;
    switch(TXTvalid(c))
    { case 1:
       count++;
       str[istr]=c;
       istr=(count<STRMAX)? count: STRMAX-1;
       break;
      case 2:
       str[istr]='\0'; 
       if(count>=thres)
       { fprintf(out,"let: #%s#\n be: #%s#\n\n",str,str);
         found++;
       }
       if(posit>lastpos+SHOWPRGRS)
       { lastpos=posit;
	 Phase("Current offset:0x%-6lx\tfound %ld strings\n",posit,found);
       }
      default:
       count=0;
       istr=0;
     }
#ifdef WinDeuTex
    windoze(); 
#endif
  }
  Free(raw);
  Free(str);
}

void EXEsubstit(char *texin,char *doomexe,Int32 start,Int16 thres)
{ FILE *exe;
  char  *raw;	Int16   iraw,remains;
  char  *str;	Int16   strln,istr;
  char  *anew;      Int16   anewln;
  Int32 found;
  Int16 c; Int16 ref;
  Int32 lastpos,posit;
  Bool similar,same;
  struct TXTFILE *TXT;
  TXT=TXTopen(texin);

  iraw=0;raw= (char  *)Malloc(RAWSIZE*sizeof(char));
  istr=0;str= (char  *)Malloc((STRMAX+2)*sizeof(char));
  anew = (char  *)Malloc((STRMAX+2)*sizeof(char));
  exe=fopen(doomexe,FOPEN_RBP); 
  if(exe==NULL)
         ProgError("Can't open %s for writing",doomexe);

  while(1)
  {
    strln=STRMAX;
    anewln=STRMAX;
    if(TXTboundStrings(TXT,str,&strln,anew,&anewln)!=TRUE) break;
    if(strln<thres)   ProgError("String too small");
    for(istr=anewln;istr<strln;istr++) anew[istr]=' ';
    anew[strln]=str[strln]='\0';
#ifdef DEBUG
    Detail("substit\t#%s# \nby\t#%s#\n",str,anew);
#endif
    lastpos = posit = start;
    if(fseek(exe,posit,SEEK_SET))
       ProgError("Can't seek in %s",doomexe);
    found=-1;
    same=similar=FALSE;istr=0;ref=str[istr];
    for(iraw=remains=0;;remains--,iraw++,posit++)
    { if(remains==0){ remains=fread(raw,1,RAWSIZE,exe); iraw=0;}
      if(remains<=0)break;
      c=raw[iraw];
      if(c==ref)
      {  if(istr==0)     {similar=TRUE;lastpos=posit;}
	 if(similar==TRUE){istr++;ref=str[istr];}
	 if(istr==strln) {same=TRUE;ref=-1;}
      }
      else
      { if(same==TRUE)
        { if((c=='\0')||(c=='\n'))
	  { Detail("Found at\t0x%-6lx\t#%s#\n",lastpos,str);
	    if(found>=0)
	    { Warning("String defined twice: #%s#",str);
	      found=0;break;
	    }
	    found=lastpos;
	  }
        }
	same=similar=FALSE;istr=0;ref=str[istr];
      }
#ifdef WinDeuTex
    windoze();   
#endif
    }
    
    if(found>0)
    { Detail("Writing at\t0x%-6lx\t#%s#\n",found,anew);
      if(fseek(exe,found,SEEK_SET)) ProgError("Can't seek %s",doomexe);
      if(fwrite(anew,1,strln,exe)!=strln) ProgError("Can't write %s correctly",doomexe);
    }
    else
    Warning("Can't insert #%s#",anew);
  }
  fclose(exe);
  Free(raw);
  Free(str);
  Free(anew);
  TXTclose(TXT);
}
*/
#endif /*DeuTex*/

