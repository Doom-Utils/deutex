@echo off
rem buildbc.bat - build DeuTex with Borland C++
rem AYM 1999-09-09

if "%1" == "" goto usage
if not "%2" == "" goto usage
set bcdir=%1

cd src
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
