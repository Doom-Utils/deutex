/*
  Copyright ® Olivier Montanuy,
              André Majorel,
              contributors to the DeuTex project.

  DeuTex incorporates code derived from DEU 5.21 that was put in the
  public domain in 1994 by Raphaël Quinet and Brendon Wyber.

  SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef TOOLS_H
#define TOOLS_H

#define MSGCLASS_INFO 'i'
#define MSGCLASS_WARN 'w'
#define MSGCLASS_ERR  'E'
#define MSGCLASS_BUG  'B'

void PrintCopyright(void);
void print_version(void);
void NoCommandGiven(void);

char *fnameofs(const char *name, long ofs);
char *fname(const char *name);
char *lump_name(const char *name);
char *short_dump(const char *data, size_t size);
const char *quotechar(char c);

void PrintVerbosity(int16_t level);
void PrintExit(void);

void ProgErrorCancel(void);
void ProgErrorAction(void (*action) (void));
void ProgError(const char *code, const char *fmt, ...); /*fatal error. halt. */
void nf_err(const char *code, const char *fmt, ...);    /* Non-fatal error */
void Bug(const char *code, const char *fmt, ...);       /*fatal bug. halt */
void Warning(const char *code, const char *str, ...);   /*I'm not happy */
void LimitedWarn(int *left, const char *code, const char *fmt, ...);
void LimitedEpilog(int *left, const char *code, const char *fmt, ...);
void Output(const char *fmt, ...);      /*command text output */
void Info(const char *code, const char *fmt, ...);      /*useful indications */
void Phase(const char *code, const char *fmt, ...);     /*phase of executions */
void Detail(const char *code, const char *fmt, ...);    /*technical details */

void ToLowerCase(char *file);
void MakeDir(char file[128], const char *path, const char *dir, const char
             *sdir);
bool MakeFileName(char file[128], const char *path, const char *dir, const
                  char *sdir, const char *name, const char *extens);
void GetNameOfWAD(char name[8], const char *path);
int32_t Get_File_Time(const char *path);
void Set_File_Time(const char *path, int32_t time);
void Memcpy(void *dest, const void *src, long n);
void Memset(void *dest, char car, long n);
void *Malloc(long size);
void *Realloc(void *old, long size);

void Normalise(char dest[8], const char *src);

void Progress(void);
void ProgressEnds(void);

#endif //TOOLS_H
