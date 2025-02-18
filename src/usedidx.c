/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/
/*
 *      usedidx.c
 *      Palette index usage statistics (cf -usedidx)
 */

#include "deutex.h"
#include "tools.h"
#include "usedidx.h"

/*
 *      usedidx_begin_lump
 *      Call this before the first call to usedidx_count for
 *      a given lump.
 */
void usedidx_begin_lump(cusage_t * cusage, const char *name)
{
    int i;

    if (cusage == NULL)
        Bug("UI10", "usedidx_begin_lump: null cusage");
    cusage->lump_uses = Malloc(NCOLOURS * sizeof *cusage->lump_uses);
    for (i = 0; i < NCOLOURS; i++)
        cusage->lump_uses[i] = 0;
    memcpy(cusage->lump_name, name, sizeof cusage->lump_name);
}

/*
 *      usedidx_pixel
 *      Call this for each pixel for a given lump.
 */
void usedidx_pixel(cusage_t * cusage, unsigned char idx)
{
    cusage->lump_uses[idx]++;
}

/*
 *      usedidx_end_lump
 *      Call this when you're done with a given lump.
 */
void usedidx_end_lump(cusage_t * cusage)
{
    int i;
    if (cusage == NULL)
        Bug("UI20", "usedidx_end_lump: null cusage");
    for (i = 0; i < NCOLOURS; i++) {
        if (cusage->lump_uses[i] > 0) {
            if (cusage->uses[i] == 0)
                memcpy(cusage->where_first[i], cusage->lump_name,
                       sizeof cusage->where_first[i]);
            cusage->uses[i] += cusage->lump_uses[i];
            cusage->nlumps[i]++;
        }
    }
    free(cusage->lump_uses);
}

/*
 *      usedidx_rectangle
 *      Call this to update the cusage_t structure for a whole
 *      array of pixels at once. The array is supposed not to
 *      contain transparent pixels so this is not for pictures;
 *      it's for flats and rectangular graphic lumps (E.G.
 *      320x200 "TITLE" from Heretic or Hexen and 10x12
 *      "GNUM[0-9]" from Doom alpha.)
 */
void usedidx_rectangle(const char *buf, long buf_size, const char *name,
                       cusage_t * cusage)
{
    const unsigned char *p = (const unsigned char *) buf;
    const unsigned char *pmax = p + buf_size;
    unsigned long *uses = Malloc(NCOLOURS * sizeof *uses);
    int i;

    for (i = 0; i < NCOLOURS; i++)
        uses[i] = 0;
    for (; p < pmax; p++)
        uses[*p]++;
    for (i = 0; i < NCOLOURS; i++) {
        if (uses[i] > 0) {
            if (cusage->uses[i] == 0)
                memcpy(cusage->where_first[i], name, 8);
            cusage->uses[i] += uses[i];
            cusage->nlumps[i]++;
        }
    }
    free(uses);
}
