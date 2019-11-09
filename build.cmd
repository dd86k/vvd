@ECHO OFF
::
:: ** CL CLI reminder **
:: optimize size: /Os
:: optimize speed: /Ot
:: compile dynamic: /MD
:: compile static: /MT
::
:: Don't forget setargv_*.obj
::
::TODO: Function for every file being compiled

SET CC=%1
SET CTIME="\"%DATE% %TIME%\""

IF /I "%CC%"=="clang-cl" GOTO :CLANGCL
IF /I "%CC%"=="clean" GOTO :CLEAN
IF /I "%CC%"=="help" GOTO :SHOW_HELP
IF /I "%CC%"=="" (
	SET CC=clang-cl
	GOTO :CLANGCL
)

ECHO ERROR: Action not found (%CC%)
GOTO :EOF

:SHOW_HELP
ECHO USAGE
ECHO   build ACTION [OPTIONS]
ECHO.
ECHO ACTION
ECHO   ^<COMPILER^>  Choose a compiler (clang, clang-cl)
ECHO   CLEAN       Delete binaries
GOTO :EOF

::
:: CLEAN
::

:CLEAN
DEL /S /Q bin\* vvd vvd.exe
GOTO :EOF

::
:: clang-cl
::

:CLANGCL
SET CFLAGS=%CC% -c %2 %3 %4 %5 /Zp -DTIMESTAMP=%CTIME% -D_CRT_SECURE_NO_WARNINGS -ferror-limit=2
ECHO [%CC%] main.o
%CFLAGS% src\main.c -o bin\main.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] guid.o
%CFLAGS% src\guid.c -o bin\guid.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] utils.o
%CFLAGS% src\utils.c -o bin\utils.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] vdisk.o
%CFLAGS% src\vdisk.c -o bin\vdisk.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] vvd.o
%CFLAGS% src\vvd.c -o bin\vvd.o
IF ERRORLEVEL 1 GOTO :EOF
:: os
ECHO [%CC%] fs.o
%CFLAGS% src\os\fs.c -o bin\fs.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] err.o
%CFLAGS% src\os\err.c -o bin\err.o
IF ERRORLEVEL 1 GOTO :EOF
:: fs
ECHO [%CC%] mbr.o
%CFLAGS% src\fs\mbr.c -o bin\mbr.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] gpt.o
%CFLAGS% src\fs\gpt.c -o bin\gpt.o
IF ERRORLEVEL 1 GOTO :EOF
:: vdisk
ECHO [%CC%] vdi.o
%CFLAGS% src\vdisk\vdi.c -o bin\vdi.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] vmdk.o
%CFLAGS% src\vdisk\vmdk.c -o bin\vmdk.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] vhd.o
%CFLAGS% src\vdisk\vhd.c -o bin\vhd.o
IF ERRORLEVEL 1 GOTO :EOF
ECHO [%CC%] vhdx.o
%CFLAGS% src\vdisk\vhdx.c -o bin\vhdx.o
IF ERRORLEVEL 1 GOTO :EOF
:: LINK
ECHO [%CC%] Linking vvd
%CC% %2 %3 %4 %5 bin\*.o -o vvd.exe
GOTO :EOF
