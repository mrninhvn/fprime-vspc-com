
add_fprime_subdirectory("${CMAKE_CURRENT_LIST_DIR}/VspcComConfig/")

register_fprime_module(
    EXCLUDE_FROM_ALL
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/VspcCom.fpp"
    HEADERS
        "${CMAKE_CURRENT_LIST_DIR}/SubtopologyTopologyDefs.hpp"
        "${CMAKE_CURRENT_LIST_DIR}/PingEntries.hpp"
    DEPENDS
        VspcCom_VspcComConfig
        ApidRouter
    INTERFACE
)
