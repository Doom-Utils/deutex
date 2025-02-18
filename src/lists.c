/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/*
  This codes needs a serious cleaning now that it's stable!
  Too many parts are non optimised.
*/

#include "deutex.h"
#include "tools.h"
#include "mkwad.h"              /*for find entry */
#include "lists.h"

#define X_NONE 0
#define X_START 2
#define X_END 1

/*
** list of distinct entries types
*/
struct ELIST {
    int16_t Top;                /*num of entries allocated for list */
    int16_t Pos;                /*current top position */
    struct WADDIR *Lst;
};
static struct ELIST LISlmp;
static struct ELIST LISspr;
static struct ELIST LISpat;
static struct ELIST LISflt;

/*
** init
*/
static void LISinitLists(void)
{                               /* GRAPHIC, LEVEL, LUMP */
    LISlmp.Top = 1;
    LISlmp.Pos = 0;
    LISlmp.Lst = NULL;
    LISspr.Top = 1;
    LISspr.Pos = 0;
    LISspr.Lst = NULL;
    LISpat.Top = 1;
    LISpat.Pos = 0;
    LISpat.Lst = NULL;
    LISflt.Top = 1;
    LISflt.Pos = 0;
    LISflt.Lst = NULL;
}

/*
** count the number of entry types
*/
static void LIScountTypes(ENTRY * ids, int16_t nb)
{
    int16_t i;
    for (i = 0; i < nb; i++) {
        switch (ids[i] & EMASK) {
        case EPNAME:
        case ETEXTUR:
        case EMAP:
        case ELEVEL:
        case ELUMP:
        case EGRAPHIC:
        case EMUSIC:
        case ESOUND:
        case EDATA:
            LISlmp.Top++;
            break;
        case ESPRITE:
            LISspr.Top++;
            break;
        case EPATCH:
            LISpat.Top++;
            break;
        case EFLAT:
            LISflt.Top++;
            break;
        case EZZZZ:
        case EVOID:
            break;
        default:
            Bug("LI01", "LisUkn");
        }
    }
}

static void LISallocLists(void)
{                               /*allocate memory for the lists */
    LISlmp.Lst =
        (struct WADDIR *) Malloc(LISlmp.Top * sizeof(struct WADDIR));
    LISspr.Lst =
        (struct WADDIR *) Malloc(LISspr.Top * sizeof(struct WADDIR));
    LISpat.Lst =
        (struct WADDIR *) Malloc(LISpat.Top * sizeof(struct WADDIR));
    LISflt.Lst =
        (struct WADDIR *) Malloc(LISflt.Top * sizeof(struct WADDIR));
}

static void LISfreeLists(void)
{
    free(LISflt.Lst);
    free(LISpat.Lst);
    free(LISspr.Lst);
    free(LISlmp.Lst);
}

/*
** Delete unwanted sprites
**
*/
static bool LISunwantedPhase(char pv[2], char phase, char view,
                             bool AllViews)
{
    if (pv[0] != phase)
        return false;
    if (pv[1] == view)
        return true;            /*delete phase if equal */
    switch (pv[1]) {            /*view 0 unwanted if any other view exist */
    case '0':
        return (AllViews == true);
        /*view unwanted if view 0 exist */
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        return (AllViews == false);
    }
    return false;
}

static bool LISdeleteSprite(char root[8], char phase, char view)
{
    int16_t l, sz;
    bool Okay = false;
    bool AllViews;
    switch (view) {
    case '0':
        AllViews = false;
        break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
        AllViews = true;
        break;
    case '\0':
    default:                   /*Artifacts */
        return false;
    }
    /*root: only the 4 first char are to be tested */
    for (l = LISspr.Pos - 1; l >= 0; l--) {
        if (strncmp(LISspr.Lst[l].name, root, 4) == 0) {
            /*2nd unwanted: replace by 0 */
            if (LISunwantedPhase
                (&(LISspr.Lst[l].name[6]), phase, view, AllViews)) {
                LISspr.Lst[l].name[6] = '\0';
                LISspr.Lst[l].name[7] = '\0';
                Okay = true;
            }
            /*1st unwanted: replace by second */
            if (LISunwantedPhase
                (&(LISspr.Lst[l].name[4]), phase, view, AllViews)) {
                LISspr.Lst[l].name[4] = LISspr.Lst[l].name[6];
                LISspr.Lst[l].name[5] = LISspr.Lst[l].name[7];
                LISspr.Lst[l].name[6] = '\0';
                LISspr.Lst[l].name[7] = '\0';
                Okay = true;
            }
            /*neither 1st nor second wanted: delete */
            if (LISspr.Lst[l].name[4] == '\0') {
                sz = LISspr.Pos - (l + 1);
                if (sz > 0)
                    Memcpy(&(LISspr.Lst[l]), &(LISspr.Lst[l + 1]),
                           (sz) * sizeof(struct WADDIR));
                LISspr.Pos--;
                Okay = true;
            }
        }
    }
    return Okay;
}

/*
** list management. very basic. no hash table.
*/
static void LISaddMission(struct WADDIR *dir, int16_t found)
{
    int16_t l = 0, dummy = 0;
    /*check */
    if (LISlmp.Pos >= LISlmp.Top)
        Bug("LI03", "LisTsm");  /*level list too small */
    /*try to locate mission entry at l */
    for (l = 0; l < LISlmp.Pos; l++) {
        if (strncmp(LISlmp.Lst[l].name, dir[0].name, 8) == 0)
            break;              /*l<pos */
    }
    /*update pos */
    dummy = l + found;          /*bug of GCC */
    if (dummy > LISlmp.Pos)
        LISlmp.Pos = dummy;
    if (LISlmp.Pos > LISlmp.Top)
        Bug("LI05", "LisTsm");  /*level list too small */
    /*copy new level, or replace existing */
    Memcpy((&(LISlmp.Lst[l])), (dir), found * sizeof(struct WADDIR));
}

static void LISadd(struct ELIST *L, struct WADDIR *dir)
{                               /*check current position */
    if ((L->Pos) >= L->Top)
        Bug("LI07", "LisSml");  /*list count too small */
    /* ADD in List */
    Memcpy(&(L->Lst[(L->Pos)]), (dir), sizeof(struct WADDIR));
    (L->Pos)++;                 /*increase pointer in list */
    return;
}

/*
** Find in list, and replace if exist
**
*/
static bool LISfindRplc(struct ELIST *L, struct WADDIR *dir)
{
    int16_t l;
    for (l = 0; l < (L->Pos); l++)
        if (strncmp(L->Lst[l].name, dir[0].name, 8) == 0) {
            Memcpy(&(L->Lst[l]), &(dir[0]), sizeof(struct WADDIR));
            return true;
        }
    return false;               /*not found */
}

/* Substitute In List
** if entry exist, it is substituted, else added at the end of list
** suitable for LUMPS and SPRITES
** but generally those shall not be added: they may not be recognized
*/
static void LISsubstit(struct ELIST *L, struct WADDIR *dir)
{                               /* SUBSTIT in List */
    if (LISfindRplc(L, dir))
        return;
    Warning("LI09", "Entry %s might be ignored by the game",
            lump_name(dir[0].name));
    LISadd(L, dir);
}

/* Move In List
** if entry already exist, destroy it, and put at the end
** suitable for PATCH and FLATS  (to define new animations)
*/
static void LISmove(struct ELIST *L, struct WADDIR *dir)
{
    int16_t l, sz;
    for (l = 0; l < (L->Pos); l++)
        if (strncmp(L->Lst[l].name, dir[0].name, 8) == 0) {
            /*entry already exist. destroy it */
            sz = (L->Pos) - (l + 1);
            if (sz > 0)
                Memcpy(&(L->Lst[l]), &(L->Lst[l + 1]),
                       (sz) * sizeof(struct WADDIR));
            (L->Pos)--;
        }
    LISadd(L, dir);
}

/* Put Sprites In List
** if entry already exist, replace it
** if entry does not exist, check if it does not obsolete
** an IWAD sprite entry
** suitable for SPRITES only
*/
static void LISaddSprite(struct WADDIR *dir, bool Warn)
{
    bool Okay = false;
    /*warn:  false= no warning. true = warn. */
    /* If entry already exists, replace it */
    if (LISfindRplc(&LISspr, dir))
        return;
    /* Entry does not exist. Check that this sprite
    ** viewpoint doesn't obsolete other sprite viewpoints.
    */
    if (dir[0].name[4] != '\0') {
        Okay |=
            LISdeleteSprite(dir[0].name, dir[0].name[4], dir[0].name[5]);
        if (dir[0].name[6] != '\0')
            Okay |=
                LISdeleteSprite(dir[0].name, dir[0].name[6],
                                dir[0].name[7]);
    }
    if (!Okay && Warn) {
        Warning("LI11", "Entry %s might be ignored by the game",
                lump_name(dir[0].name));
    }
    LISadd(&LISspr, dir);
}

/*
** create a new directory list
**
*/
static void LISmakeNewDir(struct WADINFO *nwad, int16_t S_END,
                          int16_t P_END, int16_t F_END, int16_t Pn,
                          int16_t Fn)
{
    int16_t n;
    struct WADDIR *dir;
    /*levels,lumps, graphics into new dir */
    if (LISlmp.Pos > 0) {
        for (n = 0; n < LISlmp.Pos; n++) {
            dir = &LISlmp.Lst[n];
            WADRdirAddPipo(nwad, dir->start, dir->size, dir->name);
        }
    }
    /*sprites into new dir */
    if (LISspr.Pos > 0) {
        WADRdirAddPipo(nwad, 0, 0,
                       ((S_END & X_START) ? "S_START" : "SS_START"));
        for (n = 0; n < LISspr.Pos; n++) {
            dir = &LISspr.Lst[n];
            WADRdirAddPipo(nwad, dir->start, dir->size, dir->name);
        }
        WADRdirAddPipo(nwad, 0, 0, ((S_END & X_END) ? "S_END" : "SS_END"));
    }
    /*patch 1,2,3 into new dir */
    if (LISpat.Pos > 0) {
        WADRdirAddPipo(nwad, 0, 0,
                       ((P_END & X_START) ? "P_START" : "PP_START"));
        if ((Pn >= 1) && (P_END & X_START))
            WADRdirAddPipo(nwad, 0, 0, "P1_START");
        for (n = 0; n < LISpat.Pos; n++) {
            dir = &LISpat.Lst[n];
            WADRdirAddPipo(nwad, dir->start, dir->size, dir->name);
        }
        if ((Pn >= 1) && (P_END & X_START)) {
            WADRdirAddPipo(nwad, 0, 0, "P1_END");
            if (Pn >= 2) {
                WADRdirAddPipo(nwad, 0, 0, "P2_START");
                WADRdirAddPipo(nwad, 0, 0, "P2_END");
                if (Pn >= 3) {
                    WADRdirAddPipo(nwad, 0, 0, "P3_START");
                    WADRdirAddPipo(nwad, 0, 0, "P3_END");
                }
            }
        }
        WADRdirAddPipo(nwad, 0, 0, ((P_END & X_END) ? "P_END" : "PP_END"));
    }
    /*flat 1,2,3 into new dir */
    if (LISflt.Pos > 0) {
        WADRdirAddPipo(nwad, 0, 0,
                       ((F_END & X_START) ? "F_START" : "FF_START"));
        if ((Fn >= 1) && (F_END & X_START))
            WADRdirAddPipo(nwad, 0, 0, "F1_START");
        for (n = 0; n < LISflt.Pos; n++) {
            dir = &LISflt.Lst[n];
            WADRdirAddPipo(nwad, dir->start, dir->size, dir->name);
        }
        if ((Fn >= 1) && (F_END & X_START)) {
            WADRdirAddPipo(nwad, 0, 0, "F1_END");
            if (Fn >= 2) {
                WADRdirAddPipo(nwad, 0, 0, "F2_START");
                WADRdirAddPipo(nwad, 0, 0, "F2_END");
                if (Fn >= 3) {
                    WADRdirAddPipo(nwad, 0, 0, "F3_START");
                    WADRdirAddPipo(nwad, 0, 0, "F3_END");
                }
            }
        }
        WADRdirAddPipo(nwad, 0, 0, ((F_END & X_END) ? "F_END" : "FF_END"));
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
struct WADDIR *LISmergeDir(int32_t * pNtry, bool Append, bool Complain,
                           NTRYB select, struct WADINFO *iwad,
                           ENTRY * iiden, int32_t iwadflag,
                           struct WADINFO *pwad, ENTRY * piden,
                           int32_t pwadflag)
{
    struct WADDIR *idir;
    struct WADDIR *pdir;
    struct WADINFO nwad;
    int16_t inb, pnb;
    int16_t i, p, found;
    struct WADINFO *refwad;
    ENTRY type;
    int16_t Pn = 0;             /*0=nothing 1=P1_ 2=P2_ 3=P3_ */
    int16_t Fn = 0;             /*0=nothing 1=F1_ 2=F2_ 3=F3_ */
    bool NoSprite = false;      /*no sprites declared. seek in graphics */
    int16_t S_END = X_NONE;
    int16_t F_END = X_NONE;
    int16_t P_END = X_NONE;
    /*
    ** select Sprites markers  (tricky hack!)
    */
    refwad = (!Append || (select & BSPRITE)) ? iwad : pwad;
    if (WADRfindEntry(refwad, "S_END") >= 0) {
        /*full sprite list is here */
        S_END |= X_END;
    }
    if (WADRfindEntry(refwad, "S_START") >= 0) {
        /*full sprite list is here */
        S_END |= X_START;
    }
    if (!(select & BSPRITE)) {
        if (S_END & X_END) {
            /*sprites already appended. keep it so. */
            Complain = false;
        }
    }
    /*
    ** select Patches markers  (tricky hack!)
    */
    refwad = (!Append) ? iwad : pwad;
    if (WADRfindEntry(refwad, "P_END") >= 0) {
        /*full patch list is here */
        P_END |= X_END;
        if (WADRfindEntry(refwad, "P3_END") >= 0)
            Pn = 3;
        else if (WADRfindEntry(refwad, "P2_END") >= 0)
            Pn = 2;
        else if (WADRfindEntry(refwad, "P1_END") >= 0)
            Pn = 1;
    }
    if (WADRfindEntry(refwad, "P_START") >= 0) {
        /*full patch list is here */
        P_END |= X_START;
    }
    /*
    ** select Flats markers  (tricky hack!)
    */
    refwad = (!Append || (select & BFLAT)) ? iwad : pwad;
    if (WADRfindEntry(refwad, "F_END") >= 0) {
        /*full flat list is here */
        F_END |= X_END;
        if (WADRfindEntry(refwad, "F3_END") >= 0)
            Fn = 3;
        else if (WADRfindEntry(refwad, "F2_END") >= 0)
            Fn = 2;
        else if (WADRfindEntry(refwad, "F1_END") >= 0)
            Fn = 1;
    }
    if (WADRfindEntry(refwad, "F_START") >= 0) {
        /*full flat list is here */
        F_END |= X_START;
    }
    if (!(select & BFLAT)) {
        if (F_END & X_END) {
            /*flats already appended. keep it so. */
            Complain = false;
        }
    }
    /*
    ** make lists of types lists
    */
    inb = (int16_t) iwad->ntry;
    pnb = (int16_t) pwad->ntry;
    idir = iwad->dir;
    pdir = pwad->dir;
    /*alloc memory for a fake new wad */
    nwad.ok = 0;
    WADRopenPipo(&nwad, (int32_t) inb + (int32_t) pnb + 40);
    /*init lists */
    LISinitLists();
    /*identify the elements and count types */
    LIScountTypes(iiden, (int16_t) iwad->ntry);
    LIScountTypes(piden, (int16_t) pwad->ntry);
    LISallocLists();
    /* distribute IWAD enties */
    for (i = 0; i < inb; i++) {
        idir[i].start |= iwadflag;
    }
    for (i = 0; i < inb; i++) {
        type = iiden[i];
        switch (type & EMASK) {
        case ELEVEL:
        case EMAP:
            if (!Append) {
                /*APPEND doesn't need old enties */
                if (select & BLEVEL) {
                    for (found = 1; found < 11; found++) {
                        if (iiden[i + found] != iiden[i])
                            break;
                    }
                    LISaddMission(&(idir[i]), found);
                    i += found - 1;
                }
            }
            break;
        case ELUMP:
        case EGRAPHIC:
        case ETEXTUR:
        case EPNAME:
        case EMUSIC:
        case ESOUND:
        case EDATA:
            if (!Append) {
                /*APPEND doesn't need old enties */
                LISadd(&LISlmp, &(idir[i]));
            }
            break;
        case EPATCH:
            /*if(AllPat!=true) type=ELUMP; */
            if (!Append) {
                /*APPEND doesn't need old enties */
                if (select & BPATCH) {
                    LISadd(&LISpat, &(idir[i]));
                }
            }
            break;
        case ESPRITE:
            if (select & BSPRITE) {
                LISadd(&LISspr, &(idir[i]));
            }
            break;
        case EFLAT:
            if (select & BFLAT) {
                LISadd(&LISflt, &(idir[i]));
            }
            break;
        case EVOID:
        case EZZZZ:
            break;
        default:
            /*LisUkI */
            Bug("LI13", "Iwad entry has unknown type %04X",
                (unsigned) type);
            break;
        }
    }
    free(iiden);
    /*update position of PWAD entries */
    for (p = 0; p < pnb; p++) {
        pdir[p].start |= pwadflag;
    }
    /*
    ** detect the absence of sprites
    */
    if (select & BSPRITE) {
        NoSprite = true;        /*if no sprites, graphics could be sprites */
        for (p = 0; p < pnb; p++) {
            if ((piden[p] & EMASK) == ESPRITE) {
                NoSprite = false;
                break;          /*don't bother searching */
            }
        }
    }
    /*put PWAD entries */
    for (p = 0; p < pnb; p++) {
        type = piden[p];
        switch (piden[p] & EMASK) {
            /*special treatment for missions */
        case ELEVEL:
        case EMAP:
            for (found = 1; found < 11; found++) {
                if (piden[p + found] != piden[p])
                    break;
            }
            LISaddMission(&(pdir[p]), found);
            p += found - 1;
            break;
        case EPATCH:
            LISmove(&LISpat, &(pdir[p]));
            break;
        case EDATA:
        case ELUMP:
            if (!Append && Complain)
                /*warn if missing */
                LISsubstit(&LISlmp, &(pdir[p]));
            else
                /*no warning needed */
                LISmove(&LISlmp, &(pdir[p]));
            break;
        case ETEXTUR:
        case EPNAME:
            if (!Append && Complain)
                /*warn if missing */
                LISsubstit(&LISlmp, &(pdir[p]));
            else
                /*no warning needed */
                LISmove(&LISlmp, &(pdir[p]));
            break;
        case ESOUND:
            if (!Append && Complain)
                /*warn if missing */
                LISsubstit(&LISlmp, &(pdir[p]));
            else
                /*no warning needed */
                LISmove(&LISlmp, &(pdir[p]));
            break;
        case EMUSIC:
            if (!Append && Complain)
                /*warn if missing */
                LISsubstit(&LISlmp, &(pdir[p]));
            else
                /*no warning needed */
                LISmove(&LISlmp, &(pdir[p]));
            break;
        case EGRAPHIC:
            if (NoSprite) {
                /*special: look for sprites identified as graphics */
                if (!LISfindRplc(&LISspr, &(pdir[p]))) {
                    /*not a sprite. add in lumps */
                    LISmove(&LISlmp, &(pdir[p]));
                }
            } else {
                /*normal */
                if (!Append && Complain)
                    /*warn if missing */
                    LISsubstit(&LISlmp, &(pdir[p]));
                else
                    /*no warning needed */
                    LISmove(&LISlmp, &(pdir[p]));
            }
            break;
        case ESPRITE:
            /*special sprite viewpoint kill */
            /*warn if missing */
            LISaddSprite(&(pdir[p]), Complain);
            break;
        case EFLAT:
            /*add and replace former flat if needed */
            LISmove(&LISflt, &(pdir[p]));
            break;
        case EVOID:
        case EZZZZ:
            break;
        default:
            /* LisUkP */
            Bug("LI15", "Pwad entry has unknown type %04X",
                (unsigned) type);
            break;
        }
    }
    free(piden);
    /*create the new directory */
    LISmakeNewDir(&nwad, S_END, P_END, F_END, Pn, Fn);
    /*free memory */
    LISfreeLists();
    /*return parameters wad dir and wad dir size */
    return WADRclosePipo(&nwad, /*size */ pNtry);
}
