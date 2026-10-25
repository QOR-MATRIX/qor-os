# QTOS tests. Run with `ctest --preset <preset>`.

find_package(Qt6 ${QTOS_QT_MIN_VERSION} REQUIRED COMPONENTS Test QuickTest)

# QtTest logs through Qt's message handler; on Windows that falls back to
# OutputDebugString when stderr is not a console (as under ctest). Tests never
# make sound (the audio engine renders in real time to its null output), and
# only virtual game controllers exist.
set(QTOS_TEST_ENV "QT_FORCE_STDERR_LOGGING=1;QTOS_AUDIO_OUTPUT=null;QTOS_GAMEPADS=virtual")

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
qtos_add_test(tst_audio SOURCES audio/tst_audio.cpp LIBRARIES qtos_audio)
qtos_add_test(tst_input SOURCES input/tst_input.cpp LIBRARIES qtos_input qtos_kit qtos_kitplugin)

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

# ── Golden-image tests (GPU) ────────────────────────────────────────────────
# Renders golden/scenes in every variant and compares them perceptually with
# golden/references/. `cmake --build --preset <preset> --target update_golden`
# rewrites the references; see docs/DEVELOPMENT.md.
qt_add_executable(tst_golden golden/tst_golden.cpp)
target_link_libraries(tst_golden PRIVATE
    Qt6::Test Qt6::Quick Qt6::QuickControls2
    qtos_kit qtos_kitplugin qtos_lumen_style qtos_lumen_styleplugin qtos::compiler_settings)
target_compile_definitions(tst_golden PRIVATE
    QTOS_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
    QTOS_GOLDEN_OUT="${CMAKE_CURRENT_BINARY_DIR}/golden-out")
set_target_properties(tst_golden PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_golden COMMAND tst_golden)
set_tests_properties(tst_golden PROPERTIES LABELS "gpu" ENVIRONMENT "${QTOS_TEST_ENV}")
add_custom_target(update_golden
    COMMAND ${CMAKE_COMMAND} -E env QTOS_GOLDEN_UPDATE=1 ${QTOS_TEST_ENV} $<TARGET_FILE:tst_golden>
    DEPENDS tst_golden
    COMMENT "Rewriting golden references in tests/golden/references"
    USES_TERMINAL)

find_package(Python3 3.10 COMPONENTS Interpreter)

# ── Shell end-to-end (GPU) ──────────────────────────────────────────────────
# First-party apps driven through qtos-shell --script with synthetic input:
# controls receive clicks, and maximise, snap, minimise, restore, Overview and
# close work. The script asserts on its own ("expect" steps).
add_test(NAME shell_e2e
    COMMAND qtos-shell --unlocked --size 1280x800 --script "${CMAKE_CURRENT_SOURCE_DIR}/shell/apps.e2e.json")
set_tests_properties(shell_e2e PROPERTIES LABELS "gpu" TIMEOUT 120 ENVIRONMENT "${QTOS_TEST_ENV}")

# ── Prism end-to-end (Linux, GPU) ───────────────────────────────────────────
# Real Wayland clients in a nested Prism, driven by `prism --script`:
# decorations, maximise, snap, client moves, minimise and restore, popups,
# Overview and close. Needs a display (Wayland session or WSLg) and, for the
# client-side decoration checks, weston-terminal.
if(TARGET prism)
    find_package(Qt6 ${QTOS_QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
    qt_add_executable(prism-test-client prism/prism_test_client.cpp)
    target_link_libraries(prism-test-client PRIVATE Qt6::Widgets qtos::compiler_settings)
    if(Python3_Interpreter_FOUND)
        add_test(NAME prism_e2e
            COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/dev/prism-e2e.py" --build-dir "${CMAKE_BINARY_DIR}")
        set_tests_properties(prism_e2e PROPERTIES LABELS "gpu" TIMEOUT 180)
    endif()
endif()

# ── Tooling tests (Python) ──────────────────────────────────────────────────
if(Python3_Interpreter_FOUND)
    add_test(NAME tools_generate_tokens
        COMMAND Python3::Interpreter -m unittest -v test_generate_tokens
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools"
    )
endif()
