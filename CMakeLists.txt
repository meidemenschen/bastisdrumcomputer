cmake_minimum_required(VERSION 3.22)

# macOS: Universal Binary (Apple Silicon + Intel), ab macOS 12.0.
# Muss VOR project() stehen. Aeltere Ziele (z.B. 10.13) loesen bei aktuellen
# Xcode/Command-Line-Tools die libc++-Warnung "The selected platform is no
# longer supported by libc++" aus.
if(CMAKE_HOST_APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimale macOS-Version")
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Mac-Architekturen")
endif()

project(BastisDrumcomputer VERSION 0.1.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows/MSVC: statische Laufzeitbibliothek, damit das VST3 ohne
# Visual-C++-Redistributable auf anderen Rechnern laeuft
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ------------------------------------------------------------------------------
# JUCE einbinden
#   a) lokale JUCE-Kopie:  cmake -B build -DJUCE_DIR=/pfad/zu/JUCE
#   b) sonst automatischer Download per FetchContent (getestet mit JUCE 8.0.9)
# ------------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Pfad zu einer lokalen JUCE-Kopie (leer = FetchContent)")

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ------------------------------------------------------------------------------
# Plugin-Formate: VST3 + Standalone (Windows und macOS)
# (AU liesse sich auf dem Mac spaeter einfach mit "AU" ergaenzen)
# ------------------------------------------------------------------------------
set(BASTI_FORMATS VST3 Standalone)

option(BASTI_COPY_PLUGIN_AFTER_BUILD "Plugin nach dem Build in den System-Plugin-Ordner kopieren" OFF)

juce_add_plugin(BastisDrumcomputer
    COMPANY_NAME                "Basti"
    COMPANY_WEBSITE             ""
    BUNDLE_ID                   "com.basti.bastisdrumcomputer"
    PRODUCT_NAME                "Bastis Drumcomputer"
    DESCRIPTION                 "Analog-Style Drum Synthesizer"
    VERSION                     ${PROJECT_VERSION}

    PLUGIN_MANUFACTURER_CODE    Bsti          # 4 Zeichen, mind. ein Grossbuchstabe
    PLUGIN_CODE                 Bdc1          # 4 Zeichen, eindeutig pro Plugin

    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    VST3_CATEGORIES             Instrument Drum

    COPY_PLUGIN_AFTER_BUILD     ${BASTI_COPY_PLUGIN_AFTER_BUILD}
    FORMATS                     ${BASTI_FORMATS}
)

juce_generate_juce_header(BastisDrumcomputer)

target_sources(BastisDrumcomputer
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.cpp
        Source/DSP/DrumEngine.cpp
        Source/DSP/Sequencer.cpp
        Source/UI/DrumLookAndFeel.cpp
        Source/UI/LogoComponent.cpp
        Source/UI/StepGrid.cpp
        Source/UI/InstrumentStrip.cpp
)

target_include_directories(BastisDrumcomputer PRIVATE Source)

# ------------------------------------------------------------------------------
# Optionale Schreibschrift fuer das Logo: jede .ttf/.otf in Resources/Fonts wird
# als BinaryData eingebettet. Ohne Datei faellt das Logo auf eine System-
# Schreibschrift zurueck (siehe LogoComponent.cpp).
# ------------------------------------------------------------------------------
file(GLOB BASTI_FONT_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/*.ttf"
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources/Fonts/*.otf")

if(BASTI_FONT_FILES)
    juce_add_binary_data(BastisDrumcomputerData SOURCES ${BASTI_FONT_FILES})
    target_link_libraries(BastisDrumcomputer PRIVATE BastisDrumcomputerData)
    target_compile_definitions(BastisDrumcomputer PRIVATE BASTI_HAS_EMBEDDED_FONT=1)
endif()

target_compile_definitions(BastisDrumcomputer
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(BastisDrumcomputer
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
