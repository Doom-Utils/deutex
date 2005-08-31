/*
 *	log.h
 *	AYM 2001-02-02
 */


int lopen (void);
void lputc (char c);
void lputs (const char *str);
void lprintf (const char *fmt, ...);
void vlprintf (const char *fmt, va_list list);
