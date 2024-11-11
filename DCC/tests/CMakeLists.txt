add_executable(test-dcc test-dcc.c)
target_link_libraries(test-dcc dcc-util)
target_include_directories(test-dcc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME test-basic-packet COMMAND test-dcc basic-packet )

add_executable(test-packet-parse test-packet-parse.c)
target_link_libraries(test-packet-parse dcc-util)
target_include_directories(test-packet-parse PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME test-short-addr1 COMMAND test-packet-parse short-addr1)
add_test( NAME test-short-addr2 COMMAND test-packet-parse short-addr2)
