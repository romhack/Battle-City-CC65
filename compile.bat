@echo off

:build
cls

REM Configuration
set src=src
set name=main
set config=nrom_32k_horz.cfg

REM --- COMPILER FLAGS ---
REM -t nes       : Target NES (defines __NES__, correct charmap)
REM -Oirs        : Optimize, Inline runtime, Register vars, Inline std funcs
REM -Cl          : Static locals (moves locals off slow software stack)
REM --add-source : Adds C source as comments in the .s file
REM -g           : Debug info
set CC_FLAGS=-t nes -Oirs -Cl --add-source -g

REM Create BUILD directory if it doesn't exist
if not exist build mkdir build

echo Compiling C source...
cc65 %CC_FLAGS% %src%\globals.c
if errorlevel 1 goto error

cc65 %CC_FLAGS% %src%\util.c
if errorlevel 1 goto error

cc65 %CC_FLAGS% %src%\%name%.c
if errorlevel 1 goto error

cc65 %CC_FLAGS% %src%\levels.c
if errorlevel 1 goto error

echo Assembling startup code...
ca65 crt0.s -g
if errorlevel 1 goto error

echo Assembling sound engine...
ca65 %src%\sound.s -g
if errorlevel 1 goto error

echo Assembling generated code...
ca65 %src%\globals.s -g
ca65 %src%\util.s -g
ca65 %src%\%name%.s -g
ca65 %src%\levels.s -g
if errorlevel 1 goto error

echo Linking...
ld65 -C %config% -m %name%.map -o %name%.nes crt0.o %src%\sound.o %src%\globals.o %src%\util.o %src%\levels.o %src%\%name%.o nes.lib -Ln labels.txt --dbgfile %name%.dbg
if errorlevel 1 goto error

REM Cleanup and organize
echo.
echo Cleaning up...
del %src%\*.o
del *.o
move /Y labels.txt build\ >nul
move /Y %src%\globals.s build\ >nul
move /Y %src%\levels.s build\ >nul
move /Y %src%\main.s build\ >nul
move /Y %src%\util.s build\ >nul

move /Y %name%.nes build\ >nul
move /Y %name%.dbg build\ >nul
move /Y %name%.map build\ >nul

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