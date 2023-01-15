project( LibLCC
	VERSION 0.1.0 
	DESCRIPTION "Layout Command and control library")

set(headers
    lcc.h
    lcc-print.h
    lcc-common.h
    lcc-gridconnect.h
)

add_library(lcc STATIC
    ${headers}
    lcc-common-internal.h
    lcc.c
    lcc-print.c
    lcc-gridconnect.c
)
target_compile_definitions(lcc PRIVATE LIBLCC_BUILD)

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
