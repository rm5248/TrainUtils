project( LoconetTCP
	VERSION 0.1.0 
	DESCRIPTION "Loconet over TCP utilities")

set(headers
    loconet-tcp.h
)

add_library(loconet-tcp STATIC
    ${headers}
    loconet-tcp.cpp
)

target_include_directories(loconet-tcp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../LocoNetCommunications)

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
        "-L\${libdir} -lloconet-tcp"
    )
    SET(PKG_CONFIG_CFLAGS
        "-I\${includedir}"
    )

    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/loconettcp.pc.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/loconettcp.pc"
    )
    INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/loconettcp.pc"
             DESTINATION lib/pkgconfig)
ENDIF( UNIX )

#
# Install Information
#
SET(install_targets ${install_targets} loconet-tcp )
IF( UNIX )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" )
    INSTALL( FILES ${headers} DESTINATION include/loconet-tcp )
ENDIF( UNIX )
IF( WIN32 )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib )
    INSTALL( FILES ${headers} DESTINATION include/loconet-tcp )
ENDIF( WIN32 )
