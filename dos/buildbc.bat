@echo off
rem buildbc.bat - build DeuTex with Borland C++
rem AYM 1999-09-09

if "%1" == "" goto usage
if not "%2" == "" goto usage
set bcdir=%1

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
bcc -mh -I%bcdir%include -L%bcdir%lib -DDeuTex -e..\deutex.exe *.c
echo Building DeuSF
bcc -mh -I%bcdir%include -L%bcdir%lib -DDeuSF  -e..\deusf.exe  *.c
cd ..
goto end

:usage
echo Usage: buildbc (dir)
echo (dir)  The directory where Borland C is installed,
echo        followed by a backslash (E.G. "c:\bc4\").

:end
