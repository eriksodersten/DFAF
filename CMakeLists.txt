cmake_minimum_required(VERSION 3.22)
project(DFAF VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(/Users/eriksode/JUCE JUCE)

juce_add_plugin(DFAF
    COMPANY_NAME "DFAF"
    PLUGIN_MANUFACTURER_CODE Esod
    PLUGIN_CODE Dfaf
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "DFAF"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
)

juce_generate_juce_header(DFAF)

target_sources(DFAF PRIVATE
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/DecayEnvelope.h
    Source/DFAFSequencer.h
    Source/DFAFVoice.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

juce_add_binary_data(DFAFGuiAssets
    SOURCES
        ig_0b9a34b24e54dfb20169ecc11305388191ac4d2979b55f8ab8.png
)

source_group("Header Files" FILES
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/DecayEnvelope.h
    Source/DFAFSequencer.h
    Source/DFAFVoice.h
    Source/MoogLadderFilter.h
)

source_group("Source Files" FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

source_group("Header Files" FILES
    Source/PluginProcessor.h
    Source/PluginEditor.h
    Source/DecayEnvelope.h
    Source/DFAFSequencer.h
    Source/DFAFVoice.h
)

source_group("Source Files" FILES
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(DFAF PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(DFAF PRIVATE
    DFAFGuiAssets
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
