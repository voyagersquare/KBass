cmake_minimum_required(VERSION 3.22)
project(KBass VERSION 0.3.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(KBASS_DSP_ONLY "Build portable audio engine tests without JUCE" OFF)
option(KBASS_BUILD_TESTS "Build audio and processor checks" ON)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(KBASS_BUILD_TESTS OR KBASS_DSP_ONLY)
    enable_testing()
    add_executable(KBassDspTests Tests/DspTests.cpp)
    target_include_directories(KBassDspTests PRIVATE Source)
    add_test(NAME synth-engine COMMAND KBassDspTests)
    add_executable(KBassSaturationTests Tests/SaturationTests.cpp)
    target_include_directories(KBassSaturationTests PRIVATE Source)
    add_test(NAME saturation-dsp COMMAND KBassSaturationTests)
endif()
if(KBASS_DSP_ONLY)
    return()
endif()
include(FetchContent)
FetchContent_Declare(JUCE
    URL https://github.com/juce-framework/JUCE/archive/51a8a6d7aeae7326956d747737ccf1575e61e209.tar.gz
    URL_HASH SHA256=20b740351cc030086951d87614bc08b42d072f1e4db68db1ef1d988e66dbbdbe
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(KBass COMPANY_NAME "Independent Audio" BUNDLE_ID "audio.voyagersquare.kbass"
    PLUGIN_MANUFACTURER_CODE VySq PLUGIN_CODE KBss PRODUCT_NAME "KBass"
    FORMATS VST3 IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES Instrument Synth COPY_PLUGIN_AFTER_BUILD FALSE)
juce_generate_juce_header(KBass)
target_sources(KBass PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp Source/UserPresets.cpp Source/PresetBrowser.cpp)
target_compile_definitions(KBass PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0 JUCE_REPORT_APP_USAGE=0
    JUCE_USE_MP3AUDIOFORMAT=0)
target_link_libraries(KBass PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)
if(MSVC)
    target_compile_options(KBass PRIVATE /bigobj)
endif()
if(KBASS_BUILD_TESTS)
    add_executable(KBassProcessorTests Tests/ProcessorTests.cpp)
    target_include_directories(KBassProcessorTests PRIVATE Source "${juce_SOURCE_DIR}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/KBass_artefacts/JuceLibraryCode")
    target_compile_definitions(KBassProcessorTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
    target_link_libraries(KBassProcessorTests PRIVATE KBass)
    add_test(NAME processor-state-and-ui COMMAND KBassProcessorTests)
    set_tests_properties(processor-state-and-ui PROPERTIES TIMEOUT 60)
    add_executable(KBassUserPresetTests Tests/UserPresetsTests.cpp)
    target_include_directories(KBassUserPresetTests PRIVATE Source "${juce_SOURCE_DIR}/modules"
        "${CMAKE_CURRENT_BINARY_DIR}/KBass_artefacts/JuceLibraryCode")
    target_compile_definitions(KBassUserPresetTests PRIVATE JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1)
    target_link_libraries(KBassUserPresetTests PRIVATE KBass)
    add_test(NAME user-presets COMMAND KBassUserPresetTests)
    set_tests_properties(user-presets PROPERTIES TIMEOUT 60)
endif()
