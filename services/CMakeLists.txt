# QTOS.Services — the shell-facing view of the system services.
#
# services/interfaces/*.rep are the authoritative service contracts. They are
# compiled twice: here into Qt Remote Objects *replicas* (what the shell and
# apps consume) and in sim/ into *sources* (the simulated implementations).

set(QTOS_SERVICE_INTERFACES
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/audio.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/bluetooth.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/display.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/media.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/network.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/notifications.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/power.rep"
    "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/settings.rep"
    CACHE INTERNAL "QTOS service interface definitions"
)
set(QTOS_SIM_CONTROL_INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/interfaces/simcontrol.rep"
    CACHE INTERNAL "Simulation-only control interface"
)

include(QtosRepc)

qt_add_library(qtos_services STATIC)

qtos_add_repc_replicas(qtos_services ${QTOS_SERVICE_INTERFACES})

set_source_files_properties(qml/Intents.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS Intents.qml
)
set_source_files_properties(qml/Reactive.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_RESOURCE_ALIAS Reactive.qml
)

qt_add_qml_module(qtos_services
    URI QTOS.Services
    VERSION 1.0
    RESOURCE_PREFIX /qt/qml
    QML_FILES
        qml/Intents.qml
        qml/Reactive.qml
    SOURCES
        qml/services.h qml/services.cpp
    DEPENDENCIES
        QtQuick
)

target_include_directories(qtos_services PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/qml"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(qtos_services
    PUBLIC
        Qt6::Qml
        Qt6::Quick
        Qt6::RemoteObjects
    PRIVATE
        qtos::compiler_settings
)
