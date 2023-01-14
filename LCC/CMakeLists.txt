include( GNUInstallDirs )

set(lcc_headers
    lcc.h
    lcc-print.h
    lcc-common.h
    lcc-gridconnect.h
)

add_library(lcc STATIC
    ${lcc_headers}
    lcc-common-internal.h
    lcc.c
    lcc-print.c
    lcc-gridconnect.c
)
target_compile_definitions(lcc PRIVATE LIBLCC_BUILD)

if(BUILD_TESTING)
    add_subdirectory(test)
endif()

install(FILES ${lcc_headers}
    DESTINATION include/liblcc)
