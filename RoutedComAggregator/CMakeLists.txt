register_fprime_module(
    AUTOCODER_INPUTS
       "${CMAKE_CURRENT_LIST_DIR}/RoutedComAggregator.fpp"
    SOURCES
       "${CMAKE_CURRENT_LIST_DIR}/RoutedComAggregator.cpp"
    DEPENDS
       Svc_ComAggregator
)
