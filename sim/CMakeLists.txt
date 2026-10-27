# qor-sim — simulated implementations of every QOR OS service.
#
# Compiles the service contracts in services/interfaces into Qt Remote Objects
# *sources* and implements them with scenario-driven behaviour, so the whole
# shell runs on Windows, macOS or Linux without device hardware.

qt_add_library(qor_sim STATIC
    src/carryover.h src/carryover.cpp
    src/simdevices.h src/simdevices.cpp
    src/simsession.h src/simsession.cpp
    src/simservices.h src/simservices.cpp
)

qt_add_repc_sources(qor_sim ${QOR_SERVICE_INTERFACES} ${QOR_SIM_CONTROL_INTERFACE})

qt_add_resources(qor_sim "qor_sim_scenarios"
    PREFIX "/qor/sim"
    FILES
        scenarios/default.json
)

target_include_directories(qor_sim PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(qor_sim
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::RemoteObjects
        qor_audio
    PRIVATE
        Qt6::Concurrent
        qor::compiler_settings
)

if(QOR_BUILD_TOOLS)
    qt_add_executable(qor-simd simd/main.cpp)
    target_link_libraries(qor-simd PRIVATE qor_sim qor::compiler_settings)
    set_target_properties(qor-simd PROPERTIES WIN32_EXECUTABLE OFF)
    install(TARGETS qor-simd RUNTIME DESTINATION bin)
endif()
