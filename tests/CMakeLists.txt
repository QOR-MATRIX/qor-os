# QTOS tests. Run with `ctest --preset <preset>`.

find_package(Qt6 ${QTOS_QT_MIN_VERSION} REQUIRED COMPONENTS Test QuickTest)

# QtTest logs through Qt's message handler; on Windows that falls back to
# OutputDebugString when stderr is not a console (as under ctest).
set(QTOS_TEST_ENV "QT_FORCE_STDERR_LOGGING=1")

# ── C++ unit tests ───────────────────────────────────────────────────────────
function(qtos_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBRARIES} qtos::compiler_settings)
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE OFF)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${QTOS_TEST_ENV}")
endfunction()

qtos_add_test(tst_spring SOURCES kit/tst_spring.cpp LIBRARIES qtos_kit)
qtos_add_test(tst_oklch SOURCES kit/tst_oklch.cpp LIBRARIES qtos_kit)
qtos_add_test(tst_services SOURCES services/tst_services.cpp LIBRARIES qtos_services qtos_sim)

# ── QML tests (Qt Quick Test) ───────────────────────────────────────────────
qt_add_executable(tst_shell shell/tst_shell.cpp)
target_link_libraries(tst_shell PRIVATE Qt6::QuickTest)
target_compile_definitions(tst_shell PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shell")
set_target_properties(tst_shell PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_shell COMMAND tst_shell)
set_tests_properties(tst_shell PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${QTOS_TEST_ENV}")

# Kit QML tests render real shaders, so they need a GPU-capable platform.
qt_add_executable(tst_kit_qml kit/tst_kit_qml.cpp)
target_link_libraries(tst_kit_qml PRIVATE Qt6::QuickTest qtos_kit qtos_kitplugin qtos::compiler_settings)
target_compile_definitions(tst_kit_qml PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/kit")
set_target_properties(tst_kit_qml PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_kit_qml COMMAND tst_kit_qml -input "${CMAKE_CURRENT_SOURCE_DIR}/kit/tst_kit.qml")
set_tests_properties(tst_kit_qml PROPERTIES LABELS "gpu" ENVIRONMENT "${QTOS_TEST_ENV}")

# ── Tooling tests (Python) ──────────────────────────────────────────────────
find_package(Python3 3.10 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_test(NAME tools_generate_tokens
        COMMAND Python3::Interpreter -m unittest -v test_generate_tokens
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools"
    )
endif()
