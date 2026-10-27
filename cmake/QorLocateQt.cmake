# Locates the Qt 6 installation when the caller did not pass one explicitly.
#
# Resolution order:
#   1. Qt6_DIR or CMAKE_PREFIX_PATH supplied by the caller / IDE kit
#   2. The QOR_QT_DIR environment variable (e.g. C:/Qt/6.12.0/msvc2022_64)
#   3. The default Qt Online Installer location for the pinned version

set(QOR_QT_PINNED_VERSION "6.12.0" CACHE STRING "Qt version used when auto-locating Qt")

if(DEFINED Qt6_DIR OR CMAKE_PREFIX_PATH MATCHES "[Qq]t")
    return()
endif()

if(DEFINED ENV{QOR_QT_DIR})
    list(APPEND CMAKE_PREFIX_PATH "$ENV{QOR_QT_DIR}")
    message(STATUS "QOR OS: using Qt from QOR_QT_DIR=$ENV{QOR_QT_DIR}")
    return()
endif()

if(WIN32)
    set(_qor_candidate "C:/Qt/${QOR_QT_PINNED_VERSION}/msvc2022_64")
elseif(APPLE)
    set(_qor_candidate "$ENV{HOME}/Qt/${QOR_QT_PINNED_VERSION}/macos")
else()
    set(_qor_candidate "$ENV{HOME}/Qt/${QOR_QT_PINNED_VERSION}/gcc_64")
endif()

if(EXISTS "${_qor_candidate}")
    list(APPEND CMAKE_PREFIX_PATH "${_qor_candidate}")
    message(STATUS "QOR OS: auto-located Qt at ${_qor_candidate}")
endif()
unset(_qor_candidate)
