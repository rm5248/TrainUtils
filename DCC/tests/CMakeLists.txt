add_executable(test-dcc test-dcc.c)
target_link_libraries(test-dcc dcc-util)
target_include_directories(test-dcc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME test-basic-packet COMMAND test-dcc basic-packet )

add_executable(test-packet-parse test-packet-parse.c)
target_link_libraries(test-packet-parse dcc-util)
target_include_directories(test-packet-parse PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME test-short-addr1 COMMAND test-packet-parse short-addr1)
add_test( NAME test-short-addr2 COMMAND test-packet-parse short-addr2)
add_test( NAME accy30-normal COMMAND test-packet-parse accy30-normal)
add_test( NAME accy30-reverse COMMAND test-packet-parse accy30-reverse)
add_test( NAME accy1-normal COMMAND test-packet-parse accy1-normal)
add_test( NAME accy1-reverse COMMAND test-packet-parse accy1-reverse)
