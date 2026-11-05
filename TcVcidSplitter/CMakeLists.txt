####
# TcVcidSplitter: diverts the uplink TC frames of one virtual channel out of the
# local deframe chain (placed between FrameAccumulator and TcDeframer)
####
register_fprime_module(
    AUTOCODER_INPUTS
        "${CMAKE_CURRENT_LIST_DIR}/TcVcidSplitter.fpp"
    SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/TcVcidSplitter.cpp"
    DEPENDS
        Fw_Types
)
