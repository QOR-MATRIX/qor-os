# Exploit mitigations for everything QOR OS builds: the OS and, included from their own builds, the products. A bug in
# code that reads what a game, a Wayland client, a controller or a file hands it should end in a crash, not in someone
# else's code running as the person. The flags follow OpenSSF's Compiler Options Hardening Guide and Microsoft's
# recommended linker options; tools/ci/check_hardening.py checks each built program for them, and QOR-0018 says why.
#
# Included once per project, before its add_subdirectory() calls, so every target below gets them. Debug builds leave out
# _FORTIFY_SOURCE, which needs the optimiser. A browser build (Qt for WebAssembly) has its own sandbox and none of these.
#
#   QOR_HARDENING  on by default; off only to compare, never in anything anyone installs
#   QOR_SANITIZE   e.g. "address,undefined" for GCC or Clang: the suites run with them to find what the mitigations only
#                  contain (the linux-asan preset; for the products, the same name in the environment, for
#                  tools/dev/products.sh). Never in anything anyone installs.

include_guard(DIRECTORY)

option(QOR_HARDENING "Build with exploit mitigations (never off in anything anyone installs)" ON)
set(QOR_SANITIZE "$ENV{QOR_SANITIZE}" CACHE STRING "Sanitizers for the suites, e.g. address,undefined (GCC or Clang)")

if(QOR_HARDENING AND NOT EMSCRIPTEN)
    if(MSVC)
        # Control Flow Guard and CET shadow stacks; address randomisation and no execution from data, spelled out so a
        # change of default cannot lose them.
        add_compile_options(/guard:cf /GS)
        add_link_options(/guard:cf /CETCOMPAT /DYNAMICBASE /HIGHENTROPYVA /NXCOMPAT)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # -U first: some distributions' compilers define _FORTIFY_SOURCE themselves, and a redefinition is a warning.
        add_compile_options(
            $<$<NOT:$<CONFIG:Debug>>:-U_FORTIFY_SOURCE>
            $<$<NOT:$<CONFIG:Debug>>:-D_FORTIFY_SOURCE=3>
            -D_GLIBCXX_ASSERTIONS
            -fstack-protector-strong
            -fstack-clash-protection
        )
        if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
            add_compile_options(-fcf-protection=full)
        endif()
        # Qt's platform target compiles position-independent code; this makes executables position-independent too.
        add_link_options(
            LINKER:-z,relro
            LINKER:-z,now
            LINKER:-z,noexecstack
            $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-pie>
        )
    endif()
endif()

if(QOR_SANITIZE)
    if(MSVC)
        message(FATAL_ERROR "QOR_SANITIZE is for GCC and Clang.")
    endif()
    add_compile_options(-fsanitize=${QOR_SANITIZE} -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=${QOR_SANITIZE})
endif()
