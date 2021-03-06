cmake_minimum_required (VERSION 3.13)
project( TrainUtils )

option (BUILD_CABBUS_SNIFFER
        "Build the CabBus sniffer project"
		ON)
option (BUILD_LOCONET_SNIFFER
        "Build the LocoNet sniffer project"
		ON)
option (BUILD_CAB2LOCONET_COMPUTER
        "Build the Cab2Loconet program for use with a computer"
		OFF)
		
IF( NOT UNIX )
    SET(CSERIAL_INCLUDE_DIR         "${CMAKE_INSTALL_PREFIX}/include"           CACHE STRING "Directory with CSerial include files")
    SET(CSERIAL_LIBRARIES           "${CMAKE_INSTALL_PREFIX}/lib/cserial.lib"   CACHE STRING "CSerial library to link with")
ENDIF( NOT UNIX )

IF( UNIX )
    include( FindPkgConfig )
    pkg_check_modules( CSERIAL REQUIRED cserial )
ENDIF( UNIX )

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wmissing-prototypes")

add_subdirectory(CabBusCommunications)
add_subdirectory(CabBusSniffer)
add_subdirectory(LocoNetCommunications)
add_subdirectory(LoconetSniffer)

IF( BUILD_CAB2LOCONET_COMPUTER )
    IF( UNIX )
        pkg_check_modules( POPT REQUIRED popt )
    ENDIF( UNIX )

    include_directories( LocoNetCommunications CabBusCommunications )
    
    add_executable( Cab2Loconet  CabBusToLoconetComputer/CabBusToLoconet.c CabBusToLoconetComputer/main_linux.c )
    target_link_libraries( Cab2Loconet PRIVATE cabbus_utils loconet_utils ${POPT_LIBRARIES} ${CSERIAL_LIBRARIES} )
ENDIF( BUILD_CAB2LOCONET_COMPUTER )
