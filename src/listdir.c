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
#include "endianm.h"
#include "mkwad.h"
#include "texture.h"
#include "ident.h"

extern char file[128];
/*
** list WAD directory and identify entries
*/
static char *Views[] = {"?","All  ",
        "Front","FrRgt","Right","RrRgt",
        "Rear ","RrLft","Left ","FrLft"};
static char *IdentView(char view)
{ switch(view)
  { case '0':
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8':
    return Views[1+view-'0'];
  }
  return Views[0];
}

void XTRlistDir(const char *doomwad, const char *wadin, NTRYB select)
{  Int16 n;
   static struct WADINFO pwad;
   static struct WADINFO iwad;
   static char buffer[128];
   ENTRY  *iden;
   ENTRY type;
   Int32 ntry;
   struct WADDIR  *dir;
   char *typ;
   Int16 pnm;char  *Pnam;Int32 Pnamsz=0;

   /*open iwad,get iwad directory*/
   iwad.ok=0;
   WADRopenR(&iwad,doomwad);
   /*find PNAMES*/
   pnm=WADRfindEntry(&iwad,"PNAMES");
   if(pnm<0) ProgError("Can't find PNAMES in main WAD");
   Pnam=WADRreadEntry(&iwad,pnm,&Pnamsz);
   WADRclose(&iwad);
   /**/
   if(wadin!=NULL)
   { pwad.ok=0;
     WADRopenR(&pwad,wadin);
     iden=IDENTentriesPWAD(&pwad, Pnam, Pnamsz);
     ntry= pwad.ntry;
     dir=  pwad.dir;
   }
   else
   { iwad.ok=0;
     WADRopenR(&iwad,doomwad);
     iden=IDENTentriesIWAD(&iwad, Pnam, Pnamsz,FALSE);
     ntry= iwad.ntry;
     dir=  iwad.dir;
   }
   /**/
        Free(Pnam);
        /**/
        Output("\nListing of WAD directory for %s\n\n",wadin);
        Output("Entry\t      Size\tType\n\n");
        for(n=0;n<ntry;n++)
        { type=iden[n];
          typ="unknown";
          /*Don't list unwanted entries*/
          switch(type&EMASK)
          { case EVOID:
                 case EDATA:
                        if((select&BALL)!=BALL)continue;
                        break;   /*show VOID? only if all entries*/
                 case ELEVEL:  if(!(select&BLEVEL))  continue;
                        break;
                 case EMAP:    if(!(select&BLEVEL))  continue;
                        break;
                 case ETEXTUR: if(!(select&BTEXTUR)) continue;
                        break;
                 case EPNAME:  if(!(select&BTEXTUR)) continue;
                        break;
                 case ESOUND:  if(!(select&BSOUND))  continue;
                        break;
                 case EGRAPHIC:if(!(select&BGRAPHIC))continue;
                        break;
                 case ELUMP:   if(!(select&BLUMP))   continue;
                        break;
                 case ESPRITE: if(!(select&BSPRITE)) continue;
                        break;
                 case EPATCH:  if(!(select&BPATCH))  continue;
                        break;
                 case EFLAT:   if(!(select&BFLAT))   continue;
                        break;
                 case EMUSIC:  if(!(select&BMUSIC))  continue;
                }
                switch(type&EMASK)
                { case EVOID:
                  case EDATA:
                         typ="."; break;
                  case ELEVEL:
			 sprintf(buffer, "Episod %c Map %c",
			     '0'+((type&0xF0)>>4),'0'+(type&0xF));
			 typ=buffer; break;
		  case EMAP:
			 sprintf(buffer,"Level Map %d",(type&0xFF));
                         typ=buffer; break;
                  case ETEXTUR:        typ="List of textures"; break;
                  case EPNAME:        typ="List of wall patches"; break;
                  case ESOUND:    typ="Sound";break;
                  case EMUSIC:    typ="Music";break;
                  case EGRAPHIC:
                         if((type&0xFF)==0xFF)
                         { typ="Graphic";
                         }
                         else
                         { sprintf(buffer,"Graphic");
                                typ=buffer;
                         }
                         break;
                  case ELUMP:        typ="Lump of raw data"; break;
                  case ESPRITE:        typ="Sprite";
                         if(strncmp(dir[n].name,"ARTI",4)==0)
                         { sprintf(buffer,"Artifact\t%4.4s",&(dir[n].name[4]));
                         }
                         else if(dir[n].name[6]!='\0')
                         { sprintf(buffer,"Sprite %4.4s\tph:%c %s\tph:%c %s inv.",
                                dir[n].name, dir[n].name[4],
				IdentView(dir[n].name[5]), dir[n].name[6],
				IdentView(dir[n].name[7]));
                         }
                         else
                         { sprintf(buffer,"Sprite %4.4s\tph:%c %s",
                                dir[n].name, dir[n].name[4],
				IdentView(dir[n].name[5]));
                         }
                         typ = buffer; break;
                  case EPATCH:        typ="Wall patch"; break;
                  case EFLAT:        typ="Flat";break;
                  case EZZZZ:     typ="?";break;
                }
                switch(type)
                {
                  case EPATCH1:        typ="Wall patch 1"; break;
                  case EPATCH2:        typ="Wall patch 2"; break;
                  case EPATCH3:        typ="Wall patch 3"; break;
                  case EFLAT1:           typ="Flat 1"; break;
                  case EFLAT2:           typ="Flat 2"; break;
                  case EFLAT3:           typ="Flat 3"; break;
                  case ESNDPC:    typ = "PC sound"; break;
                  case ESNDWAV:   typ = "WAV sound"; break;
                }
                Output("%-8s  %8ld\t%s\n",
		    lump_name (dir[n].name), dir[n].size, typ);
        }
        if(wadin!=NULL)
          WADRclose(&pwad);
        else
          WADRclose(&iwad);
        Free(iden);
}


#if defined(DeuTex)


#if DT_OS == 'o'
#  if DT_CC = 'b'
int _USERENTRY XTRdirCmp(const void *d1,const void *d2)
#  else
int _Optlink XTRdirCmp(const void *d1,const void *d2)
#  endif
#else
int XTRdirCmp(const void *d1,const void *d2)
#endif
{  Int32  res;
	struct WADDIR *dir1=(struct WADDIR *)d1;
	struct WADDIR *dir2=(struct WADDIR *)d2;
	res= (dir1->start)-(dir2->start);
	if(res<0)return -1;
	if(res>0)return 1;
	res = (dir1->size)-(dir2->size);
	if(res<0)return -1;
	if(res>0)return 1;
	return 0;
}

void XTRvoidSpacesInWAD(const char *wadin)
{  Int16 n;
	static struct WADINFO pwad;
	Int32 ntry;
	struct WADDIR  *dir;
	Int32 startpos,lastpos,ll,diff,wtotal;
	Int32 w3,w20,w100,w1000,w10000,w100000;
	wtotal=w3=w20=w100=w1000=w10000=w100000=0;

	pwad.ok=0;
	WADRopenR(&pwad,wadin);
	ntry= pwad.ntry;
	dir=  pwad.dir;
	qsort(dir,(size_t)ntry,sizeof(struct WADDIR),XTRdirCmp);
	Output("\nListing of unused spaces in WAD %s\n",wadin);
	lastpos=4+4+4;
	for(n=0;n<ntry;n++)
	{  diff=dir[n].start-lastpos;
		startpos=lastpos;
		ll=dir[n].start+dir[n].size;
		if(lastpos<ll) lastpos=ll;
		if(diff<0)continue;
		wtotal+=diff;
		if(diff<=3)
		w3+=diff;
		else if(diff<=20)
		w20+=diff;
		else if(diff<=100)
		w100+=diff;
		else if(diff<=1000)
		w1000+=diff;
		else if(diff<=10000)
		w10000+=diff;
		else
		w100000+=diff;
		if(diff>=4) /*suppress word alignement*/
		 Output("  At offset 0x%8.8lx, %ld bytes wasted\n",startpos,diff);
	}
	Output("\nRepartition of wasted bytes:\n");
	Output(" All blocks<=3    \t%ld\n",w3);
	Output(" All blocks<=20   \t%ld\n",w20);
	Output(" All blocks<=100  \t%ld\n",w100);
	Output(" All blocks<=1000 \t%ld\n",w1000);
	Output(" All blocks<=10000\t%ld\n",w10000);
	Output(" All blocks> 10000\t%ld\n",w100000);
	Output("                  \t-------\n");
	Output(" Total wasted bytes\t%ld\n",wtotal);
	WADRclose(&pwad);
}
#endif


struct SIDEDEF
{  Int16 ofsx;      /* X offset for texture */
	Int16 ofsy;      /* Y offset for texture */
	char  Above[8];  /* texture name for the part above */
	char  Below[8];  /* texture name for the part below */
   char  Center[8]; /* texture name for the regular part */
	Int16 Sector;    /* adjacent sector */
};

/*
** Check a level
** Assumes TEXTURES are already read somewhere.
*/                            
void CheckTexture(char  *tex,Bool IsDef)
{ int n;
  char Name[8];
  for(n=0;n<8;n++)
  { Name[n]=tex[n];
    if(tex[n]=='\0')break;
  }
  Normalise(Name,Name);
  switch(Name[0])
  { case '-': case '\0': break;
    default:
    if(IsDef==TRUE) /*if we only wish to declare the tex*/
    { TXUfakeTex(Name);
    }
    else
    { if(TXUexist(Name)==FALSE)
        Output("Warning: undefined sidedef %s\n", lump_name (Name));
    }
  }
}

/*
**check textures
** IsDef=TRUE if we just wish to declare textures
*/
void CheckSideDefs(struct WADINFO *pwad,Int32 start,Int32 size,Bool IsDef)
{ struct SIDEDEF  *sid;
  struct SIDEDEF  *side;
  Int32 s;
  sid = (struct SIDEDEF  *)Malloc(size);
  WADRseek(pwad,start);
  WADRreadBytes(pwad,(char  *)sid,size);
  for(s=0;(s*sizeof(struct SIDEDEF))<size;s+=1)
  { side=sid+s;
    CheckTexture(side->Above,IsDef);
    CheckTexture(side->Below,IsDef);
    CheckTexture(side->Center,IsDef);
  }
  Free(sid);
}

void CheckLevels(struct WADINFO *pwad, Bool IsDef)
{ Int16 lev,lin,id,top;
  Int32 ntry=pwad->ntry;
  struct WADDIR  *pdir=pwad->dir;
  for(lev=0;lev<ntry;lev++)
  { id=IDENTlevel(pdir[lev].name);
	 if(id>=0)
    { for(lin=lev;lin<ntry;lin++)
      { top=lev+11;
        if(lin>top) break;
        if(strncmp(pdir[lin].name,"SIDEDEFS",8)==0)
        { Output("\nChecking LEVEL %s\n\n", lump_name (pdir[lev].name));
          CheckSideDefs(pwad,pdir[lin].start,pdir[lin].size,IsDef);
        }
      }
    }
  }
}

/*
** Test a PWAD
** 
** this is absolutely sub optimal. 
*/
void XTRstructureTest(const char *doomwad,const char *wadin)
{ static struct WADINFO pwad,iwad;
  char  *Pnames;
  Int16 p,pnm,nbPatchs;
  Int32 size;
  static struct PICH{Int16 Xsz;Int16 Ysz;} pich;
  Int16  *PszX;
  static char name[8];
  char  *buffer;
  Int16 cs,ce;

  /*read PNAME in wad, if defined*/
  Phase("Reading WADs\n");
  iwad.ok=0;
  WADRopenR(&iwad,doomwad);
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  Phase("Reading Patches\n");
  pnm=WADRfindEntry(&pwad,"PNAMES");
  if(pnm>=0)
  { size=pwad.dir[pnm].size;
    Pnames=(char  *)Malloc(size);
	 WADRseek(&pwad,pwad.dir[pnm].start);
    WADRreadBytes(&pwad,Pnames,size);
  }
  else
  { pnm=WADRfindEntry(&iwad,"PNAMES");
    if(pnm<0)ProgError("PNAMES not found");
    size=iwad.dir[pnm].size;
    Pnames=(char  *)Malloc(size);
    WADRseek(&iwad,iwad.dir[pnm].start);
    WADRreadBytes(&iwad,Pnames,size);
  }
  PNMinit(Pnames,size);
  Free(Pnames);
  /*
  ** find each PNAME and identify Xsz Ysz
  */
  Phase("Checking Patches\n");
  nbPatchs=PNMgetNbOfPatch();
  PszX=(Int16  *)Malloc(nbPatchs*sizeof(Int16));
  for(p=0;p<nbPatchs;p++)                /*for all patches*/
  { PNMgetPatchName(name,p);
    pnm=WADRfindEntry(&pwad,name);
    if(pnm>=0)
    {  WADRseek(&pwad,pwad.dir[pnm].start);
       WADRreadBytes(&pwad,(char  *)&pich,sizeof(struct PICH));
    }
	 else
    { pnm=WADRfindEntry(&iwad,name);
      if(pnm<0)
        Output("Warning: Patch %s not found\n", lump_name (name));
      else
      { WADRseek(&iwad,iwad.dir[pnm].start);
        WADRreadBytes(&iwad,(char  *)&pich,sizeof(struct PICH));
      }
    }
    PszX[p] = peek_i16_le (&pich.Xsz);
  }
  /*
  ** read TEXTURES
  */
  Phase("Reading Textures\n");
  pnm=WADRfindEntry(&pwad,"TEXTURE1");
  if(pnm>=0)
  { /* read texture */
    buffer=(char  *)Malloc(pwad.dir[pnm].size);
    WADRseek(&pwad,pwad.dir[pnm].start);
    WADRreadBytes(&pwad,buffer,pwad.dir[pnm].size);
    TXUinit();
        TXUreadTEXTURE(buffer,pwad.dir[pnm].size,NULL,0,TRUE);
    Free(buffer);
    /*for each textures, check what is covered */
	 Phase("Checking TEXTURE1\n");
    TXUcheckTex(nbPatchs,PszX);
    TXUfree();
  }
  pnm=WADRfindEntry(&pwad,"TEXTURE2");
  if(pnm>=0)
  { /* read texture */
    buffer=(char  *)Malloc(pwad.dir[pnm].size);
    WADRseek(&pwad,pwad.dir[pnm].start);
    WADRreadBytes(&pwad,buffer,pwad.dir[pnm].size);
    TXUinit();
        TXUreadTEXTURE(buffer,pwad.dir[pnm].size,NULL,0,TRUE);
    Free(buffer);
	 /*for each textures, check what is covered */
    Phase("Checking TEXTURE2\n");
    TXUcheckTex(nbPatchs,PszX);
    TXUfree();
  }
  Free(PszX);
  /*
  ** check if all textures composing walls are here
  **
  */
  Phase("Checking Level SIDEDEFS for missing textures\n");
  TXUinit();
  pnm=WADRfindEntry(&pwad,"TEXTURE1");
  if(pnm>=0)
  {  buffer=(char  *)Malloc(pwad.dir[pnm].size);
     WADRseek(&pwad,pwad.dir[pnm].start);
     WADRreadBytes(&pwad,buffer,pwad.dir[pnm].size);
         TXUreadTEXTURE(buffer,pwad.dir[pnm].size,NULL,0,TRUE);
     Free(buffer);
  }
  else
  {  pnm = WADRfindEntry(&iwad,"TEXTURE1");
     if(pnm>=0)
     { buffer=(char  *)Malloc(iwad.dir[pnm].size);
       WADRseek(&iwad,iwad.dir[pnm].start);
       WADRreadBytes(&iwad,buffer,iwad.dir[pnm].size);
			  TXUreadTEXTURE(buffer,iwad.dir[pnm].size,NULL,0,TRUE);
       Free(buffer);
     }
  }
  pnm=WADRfindEntry(&pwad,"TEXTURE2");
  if(pnm>=0)
  {  buffer=(char  *)Malloc(pwad.dir[pnm].size);
     WADRseek(&pwad,pwad.dir[pnm].start);
     WADRreadBytes(&pwad,buffer,pwad.dir[pnm].size);
         TXUreadTEXTURE(buffer,pwad.dir[pnm].size,NULL,0,TRUE);
     Free(buffer);
  }
  else
  {  pnm = WADRfindEntry(&iwad,"TEXTURE2");
     if(pnm>=0)
     { buffer=(char  *)Malloc(iwad.dir[pnm].size);
       WADRseek(&iwad,iwad.dir[pnm].start);
       WADRreadBytes(&iwad,buffer,iwad.dir[pnm].size);
       TXUreadTEXTURE(buffer,iwad.dir[pnm].size,NULL,0,TRUE);
       Free(buffer);
     }
  }
  CheckLevels(&pwad,FALSE);
  TXUfree();
  PNMfree();
  WADRclose(&iwad);
  /*
  ** check sprite markers
  **
  */
  Phase("Checking Sprites\n");
  for(cs=ce=0,p=0;p<pwad.ntry;p++)
  { if(strncmp(pwad.dir[p].name,"S_START",8)==0) cs++;
    if(strncmp(pwad.dir[p].name,"SS_START",8)==0) cs++;
    if(strncmp(pwad.dir[p].name,"SS_END",8)==0) ce++;
    if(strncmp(pwad.dir[p].name,"S_END",8)==0) ce++;
  }
  if(cs>1) ProgError("Duplicate S_START");
  if(ce>1) ProgError("Duplicate S_END");
  if((cs>0)&(ce<1)) Output("Warning: S_START but no S_END\n");
  if((cs<1)&(ce<1))
    Info("No need to check SPRITES.\n");
  else
  { for(cs=ce=0,p=0;p<pwad.ntry;p++)
    { if(strncmp(pwad.dir[p].name,"S_START",8)==0) cs=p+1;
      if(strncmp(pwad.dir[p].name,"SS_START",8)==0) cs=p+1;
      if(strncmp(pwad.dir[p].name,"SS_END",8)==0) ce=p;
      if(strncmp(pwad.dir[p].name,"S_END",8)==0) ce=p;
    }
  }
  /*
  ** Check flat markers
  */
  Phase("Checking Flats\n");
  for(cs=ce=0,p=0;p<pwad.ntry;p++)
  { if(strncmp(pwad.dir[p].name,"F_START",8)==0) cs++;
    if(strncmp(pwad.dir[p].name,"FF_START",8)==0) cs++;
    if(strncmp(pwad.dir[p].name,"FF_END",8)==0) ce++;
    if(strncmp(pwad.dir[p].name,"F_END",8)==0) ce++;
  }
  if(cs>1) Output("Warning: Duplicate F_START\n");
  if(ce>1) Output("Warning: Duplicate F_END\n");
  if((cs>0)&(ce<1)) Output("Warning: F_START but no F_END\n");
  if((cs<1)&(ce<1))Info("No need to check FLATS.\n");
  WADRclose(&pwad);
}




#if defined(DeuTex)
/*
** Test a PWAD
** 
*/
void XTRtextureUsed(const char *wadin)
{ static struct WADINFO pwad;
  /*read PNAME in wad, if defined*/
  Phase("listing texture used in the levels of %s\n",wadin);
  pwad.ok=0;
  WADRopenR(&pwad,wadin);
  /*
  ** list all textures composing walls
  */
  TXUinit();
  CheckLevels(&pwad,TRUE);
  Output("List of textures used in %s\n\n",wadin);
  TXUlistTex();
  TXUfree();
  WADRclose(&pwad);
}
#endif


#if defined(DeuTex)

/*
** Detect duplicate entries
** ShowIdx = TRUE if we also output the indexes
**
** Optimise for speed with a CRC-based check
*/
void XTRcompakWAD(const char *DataDir,const char *wadin, const char *texout,Bool ShowIdx)
{  static struct WADINFO pwad;
   struct WADDIR  *pdir;
   Int16 pnb;
   Int16 p,bas,tst,ofsx,ofsy;
   Int32 size,rsize,sz;
   Bool  *psame;
   FILE *out;
   char  *bbas;
   char  *btst;
   Phase("Detecting duplicate entries in WAD %s\n",wadin);
   pwad.ok=0;
   WADRopenR(&pwad,wadin);
   pnb=(Int16)pwad.ntry;
   pdir=pwad.dir;
   psame=(Bool  *)Malloc(pnb*sizeof(Bool));
   for(p=0;p<pnb;p++)
   { psame[p]=FALSE;}
   if(texout==NULL)
     MakeFileName(file,DataDir,"","","WADINFOP","TXT");
   else
     sprintf(file,"%.120s",texout);
   out=fopen(file,FOPEN_WT);
   fprintf(out,"; Indication of similar entries\n\n");
   bbas=(char  *)Malloc(MEMORYCACHE);
   btst=(char  *)Malloc(MEMORYCACHE);
   for(bas=0;bas<pnb;bas++)
     if(psame[bas]==FALSE) /*skip already treated*/
       { size=pdir[bas].size;
         if(pdir[bas].start<=8)continue;
         if(size<1)continue;
         if((size>=8)&&(ShowIdx==TRUE))
         { WADRseek(&pwad,pdir[bas].start);
           WADRreadBytes(&pwad,bbas,8);
           ofsx=((bbas[5]<<8)&0xFF00)+(bbas[4]&0xFF);
           ofsy=((bbas[7]<<8)&0xFF00)+(bbas[6]&0xFF);
           fprintf(out,"%-8.8s\t%d\t%d\n",pdir[bas].name,ofsx,ofsy);
         }
         else
           fprintf(out,"%-8.8s\n",pdir[bas].name);
         for(tst=bas+1;tst<pnb;tst++)
         { /*if same size*/
           if(pdir[tst].start<0)continue;
           if(pdir[tst].size!=size)continue;
           /*check that equal*/
           for(sz=rsize=0;rsize<size;rsize+=sz)
           { sz = (size-rsize>MEMORYCACHE)? MEMORYCACHE : size-rsize;
             WADRseek(&pwad,pdir[bas].start+rsize);
             WADRreadBytes(&pwad,bbas,sz);
             WADRseek(&pwad,pdir[tst].start+rsize);
             WADRreadBytes(&pwad,btst,sz);
             for(p=0;p<sz;p++)
             { if(bbas[p]!=btst[p]) break;
             }
             if(p<sz)break;
           }
           if(rsize==size) /*entry identical to reference*/
              { psame[tst]=TRUE;
             fprintf(out,"%-8.8s\t*\n",pdir[tst].name);
           }
         }
       }
   fclose(out);
   WADRclose(&pwad);
   Free(bbas);
   Free(btst);
   Free(psame);
}
#endif /*DeuTex*/

