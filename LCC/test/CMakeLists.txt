add_executable(lcc-test-gridconnect test-gridconnect.c)
target_link_libraries(lcc-test-gridconnect lcc)
target_include_directories(lcc-test-gridconnect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME gridconnect-parse1 COMMAND lcc-test-gridconnect parse1)
add_test( NAME gridconnect-parse2 COMMAND lcc-test-gridconnect parse2)

add_test( NAME gridconnect-encode1 COMMAND lcc-test-gridconnect encode1)
add_test( NAME gridconnect-encode2 COMMAND lcc-test-gridconnect encode2)

add_executable(lcc-test-alias test-alias.c test-common.c)
target_link_libraries(lcc-test-alias lcc)
target_include_directories(lcc-test-alias PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME lcc-alias1 COMMAND lcc-test-alias alias-collision)

add_executable(lcc-test-datagram test-datagram.c test-common.c test-common.h)
target_link_libraries(lcc-test-datagram lcc)
target_include_directories(lcc-test-datagram PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME datagram-test COMMAND lcc-test-datagram datagram )
#add_test( NAME datagram-same-time COMMAND lcc-test-datagram datagram-same-time )

add_executable(lcc-test-events test-events.c)
target_link_libraries(lcc-test-events lcc)
target_include_directories(lcc-test-events PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME decode_2048_1 COMMAND lcc-test-events decode_2048_1 )
add_test( NAME decode_2048_2 COMMAND lcc-test-events decode_2048_2 )
add_test( NAME report_time COMMAND lcc-test-events report_time )
add_test( NAME encode_2048_1 COMMAND lcc-test-events encode_2048_1 )
add_test( NAME encode_2048_2 COMMAND lcc-test-events encode_2048_2 )

#
# Use the olcbchecker application to run these tests
#
add_executable(lcc-test-implementation test-implementation.c)
target_link_libraries(lcc-test-implementation lcc)
target_include_directories(lcc-test-implementation PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")
