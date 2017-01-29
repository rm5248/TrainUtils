cmake_minimum_required (VERSION 2.6)

option (BUILD_CABBUS_SNIFFER
        "Build the CabBus sniffer project"
		OFF)
option (BUILD_LOCONET_SNIFFER
        "Build the LocoNet sniffer project"
		OFF)
option (BUILD_CAB2LOCONET_COMPUTER
        "Build the Cab2Loconet program for use with a computer"
		OFF)
		
IF( NOT UNIX )
    SET(CSERIAL_INCLUDE_DIR         "${CMAKE_INSTALL_PREFIX}/include"           CACHE STRING "Directory with CSerial include files")
    SET(CSERIAL_LIBRARIES           "${CMAKE_INSTALL_PREFIX}/lib/cserial.lib"   CACHE STRING "CSerial library to link with")
ENDIF( NOT UNIX )

IF( UNIX )
    find_package(PkgConfig)
    pkg_search_module( CSERIAL REQUIRED cserial )
ENDIF( UNIX )

INCLUDE_DIRECTORIES(${CSERIAL_INCLUDE_DIR})

IF( BUILD_CABBUS_SNIFFER )
    project (CabBusSniffer)
    #include_directories ("${PROJECT_SOURCE_DIR}/CabBusCommunications")
    #add_subdirectory (CabBusCommunications)
	
    # add the executable
    add_executable(CabBusSniffer CabBusSniffer/CabBusSniffer.c ${CABBUS_SOURCES})
    target_link_libraries (CabBusSniffer ${CSERIAL_LIBRARIES})
ENDIF( BUILD_CABBUS_SNIFFER )

IF( BUILD_LOCONET_SNIFFER )
    project (LoconetSniffer)
    include_directories(LocoNetCommunications)
    set( LOCONET_SNIFFER 
         LoconetSniffer/LoconetSniffer.c 
         LocoNetCommunications/loconet_buffer.c
         LocoNetCommunications/loconet_print.c
    )

    IF( UNIX )
        find_package(PkgConfig)
        pkg_search_module( POPT REQUIRED popt )
    ENDIF( UNIX )
	
    add_executable(LoconetSniffer ${LOCONET_SNIFFER})
    target_link_libraries (LoconetSniffer ${CSERIAL_LIBRARIES} ${POPT_LIBRARIES})
ENDIF( BUILD_LOCONET_SNIFFER )

IF( BUILD_CAB2LOCONET_COMPUTER )
    project( Cab2Loconet )
    include_directories( LocoNetCommunications CabBusCommunications )
    set( CAB2LOCONET
        CabBusToLoconetComputer/CabBusToLoconet.c
        LocoNetCommunications/loconet_buffer.c
        LocoNetCommunications/loconet_print.c
        CabBusCommunications/CabBus.c
        )
    
    add_executable( Cab2Loconet ${CAB2LOCONET} )
    target_link_libraries( Cab2Loconet ${CSERIAL_LIBRARIES} )
ENDIF( BUILD_CAB2LOCONET_COMPUTER )
