project( LoconetUtils
	VERSION 0.1.0 
	DESCRIPTION "Loconet utilities")

set(PROJECT_NAME "Loconet Utils")

set(headers
  loconet_buffer.h
  loconet_print.h
  loconet_switch.h
  loconet_throttle.h
)
add_library(loconet_utils STATIC
  loconet_buffer.c
  loconet_print.c
  loconet_switch.c
  loconet_throttle.c
  ${headers}
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
        "-L\${libdir} -lloconet_utils"
    )
    SET(PKG_CONFIG_CFLAGS
        "-I\${includedir}"
    )

    CONFIGURE_FILE(
        "${CMAKE_CURRENT_SOURCE_DIR}/loconet.pc.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/loconet.pc"
    )
    INSTALL( FILES "${CMAKE_CURRENT_BINARY_DIR}/loconet.pc"
             DESTINATION lib/pkgconfig)
ENDIF( UNIX )

#
# Install Information
#
SET(install_targets ${install_targets} loconet_utils )
IF( UNIX )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" )
    INSTALL( FILES ${headers} DESTINATION include/loconet )
ENDIF( UNIX )
IF( WIN32 )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib )
    INSTALL( FILES ${headers} DESTINATION include/loconet )
ENDIF( WIN32 )
