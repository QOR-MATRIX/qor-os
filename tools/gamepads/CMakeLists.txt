# qor-gamepads — lists game controllers and streams their state.

qt_add_executable(qor-gamepads main.cpp)
target_link_libraries(qor-gamepads PRIVATE qor_input qor::compiler_settings)
set_target_properties(qor-gamepads PROPERTIES WIN32_EXECUTABLE OFF)
install(TARGETS qor-gamepads RUNTIME DESTINATION bin)
