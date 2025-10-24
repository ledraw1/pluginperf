cmake_minimum_required(VERSION 3.21)
project(SyntheticTestPlugin VERSION 1.0.0)

# JUCE setup
include(FetchContent)
set(JUCE_FROM_ENV OFF)

if(DEFINED ENV{JUCE_SDK_PATH})
  set(_JUCE_ENV_PATH "$ENV{JUCE_SDK_PATH}")
  if(EXISTS "${_JUCE_ENV_PATH}/CMakeLists.txt")
    message(STATUS "Using JUCE from JUCE_SDK_PATH: ${_JUCE_ENV_PATH}")
    set(JUCE_FROM_ENV ON)
    add_subdirectory("${_JUCE_ENV_PATH}" JUCE)
  endif()
endif()

if(NOT JUCE_FROM_ENV)
  message(STATUS "Fetching JUCE from GitHub...")
  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
  )
  FetchContent_MakeAvailable(juce)
endif()

# Plugin definition
juce_add_plugin(SyntheticTestPlugin
    COMPANY_NAME "PlugPerf"
    PLUGIN_MANUFACTURER_CODE Plpf
    PLUGIN_CODE Synt
    FORMATS VST3
    PRODUCT_NAME "Synthetic Test Plugin"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
)

target_sources(SyntheticTestPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
)

target_compile_definitions(SyntheticTestPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(SyntheticTestPlugin PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
)
