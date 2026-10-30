# QOR OS tests. Run with `ctest --preset <preset>`.

find_package(Qt6 ${QOR_QT_MIN_VERSION} REQUIRED COMPONENTS Test QuickTest)

# QtTest logs through Qt's message handler; on Windows that falls back to
# OutputDebugString when stderr is not a console (as under ctest). Tests never
# make sound (the audio engine renders in real time to its null output), and
# only virtual game controllers exist.
set(QOR_TEST_ENV "QT_FORCE_STDERR_LOGGING=1;QOR_AUDIO_OUTPUT=null;QOR_GAMEPADS=virtual;QOR_TOUCHPAD=off")

# ── C++ unit tests ───────────────────────────────────────────────────────────
function(qor_add_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
    qt_add_executable(${name} ${ARG_SOURCES})
    target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBRARIES} qor::compiler_settings)
    set_target_properties(${name} PROPERTIES WIN32_EXECUTABLE OFF)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${QOR_TEST_ENV}")
endfunction()

qor_add_test(tst_spring SOURCES kit/tst_spring.cpp LIBRARIES qor_kit)
qor_add_test(tst_oklch SOURCES kit/tst_oklch.cpp LIBRARIES qor_kit)
qor_add_test(tst_services SOURCES services/tst_services.cpp LIBRARIES qor_services qor_sim)
qor_add_test(tst_audio SOURCES audio/tst_audio.cpp LIBRARIES qor_audio)
qor_add_test(tst_input SOURCES input/tst_input.cpp LIBRARIES qor_input qor_kit qor_kitplugin Qt6::GuiPrivate)
qor_add_test(tst_store SOURCES store/tst_store.cpp LIBRARIES qor_store qor_storeplugin Qt6::GuiPrivate)
target_compile_definitions(tst_store PRIVATE
    QOR_STORE_CATALOG="${CMAKE_SOURCE_DIR}/store/catalog"
    QOR_STORE_PUB="${CMAKE_SOURCE_DIR}/store/keys/store.pub"
    QOR_STORE_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/store/fixtures")
# Prism's side of the QOR host protocol (prism/shell/producthost.h), against
# a product with no window that tst_producthost_product stands in for.
qt_add_executable(tst_producthost_product prism/producthost_product.cpp)
target_link_libraries(tst_producthost_product PRIVATE Qt6::Core Qt6::Network qor::compiler_settings)
set_target_properties(tst_producthost_product PROPERTIES WIN32_EXECUTABLE OFF)
qor_add_test(tst_producthost SOURCES prism/tst_producthost.cpp LIBRARIES qor_shell Qt6::Network)
target_compile_definitions(tst_producthost PRIVATE QOR_TEST_PRODUCT="$<TARGET_FILE:tst_producthost_product>")
add_dependencies(tst_producthost tst_producthost_product)
# The Library (prism/shell/library.h): the games and media it finds, and what was played.
qor_add_test(tst_library SOURCES prism/tst_library.cpp LIBRARIES qor_shell)
# The QOR core through its C bridge, built from demiurge-chain at the pinned commit (core/, QOR-0003).
if(TARGET qor::core)
    qor_add_test(tst_qorcore SOURCES core/tst_qorcore.cpp LIBRARIES qor::core)
endif()

# Store widgets are QML outside any module: lint them against the built ones.
file(GLOB QOR_STORE_WIDGET_QML CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/store/content/widgets/*/*.qml")
add_test(NAME store_widgets_qmllint
    COMMAND $<TARGET_FILE:Qt6::qmllint> -I "${CMAKE_BINARY_DIR}/qml" ${QOR_STORE_WIDGET_QML})

# ── QML tests (Qt Quick Test) ───────────────────────────────────────────────
qt_add_executable(tst_shell shell/tst_shell.cpp)
target_link_libraries(tst_shell PRIVATE Qt6::QuickTest)
target_compile_definitions(tst_shell PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/shell")
set_target_properties(tst_shell PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_shell COMMAND tst_shell)
set_tests_properties(tst_shell PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;${QOR_TEST_ENV}")

# Kit QML tests render real shaders, so they need a GPU-capable platform.
qt_add_executable(tst_kit_qml kit/tst_kit_qml.cpp)
target_link_libraries(tst_kit_qml PRIVATE Qt6::QuickTest qor_kit qor_kitplugin qor::compiler_settings)
target_compile_definitions(tst_kit_qml PRIVATE QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/kit")
set_target_properties(tst_kit_qml PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_kit_qml COMMAND tst_kit_qml -input "${CMAKE_CURRENT_SOURCE_DIR}/kit/tst_kit.qml")
set_tests_properties(tst_kit_qml PROPERTIES LABELS "gpu" ENVIRONMENT "${QOR_TEST_ENV}")

# ── Golden-image tests (GPU) ────────────────────────────────────────────────
# Renders golden/scenes in every variant and compares them perceptually with
# golden/references/. `cmake --build --preset <preset> --target update_golden`
# rewrites the references; see docs/DEVELOPMENT.md.
qt_add_executable(tst_golden golden/tst_golden.cpp)
target_link_libraries(tst_golden PRIVATE
    Qt6::Test Qt6::Quick Qt6::QuickControls2
    qor_kit qor_kitplugin qor_lumen_style qor_lumen_styleplugin qor::compiler_settings)
target_compile_definitions(tst_golden PRIVATE
    QOR_GOLDEN_DIR="${CMAKE_CURRENT_SOURCE_DIR}/golden"
    QOR_GOLDEN_OUT="${CMAKE_CURRENT_BINARY_DIR}/golden-out")
set_target_properties(tst_golden PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME tst_golden COMMAND tst_golden)
set_tests_properties(tst_golden PROPERTIES LABELS "gpu" ENVIRONMENT "${QOR_TEST_ENV}")
add_custom_target(update_golden
    COMMAND ${CMAKE_COMMAND} -E env QOR_GOLDEN_UPDATE=1 ${QOR_TEST_ENV} $<TARGET_FILE:tst_golden>
    DEPENDS tst_golden
    COMMENT "Rewriting golden references in tests/golden/references"
    USES_TERMINAL)

find_package(Python3 3.10 COMPONENTS Interpreter)

# ── Shell end-to-end (GPU) ──────────────────────────────────────────────────
# First-party apps driven through qor-shell --script with synthetic input:
# controls receive clicks, and maximise, snap, minimise, restore, Overview and
# close work. The script asserts on its own ("expect" steps).
add_test(NAME shell_e2e
    COMMAND qor-shell --unlocked --size 1280x800 --script "${CMAKE_CURRENT_SOURCE_DIR}/shell/apps.e2e.json")
set_tests_properties(shell_e2e PROPERTIES LABELS "gpu" TIMEOUT 240 ENVIRONMENT "${QOR_TEST_ENV}")

# ── Prism end-to-end (Linux, GPU) ───────────────────────────────────────────
# Real Wayland clients in a nested Prism, driven by `prism --script`:
# decorations, maximise, snap, client moves, minimise and restore, popups,
# Overview and close. Needs a display (Wayland session or WSLg) and, for the
# client-side decoration checks, weston-terminal.
if(TARGET prism)
    find_package(Qt6 ${QOR_QT_MIN_VERSION} REQUIRED COMPONENTS Widgets)
    qt_add_executable(prism-test-client prism/prism_test_client.cpp)
    target_link_libraries(prism-test-client PRIVATE Qt6::Widgets qor::compiler_settings)
    if(Python3_Interpreter_FOUND)
        add_test(NAME prism_e2e
            COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/dev/prism-e2e.py" --build-dir "${CMAKE_BINARY_DIR}")
        set_tests_properties(prism_e2e PROPERTIES LABELS "gpu" TIMEOUT 180)
    endif()

    # ── Prism hosting the products (Linux, no display) ──────────────────────
    # Stand-ins for ARQADE, QOR Media, QQ Studio and the QQ Player, started by
    # Prism as a person would and laid out by it. Prism runs offscreen and
    # draws in software, so this needs no display and no graphics card, and
    # shows nothing.
    foreach(product IN ITEMS arqade media qq-studio qq-player)
        qt_add_executable(prism-test-${product} prism/prism_test_product.cpp)
        target_link_libraries(prism-test-${product} PRIVATE Qt6::Gui Qt6::Network qor::compiler_settings)
    endforeach()
    target_compile_definitions(prism-test-arqade PRIVATE PRODUCT_ID="arqade" PRODUCT_NAME="ARQADE")
    target_compile_definitions(prism-test-media PRIVATE PRODUCT_ID="qor-media" PRODUCT_NAME="QOR Media")
    target_compile_definitions(prism-test-qq-studio PRIVATE PRODUCT_ID="qq-studio" PRODUCT_NAME="QQ Studio")
    target_compile_definitions(prism-test-qq-player PRIVATE PRODUCT_ID="qq-player" PRODUCT_NAME="QQ Player")
    if(Python3_Interpreter_FOUND)
        add_test(NAME prism_products
            COMMAND Python3::Interpreter "${CMAKE_SOURCE_DIR}/tools/dev/prism-products.py"
                --prism $<TARGET_FILE:prism>
                --arqade $<TARGET_FILE:prism-test-arqade>
                --media $<TARGET_FILE:prism-test-media>
                --qq-studio $<TARGET_FILE:prism-test-qq-studio>
                --qq-player $<TARGET_FILE:prism-test-qq-player>
                --script "${CMAKE_CURRENT_SOURCE_DIR}/prism/products.e2e.json")
        set_tests_properties(prism_products PROPERTIES TIMEOUT 120)
    endif()
endif()

# ── Tooling tests (Python) ──────────────────────────────────────────────────
if(Python3_Interpreter_FOUND)
    add_test(NAME tools_generate_tokens
        COMMAND Python3::Interpreter -m unittest -v test_generate_tokens
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tools"
    )
endif()
