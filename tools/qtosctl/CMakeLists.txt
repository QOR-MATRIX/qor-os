# qtosctl — command-line client for the QTOS system services.

qt_add_executable(qtosctl main.cpp)

qt_add_repc_replicas(qtosctl ${QTOS_SERVICE_INTERFACES} ${QTOS_SIM_CONTROL_INTERFACE})

target_include_directories(qtosctl PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(qtosctl PRIVATE
    Qt6::Core
    Qt6::RemoteObjects
    qtos::compiler_settings
)

set_target_properties(qtosctl PROPERTIES WIN32_EXECUTABLE OFF)
install(TARGETS qtosctl RUNTIME DESTINATION bin)
