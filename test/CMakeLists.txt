cmake_minimum_required(VERSION 3.0)

project(UtilsTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
  gtest
  gtest/include
)

if(WIN32 AND MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /DUNICODE /D_UNICODE /DNOMINMAX /MP4 /EHsc")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /DNDEBUG /MD")
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS} /DNDEBUG /MD")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D_DEBUG /MDd /ZI /Od")
else()
if(CMAKE_BUILD_TYPE STREQUAL Debug) 
  add_definitions(-D_DEBUG -g -Wall -fprofile-arcs -ftest-coverage)
else()
  add_definitions(-DNDEBUG)
endif()
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

if(WIN32)
if (MSVC)
  set(ICONV_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/iconv/win32")
  if(CMAKE_BUILD_TYPE STREQUAL Debug)
    set(ICONV_LIB ${ICONV_LIB_DIR}/iconvd.lib)
  else()
    set(ICONV_LIB ${ICONV_LIB_DIR}/iconv.lib)
  endif() # Debug
else()
  set(ICONV_LIB iconv) # MINGW
endif() # MSVC

target_link_libraries(UtilsTest
  ${ICONV_LIB}
  dbghelp
  shlwapi
  user32
  kernel32
  shell32
)

if(MINGW)
target_link_libraries(UtilsTest gcov)
endif()

# POSIX
else()
target_link_libraries(UtilsTest
  pthread
  iconv
)

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#Mac OS X
else()
#Linux
target_link_libraries(UtilsTest gcov)
endif()

endif() # WIN32
