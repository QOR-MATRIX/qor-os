# The QOR core: the Rust library that holds the keys and talks to the QOR network, built from demiurge-chain at a
# pinned commit and never copied, so the code that handles keys has one source (QOR-0003). Cargo builds it through
# Corrosion, as the QOR Launcher's Qt shell does upstream, and QOR OS reaches it through its C bridge
# (tools/qor-launcher/core/include/qor_core.h upstream): one JSON request in, one JSON answer out.
#
#   QOR_CORE_COMMIT      the demiurge-chain commit the core is built from; docs/UPSTREAM.md says how to move it
#   QOR_CORE_SOURCE_DIR  a local demiurge-chain checkout to build instead, to change the core and QOR OS together
#   QOR_CORE_FEATURES    the crate's Cargo features: none at the current pin; "desktop" once the pin includes
#                        demiurge-chain's L13.2, which puts the modules it moved behind that feature (ADR-120 there)
#
# The target is qor::core: the static library and the bridge's header.

set(QOR_CORE_REPOSITORY "https://github.com/QOR-MATRIX/demiurge-chain.git"
    CACHE STRING "The repository the QOR core comes from")
set(QOR_CORE_COMMIT "28d9e970fef3bcedb0d0cfbdcf71c74949fed5ec"
    CACHE STRING "The demiurge-chain commit the QOR core is built from (QOR-0003)")
set(QOR_CORE_SOURCE_DIR "" CACHE PATH "A local demiurge-chain checkout to build the core from instead of the pinned commit")
set(QOR_CORE_FEATURES "" CACHE STRING "The QOR core's Cargo features (\"desktop\" once the pin includes L13.2)")

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
    message(FATAL_ERROR "No QOR core at ${qor_core_crate}. Check QOR_CORE_COMMIT or QOR_CORE_SOURCE_DIR.")
endif()

# The Corrosion release the upstream shell uses (tools/qor-launcher/shell/CMakeLists.txt).
FetchContent_Declare(Corrosion
    GIT_REPOSITORY https://github.com/corrosion-rs/corrosion.git
    GIT_TAG v0.6.1
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
