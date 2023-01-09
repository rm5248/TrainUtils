include( GNUInstallDirs )

set(lcc_headers
    lcc.h
)

add_library(lcc-rm STATIC
    ${lcc_headers}
    lcc.c
)

install(FILES ${lcc_headers}
    DESTINATION include/liblcc-rm)
