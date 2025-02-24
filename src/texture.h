/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/** PNM module **/
void PNMinit(char *buffer, int32_t size);
int16_t PNMindexOfPatch(char *patch);
        /* check if patch exists. for ident.c */
int32_t PNMwritePNAMEtoWAD(struct WADINFO *info);
        /* write PNAME entry in Wad */
void PNMfree(void);
int16_t PNMgetNbOfPatch(void);
        /* compose.c  get nb of patches */
void PNMgetPatchName(char name[8], int16_t index);
        /* compose.c  returns name of patch, from index */
bool PNMisNew(int16_t idx);
        /* compose.c is the patch not in IWAD? */
/** TXU module **/
void TXUinit(void);             /*requires the the patches are init */
void TXUfree(void);
bool TXUexist(char *Name);
        /*does it exist? */
int32_t TXUwriteTEXTUREtoWAD(struct WADINFO *info);
        /*write the TEXTURE entry in Wad */
void TXUreadTEXTURE(const char *texture1_name, const char *Data,
                    int32_t DataSz, const char *Patch, int32_t PatchSz,
                    bool Redefn);
        /*read texture from raw data = TEXTURE entry */
void TXUwriteTexFile(const char *file);
        /*write the TEXTURE entry to a file */
void TXUreadTexFile(const char *file, bool Redefn);
        /*checkif the composition of textues in okay */
bool TXUcheckTex(int16_t npatch, int16_t * PszX);
        /*declare a fake texure. to list textures */
void TXUfakeTex(char Name[8]);
        /*list all defined textures */
void TXUlistTex(void);
