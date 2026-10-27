# qor_audio — the QOR OS audio engine.
#
# Plays generative "QOR Sessions" and music files (QAudioDecoder) through Qt
# Multimedia's real-time output callback, with a five-band equaliser,
# perceptual volume, a soft limiter, and live spectrum / beat analysis for the
# audio-reactive interface. See docs/AUDIO.md.

qt_add_library(qor_audio STATIC
    src/analyzer.h src/analyzer.cpp
    src/backend.h src/backend.cpp
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

target_include_directories(qor_audio PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(qor_audio
    PUBLIC
        Qt6::Core
        Qt6::Multimedia
    PRIVATE
        qor::compiler_settings
)
