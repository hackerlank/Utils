cmake_minimum_required(VERSION 3.0)

project(UtilsTest)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

include_directories(
  gtest
  gtest/include
)

if(WIN32)
  add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DMP4)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} /DNDEBUG /MD /GL")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} /D_DEBUG /MDd /ZI /Od")
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG /OPT:REF")
else() # WIN32
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
if(CMAKE_BUILD_TYPE STREQUAL Debug) 
  add_definitions(-D_DEBUG -g -Wall -fprofile-arcs -ftest-coverage)
else()
  add_definitions(-DNDEBUG)
endif()
endif() # WIN32

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-invalid-source-encoding")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-source-encoding")
endif()

set(SOURCES
  char.cc
  charset.cc
  datatype.cc
  directory.cc
  file.cc
  filepath.cc
  filesystem.cc
  logging.cc
  math.cc
  others.cc
  string.cc
  version.cc
  main.cc
  ../cutil.h
  ../cutil.c
  ../ccutil.h
  ../ccutil.cpp
  gnu-md5.h
  gnu-md5.c
  gtest/src/gtest-all.cc
)

add_executable(UtilsTest
  ${SOURCES}
)

if(WIN32)
  if (MSVC)
    set(ICONV_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../third_party/iconv/win32")
    set(ICONV_LIB optimized ${ICONV_LIB_DIR}/iconv.lib debug ${ICONV_LIB_DIR}/iconvd.lib)
  else()
    set(ICONV_LIB iconv)
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
  
else() # WIN32
  target_link_libraries(UtilsTest
    pthread
    iconv
  )
  
  if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    #Mac OS X
    target_link_libraries(UtilsTest --coverage) 
  else()
    #Linux
    target_link_libraries(UtilsTest gcov)
  endif()
endif() # WIN32

add_custom_command(TARGET UtilsTest POST_BUILD
  COMMAND cp -r data "$<TARGET_FILE_DIR:UtilsTest>"
  WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
