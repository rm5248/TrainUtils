cmake_minimum_required (VERSION 2.6)

option (BUILD_CABBUS_SNIFFER
        "Build the CabBus sniffer project"
		OFF)
option (BUILD_LOCONET_SNIFFER
        "Build the LocoNet sniffer project"
		OFF)
		
SET(CSERIAL_INCLUDE_DIR         "${CMAKE_INSTALL_PREFIX}/include"           CACHE STRING "Directory with CSerial include files")
SET(CSERIAL_LIBRARIES           "${CMAKE_INSTALL_PREFIX}/lib/cserial.lib"   CACHE STRING "CSerial library to link with")

INCLUDE_DIRECTORIES(${CSERIAL_INCLUDE_DIR})

IF( BUILD_CABBUS_SNIFFER )
    project (CabBusSniffer)
	include_directories ("${PROJECT_SOURCE_DIR}/CabBusCommunications")
    add_subdirectory (CabBusCommunications)
	
	# add the executable
	add_executable(CabBusSniffer CabBusSniffer/CabBusSniffer.c ${CABBUS_SOURCES})
    target_link_libraries (CabBusSniffer ${CSERIAL_LIBRARIES})
ENDIF( BUILD_CABBUS_SNIFFER )

IF( BUILD_LOCONET_SNIFFER )
	project (LoconetSniffer)
	include_directories(LocoNetCommunications)
	add_subdirectory(LocoNetCommunications)
	
	add_executable(LoconetSniffer LoconetSniffer/LoconetSniffer.c ${LOCONET_SOURCES})
ENDIF( BUILD_LOCONET_SNIFFER )