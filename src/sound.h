/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

void SNDsaveSound(char *file, char *buffer, int32_t size, SNDTYPE Sound,
                  const char *name);
int32_t SNDcopyInWAD(struct WADINFO *info, char *file, SNDTYPE Sound);
void SNDsavePCSound(const char *name, const char *file, const char *buffer,
                    int32_t size);
int32_t SNDcopyPCSoundInWAD(struct WADINFO *info, char *file);
