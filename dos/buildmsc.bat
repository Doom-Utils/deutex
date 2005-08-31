@echo off
rem buildmsc.bat - build DeuTex with MSC
rem AYM 1999-09-09

cd src
echo Generating config.h
echo #undef HAVE_INTTYPES		>config.h
echo #undef HAVE_SNPRINTF		>>config.h
echo typedef signed char    int8_t;	>>config.h
echo typedef short          int16_t;	>>config.h
echo typedef long           int32_t;	>>config.h
echo typedef unsigned char  uint8_t;	>>config.h
echo typedef unsigned short uint16_t;	>>config.h
echo typedef unsigned long  uint32_t;	>>config.h
echo Building DeuTex
cl -AH -W2 -DDeuTex -Fe..\deutex.exe *.c
echo Building DeuSF
cl -AH -W2 -DDeuSF  -Fe..\deusf.exe  *.c
cd ..
