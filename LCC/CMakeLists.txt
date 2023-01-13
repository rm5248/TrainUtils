include( GNUInstallDirs )

set(lcc_headers
    lcc.h
    lcc-print.h
    lcc-common.h
)

add_library(lcc STATIC
    ${lcc_headers}
    lcc-common-internal.h
    lcc.c
    lcc-print.c
)
target_compile_definitions(lcc PRIVATE LIBLCC_BUILD)

install(FILES ${lcc_headers}
    DESTINATION include/liblcc)
