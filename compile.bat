@echo off

:build
cls

REM Configuration
set src=src
set name=main
set config=nrom_16k_horz.cfg

REM Create BUILD directory if it doesn't exist
if not exist build mkdir build

REM Compilation steps
echo Compiling C source...
cc65 -Oirs %src%\globals.c --add-source -g
cc65 -Oirs %src%\util.c --add-source -g
cc65 -Oirs %src%\%name%.c --add-source -g
cc65 -Oirs %src%\levels.c --add-source -g

if errorlevel 1 goto error

echo Assembling startup code...
ca65 crt0.s -g
if errorlevel 1 goto error

echo Assembling generated code...
ca65 %src%\globals.s -g
ca65 %src%\util.s -g
ca65 %src%\%name%.s -g
ca65 %src%\levels.s -g
if errorlevel 1 goto error

echo Linking...
ld65 -C %config% -o %name%.nes crt0.o %src%\globals.o %src%\util.o %src%\levels.o %src%\%name%.o nes.lib -Ln labels.txt --dbgfile %name%.dbg
if errorlevel 1 goto error

REM Cleanup and organize
echo.
echo Cleaning up...
del %src%\*.o
del *.o
move /Y labels.txt build\ >nul
move /Y %src%\*.s build\ >nul
move /Y %name%.nes build\ >nul
move /Y %name%.dbg build\ >nul

REM Success
echo.
echo ================================
echo SUCCESS! ROM: build\%name%.nes
echo ================================
echo.
echo Press R to rebuild, any other key to run and exit...
choice /c RX /n /m ""
if errorlevel 2 goto run
if errorlevel 1 goto build

:error
echo.
echo ================================
echo COMPILATION FAILED!
echo ================================
echo.
echo Press R to retry, any other key to exit...
choice /c RX /n /m ""
if errorlevel 2 goto end
if errorlevel 1 goto build

:run
BUILD\%name%.nes

:end