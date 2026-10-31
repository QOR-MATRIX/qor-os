# The QOR core: the Rust library that holds the keys and talks to the QOR network, built from demiurge-chain at a
# pinned commit and never copied, so the code that handles keys has one source (QOR-0003). Cargo builds it through
# Corrosion, as the QOR Launcher's Qt shell does upstream, and QOR OS reaches it through its C bridge
# (tools/qor-launcher/core/include/qor_core.h upstream): one JSON request in, one JSON answer out.
#
#   QOR_CORE_COMMIT      the pin: the demiurge-chain commit the core is built from; docs/UPSTREAM.md says how to move it
#   QOR_CORE_FEATURES    the crate's Cargo features: "desktop", every module demiurge-chain's L13.2 moved into the core
#                        (QOR ID's and Social's clients, the chain client, the Library's index, the Mesh and Qontrol),
#                        as the Windows launcher builds it (ADR-120 there)
#   QOR_CORE_SOURCE_DIR  a local demiurge-chain checkout to build instead: to change the core and QOR OS together, or
#                        to try another commit before the pin moves to it
#
# The pin and the features are written here on every configure, never kept from an earlier one, so a build folder
# configured at an older pin follows a new one; CI reuses its build folders. The target is qor::core: the static
# library and the bridge's header.

set(QOR_CORE_REPOSITORY "https://github.com/QOR-MATRIX/demiurge-chain.git"
    CACHE STRING "The repository the QOR core comes from")
set(qor_core_cached "$CACHE{QOR_CORE_COMMIT}")
set(QOR_CORE_COMMIT "6a4822d10c93694d6d12abf33a75b4df6cac918a" CACHE INTERNAL "The pin (QOR-0003)")
set(QOR_CORE_FEATURES "desktop" CACHE INTERNAL "The QOR core's Cargo features")
set(QOR_CORE_SOURCE_DIR "" CACHE PATH "A local demiurge-chain checkout to build the core from instead of the pin")
if(qor_core_cached AND NOT qor_core_cached STREQUAL QOR_CORE_COMMIT)
    message(STATUS "QOR core: the pin moved to ${QOR_CORE_COMMIT} (this build folder had ${qor_core_cached})")
endif()

include(FetchContent)

if(QOR_CORE_SOURCE_DIR)
    set(qor_core_root "${QOR_CORE_SOURCE_DIR}")
    message(STATUS "QOR core: from ${qor_core_root} (QOR_CORE_SOURCE_DIR), not the pinned commit")
else()
    # A full clone: a commit cannot be fetched shallowly by its hash. Git checks the content against the hash, so the
    # pin is exact. SOURCE_SUBDIR names a folder without a CMakeLists.txt, so nothing upstream joins this build.
    FetchContent_Declare(demiurge_chain
        GIT_REPOSITORY "${QOR_CORE_REPOSITORY}"
        GIT_TAG "${QOR_CORE_COMMIT}"
        SOURCE_SUBDIR tools/qor-launcher/core
    )
    FetchContent_MakeAvailable(demiurge_chain)
    set(qor_core_root "${demiurge_chain_SOURCE_DIR}")
    message(STATUS "QOR core: demiurge-chain at ${QOR_CORE_COMMIT}")
endif()

set(qor_core_crate "${qor_core_root}/tools/qor-launcher/core")
if(NOT EXISTS "${qor_core_crate}/Cargo.toml")
    message(FATAL_ERROR "No QOR core at ${qor_core_crate}. Check the pin (QOR_CORE_COMMIT) or QOR_CORE_SOURCE_DIR.")
endif()

# The Corrosion release the upstream shell uses (tools/qor-launcher/shell/CMakeLists.txt): v0.6.1, by its commit, so a
# moved tag cannot change what runs in this build.
FetchContent_Declare(Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG 1499b14e4906a2890f5cee1547c8848db261753d
)
FetchContent_MakeAvailable(Corrosion)

# LOCKED: exactly the dependency versions in the core's Cargo.lock at the pinned commit.
set(qor_core_features)
if(QOR_CORE_FEATURES)
    set(qor_core_features FEATURES ${QOR_CORE_FEATURES})
endif()
corrosion_import_crate(MANIFEST_PATH "${qor_core_crate}/Cargo.toml"
    CRATES qor-launcher-core
    CRATE_TYPES staticlib
    LOCKED
    ${qor_core_features}
)

add_library(qor_core INTERFACE)
add_library(qor::core ALIAS qor_core)
target_link_libraries(qor_core INTERFACE qor_launcher_core)
target_include_directories(qor_core INTERFACE "${qor_core_crate}/include")
