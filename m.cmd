@ECHO OFF

IF /I "%1"=="clean" GOTO :CLEAN
IF /I "%1"=="help" GOTO :HELP
IF /I "%1"=="--help" GOTO :HELP
IF /I "%1"=="tips" GOTO :TIPS

IF NOT DEFINED CC SET CC=clang-cl
IF NOT DEFINED CF SET CF=/Zp -D_CRT_SECURE_NO_WARNINGS -Isrc -ferror-limit=2 -c

IF /I "%1"=="make" (
	CALL :MAKE %2 %3 %4 %5
	GOTO :EOF
)
IF /I "%1"=="build" (
	CALL :BUILD %2 %3 %4 %5
	GOTO :EOF
)
IF /I "%1"=="link" (
	CALL :LINK %2 %3 %4 %5
	GOTO :EOF
)

ECHO Unknown command: %1
GOTO :EOF

::
:: HELP
::

:HELP
ECHO USAGE
ECHO 	m	ACTION
ECHO.
ECHO ACTION
ECHO 	MAKE	Build and link binary
ECHO 	BUILD	Only build binary
ECHO 	LINK	Link binary
ECHO 	CLEAN	Clean bin folder, vvd, and vvd.exe
ECHO 	TIPS	Shows important compiler parameters
ECHO 	HELP	This help screen
GOTO :EOF

::
:: TIPS
::

:TIPS
ECHO clang-cl
ECHO 	/Os	Optimize for size
ECHO 	/Ot	Optimize for speed
ECHO 	/MT	Use static runtime
ECHO 	/MTd	Use static debug runtime
ECHO 	/MD	Use dynamic runtime
ECHO 	/MDd	Use dynamic debug runtime
ECHO clang
ECHO 	-On	Optimize to level n
GOTO :EOF

::
:: CLEAN
::

:CLEAN
DEL /S /Q bin\* vvd vvd.exe
GOTO :EOF

::
:: MAKE
::

:MAKE
CALL :BUILD %1 %2 %3 %4
CALL :LINK %1 %2 %3 %4
GOTO :EOF

::
:: BUILD
::

:BUILD
IF NOT EXIST "bin" mkdir bin
FOR /R src %%F IN (*.c) DO (
	ECHO %CC%: %%~nxF
	%CC% %CF% %%F %1 %2 %3 %4 -o bin\%%~nF.o
	IF ERRORLEVEL 1 EXIT /B 1
)
GOTO :EOF

::
:: LINK
::

:LINK
ECHO %CC%: vvd.exe
%CC% bin\*.o %1 %2 %3 %4 -o vvd.exe
GOTO :EOF
