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
  This codes needs a serious cleaning now that it's stable!
  Too many parts are non optimised.
*/

#include "deutex.h"
#include "tools.h"
#include "mkwad.h" /*for find entry*/
#include "lists.h"

#define X_NONE 0
#define X_START 2
#define X_END 1
/********************** begin LIST module **************/
/*
** list of distinct entries types
*/
struct ELIST
{ Int16 Top;     /*num of entries allocated for list*/
  Int16 Pos;     /*current top position*/
  struct WADDIR  *Lst;
};
static struct ELIST LISlmp;
static struct ELIST LISspr;
static struct ELIST LISpat;
static struct ELIST LISflt;
        
/*
** init
*/
static void LISinitLists(void)
{  /* GRAPHIC, LEVEL, LUMP */
   LISlmp.Top=1;LISlmp.Pos=0;LISlmp.Lst=NULL;
   LISspr.Top=1;LISspr.Pos=0;LISspr.Lst=NULL;
   LISpat.Top=1;LISpat.Pos=0;LISpat.Lst=NULL;
   LISflt.Top=1;LISflt.Pos=0;LISflt.Lst=NULL;
}
/*
** count the number of entry types
*/
static void LIScountTypes(ENTRY  *ids,Int16 nb)
{  Int16 i;
	for(i=0;i<nb;i++)
	{  switch(ids[i]&EMASK)
		{ case EPNAME:  case ETEXTUR:
		  case EMAP:    case ELEVEL:
		  case ELUMP:   case EGRAPHIC:
		  case EMUSIC:  case ESOUND:
		  case EDATA:
			 LISlmp.Top++;        break;
		  case ESPRITE:
			 LISspr.Top++;        break;
		  case EPATCH:
			 LISpat.Top++;        break;
		  case EFLAT:
			 LISflt.Top++;        break;
		  case EZZZZ:        case EVOID:
			 break;
		  default: Bug("LisUkn");
		}
	}
}
static void LISallocLists(void)
{  /*allocate memory for the lists*/
	LISlmp.Lst =(struct WADDIR  *)Malloc(LISlmp.Top*sizeof(struct WADDIR));
	LISspr.Lst =(struct WADDIR  *)Malloc(LISspr.Top*sizeof(struct WADDIR));
	LISpat.Lst =(struct WADDIR  *)Malloc(LISpat.Top*sizeof(struct WADDIR));
	LISflt.Lst =(struct WADDIR  *)Malloc(LISflt.Top*sizeof(struct WADDIR));
}
static void LISfreeLists(void)
{  Free(LISflt.Lst);
	Free(LISpat.Lst);
	Free(LISspr.Lst);
	Free(LISlmp.Lst);
}
/*
** Delete unwanted sprites
**
*/
static Bool LISunwantedPhase(char pv[2],char phase, char view,Bool AllViews)
{ if(pv[0]!=phase) return FALSE;
  if(pv[1]==view) return TRUE;   /*delete phase if equal*/
  switch(pv[1])
  { /*view 0 unwanted if any other view exist*/
	 case '0':
	 return (AllViews==TRUE)? TRUE:FALSE;
	 /*view unwanted if view 0 exist*/
	 case '1': case '2':case '3':case '4':
	 case '5': case '6':case '7':case '8':
	 return (AllViews==TRUE)? FALSE:TRUE;
  }
  return FALSE;
}
static Bool LISdeleteSprite(char root[8],char phase,char view)
{
  Int16 l,sz;
  Bool Okay=FALSE;
  Bool AllViews;
  switch(view)
  { case '0':
		 AllViews=FALSE;break;
	 case '1': case '2':case '3':case '4':
	 case '5': case '6':case '7':case '8':
		 AllViews=TRUE;break;
	 case '\0':
	 default:   /*Artifacts*/
	  return FALSE;
  }
  /*root: only the 4 first char are to be tested*/
  for(l=LISspr.Pos-1;l>=0;l--)
  {
	 if(strncmp(LISspr.Lst[l].name,root,4)==0)
	 { /*2nd unwanted: replace by 0*/
		if(LISunwantedPhase(&(LISspr.Lst[l].name[6]),phase,view,AllViews)==TRUE)
		{ LISspr.Lst[l].name[6]='\0';
		  LISspr.Lst[l].name[7]='\0';
		  Okay=TRUE;
		}
		/*1st unwanted: replace by second*/
		if(LISunwantedPhase(&(LISspr.Lst[l].name[4]),phase,view,AllViews)==TRUE)
		{ LISspr.Lst[l].name[4]=LISspr.Lst[l].name[6];
		  LISspr.Lst[l].name[5]=LISspr.Lst[l].name[7];
		  LISspr.Lst[l].name[6]='\0';
		  LISspr.Lst[l].name[7]='\0';
		  Okay=TRUE;
		}
		/*neither 1st nor second wanted: delete*/
		if(LISspr.Lst[l].name[4]=='\0')
		{ sz=LISspr.Pos-(l+1);
		  if(sz>0)
			 Memcpy(&(LISspr.Lst[l]),&(LISspr.Lst[l+1]),(sz)*sizeof(struct WADDIR));
		  LISspr.Pos--;
		  Okay=TRUE;
		}
	 }
  }
  return Okay;
}
/*
** list management. very basic. no hash table.
*/
static void LISaddMission(struct WADDIR  *dir, Int16 found)
{
	Int16 l=0,dummy=0;
	/*check*/
	if(LISlmp.Pos>=LISlmp.Top) Bug("LisTsm"); /*level list too small*/
	/*try to locate mission entry at l*/
	for(l=0;l<LISlmp.Pos;l++)
	{ if(strncmp(LISlmp.Lst[l].name,dir[0].name,8)==0) break; /*l<pos*/
	}
	/*update pos*/
	dummy=l+found;/*bug of GCC*/
	if(dummy>LISlmp.Pos) LISlmp.Pos=dummy;
	if(LISlmp.Pos>LISlmp.Top) Bug("LisTsm"); /*level list too small*/
	/*copy new level, or replace existing*/
	Memcpy((&(LISlmp.Lst[l])),(dir),found*sizeof(struct WADDIR));
}

static void LISadd(struct ELIST *L,struct WADDIR  *dir)
{   /*check current position*/
	 if((L->Pos)>=L->Top) Bug("LisSml"); /*list count too small*/
	 /* ADD in List */
	 Memcpy(&(L->Lst[(L->Pos)]),(dir),sizeof(struct WADDIR));
	 (L->Pos)++;  /*increase pointer in list*/
	 return;
}
/*
** Find in list, and replace if exist
**
*/
static Bool LISfindRplc(struct ELIST *L,struct WADDIR  *dir)
{ Int16 l;
  for(l=0;l<(L->Pos);l++)
	 if(strncmp(L->Lst[l].name,dir[0].name,8)==0)
	 { Memcpy(&(L->Lst[l]),&(dir[0]),sizeof(struct WADDIR));
		return TRUE;
	 }
  return FALSE;  /*not found*/
}
 /* Substitute In List
** if entry exist, it is substituted, else added at the end of list
** suitable for LUMPS and SPRITES
** but generally those shall not be added: they may not be recognized
*/
static void LISsubstit(struct ELIST *L,struct WADDIR  *dir)
{ /* SUBSTIT in List */
  if(LISfindRplc(L,dir)==TRUE) return;
  Warning("entry %s might be ignored by DOOM.", lump_name (dir[0].name));
  LISadd(L,dir);
}
/* Move In List
** if entry already exist, destroy it, and put at the end
** suitable for PATCH and FLATS  (to define new animations)
*/
static void LISmove(struct ELIST *L,struct WADDIR  *dir)
{ Int16 l,sz;
  for(l=0;l<(L->Pos);l++)
	 if(strncmp(L->Lst[l].name,dir[0].name,8)==0)
	 { /*entry already exist. destroy it*/
		sz=(L->Pos)-(l+1);
		if(sz>0) Memcpy(&(L->Lst[l]),&(L->Lst[l+1]),(sz)*sizeof(struct WADDIR));
		(L->Pos)--;
	 }
  LISadd(L,dir);
}
	/* Put Sprites In List
	 ** if entry already exist, replace it
	 ** if entry does not exist, check if it does not obsolete
	 ** an IWAD sprite entry
	 ** suitable for SPRITES only
	 */ /***/
static void LISaddSprite(struct WADDIR  *dir,Bool Warn)
{ Bool Okay=FALSE;
  /*warn:  FALSE= no warning. TRUE = warn.*/
  /* If entry already exists, replace it*/
  if(LISfindRplc(&LISspr,dir)==TRUE) return;
  /* Entry does not exist. Check that this sprite
  ** viewpoint doesn't obsolete other sprite viewpoints.
  */
  if(dir[0].name[4]!='\0')
  { Okay|=LISdeleteSprite(dir[0].name,dir[0].name[4],dir[0].name[5]);
	 if(dir[0].name[6]!='\0')
	 Okay|=LISdeleteSprite(dir[0].name,dir[0].name[6],dir[0].name[7]);
  }
  if((Okay==FALSE)&&(Warn==TRUE))
  { Warning("entry %s might be ignored by DOOM.", lump_name (dir[0].name));
  }
  LISadd(&LISspr,dir);
}
/*
** create a new directory list
**
*/
static void LISmakeNewDir(struct WADINFO *nwad,Int16 S_END,Int16 P_END,Int16 F_END,Int16 Pn,Int16 Fn)
{  Int16 n;
	struct WADDIR  *dir;
	/*levels,lumps, graphics into new dir*/
	if(LISlmp.Pos>0)
	{ for(n=0;n<LISlmp.Pos;n++)
	  { dir=&LISlmp.Lst[n];
		 WADRdirAddPipo(nwad,dir->start,dir->size,dir->name);
	  }
	}
	/*sprites into new dir*/
	if(LISspr.Pos>0)
	{ WADRdirAddPipo(nwad,0,0,((S_END & X_START)? "S_START":"SS_START"));
	  for(n=0;n<LISspr.Pos;n++)
	  { dir=&LISspr.Lst[n];
		 WADRdirAddPipo(nwad,dir->start,dir->size,dir->name);
	  }
	  WADRdirAddPipo(nwad,0,0,((S_END & X_END)? "S_END":"SS_END"));
	}
	/*patch 1,2,3 into new dir*/
	if(LISpat.Pos>0)
	{ WADRdirAddPipo(nwad,0,0,((P_END & X_START)?"P_START":"PP_START"));
	  if((Pn>=1)&&(P_END & X_START))
		 WADRdirAddPipo(nwad,0,0,"P1_START");
	  for(n=0;n<LISpat.Pos;n++)
	  { dir=&LISpat.Lst[n];
		 WADRdirAddPipo(nwad,dir->start,dir->size,dir->name);
	  }
	  if((Pn>=1)&&(P_END & X_START))
	  { WADRdirAddPipo(nwad,0,0,"P1_END");
		 if(Pn>=2)
		 { WADRdirAddPipo(nwad,0,0,"P2_START");
			WADRdirAddPipo(nwad,0,0,"P2_END");
			if(Pn>=3)
			{ WADRdirAddPipo(nwad,0,0,"P3_START");
			  WADRdirAddPipo(nwad,0,0,"P3_END");
			}
		 }
	  }
	  WADRdirAddPipo(nwad,0,0,((P_END & X_END)?"P_END":"PP_END"));
	}
	/*flat 1,2,3 into new dir*/
	if(LISflt.Pos>0)
	{ WADRdirAddPipo(nwad,0,0,((F_END & X_START)?"F_START":"FF_START"));
	  if((Fn>=1)&&(F_END & X_START))
		 WADRdirAddPipo(nwad,0,0,"F1_START");
	  for(n=0;n<LISflt.Pos;n++)
	  { dir=&LISflt.Lst[n];
		 WADRdirAddPipo(nwad,dir->start,dir->size,dir->name);
	  }
	  if((Fn>=1)&&(F_END & X_START))
	  { WADRdirAddPipo(nwad,0,0,"F1_END");
		 if(Fn>=2)
		 { WADRdirAddPipo(nwad,0,0,"F2_START");
			WADRdirAddPipo(nwad,0,0,"F2_END");
			if(Fn>=3)
			{ WADRdirAddPipo(nwad,0,0,"F3_START");
			  WADRdirAddPipo(nwad,0,0,"F3_END");
         }
       }
     }
     WADRdirAddPipo(nwad,0,0,((F_END & X_END)?"F_END":"FF_END"));
	}
}




/*
** merge IWAD and PWAD directories
** This function is a good example of COMPLETELY INEFFICIENT CODING.
** but since it's so hard to have it work correctly,
** Optimising was out of question.
**
**
*/
struct WADDIR  *LISmergeDir(Int32 *pNtry,Bool Append,Bool Complain,NTRYB select,
		  struct WADINFO *iwad,ENTRY  *iiden,Int32 iwadflag,
		  struct WADINFO *pwad,ENTRY  *piden,Int32 pwadflag)
{
	struct WADDIR  *idir;
	struct WADDIR  *pdir;
	struct WADINFO nwad;
	Int16  inb,  pnb;
	Int16  i,p,found;
   struct WADINFO *refwad;
	ENTRY type;
	Int16 Pn=0;/*0=nothing 1=P1_ 2=P2_ 3=P3_*/
	Int16 Fn=0;/*0=nothing 1=F1_ 2=F2_ 3=F3_*/
	Bool NoSprite=FALSE; /*no sprites declared. seek in graphics*/
	Int16 S_END=X_NONE;
	Int16 F_END=X_NONE;
	Int16 P_END=X_NONE;
	/*
	** select Sprites markers  (tricky hack!)
	*/
	refwad = ((Append!=TRUE)||(select&BSPRITE))? iwad : pwad;
	if(WADRfindEntry(refwad,"S_END")>=0)/*full sprite list is here*/
	{ S_END|=X_END;
	}
	if(WADRfindEntry(refwad,"S_START")>=0)/*full sprite list is here*/
	{ S_END|=X_START;
	}
	if(!(select&BSPRITE))
	{ if(S_END & X_END) /*sprites already appended. keep it so.*/
	  { Complain=FALSE;
	  }
	}
	/*
	** select Patches markers  (tricky hack!)
	*/
	refwad = (Append!=TRUE)? iwad : pwad;
	if(WADRfindEntry(refwad,"P_END")>=0)/*full patch list is here*/
	{ P_END|=X_END;
	  if(WADRfindEntry(refwad,"P3_END")>=0) Pn=3;
	  else if(WADRfindEntry(refwad,"P2_END")>=0) Pn=2;
	  else if(WADRfindEntry(refwad,"P1_END")>=0) Pn=1;
	}
	if(WADRfindEntry(refwad,"P_START")>=0)/*full patch list is here*/
	{ P_END|=X_START;
	}
	/*
	** select Flats markers  (tricky hack!)
	*/
	refwad = ((Append!=TRUE)||(select&BFLAT))? iwad : pwad;
	if(WADRfindEntry(refwad,"F_END")>=0) /*full flat list is here*/
	{ F_END|=X_END;
	  if(WADRfindEntry(refwad,"F3_END")>=0) Fn=3;
	  else if(WADRfindEntry(refwad,"F2_END")>=0) Fn=2;
	  else if(WADRfindEntry(refwad,"F1_END")>=0) Fn=1;
	}
	if(WADRfindEntry(refwad,"F_START")>=0) /*full flat list is here*/
	{ F_END|=X_START;
	}
	if(!(select&BFLAT))
	{ if(F_END & X_END) /*flats already appended. keep it so.*/
	  { Complain=FALSE;
	  }
	}
	/*
	** make lists of types lists
	*/
	inb= (Int16)iwad->ntry;
	pnb= (Int16)pwad->ntry;
	idir=iwad->dir;
	pdir=pwad->dir;
	/*alloc memory for a fake new wad*/
	nwad.ok=0;
	WADRopenPipo(&nwad,(Int32)inb+(Int32)pnb+40);
	/*init lists*/
	LISinitLists();
	/*identify the elements and count types*/
	LIScountTypes(iiden,(Int16)iwad->ntry);
	LIScountTypes(piden,(Int16)pwad->ntry);
	LISallocLists();
	/* distribute IWAD enties*/
	for(i=0;i<inb;i++)
	{ idir[i].start|=iwadflag;
	}
	for(i=0;i<inb;i++)
	{ type= iiden[i] ;
	  switch(type&EMASK)
	  {
		 case ELEVEL:    case EMAP:
			if(Append==FALSE) /*APPEND doesn't need old enties*/
			{ if(select&BLEVEL)
			  { for(found=1;found<11;found++)
				 { if(iiden[i+found]!=iiden[i])break;
				 }
				 LISaddMission(&(idir[i]),found);
				 i+=found-1;
			  }
			}
			break;
		 case ELUMP:   case EGRAPHIC:
		 case ETEXTUR: case EPNAME:
		 case EMUSIC:  case ESOUND:
		 case EDATA:
			if(Append==FALSE)  /*APPEND doesn't need old enties*/
			{ LISadd(&LISlmp,&(idir[i]));
			}
			break;
		 case EPATCH:
			/*if(AllPat!=TRUE) type=ELUMP;*/
			if(Append==FALSE)  /*APPEND doesn't need old enties*/
			{ if(select&BPATCH)
			  { LISadd(&LISpat,&(idir[i]));
			  }
			}
			break;
		 case ESPRITE:
			if(select&BSPRITE)
			{  LISadd(&LISspr,&(idir[i]));
			}
			break;
		 case EFLAT:
			if(select&BFLAT)
			{  LISadd(&LISflt,&(idir[i]));
			}
			break;
		 case EVOID:
		 case EZZZZ:
			break;
		 default:
			Bug("LisUkI");/*unknown entry type in IWAD*/
			break;
	  }
	}
	Free(iiden);
	/*update position of PWAD entries*/
	for(p=0;p<pnb;p++)
	{ pdir[p].start|=pwadflag;
	}
	/*
	** detect the absence of sprites
	*/
	if(select&BSPRITE)
	{ NoSprite=TRUE;  /*if no sprites, graphics could be sprites*/
	  for(p=0;p<pnb;p++)
	  { if((piden[p] & EMASK)==ESPRITE)
		 { NoSprite=FALSE;break; /*don't bother searching*/
		 }
	  }
	}
	/*put PWAD entries*/
	for(p=0;p<pnb;p++)
	{ type= piden[p] ;
	  switch(piden[p] & EMASK)
	  {   /*special treatment for missions*/
		 case ELEVEL:    case EMAP:
			{ for(found=1;found<11;found++)
			  {   if(piden[p+found]!=piden[p])break;
			  }
			  LISaddMission(&(pdir[p]),found);
			  p+=found-1;
			}
			break;
		 case EPATCH:
			{ LISmove(&LISpat,&(pdir[p]));
			}
			break;
		 case EDATA:
		 case ELUMP:
			{ if((Append!=TRUE)&&(Complain==TRUE))
				 LISsubstit(&LISlmp,&(pdir[p]));  /*warn if missing*/
			  else
				 LISmove(&LISlmp,&(pdir[p]));      /*no warning needed*/
			}
			break;
       case ETEXTUR: case EPNAME:
         { if((Append!=TRUE)&&(Complain==TRUE))
             LISsubstit(&LISlmp,&(pdir[p]));  /*warn if missing*/
           else
             LISmove(&LISlmp,&(pdir[p]));      /*no warning needed*/
         }
         break;
       case ESOUND:
         { if((Append!=TRUE)&&(Complain==TRUE))
             LISsubstit(&LISlmp,&(pdir[p]));  /*warn if missing*/
           else
             LISmove(&LISlmp,&(pdir[p]));      /*no warning needed*/
         }
         break;
       case EMUSIC:
         { if((Append!=TRUE)&&(Complain==TRUE))
             LISsubstit(&LISlmp,&(pdir[p]));  /*warn if missing*/
           else
             LISmove(&LISlmp,&(pdir[p]));      /*no warning needed*/
         }
         break;
       case EGRAPHIC:
         { if(NoSprite==TRUE) /*special: look for sprites identfied as graphics*/
           { if(LISfindRplc(&LISspr,&(pdir[p]))==FALSE)
             { /*not a sprite. add in lumps*/
               LISmove(&LISlmp,&(pdir[p]));
             }
           }
           else /*normal*/
           { if((Append!=TRUE)&&(Complain==TRUE))
               LISsubstit(&LISlmp,&(pdir[p]));  /*warn if missing*/
             else
               LISmove(&LISlmp,&(pdir[p]));      /*no warning needed*/
           }
         }
         break;
       case ESPRITE:/*special sprite viewpoint kill*/
         { LISaddSprite(&(pdir[p]),Complain);    /*warn if missing*/
         }
         break;
       case EFLAT:/*add and replace former flat if needed*/
         { LISmove(&LISflt,&(pdir[p]));
         }
         break;
       case EVOID:
       case EZZZZ:
         break;
       default:
         Bug("LisUkP");/*unknown entry type in PWAD*/
        break;
      }
   }
   Free(piden);
   /*create the new directory*/
   LISmakeNewDir(&nwad,S_END,P_END,F_END,Pn,Fn);
   /*free memory*/
	LISfreeLists();
   /*return parameters wad dir and wad dir size*/
   return WADRclosePipo(&nwad,/*size */pNtry);
}

/******************* end List module *********************/

