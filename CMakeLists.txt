# QTOS — top-level build
#
# Builds the Lumen kit, the simulated system services, the Prism shell, the
# first-party apps and the developer tools. See docs/BUILDING.md.

cmake_minimum_required(VERSION 3.27)

project(QTOS
    VERSION 0.1.0
    DESCRIPTION "A next-generation operating system experience built entirely with Qt 6"
    HOMEPAGE_URL "https://github.com/QOR-MATRIX/QTOS"
    LANGUAGES CXX
)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(QtosLocateQt)

option(QTOS_BUILD_TESTS "Build unit and QML tests" ON)
option(QTOS_BUILD_TOOLS "Build developer tools (qtosctl, qtos-simd)" ON)
option(QTOS_BUILD_PLATFORM "Build the QPA platform theme and Lumen QStyle plugins" ON)
option(QTOS_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    option(QTOS_BUILD_COMPOSITOR "Build the Prism Wayland compositor (Linux only)" ON)
else()
    set(QTOS_BUILD_COMPOSITOR OFF CACHE BOOL "Build the Prism Wayland compositor (Linux only)" FORCE)
endif()

set(QTOS_QT_MIN_VERSION 6.12)

# GuiPrivate (QWindowSystemInterface for the shell's --script runner, the QPA
# platform theme) is found here, not in a subdirectory: its dependencies (XKB on
# Linux) must be visible to every executable that links it indirectly. Private
# Qt API is used knowingly and pinned to the Qt version above.
set(QT_NO_PRIVATE_MODULE_WARNING ON)
find_package(Qt6 ${QTOS_QT_MIN_VERSION} REQUIRED COMPONENTS
    Concurrent
    Core
    Gui
    GuiPrivate
    Network
    Qml
    Quick
    QuickControls2
    QuickEffects
    QuickLayouts
    QuickShapes
    RemoteObjects
    ShaderTools
    Svg
    Graphs
    Multimedia
)

# The Store verifies signatures with the platform's crypto: CNG on Windows,
# OpenSSL's libcrypto elsewhere. Found here so every target that links
# qtos_store sees the imported target.
if(NOT WIN32)
    find_package(OpenSSL 3 REQUIRED COMPONENTS Crypto)
endif()

qt_standard_project_setup(REQUIRES ${QTOS_QT_MIN_VERSION})

# All QML modules land in one import directory so qmllint, qmlls and the
# Qt Quick tooling see the same module layout the runtime does.
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")
# .qmlls.ini files are written into the source tree and point at this build.
# Turn this off for secondary builds of the same checkout (for example a WSL
# build of a Windows working tree) so they do not repoint the editor.
set(QT_QML_GENERATE_QMLLS_INI ON CACHE BOOL "Write .qmlls.ini files for qmlls into the source tree")

include(QtosCompilerSettings)

add_subdirectory(kit)
add_subdirectory(audio)
add_subdirectory(input)
add_subdirectory(services)
add_subdirectory(store)
add_subdirectory(sim)
add_subdirectory(apps)
add_subdirectory(prism)

if(QTOS_BUILD_TOOLS)
    add_subdirectory(tools/qtosctl)
    add_subdirectory(tools/gamepads)
endif()

if(QTOS_BUILD_PLATFORM)
    add_subdirectory(platform)
endif()

if(QTOS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

include(QtosSummary)
