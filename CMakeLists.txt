

cmake_minimum_required(VERSION 3.21)
project(plugperf LANGUAGES C CXX)

# =========================
# Build type resolution
# =========================
# If user didn't pass -DCMAKE_BUILD_TYPE, honor ENV{BUILD_TYPE} or default to Release
if(NOT CMAKE_BUILD_TYPE)
  if(DEFINED ENV{BUILD_TYPE})
    set(CMAKE_BUILD_TYPE "$ENV{BUILD_TYPE}" CACHE STRING "Build type" FORCE)
  else()
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
  endif()
endif()
message(STATUS "CMAKE_BUILD_TYPE = ${CMAKE_BUILD_TYPE}")

# =========================
# JUCE setup (prefer local SDK via env var, else FetchContent)
# =========================
include(FetchContent)
set(JUCE_FROM_ENV OFF)

if(DEFINED ENV{JUCE_SDK_PATH})
  set(_JUCE_ENV_PATH "$ENV{JUCE_SDK_PATH}")
  if(EXISTS "${_JUCE_ENV_PATH}/CMakeLists.txt")
    message(STATUS "Using JUCE from JUCE_SDK_PATH: ${_JUCE_ENV_PATH}")
    set(JUCE_FROM_ENV ON)
    add_subdirectory("${_JUCE_ENV_PATH}" JUCE)
  else()
    message(WARNING "JUCE_SDK_PATH is set but no CMakeLists.txt found at: ${_JUCE_ENV_PATH}. Falling back to FetchContent.")
  endif()
endif()

if(NOT JUCE_FROM_ENV)
  message(STATUS "Fetching JUCE from GitHub (fallback)...")
  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    # Pin to a stable tag; update as needed
    GIT_TAG 8.0.8
  )
  FetchContent_MakeAvailable(juce)
endif()

# =========================
# Target
# =========================
add_executable(plugperf
  src/main.cpp
  src/argparse.hpp
  src/csv.hpp
)

# C++ standard and warnings
set_target_properties(plugperf PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED YES)
if (MSVC)
  target_compile_options(plugperf PRIVATE /W4 /permissive-)
else()
  target_compile_options(plugperf PRIVATE -Wall -Wextra -Wpedantic)
endif()

# JUCE host flags: enable VST3 hosting (AU later)
target_compile_definitions(plugperf PRIVATE
  JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1
  JUCE_PLUGINHOST_VST3=1
  $<$<CONFIG:Debug>:JUCE_LOG_ASSERTIONS=1>
)

# Link against minimal JUCE modules required for a headless host
target_link_libraries(plugperf PRIVATE
  juce::juce_core
  juce::juce_audio_basics
  juce::juce_audio_formats
  juce::juce_audio_processors
)

# macOS specific: Encourage new runtime where applicable
if(APPLE)
  # Set a sensible deployment target if not already set
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Mac deployment target")
  endif()
  message(STATUS "CMAKE_OSX_DEPLOYMENT_TARGET = ${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

# =========================
# Install
# =========================
install(TARGETS plugperf RUNTIME DESTINATION bin)

# Install tools
install(PROGRAMS tools/visualize.py DESTINATION bin)
install(PROGRAMS tools/run_measure.py DESTINATION bin)

# Install documentation
install(FILES README.md DESTINATION share/doc/plugperf)
install(FILES backlog.md DESTINATION share/doc/plugperf)

# =========================
# Packaging
# =========================
set(CPACK_PACKAGE_NAME "PlugPerf")
set(CPACK_PACKAGE_VENDOR "PlugPerf")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "High-precision audio plugin performance benchmarking tool")
set(CPACK_PACKAGE_VERSION_MAJOR "1")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "0")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "PlugPerf")

# macOS specific
if(APPLE)
  set(CPACK_GENERATOR "DragNDrop;TGZ")
  set(CPACK_DMG_VOLUME_NAME "PlugPerf")
  set(CPACK_DMG_FORMAT "UDBZ")
  # Optional: set custom icon and background if resources exist
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icon.icns")
  endif()
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/resources/background.png")
    set(CPACK_DMG_BACKGROUND_IMAGE "${CMAKE_CURRENT_SOURCE_DIR}/resources/background.png")
  endif()
endif()

# Linux specific
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "TGZ;DEB")
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "PlugPerf")
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "")
endif()

# Windows specific
if(WIN32)
  set(CPACK_GENERATOR "ZIP;NSIS")
endif()

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

include(CPack)

# Helpful summary
if(JUCE_FROM_ENV)
  message(STATUS "JUCE source: ENV: ${_JUCE_ENV_PATH}")
else()
  message(STATUS "JUCE source: FetchContent: JUCE ${juce_SOURCE_DIR}")
endif()