/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/*merge a WAD into an IWAD*/
void PSTmergeWAD(const char *doomwad, const char *wadin, NTRYB select);
/*put all sprites in another WAD, like DMADDS*/
void ADDallSpriteFloor(const char *wadout, const char *doomwad,
                       const char *wadin, NTRYB select);
/*append all sprites*/
void ADDappendSpriteFloor(const char *doomwad, const char *wadin,
                          NTRYB select);
/*join two WADs, including textures and pnames*/
void ADDjoinWads(const char *doomwad, const char *wadres,
                 const char *wadext, NTRYB select);
/*restore a modified WAD*/
void HDRrestoreWAD(const char *wadres);
