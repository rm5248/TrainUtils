include( GNUInstallDirs )

set(locotcp_headers
    loconet-tcp.h
)

add_library(loconet-tcp STATIC
    ${locotcp_headers}
    loconet-tcp.cpp
)

target_include_directories(loconet-tcp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../LocoNetCommunications)

install(FILES ${locotcp_headers}
    DESTINATION include/loconet-tcp)
