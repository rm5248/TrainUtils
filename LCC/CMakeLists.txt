cmake_minimum_required(VERSION 3.15)
project( LibLCC
        VERSION 0.6.0
	DESCRIPTION "Layout Command and control library")

#
# Check our options
#
option( LIBLCC_ENABLE_STATIC_CONTEXT "Enable static contexts.  Generally used on a microcontroller where you only want to have at most once instance" OFF)

# These are the public headers that get installed
# No internal headers should go in this variable
set(headers
    lcc.h
    lcc-print.h
    lcc-common.h
    lcc-gridconnect.h
    simplelogger_defs.h
)

add_library(lcc STATIC
    ${headers}
    lcc-simple.h
    lcc-common-internal.h
    lcc-common-internal.c
    lcc-addressed.h
    lcc-addressed.c
    lcc.c
    lcc-print.c
    lcc-gridconnect.c
    lcc-simple.c
    lcc-network-info.h
    lcc-network-info.c
    lcc-node-info.h
    lcc-node-info.c
    lcc-network-internal.h
    lcc-datagram.c
    lcc-datagram.h
    lcc-memory.c
    lcc-memory.h
    lcc-event.c
    lcc-event.h
    lcc-simple-node-info.c
    lcc-simple-node-info.h
    lcc-remote-memory.c
    lcc-remote-memory.h
)
target_compile_definitions(lcc PRIVATE
    LIBLCC_BUILD
    LIBLCC_MAJOR=${PROJECT_VERSION_MAJOR}
    LIBLCC_MINOR=${PROJECT_VERSION_MINOR}
    LIBLCC_MICRO=${PROJECT_VERSION_PATCH}
    LIBLCC_HAS_CONFIG_H
)
target_include_directories( lcc INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    $<INSTALL_INTERFACE:include/lcc>
)
target_include_directories( lcc PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}
)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

IF( UNIX )
    #
    # pkg-config information
    #
    SET(PKG_CONFIG_LIBDIR
        "\${prefix}/lib"
    )
    SET(PKG_CONFIG_INCLUDEDIR
        "\${prefix}/include"
    )
    SET(PKG_CONFIG_LIBS
        "-L\${libdir} -llcc"
    )
    SET(PKG_CONFIG_CFLAGS
        "-I\${includedir}"
    )

    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/liblcc.pc.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/liblcc.pc"
    )
    INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/liblcc.pc"
             DESTINATION lib/pkgconfig)
ENDIF()

#
# Install Information
#
include(GNUInstallDirs)
if( CMAKE_INSTALL_LIBDIR STREQUAL "" )
    set(CMAKE_INSTALL_LIBDIR "lib")
endif()

#IF( UNIX )
    INSTALL( TARGETS lcc
#             COMPONENT lcc
             EXPORT liblccTargets
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" )
    INSTALL( FILES ${headers} DESTINATION include/lcc )
#ENDIF( UNIX )
#IF( WIN32 )
#    INSTALL( TARGETS lcc
#             COMPONENT lcc
#             EXPORT lccTargets
#             RUNTIME DESTINATION bin
#             LIBRARY DESTINATION lib
#             ARCHIVE DESTINATION lib )
#    INSTALL( FILES ${headers} DESTINATION include/lcc )
#ENDIF( WIN32 )

#
# Support for find_package
#
install(EXPORT liblccTargets
  FILE         liblccConfig.cmake
  NAMESPACE     lcc::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/liblcc
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/liblccConfigVersion.cmake"
  VERSION       ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)
install(FILES   "${CMAKE_CURRENT_BINARY_DIR}/liblccConfigVersion.cmake"
  DESTINATION   ${CMAKE_INSTALL_LIBDIR}/cmake/liblcc
)

#
# Configure our compile options
#
configure_file( lcc-config.h.cmake lcc-config.h )

#
# Print configuration information for the user
#
message(STATUS "")
message(STATUS "")
message(STATUS "LibLCC configuration summary:")
message(STATUS "")

message(STATUS "  Build type ...................... : ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix .................. : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  Library location ................ : ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
message(STATUS "  C compiler ...................... : ${CMAKE_C_COMPILER}")
message(STATUS "  Static context .................. : ${LIBLCC_ENABLE_STATIC_CONTEXT}")
