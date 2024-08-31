add_executable(test-dcc test-dcc.c)
target_link_libraries(test-dcc dcc-util)
target_include_directories(test-dcc PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME test-basic-packet COMMAND test-dcc basic-packet )
