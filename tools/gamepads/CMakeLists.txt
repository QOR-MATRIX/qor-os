# qtos-gamepads — lists game controllers and streams their state.

qt_add_executable(qtos-gamepads main.cpp)
target_link_libraries(qtos-gamepads PRIVATE qtos_input qtos::compiler_settings)
set_target_properties(qtos-gamepads PROPERTIES WIN32_EXECUTABLE OFF)
install(TARGETS qtos-gamepads RUNTIME DESTINATION bin)
