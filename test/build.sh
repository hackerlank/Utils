#!/bin/sh

g++ *.cc ../ccutil.cpp ../cutil.c gtest/src/gtest-all.cc -D_DEBUG -I gtest/include/ -I gtest/ -liconv
