# A QOR OS app built on QOR.Kit and the Lumen style.
#
# Copy this directory, rename the project and URI, then build against a QOR OS
# SDK (or the QOR OS source tree via CMAKE_PREFIX_PATH). See sdk/README.md.
cmake_minimum_required(VERSION 3.27)
project(HelloLumen VERSION 0.1.0 LANGUAGES CXX)

find_package(Qt6 6.12 REQUIRED COMPONENTS Quick QuickControls2)
qt_standard_project_setup(REQUIRES 6.12)

qt_add_executable(hello-lumen main.cpp)

qt_add_qml_module(hello-lumen
    URI Hello.Lumen
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(hello-lumen PRIVATE Qt6::Quick Qt6::QuickControls2)
