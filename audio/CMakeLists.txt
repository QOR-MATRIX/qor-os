# qtos_audio — the QTOS audio engine.
#
# Plays generative "QTOS Sessions" and music files (QAudioDecoder) through Qt
# Multimedia's real-time output callback, with a five-band equaliser,
# perceptual volume, a soft limiter, and live spectrum / beat analysis for the
# audio-reactive interface. See docs/AUDIO.md.

qt_add_library(qtos_audio STATIC
    src/analyzer.h src/analyzer.cpp
    src/dsp.h src/dsp.cpp
    src/engine.h src/engine.cpp
    src/fft.h src/fft.cpp
    src/filesource.h src/filesource.cpp
    src/library.h src/library.cpp
    src/renderer.h src/renderer.cpp
    src/session.h src/session.cpp
    src/source.h
    src/spsc.h
)

target_include_directories(qtos_audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(qtos_audio
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
    PRIVATE
        qtos::compiler_settings
)
