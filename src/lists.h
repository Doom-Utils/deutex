/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2018 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0+
*/

/*function to merge two WAD directories (complex)*/
struct WADDIR *LISmergeDir(int32_t * pNtry, bool OnlySF, bool Complain,
                           NTRYB select, struct WADINFO *iwad,
                           ENTRY * iiden, int32_t iwadflag,
                           struct WADINFO *pwad, ENTRY * piden,
                           int32_t pwadflag);
