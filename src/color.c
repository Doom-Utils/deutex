/*
  This file is Copyright © 1994-1995 Olivier Montanuy,
               Copyright © 1999-2005 André Majorel,
               Copyright © 2006-2021 contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "deutex.h"
#include "tools.h"
#include "color.h"

/*
** hash table
**
*/
uint8_t COLindex(uint8_t R, uint8_t G, uint8_t B, uint8_t index);
static uint8_t COLpalMatch(uint8_t R, uint8_t G, uint8_t B);

static struct PIXEL *COLpal;    /* The game palette (comes from PLAYPAL) */
static struct PIXEL *COLpalAlt = NULL;  /* Alternate palette (TITLEPAL) */
static struct PIXEL COLinv;
static uint8_t COLinvisib;
static bool COLok = false;

/*
 *      pixel_cmp
 *      Compare two PIXEL structures à la memcmp()
 */
static int pixel_cmp(const void *pixel1, const void *pixel2)
{
    const struct PIXEL *const p1 = (const struct PIXEL *) pixel1;
    const struct PIXEL *const p2 = (const struct PIXEL *) pixel2;
    if (p1->R != p2->R)
        return p1->R - p2->R;
    if (p1->G != p2->G)
        return p1->G - p2->G;
    if (p1->B != p2->B)
        return p1->B - p2->B;
    return 0;

}

const int COLsame = 3;

/*
 *      COLdiff - compute the distance between two colours
 *
 *      Return a number between 0 (closest) and 24384 (farthest).
 */
int16_t COLdiff(uint8_t R, uint8_t G, uint8_t B, uint8_t idx)
{
    register struct PIXEL *pixel = &COLpal[(int16_t) (idx & 0xFF)];
    register int16_t d;         /*signed */
    register int16_t e = 0;
    d = (((int16_t) R) & 0xFF) - (((int16_t) pixel->R) & 0xFF);
    d >>= 1;
    e += d * d;
    d = (((int16_t) G) & 0xFF) - (((int16_t) pixel->G) & 0xFF);
    d >>= 1;
    e += d * d;
    d = (((int16_t) B) & 0xFF) - (((int16_t) pixel->B) & 0xFF);
    d >>= 1;
    e += d * d;
    if (e < 0)
        return 0x7FFF;
    return e;
}

static uint8_t COLpalMatch(uint8_t R, uint8_t G, uint8_t B)
{
    int16_t i, test, min = 0x7FFF;
    uint8_t idxmin = '\0';
    if (!COLok)
        Bug("PL24", "COLok");
    for (i = 0; i < 256; i++) {
        if ((uint8_t) i != COLinvisib) {
            test = COLdiff(R, G, B, (uint8_t) i);
            if (test < min) {
                min = test;
                idxmin = (uint8_t) i;
            }
            if (min < COLsame) {
                break;
            }
        }
    }
    return idxmin;
}

/* Choose only 10,11...16 */
#define POWER 12
const int16_t HashP2 = POWER;   /* 10=1024 */
const int16_t HashSz = 1 << POWER;      /* 1<< HashP2   */
const int16_t HashMask = (1 << POWER) - 1;      /* HashSz-1     */
/*const int16_t HashStop = -1;*/
static uint8_t *COLhash;        /*hash table */
/*static int16_t  *COLnext;*/

int16_t Hash(uint8_t r, uint8_t g, uint8_t b)
{
    int res;
    uint8_t R = r & 0xFC, G = g & 0xFC, B = b & 0xFC;
    res = (((R << 3) ^ G) << 2) ^ B;
    res = (res << 3) + (R & 0xC3) + (G & 0x61) + (~B & 0x98);
    res = (res << 5) + (R & 0x3B) + (~G & 0x95) + (B & 0x33);
    res ^= res >> 8;
    res &= HashMask;
    return (int16_t) res;
}

/*original colors*/
static void COLputColHash(int16_t index, uint8_t R, uint8_t G, uint8_t B)
{
    int16_t count, idx, nextidx;
    idx = Hash(R, G, B);
    for (count = 0; count < 8; count++) {
        nextidx = (idx + count) & HashMask;
        if (COLhash[nextidx] == COLinvisib) {
            COLhash[nextidx] = (uint8_t) (index & 0xFF);
            return;
        }
    }
    Bug("PL07", "Can't hash Doom pal");
}

/*new colors, with matching*/
static uint8_t COLgetIndexHash(uint8_t R, uint8_t G, uint8_t B)
{
    int16_t idx, nextidx, count;
    uint8_t res;
    idx = Hash(R, G, B);
    for (count = 0; count < 8; count++) {
        nextidx = (idx + count) & HashMask;
        res = COLhash[nextidx];
        if (res == COLinvisib) {        /*free */
            COLhash[nextidx] = res = COLpalMatch(R, G, B);
            return res;
        } else if (COLdiff(R, G, B, res) < COLsame) {
            return res;
        }
    }
    /*no good solution. slow match */
    return COLpalMatch(R, G, B);
}

/*
 *      Normal palette (PLAYPAL)
 */
void COLinit(uint8_t invR, uint8_t invG, uint8_t invB, char *Colors,
             int16_t Colsz, const char *pathname, const char *lumpname)
{
    int16_t i;
    const char *name = NULL;
    /*int16_t R,G,B; */
    if (COLok)
        Bug("PL02", "COLok");
    if (Colsz < 256 * sizeof(struct PIXEL)) {
        if (lumpname == NULL) {
            ProgError("PL03", "%s: wrong size for PLAYPAL",
                      fname(pathname));
        } else {
            ProgError("PL04", "%s: %s: wrong size for PLAYPAL",
                      fname(pathname), lump_name(lumpname));
        }
    }
    COLok = true;
    COLpal = (struct PIXEL *) Malloc(256 * sizeof(struct PIXEL));
    for (i = 0; i < NCOLOURS; i++) {
        COLpal[i].R = Colors[i * 3 + 0];
        COLpal[i].G = Colors[i * 3 + 1];
        COLpal[i].B = Colors[i * 3 + 2];
    }
    if (COLpal[0].R == 0 && COLpal[0].G == 0 && COLpal[0].B == 0
        && COLpal[0xf7].R == 0 && COLpal[0xf7].G == 0
        && COLpal[0xf7].B == 0) {
        i = 0xf7;
        name = "Doom";
    } else if (COLpal[35].R == 255 && COLpal[35].G == 255
               && COLpal[35].B == 255 && COLpal[255].R == 255
               && COLpal[255].G == 255 && COLpal[255].B == 255) {
        i = 0xff;
        name = "Heretic";
    } else if (COLpal[33].R == 29 && COLpal[33].G == 32
               && COLpal[33].B == 29 && COLpal[255].R == 255
               && COLpal[255].G == 255 && COLpal[255].B == 255) {
        i = 0xff;
        name = "Hexen";
    } else if (COLpal[0].R == 0 && COLpal[0].G == 0 && COLpal[0].B == 0
               && COLpal[240].R == 0 && COLpal[240].G == 0
               && COLpal[240].B == 0) {
        i = 0xf0;
        name = "Strife";
    } else {
        i = 0xff;
        name = NULL;
    }
    /*
    ** correction to doom palette
    */
    COLinvisib = (uint8_t) (i & 0xFF);
    Info("PL05", "Palette is %s", name ? name : "(unknown)");
    if (name == NULL) {
        Warning("PL06",
                "Unknown palette, using colour 0xff as transparent colour");
        Warning("PL06", "Some graphics may appear moth-eaten");
    }
    COLinv.R = COLpal[i].R = invR;
    COLinv.G = COLpal[i].G = invG;
    COLinv.B = COLpal[i].B = invB;

    /* Init hash table.
       We take special care of hashing only unique RGB triplets.
       This precaution is unnecessary for Doom, Heretic, Hexen
       and Strife but Doom alpha O.2, 0.4 and 0.5 have a PLAYPAL
       that contains many duplicates that would fill the hash
       table with useless data. */
    {
        struct PIXEL *unique = Malloc(NCOLOURS * sizeof *unique);
        for (i = 0; i < NCOLOURS; i++)
            unique[i] = COLpal[i];
        qsort(unique, NCOLOURS, sizeof *unique, pixel_cmp);
        COLhash = (uint8_t *) Malloc(HashSz);
        Memset(COLhash, COLinvisib, HashSz);    /*clear hash table */
        for (i = 0; i < NCOLOURS; i++) {
            if ((uint8_t) i != COLinvisib
                && (i == 0 || pixel_cmp(unique + i, unique + i - 1) != 0))
                COLputColHash(i, unique[i].R, unique[i].G, unique[i].B);
        }
        free(unique);
    }
}

void COLfree(void)
{
    if (!COLok)
        Bug("PL99", "COLok");
    COLok = false;
    free(COLpal);
    free(COLhash);
    if (COLpalAlt != NULL)
        free(COLpalAlt);
}

uint8_t COLinvisible(void)
{
    if (!COLok)
        Bug("PL27", "COLok");
    return COLinvisib;
}

struct PIXEL *COLdoomPalet(void)
{
    if (!COLok)
        Bug("PL20", "COLok");
    return COLpal;
}

uint8_t COLindex(uint8_t R, uint8_t G, uint8_t B, uint8_t index)
{
    int16_t i;
    if (!COLok)
        Bug("PL23", "COLok");
    /*check for invisible color */
    if (R == COLinv.R && G == COLinv.G && B == COLinv.B)
        return COLinvisib;
    /*check for DOOM palette */
    i = ((int16_t) index) & 0xFF;
    if (R == COLpal[i].R)
        if (G == COLpal[i].G)
            if (B == COLpal[i].B)
                return index;
    /*else, check hash palette */
    i = (int16_t) COLgetIndexHash(R, G, B);
    return (uint8_t) i;
}

/*
 *      Alternate palette (TITLEPAL)
 */
static char *titlepal_data = NULL;
static size_t titlepal_size = 0;

void COLinitAlt(char *_titlepal_data, int32_t _titlepal_size)
{
    titlepal_data = _titlepal_data;
    titlepal_size = _titlepal_size;
}

struct PIXEL *COLaltPalet(void)
{
    if (COLpalAlt != NULL)
        return COLpalAlt;

    /* What follows is done only once : */
    if (titlepal_data == NULL) {
        int n;

        Warning("PL11", "TITLEPAL not found, using PLAYPAL instead");
        COLpalAlt = (struct PIXEL *) Malloc(NCOLOURS * sizeof *COLpalAlt);
        for (n = 0; n < NCOLOURS; n++)
            COLpalAlt[n] = COLpal[n];
    } else {
        struct PIXEL *p;
        struct PIXEL *pmax;
        const unsigned char *d = (const unsigned char *) titlepal_data;
        const unsigned char *dmax = d + titlepal_size;

        if (titlepal_size < 3 * NCOLOURS)
            Warning("PL13", "TITLEPAL too short (%ld), filling with black",
                    (long) titlepal_size);
        COLpalAlt = (struct PIXEL *) Malloc(NCOLOURS * sizeof *COLpalAlt);
        /* Copy the contents of TITLEPAL into COLpalAlt */
        for (p = COLpalAlt, pmax = p + NCOLOURS; p < pmax; p++) {
            p->R = d < dmax ? *d++ : 0;
            p->G = d < dmax ? *d++ : 0;
            p->B = d < dmax ? *d++ : 0;
        }
        free(titlepal_data);
        titlepal_data = NULL;   /* Paranoia */
    }

    return COLpalAlt;
}
