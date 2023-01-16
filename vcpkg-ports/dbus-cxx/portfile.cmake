vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO dbus-cxx/dbus-cxx
	REF 2.3.1
	SHA512 51fc9887cd569dd22526be7743ae819bb3a6667dc792a47b57bac36bb328b318a32923c345f71ea41aac4777fcee6c60e9bfcb6f413817b0bcf0ff90325f0bde
	HEAD_REF master
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DENABLE_QT_SUPPORT=ON
)
vcpkg_cmake_install()
file(INSTALL "${SOURCE_PATH}/COPYING" DESTINATION "${CURRENT_PACKAGES_DIR}/share/dbus-cxx" RENAME copyright)
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

