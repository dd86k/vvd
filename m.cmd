@ECHO OFF
::
:: ** CL CLI reminder **
:: optimize size: /Os
:: optimize speed: /Ot
:: compile dynamic: /MD
:: compile static: /MT
::

IF "%CC%"=="" SET CC=clang-cl
if "%CF%"=="" SET CF=/Zp -D_CRT_SECURE_NO_WARNINGS -ferror-limit=2 %2 %3 %4 %5 -c

IF /I "%1"=="clean" GOTO :CLEAN
IF /I "%1"=="help" GOTO :HELP
IF /I "%1"=="--help" GOTO :HELP
IF /I "%1"=="build" (
	CALL :BUILD
	GOTO :EOF
)
IF /I "%1"=="link" (
	CALL :LINK
	GOTO :EOF
)

ECHO Unknown command: %1
GOTO :EOF

:HELP
ECHO USAGE
ECHO   smake ACTION
ECHO.
ECHO ACTION
ECHO   BUILD       Build binary
ECHO   CLEAN       Clean bin folder, vvd, and vvd.exe
GOTO :EOF

::
:: CLEAN
::

:CLEAN
DEL /S /Q bin\* vvd vvd.exe
GOTO :EOF

::
:: BUILD
::

:BUILD
IF NOT EXIST "bin" mkdir bin
FOR /R src %%F IN (*.c) DO (
	ECHO %CC%: %%~nxF
	%CC% %CF% %%F -o bin\%%~nF.o
	IF ERRORLEVEL 1 EXIT 1
)
CALL :LINK
GOTO :EOF

::
:: LINK
::

:LINK
ECHO %CC%: vvd.exe
%CC% bin\*.o -o vvd.exe
GOTO :EOF
