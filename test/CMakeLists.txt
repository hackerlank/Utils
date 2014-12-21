cmake_minimum_required(VERSION 3.0)

project(UtilsTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
  gtest
  gtest/include
)

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  add_definitions(-D_DEBUG)
else()
  add_definitions(-DNDEBUG)
endif()

set(SOURCES
  char.cc
  charset.cc
  datatype.cc
  filepath.cc
  filesystem.cc
  math.cc
  others.cc
  string.cc
  unittest.cc
  ../cutil.h
  ../cutil.c
  ../ccutil.h
  ../ccutil.cpp
  gtest/src/gtest-all.cc
)

add_executable(UtilsTest
  ${SOURCES}
)

target_link_libraries(UtilsTest
  pthread
  iconv
)
