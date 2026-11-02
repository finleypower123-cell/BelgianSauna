cmake_minimum_required(VERSION 3.22)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build architectures")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS version")
endif()

project(BelgianSauna VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE 9.0.4 (includes Avid's AAX SDK). Use -DJUCE_DIR=/path/to/JUCE to build against a local copy.
if(DEFINED JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 9.0.4 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

set(SAUNA_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND SAUNA_FORMATS AU AAX)
elseif(WIN32)
    list(APPEND SAUNA_FORMATS AAX)
endif()

juce_add_plugin(BelgianSauna
    COMPANY_NAME "Ego Labs"
    COMPANY_WEBSITE "https://github.com/finleypower123-cell/BelgianSauna"
    PRODUCT_NAME "Belgian Sauna"
    PLUGIN_NAME "Belgian Sauna"
    BUNDLE_ID "com.egolabs.belgiansauna"
    PLUGIN_MANUFACTURER_CODE Eglb
    PLUGIN_CODE BSna
    FORMATS ${SAUNA_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "Belgian Sauna needs your microphone so you can sing into it."
    HARDENED_RUNTIME_ENABLED FALSE
    VST3_CATEGORIES Fx Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    AAX_CATEGORY AAX_ePlugInCategory_Reverb
    DISABLE_AAX_MULTI_MONO TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE)

set(AIRWINDOWS_UNITS
    Source/airwindows/Unit_Tube2.cpp
    Source/airwindows/Unit_Pressure4.cpp
    Source/airwindows/Unit_Air3.cpp
    Source/airwindows/Unit_ToTape6.cpp
    Source/airwindows/Unit_Ensemble.cpp
    Source/airwindows/Unit_TapeDelay2.cpp
    Source/airwindows/Unit_Galactic.cpp)

target_sources(BelgianSauna PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp ${AIRWINDOWS_UNITS})
target_include_directories(BelgianSauna PRIVATE Source)
target_compile_definitions(BelgianSauna PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 _USE_MATH_DEFINES)

target_link_libraries(BelgianSauna
    PRIVATE juce::juce_audio_utils juce::juce_audio_processors juce::juce_gui_basics
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# Airwindows' original code predates modern warning flags; don't bury real warnings under theirs.
if(MSVC)
    set_source_files_properties(${AIRWINDOWS_UNITS} PROPERTIES COMPILE_OPTIONS "/W0")
else()
    set_source_files_properties(${AIRWINDOWS_UNITS} PROPERTIES COMPILE_OPTIONS "-w")
endif()

add_executable(SaunaEngineTest tests/engine_test.cpp ${AIRWINDOWS_UNITS})
target_include_directories(SaunaEngineTest PRIVATE Source)
target_compile_definitions(SaunaEngineTest PRIVATE _USE_MATH_DEFINES)
