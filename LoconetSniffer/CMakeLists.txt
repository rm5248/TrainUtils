project( LoconetSniffer
	VERSION 0.1.0 
	DESCRIPTION "Loconet sniffer")

IF( BUILD_LOCONET_SNIFFER )
    IF( UNIX )
        pkg_check_modules( POPT REQUIRED popt )
    ENDIF( UNIX )
	
    add_executable(LoconetSniffer LoconetSniffer.c)
    target_include_directories(LoconetSniffer PRIVATE ../LocoNetCommunications)
    target_link_libraries (LoconetSniffer PRIVATE loconet_utils ${CSERIAL_LIBRARIES} ${POPT_LIBRARIES})

    install( TARGETS LoconetSniffer RUNTIME DESTINATION bin )
ENDIF( BUILD_LOCONET_SNIFFER )

