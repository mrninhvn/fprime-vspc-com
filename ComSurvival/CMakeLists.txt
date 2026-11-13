####
# ComSurvival: Survival mode 2 beacon rounds over every COM of the deployment,
# driven by command tables the deployment gives (configure)
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/ComSurvival.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/ComSurvival.cpp"
    DEPENDS
        Fw_Types
)
