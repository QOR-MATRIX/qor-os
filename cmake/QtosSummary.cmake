# Prints a short configuration summary at the end of configure.

message(STATUS "")
message(STATUS "QTOS ${PROJECT_VERSION} configuration")
message(STATUS "  Qt              ${Qt6_VERSION} (${Qt6_DIR})")
message(STATUS "  Build type      ${CMAKE_BUILD_TYPE}")
message(STATUS "  Tests           ${QTOS_BUILD_TESTS}")
message(STATUS "  Tools           ${QTOS_BUILD_TOOLS}")
message(STATUS "  Platform        ${QTOS_BUILD_PLATFORM}")
message(STATUS "  Compositor      ${QTOS_BUILD_COMPOSITOR}")
message(STATUS "  QML output dir  ${QT_QML_OUTPUT_DIRECTORY}")
message(STATUS "")
