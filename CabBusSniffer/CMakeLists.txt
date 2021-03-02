IF( BUILD_CABBUS_SNIFFER )
    IF( UNIX )
        pkg_check_modules( POPT REQUIRED popt )
    ENDIF( UNIX )

    # add the executable
    add_executable(CabBusSniffer CabBusSniffer.c )
    target_link_libraries (CabBusSniffer PRIVATE cabbus_utils ${CSERIAL_LIBRARIES})
ENDIF( BUILD_CABBUS_SNIFFER )

