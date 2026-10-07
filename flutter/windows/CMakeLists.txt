cmake_minimum_required(VERSION 3.20)

# Project-level configuration.
set(PROJECT_NAME "inditrans")
project(${PROJECT_NAME} VERSION 0.13.0 LANGUAGES CXX)

# Invoke the build for native code shared with the other target platforms.
# This can be changed to accomodate different builds.
get_filename_component(INDITRANS_FLUTTER_PLUGIN_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
add_subdirectory("${INDITRANS_FLUTTER_PLUGIN_ROOT}/../../native/src" "${CMAKE_CURRENT_BINARY_DIR}/shared")

# List of absolute paths to libraries that should be bundled with the plugin.
# This list could contain prebuilt libraries, or libraries created by an
# external build triggered from this build file.
set(inditrans_bundled_libraries
  # Defined in ../../native/src/CMakeLists.txt.
  # This can be changed to accomodate different builds.
  $<TARGET_FILE:inditrans>
  PARENT_SCOPE
)
