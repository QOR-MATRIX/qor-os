# qorctl — command-line client for the QOR OS system services.

qt_add_executable(qorctl main.cpp)

qt_add_repc_replicas(qorctl ${QOR_SERVICE_INTERFACES} ${QOR_SIM_CONTROL_INTERFACE})

target_include_directories(qorctl PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(qorctl PRIVATE
    Qt6::Core
    Qt6::RemoteObjects
    qor_service_endpoint
    qor::compiler_settings
)

set_target_properties(qorctl PROPERTIES WIN32_EXECUTABLE OFF)
install(TARGETS qorctl RUNTIME DESTINATION bin)
