add_executable(loconet-test-parse test-loconet.c)
target_link_libraries(loconet-test-parse loconet_utils)
target_include_directories(loconet-test-parse PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/..")

add_test( NAME loconet-parse-speed COMMAND loconet-test-parse parse_speed)
add_test( NAME loconet-parse-sensor COMMAND loconet-test-parse parse_sensor)
add_test( NAME loconet-parse-sensor2 COMMAND loconet-test-parse parse_sensor2)
