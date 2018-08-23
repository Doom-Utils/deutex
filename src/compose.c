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
#include "mkwad.h"
#include "texture.h"
#include "ident.h"
#include "color.h"
#include "picture.h"
#include "sound.h"
#include "text.h"

static void AddSomeJunk(const char *file);
/*
** Translate TEXTURE1,TEXTURE2 and PNAME in
** a texture list for future modifications
*/
extern char file[128];

/*
** make a PWAD from creation directives
** load levels,lumps,
**  create textures
** load sounds, pics, sprites, patches, flats,
*/

/*
** Can't handle PATCHES redefined from WAD
*/
static bool CMPOcopyFromWAD(int32_t * size, struct WADINFO *rwad,
                            const char *DataDir, const char *Dir,
                            const char *nam, const char *filenam)
{
    static struct WADINFO pwad;
    int16_t entry;
    if (!MakeFileName(file, DataDir, Dir, "", filenam, "WAD"))
        return false;
    WADRopenR(&pwad, file);
    entry = WADRfindEntry(&pwad, nam);
    if (entry >= 0) {
        *size = WADRwriteWADentry(rwad, &pwad, entry);
    }
    WADRclose(&pwad);
    if (entry <= 0)
        return false;
    return true;
}

/*
** find a picture.
** must=true is picture must exist
** returns picture type
*/
static int16_t CMPOloadPic(int32_t * size, struct WADINFO *rwad,
                           char *file, const char *DataDir,
                           const char *Dir, const char *nam,
                           const char *filenam, int16_t Type, int16_t OfsX,
                           int16_t OfsY)
{
    int res = PICNONE;
    if (MakeFileName(file, DataDir, Dir, "", filenam, "ppm"))
        res = PICPPM;
    else if (MakeFileName(file, DataDir, Dir, "", filenam, "bmp"))
        res = PICBMP;
#ifdef HAVE_LIBPNG
    else if (MakeFileName(file, DataDir, Dir, "", filenam, "png"))
        res = PICPNG;
#endif
    else if (MakeFileName(file, DataDir, Dir, "", filenam, "gif"))
        res = PICGIF;
    else if (CMPOcopyFromWAD(size, rwad, DataDir, Dir, nam, filenam))
        return PICWAD;
    if (res != PICNONE)
        *size = PICsaveInWAD(rwad, file, Type, OfsX, OfsY, res);
    else if (Type != PLUMP)
#ifdef HAVE_LIBPNG
        Warning("PC90", "could not find file %s, .png, .ppm, .bmp or .gif",
                file);
#else
        Warning("PC90", "could not find file %s, .ppm, .bmp or .gif",
                file);
#endif
    return res;
}

/*
** find a raw picture, for TX_START support.
*/
static bool CMPOloadRawPic(int32_t * size, struct WADINFO *rwad,
                           char *file, const char *DataDir,
                           const char *Dir, const char *nam,
                           const char *filenam)
{
    /* most ports support PNG, ZDoom also supports JPEG */
    if (MakeFileName(file, DataDir, Dir, "", filenam, "jpeg") ||
        MakeFileName(file, DataDir, Dir, "", filenam, "jpg") ||
        MakeFileName(file, DataDir, Dir, "", filenam, "png")) {

        *size = WADRwriteLump(rwad, file);
        return true;
    }

    /* file has the last filename tried, i.e. NAME.png */
    Warning("PC91", "could not find file %s, .gif, or .jpeg", file);
    return false;
}

struct WADINFO *CMPOrwad;
const char *CMPOwadout = NULL;
void CMPOerrorAction(void)
{
    if (CMPOwadout == NULL)
        return;
    WADRclose(CMPOrwad);        /*close file */
    remove(CMPOwadout);         /*delete file */
}

void CMPOmakePWAD(const char *doomwad, WADTYPE type, const char *PWADname,
                  const char *DataDir, const char *texin, NTRYB select,
                  char trnR, char trnG, char trnB, bool George)
{
    /*
    ** type PWAD as we are generating a real PWAD
    */
    int32_t start = 0, size = 0;
    static char name[8];
    static char filenam[8];
    /*PNAMES */
    int16_t nbPatchs, p;
    bool NeedPNAME = false;
    bool FoundOne = false;
    bool Repeat;
    FLAGS Flags;
    IMGTYPE Picture;
    /*optional insertion point */
    int16_t X, Y;
    /*text file to read */
    static struct TXTFILE *TXT;
    /*DOOM wad */
    static struct WADINFO iwad, pwad;
    /*result wad file */
    static struct WADINFO rwad;
    /*for Pnames */
    int16_t entry;
    char *EntryP;
    int32_t EntrySz = 0;
    /* initialisation */

    Info("CM01", "Composing %cWAD %s from %s", (type == IWAD) ? 'I' : 'P',
         fname(PWADname), texin);

    /*open iwad,get iwad directory */
    iwad.ok = 0;
    WADRopenR(&iwad, doomwad);

    TXT = TXTopenR(texin, 0);
    WADRopenW(&rwad, PWADname, type, 1);        /* fake IWAD or real PWAD */
    /*
    ** dirty: set error handler to delete the wad out file,
    ** if an error occurs.
    */
    CMPOrwad = &rwad;
    CMPOwadout = PWADname;
    ProgErrorAction(CMPOerrorAction);
    /*
    ** levels! add your own new levels to DOOM!
    ** read level from a PWAD file
    */
    if (select & BLEVEL) {
        if (TXTseekSection(TXT, "LEVELS")) {
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                p = IDENTlevel(name);
                if (p < 0)
                    ProgError("CM11", "Illegal level name %s",
                              lump_name(name));
                if (!MakeFileName
                    (file, DataDir, "LEVELS", "", filenam, "WAD"))
                    ProgError("CM12", "Can't find level WAD %s",
                              fname(file));
                Detail("CM13", "Reading level WAD file %s", fname(file));
                WADRwriteWADlevel(&rwad, file, name);
            }
        }
    }
    /*
    ** prepare palette for graphics
    */
    /*find PLAYPAL */
    if (select & (BGRAPHIC | BSPRITE | BPATCH | BFLAT)) {
        /* If wadinfo.txt mentions a custom PLAYPAL, use that.
           Otherwise, use the one in the iwad. */
        char *paldata = NULL;
        const char *playpal_pathname = NULL;
        const char *playpal_lumpname = NULL;

        if (TXTseekSection(TXT, "LUMPS")) {
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (strcmp(name, "PLAYPAL") == 0) {
                    FILE *playpal_fp;

                    MakeFileName(file, DataDir, "LUMPS", "", filenam,
                                 "LMP");
                    playpal_pathname = file;
                    playpal_lumpname = NULL;
                    EntrySz = 768;
                    paldata = Malloc(EntrySz);
                    playpal_fp = fopen(file, FOPEN_RB);
                    if (playpal_fp == NULL)
                        ProgError("CM21", "%s: %s", fname(file),
                                  strerror(errno));
                    EntrySz = fread(paldata, 1, EntrySz, playpal_fp);
                    if (EntrySz != 768)
                        /* DEBUG was ProgError */
                        Warning("CM22", "%s: short read", fname(file));
                    fclose(playpal_fp);
                    break;
                }
            }
        }
        if (paldata == NULL) {
            playpal_pathname = iwad.filename;
            playpal_lumpname = palette_lump;
            entry = WADRfindEntry(&iwad, palette_lump);
            if (entry < 0)
                ProgError("CM23", "Can't find %s in main WAD",
                          lump_name(palette_lump));
            paldata = WADRreadEntry(&iwad, entry, &EntrySz);
        }
        COLinit(trnR, trnG, trnB, paldata, (int16_t) EntrySz,
                playpal_pathname, playpal_lumpname);
        free(paldata);
    }

    /*
    **
    **   lumps. non graphic raw data for DOOM
    */
    if (select & BLUMP) {
        start = size = 0;
        if (TXTseekSection(TXT, "LUMPS")) {
            Phase("CM30", "Making lumps");
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (!Repeat) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    if (MakeFileName
                        (file, DataDir, "LUMPS", "", filenam, "LMP")) {
                        size = WADRwriteLump(&rwad, file);
                    } else {
                        Picture =
                            CMPOloadPic(&size, &rwad, file, DataDir,
                                        "LUMPS", name, filenam, PLUMP, X,
                                        Y);
                        if (Picture == PICNONE)
                            if (!CMPOcopyFromWAD
                                (&size, &rwad, DataDir, "LUMPS", name,
                                 filenam))
                                ProgError("CM31",
                                          "Can't find lump or picture file %s",
                                          file);
                    }
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
        }
    }
    /*
    ** initialise list of patch names
    */
    if (select & (BTEXTUR | BPATCH)) {
        entry = WADRfindEntry(&iwad, "PNAMES");
        if (entry < 0)
            ProgError("CM40", "Can't find PNAMES in main WAD");
        EntryP = WADRreadEntry(&iwad, entry, &EntrySz);
        PNMinit(EntryP, EntrySz);
        free(EntryP);
        NeedPNAME = false;
    }
    /*
    ** read texture1
    */
    if (select & BTEXTUR) {
        if (TXTseekSection(TXT, "TEXTURE1")) {
            Phase("CM50", "Making TEXTURE1");
            TXUinit();
            entry = WADRfindEntry(&iwad, "TEXTURE1");
            if (entry >= 0) {
                EntryP = WADRreadEntry(&iwad, entry, &EntrySz);
                TXUreadTEXTURE("TEXTURE1", EntryP, EntrySz, NULL, 0, true);
                free(EntryP);
            } else
                Warning("CM51", "Can't find TEXTURE1 in main WAD");
            FoundOne = false;
            /*read TEXTURES composing TEXTURE1 */
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (MakeFileName
                    (file, DataDir, "TEXTURES", "", filenam, "TXT")) {
                    Detail("CM52", "Reading texture file %s", fname(file));
                    TXUreadTexFile(file, true);
                    NeedPNAME = true;
                    FoundOne = true;
                } else
                    if (MakeFileName
                        (file, DataDir, "TEXTURES", "", name, "WAD")) {
                        Detail("CM53", "Reading texture WAD %s", fname(file));
                        WADRopenR(&pwad, file);
                        entry = WADRfindEntry(&pwad, "TEXTURE1");
                        if (entry >= 0) {
                            EntryP = WADRreadEntry(&pwad, entry, &EntrySz);
                            TXUreadTEXTURE("TEXTURE1", EntryP, EntrySz, NULL,
                                           0, true);
                            free(EntryP);
                            NeedPNAME = true;
                            FoundOne = true;
                        }
                        WADRclose(&pwad);
                    } else
                        ProgError("CM54", "Can't find texture list %s", file);
            }
            /*write texture */
            if (FoundOne) {
                WADRalign4(&rwad);      /*align entry on int32_t word */
                start = WADRposition(&rwad);
                size = TXUwriteTEXTUREtoWAD(&rwad);
                WADRdirAddEntry(&rwad, start, size, "TEXTURE1");
            }
            TXUfree();
        }
    }
    /*
    ** read texture2
    */
    if (select & BTEXTUR) {
        if (TXTseekSection(TXT, "TEXTURE2")) {
            Phase("CM55", "Making TEXTURE2");
            TXUinit();
            entry = WADRfindEntry(&iwad, "TEXTURE2");
            if (entry >= 0) {
                EntryP = WADRreadEntry(&iwad, entry, &EntrySz);
                TXUreadTEXTURE("TEXTURE2", EntryP, EntrySz, NULL, 0, true);
                free(EntryP);
            } else
                Warning("CM56", "Can't find TEXTURE2 in main WAD");
            FoundOne = false;
            /*read TEXTURES composing TEXTURE2 */
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (MakeFileName
                    (file, DataDir, "TEXTURES", "", filenam, "TXT")) {
                    Detail("CM57", "Reading texture file %s", fname(file));
                    TXUreadTexFile(file, true);
                    NeedPNAME = true;
                    FoundOne = true;
                } else
                    if (MakeFileName
                        (file, DataDir, "TEXTURES", "", name, "WAD")) {
                        Detail("CM58", "Reading texture WAD %s", fname(file));
                        WADRopenR(&pwad, file);
                        entry = WADRfindEntry(&pwad, "TEXTURE2");
                        if (entry >= 0) {
                            EntryP = WADRreadEntry(&pwad, entry, &EntrySz);
                            TXUreadTEXTURE("TEXTURE2", EntryP, EntrySz, NULL,
                                           0, true);
                            free(EntryP);
                            NeedPNAME = true;
                            FoundOne = true;
                        }
                        WADRclose(&pwad);
                    } else
                        ProgError("CM59", "Can't find texture list %s", file);
            }
            /*write texture */
            if (FoundOne) {
                WADRalign4(&rwad);      /*align entry on int32_t word */
                start = WADRposition(&rwad);
                size = TXUwriteTEXTUREtoWAD(&rwad);
                WADRdirAddEntry(&rwad, start, size, "TEXTURE2");
            }
            TXUfree();
        }
    }
    /*
    ** PNAME
    */
    if (select & BTEXTUR) {
        if (NeedPNAME) {
            /*write PNAME in PWAD */
            Phase("CM41", "Making PNAMES");
            WADRalign4(&rwad);  /*align entry on int32_t word */
            start = WADRposition(&rwad);
            size = PNMwritePNAMEtoWAD(&rwad);
            WADRdirAddEntry(&rwad, start, size, "PNAMES");
        }
    }
    /*
    ** TX_START
    ** textures  (patch or PNG format)
    ** TX_END
    */
    if (select & BTEXTUR) {
        start = size = 0;
        if (TXTseekSection(TXT, "TX_START")) {
            Phase("CM59", "Making TX_START textures");
            FoundOne = false;
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                /* only add markers if we have some textures */
                if (!FoundOne) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    WADRdirAddEntry(&rwad, start, 0L, "TX_START");
                    FoundOne = true;
                }

                WADRalign4(&rwad);  /*align entry on int32_t word */
                start = WADRposition(&rwad);
                size = 0;

                /* when marked "raw", insert as a raw lump (no conversion) */
                if (Flags & F_RAW) {
                    CMPOloadRawPic(&size, &rwad, file, DataDir, "TX_START",
                                   name, filenam);
                } else {
                    CMPOloadPic(&size, &rwad, file, DataDir, "TX_START",
                                name, filenam, PGRAPH, X, Y);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
            if (FoundOne) {
                WADRalign4(&rwad);
                start = WADRposition(&rwad);
                WADRdirAddEntry(&rwad, start, 0L, "TX_END");
            }
        }
    }
    /*
    **
    **   sounds. all sounds entries
    */
    if (select & BSOUND) {
        start = size = 0;
        if (TXTseekSection(TXT, "SOUNDS")) {
            Phase("CM60", "Making sounds");
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (!Repeat) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    if (MakeFileName
                        (file, DataDir, "SOUNDS", "", filenam, "TXT")) {
                        size = SNDcopyPCSoundInWAD(&rwad, file);
                        Detail("CM62", "Reading PC sound from file %s",
                               fname(file));
                    } else {
                        if (MakeFileName
                            (file, DataDir, "SOUNDS", "", filenam, "WAV")) {
                            size = SNDcopyInWAD(&rwad, file, SNDWAV);
                        } else {
                            if (!CMPOcopyFromWAD
                                (&size, &rwad, DataDir, "SOUNDS", name,
                                 filenam)) {
                                ProgError("CM63",
                                          "Can't find sound %s, WAV",
                                          file);
                            }
                        }
                        Detail("CM64", "Reading sound file %s",
                               fname(file));
                    }
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
        }
    }
    /*
    **
    **   Musics
    */
    if (select & BMUSIC) {
        start = size = 0;
        if (TXTseekSection(TXT, "MUSICS")) {
            Phase("CM65", "Making musics");
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (!Repeat) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    /*Music */
                    if (MakeFileName
                        (file, DataDir, "MUSICS", "", filenam, "MUS")) {
                        size = WADRwriteLump(&rwad, file);
                        Detail("CM66", "Reading music file %s",
                               fname(file));
                    } else
                        if (MakeFileName
                            (file, DataDir, "MUSICS", "", filenam, "MID")) {
                            size = WADRwriteLump(&rwad, file);
                            Detail("CM68", "Reading music file %s",
                                   fname(file));
                        } else
                            if (!CMPOcopyFromWAD
                                (&size, &rwad, DataDir, "MUSICS", name,
                                 filenam))
                                ProgError("CM67", "Can't find music %s", file);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
        }
    }
    /*
    **  ordinary graphics
    */
    if (select & BGRAPHIC) {
        start = size = 0;
        if (TXTseekSection(TXT, "GRAPHICS")) {
            Phase("CM70", "Making graphics");
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, true)) {
                if (!Repeat) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    Picture =
                        CMPOloadPic(&size, &rwad, file, DataDir,
                                    "GRAPHICS", name, filenam, PGRAPH, X,
                                    Y);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
        }
    }
    /*
    **  SS_START
    **  sprites
    **  SS_END
    */
    if (select & BSPRITE) {
        start = size = 0;
        if (TXTseekSection(TXT, "SPRITES")) {
            Phase("CM75", "Making sprites");
            FoundOne = false;
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, true)) {
                /* first sprite seen? */
                if (!Repeat || !FoundOne) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    if (!FoundOne) {
                        if (type == IWAD)
                            WADRdirAddEntry(&rwad, start, 0L, "S_START");
                        else
                            WADRdirAddEntry(&rwad, start, 0L, "SS_START");
                    }
                    FoundOne = true;
                    CMPOloadPic(&size, &rwad, file, DataDir, "SPRITES",
                                name, filenam, PSPRIT, X, Y);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
            if (FoundOne) {
                WADRalign4(&rwad);
                start = WADRposition(&rwad);
                if ((type == IWAD) || George)
                    WADRdirAddEntry(&rwad, start, 0L, "S_END");
                else
                    WADRdirAddEntry(&rwad, start, 0L, "SS_END");
            }
        }
    }
    /*
    ** Try to load patches
    **   even if no new textures (old patches could be redefined)
    */
    /* write new patches  in PWAD */
    /* read the name of the new textures and insert them */
    /* between P_START and P_END for future completion */

    if (select & BPATCH) {
        FoundOne = false;
        /*
        ** First look for patches in [PATCHES]
        */
        start = size = 0;
        if (TXTseekSection(TXT, "PATCHES")) {
            Phase("CM80", "Making patches");
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, true)) {
                if (!Repeat || !FoundOne) {
                    WADRalign4(&rwad);  /*align entry on int32_t word */
                    start = WADRposition(&rwad);
                    if (!FoundOne) {
                        if (type == IWAD) {
                            WADRdirAddEntry(&rwad, start, 0L, "P_START");
                            WADRdirAddEntry(&rwad, start, 0L, "P1_START");
                        } else {
                            WADRdirAddEntry(&rwad, start, 0L, "PP_START");
                        }
                    }
                    FoundOne = true;
                    CMPOloadPic(&size, &rwad, file, DataDir, "PATCHES",
                                name, filenam, PPATCH, X, Y);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
        }
        /*
        ** Check if all the needed patches are defined.
        */
        nbPatchs = PNMgetNbOfPatch();
        for (p = 0; p < nbPatchs; p++) {
            if (!PNMisNew(p)) {
                continue;       /*if old patch, forget it */
            }
            PNMgetPatchName(name, p);
            Normalise(filenam, name);
            /*search in main IWAD directory */
            if (WADRfindEntry(&iwad, name) >= 0) {
                Output("Reusing DOOM entry %s as patch\n",
                       lump_name(name));
            }
            /*search in current PWAD */
            else if (WADRfindEntry(&rwad, name) < 0) {
                /*PATCH not found in current WAD, load automatically
                **from the PATCH directory
                */
                WADRalign4(&rwad);      /*align entry on int32_t word */
                start = WADRposition(&rwad);
                Picture =
                    CMPOloadPic(&size, &rwad, file, DataDir, "PATCHES",
                                name, filenam, PPATCH, INVALIDINT,
                                INVALIDINT);
                if (Picture != PICNONE) {
                    if (!FoundOne) {
                        Phase("CM82", "Making patches");
                        if (type == IWAD) {
                            WADRdirAddEntry(&rwad, start, 0L, "P_START");
                            WADRdirAddEntry(&rwad, start, 0L, "P1_START");
                        } else
                            WADRdirAddEntry(&rwad, start, 0L, "PP_START");
                    }
                    FoundOne = true;
                    WADRdirAddEntry(&rwad, start, size, name);
                }
            }
        }
        if (FoundOne) {
            WADRalign4(&rwad);  /*align entry on int32_t word */
            start = WADRposition(&rwad);
            if (type == IWAD) {
                WADRdirAddEntry(&rwad, start, 0L, "P1_END");
                WADRdirAddEntry(&rwad, start, 0L, "P2_START");
                WADRdirAddEntry(&rwad, start, 0L, "P2_END");
                WADRdirAddEntry(&rwad, start, 0L, "P3_START");
                WADRdirAddEntry(&rwad, start, 0L, "P3_END");
                WADRdirAddEntry(&rwad, start, 0L, "P_END");
            } else
                WADRdirAddEntry(&rwad, start, 0L, "PP_END");
        }
    }
    /*
    ** clear off Pnames
    */
    if (select & (BTEXTUR | BPATCH)) {
        PNMfree();
    }
    /*  FF_START
    **  Flats
    **  F_END
    */
    if (select & BFLAT) {
        if (TXTseekSection(TXT, "FLATS")) {
            Phase("CM85", "Making flats");
            FoundOne = false;
            while (TXTparseEntry
                   (name, filenam, &X, &Y, &Flags, &Repeat, TXT, false)) {
                if (!Repeat || !FoundOne) {
                    /*align entry on int32_t word */
                    WADRalign4(&rwad);

                    start = WADRposition(&rwad);
                    if (!FoundOne) {
                        if (type == IWAD) {
                            WADRdirAddEntry(&rwad, start, 0L, "F_START");
                            WADRdirAddEntry(&rwad, start, 0L, "F1_START");
                        } else
                            WADRdirAddEntry(&rwad, start, 0L, "FF_START");
                    }
                    FoundOne = true;
                    CMPOloadPic(&size, &rwad, file, DataDir, "FLATS", name,
                                filenam, PFLAT, INVALIDINT, INVALIDINT);
                }
                WADRdirAddEntry(&rwad, start, size, name);
            }
            if (FoundOne) {
                start = WADRposition(&rwad);
                if (type == IWAD) {
                    WADRdirAddEntry(&rwad, start, 0L, "F1_END");
                    WADRdirAddEntry(&rwad, start, 0L, "F2_START");
                    WADRdirAddEntry(&rwad, start, 0L, "F2_END");
                    WADRdirAddEntry(&rwad, start, 0L, "F3_START");
                    WADRdirAddEntry(&rwad, start, 0L, "F3_END");
                    WADRdirAddEntry(&rwad, start, 0L, "F_END");
                } else
                    WADRdirAddEntry(&rwad, start, 0L, "F_END");
            }
        }
    }
    /*
    ** exit from graphic
    */
    if (select & (BGRAPHIC | BSPRITE | BPATCH | BFLAT))
        COLfree();
    /*
    ** iwad not needed anymore
    */
    WADRclose(&iwad);
    /*
    ** the end
    */
    TXTcloseR(TXT);
    WADRwriteDir(&rwad, 1);     /* write the WAD directory */
    ProgErrorCancel();
    WADRclose(&rwad);
    /*add some junk at end of wad file, for DEU 5.21 */
    if (type == PWAD)
        AddSomeJunk(PWADname);
}

extern int16_t HowMuchJunk;
static char Junk[]
= "This junk is here for DEU 5.21. I am repeating myself anyway... ";
static void AddSomeJunk(const char *file)
{
    FILE *out;
    int16_t n;
    out = fopen(file, FOPEN_AB);        /*open R/W at the end */
    if (out == NULL)
        ProgError("CM97", "%s: %s", fname(file), strerror(errno));
    for (n = 0; n < HowMuchJunk; n++)
        if (fwrite(Junk, 1, 64, out) < 64)
            Warning("CM98", "Can't insert my junk!");
    fclose(out);

}
