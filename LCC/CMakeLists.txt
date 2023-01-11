include( GNUInstallDirs )

set(lcc_headers
    lcc.h
    lcc-print.h
    lcc-common.h
)

add_library(lcc-rm STATIC
    ${lcc_headers}
    lcc-common-internal.h
    lcc.c
    lcc-print.c
)
target_compile_definitions(lcc-rm PRIVATE LIBLCC_BUILD)

install(FILES ${lcc_headers}
    DESTINATION include/liblcc-rm)
