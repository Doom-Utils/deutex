/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/

/*for merging directories*/
void WADRopenPipo(struct WADINFO *info, int32_t ntry);
struct WADDIR *WADRclosePipo(struct WADINFO *info, int32_t * ntry);
int32_t WADRdirAddPipo(struct WADINFO *info, int32_t start, int32_t size, const char
                       *entry);

/*Open a WAD file for read*/
void WADRopenR(struct WADINFO *info, const char *wadin);
/*Open a WAD file for write*/
void WADRopenW(struct WADINFO *info, const char *wadout, WADTYPE type,
               int verbose);
/*Open a WAD file for append*/
void WADRopenA(struct WADINFO *info, const char *wadinout);
/*Close a WAD file*/
void WADRclose(struct WADINFO *info);

/*WAD file structure*/
/*set position of internal WAD directory*/
void WADRsetDirRef(struct WADINFO *info, int32_t ntry, int32_t dirpos);
/*change size of a WAD*/
void WADRchsize(struct WADINFO *info, int32_t fsize);
/*increase size of WAD, do not update position*/
bool WADRchsize2(struct WADINFO *info, int32_t fsize);

/*composition of internal WAD directory*/
/*add an entry to the directory*/
int32_t WADRdirAddEntry(struct WADINFO *info, int32_t start, int32_t size, const char
                        *name);
/*write the directory (and set it's position)*/
void WADRwriteDir(struct WADINFO *info, int verbose);
/*find an entry in the directory*/
int16_t WADRfindEntry(struct WADINFO *info, const char *entry);  /*-1 or index of entry in directory*/

/*set data in a WAD (write position doesn't change)*/
void WADRsetLong(struct WADINFO *info, int32_t pos, int32_t val);
void WADRsetShort(struct WADINFO *info, int32_t pos, int16_t val);
/*align on long*/
void WADRalign4(struct WADINFO *info);  /*align on long word, for next entry */
/*tell position of pointer*/
int32_t WADRposition(struct WADINFO *info);     /*current position */
/*write date (write position increase)*/
int32_t WADRwriteLong(struct WADINFO *info, int32_t val);
int32_t WADRwriteShort(struct WADINFO *info, int16_t val);
int32_t WADRwriteBytes(struct WADINFO *info, char *data, int32_t size);
int32_t WADRwriteBytes2(struct WADINFO *info, char *data, int32_t size);
int32_t WADRwriteLump(struct WADINFO *info, const char *file);
int32_t WADRwriteWADbytes(struct WADINFO *info, struct WADINFO *src,
                          int32_t start, int32_t size);
int32_t WADRwriteWADentry(struct WADINFO *info, struct WADINFO *src,
                          int16_t n);
void WADRwriteWADlevelParts(struct WADINFO *info, struct WADINFO *src,
                            int16_t n, size_t nlumps);
void WADRwriteWADlevel(struct WADINFO *info, const char *file, const char
                       *level);

/*read data*/
void WADRseek(struct WADINFO *info, int32_t position);
iolen_t WADRreadBytes(struct WADINFO *info, char *buffer, iolen_t nbytes);
iolen_t WADRreadBytes(struct WADINFO *info, char *buffer, iolen_t nbytes);
int16_t WADRreadShort(struct WADINFO *info);
int32_t WADRreadLong(struct WADINFO *info);
char *WADRreadEntry(struct WADINFO *info, int16_t N, int32_t * psize);
char *WADRreadEntry2(struct WADINFO *info, int16_t n, int32_t * psize);
void WADRsaveEntry(struct WADINFO *info, int16_t N, const char *file);

/*make some preparations before appending data to an existing WAD*/
/*so that it can be restored later*/
int32_t WADRprepareAppend(const char *wadres, struct WADINFO *rwad, struct WADDIR
                          *NewDir, int32_t NewNtry, int32_t * dirpos,
                          int32_t * ntry, int32_t * size);
