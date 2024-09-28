cmake_minimum_required(VERSION 3.15)
project(DCC-util
        VERSION 0.1
	DESCRIPTION "DCC packet utility library")

add_library(dcc-util
    dcc-decoder.c
    dcc-decoder.h
    dcc-packet-parser.h
    dcc-packet-parser.c
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
