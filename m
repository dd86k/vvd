#!/bin/sh

m_help()
{
	echo USAGE
	echo "\t./m\tACTION"
	echo
	echo ACTION
	echo "\tmake\tBuild and link binary"
	echo "\tbuild\tOnly build binary"
	echo "\tlink\tLink binary"
	echo "\tclean\tClean bin folder, vvd, and vvd.exe"
	echo "\ttips\tShows important compiler parameters"
	echo "\thelp\tThis help screen"
	exit
}

m_tips()
{
	echo clang
}

m_clean()
{
	rm -r bin/* vvd vvd.exe
	exit
}

m_make()
{
	m_build $1 $2 $3 $4
	m_link $1 $2 $3 $4
}

m_build()
{
	mkdir -p bin
	for file in src/*.c src/**/*.c; do
		base=${file##*/}
		echo $CC: $base
		$CC $CF $file $1 $2 $3 $4 -o bin/${base%%.*}.obj
		if [ $? -ne 0 ]; then exit; fi
	done
}

m_link()
{
	echo $CC: vvd
	$CC bin/*.obj $1 $2 $3 $4 -o vvd
}

if [ "$1" = "clean" ]; then m_clean; fi
if [ "$1" = "help" ]; then m_help; fi
if [ "$1" = "--help" ]; then m_help; fi
if [ "$1" = "tips" ]; then m_tips; fi

if [ -z ${CC+x} ]; then
	CC=clang
fi
if [ -z ${CF+x} ]; then
	CF="-D_FILE_OFFSET_BITS=64 -ferror-limit=2 -std=c99 -fpack-struct=1 -c"
fi

if [ "$1" = "make" ]; then m_make $2 $3 $4 $5; exit; fi
if [ "$1" = "build" ]; then m_build $2 $3 $4 $5; exit; fi
if [ "$1" = "link" ]; then m_link $2 $3 $4 $5; exit; fi

echo "ERROR: Action not found ($1)"