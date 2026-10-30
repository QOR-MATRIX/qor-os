# qor_gamepads — the controllers themselves, without the controller interface.
#
# The backends and GamepadManager from ../src: XInput and raw HID (DualShock 4,
# DualSense, generic HID game pads) on Windows, evdev on Linux, and virtual
# controllers for tests, in one standard layout. Qt Core and Gui only, so the
# products can link it as well as QOR.Input: QQ's Input reads its controllers
# here (products/qq/runtime/CMakeLists.txt adds this folder when built on its
# own). See docs/CONTROLLERS.md.

set(QOR_INPUT_SOURCE "${CMAKE_CURRENT_LIST_DIR}/../src")

qt_add_library(qor_gamepads STATIC
    "${QOR_INPUT_SOURCE}/backends.h"
    "${QOR_INPUT_SOURCE}/gamepad.h"
    "${QOR_INPUT_SOURCE}/gamepadbackend.h"
    "${QOR_INPUT_SOURCE}/gamepadmanager.h" "${QOR_INPUT_SOURCE}/gamepadmanager.cpp"
    "${QOR_INPUT_SOURCE}/mapping.h" "${QOR_INPUT_SOURCE}/mapping.cpp"
    "${QOR_INPUT_SOURCE}/playstation.h" "${QOR_INPUT_SOURCE}/playstation.cpp"
)

if(WIN32)
    target_sources(qor_gamepads PRIVATE "${QOR_INPUT_SOURCE}/backends_windows.cpp")
    target_link_libraries(qor_gamepads PRIVATE hid setupapi cfgmgr32)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(qor_gamepads PRIVATE "${QOR_INPUT_SOURCE}/backends_linux.cpp")
else()
    target_sources(qor_gamepads PRIVATE "${QOR_INPUT_SOURCE}/backends_other.cpp")
endif()

set_target_properties(qor_gamepads PROPERTIES AUTOMOC ON)
target_include_directories(qor_gamepads PUBLIC "${QOR_INPUT_SOURCE}")
target_compile_features(qor_gamepads PUBLIC cxx_std_20)
target_link_libraries(qor_gamepads PUBLIC Qt6::Core Qt6::Gui)
if(TARGET qor::compiler_settings)
    target_link_libraries(qor_gamepads PRIVATE qor::compiler_settings)
endif()
