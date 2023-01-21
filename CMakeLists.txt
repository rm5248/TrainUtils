cmake_minimum_required (VERSION 3.13)
project( TrainUtils )

include( CTest )
include( GNUInstallDirs )

option (BUILD_CABBUS_SNIFFER
        "Build the CabBus sniffer project"
		ON)
option (BUILD_LOCONET_SNIFFER
        "Build the LocoNet sniffer project"
		ON)
option (BUILD_CAB2LOCONET_COMPUTER
        "Build the Cab2Loconet program for use with a computer"
		OFF)
option (BUILD_GUI
	"Build the Train GUI"
	OFF)

# Request C++17, if available
# This *should* fallback to an older standard if it is not available
if( NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 17)
endif()
		
IF( NOT UNIX )
    SET(CSERIAL_INCLUDE_DIR         "${CMAKE_INSTALL_PREFIX}/include"           CACHE STRING "Directory with CSerial include files")
    SET(CSERIAL_LIBRARIES           "${CMAKE_INSTALL_PREFIX}/lib/cserial.lib"   CACHE STRING "CSerial library to link with")
ENDIF( NOT UNIX )

IF( UNIX )
    include( FindPkgConfig )
    pkg_check_modules( CSERIAL REQUIRED cserial )
ENDIF( UNIX )

if("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes -Wmissing-prototypes")
endif()

#set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} \
#    -fsanitize=address \
#    -fsanitize=null \
#    -fsanitize=return \
#    -fsanitize=object-size \
#    -fsanitize=bool \
#    -fsanitize=enum" )
#set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
#    -fsanitize=address \
#    -fsanitize=null \
#    -fsanitize=return \
#    -fsanitize=object-size \
#    -fsanitize=bool \
#    -fsanitize=enum" )

add_subdirectory(CabBusCommunications)
add_subdirectory(CabBusSniffer)
add_subdirectory(LocoNetCommunications)
add_subdirectory(LoconetSniffer)
add_subdirectory(LoconetTCPUtil)
add_subdirectory(LCC)

if(BUILD_GUI)
	add_subdirectory(GUI)
endif()

IF( BUILD_CAB2LOCONET_COMPUTER )
    IF( UNIX )
        pkg_check_modules( POPT REQUIRED popt )
    ENDIF( UNIX )

    include_directories( LocoNetCommunications CabBusCommunications )
    
    add_executable( Cab2Loconet  CabBusToLoconetComputer/CabBusToLoconet.c CabBusToLoconetComputer/main_linux.c )
    target_link_libraries( Cab2Loconet PRIVATE cabbus_utils loconet_utils ${POPT_LIBRARIES} ${CSERIAL_LIBRARIES} )
ENDIF( BUILD_CAB2LOCONET_COMPUTER )

if(UNIX)
  install(FILES udev-rules/50-digitrax.rules
          DESTINATION /lib/udev/rules.d)
endif(UNIX)

#
# Output configuration information
#
message(STATUS "")
message(STATUS "")
message(STATUS "Train Utils configuration summary:")
message(STATUS "")

message(STATUS "  C++ standard: ................... : ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Cabbus comms: ............. : ON")
message(STATUS "  Build Cabbus sniffer: ........... : ${BUILD_CABBUS_SNIFFER}")
message(STATUS "  Build Loconet comms: ............ : ON")
message(STATUS "  Build Loconet sniffer: .......... : ${BUILD_LOCONET_SNIFFER}")
message(STATUS "  Build Loconet TCP util: ......... : ON")
message(STATUS "  Build Cab2Loconet(Computer): .... : ${BUILD_CAB2LOCONET_COMPUTER}")
message(STATUS "  Build liblcc: ................... : ON")
message(STATUS "  Build GUI: ...................... : ${BUILD_GUI}")
