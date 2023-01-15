project( CabbusUtils 
	VERSION 0.1.0 
	DESCRIPTION "NCE Cab bus utilities")

set(PROJECT_NAME "Cabbus Utils")

set(headers
  CabBus.h
  cabbus_cab.h
  cab_commands.h
)

add_library(cabbus_utils STATIC
  CabBus.c 
  command_to_string.c
  cabbus_cab.c
)

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
        "-L\${libdir} -lcabbus_utils"
    )
    SET(PKG_CONFIG_CFLAGS
        "-I\${includedir}"
    )

    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/cabbus.pc.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/cabbus.pc"
    )
    INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/cabbus.pc"
             DESTINATION lib/pkgconfig)
ENDIF( UNIX )

#
# Install Information
#
SET(install_targets ${install_targets} cabbus_utils )
IF( UNIX )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" )
    INSTALL( FILES ${headers} DESTINATION include/cabbus )
ENDIF( UNIX )
IF( WIN32 )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib )
    INSTALL( FILES ${headers} DESTINATION include/cabbus )
ENDIF( WIN32 )
