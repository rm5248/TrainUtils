project( LibLCC
	VERSION 0.4.0 
	DESCRIPTION "Layout Command and control library")

# These are the public headers that get installed
# No internal headers should go in this variable
set(headers
    lcc.h
    lcc-print.h
    lcc-common.h
    lcc-gridconnect.h
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
target_compile_definitions(lcc PRIVATE LIBLCC_BUILD LIBLCC_MAJOR=${PROJECT_VERSION_MAJOR} LIBLCC_MINOR=${PROJECT_VERSION_MINOR} LIBLCC_MICRO=${PROJECT_VERSION_PATCH} )

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
ENDIF( UNIX )

#
# Install Information
#
SET(install_targets ${install_targets} lcc )
IF( UNIX )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
             ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" )
    INSTALL( FILES ${headers} DESTINATION include/lcc )
ENDIF( UNIX )
IF( WIN32 )
    INSTALL( TARGETS ${install_targets}
             RUNTIME DESTINATION bin
             LIBRARY DESTINATION lib
             ARCHIVE DESTINATION lib )
    INSTALL( FILES ${headers} DESTINATION include/lcc )
ENDIF( WIN32 )
