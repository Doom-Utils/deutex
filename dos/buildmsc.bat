@echo off
rem buildmsc.bat - build DeuTex with MSC
rem AYM 1999-09-09

cd src
echo Building DeuTex
cl -AH -W2 -DDeuTex -Fe..\deutex.exe *.c
echo Building DeuSF
cl -AH -W2 -DDeuSF  -Fe..\deusf.exe  *.c
cd ..
