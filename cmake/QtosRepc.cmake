# qtos_add_repc_replicas(<target> <rep files>...)
#
# Generates Qt Remote Objects replica headers like qt_add_repc_replicas(), but
# lets AUTOMOC process them instead of a manual moc step. AUTOMOC emits the
# metatypes JSON that qmltyperegistrar needs, so QML_FOREIGN registrations of
# the replicas carry their full property / method lists — qmllint and the QML
# script compiler can then type-check `Services.network.wifiEnabled`.

function(qtos_add_repc_replicas target)
    foreach(rep IN LISTS ARGN)
        get_filename_component(name "${rep}" NAME_WE)
        set(out "${CMAKE_CURRENT_BINARY_DIR}/rep_${name}_replica.h")
        add_custom_command(
            OUTPUT "${out}"
            COMMAND $<TARGET_FILE:Qt6::repc> -i rep -o replica "${rep}" "${out}"
            DEPENDS "${rep}" Qt6::repc
            COMMENT "Generating Qt Remote Objects replica rep_${name}_replica.h"
            VERBATIM
        )
        set_source_files_properties("${out}" PROPERTIES GENERATED TRUE)
        target_sources(${target} PRIVATE "${out}")
    endforeach()
    target_include_directories(${target} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()
