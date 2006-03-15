/*
 *	log.c
 *	AYM 2001-02-02
 */

/*
This file is copyright André Majorel 2001-2005.

This program is free software; you can redistribute it and/or modify it under
the terms of version 2 of the GNU General Public License as published by the
Free Software Foundation.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/


#include <stdarg.h>
#include <errno.h>
#include "deutex.h"
#include "log.h"
#include "tools.h"


static FILE *logfp = NULL;
static FILE nolog;


/*
 *	lopen - open the log file if necessary
 *
 *	Return 0 if logfp can be written to, non-zero otherwise.
 */
int lopen (void)
{
  if (logfp == &nolog)
    return 1;
  if (logfp == NULL || logfp == &nolog)
  {
    logfp = fopen (logfile, "w");
    if (logfp == NULL || logfp == &nolog)
    {
      /* Can't use Warning(), we would loop. */
      fflush (stdout);
      fprintf (stderr, "%c LG10 %s: %s\n",
	  MSGCLASS_WARN, logfile, strerror (errno));
      logfp = &nolog;
      return 1;
    }
  }
  return 0;
}


/* No lclose(). Bah. */


/*
 *	lputc - write a character into the log file
 */
void lputc (char c)
{
  if (logfp == NULL || logfp == &nolog)
    return;
  fputc (c, logfp);
  fflush (logfp);  /* We don't want a segfault to truncate the log */
}


/*
 *	lputs - write a string into the log file
 */
void lputs (const char *str)
{
  if (logfp == NULL || logfp == &nolog)
    return;
  fputs (str, logfp);
  fflush (logfp);  /* We don't want a segfault to truncate the log */
}


/*
 *	lprintf - printf into the log file
 */
void lprintf (const char *fmt, ...)
{
  va_list list;

  if (logfp == NULL || logfp == &nolog)
    return;
  va_start (list, fmt);
  vlprintf (fmt, list);
  va_end (list);
}


/*
 *	vlprintf - vprintf into the log file
 */
void vlprintf (const char *fmt, va_list list)
{
  if (logfp == NULL || logfp == &nolog)
    return;
  vfprintf (logfp, fmt, list);
  fflush (logfp);  /* We don't want a segfault to truncate the log */
}


