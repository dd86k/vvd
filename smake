#!/bin/sh

if [ -z ${CC+x} ]; then
	CC=clang
fi
if [ -z ${CF+x} ]; then
	CF="-D_FILE_OFFSET_BITS=64 $2 $3 $4 -ferror-limit=2 -std=c99 -fpack-struct=1 -c"
fi

help()
{
	echo USAGE
	echo "  ./smake ACTION"
	echo
	echo ACTION
	echo "  build  Build binary"
	echo "  clean  Clean bin folder, vvd, and vvd.exe"
	exit
}

clean()
{
	rm -r bin/* vvd vvd.exe
	exit
}

build()
{
	mkdir -p bin
	for file in src/*.c src/**/*.c; do
		base=${file##*/}
		echo $CC: $base
		$CC $CF $file -o bin/${base%%.*}.obj
		if [ $? -ne 0 ]; then exit; fi
	done
	link
	exit
}

link()
{
	echo $CC: vvd
	$CC bin/*.obj -o vvd
	exit
}

if [ "$1" = "help" ]; then help; fi
if [ "$1" = "--help" ]; then help; fi
if [ "$1" = "clean" ]; then clean; fi
if [ "$1" = "build" ]; then build; fi
if [ "$1" = "link" ]; then link; fi

echo "ERROR: Action not found ($1)"