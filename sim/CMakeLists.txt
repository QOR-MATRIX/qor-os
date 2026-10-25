# qtos-sim — simulated implementations of every QTOS service.
#
# Compiles the service contracts in services/interfaces into Qt Remote Objects
# *sources* and implements them with scenario-driven behaviour, so the whole
# shell runs on Windows, macOS or Linux without device hardware.

qt_add_library(qtos_sim STATIC
    src/simdevices.h src/simdevices.cpp
    src/simsession.h src/simsession.cpp
    src/simservices.h src/simservices.cpp
)

qt_add_repc_sources(qtos_sim ${QTOS_SERVICE_INTERFACES} ${QTOS_SIM_CONTROL_INTERFACE})

qt_add_resources(qtos_sim "qtos_sim_scenarios"
    PREFIX "/qtos/sim"
    FILES
        scenarios/default.json
)

target_include_directories(qtos_sim PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)
target_link_libraries(qtos_sim
    PUBLIC
        Qt6::Core
        Qt6::Network
        Qt6::RemoteObjects
        qtos_audio
    PRIVATE
        Qt6::Concurrent
        qtos::compiler_settings
)

if(QTOS_BUILD_TOOLS)
    qt_add_executable(qtos-simd simd/main.cpp)
    target_link_libraries(qtos-simd PRIVATE qtos_sim qtos::compiler_settings)
    set_target_properties(qtos-simd PROPERTIES WIN32_EXECUTABLE OFF)
    install(TARGETS qtos-simd RUNTIME DESTINATION bin)
endif()
