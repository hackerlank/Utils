#!/bin/sh

g++ -o testcpp ../../cutil.c ../../ccutil.cpp ../../lib/string16.cpp ../../lib/ConvertUTF.c ../cppstring.cpp  ../test-cpp.cpp test.cpp -liconv -lpthread -lcunit -D_DEBUG -D_GNU_SOURCE

./testcpp

if [ -f "TestCpp-Results.xml" ];then
  firefox "TestCpp-Results.xml" &
fi

exit 0
