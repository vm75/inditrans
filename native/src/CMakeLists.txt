cmake_minimum_required(VERSION 3.20)

project(inditrans_library VERSION 0.0.6 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (
    MINGW
    AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
    AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0"
)
  message(
      FATAL_ERROR
      "MinGW GCC ${CMAKE_CXX_COMPILER_VERSION} does not meet the minimum C++23 requirement (GCC 13+)."
  )
endif()

add_library(inditrans SHARED
  "inditrans.cpp"
)

set_target_properties(inditrans PROPERTIES
  PUBLIC_HEADER inditrans.h
  OUTPUT_NAME "inditrans"
)

add_definitions(-D_CRT_SECURE_NO_WARNINGS)
# add_definitions(-D__DART__)
# add_definitions(-DCMAKE_CXX_COMPILER=clang-cl)
if (WIN32)
  add_definitions(-DMAKE_DLL)
  set_target_properties(inditrans PROPERTIES PREFIX "")
  if (MINGW)
    target_link_options(inditrans PRIVATE -static-libgcc -static-libstdc++)
  elseif (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8")
  endif()
endif()

target_compile_definitions(inditrans PUBLIC DART_SHARED_LIB)
