@echo off
rem builddj.bat - build DeuTex with DJGPP 2
rem AYM 2005-08-19

echo Generating Makefile
echo AWK               = awk		>Makefile
echo BINDIR            = /usr/local/bin	>>Makefile
echo CC                = cc		>>Makefile
echo CFLAGS            = -O2 -Wall	>>Makefile
echo HAVE_INTTYPES     =		>>Makefile
echo HAVE_SNPRINTF     =		>>Makefile
echo LDFLAGS           = 		>>Makefile
echo MANDIR            = /usr/local/man	>>Makefile
type Makefile.in >>Makefile
echo Generating config.h
echo #undef HAVE_INTTYPES		>src\config.h
echo #undef HAVE_SNPRINTF		>>src\config.h
echo typedef signed char    int8_t;	>>src\config.h
echo typedef short          int16_t;	>>src\config.h
echo typedef long           int32_t;	>>src\config.h
echo typedef unsigned char  uint8_t;	>>src\config.h
echo typedef unsigned short uint16_t;	>>src\config.h
echo typedef unsigned long  uint32_t;	>>src\config.h
echo Building DeuTex and DeuSF
bash -c 'make SHELL=`type -p bash`'
