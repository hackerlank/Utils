#!/bin/sh

gcc -o testc ../../cutil.c ../../lib/ConvertUTF.c ../../lib/sqlite3.h ../../lib/sqlite3.c ../charset.c ../datatype.c ../filesystem.c ../math.c ../string.c ../others.h ../others.c ../sqlite.h ../sqlite.h ../sqlite.c ../zlibtest.h ../zlibtest.c ../test-c.c test.c -I/usr/local/include -L/usr/local/lib -liconv -lpthread -lcunit -lz -ldl -D_DEBUG -D_GNU_SOURCE

./testc

if [ -f "TestC-Results.xml" ];then
  firefox "TestC-Results.xml" &
fi

exit 0
