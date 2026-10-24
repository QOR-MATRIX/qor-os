# Locates the Qt 6 installation when the caller did not pass one explicitly.
#
# Resolution order:
#   1. Qt6_DIR or CMAKE_PREFIX_PATH supplied by the caller / IDE kit
#   2. The QTOS_QT_DIR environment variable (e.g. C:/Qt/6.12.0/msvc2022_64)
#   3. The default Qt Online Installer location for the pinned version

set(QTOS_QT_PINNED_VERSION "6.12.0" CACHE STRING "Qt version used when auto-locating Qt")

if(DEFINED Qt6_DIR OR CMAKE_PREFIX_PATH MATCHES "[Qq]t")
    return()
endif()

if(DEFINED ENV{QTOS_QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QTOS_QT_DIR}")
    message(STATUS "QTOS: using Qt from QTOS_QT_DIR=$ENV{QTOS_QT_DIR}")
    return()
endif()

if(WIN32)
    set(_qtos_candidate "C:/Qt/${QTOS_QT_PINNED_VERSION}/msvc2022_64")
elseif(APPLE)
    set(_qtos_candidate "$ENV{HOME}/Qt/${QTOS_QT_PINNED_VERSION}/macos")
else()
    set(_qtos_candidate "$ENV{HOME}/Qt/${QTOS_QT_PINNED_VERSION}/gcc_64")
endif()

if(EXISTS "${_qtos_candidate}")
    list(APPEND CMAKE_PREFIX_PATH "${_qtos_candidate}")
    message(STATUS "QTOS: auto-located Qt at ${_qtos_candidate}")
endif()
unset(_qtos_candidate)
