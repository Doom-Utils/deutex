/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/*extract.c: list dir and extract entries*/
void XTRextractWAD(const char *doomwad, const char *DataDir, const char
                   *wadin, const char *texout, IMGTYPE Picture,
                   SNDTYPE Sound, NTRYB select, char trnR, char trnG,
                   char trnB, bool WSafe, cusage_t * cusage);

/*extract.c: get a single entry*/
void XTRgetEntry(const char *doomwad, const char *DataDir,
                 const char *wadin, const char *entry, IMGTYPE Picture,
                 SNDTYPE Sound, char trnR, char trnG, char trnB);

/*obsolete junk*/
void XTRtextureList(char *doomwad, char *DataDir);

void XTRpatchList(char *doomwad, char *DataDir, IMGTYPE Picture, char trnR,
                  char trnG, char trnB);
