#
# Tests for the parts of the GUI that don't need a display
#
add_executable(train-gui-test-eventgraph
    test-eventgraph.cpp
    ../lcc/cdi/acdi.cpp
    ../lcc/cdi/cdi.cpp
    ../lcc/cdi/cdilayout.cpp
    ../lcc/cdi/cdivariant.cpp
    ../lcc/cdi/eventidtype.cpp
    ../lcc/cdi/grouptype.cpp
    ../lcc/cdi/identification.cpp
    ../lcc/cdi/inttype.cpp
    ../lcc/cdi/maptype.cpp
    ../lcc/cdi/segment.cpp
    ../lcc/cdi/stringtype.cpp
    ../lcc/eventgraph/eventgraphmodel.cpp
    ../lcc/eventgraph/eventgraphstorage.cpp
)
target_link_libraries(train-gui-test-eventgraph PRIVATE Qt${QT_VERSION_MAJOR}::Core)
target_include_directories(train-gui-test-eventgraph PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

foreach(test_name
        layout-addresses
        pins-from-layout
        event-id-from-bytes
        links-producer
        links-unknown-direction
        links-ignored
        connect-copies-from-source
        connect-producer-wins
        connect-null-source
        connect-rejected
        disconnect
        direction-identified
        direction-observed-overrides
        direction-learned
        storage-round-trip)
    add_test(NAME eventgraph-${test_name} COMMAND train-gui-test-eventgraph ${test_name})
endforeach()
