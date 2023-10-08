add_executable(lcc-test-gridconnect test-gridconnect.c)
target_link_libraries(lcc-test-gridconnect lcc)
target_include_directories(lcc-test-gridconnect PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME gridconnect-parse1 COMMAND lcc-test-gridconnect parse1)
add_test( NAME gridconnect-parse2 COMMAND lcc-test-gridconnect parse2)

add_test( NAME gridconnect-encode1 COMMAND lcc-test-gridconnect encode1)
add_test( NAME gridconnect-encode2 COMMAND lcc-test-gridconnect encode2)

add_executable(lcc-test-alias test-alias.c)
target_link_libraries(lcc-test-alias lcc)
target_include_directories(lcc-test-alias PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME lcc-alias1 COMMAND lcc-test-alias alias-collision)
