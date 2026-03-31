cmake_minimum_required(VERSION 3.22)
project(DFAF VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(/Users/eriksode/JUCE JUCE)

juce_add_plugin(DFAF
    COMPANY_NAME "ErikSodersten"
    PLUGIN_MANUFACTURER_CODE Esod
    PLUGIN_CODE Dfaf
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DFAF"
)

juce_generate_juce_header(DFAF)

target_sources(DFAF PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(DFAF PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DFAF PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
