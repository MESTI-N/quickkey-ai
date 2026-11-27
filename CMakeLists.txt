cmake_minimum_required(VERSION 3.21)
project(QuickKeyAIMac LANGUAGES CXX)
if(NOT APPLE)
  message(FATAL_ERROR "QuickKey requires Apple Silicon macOS.")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/openfx/include/ofxCore.h")
  message(FATAL_ERROR "Fetch AcademySoftwareFoundation/openfx to third_party/openfx.")
endif()
if(NOT EXISTS "${CMAKE_SOURCE_DIR}/third_party/tinyexr/tinyexr.h")
  message(FATAL_ERROR "Fetch syoyo/tinyexr to third_party/tinyexr.")
endif()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ZLIB REQUIRED)
set(BUNDLE_DIR "${CMAKE_BINARY_DIR}/bundles/QuickKeyAI.ofx.bundle")
file(MAKE_DIRECTORY "${BUNDLE_DIR}/Contents/MacOS")
file(MAKE_DIRECTORY "${BUNDLE_DIR}/Contents/Resources")
configure_file("${CMAKE_SOURCE_DIR}/Info.plist.in" "${BUNDLE_DIR}/Contents/Info.plist" @ONLY)
configure_file("${CMAKE_SOURCE_DIR}/scripts/quickkey-worker.sh" "${BUNDLE_DIR}/Contents/Resources/quickkey-worker.sh" COPYONLY)
add_library(QuickKeyAI MODULE src/QuickKeyAI.cpp)
target_include_directories(QuickKeyAI PRIVATE
  "${CMAKE_SOURCE_DIR}/third_party/openfx/include"
  "${CMAKE_SOURCE_DIR}/third_party/tinyexr")
target_link_libraries(QuickKeyAI PRIVATE ZLIB::ZLIB)
set_target_properties(QuickKeyAI PROPERTIES
 PREFIX ""
 SUFFIX ".ofx"
 LIBRARY_OUTPUT_DIRECTORY "${BUNDLE_DIR}/Contents/MacOS"
 POSITION_INDEPENDENT_CODE ON
 OSX_ARCHITECTURES "arm64"
)
target_compile_options(QuickKeyAI PRIVATE "-Wall" "-Wextra" "-Werror")
