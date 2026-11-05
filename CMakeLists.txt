cmake_minimum_required(VERSION 3.22)
project(AutoMaster VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(AutoMaster
    COMPANY_NAME "YazaAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Yaza
    PLUGIN_CODE Amst
    FORMATS VST3
    PRODUCT_NAME "AutoMaster")

target_sources(AutoMaster PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(AutoMaster PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_options(AutoMaster PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(AutoMaster
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)


#==============================================================================
# VoxChain : chaine voix (EQ, de-esser, compresseur, saturation, delay, reverb)
juce_add_plugin(VoxChain
    COMPANY_NAME "YazaAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Yaza
    PLUGIN_CODE Vxch
    FORMATS VST3
    PRODUCT_NAME "VoxChain")

target_sources(VoxChain PRIVATE
    VoxChain/Source/PluginProcessor.cpp
    VoxChain/Source/PluginEditor.cpp)

target_compile_definitions(VoxChain PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_options(VoxChain PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(VoxChain
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)


#==============================================================================
# YAZA VOX : suite de traitement vocal (COMP, EQ, FX, DLY, REV)
juce_add_plugin(YazaVox
    COMPANY_NAME "YazaAudio"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Yaza
    PLUGIN_CODE Yvox
    FORMATS VST3
    PRODUCT_NAME "YAZA VOX")

target_sources(YazaVox PRIVATE
    YazaVox/Source/PluginProcessor.cpp
    YazaVox/Source/PluginEditor.cpp)

target_compile_definitions(YazaVox PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_compile_options(YazaVox PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(YazaVox
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags)

# Test du moteur (sans interface) : cmake -DAUTOMASTER_BUILD_TESTS=ON
option(AUTOMASTER_BUILD_TESTS "Build engine test" OFF)
if(AUTOMASTER_BUILD_TESTS)
    add_executable(engine_test tests/engine_test.cpp)
    target_include_directories(engine_test PRIVATE Source)
    target_compile_definitions(engine_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(engine_test PRIVATE juce::juce_dsp juce::juce_recommended_config_flags)
endif()
