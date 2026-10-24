# Project-wide compiler settings applied through an interface target.
#
# Every QTOS C++ target links `qtos::compiler_settings` so warnings and Qt
# hygiene defines stay identical across the code base.

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(qtos_compiler_settings INTERFACE)
add_library(qtos::compiler_settings ALIAS qtos_compiler_settings)

target_compile_definitions(qtos_compiler_settings INTERFACE
    QT_NO_CAST_FROM_ASCII
    QT_NO_CAST_TO_ASCII
    QT_NO_URL_CAST_FROM_STRING
    QT_NO_FOREACH
    QT_USE_QSTRINGBUILDER
    QT_DISABLE_DEPRECATED_UP_TO=0x060800
    QTOS_VERSION_STRING="${PROJECT_VERSION}"
)

if(MSVC)
    # Third-party headers (Qt, generated code included with <>) are external:
    # warnings there are not ours to fix. C4702 (unreachable code) fires
    # spuriously on inlined Qt templates in optimised builds.
    target_compile_options(qtos_compiler_settings INTERFACE
        /W4 /permissive- /utf-8 /Zc:__cplusplus /Zc:preprocessor /MP
        /external:anglebrackets /external:W0 /wd4702
    )
    if(QTOS_WARNINGS_AS_ERRORS)
        target_compile_options(qtos_compiler_settings INTERFACE /WX)
    endif()
else()
    target_compile_options(qtos_compiler_settings INTERFACE
        -Wall -Wextra -Wpedantic -Wshadow
        $<$<COMPILE_LANGUAGE:CXX>:-Wnon-virtual-dtor -Wold-style-cast>
    )
    if(QTOS_WARNINGS_AS_ERRORS)
        target_compile_options(qtos_compiler_settings INTERFACE -Werror)
    endif()
endif()
