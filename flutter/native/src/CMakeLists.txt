# The Flutter tooling requires that developers have CMake 3.10 or later
# installed. You should not increase this version, as doing so will cause
# the plugin to fail to compile for some customers of the plugin.
cmake_minimum_required(VERSION 3.10)

project(inditrans_library VERSION 0.0.6 LANGUAGES CXX)

# set(CMAKE_CXX_STANDARD 17)
# set_property(TARGET inditrans PROPERTY CXX_STANDARD 20)
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fPIC")
set(CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)

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
IF (WIN32)
  add_definitions(-DMAKE_DLL)
ENDIF()

target_compile_definitions(inditrans PUBLIC DART_SHARED_LIB)
