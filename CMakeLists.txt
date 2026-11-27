cmake_minimum_required(VERSION 3.21)
project(QuickKeyAIFoundation LANGUAGES CXX)
if(NOT APPLE)
  message(FATAL_ERROR "This foundation build targets macOS only.")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/openfx/include/ofxCore.h")
  message(FATAL_ERROR "Fetch the OpenFX SDK into third_party/openfx.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/bundles/QuickKeyAI.ofx.bundle")
file(MAKE_DIRECTORY "${BUNDLE_DIR}/Contents/MacOS")
configure_file("${CMAKE_SOURCE_DIR}/Info.plist.in" "${BUNDLE_DIR}/Contents/Info.plist" @ONLY)
add_library(QuickKeyAI MODULE src/QuickKeyAI.cpp)
target_include_directories(QuickKeyAI PRIVATE "${CMAKE_SOURCE_DIR}/third_party/openfx/include")
set_target_properties(QuickKeyAI PROPERTIES
 PREFIX ""
 SUFFIX ".ofx"
 LIBRARY_OUTPUT_DIRECTORY "${BUNDLE_DIR}/Contents/MacOS"
 POSITION_INDEPENDENT_CODE ON
 OSX_ARCHITECTURES "arm64"
)
target_compile_options(QuickKeyAI PRIVATE "-Wall" "-Wextra" "-Werror")
