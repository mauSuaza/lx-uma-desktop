# Copyright (C) 2026 Erica Synths
# SPDX-License-Identifier: GPL-3.0-only

# LXR V2 application code, split into modules.
#
# The modules call into each other in a cycle (sequencer <-> menu <-> preset
# <-> DSP), so they are OBJECT libraries that are all linked directly into the
# firmware executable rather than static libraries with a link order.

add_library(lxr_app_common INTERFACE)
target_link_libraries(lxr_app_common INTERFACE
    lxr_mcu
    lxr_board_config
    stm32f7_hal
    fatfs
    usb_midi
)
target_compile_options(lxr_app_common INTERFACE $<$<COMPILE_LANGUAGE:C>:-Wall>)

# Startup, clocks, CubeMX peripheral init, interrupt vectors, newlib stubs.
add_library(lxr_platform OBJECT
    Startup/startup_stm32f765vgtx.s
    Src/main.c
    Src/stm32f7xx_it.c
    Src/stm32f7xx_hal_msp.c
    Src/system_stm32f7xx.c
    Src/syscalls.c
    Src/sysmem.c
    Src/delay.c
    Src/flash_if.c
    Src/uma_s7test.c # LX_UMA test 0.0b: sector-7 pattern
)
# Shown on the boot splash screen; comes from project(VERSION) in the top-level CMakeLists.txt.
set_source_files_properties(Src/main.c PROPERTIES
    COMPILE_DEFINITIONS "FIRMWARE_VERSION=\" ${PROJECT_VERSION}\""
)

# Hardware inputs/outputs: shift-register buttons/LEDs, pots & faders.
add_library(lxr_io OBJECT
    Src/IO/adcPots.c
    Src/IO/DIO.c
    Src/IO/uart.c
)

# User interface: LCD, encoder, buttons, LEDs, menu pages, screensaver.
add_library(lxr_ui OBJECT
    Src/lcd.c
    Src/encoder.c
    Src/buttonHandler.c
    Src/ledHandler.c
    Src/graphic.c
    Src/frontPanelParser.c
    Src/Parameters.c
    Src/menu/Cc2Text.c
    Src/menu/copyClearTools.c
    Src/menu/menu.c
    Src/menu/screensaver.c
)

# Kit/pattern/project storage on the SD card.
add_library(lxr_preset OBJECT
    Src/Preset/initKit.c
    Src/Preset/presetManager.c
    Src/uma_sdfiles.c
    Src/uma_enginetest.c # LX_UMA test 0.0c
    Src/uma_sdreadtest.c # LX_UMA test 0.1a
)

# UMA: the pla engine. Plaits' code (Mutable Instruments, MIT, mi/README.md) is included unchanged; it includes its
# files as "plaits/..." and "stmlib/...", from mi/
set(UMA_PLAITS_SOURCES
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/wavetable_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/fm_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine2/six_op_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/fm/algorithms.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/fm/dx_units.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/modal_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/physical_modelling/modal_voice.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/physical_modelling/resonator.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/physical_modelling/string_voice.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/physical_modelling/string.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine2/wave_terrain_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/virtual_analog_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/waveshaping_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine2/phase_distortion_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/additive_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/engine/chord_engine.cc
    Src/Soundengine/DSPAudio/mi/plaits/dsp/chords/chord_bank.cc
    Src/Soundengine/DSPAudio/mi/plaits/resources.cc
    Src/Soundengine/DSPAudio/mi/stmlib/dsp/units.cc
    Src/Soundengine/DSPAudio/mi/stmlib/utils/random.cc
)

# DSP building blocks, drum voices and master effects.
add_library(lxr_dsp OBJECT
    Src/Soundengine/DSPAudio/1PoleLp.c
    Src/Soundengine/DSPAudio/automationNode.c
    Src/Soundengine/DSPAudio/BufferTools.c
    Src/Soundengine/DSPAudio/CymbalVoice.c
    Src/Soundengine/DSPAudio/Decay.c
    Src/Soundengine/DSPAudio/distortion.c
    Src/Soundengine/DSPAudio/dither.c
    Src/Soundengine/DSPAudio/DrumVoice.c
    # UMA engines, copied from LX_UMA (docs/SOURCES.md)
    Src/Soundengine/DSPAudio/FmDrum.c
    Src/Soundengine/DSPAudio/FmWaves.c
    Src/Soundengine/DSPAudio/MdDrum.cpp
    Src/Soundengine/DSPAudio/PlaitsDrum.cpp
    ${UMA_PLAITS_SOURCES}
    Src/Soundengine/DSPAudio/HiHat.c
    Src/Soundengine/DSPAudio/lfo.c
    Src/Soundengine/DSPAudio/mixer.c
    Src/Soundengine/DSPAudio/modulationNode.c
    Src/Soundengine/DSPAudio/Oscillator.c
    Src/Soundengine/DSPAudio/random.c
    Src/Soundengine/DSPAudio/ResonantFilter.c
    Src/Soundengine/DSPAudio/Samples.c
    Src/Soundengine/DSPAudio/SlopeEg2.c
    Src/Soundengine/DSPAudio/snapEg.c
    Src/Soundengine/DSPAudio/Snare.c
    Src/Soundengine/DSPAudio/squareRootLut.c
    Src/Soundengine/DSPAudio/TiltEq.c
    Src/Soundengine/DSPAudio/transientGenerator.c
    Src/Soundengine/DSPAudio/transientTables.c
    Src/Soundengine/DSPAudio/wavetable.c
    Src/Soundengine/DSPAudio/FX/Compressor.c
    Src/Soundengine/DSPAudio/FX/Delay.c
    Src/Soundengine/DSPAudio/FX/Distortion.c
    Src/Soundengine/DSPAudio/FX/FxRamBuffer.c
    Src/Soundengine/DSPAudio/FX/Reverb.c
    Src/Soundengine/DSPAudio/FX/Ringmod.c
    Src/Soundengine/DSPAudio/FX/VerbTools/AllpassDelay.c
    Src/Soundengine/DSPAudio/FX/VerbTools/sinCos.c
    Src/Soundengine/DSPAudio/FX/VerbTools/VerbDelay.c
    Src/Soundengine/DSPAudio/FX/VerbTools/VerbModDelay.c
)

# Audio engine: codec/I2S DMA, sample block rendering, FX bus, clock/trigger I/O.
add_library(lxr_engine OBJECT
    Src/Soundengine/AudioCodecManager.c
    Src/Soundengine/cs4344_cs5343.c
    Src/Soundengine/FIFO.c
    Src/Soundengine/FxBus.c
    Src/Soundengine/Soundengine.c
    Src/Soundengine/TriggerOut.c
)

# MIDI: UART driver, parsers, parameter mapping, voice control.
add_library(lxr_midi OBJECT
    Src/Soundengine/MIDI/CortexParser.c
    Src/Soundengine/MIDI/MidiMessages.c
    Src/Soundengine/MIDI/MidiParser.c
    Src/Soundengine/MIDI/MidiVoiceControl.c
    Src/Soundengine/MIDI/ParameterArray.c
    Src/Soundengine/MIDI/Uart.c
)

# Step sequencer, Euclidean generator, external clock sync.
add_library(lxr_sequencer OBJECT
    Src/Soundengine/Sequencer/clockSync.c
    Src/Soundengine/Sequencer/EuklidGenerator.c
    Src/Soundengine/Sequencer/sequencer.c
)

# USB-MIDI message queue on top of the USB device stack.
add_library(lxr_usb_app OBJECT
    Src/Soundengine/USB/usb_manager.c
)

set(LXR_APP_MODULES
    lxr_platform
    lxr_io
    lxr_ui
    lxr_preset
    lxr_dsp
    lxr_engine
    lxr_midi
    lxr_sequencer
    lxr_usb_app
)
foreach(module IN LISTS LXR_APP_MODULES)
    target_link_libraries(${module} PUBLIC lxr_app_common)
endforeach()
set(LXR_APP_MODULES ${LXR_APP_MODULES} PARENT_SCOPE)

# --- ITCM fit -----------------------------------------------------------------
# Functions tagged IN_ITCM_RAM are linked into the 16 KB instruction TCM. Newer
# GCC releases generate more code at -O3 than the GCC 10/11 the project was
# developed with, which overflows ITCM. LXR_ITCM_OPT overrides the optimisation
# level for just the translation units that contain ITCM code.
set(LXR_ITCM_SOURCES
    Src/Preset/presetManager.c
    Src/Soundengine/cs4344_cs5343.c
    Src/Soundengine/FxBus.c
    Src/Soundengine/DSPAudio/1PoleLp.c
    Src/Soundengine/DSPAudio/CymbalVoice.c
    Src/Soundengine/DSPAudio/Decay.c
    Src/Soundengine/DSPAudio/distortion.c
    Src/Soundengine/DSPAudio/DrumVoice.c
    Src/Soundengine/DSPAudio/HiHat.c
    Src/Soundengine/DSPAudio/lfo.c
    Src/Soundengine/DSPAudio/mixer.c
    Src/Soundengine/DSPAudio/modulationNode.c
    Src/Soundengine/DSPAudio/Oscillator.c
    Src/Soundengine/DSPAudio/ResonantFilter.c
    Src/Soundengine/DSPAudio/SlopeEg2.c
    Src/Soundengine/DSPAudio/Snare.c
    Src/Soundengine/DSPAudio/transientGenerator.c
    Src/Soundengine/DSPAudio/FX/Distortion.c
    Src/Soundengine/DSPAudio/FX/Reverb.c
    Src/Soundengine/MIDI/ParameterArray.c
    Src/Soundengine/Sequencer/sequencer.c
)
if(LXR_ITCM_OPT)
    set_source_files_properties(${LXR_ITCM_SOURCES} PROPERTIES COMPILE_OPTIONS "${LXR_ITCM_OPT}")
endif()
# The SVF filters switch on the filter type inside the per-sample loop. GCC 15
# jump-threads the loop back edge through that switch and duplicates the whole
# loop body per case (ResonantFilter.c: 2772 B -> 6720 B at -O3, 8028 B at -O2).
set_property(SOURCE Src/Soundengine/DSPAudio/ResonantFilter.c APPEND PROPERTY COMPILE_OPTIONS -fno-thread-jumps)

# UMA: the drum-machine engine's machines (OP-XY-MNMD, MIT) are included unchanged; their unused parameters warn
set_property(SOURCE Src/Soundengine/DSPAudio/MdDrum.cpp APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
set_property(SOURCE Src/Soundengine/DSPAudio/MdDrum.cpp APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Src/Soundengine/DSPAudio/mnmd)
# UMA: Plaits includes its files from mi/
set_property(SOURCE Src/Soundengine/DSPAudio/PlaitsDrum.cpp ${UMA_PLAITS_SOURCES} APPEND PROPERTY COMPILE_OPTIONS -Wno-unused-parameter)
set_property(SOURCE Src/Soundengine/DSPAudio/PlaitsDrum.cpp ${UMA_PLAITS_SOURCES} APPEND PROPERTY INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/Src/Soundengine/DSPAudio/mi)
# UMA: card files are loaded on a button press or at start-up, never per block
set_property(SOURCE Src/uma_sdfiles.c APPEND PROPERTY COMPILE_OPTIONS -Os)
# LX_UMA (0.1a, as the Eurorack's 0.33l): the user interface and the inputs are compiled for size, not speed: none of
# it runs per sample, and the firmware has to end below flash sector 7 (STM32F765VGTX_FLASH.ld). MIDI too, except
# ParameterArray.c (ITCM code, LXR_ITCM_SOURCES)
target_compile_options(lxr_ui PRIVATE -Os)
target_compile_options(lxr_io PRIVATE -Os)
set_property(SOURCE Src/Soundengine/MIDI/MidiParser.c Src/Soundengine/MIDI/MidiMessages.c Src/Soundengine/MIDI/Uart.c
	Src/Soundengine/MIDI/CortexParser.c Src/Soundengine/MIDI/MidiVoiceControl.c Src/Preset/initKit.c
	APPEND PROPERTY COMPILE_OPTIONS -Os)

